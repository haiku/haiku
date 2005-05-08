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
#include <LinkMsgReader.h>
#include <LinkMsgSender.h>
#include <List.h>
#include <String.h>
#include <PortLink.h>
#include <SysCursor.h>
#include <ColorSet.h>
#include <RGBColor.h>
#include <stdio.h>
#include <string.h>
#include <ScrollBar.h>
#include <Shape.h>
#include <ServerProtocol.h>

#include "AppServer.h"
#include "BitmapManager.h"
#include "BGet++.h"
#include "CursorManager.h"

#include "Desktop.h"
#include "DisplayDriver.h"
#include "FontServer.h"
#include "RAMLinkMsgReader.h"
#include "RootLayer.h"
#include "ServerApp.h"
#include "ServerScreen.h"
#include "ServerWindow.h"
#include "ServerCursor.h"
#include "ServerBitmap.h"
#include "ServerPicture.h"
#include "ServerConfig.h"
#include "WinBorder.h"
#include "LayerData.h"
#include "Utils.h"

//#define DEBUG_SERVERAPP

#ifdef DEBUG_SERVERAPP
#	include <stdio.h>
#	define STRACE(x) printf x
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
		:
		fClientAppPort(sendport),
		fMessagePort(rcvport),
		fClientLooperPort(clientLooperPort),
		fSignature(signature),
		fMonitorThreadID(-1),
		fClientTeamID(clientTeamID),
		fMsgReader(NULL),
		fMsgSender(NULL),
		fSWindowList(NULL),
		fBitmapList(NULL),
		fPictureList(NULL),
		fAppCursor(NULL),
		fLockSem(-1),
		fCursorHidden(false),
		fIsActive(false),
		//fHandlerToken(handlerID),
		fSharedMem(NULL),
		fQuitting(false)
{
	if (fSignature == "")
		fSignature = "application/x-vnd.NULL-application-signature";
	
	fMsgReader = new LinkMsgReader(fMessagePort);
	fMsgSender = new LinkMsgSender(fClientAppPort);

	fSWindowList = new BList();
	fBitmapList = new BList();
	fPictureList = new BList();
	
	fSharedMem = new AreaPool;

	// although this isn't pretty, ATM we have only one RootLayer.
	// there should be a way that this ServerApp be attached to a particular
	// RootLayer to know which RootLayer's cursor to modify.
	ServerCursor *defaultCursor = 
		desktop->ActiveRootLayer()->GetCursorManager().GetCursor(B_CURSOR_DEFAULT);
	
	if (defaultCursor) {
		fAppCursor = new ServerCursor(defaultCursor);
		fAppCursor->SetOwningTeam(fClientTeamID);
	}
	
	fLockSem = create_sem(1, "ServerApp sem");

	Run();

	STRACE(("ServerApp %s:\n", fSignature.String()));
	STRACE(("\tBApp port: %ld\n", fClientAppPort));
	STRACE(("\tReceiver port: %ld\n", fMessagePort));
}


//! Does all necessary teardown for application
ServerApp::~ServerApp(void)
{
	STRACE(("*ServerApp %s:~ServerApp()\n",fSignature.String()));
	
	fQuitting = true;
	
	for (int32 i = 0; i< fBitmapList->CountItems(); i++)
		delete static_cast<ServerBitmap *>(fBitmapList->ItemAt(i));
	
	fBitmapList->MakeEmpty();
	delete fBitmapList;

	for (int32 i = 0; i < fPictureList->CountItems(); i++)
		delete static_cast<ServerPicture *>(fPictureList->ItemAt(i));
	
	fPictureList->MakeEmpty();
	delete fPictureList;

	delete fMsgReader;
	delete fMsgSender;

	// This shouldn't be necessary -- all cursors owned by the app
	// should be cleaned up by RemoveAppCursors	
//	if(fAppCursor)
//		delete fAppCursor;

	// although this isn't pretty, ATM we have only one RootLayer.
	// there should be a way that this ServerApp be attached to a particular
	// RootLayer to know which RootLayer's cursor to modify.
	desktop->ActiveRootLayer()->GetCursorManager().RemoveAppCursors(fClientTeamID);
	delete_sem(fLockSem);
	
	STRACE(("#ServerApp %s:~ServerApp()\n",fSignature.String()));

	// TODO: Is this the right place for this ?
	// From what I've understood, this is the port created by
	// the BApplication (?), but if I delete it in there, GetNextMessage()
	// in the MonitorApp thread never returns. Cleanup.
	delete_port(fMessagePort);
	
	status_t dummyStatus;
	wait_for_thread(fMonitorThreadID, &dummyStatus);
	
	delete fSharedMem;
	
	STRACE(("ServerApp %s::~ServerApp(): Exiting\n", fSignature.String()));
}

