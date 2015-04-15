/*
 * Copyright 2001-2015, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef	APP_SERVER_H
#define	APP_SERVER_H


#include <Application.h>
#include <List.h>
#include <Locker.h>
#include <ObjectList.h>
#include <OS.h>
#include <Server.h>
#include <String.h>
#include <Window.h>

#include "MessageLooper.h"
#include "ServerConfig.h"


class ServerApp;
class BitmapManager;
class Desktop;


class AppServer : public BServer  {
public:
								AppServer(status_t* status);
	virtual						~AppServer();

	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();

private:
			Desktop*			_CreateDesktop(uid_t userID,
									const char* targetScreen);
			Desktop*			_FindDesktop(uid_t userID,
									const char* targetScreen);

			void				_LaunchInputServer();

private:
			BObjectList<Desktop> fDesktops;
			BLocker				fDesktopLock;
};


extern BitmapManager *gBitmapManager;
extern port_id gAppServerPort;


#endif	/* APP_SERVER_H */
