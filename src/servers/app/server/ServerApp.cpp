//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
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
#include <SysCursor.h>
#include <ColorSet.h>
#include <RGBColor.h>
#include <stdio.h>
#include <string.h>
#include <ScrollBar.h>
#include <ServerProtocol.h>

#include "AppServer.h"
#include "BitmapManager.h"
#include "CursorManager.h"
#include "Desktop.h"
#include "DisplayDriver.h"
#include "FontServer.h"
#include "ServerApp.h"
#include "ServerWindow.h"
#include "ServerCursor.h"
#include "ServerBitmap.h"
#include "ServerPicture.h"
#include "ServerConfig.h"
#include "WinBorder.h"
#include "LayerData.h"
#include "Utils.h"

#define DEBUG_SERVERAPP

#ifdef DEBUG_SERVERAPP
#	include <stdio.h>
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
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

	fAppLink = new BPortLink(fClientAppPort, fMessagePort);

	fSWindowList=new BList(0);
	fBitmapList=new BList(0);
	fPictureList=new BList(0);
	fIsActive=false;

	ServerCursor *defaultc=cursormanager->GetCursor(B_CURSOR_DEFAULT);
	
	fAppCursor=(defaultc)?new ServerCursor(defaultc):NULL;
	fLockSem=create_sem(1,"ServerApp sem");

	// Does this even belong here any more? --DW
//	_driver=desktop->GetDisplayDriver();

	fCursorHidden=false;

	Run();

	STRACE(("ServerApp %s:\n",fSignature.String()));
	STRACE(("\tBApp port: %ld\n",fClientAppPort));
	STRACE(("\tReceiver port: %ld\n",fMessagePort));
}

//! Does all necessary teardown for application
ServerApp::~ServerApp(void)
{
	STRACE(("*ServerApp %s:~ServerApp()\n",fSignature.String()));
	int32 i;
	
//	WindowBroadcast(AS_QUIT_APP);
	
// TODO: wait for our ServerWindow threads.
/*
	bool ready=true;
	desktop->fLayerLock.Lock();
	do{
		ready	= true;

		int32	count = desktop->fWinBorderList.CountItems();
		for( int32 i = 0; i < count; i++)
		{
			ServerWindow *sw = ((WinBorder*)desktop->fWinBorderList.ItemAt(i))->Window();
			if (ClientTeamID() == sw->ClientTeamID())
			{
				thread_id		tid = sw->ThreadID();
				status_t		temp;

				desktop->fLayerLock.Unlock();
				
				printf("waiting for thread %s\n", sw->Title());
				wait_for_thread(tid, &temp);

				desktop->fLayerLock.Lock();

				ready		= false;
				break;
			}
		}
	} while(!ready);
	desktop->fLayerLock.Unlock();
*/
/*	
	ServerWindow *tempwin;
	for(i=0;i<fSWindowList->CountItems();i++)
	{
		tempwin=(ServerWindow*)fSWindowList->ItemAt(i);
		if(tempwin)
			delete tempwin;
	}
	fSWindowList->MakeEmpty();
	delete fSWindowList;
*/

	ServerBitmap *tempbmp;
	for(i=0;i<fBitmapList->CountItems();i++)
	{
		tempbmp=(ServerBitmap*)fBitmapList->ItemAt(i);
		if(tempbmp)
			delete tempbmp;
	}
	fBitmapList->MakeEmpty();
	delete fBitmapList;

	ServerPicture *temppic;
	for(i=0;i<fPictureList->CountItems();i++)
	{
		temppic=(ServerPicture*)fPictureList->ItemAt(i);
		if(temppic)
			delete temppic;
	}
	fPictureList->MakeEmpty();
	delete fPictureList;

	delete fAppLink;
	fAppLink=NULL;
	if(fAppCursor)
		delete fAppCursor;


	cursormanager->RemoveAppCursors(fSignature.String());
	delete_sem(fLockSem);
	
	STRACE(("#ServerApp %s:~ServerApp()\n",fSignature.String()));

	// Kill the monitor thread if it exists
	thread_info info;
	if(get_thread_info(fMonitorThreadID,&info)==B_OK)
		kill_thread(fMonitorThreadID);

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

/*!
	\brief Pings the target app to make sure it's still working
	\return true if target is still "alive" and false if "He's dead, Jim." 
	"But that's impossible..."
	
	This function is called by the app_server thread to ensure that
	the target app still exists. We do this not by sending a message
	but by calling get_port_info. We don't want to send ping messages
	just because the app might simply be hung. If this is the case, it
	should be up to the user to kill it. If the app has been killed, its
	ports will be invalid. Thus, if get_port_info returns an error, we
	tell the app_server to delete the respective ServerApp.
*/
bool ServerApp::PingTarget(void)
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
		fAppLink->SetSendPort(serverport);
		fAppLink->StartMessage(AS_DELETE_APP);
		fAppLink->Attach(&fMonitorThreadID,sizeof(thread_id));
		fAppLink->Flush();
		return false;
	}
	return true;
}

