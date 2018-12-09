/*
 * Copyright 2013-2016, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "IMAPProtocol.h"

#include <Directory.h>
#include <Messenger.h>

#include "IMAPConnectionWorker.h"
#include "IMAPFolder.h"
#include "Utilities.h"

#include <mail_util.h>


IMAPProtocol::IMAPProtocol(const BMailAccountSettings& settings)
	:
	BInboundMailProtocol("IMAP", settings),
	fSettings(settings.Name(), settings.InboundSettings()),
	fWorkers(5, false)
{
	BPath destination = fSettings.Destination();

	status_t status = create_directory(destination.Path(), 0755);
	if (status != B_OK) {
		fprintf(stderr, "IMAP: Could not create destination directory %s: %s\n",
			destination.Path(), strerror(status));
	}

	mutex_init(&fWorkerLock, "imap worker lock");

	PostMessage(B_READY_TO_RUN);
}


IMAPProtocol::~IMAPProtocol()
{
	MutexLocker locker(fWorkerLock);
	std::vector<thread_id> threads;
	for (int32 i = 0; i < fWorkers.CountItems(); i++) {
		threads.push_back(fWorkers.ItemAt(i)->Thread());
		fWorkers.ItemAt(i)->Quit();
	}
	locker.Unlock();

	for (uint32 i = 0; i < threads.size(); i++)
		wait_for_thread(threads[i], NULL);

	FolderMap::iterator iterator = fFolders.begin();
	for (; iterator != fFolders.end(); iterator++) {
		IMAPFolder* folder = iterator->second;
		delete folder; // to stop thread
	}
}

status_t
IMAPProtocol::CheckSubscribedFolders(IMAP::Protocol& protocol, bool idle)
{
	// Get list of subscribed folders

	BStringList newFolders;
	BString separator;
	status_t status = protocol.GetSubscribedFolders(newFolders, separator);
	if (status != B_OK)
		return status;

	// Determine how many new mailboxes we have

	for (int32 i = 0; i < newFolders.CountStrings();) {
		if (fFolders.find(newFolders.StringAt(i)) != fFolders.end())
			newFolders.Remove(i);
		else
			i++;
	}

	int32 totalMailboxes = fFolders.size() + newFolders.CountStrings();
	int32 workersWanted = 1;
	if (idle)
		workersWanted = std::min(fSettings.MaxConnections(), totalMailboxes);

	MutexLocker locker(fWorkerLock);

	if (newFolders.IsEmpty() && fWorkers.CountItems() == workersWanted) {
		// Nothing to do - we've already distributed everything
		return _EnqueueCheckMailboxes();
	}

	// Remove mailboxes from workers
	for (int32 i = 0; i < fWorkers.CountItems(); i++) {
		fWorkers.ItemAt(i)->RemoveAllMailboxes();
	}
	fWorkerMap.clear();

	// Create/remove connection workers as allowed and needed
	while (fWorkers.CountItems() < workersWanted) {
		IMAPConnectionWorker* worker = new IMAPConnectionWorker(*this,
			fSettings);
		if (!fWorkers.AddItem(worker)) {
			delete worker;
			break;
		}

		status = worker->Run();
		if (status != B_OK) {
			fWorkers.RemoveItem(worker);
			delete worker;
		}
	}

	while (fWorkers.CountItems() > workersWanted) {
		IMAPConnectionWorker* worker
			= fWorkers.RemoveItemAt(fWorkers.CountItems() - 1);
		worker->Quit();
	}

	// Update known mailboxes
	for (int32 i = 0; i < newFolders.CountStrings(); i++) {
		const BString& mailbox = newFolders.StringAt(i);
		fFolders.insert(std::make_pair(mailbox,
			_CreateFolder(mailbox, separator)));
	}

	// Distribute the mailboxes evenly to the workers
	FolderMap::iterator iterator = fFolders.begin();
	int32 index = 0;
	for (; iterator != fFolders.end(); iterator++) {
		IMAPConnectionWorker* worker = fWorkers.ItemAt(index);
		IMAPFolder* folder = iterator->second;
		worker->AddMailbox(folder);
		fWorkerMap.insert(std::make_pair(folder, worker));

		index = (index + 1) % fWorkers.CountItems();
	}

	// Start waiting workers
	return _EnqueueCheckMailboxes();
}


void
IMAPProtocol::WorkerQuit(IMAPConnectionWorker* worker)
{
	MutexLocker locker(fWorkerLock);
	fWorkers.RemoveItem(worker);

	WorkerMap::iterator iterator = fWorkerMap.begin();
	while (iterator != fWorkerMap.end()) {
		WorkerMap::iterator removed = iterator++;
		if (removed->second == worker)
			fWorkerMap.erase(removed);
	}
}


void
IMAPProtocol::MessageStored(IMAPFolder& folder, entry_ref& ref,
	BFile& stream, uint32 fetchFlags, BMessage& attributes)
{
	if ((fetchFlags & (IMAP::kFetchHeader | IMAP::kFetchBody))
			== (IMAP::kFetchHeader | IMAP::kFetchBody)) {
		ProcessMessageFetched(ref, stream, attributes);
	} else if ((fetchFlags & IMAP::kFetchHeader) != 0) {
		ProcessHeaderFetched(ref, stream, attributes);
	} else if ((fetchFlags & IMAP::kFetchBody) != 0) {
		NotifyBodyFetched(ref, stream, attributes);
	}
}


status_t
IMAPProtocol::UpdateMessageFlags(IMAPFolder& folder, uint32 uid, uint32 flags)
{
	MutexLocker locker(fWorkerLock);

	WorkerMap::const_iterator found = fWorkerMap.find(&folder);
	if (found == fWorkerMap.end())
		return B_ERROR;

	IMAPConnectionWorker* worker = found->second;
	return worker->EnqueueUpdateFlags(folder, uid, flags);
}


status_t
IMAPProtocol::SyncMessages()
{
	puts("IMAP: sync");

	MutexLocker locker(fWorkerLock);
	if (fWorkers.IsEmpty()) {
		// Create main (and possibly initial) connection worker
		IMAPConnectionWorker* worker = new IMAPConnectionWorker(*this,
			fSettings, true);
		if (!fWorkers.AddItem(worker)) {
			delete worker;
			return B_NO_MEMORY;
		}

		worker->EnqueueCheckSubscribedFolders();
		return worker->Run();
	}
	fWorkers.ItemAt(0)->EnqueueCheckSubscribedFolders();
	return B_OK;
}


status_t
IMAPProtocol::MarkMessageAsRead(const entry_ref& ref, read_flags flags)
{
	printf("IMAP: mark as read %s: %d\n", ref.name, flags);
	return B_ERROR;
}


void
IMAPProtocol::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_READY_TO_RUN:
			ReadyToRun();
			break;

		default:
			BInboundMailProtocol::MessageReceived(message);
			break;
	}
}


status_t
IMAPProtocol::HandleFetchBody(const entry_ref& ref, const BMessenger& replyTo)
{
	printf("IMAP: fetch body %s\n", ref.name);
	MutexLocker locker(fWorkerLock);

	IMAPFolder* folder = _FolderFor(ref.directory);
	if (folder == NULL)
		return B_ENTRY_NOT_FOUND;

	uint32 uid;
	status_t status = folder->GetMessageUID(ref, uid);
	if (status != B_OK)
		return status;

	WorkerMap::const_iterator found = fWorkerMap.find(folder);
	if (found == fWorkerMap.end())
		return B_ERROR;

	IMAPConnectionWorker* worker = found->second;
	return worker->EnqueueFetchBody(*folder, uid, replyTo);
}


void
IMAPProtocol::ReadyToRun()
{
	puts("IMAP: ready to run!");
	if (fSettings.IdleMode())
		SyncMessages();
}


IMAPFolder*
IMAPProtocol::_CreateFolder(const BString& mailbox, const BString& separator)
{
	BString name = MailboxToFolderName(mailbox, separator);

	BPath path(fSettings.Destination());
	if (path.Append(name.String()) != B_OK) {
		fprintf(stderr, "Could not append path: %s\n", name.String());
		return NULL;
	}

	status_t status = create_directory(path.Path(), 0755);
	if (status != B_OK) {
		fprintf(stderr, "Could not create path %s: %s\n", path.Path(),
			strerror(status));
		return NULL;
	}

	CopyMailFolderAttributes(path.Path());

	entry_ref ref;
	status = get_ref_for_path(path.Path(), &ref);
	if (status != B_OK) {
		fprintf(stderr, "Could not get ref for %s: %s\n", path.Path(),
			strerror(status));
		return NULL;
	}

	IMAPFolder* folder = new IMAPFolder(*this, mailbox, ref);
	status = folder->Init();
	if (status != B_OK) {
		fprintf(stderr, "Initializing folder %s failed: %s\n", path.Path(),
			strerror(status));
		return NULL;
	}

	fFolderNodeMap.insert(std::make_pair(folder->NodeID(), folder));
	return folder;
}


IMAPFolder*
IMAPProtocol::_FolderFor(ino_t directory)
{
	FolderNodeMap::const_iterator found = fFolderNodeMap.find(directory);
	if (found != fFolderNodeMap.end())
		return found->second;

	return NULL;
}


status_t
IMAPProtocol::_EnqueueCheckMailboxes()
{
	for (int32 i = 0; i < fWorkers.CountItems(); i++) {
		fWorkers.ItemAt(i)->EnqueueCheckMailboxes();
	}

	return B_OK;
}


// #pragma mark -


extern "C" BInboundMailProtocol*
instantiate_inbound_protocol(const BMailAccountSettings& settings)
{
	return new IMAPProtocol(settings);
}
