#include <AppDefs.h>
#include <stdio.h>
#include "ServerProtocol.h"

#include "AppServer.h"
#include "ServerApp.h"
#include "PortLink.h"

AppServer::AppServer(void) : BApplication("application/x-vnd.obe-OBAppServer")
{
	printf("AppServer()\n");

	mouseport=create_port(30,"OBpcreate");
//	messageport=create_port(20,"OBpcreate");
	messageport=create_port(20,SERVER_PORT_NAME);
	printf("Server message port: %ld\n",messageport);
	applist=new BList(0);
	quitting_server=false;
}

AppServer::~AppServer(void)
{
	printf("~AppServer()\n");

	ServerApp *temp;
	for(int32 i=0;i<applist->CountItems();i++)
	{
		temp=(ServerApp *)applist->ItemAt(i);
		if(temp!=NULL)
			delete temp;
	}
	delete applist;
}

thread_id AppServer::Run(void)
{
	printf("AppServer::Run()\n");
	
	MainLoop();
	return 0;
}

void AppServer::MainLoop(void)
{
	printf("AppServer::MainLoop()\n");

	// Main loop. Monitors the message queue and dispatches as appropriate
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
					printf("Server received unexpected code %ld\n",msgcode);
					break;
			}

		}

		if(buffersize>0)
			delete msgbuffer;
		if(msgcode==DELETE_APP)
		{
			if(quitting_server==true && applist->CountItems()==0)
				break;
		}
	}

}


void AppServer::DispatchMessage(int32 code, int8 *buffer)
{
	int8 *index=buffer;
	switch(code)
	{
		case CREATE_APP:
		{
			printf("AppServer: Create App\n");
			
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
			newapp->Run();
			break;
		}
		case DELETE_APP:
		{
			// Delete a ServerApp. Received only from the respective ServerApp
			printf("AppServer: Delete App\n");
			
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
				}
			}
			break;
		}
		case B_QUIT_REQUESTED:
		{
			printf("Shutdown sequence initiated.\n");
			// Attached Data:
			// none
			
			// We've been asked to quit, so (for now) broadcast to all
			// test apps to quit
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
			break;
		}
		default:
			printf("Unexpected message %ld (%lx) in AppServer::DispatchMessage()\n",code,code);
			break;
	}
}

int main( int argc, char** argv )
{
	AppServer *obas = new AppServer();
	obas->Run();
	delete obas;
	return( 0 );
}

