/*
	AppServer.cpp
		File for the main app_server thread. This particular thread monitors for
		application start and quit messages. It also starts the housekeeping threads
		and initializes most of the server's globals.
*/
#include <Message.h>
#include <AppDefs.h>
#include "scheduler.h"
#include <iostream.h>
#include <stdio.h>
#include <OS.h>
#include <Debug.h>
#include "AppServer.h"
#include "Layer.h"
#include "ServerApp.h"
#include "ServerWindow.h"
#include "Desktop.h"
#include "ServerProtocol.h"
#include "PortLink.h"
#include "DisplayDriver.h"
#include "DebugTools.h"
#include "BeDecorator.h"
#include "WinDecorator.h"
#include "YMakDecorator.h"

//#define DEBUG_APPSERVER_THREAD

// This bad boy holds only the top layer for each workspace
//BList *layerlist;

// Used for mediating access to the layer lists
//BLocker *layerlock;

// Used for access via instantiate_decorator
AppServer *app_server;

AppServer::AppServer(void) : BApplication("application/x-vnd.obe-OBAppServer")
{
#ifdef DEBUG_APPSERVER_THREAD
	printf("AppServer()\n");
#endif

	mouseport=create_port(30,SERVER_INPUT_PORT);
	messageport=create_port(20,SERVER_PORT_NAME);
#ifdef DEBUG_APPSERVER_THREAD
printf("Server message port: %ld\n",messageport);
printf("Server input port: %ld\n",mouseport);
#endif
	applist=new BList(0);
	quitting_server=false;
	exit_poller=false;

	// Set up the Desktop
	init_desktop(3);

	// This is necessary to mediate access between the Poller and app_server threads
	active_lock=new BLocker;

	// This locker is for app_server and Picasso to vy for control of the ServerApp list
	applist_lock=new BLocker;

	// This locker is to mediate access to the make_decorator pointer
	decor_lock=new BLocker;
	
	// Get the driver first - Poller thread utilizes the thing
	driver=get_gfxdriver();

	// Spawn our input-polling thread
	poller_id = spawn_thread(PollerThread, "Poller", B_NORMAL_PRIORITY, this);
	if (poller_id >= 0)
		resume_thread(poller_id);

	// Spawn our thread-monitoring thread
	picasso_id = spawn_thread(PicassoThread,"Picasso", B_NORMAL_PRIORITY, this);
	if (picasso_id >= 0)
		resume_thread(picasso_id);

	active_app=-1;
	p_active_app=NULL;

	make_decorator=NULL;	
//	layerlock=new BLocker;
//	layerlist=new BList(1);
}

AppServer::~AppServer(void)
{
	shutdown_desktop();

	ServerApp *tempapp;
//	Layer *templayer;
	int32 i;
	applist_lock->Lock();
	for(i=0;i<applist->CountItems();i++)
	{
		tempapp=(ServerApp *)applist->ItemAt(i);
		if(tempapp!=NULL)
			delete tempapp;
	}
	delete applist;
	applist_lock->Unlock();

/*	layerlock->Lock();	
	for(i=0;i<layerlist->CountItems();i++)
	{
		templayer=(Layer *)layerlist->ItemAt(i);
		
		if(templayer!=NULL)
		{
			templayer->PruneTree();
			delete templayer;
		}
	}
	delete layerlist;
	layerlock->Unlock();

	delete layerlock;
*/	delete active_lock;
	delete applist_lock;

	// If these threads are still running, kill them - after this, if exit_poller
	// is deleted, who knows what will happen... These things will just return an
	// error and fail if the threads have already exited.
	kill_thread(poller_id);
	kill_thread(picasso_id);

	delete decor_lock;
	
	make_decorator=NULL;
}

thread_id AppServer::Run(void)
{
#ifdef DEBUG_APPSERVER_THREAD
printf("AppServer::Run()\n");
#endif	

	MainLoop();
	return 0;
}

