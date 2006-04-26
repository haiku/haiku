//------------------------------------------------------------------------------
//	Copyright (c) 2001-2005, Haiku, Inc.
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		AppServer.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	main manager object for the app_server
//  
//------------------------------------------------------------------------------
#include "AppServer.h"
#include "ServerApp.h"
#include "ServerProtocol.h"

#include <Accelerant.h>
#include <AppDefs.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <Message.h>
#include <Path.h>
#include <PortLink.h>
#include <StopWatch.h>

#include <stdio.h>
#include <stdlib.h>

//#define DEBUG_KEYHANDLING
//#define DEBUG_SERVER

#ifdef DEBUG_KEYHANDLING
#	define KBTRACE(x) printf x
#else
#	define KBTRACE(x) ;
#endif

#ifdef DEBUG_SERVER
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif

// Globals

port_id gAppServerPort;

//! Used to access the app_server from new_decorator
static AppServer *sAppServer = NULL;

//! Default background color for workspaces
//RGBColor workspace_default_color(51,102,160);


/*!
	\brief Constructor
	
	This loads the default fonts, allocates all the major global variables, spawns the main housekeeping
	threads, loads user preferences for the UI and decorator, and allocates various locks.
*/
AppServer::AppServer(void)
	:
	fCursorSem(-1),
	fCursorArea(-1)
{
	fMessagePort = create_port(200, SERVER_PORT_NAME);
	if (fMessagePort == B_NO_MORE_PORTS)
		debugger("app_server could not create message port");

	gAppServerPort = fMessagePort;

	fAppList = new BList();
	fQuittingServer = false;

	// We need this in order for new_decorator to be able to instantiate new decorators
	sAppServer = this;

	// This is necessary to mediate access between the Poller and app_server threads
	fActiveAppLock = create_sem(1,"app_server_active_sem");

	// This locker is for app_server and Picasso to vy for control of the ServerApp list
	fAppListLock = create_sem(1,"app_server_applist_sem");

	// Spawn our thread-monitoring thread
	fPicassoThreadID = spawn_thread(PicassoThread, "picasso", B_NORMAL_PRIORITY, this);
	if (fPicassoThreadID >= 0)
		resume_thread(fPicassoThreadID);
}

/*!
	\brief Destructor
	
	Reached only when the server is asked to shut down in Test mode. Kills all apps, shuts down the 
	desktop, kills the housekeeping threads, etc.
*/
AppServer::~AppServer()
{
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
}


/*!
	\brief Starts the Cursor Thread
*/
void
AppServer::LaunchCursorThread()
{
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
			case AS_GET_DESKTOP:
			case AS_CREATE_APP:
			case AS_DELETE_APP:
				DispatchMessage(code, pmsg);
				break;
			default:
			{
				STRACE(("Server::MainLoop received unexpected code %ld\n",
					code));
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
		case AS_GET_DESKTOP:
		{
			port_id replyPort;
			if (msg.Read<port_id>(&replyPort) < B_OK)
				break;

			int32 userID;
			msg.Read<int32>(&userID);

			BPrivate::LinkSender reply(replyPort);
			reply.StartMessage(B_OK);
			reply.Attach<port_id>(fMessagePort);
			reply.Flush();
			break;
		}

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
			int32 clientToken;
			char *appSignature = NULL;

			msg.Read<port_id>(&clientReplyPort);
			msg.Read<port_id>(&clientLooperPort);
			msg.Read<team_id>(&clientTeamID);
			msg.Read<int32>(&clientToken);
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
				clientTeamID, clientToken, appSignature);

			// add the new ServerApp to the known list of ServerApps
			fAppList->AddItem(app);

			release_sem(fAppListLock);

			BPrivate::PortLink replylink(clientReplyPort);
			replylink.StartMessage(B_OK);
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

		case B_QUIT_REQUESTED:
			break;

		default:
			// we should never get here.
			break;
	}
}

/*!
	\brief Send a quick (no attachments) message to all applications
	
	Quite useful for notification for things like server shutdown, system 
	color changes, etc.
*/
void
AppServer::Broadcast(int32 code)
{
	acquire_sem(fAppListLock);

	for (int32 i = 0; i < fAppList->CountItems(); i++) {
		ServerApp *app = (ServerApp *)fAppList->ItemAt(i);

		if (!app)
			{ printf("PANIC in AppServer::Broadcast()\n"); continue; }
		app->PostMessage(code);
	}

	release_sem(fAppListLock);
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

	AppServer app_server;
	app_server.Run();
	return 0;
}
