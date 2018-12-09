/*
 * Copyright 2011-2016, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "IMAPConnectionWorker.h"

#include <Autolock.h>
#include <Messenger.h>

#include <AutoDeleter.h>

#include "IMAPFolder.h"
#include "IMAPMailbox.h"
#include "IMAPProtocol.h"


using IMAP::MessageUIDList;


static const uint32 kMaxFetchEntries = 500;
static const uint32 kMaxDirectDownloadSize = 4096;


class WorkerPrivate {
public:
	WorkerPrivate(IMAPConnectionWorker& worker)
		:
		fWorker(worker)
	{
	}

	IMAP::Protocol& Protocol()
	{
		return fWorker.fProtocol;
	}

	status_t AddFolders(BObjectList<IMAPFolder>& folders)
	{
		IMAPConnectionWorker::MailboxMap::iterator iterator
			= fWorker.fMailboxes.begin();
		for (; iterator != fWorker.fMailboxes.end(); iterator++) {
			IMAPFolder* folder = iterator->first;
			if (!folders.AddItem(folder))
				return B_NO_MEMORY;
		}
		return B_OK;
	}

	status_t SelectMailbox(IMAPFolder& folder)
	{
		return fWorker._SelectMailbox(folder, NULL);
	}

	status_t SelectMailbox(IMAPFolder& folder, uint32& nextUID)
	{
		return fWorker._SelectMailbox(folder, &nextUID);
	}

	IMAPMailbox* MailboxFor(IMAPFolder& folder)
	{
		return fWorker._MailboxFor(folder);
	}

	int32 BodyFetchLimit() const
	{
		return fWorker.fSettings.BodyFetchLimit();
	}

	uint32 MessagesExist() const
	{
		return fWorker._MessagesExist();
	}

	status_t EnqueueCommand(WorkerCommand* command)
	{
		return fWorker._EnqueueCommand(command);
	}

	void SyncCommandDone()
	{
		fWorker._SyncCommandDone();
	}

	void Quit()
	{
		fWorker.fStopped = true;
	}

private:
	IMAPConnectionWorker&	fWorker;
};


class WorkerCommand {
public:
	WorkerCommand()
		:
		fContinuation(false)
	{
	}

	virtual ~WorkerCommand()
	{
	}

	virtual	status_t Process(IMAPConnectionWorker& worker) = 0;

	virtual bool IsDone() const
	{
		return true;
	}

	bool IsContinuation() const
	{
		return fContinuation;
	}

	void SetContinuation()
	{
		fContinuation = true;
	}

private:
			bool				fContinuation;
};


/*!	All commands that inherit from this class will automatically maintain the
	worker's fSyncPending member, and will thus prevent syncing more than once
	concurrently.
*/
class SyncCommand : public WorkerCommand {
};


class QuitCommand : public WorkerCommand {
public:
	QuitCommand()
	{
	}

	virtual status_t Process(IMAPConnectionWorker& worker)
	{
		WorkerPrivate(worker).Quit();
		return B_OK;
	}
};


class CheckSubscribedFoldersCommand : public WorkerCommand {
public:
	virtual status_t Process(IMAPConnectionWorker& worker)
	{
		IMAP::Protocol& protocol = WorkerPrivate(worker).Protocol();

		// The main worker checks the subscribed folders, and creates
		// other workers as needed
		return worker.Owner().CheckSubscribedFolders(protocol,
			worker.UsesIdle());
	}
};


class FetchBodiesCommand : public SyncCommand, public IMAP::FetchListener {
public:
	FetchBodiesCommand(IMAPFolder& folder, IMAPMailbox& mailbox,
		MessageUIDList& entries, const BMessenger* replyTo = NULL)
		:
		fFolder(folder),
		fMailbox(mailbox),
		fEntries(entries)
	{
		folder.RegisterPendingBodies(entries, replyTo);
	}

