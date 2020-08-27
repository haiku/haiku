/*
 * Copyright 2001-2016, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Axel Dörfler, axeld@pinc-software.de
 *		Stephan Aßmus <superstippi@gmx.de>
 * 		Christian Packmann
 */


#include "AppServer.h"

#include <syslog.h>

#include <AutoDeleter.h>
#include <LaunchRoster.h>
#include <PortLink.h>

#include "BitmapManager.h"
#include "Desktop.h"
#include "FontManager.h"
#include "InputManager.h"
#include "ScreenManager.h"
#include "ServerProtocol.h"


//#define DEBUG_SERVER
#ifdef DEBUG_SERVER
#	include <stdio.h>
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif


// Globals
port_id gAppServerPort;
BTokenSpace gTokenSpace;
uint32 gAppServerSIMDFlags = 0;


/*!	\brief Constructor

	This loads the default fonts, allocates all the major global variables,
	spawns the main housekeeping threads, loads user preferences for the UI
	and decorator, and allocates various locks.
*/
AppServer::AppServer(status_t* status)
	:
	SERVER_BASE("application/x-vnd.Haiku-app_server", "picasso", -1, false,
		status),
	fDesktopLock("AppServerDesktopLock")
{
	openlog("app_server", 0, LOG_DAEMON);

	gInputManager = new InputManager();

	// Create the font server and scan the proper directories.
	gFontManager = new FontManager;
	if (gFontManager->InitCheck() != B_OK)
		debugger("font manager could not be initialized!");

	gFontManager->Run();

	gScreenManager = new ScreenManager();
	gScreenManager->Run();

	// Create the bitmap allocator. Object declared in BitmapManager.cpp
	gBitmapManager = new BitmapManager();

#ifndef HAIKU_TARGET_PLATFORM_LIBBE_TEST
	// TODO: check the attached displays, and launch login session for them
	BMessage data;
	data.AddString("name", "app_server");
	data.AddInt32("session", 0);
	BLaunchRoster().Target("login", data);
#endif
}


/*!	\brief Destructor
	Reached only when the server is asked to shut down in Test mode.
*/
AppServer::~AppServer()
{
	delete gBitmapManager;

	gScreenManager->Lock();
	gScreenManager->Quit();

	gFontManager->Lock();
	gFontManager->Quit();

	closelog();
}


void
AppServer::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case AS_GET_DESKTOP:
		{
			Desktop* desktop = NULL;

			int32 userID = message->GetInt32("user", 0);
			int32 version = message->GetInt32("version", 0);
			const char* targetScreen = message->GetString("target");

			if (version != AS_PROTOCOL_VERSION) {
				syslog(LOG_ERR, "Application for user %" B_PRId32 " does not "
					"support the current server protocol (%" B_PRId32 ").\n",
					userID, version);
			} else {
				desktop = _FindDesktop(userID, targetScreen);
				if (desktop == NULL) {
					// we need to create a new desktop object for this user
					// TODO: test if the user exists on the system
					// TODO: maybe have a separate AS_START_DESKTOP_SESSION for
					// authorizing the user
					desktop = _CreateDesktop(userID, targetScreen);
				}
			}

			BMessage reply;
			if (desktop != NULL)
				reply.AddInt32("port", desktop->MessagePort());
			else
				reply.what = (uint32)B_ERROR;

			message->SendReply(&reply);
			break;
		}

		default:
			// We don't allow application scripting
			STRACE(("AppServer received unexpected code %" B_PRId32 "\n",
				message->what));
			break;
	}
}


bool
AppServer::QuitRequested()
{
#if TEST_MODE
	while (fDesktops.CountItems() > 0) {
		Desktop *desktop = fDesktops.RemoveItemAt(0);

		thread_id thread = desktop->Thread();
		desktop->PostMessage(B_QUIT_REQUESTED);

		// we just wait for the desktop to kill itself
		status_t status;
		wait_for_thread(thread, &status);
	}

	delete this;
	exit(0);

	return SERVER_BASE::QuitRequested();
#else
	return false;
#endif

}


/*!	\brief Creates a desktop object for an authorized user
*/
Desktop*
AppServer::_CreateDesktop(uid_t userID, const char* targetScreen)
{
	BAutolock locker(fDesktopLock);
	ObjectDeleter<Desktop> desktop;
	try {
		desktop.SetTo(new Desktop(userID, targetScreen));

		status_t status = desktop->Init();
		if (status == B_OK)
			status = desktop->Run();
		if (status == B_OK && !fDesktops.AddItem(desktop.Get()))
			status = B_NO_MEMORY;

		if (status != B_OK) {
			syslog(LOG_ERR, "Cannot initialize Desktop object: %s\n",
				strerror(status));
			return NULL;
		}
	} catch (...) {
		// there is obviously no memory left
		return NULL;
	}

	return desktop.Detach();
}


/*!	\brief Finds the desktop object that belongs to a certain user
*/
Desktop*
AppServer::_FindDesktop(uid_t userID, const char* targetScreen)
{
	BAutolock locker(fDesktopLock);

	for (int32 i = 0; i < fDesktops.CountItems(); i++) {
		Desktop* desktop = fDesktops.ItemAt(i);

		if (desktop->UserID() == userID
			&& ((desktop->TargetScreen() == NULL && targetScreen == NULL)
				|| (desktop->TargetScreen() != NULL && targetScreen != NULL
					&& strcmp(desktop->TargetScreen(), targetScreen) == 0))) {
			return desktop;
		}
	}

	return NULL;
}


//	#pragma mark -


int
main(int argc, char** argv)
{
	srand(real_time_clock_usecs());

	status_t status;
	AppServer* server = new AppServer(&status);
	if (status == B_OK)
		server->Run();

	return status == B_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
