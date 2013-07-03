/*
 * Copyright 2013, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "IMAPProtocol.h"

#include <Directory.h>

#include "IMAPConnectionWorker.h"
#include "IMAPFolder.h"
#include "Utilities.h"


IMAPProtocol::IMAPProtocol(const BMailAccountSettings& settings)
	:
	BInboundMailProtocol(settings),
	fSettings(settings.Name(), settings.InboundSettings()),
	fWorkers(5, false)
{
	BPath destination = fSettings.Destination();

	status_t status = create_directory(destination.Path(), 0755);
	if (status != B_OK) {
		fprintf(stderr, "IMAP: Could not create destination directory %s: %s\n",
			destination.Path(), strerror(status));
	}

	PostMessage(B_READY_TO_RUN);
}


IMAPProtocol::~IMAPProtocol()
{
}


status_t
IMAPProtocol::CheckSubscribedFolders(IMAP::Protocol& protocol, bool idle)
{
	// Get list of subscribed folders

	StringList newFolders;
	BString separator;
	status_t status = protocol.GetSubscribedFolders(newFolders, separator);
	if (status != B_OK)
		return status;

	// Determine how many new mailboxes we have

	StringList::iterator folderIterator = newFolders.begin();
	while (folderIterator != newFolders.end()) {
		if (fFolders.find(*folderIterator) != fFolders.end())
			folderIterator = newFolders.erase(folderIterator);
		else
			folderIterator++;
	}

	int32 totalMailboxes = fFolders.size() + newFolders.size();
	int32 workersWanted = 1;
	if (idle)
		workersWanted = std::min(fSettings.MaxConnections(), totalMailboxes);

	if (newFolders.empty() && fWorkers.CountItems() == workersWanted) {
		// Nothing to do - we've already distributed everything
		return B_OK;
	}

	// Remove mailboxes from workers
	for (int32 i = 0; i < fWorkers.CountItems(); i++) {
		fWorkers.ItemAt(i)->RemoveAllMailboxes();
	}

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
	folderIterator = newFolders.begin();
	for (; folderIterator != newFolders.end(); folderIterator++) {
		const BString& mailbox = *folderIterator;
		fFolders.insert(std::make_pair(mailbox,
			_CreateFolder(mailbox, separator)));
	}

	// Distribute the mailboxes evenly to the workers
	FolderMap::iterator iterator = fFolders.begin();
	int32 index = 0;
	for (; iterator != fFolders.end(); iterator++) {
		fWorkers.ItemAt(index)->AddMailbox(iterator->second);
		index = (index + 1) % fWorkers.CountItems();
	}

	// Start waiting workers
	return _EnqueueCheckMailboxes();
}


void
IMAPProtocol::WorkerQuit(IMAPConnectionWorker* worker)
{
	fWorkers.RemoveItem(worker);
}


void
IMAPProtocol::MessageStored(IMAPFolder& folder, entry_ref& ref, BFile& stream,
	uint32 fetchFlags, BMessage& attributes)
{
	if ((fetchFlags & (IMAP::kFetchHeader | IMAP::kFetchBody))
			== IMAP::kFetchHeader | IMAP::kFetchBody) {
		ProcessMessageFetched(ref, stream, attributes);
	} else if ((fetchFlags & IMAP::kFetchHeader) != 0) {
		ProcessHeaderFetched(ref, stream, attributes);
	} else if ((fetchFlags & IMAP::kFetchBody) != 0) {
		NotifyBodyFetched(ref, stream, attributes);
	}
}


status_t
IMAPProtocol::SyncMessages()
{
	puts("IMAP: sync");

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

	return _EnqueueCheckMailboxes();
}


status_t
IMAPProtocol::FetchBody(const entry_ref& ref)
{
	printf("IMAP: fetch body %s\n", ref.name);
	return B_ERROR;
}


status_t
IMAPProtocol::MarkMessageAsRead(const entry_ref& ref, read_flags flags)
{
	printf("IMAP: mark as read %s: %d\n", ref.name, flags);
	return B_ERROR;
}


status_t
IMAPProtocol::DeleteMessage(const entry_ref& ref)
{
	printf("IMAP: delete message %s\n", ref.name);
	return B_ERROR;
}


status_t
IMAPProtocol::AppendMessage(const entry_ref& ref)
{
	printf("IMAP: append message %s\n", ref.name);
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

	entry_ref ref;
	status = get_ref_for_path(path.Path(), &ref);
	if (status != B_OK) {
		fprintf(stderr, "Could not get ref for %s: %s\n", path.Path(),
			strerror(status));
		return NULL;
	}

	return new IMAPFolder(*this, mailbox, ref);
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