	virtual status_t Process(IMAPConnectionWorker& worker)
	{
		IMAP::Protocol& protocol = WorkerPrivate(worker).Protocol();

		if (fEntries.empty())
			return B_OK;

		fUID = *fEntries.begin();
		fEntries.erase(fEntries.begin());

		status_t status = WorkerPrivate(worker).SelectMailbox(fFolder);
		if (status == B_OK) {
			printf("IMAP: fetch body for %" B_PRIu32 "\n", fUID);
			// Since RFC3501 does not specify whether the FETCH response may
			// alter the order of the message data items we request, we cannot
			// request more than a single UID at a time, or else we may not be
			// able to assign the data to the correct message beforehand.
			IMAP::FetchCommand fetch(fUID, fUID, IMAP::kFetchBody);
			fetch.SetListener(this);

			status = protocol.ProcessCommand(fetch);
		}
		if (status == B_OK)
			status = fFetchStatus;
		if (status != B_OK)
			fFolder.StoringBodyFailed(fRef, fUID, status);

		return status;
	}

	virtual bool IsDone() const
	{
		return fEntries.empty();
	}

	virtual bool FetchData(uint32 fetchFlags, BDataIO& stream, size_t& length)
	{
		fFetchStatus = fFolder.StoreBody(fUID, stream, length, fRef, fFile);
		return true;
	}

	virtual void FetchedData(uint32 fetchFlags, uint32 uid, uint32 flags)
	{
		if (fFetchStatus == B_OK)
			fFolder.BodyStored(fRef, fFile, uid);
	}

private:
	IMAPFolder&				fFolder;
	IMAPMailbox&			fMailbox;
	MessageUIDList			fEntries;
	uint32					fUID;
	entry_ref				fRef;
	BFile					fFile;
	status_t				fFetchStatus;
};


class FetchHeadersCommand : public SyncCommand, public IMAP::FetchListener {
public:
	FetchHeadersCommand(IMAPFolder& folder, IMAPMailbox& mailbox,
		MessageUIDList& uids, int32 bodyFetchLimit)
		:
		fFolder(folder),
		fMailbox(mailbox),
		fUIDs(uids),
		fBodyFetchLimit(bodyFetchLimit)
	{
	}

	virtual status_t Process(IMAPConnectionWorker& worker)
	{
		IMAP::Protocol& protocol = WorkerPrivate(worker).Protocol();

		status_t status = WorkerPrivate(worker).SelectMailbox(fFolder);
		if (status != B_OK)
			return status;

		printf("IMAP: fetch %" B_PRIuSIZE "u headers\n", fUIDs.size());

		IMAP::FetchCommand fetch(fUIDs, kMaxFetchEntries,
			IMAP::kFetchHeader | IMAP::kFetchFlags);
		fetch.SetListener(this);

		status = protocol.ProcessCommand(fetch);
		if (status != B_OK)
			return status;

		if (IsDone() && !fFetchBodies.empty()) {
			// Enqueue command to fetch the message bodies
			WorkerPrivate(worker).EnqueueCommand(new FetchBodiesCommand(fFolder,
				fMailbox, fFetchBodies));
		}

		return B_OK;
	}

	virtual bool IsDone() const
	{
		return fUIDs.empty();
	}

	virtual bool FetchData(uint32 fetchFlags, BDataIO& stream, size_t& length)
	{
		fFetchStatus = fFolder.StoreMessage(fetchFlags, stream, length,
			fRef, fFile);
		return true;
	}

	virtual void FetchedData(uint32 fetchFlags, uint32 uid, uint32 flags)
	{
		if (fFetchStatus == B_OK) {
			fFolder.MessageStored(fRef, fFile, fetchFlags, uid, flags);

			uint32 size = fMailbox.MessageSize(uid);
			if (fBodyFetchLimit < 0 || size < fBodyFetchLimit)
				fFetchBodies.push_back(uid);
		}
	}

private:
	IMAPFolder&				fFolder;
	IMAPMailbox&			fMailbox;
	MessageUIDList			fUIDs;
	MessageUIDList			fFetchBodies;
	uint32					fBodyFetchLimit;
	entry_ref				fRef;
	BFile					fFile;
	status_t				fFetchStatus;
};