void AppServer::MainLoop(void)
{
	// Main loop (duh). Monitors the message queue and dispatches as appropriate.
	// Input messages are handled by the poller, however.

#ifdef DEBUG_APPSERVER_THREAD
printf("AppServer::MainLoop()\n");
#endif

	int32 msgcode;
	int8 *msgbuffer=NULL;
	ssize_t buffersize,bytesread;
	
	for(;;)
	{
		buffersize=port_buffer_size(messageport);
		if(buffersize>0)
			msgbuffer=new int8[buffersize];
		bytesread=read_port(messageport,&msgcode,msgbuffer,buffersize);

		if (bytesread != B_BAD_PORT_ID && bytesread != B_TIMED_OUT && bytesread != B_WOULD_BLOCK)
		{
			switch(msgcode)
			{
				case CREATE_APP:
				case DELETE_APP:
				case GET_SCREEN_MODE:
				case B_QUIT_REQUESTED:
					DispatchMessage(msgcode,msgbuffer);
					break;
				default:
				{
#ifdef DEBUG_APPSERVER_THREAD
printf("Server received unexpected code %ld\n",msgcode);
#endif
					break;
				}
			}

		}

		if(buffersize>0)
			delete msgbuffer;
		if(msgcode==DELETE_APP || msgcode==B_QUIT_REQUESTED)
		{
			if(quitting_server==true && applist->CountItems()==0)
				break;
		}
	}
	// Make sure our polling thread has exited
	if(find_thread("Poller")!=B_NAME_NOT_FOUND)
		kill_thread(poller_id);

}


void AppServer::DispatchMessage(int32 code, int8 *buffer)
{
	int8 *index=buffer;
	switch(code)
	{
		case CREATE_APP:
		{
			// Create the ServerApp to node monitor a new BApplication
			
#ifdef DEBUG_APPSERVER_THREAD
printf("AppServer: Create App\n");
#endif
			
			// Attached data:
			// 1) port_id - port to reply to
			// 2) port_id - receiver port of a regular app
			// 3) char * - signature of the regular app

			// Find the necessary data
			port_id reply_port=*((port_id*)index); index+=sizeof(port_id);
			port_id app_port=*((port_id*)index); index+=sizeof(port_id);

			char *app_signature=(char *)index;
			
			// Create the ServerApp subthread for this app
			applist_lock->Lock();
			ServerApp *newapp=new ServerApp(app_port,app_signature);
			applist->AddItem(newapp);
			applist_lock->Unlock();

			active_lock->Lock();
			p_active_app=newapp;
			active_app=applist->CountItems()-1;

			PortLink *replylink=new PortLink(reply_port);
			replylink->SetOpCode(SET_SERVER_PORT);
			replylink->Attach((int32)newapp->receiver);
			replylink->Flush();

			delete replylink;

			active_lock->Unlock();
			
			newapp->Run();

			break;
		}
		case DELETE_APP:
		{
			// Delete a ServerApp. Received only from the respective ServerApp when a
			// BApplication asks it to quit.
			
#ifdef DEBUG_APPSERVER_THREAD
printf("AppServer: Delete App\n");
#endif
			
			// Attached Data:
			// 1) thread_id - thread ID of the ServerApp to be deleted
			
			int32 i, appnum=applist->CountItems();
			ServerApp *srvapp;
			thread_id srvapp_id=*((thread_id*)buffer);
			
			// Run through the list of apps and nuke the proper one
			for(i=0;i<appnum;i++)
			{
				srvapp=(ServerApp *)applist->ItemAt(i);

				if(srvapp!=NULL && srvapp->monitor_thread==srvapp_id)
				{
					applist_lock->Lock();
					srvapp=(ServerApp *)applist->RemoveItem(i);
					if(srvapp)
					{
						delete srvapp;
						srvapp=NULL;
					}
					applist_lock->Unlock();
					active_lock->Lock();

					if(applist->CountItems()==0)
					{
						// active==-1 signifies that no other apps are running - NOT good
						active_app=-1;
					}
					else
					{
						// we actually still have apps running, so make a new one active
						if(active_app>0)
							active_app--;
						else
							active_app=0;
					}
					p_active_app=(active_app>-1)?(ServerApp*)applist->ItemAt(active_app):NULL;
					active_lock->Unlock();
					break;	// jump out of our for() loop
				}

			}
			break;
		}
		case GET_SCREEN_MODE:
		{
			// Synchronous message call to get the stats on the current screen mode
			// in the app_server. Simply a hack in place for the Input Server until
			// BScreens are done.
			
#ifdef DEBUG_APPSERVER_THREAD
printf("AppServer: Get Screen Mode\n");
#endif
			// Attached Data:
			// 1) port_id - port to reply to
			
			// Returned Data:
			// 1) int32 width
			// 2) int32 height
			// 3) int depth
			
			PortLink *replylink=new PortLink(*((port_id*)index));
			replylink->SetOpCode(GET_SCREEN_MODE);
			replylink->Attach(driver->GetWidth());
			replylink->Attach(driver->GetHeight());
			replylink->Attach((int16)driver->GetDepth());
			replylink->Flush();
			delete replylink;
			break;
		}
		case B_QUIT_REQUESTED:
		{
#ifdef DEBUG_APPSERVER_THREAD
printf("Shutdown sequence initiated.\n");
#endif

			// Attached Data:
			// none
			
			// We've been asked to quit, so (for now) broadcast to all
			// test apps to quit. This situation will occur only when the server
			// is compiled as a regular Be application.
			PortLink *applink=new PortLink(0);
			ServerApp *srvapp;

			applist_lock->Lock();
			int32 i,appnum=applist->CountItems();
			
			applink->SetOpCode(QUIT_APP);
			for(i=0;i<appnum;i++)
			{
				srvapp=(ServerApp *)applist->ItemAt(i);
				if(srvapp!=NULL)
				{
					applink->SetPort(srvapp->receiver);
					applink->Flush();
				}
			}
			applist_lock->Unlock();

			delete applink;
			// So when we delete the last ServerApp, we can exit the server
			quitting_server=true;
			exit_poller=true;
			break;
		}
		default:
			//printf("Unexpected message %ld (%lx) in AppServer::DispatchMessage()\n",code,code);
			break;
	}
}

