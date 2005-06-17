/*
 * Copyright (c) 2001-2005, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author: DarkWyrm <bpmagic@columbus.rr.com>
 */
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

#include "BitmapManager.h"
#include "ColorSet.h"
#include "CursorManager.h"
#include "DecorManager.h"
#include "DefaultDecorator.h"
#include "Desktop.h"
#include "DisplayDriver.h"
#include "FontServer.h"
#include "RegistrarDefs.h"
#include "RGBColor.h"
#include "RootLayer.h"
#include "ServerApp.h"
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
Desktop *gDesktop;

port_id gAppServerPort;

static AppServer *sAppServer;

//! Default background color for workspaces
//RGBColor workspace_default_color(51,102,160);

//! System-wide GUI color object
ColorSet gui_colorset;

/*!
	\brief Constructor
	
	This loads the default fonts, allocates all the major global variables, spawns the main housekeeping
	threads, loads user preferences for the UI and decorator, and allocates various locks.
*/
#if TEST_MODE
AppServer::AppServer(void) : BApplication (SERVER_SIGNATURE),
#else
AppServer::AppServer(void) : 
#endif
	fCursorSem(-1),
	fCursorArea(-1)
{
	fMessagePort = create_port(200, SERVER_PORT_NAME);
	if (fMessagePort == B_NO_MORE_PORTS)
		debugger("app_server could not create message port");

	gAppServerPort = fMessagePort;

	// Create the input port. The input_server will send it's messages here.
	// TODO: If we want multiple user support we would need an individual
	// port for each user and do the following for each RootLayer.
	fServerInputPort = create_port(200, SERVER_INPUT_PORT);
	if (fServerInputPort == B_NO_MORE_PORTS)
		debugger("app_server could not create input port");

	fAppList = new BList();
	fQuittingServer = false;
	sAppServer = this;
	
	// Create the font server and scan the proper directories.
	fontserver = new FontServer;
	fontserver->Lock();

	// Used for testing purposes

	// TODO: Re-enable scanning of all font directories when server is actually put to use
	fontserver->ScanDirectory("/boot/beos/etc/fonts/ttfonts/");
//	fontserver->ScanDirectory("/boot/beos/etc/fonts/PS-Type1/");
//	fontserver->ScanDirectory("/boot/home/config/fonts/ttfonts/");
//	fontserver->ScanDirectory("/boot/home/config/fonts/psfonts/");
	fontserver->SaveList();

	if (!fontserver->SetSystemPlain(DEFAULT_PLAIN_FONT_FAMILY,
									DEFAULT_PLAIN_FONT_STYLE,
									DEFAULT_PLAIN_FONT_SIZE) &&
		!fontserver->SetSystemPlain(FALLBACK_PLAIN_FONT_FAMILY,
									DEFAULT_PLAIN_FONT_STYLE,
									DEFAULT_PLAIN_FONT_SIZE)) {
		printf("Couldn't set plain to %s (fallback: %s), %s %d pt\n",
				DEFAULT_PLAIN_FONT_FAMILY,
				FALLBACK_PLAIN_FONT_FAMILY,
				DEFAULT_PLAIN_FONT_STYLE,
				DEFAULT_PLAIN_FONT_SIZE);
	}

	if (!fontserver->SetSystemBold(DEFAULT_BOLD_FONT_FAMILY,
								   DEFAULT_BOLD_FONT_STYLE,
								   DEFAULT_BOLD_FONT_SIZE) &&
		!fontserver->SetSystemBold(FALLBACK_BOLD_FONT_FAMILY,
								   DEFAULT_BOLD_FONT_STYLE,
								   DEFAULT_BOLD_FONT_SIZE)) {
		printf("Couldn't set bold to %s (fallback: %s), %s %d pt\n",
				DEFAULT_BOLD_FONT_FAMILY,
				FALLBACK_BOLD_FONT_FAMILY,
				DEFAULT_BOLD_FONT_STYLE,
				DEFAULT_BOLD_FONT_SIZE);
	}

	if (!fontserver->SetSystemFixed(DEFAULT_FIXED_FONT_FAMILY,
									DEFAULT_FIXED_FONT_STYLE,
									DEFAULT_FIXED_FONT_SIZE) &&
		!fontserver->SetSystemFixed(FALLBACK_FIXED_FONT_FAMILY,
									DEFAULT_FIXED_FONT_STYLE,
									DEFAULT_FIXED_FONT_SIZE)) {
		printf("Couldn't set fixed to %s (fallback: %s), %s %d pt\n",
				DEFAULT_FIXED_FONT_FAMILY,
				FALLBACK_FIXED_FONT_FAMILY,
				DEFAULT_FIXED_FONT_STYLE,
				DEFAULT_FIXED_FONT_SIZE);
	}

	fontserver->Unlock();

	// Load the GUI colors here and set the global set to the values contained therein. If this
	// is not possible, set colors to the defaults
	if (LoadColorSet(SERVER_SETTINGS_DIR COLOR_SETTINGS_NAME,&gui_colorset)!=B_OK)
		gui_colorset.SetToDefaults();

	// Set up the Desktop
	gDesktop = new Desktop();
	gDesktop->Init();

	// TODO: Maybe this is not the best place for this
	InitializeColorMap();
	
	// Create the bitmap allocator. Object declared in BitmapManager.cpp
	bitmapmanager = new BitmapManager();

	// This is necessary to mediate access between the Poller and app_server threads
	fActiveAppLock = create_sem(1,"app_server_active_sem");

	// This locker is for app_server and Picasso to vy for control of the ServerApp list
	fAppListLock = create_sem(1,"app_server_applist_sem");

	// Spawn our thread-monitoring thread
	fPicassoThreadID = spawn_thread(PicassoThread, "picasso", B_NORMAL_PRIORITY, this);
	if (fPicassoThreadID >= 0)
		resume_thread(fPicassoThreadID);

#if 0
	LaunchCursorThread();
#endif
}