class CheckMailboxesCommand : public SyncCommand {
public:
	CheckMailboxesCommand(IMAPConnectionWorker& worker)
		:
		fWorker(worker),
		fFolders(5, false),
		fState(INIT),
		fFolder(NULL),
		fMailbox(NULL)
	{
	}

	virtual status_t Process(IMAPConnectionWorker& worker)
	{
		IMAP::Protocol& protocol = WorkerPrivate(worker).Protocol();

		if (fState == INIT) {
			// Collect folders
			status_t status = WorkerPrivate(worker).AddFolders(fFolders);
			if (status != B_OK || fFolders.IsEmpty()) {
				fState = DONE;
				return status;
			}

			fState = SELECT;
		}

		if (fState == SELECT) {
			// Get next mailbox from list, and select it
			fFolder = fFolders.RemoveItemAt(fFolders.CountItems() - 1);
			if (fFolder == NULL) {
				for (int32 i = 0; i < fFetchCommands.CountItems(); i++) {
					WorkerPrivate(worker).EnqueueCommand(
						fFetchCommands.ItemAt(i));
				}

				fState = DONE;
				return B_OK;
			}

			fMailbox = WorkerPrivate(worker).MailboxFor(*fFolder);

			status_t status = WorkerPrivate(worker).SelectMailbox(*fFolder);
			if (status != B_OK)
				return status;

			fLastIndex = WorkerPrivate(worker).MessagesExist();
			fFirstIndex = fMailbox->CountMessages() + 1;
			if (fLastIndex > 0)
				fState = FETCH_ENTRIES;
		}

		if (fState == FETCH_ENTRIES) {
			status_t status = WorkerPrivate(worker).SelectMailbox(*fFolder);
			if (status != B_OK)
				return status;

			uint32 to = fLastIndex;
			uint32 from = fFirstIndex + kMaxFetchEntries < to
				? fLastIndex - kMaxFetchEntries : fFirstIndex;

			printf("IMAP: get entries from %" B_PRIu32 " to %" B_PRIu32 "\n",
				from, to);

			IMAP::MessageEntryList entries;
			IMAP::FetchMessageEntriesCommand fetch(entries, from, to, false);
			status = protocol.ProcessCommand(fetch);
			if (status != B_OK)
				return status;

			// Determine how much we need to download
			// TODO: also retrieve the header size, and only take the body
			// size into account if it's below the limit -- that does not
			// seem to be possible, though
			for (size_t i = 0; i < entries.size(); i++) {
				printf("%10" B_PRIu32 " %8" B_PRIu32 " bytes, flags: %#"
					B_PRIx32 "\n", entries[i].uid, entries[i].size,
					entries[i].flags);
				fMailbox->AddMessageEntry(from + i, entries[i].uid,
					entries[i].flags, entries[i].size);

				if (entries[i].uid > fFolder->LastUID()) {
					fTotalBytes += entries[i].size;
					fUIDsToFetch.push_back(entries[i].uid);
				} else {
					fFolder->SyncMessageFlags(entries[i].uid, entries[i].flags);
				}
			}

			fTotalEntries += fUIDsToFetch.size();
			fLastIndex = from - 1;

			if (from == 1) {
				fFolder->MessageEntriesFetched();

				if (fUIDsToFetch.size() > 0) {
					// Add pending command to fetch the message headers
					WorkerCommand* command = new FetchHeadersCommand(*fFolder,
						*fMailbox, fUIDsToFetch,
						WorkerPrivate(worker).BodyFetchLimit());
					if (!fFetchCommands.AddItem(command))
						delete command;

					fUIDsToFetch.clear();
				}
				fState = SELECT;
			}
		}

		return B_OK;
	}

	virtual bool IsDone() const
	{
		return fState == DONE;
	}

private:
	enum State {
		INIT,
		SELECT,
		FETCH_ENTRIES,
		DONE
	};

