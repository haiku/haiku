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

_EXPORT status_t
BMailDaemon::CheckMail(bool send_queued_mail,const char *account)
{
	BMessenger daemon("application/x-vnd.Be-POST");
	if (!daemon.IsValid())
		return B_MAIL_NO_DAEMON;

	BMessage message(send_queued_mail ? 'mbth' : 'mnow');
	if (account != NULL) {
		BList list;

		GetInboundMailChains(&list);
		for (int32 i = list.CountItems();i-- > 0;) {
			BMailChain *chain = (BMailChain *)list.ItemAt(i);
			
			if (!strcmp(chain->Name(),account))
				message.AddInt32("chain",chain->ID());
			
			delete chain;
		}
		
		if (send_queued_mail) {
			list.MakeEmpty();
			GetOutboundMailChains(&list);
			for (int32 i = list.CountItems();i-- > 0;) {
				BMailChain *chain = (BMailChain *)list.ItemAt(i);
				
				if (!strcmp(chain->Name(),account))
					message.AddInt32("chain",chain->ID());
				
				delete chain;
			}
		}
	}
	daemon.SendMessage(&message);

	return B_OK;
}


_EXPORT status_t BMailDaemon::SendQueuedMail() {
	BMessenger daemon("application/x-vnd.Be-POST");
	if (!daemon.IsValid())
		return B_MAIL_NO_DAEMON;
	
	daemon.SendMessage('msnd');
		
	return B_OK;
}

_EXPORT int32 BMailDaemon::CountNewMessages(bool wait_for_fetch_completion) {
	BMessenger daemon("application/x-vnd.Be-POST");
	if (!daemon.IsValid())
		return B_MAIL_NO_DAEMON;
	
	BMessage reply;
	BMessage first('mnum');
	
	if (wait_for_fetch_completion)
		first.AddBool("wait_for_fetch_done",true);
		
	daemon.SendMessage(&first,&reply);	
		
	return reply.FindInt32("num_new_messages");
}

_EXPORT status_t BMailDaemon::Quit() {
	BMessenger daemon("application/x-vnd.Be-POST");
	if (!daemon.IsValid())
		return B_MAIL_NO_DAEMON;
	
	daemon.SendMessage(B_QUIT_REQUESTED);
		
	return B_OK;
}

