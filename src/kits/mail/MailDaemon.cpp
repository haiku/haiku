/*
 * Copyright 2004-2012, Haiku Inc. All Rights Reserved.
 * Copyright 2001-2002 Dr. Zoidberg Enterprises. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */


#include <MailDaemon.h>

#include <List.h>
#include <MailSettings.h>
#include <Messenger.h>
#include <Message.h>
#include <Roster.h>

#include <MailPrivate.h>


using namespace BPrivate;


BMailDaemon::BMailDaemon()
	:
	fDaemon(B_MAIL_DAEMON_SIGNATURE)
{
}


BMailDaemon::~BMailDaemon()
{
}


bool
BMailDaemon::IsRunning()
{
	return fDaemon.IsValid();
}


status_t
BMailDaemon::CheckMail(int32 accountID)
{
	if (!fDaemon.IsValid())
		return B_MAIL_NO_DAEMON;

	BMessage message(kMsgCheckMessage);
	message.AddInt32("account", accountID);
	return fDaemon.SendMessage(&message);
}


status_t
BMailDaemon::CheckAndSendQueuedMail(int32 accountID)
{
	if (!fDaemon.IsValid())
		return B_MAIL_NO_DAEMON;

	BMessage message(kMsgCheckAndSend);
	message.AddInt32("account", accountID);
	return fDaemon.SendMessage(&message);
}


status_t
BMailDaemon::SendQueuedMail()
{
	if (!fDaemon.IsValid())
		return B_MAIL_NO_DAEMON;

	return fDaemon.SendMessage(kMsgSendMessages);
}


int32
BMailDaemon::CountNewMessages(bool waitForFetchCompletion)
{
	if (!fDaemon.IsValid())
		return B_MAIL_NO_DAEMON;

	BMessage reply;
	BMessage first(kMsgCountNewMessages);

	if (waitForFetchCompletion)
		first.AddBool("wait_for_fetch_done",true);

	fDaemon.SendMessage(&first, &reply);

	return reply.FindInt32("num_new_messages");
}


status_t
BMailDaemon::MarkAsRead(int32 account, const entry_ref& ref, read_flags flag)
{
	if (!fDaemon.IsValid())
		return B_MAIL_NO_DAEMON;

	BMessage message(kMsgMarkMessageAsRead);
	message.AddInt32("account", account);
	message.AddRef("ref", &ref);
	message.AddInt32("read", flag);

	return fDaemon.SendMessage(&message);
}


status_t
BMailDaemon::FetchBody(const entry_ref& ref, BMessenger* listener)
{
	if (!fDaemon.IsValid())
		return B_MAIL_NO_DAEMON;

	BMessage message(kMsgFetchBody);
	message.AddRef("refs", &ref);
	if (listener != NULL)
		message.AddMessenger("target", *listener);

	BMessage reply;
	return fDaemon.SendMessage(&message, &reply);
}


status_t
BMailDaemon::Quit()
{
	if (!fDaemon.IsValid())
		return B_MAIL_NO_DAEMON;

	return fDaemon.SendMessage(B_QUIT_REQUESTED);
}


status_t
BMailDaemon::Launch()
{
	status_t status = be_roster->Launch(B_MAIL_DAEMON_SIGNATURE);
	if (status == B_OK)
		fDaemon = BMessenger(B_MAIL_DAEMON_SIGNATURE);

	return status;
}