bool AppServer::LoadDecorator(BString path)
{
	// Loads a window decorator based on the supplied path and forces a decorator update.
	// If it cannot load the specified decorator, it will retain the current one and
	// return false.
	
	create_decorator *pcreatefunc=NULL;
	get_decorator_version *pversionfunc=NULL;
	status_t stat;
	image_id addon;
	
	addon=load_add_on(path.String());

	stat=get_image_symbol(addon, "get_decorator_version", B_SYMBOL_TYPE_TEXT, (void**)&pversionfunc);
	if(stat!=B_OK)
	{
		printf("ERROR: Could not get version for Decorator %s\n", path.String());
		unload_add_on(addon);
		return false;
	}

	// As of now, we do nothing with decorator versions, but the possibility exists
	// that the API will change even though I cannot forsee any reason to do so. If
	// we *did* do anything with decorator versions, the assignment to a global would
	// go here.
		
	stat=get_image_symbol(addon, "get_decorator_version", B_SYMBOL_TYPE_TEXT, (void**)&pversionfunc);
	if(stat!=B_OK)
	{
		printf("ERROR: Could not get version for Decorator %s\n", path.String());
		unload_add_on(addon);
		return false;
	}
	decor_lock->Lock();
	make_decorator=pcreatefunc;
	decorator_id=addon;
	decor_lock->Unlock();
	return true;
}

void AppServer::LoadDefaultDecorator(void)
{
	// This function is called on server startup to provide a fallback decorator in
	// case the decorator specified in the config file cannot be loaded. It just
	// wouldn't do to have windows with no borders. ;) It also unloads any loaded
	// decorator addons. We can specify that we want the default. It is no less
	// functional - just internal.
	
}

Decorator *instantiate_decorator(Layer *lay, uint32 dflags, uint32 wlook)
{
	Decorator *decor=NULL;

	app_server->decor_lock->Lock();
	if(app_server->make_decorator!=NULL)
		decor=app_server->make_decorator(lay, dflags, wlook);
	else
	{
		decor=new BeDecorator(lay, dflags, wlook);
//		decor=new WinDecorator(lay, dflags, wlook);
//		decor=new YMakDecorator(lay, dflags, wlook);
	}

	app_server->decor_lock->Unlock();
	return decor;
}