	IMAPConnectionWorker&	fWorker;
	BObjectList<IMAPFolder>	fFolders;
	State					fState;
	IMAPFolder*				fFolder;
	IMAPMailbox*			fMailbox;
	uint32					fFirstIndex;
	uint32					fLastIndex;
	uint64					fTotalEntries;
	uint64					fTotalBytes;
	WorkerCommandList		fFetchCommands;
	MessageUIDList			fUIDsToFetch;
};


class UpdateFlagsCommand : public WorkerCommand {
public:
	UpdateFlagsCommand(IMAPFolder& folder, IMAPMailbox& mailbox,
		MessageUIDList& entries, uint32 flags)
		:
		fFolder(folder),
		fMailbox(mailbox),
		fEntries(entries),
		fFlags(flags)
	{
	}

	virtual status_t Process(IMAPConnectionWorker& worker)
	{
		if (fEntries.empty())
			return B_OK;

		fUID = *fEntries.begin();
		fEntries.erase(fEntries.begin());

		status_t status = WorkerPrivate(worker).SelectMailbox(fFolder);
		if (status == B_OK) {
			IMAP::Protocol& protocol = WorkerPrivate(worker).Protocol();
			IMAP::SetFlagsCommand set(fUID, fFlags);
			status = protocol.ProcessCommand(set);
		}

		return status;
	}

	virtual bool IsDone() const
	{
		return fEntries.empty();
	}

private:
	IMAPFolder&				fFolder;
	IMAPMailbox&			fMailbox;
	MessageUIDList			fEntries;
	uint32					fUID;
	uint32					fFlags;
};


struct CommandDelete
{
	inline void operator()(WorkerCommand* command)
	{
		delete command;
	}
};


/*!	An auto deleter similar to ObjectDeleter that calls SyncCommandDone()
	for all SyncCommands.
*/
struct CommandDeleter : BPrivate::AutoDeleter<WorkerCommand, CommandDelete>
{
	CommandDeleter(IMAPConnectionWorker& worker, WorkerCommand* command)
		:
		BPrivate::AutoDeleter<WorkerCommand, CommandDelete>(command),
		fWorker(worker)
	{
	}

	~CommandDeleter()
	{
		if (dynamic_cast<SyncCommand*>(fObject) != NULL)
			WorkerPrivate(fWorker).SyncCommandDone();
	}

private:
	IMAPConnectionWorker&	fWorker;
};


// #pragma mark -


IMAPConnectionWorker::IMAPConnectionWorker(IMAPProtocol& owner,
	const Settings& settings, bool main)
	:
	fOwner(owner),
	fSettings(settings),
	fPendingCommandsSemaphore(-1),
	fIdleBox(NULL),
	fMain(main),
	fStopped(false)
{
	fExistsHandler.SetListener(this);
	fProtocol.AddHandler(fExistsHandler);

	fExpungeHandler.SetListener(this);
	fProtocol.AddHandler(fExpungeHandler);
}


IMAPConnectionWorker::~IMAPConnectionWorker()
{
	puts("worker quit");
	delete_sem(fPendingCommandsSemaphore);
	_Disconnect();
}


bool
IMAPConnectionWorker::HasMailboxes() const
{
	BAutolock locker(const_cast<IMAPConnectionWorker*>(this)->fLocker);
	return !fMailboxes.empty();
}


uint32
IMAPConnectionWorker::CountMailboxes() const
{
	BAutolock locker(const_cast<IMAPConnectionWorker*>(this)->fLocker);
	return fMailboxes.size();
}


void
IMAPConnectionWorker::AddMailbox(IMAPFolder* folder)
{
	BAutolock locker(fLocker);

	fMailboxes.insert(std::make_pair(folder, (IMAPMailbox*)NULL));

	// Prefer to have the INBOX in idle mode over other mail boxes
	if (fIdleBox == NULL || folder->MailboxName().ICompare("INBOX") == 0)
		fIdleBox = folder;
}


void
IMAPConnectionWorker::RemoveAllMailboxes()
{
	BAutolock locker(fLocker);

	// Reset listeners, and delete the mailboxes
	MailboxMap::iterator iterator = fMailboxes.begin();
	for (; iterator != fMailboxes.end(); iterator++) {
		iterator->first->SetListener(NULL);
		delete iterator->second;
	}

	fIdleBox = NULL;
	fMailboxes.clear();
}


