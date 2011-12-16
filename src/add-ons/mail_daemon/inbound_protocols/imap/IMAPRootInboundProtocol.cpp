/*
 * Copyright 2011, Haiku, Inc. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */


#include "IMAPRootInboundProtocol.h"

#include <Messenger.h>

#include "IMAPFolders.h"


IMAPRootInboundProtocol::IMAPRootInboundProtocol(BMailAccountSettings* settings)
	:
	IMAPInboundProtocol(settings, "INBOX")
{
}


IMAPRootInboundProtocol::~IMAPRootInboundProtocol()
{
	_ShutdownChilds();
}


status_t
IMAPRootInboundProtocol::Connect(const char* server, const char* username,
	const char* password, bool useSSL, int32 port)
{
	status_t status = IMAPInboundProtocol::Connect(server, username, password,
		useSSL, port);
	if (status != B_OK)
		return status;

	IMAPFolders folder(fIMAPMailbox);
	FolderList folders;
	folder.GetFolders(folders);
	for (unsigned int i = 0; i < folders.size(); i++) {
		if (!folders[i].subscribed || folders[i].folder == "INBOX")
			continue;

		IMAPInboundProtocol* inboundProtocol = new IMAPInboundProtocol(
			&fAccountSettings, folders[i].folder);
		inboundProtocol->SetMailNotifier(fMailNotifier->Clone());

		InboundProtocolThread* inboundThread = new InboundProtocolThread(
			inboundProtocol);
		inboundThread->Run();

		fProtocolThreadList.AddItem(inboundThread);
	}
	return status;
}


status_t
IMAPRootInboundProtocol::Disconnect()
{
	status_t status = IMAPInboundProtocol::Disconnect();
	_ShutdownChilds();
	return status;
}


void
IMAPRootInboundProtocol::SetStopNow()
{
	for (int32 i = 0; i < fProtocolThreadList.CountItems(); i++)
		fProtocolThreadList.ItemAt(i)->SetStopNow();
	IMAPInboundProtocol::SetStopNow();
}


status_t
IMAPRootInboundProtocol::SyncMessages()
{
	for (int32 i = 0; i < fProtocolThreadList.CountItems(); i++)
		fProtocolThreadList.ItemAt(i)->SyncMessages();
	return IMAPInboundProtocol::SyncMessages();
}


status_t
IMAPRootInboundProtocol::FetchBody(const entry_ref& ref)
{
	if (InterestingEntry(ref))
		return IMAPInboundProtocol::FetchBody(ref);
	InboundProtocolThread* thread = _FindThreadFor(ref);
	if (!thread)
		return B_BAD_VALUE;
	thread->FetchBody(ref);
	return B_OK;
}


status_t
IMAPRootInboundProtocol::MarkMessageAsRead(const entry_ref& ref,
	read_flags flag)
{
	if (InterestingEntry(ref))
		return IMAPInboundProtocol::MarkMessageAsRead(ref, flag);
	InboundProtocolThread* thread = _FindThreadFor(ref);
	if (!thread)
		return B_BAD_VALUE;
	thread->MarkMessageAsRead(ref, flag);
	return B_OK;
}


status_t
IMAPRootInboundProtocol::DeleteMessage(const entry_ref& ref)
{
	if (InterestingEntry(ref))
		return IMAPInboundProtocol::DeleteMessage(ref);
	InboundProtocolThread* thread = _FindThreadFor(ref);
	if (!thread)
		return B_BAD_VALUE;
	thread->DeleteMessage(ref);
	return B_OK;
}


status_t
IMAPRootInboundProtocol::AppendMessage(const entry_ref& ref)
{
	if (InterestingEntry(ref))
		return IMAPInboundProtocol::AppendMessage(ref);
	InboundProtocolThread* thread = _FindThreadFor(ref);
	if (!thread)
		return B_BAD_VALUE;
	thread->AppendMessage(ref);
	return B_OK;
}


status_t
IMAPRootInboundProtocol::DeleteMessage(node_ref& node)
{
	status_t status = IMAPInboundProtocol::DeleteMessage(node);
	if (status == B_OK)
		return status;
	for (int32 i = 0; i < fProtocolThreadList.CountItems(); i++)
		fProtocolThreadList.ItemAt(i)->TriggerFileDeleted(node);
	return B_OK;
}


void
IMAPRootInboundProtocol::_ShutdownChilds()
{
	BMessage reply;
	for (int32 i = 0; i < fProtocolThreadList.CountItems(); i++) {
		InboundProtocolThread* thread = fProtocolThreadList.ItemAt(i);
		MailProtocol* protocol = thread->Protocol();

		thread->SetStopNow();
		BMessenger(thread).SendMessage(B_QUIT_REQUESTED, &reply);

		delete protocol;
	}
	fProtocolThreadList.MakeEmpty();
}


InboundProtocolThread*
IMAPRootInboundProtocol::_FindThreadFor(const entry_ref& ref)
{
	for (int32 i = 0; i < fProtocolThreadList.CountItems(); i++) {
		InboundProtocolThread* thread = fProtocolThreadList.ItemAt(i);
		IMAPInboundProtocol* protocol
			= (IMAPInboundProtocol*)thread->Protocol();
		if (protocol->InterestingEntry(ref))
			return thread;
	}
	return NULL;
}


// #pragma mark -


InboundProtocol*
instantiate_inbound_protocol(BMailAccountSettings* settings)
{
	return new IMAPRootInboundProtocol(settings);
}