/*!
	\brief Destructor
	
	Reached only when the server is asked to shut down in Test mode. Kills all apps, shuts down the 
	desktop, kills the housekeeping threads, etc.
*/
AppServer::~AppServer(void)
{
	debugger("We shouldn't be here! MainLoop()::B_QUIT_REQUESTED should see if we can exit the server.\n");
/*
	ServerApp *tempapp;
	int32 i;
	acquire_sem(fAppListLock);
	for(i=0;i<fAppList->CountItems();i++)
	{
		tempapp=(ServerApp *)fAppList->ItemAt(i);
		if(tempapp!=NULL)
			delete tempapp;
	}
	delete fAppList;
	release_sem(fAppListLock);

	delete bitmapmanager;

	delete gDesktop;

	// If these threads are still running, kill them - after this, if exit_poller
	// is deleted, who knows what will happen... These things will just return an
	// error and fail if the threads have already exited.
	kill_thread(fPicassoThreadID);
	kill_thread(fCursorThreadID);
	kill_thread(fISThreadID);

	delete fontserver;
	
	make_decorator=NULL;
*/
}

/*!
	\brief Thread function for watching for dead apps
	\param data Pointer to the app_server to which the thread belongs
	\return Throwaway value - always 0
*/
int32
AppServer::PicassoThread(void *data)
{
	for (;;) {
		acquire_sem(sAppServer->fAppListLock);
		for (int32 i = 0;;) {
			ServerApp *app = (ServerApp *)sAppServer->fAppList->ItemAt(i++);
			if (!app)
				break;

			app->PingTarget();
		}
		release_sem(sAppServer->fAppListLock);		
		// we do this every other second so as not to suck *too* many CPU cycles
		snooze(1000000);
	}
	return 0;
}


