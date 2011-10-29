/*
 * Copyright 2011, Haiku, Inc. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */


#include "IMAPMailbox.h"

#include "IMAPHandler.h"
#include "IMAPStorage.h"


#define DEBUG_IMAP_MAILBOX
#ifdef DEBUG_IMAP_MAILBOX
#	include <stdio.h>
#	define TRACE(x...) printf(x)
#else
#	define TRACE(x...) ;
#endif


MinMessage::MinMessage()
{
	uid = 0;
	flags = 0;
}


// #pragma mark -


IMAPMailbox::IMAPMailbox(IMAPStorage& storage)
	:
	fStorage(storage),

	fMailboxSelectHandler(*this),
	fExistsHandler(*this),
	fExpungeHandler(*this),
	fFlagsHandler(*this),

	fWatching(0),

	fFetchBodyLimit(0)
{
	fHandlerList.AddItem(&fCapabilityHandler);
	fIMAPMailboxListener = &fNULLListener;
}


IMAPMailbox::~IMAPMailbox()
{
	Disconnect();
}


void
IMAPMailbox::SetListener(IMAPMailboxListener* listener)
{
	if (listener == NULL)
		fIMAPMailboxListener = &fNULLListener;
	else
		fIMAPMailboxListener = listener;
}


status_t
IMAPMailbox::SelectMailbox(const char* mailbox)
{
	TRACE("SELECT %s\n", mailbox);
	fMailboxSelectHandler.SetTo(mailbox);
	status_t status = ProcessCommand(&fMailboxSelectHandler);
	if (status != B_OK)
		return status;
	fSelectedMailbox = mailbox;

	if (fCapabilityHandler.Capabilities() != "")
		return status;
	//else get them
	status = ProcessCommand(fCapabilityHandler.Command());
	if (status != B_OK)
		return status;

	return B_OK;
}


BString
IMAPMailbox::Mailbox()
{
	return fSelectedMailbox;
}


status_t
IMAPMailbox::Sync()
{
	TRACE("Sync\n");
	if (fMailboxSelectHandler.NextUID() <= 1)
		return B_OK;

	_InstallUnsolicitedHandler(false);

	fMessageList.clear();
	FetchMessageListCommand fetchMessageListCommand(*this, &fMessageList,
		fMailboxSelectHandler.NextUID());
	status_t status = ProcessCommand(&fetchMessageListCommand);
	if (status != B_OK)
		return status;

	_InstallUnsolicitedHandler(true);
	return B_OK;
}


bool
IMAPMailbox::SupportWatching()
{
	if (fCapabilityHandler.Capabilities().FindFirst("IDLE") < 0)
		return false;
	return true;
}


status_t
IMAPMailbox::StartWatchingMailbox(sem_id startedSem)
{
	atomic_set(&fWatching, 1);

	bool firstIDLE = true;
	// refresh every 29 min
	bigtime_t timeout = 1000 * 1000 * 60 * 29; // 29 min
	status_t status;
	while (true) {
		int32 commandID = NextCommandID();
		TRACE("IDLE ...\n");
		status = SendCommand("IDLE", commandID);
		if (firstIDLE) {
			release_sem(startedSem);
			firstIDLE = false;
		}
		if (status != B_OK)
			break;

		status = HandleResponse(commandID, timeout, false);
		ProcessAfterQuacks(kIMAP4ClientTimeout);

		if (atomic_get(&fWatching) == 0)
			break;

		if (status == B_TIMED_OUT) {
			TRACE("Renew IDLE connection.\n");
			status = SendRawCommand("DONE");
			if (status != B_OK)
				break;
			// handle IDLE response and more
			status = ProcessCommand("NOOP");
			if (status != B_OK)
				break;
			else
				continue;
		}
		if (status != B_OK)
			break;
	}

	atomic_set(&fWatching, 0);
	return status;
}