/*!
	\brief Starts the ServerApp monitoring for messages
	\return false if the application couldn't start, true if everything went OK.
*/
bool
ServerApp::Run(void)
{
	// Unlike a BApplication, a ServerApp is *supposed* to return immediately
	// when its Run() function is called.	
	fMonitorThreadID = spawn_thread(MonitorApp, fSignature.String(), B_NORMAL_PRIORITY, this);
	if (fMonitorThreadID < B_OK)
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
bool
ServerApp::PingTarget(void)
{
	team_info tinfo;
	if (get_team_info(fClientTeamID,&tinfo) == B_BAD_TEAM_ID) {
		port_id serverport = find_port(SERVER_PORT_NAME);
		if (serverport == B_NAME_NOT_FOUND) {
			printf("PANIC: ServerApp %s could not find the app_server port in PingTarget()!\n",fSignature.String());
			return false;
		}
		fMsgSender->SetPort(serverport);
		fMsgSender->StartMessage(AS_DELETE_APP);
		fMsgSender->Attach(&fMonitorThreadID, sizeof(thread_id));
		fMsgSender->Flush();
		return false;
	}
	return true;
}

/*!
	\brief Send a message to the ServerApp with no attachments
	\param code ID code of the message to post
*/
void
ServerApp::PostMessage(int32 code)
{
	BPortLink link(fMessagePort);
	link.StartMessage(code);
	link.Flush();
}

/*!
	\brief Send a message to the ServerApp's BApplication
	\param msg The message to send
*/
void
ServerApp::SendMessageToClient(const BMessage *msg) const
{
	ssize_t size = msg->FlattenedSize();
	char *buffer = new char[size];
		
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
void
ServerApp::Activate(bool value)
{
	fIsActive = value;
	SetAppCursor();
}
 
//! Sets the cursor to the application cursor, if any.
void
ServerApp::SetAppCursor(void)
{
	// although this isn't pretty, ATM we have only one RootLayer.
	// there should be a way that this ServerApp be attached to a particular
	// RootLayer to know which RootLayer's cursor to modify.
	if (fAppCursor)
		desktop->ActiveRootLayer()->GetDisplayDriver()->SetCursor(fAppCursor);
}

/*!
	\brief The thread function ServerApps use to monitor messages
	\param data Pointer to the thread's ServerApp object
	\return Throwaway value - always 0
*/
int32
ServerApp::MonitorApp(void *data)
{
	// Message-dispatching loop for the ServerApp
	
	ServerApp *app = (ServerApp *)data;
	LinkMsgReader msgqueue(app->fMessagePort);
	
	int32 code;
	status_t err = B_OK;
	
	while(!app->fQuitting) {
		STRACE(("info: ServerApp::MonitorApp listening on port %ld.\n", app->fMessagePort));
		err = msgqueue.GetNextMessage(&code);
		if (err < B_OK) {
			STRACE(("ServerApp::MonitorApp(): GetNextMessage returned %s\n", strerror(err)));
			break;
		}

		switch(code) {
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

				// ServerWindow constructor will reply with port_id of a newly created port
				ServerWindow *sw = NULL;
				sw = new ServerWindow(title, app, sendPort, looperPort, token);
				sw->Init(frame, look, feel, flags, wkspaces);

				STRACE(("\nServerApp %s: New Window %s (%.1f,%.1f,%.1f,%.1f)\n",
						app->fSignature.String(),title,frame.left,frame.top,frame.right,frame.bottom));
			
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
#if TEST_MODE
				BMessage pleaseQuit(B_QUIT_REQUESTED);
				app->SendMessageToClient(&pleaseQuit);
#endif
				break;
			}
			
			case B_QUIT_REQUESTED:
			{
				STRACE(("ServerApp %s: B_QUIT_REQUESTED\n",app->fSignature.String()));
				// Our BApplication sent us this message when it quit.
				// We need to ask the app_server to delete ourself.
				port_id	serverport = find_port(SERVER_PORT_NAME);
				if (serverport == B_NAME_NOT_FOUND){
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
void
ServerApp::DispatchMessage(int32 code, LinkMsgReader &msg)
{
	LayerData ld;
	BPortLink replylink;
	
	switch(code) {
		case AS_UPDATE_COLORS:
		{
			// NOTE: R2: Eventually we will have windows which will notify their children of changes in 
			// system colors
			
/*			STRACE(("ServerApp %s: Received global UI color update notification\n",fSignature.String()));
			ServerWindow *win;
			BMessage msg(_COLORS_UPDATED);
			
			for(int32 i=0; i<fSWindowList->CountItems(); i++)
			{
				win=(ServerWindow*)fSWindowList->ItemAt(i);
				win->fWinBorder->UpdateColors();
				win->SendMessageToClient(AS_UPDATE_COLORS, msg);
			}
*/			break;
		}
		case AS_UPDATE_FONTS:
		{
			// NOTE: R2: Eventually we will have windows which will notify their children of changes in 
			// system fonts
			
/*			STRACE(("ServerApp %s: Received global font update notification\n",fSignature.String()));
			ServerWindow *win;
			BMessage msg(_FONTS_UPDATED);
			
			for(int32 i=0; i<fSWindowList->CountItems(); i++)
			{
				win=(ServerWindow*)fSWindowList->ItemAt(i);
				win->fWinBorder->UpdateFont();
				win->SendMessageToClient(AS_UPDATE_FONTS, msg);
			}
*/			break;
		}
		case AS_AREA_MESSAGE:
		{
			// This occurs in only one kind of case: a message is too big to send over a port. This
			// is really an edge case, so this shouldn't happen *too* often
			
			// Attached Data:
			// 1) area_id id of an area already owned by the server containing the message
			// 2) size_t offset of the pointer in the area
			// 3) size_t size of the message
			
			area_id area;
			size_t offset;
			size_t msgsize;
			area_info ai;
			int8 *msgpointer;
			
			msg.Read<area_id>(&area);
			msg.Read<size_t>(&offset);
			msg.Read<size_t>(&msgsize);
			
			// Part sanity check, part get base pointer :)
			if (get_area_info(area, &ai) < B_OK)
				break;
			
			msgpointer = (int8*)ai.address + offset;
			
			RAMLinkMsgReader mlink(msgpointer);
			DispatchMessage(mlink.Code(), mlink);
			
			// This is a very special case in the sense that when ServerMemIO is used for this 
			// purpose, it will be set to NOT automatically free the memory which it had 
			// requested. This is the server's job once the message has been dispatched.
			fSharedMem->ReleaseBuffer(msgpointer);
			
			break;
		}
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

			void *sharedmem = fSharedMem->GetBuffer(memsize);
			
			replylink.SetSendPort(replyport);
			if (memsize < 1 || sharedmem == NULL) {
				replylink.StartMessage(SERVER_FALSE);
				replylink.Flush();
				break;
			}
			
			area_id owningArea = area_for(sharedmem);
			area_info ai;
			
			if (owningArea == B_ERROR || get_area_info(owningArea, &ai) < B_OK)
			{
				replylink.StartMessage(SERVER_FALSE);
				replylink.Flush();
				break;
			}
			
			int32 areaoffset = ((int32*)sharedmem) - ((int32*)ai.address);
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
			int32 areaoffset;
				
			msg.Read<area_id>(&owningArea);
			msg.Read<int32>(&areaoffset);
			
			area_info areaInfo;
			if (owningArea < 0 || get_area_info(owningArea, &areaInfo) != B_OK)
				break;
			
			STRACE(("Successfully freed shared memory\n"));
			void *sharedmem = ((int32*)areaInfo.address) + areaoffset;
			
			fSharedMem->ReleaseBuffer(sharedmem);
			
			break;
		}
		case AS_UPDATE_DECORATOR:
		{
			STRACE(("ServerApp %s: Received decorator update notification\n",fSignature.String()));

			ServerWindow *win;
			
			for(int32 i = 0; i < fSWindowList->CountItems(); i++)
			{
				win = (ServerWindow*)fSWindowList->ItemAt(i);
				win->Lock();
				win->fWinBorder->UpdateDecorator();
				win->Unlock();
			}
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
			
			ServerBitmap *sbmp = bitmapmanager->CreateBitmap(r, cs, f ,bpr, s);

			STRACE(("ServerApp %s: Create Bitmap (%.1f,%.1f,%.1f,%.1f)\n",
						fSignature.String(),r.left,r.top,r.right,r.bottom));

			replylink.SetSendPort(replyport);
			if(sbmp) {
				fBitmapList->AddItem(sbmp);
				replylink.StartMessage(SERVER_TRUE);
				replylink.Attach<int32>(sbmp->Token());
				replylink.Attach<int32>(sbmp->Area());
				replylink.Attach<int32>(sbmp->AreaOffset());
			} else {
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
			replylink.SetSendPort(replyport);
			if(sbmp) {
				STRACE(("ServerApp %s: Deleting Bitmap %ld\n",fSignature.String(),bmp_id));

				fBitmapList->RemoveItem(sbmp);
				bitmapmanager->DeleteBitmap(sbmp);
				replylink.StartMessage(SERVER_TRUE);
			} else
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
		{ 	 
			STRACE(("ServerApp %s: Set Screen Mode\n",fSignature.String())); 	 
  	 
			// Attached data 	 
			// 1) int32 workspace # 	 
			// 2) uint32 screen mode 	 
			// 3) bool make default 	 
			int32 index; 	 
			uint32 mode; 	 
			bool stick; 	 
			msg.Read<int32>(&index); 	 
			msg.Read<uint32>(&mode); 	 
			msg.Read<bool>(&stick); 	 
  	 
			RootLayer *root=desktop->ActiveRootLayer(); 	 
			Workspace *workspace=root->WorkspaceAt(index); 	 
  	 
			if (!workspace) { 	 
				// apparently out of range or something, so we do nothing. :) 	 
				break; 	 
			} 	 
  	 
			// TODO: Add mode-setting code to Workspace class 	 
			//workspace->SetMode(mode,stick); 	 
			break; 	 
		}
		case AS_ACTIVATE_WORKSPACE:
		{
			STRACE(("ServerApp %s: Activate Workspace UNIMPLEMETED\n",fSignature.String()));
			
			// Attached data
			// 1) int32 workspace index
			
			// Error-checking is done in ActivateWorkspace, so this is a safe call
			int32 workspace;
			msg.Read<int32>(&workspace);
			
			break;
		}
		
		// Theoretically, we could just call the driver directly, but we will
		// call the CursorManager's version to allow for future expansion
		case AS_SHOW_CURSOR:
		{
			STRACE(("ServerApp %s: Show Cursor\n",fSignature.String()));
			// although this isn't pretty, ATM we have only one RootLayer.
			// there should be a way that this ServerApp be attached to a particular
			// RootLayer to know which RootLayer's cursor to modify.
			desktop->ActiveRootLayer()->GetDisplayDriver()->ShowCursor();
			fCursorHidden=false;
			break;
		}
		case AS_HIDE_CURSOR:
		{
			STRACE(("ServerApp %s: Hide Cursor\n",fSignature.String()));
			// although this isn't pretty, ATM we have only one RootLayer.
			// there should be a way that this ServerApp be attached to a particular
			// RootLayer to know which RootLayer's cursor to modify.
			desktop->ActiveRootLayer()->GetDisplayDriver()->HideCursor();
			fCursorHidden=true;
			break;
		}
		case AS_OBSCURE_CURSOR:
		{
			STRACE(("ServerApp %s: Obscure Cursor\n",fSignature.String()));
			// although this isn't pretty, ATM we have only one RootLayer.
			// there should be a way that this ServerApp be attached to a particular
			// RootLayer to know which RootLayer's cursor to modify.
			desktop->ActiveRootLayer()->GetDisplayDriver()->ObscureCursor();
			break;
		}
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
				desktop->ActiveRootLayer()->GetCursorManager().DeleteCursor(fAppCursor->ID());

			fAppCursor=new ServerCursor(cdata);
			fAppCursor->SetOwningTeam(fClientTeamID);
			fAppCursor->SetAppSignature(fSignature.String());
			desktop->ActiveRootLayer()->GetCursorManager().AddCursor(fAppCursor);
			// although this isn't pretty, ATM we have only one RootLayer.
			// there should be a way that this ServerApp be attached to a particular
			// RootLayer to know which RootLayer's cursor to modify.
			desktop->ActiveRootLayer()->GetDisplayDriver()->SetCursor(fAppCursor);
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
			
			// although this isn't pretty, ATM we have only one RootLayer.
			// there should be a way that this ServerApp be attached to a particular
			// RootLayer to know which RootLayer's cursor to modify.
			ServerCursor	*cursor;
			if ((cursor = desktop->ActiveRootLayer()->GetCursorManager().FindCursor(ctoken)))
				desktop->ActiveRootLayer()->GetDisplayDriver()->SetCursor(cursor);
			
			if(sync)
			{
				// the application is expecting a reply, but plans to do literally nothing
				// with the data, so we'll just reuse the cursor token variable
				replylink.SetSendPort(replyport);
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
			fAppCursor->SetOwningTeam(fClientTeamID);
			fAppCursor->SetAppSignature(fSignature.String());
			// although this isn't pretty, ATM we have only one RootLayer.
			// there should be a way that this ServerApp be attached to a particular
			// RootLayer to know which RootLayer's cursor to modify.
			desktop->ActiveRootLayer()->GetCursorManager().AddCursor(fAppCursor);
			
			// Synchronous message - BApplication is waiting on the cursor's ID
			replylink.SetSendPort(replyport);
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
			
			// although this isn't pretty, ATM we have only one RootLayer.
			// there should be a way that this ServerApp be attached to a particular
			// RootLayer to know which RootLayer's cursor to modify.
			desktop->ActiveRootLayer()->GetCursorManager().DeleteCursor(ctoken);
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

			replylink.SetSendPort(replyport);
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
			
			replylink.SetSendPort(replyport);
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
			
			replylink.SetSendPort(replyport);
			replylink.StartMessage(SERVER_TRUE);
			replylink.Attach<rgb_color>(color.GetColor32());
			replylink.Flush();
			break;
		}
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
			
			fontserver->Lock();
			FontFamily *ffamily=fontserver->GetFamily(famid);
			if(ffamily)
			{
				font_family fam;
				sprintf(fam,"%s",ffamily->Name());
				replylink.StartMessage(SERVER_TRUE);
				replylink.Attach(fam,sizeof(font_family));
				replylink.Attach<int32>(ffamily->GetFlags());
				replylink.Flush();
			}
			else
			{
				replylink.StartMessage(SERVER_FALSE);
				replylink.Flush();
			}
			
			fontserver->Unlock();
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
			
			int32 styid;
			port_id replyport;
			font_family fam;
			
			msg.Read(fam,sizeof(font_family));
			msg.Read<int32>(&styid);
			msg.Read<port_id>(&replyport);
			
			replylink.SetSendPort(replyport);
			
			fontserver->Lock();
			FontStyle *fstyle=fontserver->GetStyle(fam,styid);
			if(fstyle)
			{
				font_family sty;
				sprintf(sty,"%s",fstyle->Name());
				replylink.StartMessage(SERVER_TRUE);
				replylink.Attach(sty,sizeof(font_style));
				replylink.Attach<int32>(fstyle->GetFace());
				replylink.Attach<int32>(fstyle->GetFlags());
				replylink.Flush();
			}
			else
			{
				replylink.StartMessage(SERVER_FALSE);
				replylink.Flush();
			}
			
			fontserver->Unlock();
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
			font_family fam;
			font_style sty;
			
			msg.Read<uint16>(&famid);
			msg.Read<uint16>(&styid);
			msg.Read<port_id>(&replyport);

			replylink.SetSendPort(replyport);
			
			fontserver->Lock();
			FontStyle *fstyle=fontserver->GetStyle(famid,styid);
			if(fstyle)
			{
				sprintf(fam,"%s",fstyle->Family()->Name());
				sprintf(sty,"%s",fstyle->Name());
				replylink.StartMessage(SERVER_TRUE);
				replylink.Attach(fam,sizeof(font_family));
				replylink.Attach(sty,sizeof(font_style));
				replylink.Flush();
			}
			else
			{
				replylink.StartMessage(SERVER_FALSE);
				replylink.Flush();
			}
			
			fontserver->Unlock();

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
			float size,width=0;
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

			ServerFont font;

			if (length > 0 && font.SetFamilyAndStyle(family, style) == B_OK &&
				size > 0 && string) {

				font.SetSize(size);
				font.SetSpacing(spacing);

				width = desktop->GetDisplayDriver()->StringWidth(string, length, font);
				
				replylink.StartMessage(SERVER_TRUE);
				replylink.Attach<float>(width);
				replylink.Flush();
				
				free(string);
			} else {
				replylink.StartMessage(SERVER_FALSE);
				replylink.Flush();
			}
			
			
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
			
			fontserver->Lock();
			FontStyle *fstyle=fontserver->GetStyle(famid,styid);
			if(fstyle)
			{
				replylink.StartMessage(SERVER_TRUE);
				replylink.Attach<int32>(fstyle->TunedCount());
				replylink.Flush();
			}
			else
			{
				replylink.StartMessage(SERVER_FALSE);
				replylink.Flush();
			}
			
			fontserver->Unlock();
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
			
			fontserver->Lock();
			FontStyle *fstyle=fontserver->GetStyle(famid,styid);
			if(fstyle)
			{
				replylink.StartMessage(SERVER_TRUE);
				replylink.Attach<bool>(fstyle->IsFixedWidth());
				replylink.Flush();
			}
			else
			{
				replylink.StartMessage(SERVER_FALSE);
				replylink.Flush();
			}
			
			fontserver->Unlock();
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
			
			fontserver->Lock();
			FontFamily *ffam=fontserver->GetFamily(fam);
			if(ffam)
			{
				replylink.StartMessage(SERVER_TRUE);
				replylink.Attach<uint16>(ffam->GetID());
				replylink.Flush();
			}
			else
			{
				replylink.StartMessage(SERVER_FALSE);
				replylink.Flush();
			}
			
			fontserver->Unlock();
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
			
			fontserver->Lock();
			FontStyle *fstyle=fontserver->GetStyle(fam,sty);
			if(fstyle)
			{
				replylink.StartMessage(SERVER_TRUE);
				replylink.Attach<uint16>(fstyle->Family()->GetID());
				replylink.Attach<uint16>(fstyle->GetID());
				replylink.Flush();
			}
			else
			{
				replylink.StartMessage(SERVER_FALSE);
				replylink.Flush();
			}
			
			fontserver->Unlock();
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
			
			ServerFont font;
			if(font.SetFamilyAndStyle(fam,sty)==B_OK)
			{
				replylink.StartMessage(SERVER_TRUE);
				replylink.Attach<uint16>(font.Face());
				replylink.Flush();
			}
			else
			{
				replylink.StartMessage(SERVER_FALSE);
				replylink.Flush();
			}
			
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
			fontserver->Lock();
			replylink.StartMessage(SERVER_TRUE);
			replylink.Attach<int32>(fontserver->CountFamilies());
			replylink.Flush();
			fontserver->Unlock();
			
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
			
			fontserver->Lock();
			FontFamily *ffam=fontserver->GetFamily(fam);
			if(ffam)
			{
				replylink.StartMessage(SERVER_TRUE);
				replylink.Attach<int32>(ffam->CountStyles());
				replylink.Flush();
			}
			else
			{
				replylink.StartMessage(SERVER_FALSE);
				replylink.Flush();
			}
			
			fontserver->Unlock();
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
			
			fontserver->Lock();
			ServerFont *sf=fontserver->GetSystemPlain();
			if(sf)
			{
				replylink.StartMessage(SERVER_TRUE);
				replylink.Attach<uint16>(sf->FamilyID());
				replylink.Attach<uint16>(sf->StyleID());
				replylink.Attach<float>(sf->Size());
				replylink.Attach<uint16>(sf->Face());
				replylink.Attach<uint32>(sf->Flags());
				replylink.Flush();
			}
			else
			{
				replylink.StartMessage(SERVER_FALSE);
				replylink.Flush();
			}
			
			fontserver->Unlock();
			
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
			
			fontserver->Lock();
			FontStyle *fstyle=fontserver->GetStyle(famid,styid);
			if(fstyle)
			{
				replylink.StartMessage(SERVER_TRUE);
				replylink.Attach<font_height>(fstyle->GetHeight(ptsize));
				replylink.Flush();
			}
			else
			{
				replylink.StartMessage(SERVER_FALSE);
				replylink.Flush();
			}
			
			fontserver->Unlock();
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
			
			fontserver->Lock();
			ServerFont *sf=fontserver->GetSystemBold();
			if(sf)
			{
				replylink.StartMessage(SERVER_TRUE);
				replylink.Attach<uint16>(sf->FamilyID());
				replylink.Attach<uint16>(sf->StyleID());
				replylink.Attach<float>(sf->Size());
				replylink.Attach<uint16>(sf->Face());
				replylink.Attach<uint32>(sf->Flags());
				replylink.Flush();
			}
			else
			{
				replylink.StartMessage(SERVER_FALSE);
				replylink.Flush();
			}
			
			fontserver->Unlock();
			
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
			
			fontserver->Lock();
			ServerFont *sf=fontserver->GetSystemFixed();
			if(sf)
			{
				replylink.StartMessage(SERVER_TRUE);
				replylink.Attach<uint16>(sf->FamilyID());
				replylink.Attach<uint16>(sf->StyleID());
				replylink.Attach<float>(sf->Size());
				replylink.Attach<uint16>(sf->Face());
				replylink.Attach<uint32>(sf->Flags());
				replylink.Flush();
			}
			else
			{
				replylink.StartMessage(SERVER_FALSE);
				replylink.Flush();
			}
			
			fontserver->Unlock();
			
			break;
		}
		case AS_GET_GLYPH_SHAPES:
		{
			// Attached Data:
			// 1) uint16 - family ID
			// 2) uint16 - style ID
			// 3) float - point size
			// 4) float - shear
			// 5) float - rotation
			// 6) uint32 - flags
			// 7) int32 - numChars
			// 8) char - chars (numChars times)
			// 9) port_id - reply port
			
			// Returns:
			// 1) BShape - glyph shape
			// numChars times
			
			uint16 famid, styid;
			uint32 flags;
			float ptsize, shear, rotation;
			
			msg.Read<uint16>(&famid);
			msg.Read<uint16>(&styid);
			msg.Read<float>(&ptsize);
			msg.Read<float>(&shear);
			msg.Read<float>(&rotation);
			msg.Read<uint32>(&flags);
			
			int32 numChars;
			msg.Read<int32>(&numChars);
			
			char charArray[numChars];			
			for (int32 i = 0; i < numChars; i++)
				msg.Read<char>(&charArray[i]);
			
			port_id replyport;
			msg.Read<port_id>(&replyport);
			replylink.SetSendPort(replyport);
			
			ServerFont font;
			if (font.SetFamilyAndStyle(famid, styid) == B_OK) {
				font.SetSize(ptsize);
				font.SetShear(shear);
				font.SetRotation(rotation);
				font.SetFlags(flags);
				
				BShape **shapes = font.GetGlyphShapes(charArray, numChars);
				if (shapes) {
					replylink.StartMessage(SERVER_TRUE);
					for (int32 i = 0; i < numChars; i++) {
						replylink.AttachShape(*shapes[i]);
						delete shapes[i];
					}
					
					replylink.Flush();
					delete shapes;
				} else {
					replylink.StartMessage(SERVER_FALSE);
					replylink.Flush();
				}
			} else {
				replylink.StartMessage(SERVER_FALSE);
				replylink.Flush();
			}
			
			break;
		}
		case AS_GET_ESCAPEMENTS:
		{
			// Attached Data:
			// 1) uint16 - family ID
			// 2) uint16 - style ID
			// 3) float - point size
			// 4) float - rotation
			// 5) uint32 - flags
			// 6) int32 - numChars
			
			// 7) char - char     -\       both
			// 8) BPoint - offset -/ (numChars times)
			
			// 9) port_id - reply port
			
			// Returns:
			// 1) BPoint - escapement
			// numChars times
			
			uint16 famid, styid;
			uint32 flags;
			float ptsize, rotation;
			
			msg.Read<uint16>(&famid);
			msg.Read<uint16>(&styid);
			msg.Read<float>(&ptsize);
			msg.Read<float>(&rotation);
			msg.Read<uint32>(&flags);
			
			int32 numChars;
			msg.Read<int32>(&numChars);
			
			char charArray[numChars];			
			BPoint offsetArray[numChars];
			for (int32 i = 0; i < numChars; i++) {
				msg.Read<char>(&charArray[i]);
				msg.Read<BPoint>(&offsetArray[i]);
			}
			
			port_id replyport;
			msg.Read<port_id>(&replyport);
			replylink.SetSendPort(replyport);
			
			ServerFont font;
			if (font.SetFamilyAndStyle(famid, styid) == B_OK) {
				font.SetSize(ptsize);
				font.SetRotation(rotation);
				font.SetFlags(flags);
				
				BPoint *esc = font.GetEscapements(charArray, numChars, offsetArray);
				if (esc) {
					replylink.StartMessage(SERVER_TRUE);
					for (int32 i = 0; i < numChars; i++) {
						replylink.Attach<BPoint>(esc[i]);
					}
					
					replylink.Flush();
					delete esc;
				} else {
					replylink.StartMessage(SERVER_FALSE);
					replylink.Flush();
				}
			} else {
				replylink.StartMessage(SERVER_FALSE);
				replylink.Flush();
			}
			
			break;
		}
		case AS_GET_ESCAPEMENTS_AS_FLOATS:
		{
			// Attached Data:
			// 1) uint16 - family ID
			// 2) uint16 - style ID
			// 3) float - point size
			// 4) float - rotation
			// 5) uint32 - flags
			// 6) int32 - numChars
			// 7) char - char
			// 8) port_id - reply port
			
			// Returns:
			// 1) float - escapement
			// numChars times
			
			uint16 famid, styid;
			uint32 flags;
			float ptsize, rotation;
			
			msg.Read<uint16>(&famid);
			msg.Read<uint16>(&styid);
			msg.Read<float>(&ptsize);
			msg.Read<float>(&rotation);
			msg.Read<uint32>(&flags);
			
			int32 numChars;
			msg.Read<int32>(&numChars);
			
			char charArray[numChars];			
			BPoint offsetArray[numChars];
			for (int32 i = 0; i < numChars; i++) {
				msg.Read<char>(&charArray[i]);
				msg.Read<BPoint>(&offsetArray[i]);
			}
			
			port_id replyport;
			msg.Read<port_id>(&replyport);
			replylink.SetSendPort(replyport);
			
			// TODO: Implement AS_GET_ESCAPEMENTS_AS_FLOATS and the float version of ServerFont::GetEscapements()
			replylink.StartMessage(SERVER_FALSE);
			replylink.Flush();
			
			break;
		}
		case AS_SCREEN_GET_MODE:
		{
			// Attached data
			// 1) int32 port to reply to
			// 2) screen_id
			// 3) workspace index
			screen_id id;
			msg.Read<screen_id>(&id);
			uint32 workspace;
			msg.Read<uint32>(&workspace);
			
			// TODO: the display_mode can be different between
			// the various screens.
			// We have the screen_id and the workspace number, with these
			// we need to find the corresponding "driver", and call getmode on it
			display_mode mode;
			desktop->GetDisplayDriver()->GetMode(&mode);
			// actually this isn't still enough as different workspaces can
			// have different display_modes
			
			int32 replyport;
			msg.Read<int32>(&replyport);
			
			replylink.SetSendPort(replyport);
			replylink.StartMessage(SERVER_TRUE);
			replylink.Attach<display_mode>(mode);
			replylink.Attach<status_t>(B_OK);
			replylink.Flush();
			
			break;
		}
		case AS_SCREEN_SET_MODE:
		{
			// Attached data
			// 1) int32 port to reply to
			// 2) screen_id
			// 3) workspace index
			// 4) display_mode to set
			// 5) 'makedefault' boolean
			// TODO: See above: workspaces support, etc.
		
			screen_id id;
			msg.Read<screen_id>(&id);
			
			uint32 workspace;
			msg.Read<uint32>(&workspace);
			
			display_mode mode;
			msg.Read<display_mode>(&mode);
			
			bool makedefault = false;
			msg.Read<bool>(&makedefault);
			
			// TODO: Adi doesn't like this: see if
			// messaging is better.
			desktop->ActiveRootLayer()->Lock();
			// TODO: This should return something
			desktop->GetDisplayDriver()->SetMode(mode);
			desktop->ActiveRootLayer()->Unlock();
		
			int32 replyport;
			msg.Read<int32>(&replyport);
			
			replylink.SetSendPort(replyport);
			replylink.StartMessage(SERVER_TRUE);
			replylink.Attach<status_t>(B_OK);
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
ServerBitmap *
ServerApp::FindBitmap(int32 token) const
{
	ServerBitmap *bitmap;
	for (int32 i = 0; i < fBitmapList->CountItems(); i++) {
		bitmap = static_cast<ServerBitmap *>(fBitmapList->ItemAt(i));
		if (bitmap && bitmap->Token() == token)
			return bitmap;
	}
	
	return NULL;
}


team_id
ServerApp::ClientTeamID() const
{
	return fClientTeamID;
}


thread_id
ServerApp::MonitorThreadID() const
{
	return fMonitorThreadID;
}

