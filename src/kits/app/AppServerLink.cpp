/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Erik Jaesler (erik@cgsoftware.com)
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <Application.h>
#include <Locker.h>

#include <AppServerLink.h>


/**	AppServerLink provides proxied access to the application's
 *	connection with the app_server.
 *	It has BAutolock semantics:
 *	creating one locks the app_server connection; destroying one
 *	unlocks the connection.
 */


BLocker sLock;

#include <stdio.h>
namespace BPrivate {

AppServerLink::AppServerLink(void)
{
	sLock.Lock();

	// if there is no be_app, we can't do a whole lot, anyway
	if (be_app) {
		fReceiver = &be_app->fServerLink->Receiver();
		fSender = &be_app->fServerLink->Sender();
	} else
		puts("Noooooooooooooooooooooooooooo");
}


AppServerLink::~AppServerLink()
{
	sLock.Unlock();
}

}	// namespace BPrivate
