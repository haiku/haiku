#include <Message.h>
#include <AppDefs.h>
#include "scheduler.h"
#include <iostream.h>
#include <stdio.h>
#include "AppServer.h"
#include "ServerApp.h"
#include "Desktop.h"
#include "ServerProtocol.h"
#include "PortLink.h"
#include "DisplayDriver.h"
#include "DebugTools.h"

AppServer::AppServer(void) : BApplication("application/x-vnd.obe-OBAppServer")
{
//	printf("AppServer()\n");

	mouseport=create_port(30,SERVER_INPUT_PORT);
	messageport=create_port(20,SERVER_PORT_NAME);
//	printf("Server message port: %ld\n",messageport);
//	printf("Server input port: %ld\n",mouseport);
	applist=new BList(0);
	quitting_server=false;
	exit_poller=false;

	// Set up the Desktop
	init_desktop(3);

	// Spawn our input-polling thread
	poller_id = spawn_thread(PollerThread, "Poller", B_NORMAL_PRIORITY, this);
	if (poller_id >= 0)
		resume_thread(poller_id);
	driver=get_gfxdriver();

	active_app=-1;
	p_active_app=NULL;
	
	// This is necessary to mediate access between the Poller and app_server threads
	active_lock=new BLocker;
}

AppServer::~AppServer(void)
{
	shutdown_desktop();

	ServerApp *temp;
	for(int32 i=0;i<applist->CountItems();i++)
	{
		temp=(ServerApp *)applist->ItemAt(i);
		if(temp!=NULL)
			delete temp;
	}
	delete applist;
	delete active_lock;
}

thread_id AppServer::Run(void)
{
	//printf("AppServer::Run()\n");
	
	MainLoop();
	return 0;
}

void AppServer::MainLoop(void)
{
	// Main loop (duh). Monitors the message queue and dispatches as appropriate.
	// Input messages are handled by the poller, however.

	//printf("AppServer::MainLoop()\n");

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
				case B_QUIT_REQUESTED:
					DispatchMessage(msgcode,msgbuffer);
					break;
				default:
					//printf("Server received unexpected code %ld\n",msgcode);
					break;
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
			
			//printf("AppServer: Create App\n");
			
			// Attached data:
			// 1) port_id - receiver port of a regular app
			// 2) char * - signature of the regular app

			// Find the necessary data
			port_id app_port=*((port_id*)index);
			index+=sizeof(port_id);

			char *app_signature=(char *)index;
			
			// Create the ServerApp subthread for this app
			ServerApp *newapp=new ServerApp(app_port,app_signature);
			applist->AddItem(newapp);

			active_lock->Lock();
			p_active_app=newapp;
			active_app=applist->CountItems()-1;
			active_lock->Unlock();
			
			newapp->Run();

			break;
		}
		case DELETE_APP:
		{
			// Delete a ServerApp. Received only from the respective ServerApp when a
			// BApplication asks it to quit.
			
			//printf("AppServer: Delete App\n");
			
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
					srvapp=(ServerApp *)applist->RemoveItem(i);
					delete srvapp;
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
					break;	// jump out of the loop
				}
			}
			break;
		}
		case B_QUIT_REQUESTED:
		{
			//printf("Shutdown sequence initiated.\n");

			// Attached Data:
			// none
			
			// We've been asked to quit, so (for now) broadcast to all
			// test apps to quit. This situation will occur only when the server
			// is compiled as a regular Be application.
			PortLink *applink=new PortLink(0);
			ServerApp *srvapp;
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
					active_lock->Lock();
					if(active_app>-1)
						write_port(p_active_app->receiver, msgcode,msgbuffer,buffersize);
					active_lock->Unlock();
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

					active_lock->Lock();
					if(active_app>-1)
						write_port(p_active_app->receiver, msgcode,msgbuffer,buffersize);
					active_lock->Unlock();
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
	//printf("Starting Poller thread...\n");
	AppServer *server=(AppServer *)data;
	
	server->Poller();	// This won't return until it's told to
	
	//printf("Exiting Poller thread...\n");
	exit_thread(B_OK);
	return B_OK;
}

int main( int argc, char** argv )
{
	AppServer *obas = new AppServer();
	obas->Run();
	delete obas;
	return 0;
}

 