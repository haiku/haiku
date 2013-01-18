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
	puts("IMAP protocol started");

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
IMAPProtocol::SyncMessages()
{
	puts("IMAP: sync");
	return B_ERROR;
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
	}
}


void
IMAPProtocol::ReadyToRun()
{
	// Determine how many connection workers we'll need
	// TODO: in passive mode, this should be done on every sync

	IMAP::Protocol protocol;
	status_t status = protocol.Connect(fSettings.ServerAddress(),
		fSettings.Username(), fSettings.Password(), fSettings.UseSSL());
	if (status != B_OK)
		return;
}


// #pragma mark -


extern "C" BInboundMailProtocol*
instantiate_inbound_protocol(const BMailAccountSettings& settings)
{
	return new IMAPProtocol(settings);
}
