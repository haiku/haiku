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
//	File Name:		ServerApp.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Server-side BApplication counterpart
//  
//------------------------------------------------------------------------------
#include <AppDefs.h>
#include <List.h>
#include <String.h>
#include <PortLink.h>
#include <RGBColor.h>
#include <stdio.h>
#include <string.h>
#include <ScrollBar.h>
#include <ServerProtocol.h>
#include <TokenSpace.h>

#include "AppServer.h"
#include "BGet++.h"
#if 0
#include "BitmapManager.h"
#include "CursorManager.h"

#include "Desktop.h"
#include "DisplayDriver.h"
#include "FontServer.h"
#include "RAMLinkMsgReader.h"
#include "RootLayer.h"
#endif
#include "ServerApp.h"
#include "ServerConfig.h"
#include "Utils.h"
#if 0
#include "ServerScreen.h"
#include "ServerWindow.h"
#include "ServerCursor.h"
#include "ServerBitmap.h"
#include "ServerPicture.h"
#include "WinBorder.h"
#include "LayerData.h"
#endif

//#define DEBUG_SERVERAPP
#ifdef DEBUG_SERVERAPP
#	define DEBUG 1
#	include <Debug.h>
#	define STRACE(x) _sPrintf x
#else
#	define STRACE(x) ;
#endif

//#define DEBUG_SERVERAPP_FONT

#ifdef DEBUG_SERVERAPP_FONT
#	include <stdio.h>
#	define FTRACE(x) printf x
#else
#	define FTRACE(x) ;
#endif

/*!
	\brief Constructor
	\param sendport port ID for the BApplication which will receive the ServerApp's messages
	\param rcvport port by which the ServerApp will receive messages from its BApplication.
	\param fSignature NULL-terminated string which contains the BApplication's
	MIME fSignature.
*/
ServerApp::ServerApp(port_id sendport, port_id rcvport, port_id clientLooperPort,
		team_id clientTeamID, int32 handlerID, char *signature)
{
	// it will be of *very* musch use in correct window order
	fClientTeamID		= clientTeamID;

	// what to send a message to the client? Write a BMessage to this port.
	fClientLooperPort	= clientLooperPort;

	// need to copy the fSignature because the message buffer
	// owns the copy which we are passed as a parameter.
	fSignature=(signature)?signature:"application/x-vnd.NULL-application-signature";
	
	// token ID of the BApplication's BHandler object. Used for BMessage target specification
	fHandlerToken=handlerID;

	// fClientAppPort is the our BApplication's event port
	fClientAppPort=sendport;

	// fMessagePort is the port we receive messages from our BApplication
	fMessagePort=rcvport;

	fMsgReader = new LinkMsgReader(fMessagePort);
	fMsgSender = new LinkMsgSender(fClientAppPort);

	fIsActive=false;
	
	fSharedMem=new AreaPool;
	fLockSem=create_sem(1,"ServerApp sem");

	Run();

	STRACE(("ServerApp %s:\n",fSignature.String()));
	STRACE(("\tBApp port: %ld\n",fClientAppPort));
	STRACE(("\tReceiver port: %ld\n",fMessagePort));
}

//! Does all necessary teardown for application
ServerApp::~ServerApp(void)
{
	STRACE(("*ServerApp %s:~ServerApp()\n",fSignature.String()));

	delete fMsgReader;
	delete fMsgSender;

	delete_sem(fLockSem);
	
	STRACE(("#ServerApp %s:~ServerApp()\n",fSignature.String()));

	// Kill the monitor thread if it exists
	thread_info info;
	if(get_thread_info(fMonitorThreadID,&info)==B_OK)
		kill_thread(fMonitorThreadID);
	
	delete fSharedMem;
}

/*!
	\brief Starts the ServerApp monitoring for messages
	\return false if the application couldn't start, true if everything went OK.
*/
bool ServerApp::Run(void)
{
	// Unlike a BApplication, a ServerApp is *supposed* to return immediately
	// when its Run() function is called.
	fMonitorThreadID=spawn_thread(MonitorApp,fSignature.String(),B_NORMAL_PRIORITY,this);
	if(fMonitorThreadID==B_NO_MORE_THREADS || fMonitorThreadID==B_NO_MEMORY)
		return false;

	resume_thread(fMonitorThreadID);
	return true;
}


