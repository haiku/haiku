/*
	OBApplication.cpp:
		Beginning framework for a *real* BApplication replacement for prototype #5

*/

#include <OS.h>
#include <GraphicsDefs.h>	// for the pattern struct and screen mode defs
#include <Rect.h>
#include <Region.h>
#include <Cursor.h>
#include <Font.h>
#include <Application.h>
#include <Handler.h>
#include <Message.h>
#include <stdio.h>
#include <String.h>
#include <List.h>
#include "ServerProtocol.h"
#include "PortLink.h"
#include "OBApplication.h"
#include "OBWindow.h"
#include "DebugTools.h"

OBApplication *obe_app;

#define DEBUG_OBAPP

OBApplication::OBApplication(const char *sig) : BApplication(sig)
{
	if(sig)
		signature=new BString(sig);
	else
		signature=new BString("application/x-vnd.obos-OBApplication");

#ifdef DEBUG_OBAPP
printf("OBApplication(%s)\n",signature->String());
#endif

	// A BApplication is locked on start, but because this is not a "real"
	// one, we do not actually lock it.

	initcheckval=B_OK;

	// Receives messages from server and others
	messageport=create_port(50,"msgport");
	if(messageport==B_BAD_VALUE || messageport==B_NO_MORE_PORTS)
	{
		printf("OBApplication: Couldn't create message port\n");
		initcheckval=B_ERROR;
	}
	
	// Find the port for the app_server
	serverport=find_port("OBappserver");
	if(serverport==B_NAME_NOT_FOUND)
	{
		printf("OBApplication: Couldn't find server port\n");
		serverlink=NULL;
		write_port(messageport,B_QUIT_REQUESTED,NULL,0);
		initcheckval=B_ERROR;
	}
	else
	{
		serverlink=new PortLink(serverport);
#ifdef DEBUG_OBAPP
printf("OBApplication(%s): App Server Port %ld\n",signature->String(),serverport);
#endif

		// Notify server of app's existence
		int32 replycode;
		status_t replystat;
		ssize_t replysize;
		int8 *replybuffer;
		
		serverlink->SetOpCode(CREATE_APP);
		serverlink->Attach(&messageport,sizeof(port_id));
//		serverlink.Attach(PID);
		serverlink->Attach((char*)signature->String(),strlen(signature->String()));

		// Send and wait for ServerApp port. Necessary here (as opposed to start
		// of message loop) in case any windows are created in application constructor
		replybuffer=serverlink->FlushWithReply(&replycode,&replystat,&replysize);
		serverport=*((port_id*)replybuffer);
		serverlink->SetPort(serverport);
		delete replybuffer;
#ifdef DEBUG_OBAPP
printf("OBApplication(%s): ServerApp Port %ld\n",signature->String(),serverport);
#endif
		ReadyToRun();
	}

	obe_app=this;
	windowlist=new BList(0);
}

OBApplication::~OBApplication(void)
{
#ifdef DEBUG_OBAPP
printf("%s::~OBApplication()\n",signature->String());
#endif
	delete signature;

	if(serverlink)
		delete serverlink;

//	OBWindow *win;
//	for(int32 i=0; i<windowlist->CountItems(); i++)
//	{
//		win=(OBWindow*)windowlist->RemoveItem(i);
//		if(win)
//			win->Quit();
//	}

	delete windowlist;
}

status_t OBApplication::InitCheck(void) const
{
	// Called to make sure everything in the constructor turned out ok. If it didn't,
	// the application falls through and the process ends.
	return initcheckval;
}

void OBApplication::MainLoop(void)
{
#ifdef DEBUG_OBAPP
printf("%s::MainLoop()\n",signature->String());
#endif
	
	int32 msgcode;
	uint8 *msgbuffer=NULL;
	ssize_t buffersize,bytesread;
	
	for(;;)
	{
		// Modified this slightly so that OBApplications will eventually time out
		// if the server crashes. Saves on a lot of Ctrl-Alt-Del's. :) CPU time is
		// saved by setting the timeout to 3 seconds - more than enough time.
		if(find_thread("OBAppServer")==B_NAME_NOT_FOUND)
		{
			printf("%s lost connection with server. Quitting.\n",signature->String());
			break;
		}

		buffersize=port_buffer_size_etc(messageport,B_TIMEOUT,300000);
		if(buffersize==B_TIMED_OUT)
			continue;
		
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
//					printf("%s: Quit requested\n",signature);
					serverlink->SetOpCode(msgcode);
					serverlink->Flush();
					break;
				}
/*				case SET_SERVER_PORT:
				{
					// This message is received when the server has created
					// the ServerApp object to monitor our application. It is
					// sending us the port to which we send future messages

					// Attached data:
					// port_id - id of the port of the corresponding ServerApp
					if(buffersize<1)
					{
						printf("%s: Set Server Port has no attached data\n",signature->String());
						break;
					}
//					printf("%s: Set Server Port to %ld\n",signature,*((port_id *)msgbuffer));
					serverport=*((port_id *)msgbuffer);
					serverlink->SetPort(serverport);
					ReadyToRun();
					break;
				}
*/
				case ADDWINDOW:
				{
					// This message is received by a window's constructor so that it
					// can be added to the window list
					
					// Attached Data:
					// 1) OBWindow *window
					
					// I hope this is the right syntax...
					windowlist->AddItem(msgbuffer);
#ifdef DEBUG_OBAPP
printf("%s: Add Window @ %p\n",signature->String(),msgbuffer);
#endif
					break;
				}
				case REMOVEWINDOW:
				{
					// This message is received by a window's constructor so that it
					// can be added to the window list
					
					// Attached Data:
					// 1) OBWindow *window
					
					// I hope this is the right syntax...
					windowlist->RemoveItem(msgbuffer);
#ifdef DEBUG_OBAPP
printf("%s: Remove Window @ %p\n",signature->String(),msgbuffer);
#endif
					break;
				}
				case B_MOUSE_MOVED:
				case B_MOUSE_DOWN:
				case B_MOUSE_UP:
					DispatchMessage(msgcode, (int8*)msgbuffer);
					break;

				// Later on, this will default to passing the message to 
				// DispatchMessage(BMessage*, BHandler*)
				default:
					printf("OBApplication received unexpected code %ld - ",msgcode);
					PrintMessageCode(msgcode);
					break;
			}

			if(msgcode==B_QUIT_REQUESTED)
			{
				printf("Quitting %s\n",signature->String());
				break;
			}
		}

		if(buffersize>0)
			delete msgbuffer;
	}
}

