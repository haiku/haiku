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
#include <AppDefs.h>
#include <Accelerant.h>
#include <Entry.h>
#include <Path.h>
#include <Directory.h>
#include <PortLink.h>

#include <File.h>
#include <Message.h>
#include "AppServer.h"
#include "ServerProtocol.h"
#include "ServerApp.h"
#include <TokenSpace.h>
#if 0
#include "ColorSet.h"
#include "DisplayDriver.h"
#include "ServerCursor.h"
#include "ServerWindow.h"
#include "DefaultDecorator.h"
#include "RGBColor.h"
#include "BitmapManager.h"
#include "CursorManager.h"
#include "Utils.h"
#include "FontServer.h"
#include "Desktop.h"
#include "RootLayer.h"
#endif
#include <StopWatch.h>

#ifdef DEBUG_KEYHANDLING
#	include <stdio.h>
#	define KBTRACE(x) printf x
#else
#	define KBTRACE(x) ;
#endif

//#define DEBUG_SERVER
#ifdef DEBUG_SERVER
#	define DEBUG 1
#	include <Debug.h>
#	define STRACE(x) _sPrintf x
#else
#	define STRACE(x) ;
#endif

// Globals

AppServer::AppServer(void)
{
	fMessagePort = create_port(200, SERVER_PORT_NAME);

	fAppList = new BList(0);
	fQuittingServer= false;

	// This is necessary to mediate access between the Poller and app_server threads
	fActiveAppLock= create_sem(1,"app_server_active_sem");

	// This locker is for app_server and Picasso to vy for control of the ServerApp list
	fAppListLock= create_sem(1,"app_server_applist_sem");

	// Spawn our thread-monitoring thread
	fPicassoThreadID= spawn_thread(PicassoThread,"picasso", B_NORMAL_PRIORITY, this);
	if (fPicassoThreadID >= 0)
		resume_thread(fPicassoThreadID);
}


AppServer::~AppServer(void)
{
}


int32
AppServer::PicassoThread(void *data)
{
	int32		i;
	AppServer	*appserver=(AppServer*)data;
	ServerApp	*app;
	for(;;)
	{
		i = 0;
#if 1
		acquire_sem(appserver->fAppListLock);
		for(;;)
		{
			app=(ServerApp*)appserver->fAppList->ItemAt(i++);
			if(!app)
				break;

			app->PingTarget();
		}
		release_sem(appserver->fAppListLock);
#endif
		// we do this every other second so as not to suck *too* many CPU cycles
		snooze(1000000);
	}
	return 0;
}


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
	BPrivate::PortLink pmsg(-1,fMessagePort);
	int32 code=0;
	status_t err=B_OK;
	
	while(1)
	{
		STRACE(("info: AppServer::MainLoop listening on port %ld.\n", fMessagePort));
		err=pmsg.GetNextMessage(code);

		if(err<B_OK)
		{
			STRACE(("MainLoop:pmsg.GetNextMessage failed\n"));
			continue;
		}
		
		switch(code)
		{
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
				DispatchMessage(code,pmsg);
				break;
			default:
			{
				STRACE(("Server::MainLoop received unexpected code %ld(offset %ld)\n",
						code,code-SERVER_TRUE));
				break;
			}
		}
	}
}


void
AppServer::DispatchMessage(int32 code, BPrivate::PortLink &msg)
{
	switch(code)
	{
#if 1
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
			team_id	clientTeamID=-1;
			port_id	clientLooperPort=-1;
			port_id app_port=-1;
			int32 htoken=B_NULL_TOKEN;
			char *app_signature=NULL;

			msg.Read<port_id>(&app_port);
			msg.Read<port_id>(&clientLooperPort);
			msg.Read<team_id>(&clientTeamID);
			msg.Read<int32>(&htoken);
			msg.ReadString(&app_signature);
			
			// Create the ServerApp subthread for this app
			acquire_sem(fAppListLock);
			
			port_id server_listen=create_port(DEFAULT_MONITOR_PORT_SIZE, app_signature);
			if(server_listen<B_OK)
			{
				release_sem(fAppListLock);
				printf("No more ports left. Time to crash. Have a nice day! :)\n");
				break;
			}
			ServerApp *newapp=NULL;
			newapp= new ServerApp(app_port,server_listen, clientLooperPort, clientTeamID, 
					htoken, app_signature);

			// add the new ServerApp to the known list of ServerApps
			fAppList->AddItem(newapp);
			
			release_sem(fAppListLock);

			BPrivate::PortLink replylink(app_port);
			replylink.StartMessage(SERVER_TRUE);
			replylink.Attach<int32>(newapp->fMessagePort);
			replylink.Flush();

			// This is necessary because BPrivate::PortLink::ReadString allocates memory
			if(app_signature)
				free(app_signature);

			break;
		}
		case AS_DELETE_APP:
		{
			// Delete a ServerApp. Received only from the respective ServerApp when a
			// BApplication asks it to quit.
			
			// Attached Data:
			// 1) thread_id - thread ID of the ServerApp to be deleted
			
			int32 	i=0,
					appnum=fAppList->CountItems();
			
			ServerApp *srvapp=NULL;
			thread_id srvapp_id=-1;

			if(msg.Read<thread_id>(&srvapp_id)<B_OK)
				break;

			acquire_sem(fAppListLock);

			// Run through the list of apps and nuke the proper one
			for(i= 0; i < appnum; i++)
			{
				srvapp=(ServerApp *)fAppList->ItemAt(i);

				if(srvapp != NULL && srvapp->fMonitorThreadID== srvapp_id)
				{
					srvapp=(ServerApp *)fAppList->RemoveItem(i);
					if(srvapp)
					{
						status_t		temp;
						wait_for_thread(srvapp_id, &temp);
						delete srvapp;
						srvapp= NULL;
					}
					break;	// jump out of our for() loop
				}
			}

			release_sem(fAppListLock);
			break;
		}
#endif
		case AS_QUERY_FONTS_CHANGED:
		{
			// Seeing how the client merely wants an answer, we'll skip the BPrivate::PortLink
			// and all its overhead and just write the code to port.
			port_id replyport;
			if (msg.Read<port_id>(&replyport) < B_OK)
				break;
			BPrivate::PortLink replylink(replyport);
			replylink.StartMessage(SERVER_FALSE);
			replylink.Flush();
			break;
		}
		case AS_GET_DECORATOR:
		{
			// Attached Data:
			// 1) port_id reply port

			port_id replyport=-1;
			if(msg.Read<port_id>(&replyport)<B_OK)
				return;

			BPrivate::PortLink replylink(replyport);
			replylink.StartMessage(AS_GET_DECORATOR);
			replylink.AttachString("none");
			replylink.Flush();
			break;
		}
		
		case AS_UPDATED_CLIENT_FONTLIST:
		case AS_SET_UI_COLORS:
		case AS_SET_DECORATOR:
		case AS_R5_SET_DECORATOR:
		case AS_SET_SYSCURSOR_DEFAULTS:
		default:
			break;
	}
}


int
main(int argc, char **argv)
{
	// There can be only one....
	if (find_port(SERVER_PORT_NAME) != B_NAME_NOT_FOUND)
		return -1;

	AppServer app_server;
	app_server.Run();
	return 0;
}