status_t
IMAPMailbox::StopWatchingMailbox()
{
	if (atomic_get(&fWatching) == 0)
		return B_OK;
	atomic_set(&fWatching, 0);
	return SendRawCommand("DONE");
}


status_t
IMAPMailbox::CheckMailbox()
{
	return ProcessCommand("NOOP");
}


status_t
IMAPMailbox::FetchMinMessage(int32 messageNumber, BPositionIO** data)
{
	if (messageNumber <= 0)
		return B_BAD_VALUE;
	FetchMinMessageCommand fetchMinMessageCommand(*this, messageNumber,
		&fMessageList, data);
	return ProcessCommand(&fetchMinMessageCommand);
}


status_t
IMAPMailbox::FetchBody(int32 messageNumber, BPositionIO* data)
{
	if (data == NULL || messageNumber <= 0) {
		delete data;
		return B_BAD_VALUE;
	}

	FetchBodyCommand fetchBodyCommand(*this, messageNumber, data);
	status_t status = ProcessCommand(&fetchBodyCommand);

	return status;
}


void
IMAPMailbox::SetFetchBodyLimit(int32 limit)
{
	fFetchBodyLimit = limit;
}


status_t
IMAPMailbox::FetchMessage(int32 messageNumber)
{
	return FetchMessages(messageNumber, -1);
}


status_t
IMAPMailbox::FetchMessages(int32 firstMessage, int32 lastMessage)
{
	FetchMessageCommand fetchCommand(*this, firstMessage, lastMessage,
		fFetchBodyLimit);
	return ProcessCommand(&fetchCommand);
}


status_t
IMAPMailbox::FetchBody(int32 messageNumber)
{
	if (fStorage.BodyFetched(MessageNumberToUID(messageNumber)))
		return B_BAD_VALUE;

	BPositionIO* file;
	status_t status = fStorage.OpenMessage(MessageNumberToUID(messageNumber),
		&file);
	if (status != B_OK)
		return status;

	// fetch command deletes the file
	status = FetchBody(messageNumber, file);
	return status;
}


status_t
IMAPMailbox::SetFlags(int32 messageNumber, int32 flags)
{
	if (messageNumber <= 0)
		return B_BAD_VALUE;

	SetFlagsCommand setFlagsCommand(*this, messageNumber, flags);
	return ProcessCommand(&setFlagsCommand);
}


status_t
IMAPMailbox::AppendMessage(BPositionIO& message, off_t size, int32 flags,
	time_t time)
{
	AppendCommand appendCommand(*this, message, size, flags, time);
	return ProcessCommand(&appendCommand);
}


int32
IMAPMailbox::UIDToMessageNumber(int32 uid)
{
	for (unsigned int i = 0; i < fMessageList.size(); i++) {
		if (fMessageList[i].uid == uid)
			return i + 1;
	}
	return -1;
}


int32
IMAPMailbox::MessageNumberToUID(int32 messageNumber)
{
	int32 index = messageNumber - 1;
	if (index < 0 || index >= (int32)fMessageList.size())
		return -1;
	return fMessageList[index].uid;
}


status_t
IMAPMailbox::DeleteMessage(int32 uid, bool permanently)
{
	int32 flags = fStorage.GetFlags(uid);
	flags |= kDeleted;
	status_t status = SetFlags(UIDToMessageNumber(uid), flags);

	if (!permanently || status != B_OK)
		return status;

	// delete permanently by invoking expunge
	ExpungeCommmand expungeCommand(*this);
	return ProcessCommand(&expungeCommand);
}


void
IMAPMailbox::_InstallUnsolicitedHandler(bool install)
{
	if (install) {
		fHandlerList.AddItem(&fFlagsHandler, 0);
		fHandlerList.AddItem(&fExpungeHandler, 0);
		fHandlerList.AddItem(&fExistsHandler, 0);
	} else {
		fHandlerList.RemoveItem(&fFlagsHandler);
		fHandlerList.RemoveItem(&fExpungeHandler);
		fHandlerList.RemoveItem(&fExistsHandler);
	}
}
