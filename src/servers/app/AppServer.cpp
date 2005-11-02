/*
 * Copyright (c) 2001-2005, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Axel Dörfler, axeld@pinc-software.de
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include <unistd.h>

#include <Accelerant.h>
#include <AppDefs.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <Message.h>
#include <Path.h>
#include <PortLink.h>
#include <StopWatch.h>
#include <RosterPrivate.h>
#include <Autolock.h>

#include "BitmapManager.h"
#include "ColorSet.h"
#include "CursorManager.h"
#include "DecorManager.h"
#include "DefaultDecorator.h"
#include "Desktop.h"
#include "FontManager.h"
#include "HWInterface.h"
#include "RegistrarDefs.h"
#include "RGBColor.h"
#include "Layer.h"
#include "WinBorder.h"
#include "RootLayer.h"
#include "ScreenManager.h"
#include "ServerApp.h"
#include "ServerConfig.h"
#include "ServerCursor.h"
#include "ServerProtocol.h"
#include "ServerWindow.h"
#include "SystemPalette.h"
#include "Utils.h"

#include "AppServer.h"

//#define DEBUG_KEYHANDLING
//#define DEBUG_SERVER

#ifdef DEBUG_KEYHANDLING
#	include <stdio.h>
#	define KBTRACE(x) printf x
#else
#	define KBTRACE(x) ;
#endif

#ifdef DEBUG_SERVER
#	include <stdio.h>
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif


// Globals
port_id gAppServerPort;
static AppServer *sAppServer;

//! System-wide GUI color object
ColorSet gGUIColorSet;

/*!
	\brief Constructor
	
	This loads the default fonts, allocates all the major global variables, spawns the main housekeeping
	threads, loads user preferences for the UI and decorator, and allocates various locks.
*/
AppServer::AppServer()
	: MessageLooper("app_server"),
	fCursorSem(-1),
	fCursorArea(-1)
{
	fMessagePort = create_port(DEFAULT_MONITOR_PORT_SIZE, SERVER_PORT_NAME);
	if (fMessagePort < B_OK)
		debugger("app_server could not create message port");

	fLink.SetReceiverPort(fMessagePort);
	gAppServerPort = fMessagePort;

	// Create the input port. The input_server will send it's messages here.
	// TODO: If we want multiple user support we would need an individual
	// port for each user and do the following for each RootLayer.
	fServerInputPort = create_port(200, SERVER_INPUT_PORT);
	if (fServerInputPort < B_OK)
		debugger("app_server could not create input port");

	sAppServer = this;

	// Create the font server and scan the proper directories.
	gFontManager = new FontManager;
	gFontManager->Lock();
	gFontManager->ScanSystemFolders();

	if (!gFontManager->SetSystemPlain(DEFAULT_PLAIN_FONT_FAMILY,
									DEFAULT_PLAIN_FONT_STYLE,
									DEFAULT_PLAIN_FONT_SIZE) &&
		!gFontManager->SetSystemPlain(FALLBACK_PLAIN_FONT_FAMILY,
									DEFAULT_PLAIN_FONT_STYLE,
									DEFAULT_PLAIN_FONT_SIZE)) {
		printf("Couldn't set plain to %s (fallback: %s), %s %d pt\n",
				DEFAULT_PLAIN_FONT_FAMILY,
				FALLBACK_PLAIN_FONT_FAMILY,
				DEFAULT_PLAIN_FONT_STYLE,
				DEFAULT_PLAIN_FONT_SIZE);
	}

	if (!gFontManager->SetSystemBold(DEFAULT_BOLD_FONT_FAMILY,
								   DEFAULT_BOLD_FONT_STYLE,
								   DEFAULT_BOLD_FONT_SIZE) &&
		!gFontManager->SetSystemBold(FALLBACK_BOLD_FONT_FAMILY,
								   DEFAULT_BOLD_FONT_STYLE,
								   DEFAULT_BOLD_FONT_SIZE)) {
		printf("Couldn't set bold to %s (fallback: %s), %s %d pt\n",
				DEFAULT_BOLD_FONT_FAMILY,
				FALLBACK_BOLD_FONT_FAMILY,
				DEFAULT_BOLD_FONT_STYLE,
				DEFAULT_BOLD_FONT_SIZE);
	}

	if (!gFontManager->SetSystemFixed(DEFAULT_FIXED_FONT_FAMILY,
									DEFAULT_FIXED_FONT_STYLE,
									DEFAULT_FIXED_FONT_SIZE) &&
		!gFontManager->SetSystemFixed(FALLBACK_FIXED_FONT_FAMILY,
									DEFAULT_FIXED_FONT_STYLE,
									DEFAULT_FIXED_FONT_SIZE)) {
		printf("Couldn't set fixed to %s (fallback: %s), %s %d pt\n",
				DEFAULT_FIXED_FONT_FAMILY,
				FALLBACK_FIXED_FONT_FAMILY,
				DEFAULT_FIXED_FONT_STYLE,
				DEFAULT_FIXED_FONT_SIZE);
	}

	gFontManager->Unlock();

	// Load the GUI colors here and set the global set to the values contained therein. If this
	// is not possible, set colors to the defaults
	if (LoadColorSet(SERVER_SETTINGS_DIR COLOR_SETTINGS_NAME, &gGUIColorSet) != B_OK)
		gGUIColorSet.SetToDefaults();

	gScreenManager = new ScreenManager();
	gScreenManager->Run();

	// the system palette needs to be initialized before the desktop,
	// since it is used there already
	InitializeColorMap();
	
	// Create the bitmap allocator. Object declared in BitmapManager.cpp
	gBitmapManager = new BitmapManager();

#if 0
	_LaunchCursorThread();
#endif
}

