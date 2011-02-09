#ifndef MAIL_DAEMON_H
#define MAIL_DAEMON_H
/* Daemon - talking to the mail daemon
 *
 * Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
*/


const uint32 kMsgCheckAndSend = 'mbth';
const uint32 kMsgCheckMessage = 'mnow';
const uint32 kMsgSendMessages = 'msnd';
const uint32 kMsgSettingsUpdated = 'mrrs';
const uint32 kMsgAccountsChanged = 'macc';
const uint32 kMsgSetStatusWindowMode = 'shst';
const uint32 kMsgCountNewMessages = 'mnum';
const uint32 kMsgMarkMessageAsRead = 'mmar';


class BMailDaemon {
public:
	//! accountID = -1 means check all accounts
	static status_t				CheckMail(int32 accountID = -1);
	static status_t				CheckAndSendQueuedMail(int32 accountID = -1);
	static status_t				SendQueuedMail();
	static int32				CountNewMessages(
									bool waitForFetchCompletion = false);
	static status_t				MarkAsRead(int32 account, const entry_ref& ref,
									bool read = true);

	static status_t				Quit();
};

#endif	// MAIL_DAEMON_H