status_t
IMAPConnectionWorker::Run()
{
	fPendingCommandsSemaphore = create_sem(0, "imap pending commands");
	if (fPendingCommandsSemaphore < 0)
		return fPendingCommandsSemaphore;

	fThread = spawn_thread(&_Worker, "imap connection worker",
		B_NORMAL_PRIORITY, this);
	if (fThread < 0)
		return fThread;

	resume_thread(fThread);
	return B_OK;
}


void
IMAPConnectionWorker::Quit()
{
	printf("IMAP: worker %p: enqueue quit\n", this);
	BAutolock qlocker(fQueueLocker);
	while (!fPendingCommands.IsEmpty())
		delete(fPendingCommands.RemoveItemAt(0));
	_EnqueueCommand(new QuitCommand());
}


status_t
IMAPConnectionWorker::EnqueueCheckSubscribedFolders()
{
	printf("IMAP: worker %p: enqueue check subscribed folders\n", this);
	return _EnqueueCommand(new CheckSubscribedFoldersCommand());
}


status_t
IMAPConnectionWorker::EnqueueCheckMailboxes()
{
	// Do not schedule checking mailboxes again if we're still working on
	// those.
	if (fSyncPending > 0)
		return B_OK;

	printf("IMAP: worker %p: enqueue check mailboxes\n", this);
	return _EnqueueCommand(new CheckMailboxesCommand(*this));
}


status_t
IMAPConnectionWorker::EnqueueFetchBody(IMAPFolder& folder, uint32 uid,
	const BMessenger& replyTo)
{
	IMAPMailbox* mailbox = _MailboxFor(folder);
	if (mailbox == NULL)
		return B_ENTRY_NOT_FOUND;

	std::vector<uint32> uids;
	uids.push_back(uid);

	return _EnqueueCommand(new FetchBodiesCommand(folder, *mailbox, uids,
		&replyTo));
}


status_t
IMAPConnectionWorker::EnqueueUpdateFlags(IMAPFolder& folder, uint32 uid,
	uint32 flags)
{
	IMAPMailbox* mailbox = _MailboxFor(folder);
	if (mailbox == NULL)
		return B_ENTRY_NOT_FOUND;

	std::vector<uint32> uids;
	uids.push_back(uid);

	return _EnqueueCommand(new UpdateFlagsCommand(folder, *mailbox, uids,
		flags));
}


// #pragma mark - Handler listener


void
IMAPConnectionWorker::MessageExistsReceived(uint32 count)
{
	printf("Message exists: %" B_PRIu32 "\n", count);
	fMessagesExist = count;

	// TODO: We might want to trigger another check even during sync
	// (but only one), if this isn't the result of a SELECT
	EnqueueCheckMailboxes();
}


void
IMAPConnectionWorker::MessageExpungeReceived(uint32 index)
{
	printf("Message expunge: %" B_PRIu32 "\n", index);
	if (fSelectedBox == NULL)
		return;

	BAutolock locker(fLocker);

	IMAPMailbox* mailbox = _MailboxFor(*fSelectedBox);
	if (mailbox != NULL) {
		mailbox->RemoveMessageEntry(index);
		// TODO: remove message from folder
	}
}


// #pragma mark - private


status_t
IMAPConnectionWorker::_Worker()
{
	status_t status = B_OK;

	while (!fStopped) {
		BAutolock qlocker(fQueueLocker);

		if (fPendingCommands.IsEmpty()) {
			if (!fIdle)
				_Disconnect();
			qlocker.Unlock();

			// TODO: in idle mode, we'd need to parse any incoming message here
			_WaitForCommands();
			continue;
		}

		WorkerCommand* command = fPendingCommands.RemoveItemAt(0);
		if (command == NULL)
			continue;

		qlocker.Unlock();
		BAutolock locker(fLocker);
		CommandDeleter deleter(*this, command);

		if (dynamic_cast<QuitCommand*>(command) == NULL) { // do not connect on QuitCommand
			status = _Connect();
			if (status != B_OK)
				break;
		}

		status = command->Process(*this);
		if (status != B_OK)
			break;

		if (!command->IsDone()) {
			deleter.Detach();
			command->SetContinuation();
			locker.Unlock();
			_EnqueueCommand(command);
		}
	}

	fOwner.WorkerQuit(this);
	return status;
}