/*!
	\brief Destructor
	Reached only when the server is asked to shut down in Test mode.
*/
AppServer::~AppServer()
{
	delete gBitmapManager;

	gScreenManager->Lock();
	gScreenManager->Quit();

	delete gFontManager;
}


/*!
	\brief The call that starts it all...
*/
void
AppServer::RunLooper()
{
	rename_thread(find_thread(NULL), "picasso");
	_message_thread((void *)this);
}


/*!
	\brief Starts Input Server
*/
void
AppServer::_LaunchInputServer()
{
	// We are supposed to start the input_server, but it's a BApplication
	// that depends on the registrar running, which is started after app_server
	// so we wait on roster thread to launch the input server

	fISThreadID = B_ERROR;

	while (!BRoster::Private().IsMessengerValid(false) && !fQuitting) {
		snooze(250000);
		BRoster::Private::DeleteBeRoster();
		BRoster::Private::InitBeRoster();
	}

	if (fQuitting)
		return;

	// we use an area for cursor data communication with input_server
	// area id and sem id are sent to the input_server

	if (fCursorArea < B_OK)	
		fCursorArea = create_area("isCursor", (void**) &fCursorAddr, B_ANY_ADDRESS, B_PAGE_SIZE, B_FULL_LOCK, B_READ_AREA | B_WRITE_AREA);
	if (fCursorSem < B_OK)
		fCursorSem = create_sem(0, "isSem"); 

	int32 arg_c = 1;
	char **arg_v = (char **)malloc(sizeof(char *) * (arg_c + 1));
#if TEST_MODE
	arg_v[0] = strdup("/boot/home/svnhaiku/trunk/distro/x86.R1/beos/system/servers/input_server");
#else
	arg_v[0] = strdup("/system/servers/input_server");
#endif
	arg_v[1] = NULL;
	fISThreadID = load_image(arg_c, (const char**)arg_v, (const char **)environ);
	free(arg_v[0]);

	int32 tmpbuf[2] = {fCursorSem, fCursorArea};
	int32 code = 0;
	send_data(fISThreadID, code, (void *)tmpbuf, sizeof(tmpbuf));   

	resume_thread(fISThreadID);
	setpgid(fISThreadID, 0); 

	// we receive 

	thread_id sender;
	code = receive_data(&sender, (void *)tmpbuf, sizeof(tmpbuf));
	fISASPort = tmpbuf[0];
	fISPort = tmpbuf[1];  

	// if at any time, one of these ports is error prone, it might mean input_server is gone
	// then relaunch input_server
}


/*!
	\brief Starts the Cursor Thread
*/
void
AppServer::_LaunchCursorThread()
{
	// Spawn our cursor thread
	fCursorThreadID = spawn_thread(_CursorThread, "CursorThreadOfTheDeath",
		B_REAL_TIME_DISPLAY_PRIORITY - 1, this);
	if (fCursorThreadID >= 0)
		resume_thread(fCursorThreadID);

}

