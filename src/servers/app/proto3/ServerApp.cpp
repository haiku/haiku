/*
	ServerApp.cpp
		Class which works with a BApplication. Handles all messages coming
		from and going to its application.
*/

#include <AppDefs.h>
#include "ServerApp.h"
#include "ServerProtocol.h"
#include "PortLink.h"
#include <stdio.h>
#include <string.h>

ServerApp::ServerApp(port_id msgport, char *signature)
{
	//printf("New ServerApp %s for app at port %ld\n",signature,msgport);

	// need to copy the signature because the message buffer
	// owns the copy which we are passed as a parameter.
	if(signature)
	{
		app_sig=new char[strlen(signature)];
		sprintf(app_sig,signature);
	}
	else
	{
		app_sig=new char[strlen("Application")];
		sprintf(app_sig,"Application");
	}

	// sender is the monitored app's event port
	sender=msgport;
	applink=new PortLink(sender);
	
	// receiver is the port to which the app sends messages for the server
	receiver=create_port(30,app_sig);
	//printf("ServerApp port for app %s is at %ld\n",app_sig,receiver);
	if(receiver==B_NO_MORE_PORTS)
	{
		// uh-oh. We have a serious problem. Tell the app to quit
		applink->SetOpCode(B_QUIT_REQUESTED);
		applink->Flush();
	}
	else
	{
		// Everything checks out, so tell the application
		// where to send future messages
		applink->SetOpCode(SET_SERVER_PORT);
		applink->Attach(&receiver,sizeof(port_id));
		applink->Flush();
	}
}

ServerApp::~ServerApp(void)
{
	//printf("%s::~ServerApp()\n",app_sig);
	delete app_sig;
	delete applink;
}

bool ServerApp::Run(void)
{
	// Unlike a BApplication, a ServerApp is *supposed* to return immediately
	// when its Run function is called.
	monitor_thread=spawn_thread(MonitorApp,app_sig,B_NORMAL_PRIORITY,this);
	if(monitor_thread==B_NO_MORE_THREADS || monitor_thread==B_NO_MEMORY)
		return false;
	resume_thread(monitor_thread);
	return true;
}

int32 ServerApp::MonitorApp(void *data)
{
	ServerApp *app=(ServerApp *)data;
	app->Loop();
	exit_thread(0);
	return 0;
}

void ServerApp::Loop(void)
{
	// Message-dispatching loop for the ServerApp

	//printf("%s::MainLoop()\n",app_sig);

	int32 msgcode;
	int8 *msgbuffer=NULL;
	ssize_t buffersize,bytesread;
	
	for(;;)
	{
		buffersize=port_buffer_size(receiver);
		
		if(buffersize>0)
		{
			// buffers are PortLink messages. Allocate necessary buffer and
			// we'll cast it as a BMessage.
			msgbuffer=new int8[buffersize];
			bytesread=read_port(receiver,&msgcode,msgbuffer,buffersize);
		}
		else
			bytesread=read_port(receiver,&msgcode,NULL,0);

		if (bytesread != B_BAD_PORT_ID && bytesread != B_TIMED_OUT && bytesread != B_WOULD_BLOCK)
		{
			switch(msgcode)
			{
				case B_QUIT_REQUESTED:
				{
					// Our BApplication sent us this message when it quit.
					// We need to ask the app_server to delete our monitor
					port_id serverport=find_port(SERVER_PORT_NAME);
					if(serverport==B_NAME_NOT_FOUND)
					{
						//printf("PANIC: ServerApp %s could not find the app_server port!\n",app_sig);
						break;
					}
					//printf("ServerApp %s quitting\n",app_sig);
					applink->SetPort(serverport);
					applink->SetOpCode(DELETE_APP);
					applink->Attach(&monitor_thread,sizeof(thread_id));
					applink->Flush();
					break;
				}
				case QUIT_APP:
				{
					// This message is received from the app_server thread
					// because the server was asked to quit. Thus, we
					// ask all apps to quit. This is NOT the same as system
					// shutdown
										
					//printf("ServerApp asking %s to quit\n",app_sig);
					applink->SetOpCode(B_QUIT_REQUESTED);
					applink->Flush();
					break;
				}
				default:
					//printf("Server received unexpected code %ld\n",msgcode);
					break;
			}

		}
	
		if(buffersize>0)
			delete msgbuffer;

		if(msgcode==B_QUIT_REQUESTED)
			break;
	}
}

void ServerApp::DispatchMessage(BMessage *msg)
{
}