void OBApplication::DispatchMessage(BMessage *msg, BHandler *handler)
{
#ifdef DEBUG_OBAPP
printf("%s::DispatchMessage(BMessage*,BHandler*)\n",signature->String());
#endif
	BApplication::DispatchMessage(msg,handler);
}

void OBApplication::DispatchMessage(int32 code, int8 *buffer)
{
	// This function will be necessary to handle server messages. 
	switch(code)
	{
		case B_MOUSE_DOWN:
		case B_MOUSE_UP:
		case B_MOUSE_MOVED:
			break;
		default:
			printf("OBApplication %s received unknown code %ld\n",signature->String(),code);
			break;
	}
}

void OBApplication::MessageReceived(BMessage *msg)
{
#ifdef DEBUG_OBAPP
printf("%s::MessageReceived()\n",signature->String());
#endif
}

void OBApplication::ShowCursor(void)
{
#ifdef DEBUG_OBAPP
printf("%s::ShowCursor()\n",signature->String());
#endif
	serverlink->SetOpCode(SHOW_CURSOR);
	serverlink->Flush();
}

void OBApplication::HideCursor(void)
{
#ifdef DEBUG_OBAPP
printf("%s::HideCursor()\n",signature->String());
#endif
	serverlink->SetOpCode(HIDE_CURSOR);
	serverlink->Flush();
}

void OBApplication::ObscureCursor(void)
{
#ifdef DEBUG_OBAPP
printf("%s::ObscureCursor()\n",signature->String());
#endif
	serverlink->SetOpCode(OBSCURE_CURSOR);
	serverlink->Flush();
}

bool OBApplication::IsCursorHidden(void) const
{
	return false;
}

void OBApplication::SetCursor(const void *cursor)
{
#ifdef DEBUG_OBAPP
printf("%s::SetCursor()\n",signature->String());
#endif
	// attach & send the 68-byte chunk
	serverlink->SetOpCode(SET_CURSOR_DATA);
	serverlink->Attach((void*)cursor, 68);
	serverlink->Flush();
}

thread_id OBApplication::Run(void)
{
#ifdef DEBUG_OBAPP
printf("%s::Run()\n",signature->String());
#endif
	if(initcheckval==B_OK)
		MainLoop();

	return 0;
}

status_t OBApplication::Archive(BMessage *data, bool deep = true) const
{
	return B_ERROR;
}

void OBApplication::Quit(void)
{
#ifdef DEBUG_OBAPP
printf("%s::Quit()\n",signature->String());
#endif
	OBWindow *win;
	while(windowlist->CountItems()>0)
	{
		win=(OBWindow*)windowlist->RemoveItem(0L);
		if(win)
			win->Quit();
	}

	BApplication::Quit();
}

bool OBApplication::QuitRequested(void)
{
#ifdef DEBUG_OBAPP
printf("%s::QuitRequested()\n",signature->String());
#endif
	return true;
}

void OBApplication::Pulse(void)
{
}

void OBApplication::ArgvReceived(int32 argc, char **argv)
{
}

void OBApplication::AppActivated(bool active)
{
}

void OBApplication::RefsReceived(BMessage *a_message)
{
}

void OBApplication::AboutRequested(void)
{
}

BHandler *OBApplication::ResolveSpecifier(BMessage *msg,int32 index,BMessage *specifier,
		int32 form,const char *property)
{
	return NULL;
}

status_t OBApplication::GetSupportedSuites(BMessage *data)
{
	return B_ERROR;
}

status_t OBApplication::Perform(perform_code d, void *arg)
{
	return B_ERROR;
}

void OBApplication::ReadyToRun(void)
{
#ifdef DEBUG_OBAPP
printf("%s::ReadyToRun()\n",signature->String());
#endif
}
