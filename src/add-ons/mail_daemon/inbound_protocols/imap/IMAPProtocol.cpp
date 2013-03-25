/*
 * Copyright 2013, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "IMAPProtocol.h"

#include <Directory.h>

#include "IMAPConnectionWorker.h"


IMAPProtocol::IMAPProtocol(const BMailAccountSettings& settings)
	:
	BInboundMailProtocol(settings),
	fSettings(settings.InboundSettings())
{
	BPath destination = fSettings.Destination();

	status_t status = create_directory(destination.Path(), 0755);
	if (status != B_OK) {
		fprintf(stderr, "imap: Could not create destination directory %s: %s\n",
			destination.Path(), strerror(status));
	}

	PostMessage(B_READY_TO_RUN);
}


IMAPProtocol::~IMAPProtocol()
{
}


status_t
IMAPProtocol::CheckSubscribedFolders(IMAP::Protocol& protocol)
{
	// Get list of subscribed folders

	StringList folders;
	status_t status = protocol.GetSubscribedFolders(folders);
	if (status != B_OK)
		return status;

	// Determine how many new mailboxes we have

	StringList::iterator iterator = folders.begin();
	for (; iterator != folders.end(); iterator++) {
		if (fKnownMailboxes.find(*iterator) != fKnownMailboxes.end())
			iterator = folders.erase(iterator);
	}

	if (fSettings.IdleMode()) {
		// Create connection workers as allowed

		int32 totalMailboxes = fKnownMailboxes.size() + folders.size();

		while (fWorkers.CountItems() < fSettings.MaxConnections()
			&& fWorkers.CountItems() < totalMailboxes) {
			IMAPConnectionWorker* worker = new IMAPConnectionWorker(*this,
				fSettings);
			if (!fWorkers.AddItem(worker)) {
				delete worker;
				break;
			}

			worker->Start();
		}
	}

	// Distribute the new mailboxes to the existing workers

	int32 index = 0;
	while (!folders.empty()) {
		BString folder = folders[0];
		folders.erase(folders.begin());

		fWorkers.ItemAt(index)->AddMailbox(folder);
		fKnownMailboxes.insert(folder);

		index = (index + 1) % fWorkers.CountItems();
	}

	return B_OK;
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

		return worker->Start();
	}

	return B_OK;
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


// #pragma mark -


extern "C" BInboundMailProtocol*
instantiate_inbound_protocol(const BMailAccountSettings& settings)
{
	return new IMAPProtocol(settings);
}
