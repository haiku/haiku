/* Daemon - talking to the mail daemon
**
** Copyright 2001-2002 Dr. Zoidberg Enterprises. All rights reserved.
*/


#include <Messenger.h>
#include <Message.h>
#include <List.h>

#include <MailDaemon.h>
#include <MailSettings.h>

#include <string.h>


status_t
BMailDaemon::CheckMail(int32 accountID)
{
	BMessenger daemon(B_MAIL_DAEMON_SIGNATURE);
	if (!daemon.IsValid())
		return B_MAIL_NO_DAEMON;

	BMessage message(kMsgCheckMessage);
	message.AddInt32("account", accountID);
	return daemon.SendMessage(&message);
}


status_t
BMailDaemon::CheckAndSendQueuedMail(int32 accountID)
{
	BMessenger daemon(B_MAIL_DAEMON_SIGNATURE);
	if (!daemon.IsValid())
		return B_MAIL_NO_DAEMON;

	BMessage message(kMsgCheckAndSend);
	message.AddInt32("account", accountID);
	return daemon.SendMessage(&message);
}


status_t
BMailDaemon::SendQueuedMail()
{
	BMessenger daemon(B_MAIL_DAEMON_SIGNATURE);
	if (!daemon.IsValid())
		return B_MAIL_NO_DAEMON;
	
	return daemon.SendMessage(kMsgSendMessages);
}


int32
BMailDaemon::CountNewMessages(bool wait_for_fetch_completion)
{
	BMessenger daemon(B_MAIL_DAEMON_SIGNATURE);
	if (!daemon.IsValid())
		return B_MAIL_NO_DAEMON;

	BMessage reply;
	BMessage first(kMsgCountNewMessages);

	if (wait_for_fetch_completion)
		first.AddBool("wait_for_fetch_done",true);
		
	daemon.SendMessage(&first, &reply);	
		
	return reply.FindInt32("num_new_messages");
}


status_t
BMailDaemon::MarkAsRead(int32 account, const entry_ref& ref, read_flags flag)
{
	BMessenger daemon(B_MAIL_DAEMON_SIGNATURE);
	if (!daemon.IsValid())
		return B_MAIL_NO_DAEMON;

	BMessage message(kMsgMarkMessageAsRead);
	message.AddInt32("account", account);
	message.AddRef("ref", &ref);
	message.AddInt32("read", flag);

	return daemon.SendMessage(&message);
}


status_t
BMailDaemon::FetchBody(const entry_ref& ref, BMessenger* listener)
{
	BMessenger daemon(B_MAIL_DAEMON_SIGNATURE);
	if (!daemon.IsValid())
		return B_MAIL_NO_DAEMON;

	BMessage message(kMsgFetchBody);
	message.AddRef("refs", &ref);
	if (listener != NULL)
		message.AddMessenger("target", *listener);

	BMessage reply;
	return daemon.SendMessage(&message, &reply);
}


status_t
BMailDaemon::Quit()
{
	BMessenger daemon(B_MAIL_DAEMON_SIGNATURE);
	if (!daemon.IsValid())
		return B_MAIL_NO_DAEMON;
	
	return daemon.SendMessage(B_QUIT_REQUESTED);
}
