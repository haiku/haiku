#include "scheduler.h"
#include <stdio.h>
#include <AppDefs.h>
#include "AppServer.h"
#include "Desktop.h"

AppServer::AppServer(void) : BApplication("application/x-vnd.obe-OBAppServer")
{
	printf("AppServer()\n");
}

AppServer::~AppServer(void)
{
	printf("~AppServer()\n");

//	kill_thread(PicassoID);
//	kill_thread(InputID);
	shutdown_desktop();
}

void AppServer::Initialize(void)
{
	printf("AppServer::Initialize()\n");
	mouseport=create_port(30,"OBpcreate");
	messageport=create_port(20,"OBpcreate");

	// Set up the Desktop
	init_desktop(3);
/*
	// Start up the housekeeping threads
	PicassoID = spawn_thread(Draw, "picasso", B_DISPLAY_PRIORITY, this);
	if (PicassoID >= 0)
		resume_thread(PicassoID);

	InputID = spawn_thread(Draw, "poller", B_DISPLAY_PRIORITY, this);
	if (InputID >= 0)
		resume_thread(InputID);
	
	MainLoop();
*/
}

void AppServer::MainLoop(void)
{
	printf("AppServer::MainLoop()\n");
	// Main loop. Monitors the message queue and dispatches as appropriate
	int32 msgcode;
	uint8 *msgbuffer=new uint8[65536];
	
	for(;;)
	{
		if (read_port(messageport,&msgcode,msgbuffer,65536)!=0)
		{
			printf("Message sent to OB App server: %ld\n",msgcode);
			if(msgcode==B_QUIT_REQUESTED)
				break;
		}
	}
	delete msgbuffer;
}

int32 AppServer::Picasso(void *data)
{
	return 1;
}

int32 AppServer::Poller(void *data)
{
	return 1;
}

void AppServer::DispatchMessage(BMessage *msg)
{
	printf("AppServer::DispatchMessage():"); msg->PrintToStream();
}

int main( int argc, char** argv )
{
	AppServer *obas = new AppServer();
	obas->Initialize();
	obas->Run();
	return( 0 );
}