/*!
	\brief Send a message to the ServerApp with no attachments
	\param code ID code of the message to post
*/
void ServerApp::PostMessage(int32 code, size_t size, int8 *buffer)
{
	write_port(fMessagePort,code, buffer, size);
}

/*!
	\brief Send a simple message to all of the ServerApp's ServerWindows
	\param msg The message code to broadcast
*/
/*
void ServerApp::WindowBroadcast(int32 code)
{
	desktop->fLayerLock.Lock();
	int32 count=desktop->fWinBorderList.CountItems();
	for(int32 i=0; i<count; i++)
	{
		ServerWindow *sw = ((WinBorder*)desktop->fWinBorderList.ItemAt(i))->Window();
		BMessage msg(code);
		sw->Lock();
		sw->SendMessageToClient(&msg);
		sw->Unlock();
	}
	desktop->fLayerLock.Unlock();
}
*/
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
	if(fAppCursor)
		cursormanager->SetCursor(fAppCursor->ID());
	else
		cursormanager->SetCursor(B_CURSOR_DEFAULT);
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
	BPortLink msgqueue(-1, app->fMessagePort);
	bool quitting = false;
	int32 code;
	status_t err = B_OK;
	
	while(!quitting)
	{
		STRACE(("info: ServerApp::MonitorApp listening on port %ld.\n", app->fMessagePort));
		err = msgqueue.GetNextReply(&code);
		if (err < B_OK)
			break;

		switch(code)
		{
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
				if(DISPLAYDRIVER!=HWDRIVER)
				{
					BMessage pleaseQuit(B_QUIT_REQUESTED);
					app->SendMessageToClient(&pleaseQuit);
				}
				break;
			}
			
			// TODO: Fix
			// Using this case is a hack. The ServerApp is receiving a message with a '0' code after
			// it sends the quit message on server shutdown and I can't find what's sending it. This
			// must be found and fixed!
			case 0:
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
				app->fAppLink->SetSendPort(serverport);
				app->fAppLink->StartMessage(AS_DELETE_APP);
				app->fAppLink->Attach(&app->fMonitorThreadID, sizeof(thread_id));
				app->fAppLink->Flush();
				break;
			}
			default:
			{
				STRACE(("ServerApp %s: Got a Message to dispatch\n",app->fSignature.String()));
				app->_DispatchMessage(code, msgqueue);
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
void ServerApp::_DispatchMessage(int32 code, BPortLink& msg)
{
	LayerData ld;
	switch(code)
	{
		case AS_UPDATED_CLIENT_FONTLIST:
		{
			STRACE(("ServerApp %s: Acknowledged update of client-side font list\n",fSignature.String()));
			
			// received when the client-side global font list has been
			// refreshed
			fontserver->Lock();
			fontserver->FontsUpdated();
			fontserver->Unlock();
			break;
		}
		case AS_UPDATE_COLORS:
		{
			// Eventually we will have windows which will notify their children of changes in 
			// system colors
			
/*			STRACE(("ServerApp %s: Received global UI color update notification\n",fSignature.String()));
			ServerWindow *win;
			BMessage msg(_COLORS_UPDATED);
			
			for(int32 i=0; i<fSWindowList->CountItems(); i++)
			{
				win=(ServerWindow*)fSWindowList->ItemAt(i);
				win->Lock();
				win->fWinBorder->UpdateColors();
				win->SendMessageToClient(AS_UPDATE_COLORS, msg);
				win->Unlock();
			}
*/			break;
		}
		case AS_UPDATE_FONTS:
		{
			// Eventually we will have windows which will notify their children of changes in 
			// system fonts
			
/*			STRACE(("ServerApp %s: Received global font update notification\n",fSignature.String()));
			ServerWindow *win;
			BMessage msg(_FONTS_UPDATED);
			
			for(int32 i=0; i<fSWindowList->CountItems(); i++)
			{
				win=(ServerWindow*)fSWindowList->ItemAt(i);
				win->Lock();
				win->fWinBorder->UpdateFont();
				win->SendMessageToClient(AS_UPDATE_FONTS, msg);
				win->Unlock();
			}
*/			break;
		}
		case AS_UPDATE_DECORATOR:
		{
			STRACE(("ServerApp %s: Received decorator update notification\n",fSignature.String()));

			ServerWindow *win;
			
			for(int32 i=0; i<fSWindowList->CountItems(); i++)
			{
				win=(ServerWindow*)fSWindowList->ItemAt(i);
				win->Lock();
				win->fWinBorder->UpdateDecorator();
				win->Unlock();
			}
			break;
		}
		case AS_CREATE_WINDOW:
		{
			// Create the ServerWindow to node monitor a new OBWindow
			
			// Attached data:
			// 2) BRect window frame
			// 3) uint32 window look
			// 4) uint32 window feel
			// 5) uint32 window flags
			// 6) uint32 workspace index
			// 7) int32 BHandler token of the window
			// 8) port_id window's message port
			// 9) const char * title

			BRect frame;
			uint32 look;
			uint32 feel;
			uint32 flags;
			uint32 wkspaces;
			int32 token = B_NULL_TOKEN;
			port_id	sendPort = -1;
			port_id looperPort = -1;
			char *title = NULL;
			port_id replyport = -1;
			
			msg.Read<BRect>(&frame);
			msg.Read<int32>((int32*)&look);
			msg.Read<int32>((int32*)&feel);
			msg.Read<int32>((int32*)&flags);
			msg.Read<int32>((int32*)&wkspaces);
			msg.Read<int32>(&token);
			msg.Read<port_id>(&sendPort);
			msg.Read<port_id>(&looperPort);
			msg.ReadString(&title);

			//TODO: deprecate - just use sendPort
//			msg.Read<port_id>(&replyport); 

			STRACE(("ServerApp %s: Got 'New Window' message, trying to do smething...\n",fSignature.String()));

			// ServerWindow constructor will reply with port_id of a newly created port
			ServerWindow *sw = NULL;
			sw = new ServerWindow(frame, title, look, feel, flags, this,
						sendPort, looperPort, replyport, wkspaces, token);
			sw->Init();

			STRACE(("\nServerApp %s: New Window %s (%.1f,%.1f,%.1f,%.1f)\n",
					fSignature.String(),title,frame.left,frame.top,frame.right,frame.bottom));
			
			if (title)
				free(title);

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
			
			ServerBitmap *sbmp=bitmapmanager->CreateBitmap(r,cs,f,bpr,s);

			STRACE(("ServerApp %s: Create Bitmap (%.1f,%.1f,%.1f,%.1f)\n",
						fSignature.String(),r.left,r.top,r.right,r.bottom));

			BPortLink replylink(replyport);
			if(sbmp)
			{
				fBitmapList->AddItem(sbmp);
				replylink.StartMessage(SERVER_TRUE);
				replylink.Attach<int32>(sbmp->Token());
				replylink.Attach<int32>(sbmp->Area());
				replylink.Attach<int32>(sbmp->AreaOffset());
			}
			else
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
			
			ServerBitmap *sbmp=FindBitmap(bmp_id);
			BPortLink replylink(replyport);
			if(sbmp)
			{
				STRACE(("ServerApp %s: Deleting Bitmap %ld\n",fSignature.String(),bmp_id));

				fBitmapList->RemoveItem(sbmp);
				bitmapmanager->DeleteBitmap(sbmp);
				replylink.StartMessage(SERVER_TRUE);
			}
			else
				replylink.StartMessage(SERVER_TRUE);
			replylink.Flush();	
			break;
		}
		case AS_CREATE_PICTURE:
		{
			// TODO: Implement
			STRACE(("ServerApp %s: Create Picture unimplemented\n",fSignature.String()));

			break;
		}
		case AS_DELETE_PICTURE:
		{
			// TODO: Implement
			STRACE(("ServerApp %s: Delete Picture unimplemented\n",fSignature.String()));

			break;
		}
		case AS_CLONE_PICTURE:
		{
			// TODO: Implement
			STRACE(("ServerApp %s: Clone Picture unimplemented\n",fSignature.String()));

			break;
		}
		case AS_DOWNLOAD_PICTURE:
		{
			// TODO; Implement
			STRACE(("ServerApp %s: Download Picture unimplemented\n",fSignature.String()));

			break;
		}
		case AS_SET_SCREEN_MODE:
		{
			STRACE(("ServerApp %s: Set Screen Mode\n",fSignature.String()));

			// Attached data
			// 1) int32 workspace #
			// 2) uint32 screen mode
			// 3) bool make default
			int32 workspace;
			uint32 mode;
			bool stick;
			msg.Read<int32>(&workspace);
			msg.Read<uint32>(&mode);
			msg.Read<bool>(&stick);

			//TODO: Resolve			
			//SetSpace(workspace,mode,ActiveScreen(),stick);

			break;
		}
		case AS_ACTIVATE_WORKSPACE:
		{
			STRACE(("ServerApp %s: Activate Workspace\n",fSignature.String()));
			// Attached data
			// 1) int32 workspace index
			
			// Error-checking is done in ActivateWorkspace, so this is a safe call
			int32 workspace;
			msg.Read<int32>(&workspace);

			//TODO: Resolve
			//SetWorkspace(workspace);
			break;
		}
		
		// Theoretically, we could just call the driver directly, but we will
		// call the CursorManager's version to allow for future expansion
		case AS_SHOW_CURSOR:
		{
			STRACE(("ServerApp %s: Show Cursor\n",fSignature.String()));
			cursormanager->ShowCursor();
			fCursorHidden=false;
			break;
		}
		case AS_HIDE_CURSOR:
		{
			STRACE(("ServerApp %s: Hide Cursor\n",fSignature.String()));
			cursormanager->HideCursor();
			fCursorHidden=true;
			break;
		}
		case AS_OBSCURE_CURSOR:
		{
			STRACE(("ServerApp %s: Obscure Cursor\n",fSignature.String()));
			cursormanager->ObscureCursor();
			break;
		}
		case AS_QUERY_CURSOR_HIDDEN:
		{
			STRACE(("ServerApp %s: Received IsCursorHidden request\n",fSignature.String()));
			// Attached data
			// 1) int32 port to reply to
			int32 replyport;
			msg.Read<int32>(&replyport);
			
			BPortLink replylink(replyport);
			replylink.StartMessage(fCursorHidden ? SERVER_TRUE : SERVER_FALSE);
			replylink.Flush();
			break;
		}
		case AS_SET_CURSOR_DATA:
		{
			STRACE(("ServerApp %s: SetCursor via cursor data\n",fSignature.String()));
			// Attached data: 68 bytes of fAppCursor data
			
			int8 cdata[68];
			msg.Read(cdata,68);
			
			// Because we don't want an overaccumulation of these particular
			// cursors, we will delete them if there is an existing one. It would
			// otherwise be easy to crash the server by calling SetCursor a
			// sufficient number of times
			if(fAppCursor)
				cursormanager->DeleteCursor(fAppCursor->ID());

			fAppCursor=new ServerCursor(cdata);
			fAppCursor->SetAppSignature(fSignature.String());
			cursormanager->AddCursor(fAppCursor);
			cursormanager->SetCursor(fAppCursor->ID());
			break;
		}
		case AS_SET_CURSOR_BCURSOR:
		{
			STRACE(("ServerApp %s: SetCursor via BCursor\n",fSignature.String()));
			// Attached data:
			// 1) bool flag to send a reply
			// 2) int32 token ID of the cursor to set
			// 3) port_id port to receive a reply. Only exists if the sync flag is true.
			bool sync;
			int32 ctoken = B_NULL_TOKEN;
			port_id replyport = -1;
			
			msg.Read<bool>(&sync);
			msg.Read<int32>(&ctoken);
			if(sync)
				msg.Read<int32>(&replyport);
			
			cursormanager->SetCursor(ctoken);
			
			if(sync)
			{
				// the application is expecting a reply, but plans to do literally nothing
				// with the data, so we'll just reuse the cursor token variable
				BPortLink replylink(replyport);
				replylink.StartMessage(SERVER_TRUE);
				replylink.Flush();
			}
			break;
		}
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

			fAppCursor=new ServerCursor(cdata);
			fAppCursor->SetAppSignature(fSignature.String());
			cursormanager->AddCursor(fAppCursor);
			
			// Synchronous message - BApplication is waiting on the cursor's ID
			BPortLink replylink(replyport);
			replylink.StartMessage(SERVER_TRUE);
			replylink.Attach<int32>(fAppCursor->ID());
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
			
			if(fAppCursor && fAppCursor->ID()==ctoken)
				fAppCursor=NULL;
			
			cursormanager->DeleteCursor(ctoken);
			break;
		}
		case AS_GET_SCROLLBAR_INFO:
		{
			STRACE(("ServerApp %s: Get ScrollBar info\n",fSignature.String()));
			// Attached data:
			// 1) port_id reply port - synchronous message

			scroll_bar_info sbi=desktop->ScrollBarInfo();

			port_id replyport;
			msg.Read<int32>(&replyport);
			
			BPortLink replylink(replyport);
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

			desktop->SetScrollBarInfo(sbi);
			break;
		}
		case AS_FOCUS_FOLLOWS_MOUSE:
		{
			STRACE(("ServerApp %s: query Focus Follow Mouse in use\n",fSignature.String()));
			// Attached data:
			// 1) port_id reply port - synchronous message

			port_id replyport;
			msg.Read<int32>(&replyport);

			BPortLink replylink(replyport);
			replylink.StartMessage(SERVER_TRUE);
			replylink.Attach<bool>(desktop->FFMouseInUse());
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

			desktop->SetFFMouseMode(mmode);
			break;
		}
		case AS_GET_MOUSE_MODE:
		{
			STRACE(("ServerApp %s: Get Focus Follows Mouse mode\n",fSignature.String()));
			// Attached data:
			// 1) port_id reply port - synchronous message

			mode_mouse mmode=desktop->FFMouseMode();
			
			port_id replyport = -1;
			msg.Read<int32>(&replyport);
			
			BPortLink replylink(replyport);
			replylink.StartMessage(SERVER_TRUE);
			replylink.Attach<mode_mouse>(mmode);
			replylink.Flush();
			break;
		}
		case AS_GET_UI_COLOR:
		{
			STRACE(("ServerApp %s: Get UI color\n",fSignature.String()));

			RGBColor color;
			int32 whichcolor;
			port_id replyport = -1;
			
			msg.Read<int32>(&whichcolor);
			msg.Read<port_id>(&replyport);
			
			gui_colorset.Lock();
			color=gui_colorset.AttributeToColor(whichcolor);
			gui_colorset.Unlock();
			
			BPortLink replylink(replyport);
			replylink.StartMessage(SERVER_TRUE);
			replylink.Attach<rgb_color>(color.GetColor32());
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

/*!
	\brief Looks up a ServerApp's ServerBitmap in its list
	\param token ID token of the bitmap to find
	\return The bitmap having that ID or NULL if not found
*/
ServerBitmap *ServerApp::FindBitmap(int32 token)
{
	ServerBitmap *temp;
	for(int32 i=0; i<fBitmapList->CountItems();i++)
	{
		temp=(ServerBitmap*)fBitmapList->ItemAt(i);
		if(temp && temp->Token()==token)
			return temp;
	}
	return NULL;
}

team_id ServerApp::ClientTeamID()
{
	return fClientTeamID;
}