void AppServer::Poller(void)
{
	// This thread handles nothing but input messages for mouse and keyboard
	int32 msgcode;
	int8 *msgbuffer=NULL,*index;
	ssize_t buffersize,bytesread;
	
	for(;;)
	{
		buffersize=port_buffer_size(mouseport);
		if(buffersize>0)
			msgbuffer=new int8[buffersize];
		bytesread=read_port(mouseport,&msgcode,msgbuffer,buffersize);

		if (bytesread != B_BAD_PORT_ID && bytesread != B_TIMED_OUT && bytesread != B_WOULD_BLOCK)
		{
			switch(msgcode)
			{
				// We don't need to do anything with these two, so just pass them
				// onto the active application. Eventually, we will pass them onto
				// the window which is currently under the cursor.
				case B_MOUSE_DOWN:
				case B_MOUSE_UP:
				{
/*
					active_lock->Lock();
					if(active_app>-1 && p_active_app!=NULL)
						write_port(p_active_app->receiver, msgcode,msgbuffer,buffersize);
					active_lock->Unlock();
*/
					ServerWindow::HandleMouseEvent(msgcode,msgbuffer);
					break;
				}
				
				// Process the cursor and then pass it on
				case B_MOUSE_MOVED:
				{
					// Attached data:
					// 1) int64 - time of mouse click
					// 2) float - x coordinate of mouse click
					// 3) float - y coordinate of mouse click
					// 4) int32 - buttons down

					index=msgbuffer;

					// Time sent is not necessary for cursor processing.
					index += sizeof(int64);
					
					float tempx=0,tempy=0;
					tempx=*((float*)index);
					index+=sizeof(float);
					tempy=*((float*)index);
					index+=sizeof(float);

					driver->MoveCursorTo(tempx,tempy);

/*					active_lock->Lock();
					if(active_app>-1 && p_active_app!=NULL)
						write_port(p_active_app->receiver, msgcode,msgbuffer,buffersize);
					active_lock->Unlock();
*/
					ServerWindow::HandleMouseEvent(msgcode,msgbuffer);
					break;
				}
				default:
					//printf("Server::Poller received unexpected code %ld\n",msgcode);
					break;
			}

		}

		if(buffersize>0)
			delete msgbuffer;

		if(exit_poller)
			break;
	}
}

int32 AppServer::PollerThread(void *data)
{
#ifdef DEBUG_APPSERVER_THREAD
printf("Starting Poller thread...\n");
#endif
	AppServer *server=(AppServer *)data;
	
	// This won't return until it's told to
	server->Poller();
	
#ifdef DEBUG_APPSERVER_THREAD
printf("Exiting Poller thread...\n");
#endif
	exit_thread(B_OK);
	return B_OK;
}

void AppServer::Picasso(void)
{
	int32 i;
	ServerApp *app;
	for(;;)
	{
		applist_lock->Lock();
		for(i=0;i<applist->CountItems(); i++)
		{
			app=(ServerApp*)applist->ItemAt(i);
			ASSERT(app!=NULL);
			app->PingTarget();
		}
		applist_lock->Unlock();
		
		// if poller thread has to exit, so do we - I just was too lazy
		// to rename the variable name. ;)
		if(exit_poller)
			break;

		// we do this every other second so as not to suck *too* many CPU cycles
		snooze(2000000);
	}
}

int32 AppServer::PicassoThread(void *data)
{
#ifdef DEBUG_APPSERVER_THREAD
printf("Starting Picasso thread...\n");
#endif
	AppServer *server=(AppServer *)data;
	
	// This won't return until it's told to
	server->Picasso();
	
#ifdef DEBUG_APPSERVER_THREAD
printf("Exiting Picasso thread...\n");
#endif
	exit_thread(B_OK);
	return B_OK;
}

int main( int argc, char** argv )
{
	// There can be only one....
	if(find_port(SERVER_PORT_NAME)!=B_NAME_NOT_FOUND)
		return -1;

	app_server = new AppServer();
	app_server->Run();
	delete app_server;
	return 0;
}