/*!	Enqueues the given command to the worker queue. This method will take
	over ownership of the given command even in the error case.
*/
status_t
IMAPConnectionWorker::_EnqueueCommand(WorkerCommand* command)
{
	BAutolock qlocker(fQueueLocker);

	if (!fPendingCommands.AddItem(command)) {
		delete command;
		return B_NO_MEMORY;
	}

	if (dynamic_cast<SyncCommand*>(command) != NULL
		&& !command->IsContinuation())
		fSyncPending++;

	qlocker.Unlock();
	release_sem(fPendingCommandsSemaphore);
	return B_OK;
}


void
IMAPConnectionWorker::_WaitForCommands()
{
	int32 count = 1;
	get_sem_count(fPendingCommandsSemaphore, &count);
	if (count < 1)
		count = 1;

	while (acquire_sem_etc(fPendingCommandsSemaphore, count, 0,
			B_INFINITE_TIMEOUT) == B_INTERRUPTED);
}


status_t
IMAPConnectionWorker::_SelectMailbox(IMAPFolder& folder, uint32* _nextUID)
{
	if (fSelectedBox == &folder && _nextUID == NULL)
		return B_OK;

	IMAP::SelectCommand select(folder.MailboxName().String());

	status_t status = fProtocol.ProcessCommand(select);
	if (status == B_OK) {
		folder.SetUIDValidity(select.UIDValidity());
		if (_nextUID != NULL)
			*_nextUID = select.NextUID();
		fSelectedBox = &folder;
	}

	return status;
}


IMAPMailbox*
IMAPConnectionWorker::_MailboxFor(IMAPFolder& folder)
{
	MailboxMap::iterator found = fMailboxes.find(&folder);
	if (found == fMailboxes.end())
		return NULL;

	IMAPMailbox* mailbox = found->second;
	if (mailbox == NULL) {
		mailbox = new IMAPMailbox(fProtocol, folder.MailboxName());
		folder.SetListener(mailbox);
		found->second = mailbox;
	}
	return mailbox;
}


void
IMAPConnectionWorker::_SyncCommandDone()
{
	fSyncPending--;
}


bool
IMAPConnectionWorker::_IsQuitPending()
{
	BAutolock locker(fQueueLocker);
	WorkerCommand* nextCommand = fPendingCommands.ItemAt(0);
	return dynamic_cast<QuitCommand*>(nextCommand) != NULL;
}


status_t
IMAPConnectionWorker::_Connect()
{
	if (fProtocol.IsConnected())
		return B_OK;

	status_t status = B_INTERRUPTED;
	int tries = 10;
	while (tries-- > 0) {
		if (_IsQuitPending())
			break;
		status = fProtocol.Connect(fSettings.ServerAddress(),
			fSettings.Username(), fSettings.Password(), fSettings.UseSSL());
		if (status == B_OK)
			break;

		// Wait for 1 second, and try again
		snooze(1000000);
	}
	// TODO: if other workers are connected, but it fails for us, we need to
	// remove this worker, and reduce the number of concurrent connections
	if (status != B_OK)
		return status;

	//fIdle = fSettings.IdleMode() && fProtocol.Capabilities().Contains("IDLE");
	// TODO: Idle mode is not yet implemented!
	fIdle = false;
	return B_OK;
}


void
IMAPConnectionWorker::_Disconnect()
{
	fProtocol.Disconnect();
	fSelectedBox = NULL;
}


/*static*/ status_t
IMAPConnectionWorker::_Worker(void* _self)
{
	IMAPConnectionWorker* self = (IMAPConnectionWorker*)_self;
	status_t status = self->_Worker();

	delete self;
	return status;
}