/*!
	\brief The Cursor Thread task
*/
int32
AppServer::_CursorThread(void* data)
{
	AppServer *server = (AppServer *)data;

	server->_LaunchInputServer();

	do {
		while (acquire_sem(server->fCursorSem) == B_OK) {
			BPoint p;
			p.y = *server->fCursorAddr & 0x7fff;
			p.x = *server->fCursorAddr >> 15 & 0x7fff;

			//sDesktop->GetHWInterface()->MoveCursorTo(p.x, p.y);
			STRACE(("CursorThread : %f, %f\n", p.x, p.y));
		}

		snooze(100000);
	} while (!server->IsQuitting());

	return B_OK;
}


/*!
	\brief Creates a desktop object for an authorized user
*/
Desktop *
AppServer::_CreateDesktop(uid_t userID)
{
	BAutolock locker(fDesktopLock);
	Desktop* desktop = NULL;
	try {
		desktop = new Desktop(userID);

		desktop->Init();
		desktop->Run();

		if (!fDesktops.AddItem(desktop)) {
			delete desktop;
			return NULL;
		}
	} catch (...) {
		// there is obviously no memory left
		return NULL;
	}

	return desktop;
}


/*!
	\brief Finds the desktop object that belongs to a certain user
*/
Desktop *
AppServer::_FindDesktop(uid_t userID)
{
	BAutolock locker(fDesktopLock);
	
	for (int32 i = 0; i < fDesktops.CountItems(); i++) {
		Desktop* desktop = fDesktops.ItemAt(i);

		if (desktop->UserID() == userID)
			return desktop;
	}

	return NULL;
}


/*!
	\brief Message handling function for all messages sent to the app_server
	\param code ID of the message sent
	\param buffer Attachment buffer for the message.
	
*/
void
AppServer::_DispatchMessage(int32 code, BPrivate::LinkReceiver& msg)
{
	switch (code) {
		case AS_GET_DESKTOP:
		{
			port_id replyPort;
			if (msg.Read<port_id>(&replyPort) < B_OK)
				break;

			int32 userID;
			msg.Read<int32>(&userID);

			Desktop* desktop = _FindDesktop(userID);
			if (desktop == NULL) {
				// we need to create a new desktop object for this user
				// ToDo: test if the user exists on the system
				// ToDo: maybe have a separate AS_START_DESKTOP_SESSION for authorizing the user
				desktop = _CreateDesktop(userID);
			}

			BPrivate::LinkSender reply(replyPort);
			reply.StartMessage(B_OK);
			reply.Attach<port_id>(desktop->MessagePort());
			reply.Flush();
			break;
		}

		case AS_QUERY_FONTS_CHANGED:
		{
			// Client application is asking if the font list has changed since
			// the last client-side refresh

			// Attached data:
			// 1) port_id reply port

			gFontManager->Lock();
			bool needsUpdate = gFontManager->FontsNeedUpdated();
			gFontManager->Unlock();

			// Seeing how the client merely wants an answer, we'll skip the BPortLink
			// and all its overhead and just write the code to port.
			port_id replyPort;
			if (msg.Read<port_id>(&replyPort) < B_OK)
				break;

			BPrivate::PortLink reply(replyPort);
			reply.StartMessage(needsUpdate ? SERVER_TRUE : SERVER_FALSE);
			reply.Flush();
			break;
		}

#if TEST_MODE
		case B_QUIT_REQUESTED:
		{
			// We've been asked to quit, so (for now) broadcast to all
			// desktops to quit. This situation will occur only when the server
			// is compiled as a regular Be application.

			fQuitting = true;

			while (fDesktops.CountItems() > 0) {
				Desktop *desktop = fDesktops.RemoveItemAt(0);

				thread_id thread = desktop->Thread();
				desktop->PostMessage(B_QUIT_REQUESTED);

				// we just wait for the desktop to kill itself
				status_t status;
				wait_for_thread(thread, &status);
			}

			kill_thread(fCursorThreadID);
			delete this;

			// we are now clear to exit
			exit(0);
			break;
		}
#endif

		default:
			STRACE(("Server::MainLoop received unexpected code %ld (offset %ld)\n",
				code, code - SERVER_TRUE));
			break;
	}
}


//	#pragma mark -


/*!
	\brief Entry function to run the entire server
	\param argc Number of command-line arguments present
	\param argv String array of the command-line arguments
	\return -1 if the app_server is already running, 0 if everything's OK.
*/
int
main(int argc, char** argv)
{
	// There can be only one....
	if (find_port(SERVER_PORT_NAME) >= B_OK)
		return -1;

	srand(real_time_clock_usecs());

	AppServer* server = new AppServer;
	server->RunLooper();

	return 0;
}
