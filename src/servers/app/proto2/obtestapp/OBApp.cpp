/*
	OBApp.cpp:	App faker for OpenBeOS app_server prototype #2
*/

#include <OS.h>
#include <SupportDefs.h>
#include <Application.h>
#include <Handler.h>
#include <Message.h>
#include <stdio.h>
#include "ServerProtocol.h"
#include "PortLink.h"

class TestApp
{
public:
	TestApp(void);
	virtual ~TestApp(void);

	void MainLoop(void);	// Replaces BLooper message looping
	
	// Hmm... What's this do? ;^)
	virtual void DispatchMessage(BMessage *msg, BHandler *handler);
	virtual void MessageReceived(BMessage *msg);
	bool Run(void);

	port_id messageport, serverport;
	char *signature;
	bool initialized;
	PortLink *serverlink;
};


TestApp::TestApp(void)
{
	printf("TestApp::TestApp()\n");
	initialized=true;

	// Receives messages from server and others
	messageport=create_port(50,"msgport");
	if(messageport==B_BAD_VALUE || messageport==B_NO_MORE_PORTS)
	{	printf("TestApp: Couldn't create message port\n");
		initialized=false;
	}
	
	// Find the port for the app_server
	serverport=find_port("OBappserver");
	if(serverport==B_NAME_NOT_FOUND)
	{	printf("TestApp: Couldn't find server port\n");
		initialized=false;

		serverlink=new PortLink(create_port(50,"OBAppServer"));
	}
	else
	{
		serverlink=new PortLink(serverport);
	}

//	signature=new char[255];
	signature=new char[strlen("application/x-vnd.wgp-OBTestApp")];
	sprintf(signature,"application/x-vnd.wgp-OBTestApp");
}

TestApp::~TestApp(void)
{
	printf("%s::~TestApp()\n",signature);
	delete signature;

	if(initialized==true)
		delete serverlink;
}

void TestApp::MainLoop(void)
{
	printf("TestApp::MainLoop()\n");

	// Notify server of app's existence
/*	BMessage *createmsg=new BMessage(CREATE_APP);
	createmsg->AddString("signature",signature);
//	createmsg->AddInt32("PID",PID);
	createmsg->AddInt32("messageport",messageport);

	printf("port id: %ld\n",messageport);
	printf("signature: %s\n",signature);

	write_port(serverport,CREATE_APP,createmsg,sizeof(*createmsg));
*/
	serverlink->SetOpCode(CREATE_APP);
	serverlink->Attach(&messageport,sizeof(port_id));
//	serverlink.Attach(PID);
	serverlink->Attach(signature,strlen(signature));
	serverlink->Flush();
	
	int32 msgcode;
	uint8 *msgbuffer=NULL;
	ssize_t buffersize,bytesread;
	
	for(;;)
	{
		buffersize=port_buffer_size(messageport);
		
		if(buffersize>0)
		{
			// buffers are flattened messages. Allocate necessary buffer and
			// we'll cast it as a BMessage.
			msgbuffer=new uint8[buffersize];
			bytesread=read_port(messageport,&msgcode,msgbuffer,buffersize);
		}
		else
			bytesread=read_port(messageport,&msgcode,NULL,0);
			
		if (bytesread != B_BAD_PORT_ID && bytesread != B_TIMED_OUT && bytesread != B_WOULD_BLOCK)
		{
			switch(msgcode)
			{
				case B_QUIT_REQUESTED:
				{
					// Attached data:
					// None
					printf("%s: Quit requested\n",signature);
					serverlink->SetOpCode(msgcode);
					serverlink->Flush();
					break;
				}
				case SET_SERVER_PORT:
				{
					// Attached data:
					// port_id - id of the port of the corresponding ServerApp
					if(buffersize<1)
					{
						printf("%s: Set Server Port has no attached data\n",signature);
						break;
					}
					printf("%s: Set Server Port to %ld\n",signature,*((port_id *)msgbuffer));
					serverport=*((port_id *)msgbuffer);
					serverlink->SetPort(serverport);
					break;
				}
				default:
					printf("OBTestApp received unexpected code %ld\n",msgcode);
					break;
			}

			if(msgcode==B_QUIT_REQUESTED)
			{
				printf("Quitting %s\n",signature);
				break;
			}
		}

		if(buffersize>0)
			delete msgbuffer;
	}
}

void TestApp::DispatchMessage(BMessage *msg, BHandler *handler)
{
	printf("TestApp::DispatchMessage()\n");
}

void TestApp::MessageReceived(BMessage *msg)
{
	printf("TestApp::MessageReceived()\n");
}

bool TestApp::Run(void)
{
	printf("TestApp::Run()\n");
//	if(!initialized)
//		return false;

	MainLoop();
	return true;
}

int main(void)
{
	TestApp app;
	app.Run();
}