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

class Layer;
class BMessage;
class ServerApp;
class DisplayDriver;
class CursorManager;
class BitmapManager;
class DecorManager;
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

class AppServer
#if TEST_MODE
	: public BApplication
#endif
{
public:
						AppServer(void);
						~AppServer(void);

	static	int32		PollerThread(void *data);
	static	int32		PicassoThread(void *data);
			thread_id	Run(void);
			void 		MainLoop(void);
	
			void		DispatchMessage(int32 code, BPrivate::PortLink &link);
			ServerApp*	FindApp(const char *sig);

private:
	friend		void			BroadcastToAllApps(const int32 &code);
	
				void			LaunchCursorThread();
				void			LaunchInputServer();
	static		int32			CursorThread(void *data);

				port_id			fMessagePort;
				port_id			fServerInputPort;
	
	volatile	bool			fQuittingServer;
	
				BList			*fAppList;
				thread_id		fPicassoThreadID;

				thread_id		fISThreadID;
				thread_id		fCursorThreadID;
				sem_id			fCursorSem;
				area_id			fCursorArea;
				uint32			*fCursorAddr;

				port_id			fISASPort;
				port_id			fISPort;
	
				sem_id			fActiveAppLock;
				sem_id			fAppListLock;
				sem_id			fDecoratorLock;
	
				DisplayDriver	*fDriver;
};

extern BitmapManager *bitmapmanager;
extern ColorSet gui_colorset;
extern AppServer *app_server;
extern port_id gAppServerPort;

#endif	/* _HAIKU_APP_SERVER_H_ */