bool
ServerApp::PingTarget(void)
{
	team_info tinfo;
	if(get_team_info(fClientTeamID,&tinfo)==B_BAD_TEAM_ID)
	{
		port_id serverport=find_port(SERVER_PORT_NAME);
		if(serverport==B_NAME_NOT_FOUND)
		{
			printf("PANIC: ServerApp %s could not find the app_server port in PingTarget()!\n",fSignature.String());
			return false;
		}
		fMsgSender->SetPort(serverport);
		fMsgSender->StartMessage(AS_DELETE_APP);
		fMsgSender->Attach(&fMonitorThreadID,sizeof(thread_id));
		fMsgSender->Flush();
		return false;
	}
	return true;
}

/*!
	\brief Send a message to the ServerApp with no attachments
	\param code ID code of the message to post
*/
void ServerApp::PostMessage(int32 code)
{
	BPortLink link(fMessagePort);
	link.StartMessage(code);
	link.Flush();
}

/*!
	\brief Send a message to the ServerApp's BApplication
	\param msg The message to send
*/
void ServerApp::SendMessageToClient(const BMessage *msg) const
{
	ssize_t size;
	char *buffer;
	
	size=msg->FlattenedSize();
	buffer=new char[size];
	
	if (msg->Flatten(buffer, size) == B_OK)
		write_port(fClientLooperPort, msg->what, buffer, size);
	else
		printf("PANIC: ServerApp: '%s': can't flatten message in 'SendMessageToClient()'\n", fSignature.String());

	delete [] buffer;
}

/*!
	\brief Sets the ServerApp's active status
	\param value The new status of the ServerApp.
	
	This changes an internal flag and also sets the current cursor to the one specified by
	the application
*/
void ServerApp::Activate(bool value)
{
	fIsActive=value;
	SetAppCursor();
}
 
//! Sets the cursor to the application cursor, if any.
void ServerApp::SetAppCursor(void)
{
}

/*!
	\brief The thread function ServerApps use to monitor messages
	\param data Pointer to the thread's ServerApp object
	\return Throwaway value - always 0
*/
int32 ServerApp::MonitorApp(void *data)
{
	// Message-dispatching loop for the ServerApp
	
	ServerApp *app = (ServerApp *)data;
	LinkMsgReader msgqueue(app->fMessagePort);
	
	bool quitting = false;
	int32 code;
	status_t err = B_OK;
	
	while(!quitting)
	{
		STRACE(("info: ServerApp::MonitorApp listening on port %ld.\n", app->fMessagePort));
//		err = msgqueue.GetNextReply(&code);
		err = msgqueue.GetNextMessage(&code);
		if (err < B_OK)
			break;

		switch(code)
		{
			case AS_CREATE_WINDOW:
			{
				BRect frame;
				uint32 look, feel, flags, wkspaces;
				int32 token = B_NULL_TOKEN;
				port_id	sendPort = -1;
				port_id looperPort = -1;
				char *title = NULL;

				msgqueue.Read<BRect>(&frame);
				msgqueue.Read<uint32>(&look);
				msgqueue.Read<uint32>(&feel);
				msgqueue.Read<uint32>(&flags);
				msgqueue.Read<uint32>(&wkspaces);
				msgqueue.Read<int32>(&token);
				msgqueue.Read<port_id>(&sendPort);
				msgqueue.Read<port_id>(&looperPort);
				msgqueue.ReadString(&title);

				STRACE(("ServerApp %s: Got 'New Window' message, trying to do smething...\n",app->fSignature.String()));

				if (title)
					free(title);

				break;
			}
			case AS_QUIT_APP:
			{
				// This message is received only when the app_server is asked to shut down in
				// test/debug mode. Of course, if we are testing while using AccelerantDriver, we do
				// NOT want to shut down client applications. The server can be quit o in this fashion
				// through the driver's interface, such as closing the ViewDriver's window.
				
				STRACE(("ServerApp %s:Server shutdown notification received\n",app->fSignature.String()));

				// If we are using the real, accelerated version of the
				// DisplayDriver, we do NOT want the user to be able shut down
				// the server. The results would NOT be pretty
				break;
			}
			
			case B_QUIT_REQUESTED:
			{
				STRACE(("ServerApp %s: B_QUIT_REQUESTED\n",app->fSignature.String()));
				// Our BApplication sent us this message when it quit.
				// We need to ask the app_server to delete our monitor
				// ADI: No! This is a bad solution. A thead should continue its
				// execution until its exit point, and this can *very* easily be done
				quitting=true;
				// see... no need to ask the main thread to kill us.
				// still... it will delete this ServerApp object.
				port_id		serverport = find_port(SERVER_PORT_NAME);
				if(serverport == B_NAME_NOT_FOUND){
					printf("PANIC: ServerApp %s could not find the app_server port!\n",app->fSignature.String());
					break;
				}
				app->fMsgSender->SetPort(serverport);
				app->fMsgSender->StartMessage(AS_DELETE_APP);
				app->fMsgSender->Attach(&app->fMonitorThreadID, sizeof(thread_id));
				app->fMsgSender->Flush();
				break;
			}
			default:
			{
				STRACE(("ServerApp %s: Got a Message to dispatch\n",app->fSignature.String()));
				app->DispatchMessage(code, msgqueue);
				break;
			}
		}

	} // end for 

	// clean exit.
	return 0;
}

