/*
 * Copyright (c) 2001-2005, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author: DarkWyrm <bpmagic@columbus.rr.com>
 */

#ifndef	_HAIKU_APP_SERVER_H_
#define	_HAIKU_APP_SERVER_H_

#include <OS.h>
#include <Locker.h>
#include <List.h>
#include <Application.h>
#include <Window.h>
#include <String.h>

#include "ServerConfig.h"
#include "MessageLooper.h"

class ServerApp;
class BitmapManager;
class ColorSet;

namespace BPrivate {
	class PortLink;
};

/*!
	\class AppServer AppServer.h
	\brief main manager object for the app_server
	
	File for the main app_server thread. This particular thread monitors for
	application start and quit messages. It also starts the housekeeping threads
	and initializes most of the server's globals.
*/

class AppServer :
	public MessageLooper 
#if TEST_MODE
	, public BApplication
#endif
{
	public:
		AppServer();
		virtual ~AppServer();

		void			RunLooper();
		virtual port_id	MessagePort() const { return fMessagePort; }

	private:
		virtual void	_DispatchMessage(int32 code, BPrivate::LinkReceiver& link);
		void			LaunchCursorThread();
		void			LaunchInputServer();

//		static	int32	PollerThread(void *data);
		static	int32	CursorThread(void *data);

	private:
		port_id			fMessagePort;
		port_id			fServerInputPort;

		thread_id		fISThreadID;
		thread_id		fCursorThreadID;
		sem_id			fCursorSem;
		area_id			fCursorArea;
		uint32			*fCursorAddr;

		port_id			fISASPort;
		port_id			fISPort;
};

extern BitmapManager *gBitmapManager;
extern ColorSet gGUIColorSet;
extern port_id gAppServerPort;

#endif	/* _HAIKU_APP_SERVER_H_ */
