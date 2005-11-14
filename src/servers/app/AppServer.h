/*
 * Copyright (c) 2001-2005, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author: DarkWyrm <bpmagic@columbus.rr.com>
 */
#ifndef	APP_SERVER_H
#define	APP_SERVER_H


#include <OS.h>
#include <Locker.h>
#include <List.h>
#include <Application.h>
#include <Window.h>
#include <String.h>
#include <ObjectList.h>
#include <TokenSpace.h>

#include "ServerConfig.h"
#include "MessageLooper.h"

class ServerApp;
class BitmapManager;
class ColorSet;
class Desktop;

using BPrivate::BTokenSpace;

namespace BPrivate {
	class PortLink;
};


class AppServer : public MessageLooper  {
	public:
		AppServer();
		virtual ~AppServer();

		void			RunLooper();
		virtual port_id	MessagePort() const { return fMessagePort; }

	private:
		virtual void	_DispatchMessage(int32 code, BPrivate::LinkReceiver& link);

		Desktop*		_CreateDesktop(uid_t userID);
		Desktop*		_FindDesktop(uid_t userID);

		void			_LaunchCursorThread();
		void			_LaunchInputServer();

		static int32	_CursorThread(void *data);

	private:
		port_id			fMessagePort;
		port_id			fServerInputPort;

		thread_id		fISThreadID;
		thread_id		fCursorThreadID;
		sem_id			fCursorSem;
		area_id			fCursorArea;
		uint32			*fCursorAddr;

		BObjectList<Desktop> fDesktops;
		BLocker			fDesktopLock;

		port_id			fISASPort;
		port_id			fISPort;
};

extern BitmapManager *gBitmapManager;
extern ColorSet gGUIColorSet;
extern port_id gAppServerPort;

#endif	/* APP_SERVER_H */
