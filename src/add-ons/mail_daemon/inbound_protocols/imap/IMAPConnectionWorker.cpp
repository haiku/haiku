/*
 * Copyright 2011-2013, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "IMAPConnectionWorker.h"

#include <Autolock.h>

#include <AutoDeleter.h>

#include "IMAPFolder.h"
#include "IMAPMailbox.h"
#include "IMAPProtocol.h"


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
								WorkerCommand() {}
	virtual						~WorkerCommand() {}

	virtual	status_t			Process(IMAPConnectionWorker& worker) = 0;
	virtual bool				IsDone() const { return true; }
};


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


class FetchHeadersCommand : public SyncCommand, public IMAP::FetchListener {
public:
	FetchHeadersCommand(IMAPFolder& folder, IMAPMailbox& mailbox,
		uint32 from, uint32 to)
		:
		fFolder(folder),
		fMailbox(mailbox),
		fFrom(from),
		fTo(to)
	{
	}

	virtual status_t Process(IMAPConnectionWorker& worker)
	{
		IMAP::Protocol& protocol = WorkerPrivate(worker).Protocol();

		// TODO: check nextUID if we get one
		status_t status = WorkerPrivate(worker).SelectMailbox(fFolder);
		if (status != B_OK)
			return status;

		// TODO: this does not scale that well. Over time, the holes in the
		// UIDs might become really large
		uint32 to = fTo;
		if (to - fFrom >= kMaxFetchEntries)
			to = fFrom + kMaxFetchEntries - 1;

		// TODO: trigger download of mails for all messages below the
		// body fetch limit
		printf("IMAP: fetch headers from %lu to %lu\n", fFrom, to);
		IMAP::FetchCommand fetch(fFrom, to,
			IMAP::kFetchHeader | IMAP::kFetchFlags);
		fetch.SetListener(this);

		status = protocol.ProcessCommand(fetch);
		if (status != B_OK)
			return status;

		fFrom = to + 1;
		return B_OK;
	}

	virtual bool IsDone() const
	{
		return fFrom >= fTo;
	}

	virtual bool FetchData(uint32 fetchFlags, BDataIO& stream, size_t& length)
	{
		fFetchStatus = fFolder.StoreMessage(fFile, fetchFlags, stream,
			length, fRef);
		return true;
	}

	virtual void FetchedData(uint32 fetchFlags, uint32 uid, uint32 flags)
	{
		if (fFetchStatus == B_OK)
			fFolder.MessageStored(fRef, fFile, fetchFlags, uid, flags);
	}

private:
	IMAPFolder&				fFolder;
	IMAPMailbox&			fMailbox;
	uint32					fFrom;
	uint32					fTo;
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

			status_t status = WorkerPrivate(worker).SelectMailbox(*fFolder,
				fNextUID);
			if (status != B_OK)
				return status;

			fState = FETCH_ENTRIES;
			fFirstUID = fLastUID = fFolder->LastUID() + 1;
			fMailboxEntries = 0;
		}

		if (fState == FETCH_ENTRIES) {
			status_t status = WorkerPrivate(worker).SelectMailbox(*fFolder);
			if (status != B_OK)
				return status;

			// TODO: this does not scale that well. Over time, the holes in the
			// UIDs might become really large
			uint32 from = fLastUID;
			uint32 to = fNextUID;
			if (to - from >= kMaxFetchEntries)
				to = from + kMaxFetchEntries - 1;

			printf("IMAP: get entries from %lu to %lu\n", from, to);
			// TODO: we don't really need the flags at this point at all
			IMAP::MessageEntryList	entries;
			IMAP::FetchMessageEntriesCommand fetch(entries, from, to);
			status = protocol.ProcessCommand(fetch);
			if (status != B_OK)
				return status;

			// Determine how much we need to download
			for (size_t i = 0; i < entries.size(); i++) {
				printf("%10lu %8lu bytes, flags: %#lx\n", entries[i].uid,
					entries[i].size, entries[i].flags);
				fTotalBytes += entries[i].size;
			}
			fTotalEntries += entries.size();
			fMailboxEntries += entries.size();

			fLastUID = to + 1;

			if (to == fNextUID) {
				if (fMailboxEntries > 0) {
					// Add pending command to fetch the message headers
					WorkerCommand* command = new FetchHeadersCommand(*fFolder,
						*fMailbox, fFirstUID, fNextUID);
					if (!fFetchCommands.AddItem(command))
						delete command;
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
	uint32					fFirstUID;
	uint32					fLastUID;
	uint32					fNextUID;
	uint32					fMailboxEntries;
	uint64					fTotalEntries;
	uint64					fTotalBytes;
	WorkerCommandList		fFetchCommands;
};


struct CommandDelete
{
	inline void operator()(WorkerCommand* command)
	{
		delete command;
	}
};


/*!	An auto deleter similar to ObjectDeleter that called SyncCommandDone()
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
IMAPConnectionWorker::EnqueueRetrieveMail(entry_ref& ref)
{
	return B_OK;
}


// #pragma mark - Handler listener


void
IMAPConnectionWorker::MessageExistsReceived(uint32 index)
{
	printf("Message exists: %ld\n", index);
}


void
IMAPConnectionWorker::MessageExpungeReceived(uint32 index)
{
	printf("Message expunge: %ld\n", index);
}


// #pragma mark - private


status_t
IMAPConnectionWorker::_Worker()
{
	while (!fStopped) {
		BAutolock locker(fLocker);

		if (fPendingCommands.IsEmpty()) {
			_Disconnect();
			locker.Unlock();

			_WaitForCommands();
			continue;
		}

		WorkerCommand* command = fPendingCommands.RemoveItemAt(0);
		if (command == NULL)
			continue;

		CommandDeleter deleter(*this, command);

		status_t status = _Connect();
		if (status != B_OK)
			return status;

		status = command->Process(*this);
		if (status != B_OK)
			return status;

		if (!command->IsDone()) {
			deleter.Detach();
			_EnqueueCommand(command);
		}
	}

	fOwner.WorkerQuit(this);
	return B_OK;
}


/*!	Enqueues the given command to the worker queue. This method will take
	over ownership of the given command even in the error case.
*/
status_t
IMAPConnectionWorker::_EnqueueCommand(WorkerCommand* command)
{
	BAutolock locker(fLocker);

	if (!fPendingCommands.AddItem(command)) {
		delete command;
		return B_NO_MEMORY;
	}

	if (dynamic_cast<SyncCommand*>(command) != NULL)
		fSyncPending++;

	locker.Unlock();
	release_sem(fPendingCommandsSemaphore);
	return B_OK;
}


void
IMAPConnectionWorker::_WaitForCommands()
{
	while (acquire_sem(fPendingCommandsSemaphore) == B_INTERRUPTED);
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


status_t
IMAPConnectionWorker::_Connect()
{
	if (fProtocol.IsConnected())
		return B_OK;

	status_t status = fProtocol.Connect(fSettings.ServerAddress(),
		fSettings.Username(), fSettings.Password(), fSettings.UseSSL());
	if (status != B_OK)
		return status;

	fIdle = fSettings.IdleMode() && fProtocol.Capabilities().Contains("IDLE");
	return B_OK;
}


void
IMAPConnectionWorker::_Disconnect()
{
	fProtocol.Disconnect();
}


/*static*/ status_t
IMAPConnectionWorker::_Worker(void* _self)
{
	IMAPConnectionWorker* self = (IMAPConnectionWorker*)_self;
	status_t status = self->_Worker();

	delete self;
	return status;
}
