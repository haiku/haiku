/* 
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef _SYSLOG_DAEMON_H_
#define _SYSLOG_DAEMON_H_


#include <Application.h>
#include <Locker.h>
#include <List.h>
#include <OS.h>

#include <syslog_daemon.h>


typedef void (*handler_func)(syslog_message &);


class SyslogDaemon : public BApplication {
	public:
		SyslogDaemon();

		virtual void ReadyToRun();
		virtual void AboutRequested();
		virtual bool QuitRequested();
		virtual void MessageReceived(BMessage *msg);

		void AddHandler(handler_func function);

		void Daemon();
		static int32 daemon_thread(void *data);

	private:
		thread_id	fDaemon;
		port_id		fPort;

		BLocker		fHandlerLock;
		BList		fHandlers;
};

#endif	/* _SYSLOG_DAEMON_H_ */