/*!
	\brief Handler function for BApplication API messages
	\param code Identifier code for the message. Equivalent to BMessage::what
	\param buffer Any attachments
	
	Note that the buffer's exact format is determined by the particular message. 
	All attachments are placed in the buffer via a PortLink, so it will be a 
	matter of casting and incrementing an index variable to access them.
*/
void ServerApp::DispatchMessage(int32 code, LinkMsgReader &msg)
{
	BPortLink replylink;
	
	switch(code)
	{
//		case AS_UPDATE_COLORS:
//		case AS_UPDATE_FONTS:
//			break;
		case AS_ACQUIRE_SERVERMEM:
		{
			// This particular call is more than a bit of a pain in the neck. We are given a
			// size of a chunk of memory needed. We need to (1) allocate it, (2) get the area for
			// this particular chunk, (3) find the offset in the area for this chunk, and (4)
			// tell the client about it. Good thing this particular call isn't used much
			
			// Received from a ServerMemIO object requesting operating memory
			// Attached Data:
			// 1) size_t requested size
			// 2) port_id reply_port
			
			size_t memsize;
			port_id replyport;
			
			msg.Read<size_t>(&memsize);
			msg.Read<port_id>(&replyport);
			
			// TODO: I wonder if ACQUIRE_SERVERMEM should have a minimum size requirement?

			void *sharedmem=fSharedMem->GetBuffer(memsize);
			
			replylink.SetSendPort(replyport);
			if(memsize<1 || sharedmem==NULL)
			{
				replylink.StartMessage(SERVER_FALSE);
				replylink.Flush();
				break;
			}
			
			area_id owningArea=area_for(sharedmem);
			area_info ai;
			
			if(owningArea==B_ERROR || get_area_info(owningArea,&ai)!=B_OK)
			{
				replylink.StartMessage(SERVER_FALSE);
				replylink.Flush();
				break;
			}
			
			int32 areaoffset=((int32*)sharedmem)-((int32*)ai.address);
			STRACE(("Successfully allocated shared memory of size %ld\n",memsize));
			
			replylink.StartMessage(SERVER_TRUE);
			replylink.Attach<area_id>(owningArea);
			replylink.Attach<int32>(areaoffset);
			replylink.Flush();
			
			break;
		}
		case AS_RELEASE_SERVERMEM:
		{
			// Received when a ServerMemIO object on destruction
			// Attached Data:
			// 1) area_id owning area
			// 2) int32 area offset
			
			area_id owningArea;
			area_info ai;
			int32 areaoffset;
			void *sharedmem;
			
			msg.Read<area_id>(&owningArea);
			msg.Read<int32>(&areaoffset);
			
			if(owningArea<0 || get_area_info(owningArea,&ai)!=B_OK)
				break;
			
			STRACE(("Successfully freed shared memory\n"));
			sharedmem=((int32*)ai.address)+areaoffset;
			
			fSharedMem->ReleaseBuffer(sharedmem);
			
			break;
		}
		case AS_CREATE_BITMAP:
		{
			STRACE(("ServerApp %s: Received BBitmap creation request\n",fSignature.String()));
			// Allocate a bitmap for an application
			
			// Attached Data: 
			// 1) BRect bounds
			// 2) color_space space
			// 3) int32 bitmap_flags
			// 4) int32 bytes_per_row
			// 5) int32 screen_id::id
			// 6) port_id reply port
			
			// Reply Code: SERVER_TRUE
			// Reply Data:
			//	1) int32 server token
			//	2) area_id id of the area in which the bitmap data resides
			//	3) int32 area pointer offset used to calculate fBasePtr
			
			// First, let's attempt to allocate the bitmap
			port_id replyport = -1;
			BRect r;
			color_space cs;
			int32 f,bpr;
			screen_id s;

			msg.Read<BRect>(&r);
			msg.Read<color_space>(&cs);
			msg.Read<int32>(&f);
			msg.Read<int32>(&bpr);
			msg.Read<screen_id>(&s);
			msg.Read<int32>(&replyport);
			
			STRACE(("ServerApp %s: Create Bitmap (%.1f,%.1f,%.1f,%.1f)\n",
						fSignature.String(),r.left,r.top,r.right,r.bottom));

			replylink.SetSendPort(replyport);
/*			if(sbmp)
			{
				fBitmapList->AddItem(sbmp);
				replylink.StartMessage(SERVER_TRUE);
				replylink.Attach<int32>(sbmp->Token());
				replylink.Attach<int32>(sbmp->Area());
				replylink.Attach<int32>(sbmp->AreaOffset());
			}
			else*/
			{
				// alternatively, if something went wrong, we reply with SERVER_FALSE
				replylink.StartMessage(SERVER_FALSE);
			}
			replylink.Flush();
			
			break;
		}
		case AS_DELETE_BITMAP:
		{
			STRACE(("ServerApp %s: received BBitmap delete request\n",fSignature.String()));
			// Delete a bitmap's allocated memory

			// Attached Data:
			// 1) int32 token
			// 2) int32 reply port
		
			// Reply Code: SERVER_TRUE if successful, 
			//				SERVER_FALSE if the buffer was already deleted or was not found
			port_id replyport = -1;
			int32 bmp_id;
			
			msg.Read<int32>(&bmp_id);
			msg.Read<int32>(&replyport);
			
			replylink.SetSendPort(replyport);
			replylink.StartMessage(SERVER_TRUE);
			replylink.Flush();	
			break;
		}
		case AS_CREATE_PICTURE:
		{
			// TODO: Implement AS_CREATE_PICTURE
			STRACE(("ServerApp %s: Create Picture unimplemented\n",fSignature.String()));

			break;
		}
		case AS_DELETE_PICTURE:
		{
			// TODO: Implement AS_DELETE_PICTURE
			STRACE(("ServerApp %s: Delete Picture unimplemented\n",fSignature.String()));

			break;
		}
		case AS_CLONE_PICTURE:
		{
			// TODO: Implement AS_CLONE_PICTURE
			STRACE(("ServerApp %s: Clone Picture unimplemented\n",fSignature.String()));

			break;
		}
		case AS_DOWNLOAD_PICTURE:
		{
			// TODO; Implement AS_DOWNLOAD_PICTURE
			
			// What is this particular function call for, anyway?
			STRACE(("ServerApp %s: Download Picture unimplemented\n",fSignature.String()));

			break;
		}
		case AS_SET_SCREEN_MODE:
		case AS_ACTIVATE_WORKSPACE:
			break;
		
		case AS_SHOW_CURSOR:
			fCursorHidden=false;
			break;
		case AS_HIDE_CURSOR:
			fCursorHidden=true;
			break;
		case AS_OBSCURE_CURSOR:
			break;
		case AS_QUERY_CURSOR_HIDDEN:
		{
			STRACE(("ServerApp %s: Received IsCursorHidden request\n",fSignature.String()));
			// Attached data
			// 1) int32 port to reply to
			int32 replyport;
			msg.Read<int32>(&replyport);
			
			replylink.SetSendPort(replyport);
			replylink.StartMessage(fCursorHidden ? SERVER_TRUE : SERVER_FALSE);
			replylink.Flush();
			break;
		}
		case AS_SET_CURSOR_DATA:
		case AS_SET_CURSOR_BCURSOR:
			break;
		case AS_CREATE_BCURSOR:
		{
			STRACE(("ServerApp %s: Create BCursor\n",fSignature.String()));
			// Attached data:
			// 1) 68 bytes of fAppCursor data
			// 2) port_id reply port
			
			port_id replyport = -1;
			int8 cdata[68];

			msg.Read(cdata,68);
			msg.Read<int32>(&replyport);

			// Synchronous message - BApplication is waiting on the cursor's ID
			replylink.SetSendPort(replyport);
			replylink.StartMessage(SERVER_TRUE);
			replylink.Attach<int32>(-1);
			replylink.Flush();
			break;
		}
		case AS_DELETE_BCURSOR:
		{
			STRACE(("ServerApp %s: Delete BCursor\n",fSignature.String()));
			// Attached data:
			// 1) int32 token ID of the cursor to delete
			int32 ctoken = B_NULL_TOKEN;
			msg.Read<int32>(&ctoken);
			break;
		}
		case AS_GET_SCROLLBAR_INFO:
		{
			STRACE(("ServerApp %s: Get ScrollBar info\n",fSignature.String()));
			// Attached data:
			// 1) port_id reply port - synchronous message

			scroll_bar_info sbi;

			port_id replyport;
			msg.Read<int32>(&replyport);
			
			replylink.SetSendPort(replyport);
			replylink.StartMessage(SERVER_TRUE);
			replylink.Attach<scroll_bar_info>(sbi);
			replylink.Flush();
			break;
		}
		case AS_SET_SCROLLBAR_INFO:
		{
			STRACE(("ServerApp %s: Set ScrollBar info\n",fSignature.String()));
			// Attached Data:
			// 1) scroll_bar_info scroll bar info structure
			scroll_bar_info sbi;
			msg.Read<scroll_bar_info>(&sbi);
			break;
		}
		case AS_FOCUS_FOLLOWS_MOUSE:
		{
			STRACE(("ServerApp %s: query Focus Follow Mouse in use\n",fSignature.String()));
			// Attached data:
			// 1) port_id reply port - synchronous message

			port_id replyport;
			msg.Read<int32>(&replyport);

			replylink.SetSendPort(replyport);
			replylink.StartMessage(SERVER_TRUE);
			replylink.Attach<bool>(true);
			replylink.Flush();
			break;
		}
		case AS_SET_FOCUS_FOLLOWS_MOUSE:
		{
			STRACE(("ServerApp %s: Set Focus Follows Mouse in use\n",fSignature.String()));
/*			// Attached Data:
			// 1) scroll_bar_info scroll bar info structure
			scroll_bar_info sbi;
			msg.Read<scroll_bar_info>(&sbi);

			desktop->SetScrollBarInfo(sbi);*/
			break;
		}
		case AS_SET_MOUSE_MODE:
		{
			STRACE(("ServerApp %s: Set Focus Follows Mouse mode\n",fSignature.String()));
			// Attached Data:
			// 1) enum mode_mouse FFM mouse mode
			mode_mouse mmode;
			msg.Read<mode_mouse>(&mmode);
			break;
		}
		case AS_GET_MOUSE_MODE:
		{
			STRACE(("ServerApp %s: Get Focus Follows Mouse mode\n",fSignature.String()));
			// Attached data:
			// 1) port_id reply port - synchronous message

			mode_mouse mmode = (mode_mouse)0;

			port_id replyport = -1;
			msg.Read<int32>(&replyport);

			replylink.SetSendPort(replyport);
			replylink.StartMessage(SERVER_TRUE);
			replylink.Attach<mode_mouse>(mmode);
			replylink.Flush();
			break;
		}
		case AS_GET_UI_COLOR:
		{
			STRACE(("ServerApp %s: Get UI color\n",fSignature.String()));

			rgb_color color = {0,0,0,0};
			int32 whichcolor;
			port_id replyport = -1;

			msg.Read<int32>(&whichcolor);
			msg.Read<port_id>(&replyport);
			
			replylink.SetSendPort(replyport);
			replylink.StartMessage(SERVER_TRUE);
			replylink.Attach<rgb_color>(color);
			replylink.Flush();
			break;
		}
		case AS_UPDATED_CLIENT_FONTLIST:
		{
			STRACE(("ServerApp %s: Acknowledged update of client-side font list\n",fSignature.String()));
			break;
		}
		case AS_QUERY_FONTS_CHANGED:
		{
			FTRACE(("ServerApp %s: AS_QUERY_FONTS_CHANGED unimplemented\n",fSignature.String()));
			// Attached Data:
			// 1) bool check flag
			// 2) port_id reply_port
			
			// if just checking, just give an answer,
			// if not and needs updated, 
			// sync the font list and return true else return false
			break;
		}
		case AS_GET_FAMILY_NAME:
		{
			FTRACE(("ServerApp %s: AS_GET_FAMILY_NAME\n",fSignature.String()));
			// Attached Data:
			// 1) int32 the ID of the font family to get
			// 2) port_id reply port
			
			// Returns:
			// 1) font_family - name of family
			// 2) uint32 - flags of font family (B_IS_FIXED || B_HAS_TUNED_FONT)
			int32 famid;
			port_id replyport;
			
			msg.Read<int32>(&famid);
			msg.Read<port_id>(&replyport);
			
			replylink.SetSendPort(replyport);

			replylink.StartMessage(SERVER_FALSE);
			replylink.Flush();
			break;
		}
		case AS_GET_STYLE_NAME:
		{
			FTRACE(("ServerApp %s: AS_GET_STYLE_NAME\n",fSignature.String()));
			// Attached Data:
			// 1) font_family The name of the font family
			// 2) int32 ID of the style to get
			// 3) port_id reply port
			
			// Returns:
			// 1) font_style - name of the style
			// 2) uint16 - appropriate face values
			// 3) uint32 - flags of font style (B_IS_FIXED || B_HAS_TUNED_FONT)

			font_family fam;			
			int32 styid;
			port_id replyport;
			
			msg.Read(fam,sizeof(font_family));
			msg.Read<int32>(&styid);
			msg.Read<port_id>(&replyport);
			
			replylink.SetSendPort(replyport);
			
			replylink.StartMessage(SERVER_FALSE);
			replylink.Flush();
			break;
		}
		case AS_GET_FAMILY_AND_STYLE:
		{
			FTRACE(("ServerApp %s: AS_GET_FAMILY_AND_STYLE\n",fSignature.String()));
			// Attached Data:
			// 1) uint16 - family ID
			// 2) uint16 - style ID
			// 3) port_id reply port
			
			// Returns:
			// 1) font_family The name of the font family
			// 2) font_style - name of the style
			uint16 famid, styid;
			port_id replyport;
			
			msg.Read<uint16>(&famid);
			msg.Read<uint16>(&styid);
			msg.Read<port_id>(&replyport);

			replylink.SetSendPort(replyport);

			replylink.StartMessage(SERVER_FALSE);
			replylink.Flush();
			break;
		}
		case AS_GET_FONT_DIRECTION:
		{
			FTRACE(("ServerApp %s: AS_GET_FONT_DIRECTION unimplemented\n",fSignature.String()));
			// Attached Data:
			// 1) uint16 - family ID
			// 2) uint16 - style ID
			// 3) port_id reply port
			
			// Returns:
			// 1) font_direction direction of font
			
			// NOTE: While this may be unimplemented, we can safely return
			// SERVER_FALSE. This will force the BFont code to default to
			// B_LEFT_TO_RIGHT, which is what the vast majority of fonts will be.
			// This will be fixed later.
			int32 famid, styid;
			port_id replyport;
			
			msg.Read<int32>(&famid);
			msg.Read<int32>(&styid);
			msg.Read<port_id>(&replyport);
			
			replylink.SetSendPort(replyport);
			
/*			fontserver->Lock();
			FontStyle *fstyle=fontserver->GetStyle(famid,styid);
			if(fstyle)
			{
				font_direction dir=fstyle->GetDirection();
				
				replylink.StartMessage(SERVER_TRUE);
				replylink.Attach<font_direction>(dir);
				replylink.Flush();
			}
			else
			{
*/				replylink.StartMessage(SERVER_FALSE);
				replylink.Flush();
//			}
			
//			fontserver->Unlock();
			break;
		}
		case AS_GET_STRING_WIDTH:
		{
			FTRACE(("ServerApp %s: AS_GET_STRING_WIDTH\n",fSignature.String()));
			// Attached Data:
			// 1) string String to measure
			// 2) int32 string length to measure
			// 3) uint16 ID of family
			// 4) uint16 ID of style
			// 5) float point size of font
			// 6) uint8 spacing to use
			// 7) port_id reply port
			
			// Returns:
			// 1) float - width of the string in pixels
			char *string=NULL;
			int32 length;
			uint16 family,style;
			float size;
			uint8 spacing;
			port_id replyport;
			
			msg.ReadString(&string);
			msg.Read<int32>(&length);
			msg.Read<uint16>(&family);
			msg.Read<uint16>(&style);
			msg.Read<float>(&size);
			msg.Read<uint8>(&spacing);
			msg.Read<port_id>(&replyport);
			
			replylink.SetSendPort(replyport);
			replylink.StartMessage(SERVER_FALSE);
			replylink.Flush();
			break;
		}
		case AS_GET_FONT_BOUNDING_BOX:
		{
			FTRACE(("ServerApp %s: AS_GET_BOUNDING_BOX unimplemented\n",fSignature.String()));
			// Attached Data:
			// 1) uint16 - family ID
			// 2) uint16 - style ID
			// 3) port_id reply port
			
			// Returns:
			// 1) BRect - box holding entire font
			
			break;
		}
		case AS_GET_TUNED_COUNT:
		{
			FTRACE(("ServerApp %s: AS_GET_TUNED_COUNT\n",fSignature.String()));
			// Attached Data:
			// 1) uint16 - family ID
			// 2) uint16 - style ID
			// 3) port_id reply port
			
			// Returns:
			// 1) int32 - number of font strikes available
			int32 famid, styid;
			port_id replyport;
			
			msg.Read<int32>(&famid);
			msg.Read<int32>(&styid);
			msg.Read<port_id>(&replyport);
			
			replylink.SetSendPort(replyport);
			
			replylink.StartMessage(SERVER_FALSE);
			replylink.Flush();
			break;
		}
		case AS_GET_TUNED_INFO:
		{
			FTRACE(("ServerApp %s: AS_GET_TUNED_INFO unimplmemented\n",fSignature.String()));
			// Attached Data:
			// 1) uint16 - family ID
			// 2) uint16 - style ID
			// 3) uint32 - index of the particular font strike
			// 4) port_id reply port
			
			// Returns:
			// 1) tuned_font_info - info on the strike specified
			break;
		}
		case AS_QUERY_FONT_FIXED:
		{
			FTRACE(("ServerApp %s: AS_QUERY_FONT_FIXED unimplmemented\n",fSignature.String()));
			// Attached Data:
			// 1) uint16 - family ID
			// 2) uint16 - style ID
			// 3) port_id reply port
			
			// Returns:
			// 1) bool - font is/is not fixed
			int32 famid, styid;
			port_id replyport;
			
			msg.Read<int32>(&famid);
			msg.Read<int32>(&styid);
			msg.Read<port_id>(&replyport);
			
			replylink.SetSendPort(replyport);
			
			replylink.StartMessage(SERVER_FALSE);
			replylink.Flush();
			break;
		}
		case AS_SET_FAMILY_NAME:
		{
			FTRACE(("ServerApp %s: AS_SET_FAMILY_NAME\n",fSignature.String()));
			// Attached Data:
			// 1) font_family - name of font family to use
			// 2) port_id - reply port
			
			// Returns:
			// 1) uint16 - family ID
			
			port_id replyport;
			font_family fam;
			
			msg.Read(fam,sizeof(font_family));
			msg.Read<port_id>(&replyport);
			
			replylink.SetSendPort(replyport);
			
			replylink.StartMessage(SERVER_FALSE);
			replylink.Flush();
			break;
		}
		case AS_SET_FAMILY_AND_STYLE:
		{
			FTRACE(("ServerApp %s: AS_SET_FAMILY_AND_STYLE\n",fSignature.String()));
			// Attached Data:
			// 1) font_family - name of font family to use
			// 2) font_style - name of style in family
			// 3) port_id - reply port
			
			// Returns:
			// 1) uint16 - family ID
			// 2) uint16 - style ID
			
			port_id replyport;
			font_family fam;
			font_style sty;
			
			msg.Read(fam,sizeof(font_family));
			msg.Read(sty,sizeof(font_style));
			msg.Read<port_id>(&replyport);
			
			replylink.SetSendPort(replyport);
			
			replylink.StartMessage(SERVER_FALSE);
			replylink.Flush();
			break;
		}
		case AS_SET_FAMILY_AND_STYLE_FROM_ID:
		{
			FTRACE(("ServerApp %s: AS_SET_FAMILY_AND_STYLE_FROM_ID\n",fSignature.String()));
			// Attached Data:
			// 1) uint16 - ID of font family to use
			// 2) uint16 - ID of style in family
			// 3) port_id - reply port
			
			// Returns:
			// 1) uint16 - face of the font
			
			port_id replyport;
			uint16 fam, sty;
			
			msg.Read<uint16>(&fam);
			msg.Read<uint16>(&sty);
			msg.Read<port_id>(&replyport);
			
			replylink.SetSendPort(replyport);
			
			replylink.StartMessage(SERVER_FALSE);
			replylink.Flush();
			break;
		}
		case AS_SET_FAMILY_AND_FACE:
		{
			FTRACE(("ServerApp %s: AS_SET_FAMILY_AND_FACE unimplmemented\n",fSignature.String()));
			// Attached Data:
			// 1) font_family - name of font family to use
			// 2) uint16 - font face
			// 3) port_id - reply port
			
			// Returns:
			// 1) uint16 - family ID
			// 2) uint16 - style ID
			
			// TODO: Check R5 for error condition behavior in SET_FAMILY_AND_FACE
			break;
		}
		case AS_COUNT_FONT_FAMILIES:
		{
			FTRACE(("ServerApp %s: AS_COUNT_FONT_FAMILIES\n",fSignature.String()));
			// Attached Data:
			// 1) port_id - reply port
			
			// Returns:
			// 1) int32 - # of font families
			port_id replyport;
			
			msg.Read<port_id>(&replyport);
			
			replylink.SetSendPort(replyport);
			replylink.StartMessage(SERVER_TRUE);
			replylink.Attach<int32>(0);
			replylink.Flush();
			break;
		}
		case AS_COUNT_FONT_STYLES:
		{
			FTRACE(("ServerApp %s: AS_COUNT_FONT_STYLES\n",fSignature.String()));
			// Attached Data:
			// 1) font_family - name of font family
			// 2) port_id - reply port
			
			// Returns:
			// 1) int32 - # of font styles
			port_id replyport;
			font_family fam;
			
			msg.Read(fam,sizeof(font_family));
			msg.Read<port_id>(&replyport);
			
			replylink.SetSendPort(replyport);
			replylink.StartMessage(SERVER_FALSE);
			replylink.Flush();
			break;
		}
		case AS_SET_SYSFONT_PLAIN:
		{
			FTRACE(("ServerApp %s: AS_SET_SYSFONT_PLAIN\n",fSignature.String()));
			// Attached Data:
			// port_id reply port
			
			// Returns:
			// 1) uint16 - family ID
			// 2) uint16 - style ID
			// 3) float - size in points
			// 4) uint16 - face flags
			// 5) uint32 - font flags
			
			port_id replyport;
			msg.Read<port_id>(&replyport);
			replylink.SetSendPort(replyport);
			
			replylink.StartMessage(SERVER_FALSE);
			replylink.Flush();
			break;
		}
		case AS_GET_FONT_HEIGHT:
		{
			FTRACE(("ServerApp %s: AS_GET_FONT_HEIGHT\n",fSignature.String()));
			// Attached Data:
			// 1) uint16 family ID
			// 2) uint16 style ID
			// 3) float size
			// 4) port_id reply port
			uint16 famid,styid;
			float ptsize;
			port_id replyport;
			
			msg.Read<uint16>(&famid);
			msg.Read<uint16>(&styid);
			msg.Read<float>(&ptsize);
			msg.Read<port_id>(&replyport);
			
			replylink.SetSendPort(replyport);
			
			replylink.StartMessage(SERVER_FALSE);
			replylink.Flush();
			break;
		}
		case AS_SET_SYSFONT_BOLD:
		{
			FTRACE(("ServerApp %s: AS_SET_SYSFONT_BOLD\n",fSignature.String()));
			// Attached Data:
			// port_id reply port
			
			// Returns:
			// 1) uint16 - family ID
			// 2) uint16 - style ID
			// 3) float - size in points
			// 4) uint16 - face flags
			// 5) uint32 - font flags
			
			port_id replyport;
			msg.Read<port_id>(&replyport);
			replylink.SetSendPort(replyport);
			
			replylink.StartMessage(SERVER_FALSE);
			replylink.Flush();
			break;
		}
		case AS_SET_SYSFONT_FIXED:
		{
			FTRACE(("ServerApp %s: AS_SET_SYSFONT_FIXED\n",fSignature.String()));
			// Attached Data:
			// port_id reply port
			
			// Returns:
			// 1) uint16 - family ID
			// 2) uint16 - style ID
			// 3) float - size in points
			// 4) uint16 - face flags
			// 5) uint32 - font flags
			
			port_id replyport;
			msg.Read<port_id>(&replyport);
			replylink.SetSendPort(replyport);
			
			replylink.StartMessage(SERVER_FALSE);
			replylink.Flush();
			break;
		}
		default:
		{
			STRACE(("ServerApp %s received unhandled message code offset %s\n",fSignature.String(),
				MsgCodeToBString(code).String()));
			break;
		}
	}
}


team_id
ServerApp::ClientTeamID()
{
	return fClientTeamID;
}

