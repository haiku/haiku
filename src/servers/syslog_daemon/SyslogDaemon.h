/*
 * Copyright 2003-2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SYSLOG_DAEMON_H_
#define _SYSLOG_DAEMON_H_


#include <Application.h>
#include <Locker.h>
#include <List.h>
#include <OS.h>

#include <syslog_daemon.h>


typedef void (*handler_func)(syslog_message&);


class SyslogDaemon : public BApplication {
public:
								SyslogDaemon();

	virtual	void				ReadyToRun();
	virtual	void				AboutRequested();
	virtual	bool				QuitRequested();
	virtual	void				MessageReceived(BMessage* message);

			void				AddHandler(handler_func function);

private:
			void				_Daemon();
	static	int32				_DaemonThread(void* data);

private:
			thread_id			fDaemon;
			port_id				fPort;

			BLocker				fHandlerLock;
			BList				fHandlers;
};


#endif	/* _SYSLOG_DAEMON_H_ */
