#ifndef ZOIDBERG_MAIL_DAEMON_H
#define ZOIDBERG_MAIL_DAEMON_H
/* Daemon - talking to the mail daemon
**
** Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
*/

class BMailDaemon {
	public:
		static status_t CheckMail(bool send_queued_mail = true,const char *account = NULL);
		static status_t SendQueuedMail();
		static int32	CountNewMessages(bool wait_for_fetch_completion = false);

		static status_t Quit();
};

#endif	/* ZOIDBERG_MAIL_DAEMON_H */