/*!
	\brief Starts Input Server
*/
void
AppServer::LaunchInputServer()
{
	// We are supposed to start the input_server, but it's a BApplication
	// that depends on the registrar running, which is started after app_server
	// so we wait on roster thread to launch the input server

	fISThreadID = B_ERROR;

	while (!BRoster::Private().IsMessengerValid(false) && !fQuittingServer) {
		snooze(250000);
		BRoster::Private::DeleteBeRoster();
		BRoster::Private::InitBeRoster();
	}

	if (fQuittingServer)
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
AppServer::LaunchCursorThread()
{
	// Spawn our cursor thread
	fCursorThreadID = spawn_thread(CursorThread, "CursorThreadOfTheDeath", B_REAL_TIME_DISPLAY_PRIORITY - 1, this);
	if (fCursorThreadID >= 0)
		resume_thread(fCursorThreadID);

}

/*!
	\brief The Cursor Thread task
*/
int32
AppServer::CursorThread(void* data)
{
	AppServer *app = (AppServer *)data;

	BPoint p;
	
	app->LaunchInputServer();

	do {

		while (acquire_sem(app->fCursorSem) == B_OK) {

			p.y = *app->fCursorAddr & 0x7fff;
			p.x = *app->fCursorAddr >> 15 & 0x7fff;

			gDesktop->GetDisplayDriver()->MoveCursorTo(p.x, p.y);
			STRACE(("CursorThread : %f, %f\n", p.x, p.y));
		}

		snooze(100000);
		
	} while (!app->fQuittingServer);

	return B_OK;
}


/*!
	\brief The call that starts it all...
	\return Always 0
*/
thread_id
AppServer::Run(void)
{
	MainLoop();
	kill_thread(fPicassoThreadID);
	return 0;
}


//! Main message-monitoring loop for the regular message port - no input messages!
void
AppServer::MainLoop(void)
{
	BPrivate::PortLink pmsg(-1, fMessagePort);

	while (1) {
		STRACE(("info: AppServer::MainLoop listening on port %ld.\n", fMessagePort));

		int32 code;
		status_t err = pmsg.GetNextMessage(code);
		if (err < B_OK) {
			STRACE(("MainLoop:pmsg.GetNextMessage() failed\n"));
			continue;
		}

		switch (code) {
			case B_QUIT_REQUESTED:
			case AS_CREATE_APP:
			case AS_DELETE_APP:
			case AS_UPDATED_CLIENT_FONTLIST:
			case AS_QUERY_FONTS_CHANGED:
			case AS_SET_UI_COLORS:
			case AS_GET_UI_COLOR:
			case AS_SET_DECORATOR:
			case AS_GET_DECORATOR:
			case AS_R5_SET_DECORATOR:
				DispatchMessage(code, pmsg);
				break;
			default:
			{
				STRACE(("Server::MainLoop received unexpected code %ld (offset %ld)\n",
						code, code - SERVER_TRUE));
				break;
			}
		}
	}
}

/*!
	\brief Message handling function for all messages sent to the app_server
	\param code ID of the message sent
	\param buffer Attachment buffer for the message.
	
*/
void
AppServer::DispatchMessage(int32 code, BPrivate::PortLink &msg)
{
	switch (code) {
		case AS_CREATE_APP:
		{
			// Create the ServerApp to node monitor a new BApplication
			
			// Attached data:
			// 1) port_id - receiver port of a regular app
			// 2) port_id - client looper port - for send messages to the client
			// 2) team_id - app's team ID
			// 3) int32 - handler token of the regular app
			// 4) char * - signature of the regular app

			// Find the necessary data
			team_id	clientTeamID = -1;
			port_id	clientLooperPort = -1;
			port_id clientReplyPort = -1;
			int32 htoken = B_NULL_TOKEN;
			char *appSignature = NULL;

			msg.Read<port_id>(&clientReplyPort);
			msg.Read<port_id>(&clientLooperPort);
			msg.Read<team_id>(&clientTeamID);
			msg.Read<int32>(&htoken);
			if (msg.ReadString(&appSignature) != B_OK)
				break;

			port_id serverListen = create_port(DEFAULT_MONITOR_PORT_SIZE, appSignature);
			if (serverListen < B_OK) {
				printf("No more ports left. Time to crash. Have a nice day! :)\n");
				break;
			}

			// we let the application own the port, so that we get aware when it's gone
			if (set_port_owner(serverListen, clientTeamID) < B_OK) {
				delete_port(serverListen);
				printf("Could not transfer port ownership to client %ld!\n", clientTeamID);
				break;
			}

			// Create the ServerApp subthread for this app
			acquire_sem(fAppListLock);

			ServerApp *app = new ServerApp(clientReplyPort, serverListen, clientLooperPort,
				clientTeamID, htoken, appSignature);

			// add the new ServerApp to the known list of ServerApps
			fAppList->AddItem(app);

			release_sem(fAppListLock);

			BPrivate::PortLink replylink(clientReplyPort);
			replylink.StartMessage(SERVER_TRUE);
			replylink.Attach<int32>(serverListen);
			replylink.Flush();

			// This is necessary because BPortLink::ReadString allocates memory
			free(appSignature);
			break;
		}
		case AS_DELETE_APP:
		{
			// Delete a ServerApp. Received only from the respective ServerApp when a
			// BApplication asks it to quit.

			// Attached Data:
			// 1) thread_id - thread ID of the ServerApp to be deleted

			int32 i = 0, appnum = fAppList->CountItems();
			ServerApp *srvapp = NULL;
			thread_id srvapp_id = -1;

			if (msg.Read<thread_id>(&srvapp_id) < B_OK)
				break;

			acquire_sem(fAppListLock);

			// Run through the list of apps and nuke the proper one
			for (i = 0; i < appnum; i++) {
				srvapp = (ServerApp *)fAppList->ItemAt(i);

				if (srvapp != NULL && srvapp->MonitorThreadID() == srvapp_id) {
					srvapp = (ServerApp *)fAppList->RemoveItem(i);
					if (srvapp) {
						delete srvapp;
						srvapp = NULL;
					}
					break;	// jump out of our for() loop
				}
			}

			release_sem(fAppListLock);
			break;
		}
		case AS_UPDATED_CLIENT_FONTLIST:
		{
			// received when the client-side global font list has been
			// refreshed
			fontserver->Lock();
			fontserver->FontsUpdated();
			fontserver->Unlock();
			break;
		}
		case AS_QUERY_FONTS_CHANGED:
		{
			// Client application is asking if the font list has changed since
			// the last client-side refresh

			// Attached data:
			// 1) port_id reply port
			
			fontserver->Lock();
			bool needs_update=fontserver->FontsNeedUpdated();
			fontserver->Unlock();
			
			// Seeing how the client merely wants an answer, we'll skip the BPortLink
			// and all its overhead and just write the code to port.
			port_id replyport;
			if (msg.Read<port_id>(&replyport) < B_OK)
				break;
			BPrivate::PortLink replylink(replyport);
			replylink.StartMessage(needs_update ? SERVER_TRUE : SERVER_FALSE);
			replylink.Flush();
			
			break;
		}
		case B_QUIT_REQUESTED:
		{
#if TEST_MODE
			// Attached Data:
			// none
			
			// We've been asked to quit, so (for now) broadcast to all
			// test apps to quit. This situation will occur only when the server
			// is compiled as a regular Be application.

			BroadcastToAllApps(AS_QUIT_APP);

			// we have to wait until *all* threads have finished!
			ServerApp *app;
			acquire_sem(fAppListLock);
			thread_info tinfo;
			for (int32 i = 0; i < fAppList->CountItems(); i++) {
				app = (ServerApp*)fAppList->ItemAt(i);
				if (!app)
					continue;

				// Instead of calling wait_for_thread, we will wait a bit, check for the
				// thread_id. We will only wait so long, because then the app is probably crashed
				// or hung. Seeing that being the case, we'll kill its BApp team and fake the
				// quit message
				if (get_thread_info(app->MonitorThreadID(), &tinfo)==B_OK) {
					bool killteam = true;

					for (int32 j = 0; j < 5; j++) {
						snooze(500000);	// wait half a second for it to quit
						if (get_thread_info(app->MonitorThreadID(), &tinfo) != B_OK) {
							killteam = false;
							break;
						}
					}

					if (killteam) {
						kill_team(app->ClientTeamID());
						app->PostMessage(B_QUIT_REQUESTED);
					}
				}
			}

			kill_thread(fPicassoThreadID);

			release_sem(fAppListLock);

			delete gDesktop;
			delete fAppList;
			delete bitmapmanager;
			delete fontserver;

			// we are now clear to exit
			exit_thread(0);
#endif
			break;
		}
		case AS_SET_SYSCURSOR_DEFAULTS:
		{
			// although this isn't pretty, ATM we only have RootLayer.
			// this messages should be handled somewhere into a RootLayer
			// specific area - this set is intended for a RootLayer.
			gDesktop->ActiveRootLayer()->GetCursorManager().SetDefaults();
			break;
		}
		default:
			// we should never get here.
			break;
	}
}

/*!
	\brief Finds the application with the given signature
	\param sig MIME signature of the application to find
	\return the corresponding ServerApp or NULL if not found
	
	This call should be made only when necessary because it locks the app list 
	while it does its searching.
*/
ServerApp *
AppServer::FindApp(const char *sig)
{
	if (!sig)
		return NULL;

	ServerApp *foundapp=NULL;

	acquire_sem(fAppListLock);

	for(int32 i=0; i<fAppList->CountItems();i++)
	{
		foundapp=(ServerApp*)fAppList->ItemAt(i);
		if(foundapp && foundapp->Title() == sig)
		{
			release_sem(fAppListLock);
			return foundapp;
		}
	}

	release_sem(fAppListLock);
	
	// couldn't find a match
	return NULL;
}

/*!
	\brief Send a quick (no attachments) message to all applications
	
	Quite useful for notification for things like server shutdown, system 
	color changes, etc.
*/
void
BroadcastToAllApps(const int32 &code)
{
	acquire_sem(sAppServer->fAppListLock);

	for (int32 i = 0; i < sAppServer->fAppList->CountItems(); i++) {
		ServerApp *app = (ServerApp *)sAppServer->fAppList->ItemAt(i);

		if (!app)
			{ printf("PANIC in Broadcast()\n"); continue; }
		app->PostMessage(code);
	}

	release_sem(sAppServer->fAppListLock);
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
	AppServer app_server;
	app_server.Run();
	return 0;
}
