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

#include <Session.h>

#include <stdio.h>
#include <string.h>
#include <ScrollBar.h>
#include <ServerProtocol.h>

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
	\param _signature NULL-terminated string which contains the BApplication's
	MIME _signature.
*/
ServerApp::ServerApp(port_id sendport, port_id rcvport, int32 handlerID, char *signature)
{
	// need to copy the _signature because the message buffer
	// owns the copy which we are passed as a parameter.
	_signature=(signature)?signature:"application/x-vnd.unregistered-application";
	
	// token ID of the BApplication's BHandler object. Used for BMessage target specification
	_handlertoken=handlerID;

	// _sender is the our BApplication's event port
	_sender=sendport;
	_applink=new PortLink(_sender);
	_applink->SetPort(_sender);

	// Gotta get the team ID so we can ping the application
	port_info pinfo;
	get_port_info(_sender,&pinfo);
	_target_id=pinfo.team;
	
	// _receiver is the port we receive messages from our BApplication
	_receiver=rcvport;

	_winlist=new BList(0);
	_bmplist=new BList(0);
	_piclist=new BList(0);
	_isactive=false;

	ServerCursor *defaultc=cursormanager->GetCursor(B_CURSOR_DEFAULT);
	
	_appcursor=(defaultc)?new ServerCursor(defaultc):NULL;
	_lock=create_sem(1,"ServerApp sem");

	_driver=GetGfxDriver(ActiveScreen());
	_cursorhidden=false;

STRACE(("ServerApp %s:\n",_signature.String()));
STRACE(("\tBApp port: %ld\n",_sender));
STRACE(("\tReceiver port: %ld\n",_receiver));
}

//! Does all necessary teardown for application
ServerApp::~ServerApp(void)
{
STRACE(("ServerApp %s:~ServerApp()\n",_signature.String()));
	int32 i;
	
	ServerWindow *tempwin;
	for(i=0;i<_winlist->CountItems();i++)
	{
		tempwin=(ServerWindow*)_winlist->ItemAt(i);
		if(tempwin)
			delete tempwin;
	}
	_winlist->MakeEmpty();
	delete _winlist;

	ServerBitmap *tempbmp;
	for(i=0;i<_bmplist->CountItems();i++)
	{
		tempbmp=(ServerBitmap*)_bmplist->ItemAt(i);
		if(tempbmp)
			delete tempbmp;
	}
	_bmplist->MakeEmpty();
	delete _bmplist;

	ServerPicture *temppic;
	for(i=0;i<_piclist->CountItems();i++)
	{
		temppic=(ServerPicture*)_piclist->ItemAt(i);
		if(temppic)
			delete temppic;
	}
	_piclist->MakeEmpty();
	delete _piclist;

// ADI:
	delete	ses;
	ses		= NULL;

	delete _applink;
	_applink=NULL;
	if(_appcursor)
		delete _appcursor;

	// Kill the monitor thread if it exists
	thread_info info;
	if(get_thread_info(_monitor_thread,&info)==B_OK)
		kill_thread(_monitor_thread);

	cursormanager->RemoveAppCursors(_signature.String());
	delete_sem(_lock);
}

/*!
	\brief Starts the ServerApp monitoring for messages
	\return false if the application couldn't start, true if everything went OK.
*/
bool ServerApp::Run(void)
{
	// Unlike a BApplication, a ServerApp is *supposed* to return immediately
	// when its Run() function is called.
	_monitor_thread=spawn_thread(MonitorApp,_signature.String(),B_NORMAL_PRIORITY,this);
	if(_monitor_thread==B_NO_MORE_THREADS || _monitor_thread==B_NO_MEMORY)
		return false;

	resume_thread(_monitor_thread);
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
	if(get_team_info(_target_id,&tinfo)==B_BAD_TEAM_ID)
	{
		port_id serverport=find_port(SERVER_PORT_NAME);
		if(serverport==B_NAME_NOT_FOUND)
		{
			printf("PANIC: ServerApp %s could not find the app_server port in PingTarget()!\n",_signature.String());
			return false;
		}
		_applink->SetPort(serverport);
		_applink->SetOpCode(AS_DELETE_APP);
		_applink->Attach(&_monitor_thread,sizeof(thread_id));
		_applink->Flush();
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
	write_port(_receiver,code, buffer, size);
}

/*!
	\brief Sets the ServerApp's active status
	\param value The new status of the ServerApp.
	
	This changes an internal flag and also sets the current cursor to the one specified by
	the application
*/
void ServerApp::Activate(bool value)
{
	_isactive=value;
	SetAppCursor();
}
 
//! Sets the cursor to the application cursor, if any.
void ServerApp::SetAppCursor(void)
{
	if(_appcursor)
		cursormanager->SetCursor(_appcursor->ID());
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
	ServerApp *app=(ServerApp *)data;

	// Message-dispatching loop for the ServerApp
	int32			msgCode;
	
	app->ses = new BSession( app->_receiver, 0L );
	
	for(;;)
	{
		if(app->ses->ReadInt32(&msgCode)!= B_BAD_PORT_ID ){
		printf("Received message %ld\n",msgCode);
			switch (msgCode){
				case AS_QUIT_APP:
				{
					STRACE(("ServerApp %s:Server shutdown notification received\n",app->_signature.String()));
					// If we are using the real, accelerated version of the
					// DisplayDriver, we do NOT want the user to be able shut down
					// the server. The results would NOT be pretty
					if(DISPLAYDRIVER!=HWDRIVER)
					{
						// This message is received from the app_server thread
						// because the server was asked to quit. Thus, we
						// ask all apps to quit. This is NOT the same as system
						// shutdown and will happen only in testing
						
						// For now, it seems that we can't seem to try to play nice and tell
						// an application to quit, so we'll just have to go postal. XD

//						BMessage *shutdown=new BMessage(_QUIT_);
//						SendMessage(app->_sender,shutdown);

						// DIE! DIE! DIE! :P
						port_info pi;
						get_port_info(app->_sender,&pi);
						kill_team(pi.team);
						app->PostMessage(B_QUIT_REQUESTED);
					}
					break;
				}
				
				case B_QUIT_REQUESTED:
				{
					// Our BApplication sent us this message when it quit.
					// We need to ask the app_server to delete our monitor
					port_id serverport=find_port(SERVER_PORT_NAME);
					if(serverport==B_NAME_NOT_FOUND)
					{
						printf("PANIC: ServerApp %s could not find the app_server port!\n",app->_signature.String());
						break;
					}
					app->_applink->SetPort(serverport);
					app->_applink->SetOpCode(AS_DELETE_APP);
					app->_applink->Attach(&app->_monitor_thread,sizeof(thread_id));
					app->_applink->Flush();
					break;
				}
				default:
				{
					STRACE(("ServerApp %s: Got a Message to dispatch\n",app->_signature.String()));
					app->_DispatchMessage(msgCode, NULL);
					break;
				}
			} // switch
		} // if 
	} // for 

	exit_thread(0);
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
void ServerApp::_DispatchMessage(int32 code, int8 *buffer)
{
	int8 *index=buffer;
	LayerData ld;

	switch(code)
	{
		case AS_UPDATED_CLIENT_FONTLIST:
		{
			// received when the client-side global font list has been
			// refreshed
			fontserver->Lock();
			fontserver->FontsUpdated();
			fontserver->Unlock();
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

			BRect		frame;
			uint32		look;
			uint32		feel;
			uint32		flags;
			uint32		wkspaces;
			int32		token;
			port_id		sendPort;
			port_id		looperPort;
			char		*title;
			
			ses->ReadRect( &frame );
			ses->ReadInt32( (int32*)&look );
			ses->ReadInt32( (int32*)&feel );
			ses->ReadInt32( (int32*)&flags );
			ses->ReadInt32( (int32*)&wkspaces );
			ses->ReadInt32( &token );
			ses->ReadData( &sendPort, sizeof(port_id) );
			ses->ReadData( &looperPort, sizeof(port_id) );
			title		= ses->ReadString();

			STRACE(("ServerApp %s: Got 'New Window' message, trying to do smething...\n",
					_signature.String()));

				// ServerWindow constructor will reply with port_id of a newly created port
			ServerWindow *newwin	= new ServerWindow( frame, title,
				look, feel, flags, this, sendPort, looperPort, wkspaces, token);
			_winlist->AddItem( newwin );
			//AddWindowToDesktop( newwin, workspace, ActiveScreen() );
			

			STRACE(("ServerApp %s: New Window %s (%.1f,%.1f,%.1f,%.1f)\n",
					_signature.String(),title,frame.left,frame.top,frame.right,frame.bottom));
			
			delete	title;

			break;
		}
		case AS_DELETE_WINDOW:
		{
			// Received from a ServerWindow when its window quits
			
			// Attached data:
			// 1) uint32  ServerWindow ID token
			ServerWindow *w;
			uint32 winid=*((uint32*)index);

			for(int32 i=0;i<_winlist->CountItems();i++)
			{
				w=(ServerWindow*)_winlist->ItemAt(i);
				if(w->_token==winid)
				{
					STRACE(("ServerApp %s: Deleting window %s\n",_signature.String(),w->GetTitle()));
					_winlist->RemoveItem(w);
					delete w;
					break;
				}
			}
			break;
		}
		case AS_CREATE_BITMAP:
		{
			// Allocate a bitmap for an application
			
			// Attached Data: 
			// 1) port_id reply port
			// 2) BRect bounds
			// 3) color_space space
			// 4) int32 bitmap_flags
			// 5) int32 bytes_per_row
			// 6) int32 screen_id::id
			
			// Reply Code: SERVER_TRUE
			// Reply Data:
			//	1) int32 server token
			//	2) area_id id of the area in which the bitmap data resides
			//	3) int32 area pointer offset used to calculate fBasePtr
			
			// First, let's attempt to allocate the bitmap
			port_id replyport=*((port_id*)index); index+=sizeof(port_id);
			BRect r=*((BRect*)index); index+=sizeof(BRect);
			color_space cs=*((color_space*)index); index+=sizeof(color_space);
			int32 f=*((int32*)index); index+=sizeof(int32);
			int32 bpr=*((int32*)index); index+=sizeof(int32);

			screen_id s;
			s.id=*((int32*)index);
			
			ServerBitmap *sbmp=bitmapmanager->CreateBitmap(r,cs,f,bpr,s);

			STRACE(("ServerApp %s: Create Bitmap (%.1f,%.1f,%.1f,%.1f)\n",
						_signature.String(),r.left,r.top,r.right,r.bottom));

			if(sbmp)
			{
				// list for doing faster lookups for a bitmap than what the BitmapManager
				// can do.
				_bmplist->AddItem(sbmp);
				_applink->SetOpCode(SERVER_TRUE);
				_applink->Attach(sbmp->Token());
				_applink->Attach(sbmp->Area());
				_applink->Attach(sbmp->AreaOffset());
				_applink->Flush();
			}
			else
			{
				// alternatively, if something went wrong, we reply with SERVER_FALSE
				write_port(replyport,SERVER_FALSE,NULL,0);
			}
			
			break;
		}
		case AS_DELETE_BITMAP:
		{
			// Delete a bitmap's allocated memory

			// Attached Data:
			// 1) int32 reply port
			// 2) int32 token
		
			// Reply Code: SERVER_TRUE if successful, 
			//				SERVER_FALSE if the buffer was already deleted or was not found
			port_id replyport=*((port_id*)index); index+=sizeof(port_id);
			
			ServerBitmap *sbmp=_FindBitmap(*((int32*)index));
			if(sbmp)
			{
				STRACE(("ServerApp %s: Deleting Bitmap %ld\n",_signature.String(),*((int32*)index)));

				_bmplist->RemoveItem(sbmp);
				bitmapmanager->DeleteBitmap(sbmp);
				write_port(replyport,SERVER_TRUE,NULL,0);
			}
			else
				write_port(replyport,SERVER_FALSE,NULL,0);
			
			break;
		}
		case AS_CREATE_PICTURE:
		{
			// TODO: Implement
			STRACE(("ServerApp %s: Create Picture unimplemented\n",_signature.String()));

			break;
		}
		case AS_DELETE_PICTURE:
		{
			// TODO: Implement
			STRACE(("ServerApp %s: Delete Picture unimplemented\n",_signature.String()));

			break;
		}
		case AS_CLONE_PICTURE:
		{
			// TODO: Implement
			STRACE(("ServerApp %s: Clone Picture unimplemented\n",_signature.String()));

			break;
		}
		case AS_DOWNLOAD_PICTURE:
		{
			// TODO; Implement
			STRACE(("ServerApp %s: Download Picture unimplemented\n",_signature.String()));

			break;
		}
		case AS_SET_SCREEN_MODE:
		{
			// Attached data
			// 1) int32 workspace #
			// 2) uint32 screen mode
			// 3) bool make default
			int32 workspace=*((int32*)index); index+=sizeof(int32);
			uint32 mode=*((uint32*)index); index+=sizeof(uint32);

			SetSpace(workspace,mode,ActiveScreen(),*((bool*)index));

			break;
		}
		case AS_ACTIVATE_WORKSPACE:
		{
			// Attached data
			// 1) int32 workspace index
			
			// Error-checking is done in ActivateWorkspace, so this is a safe call
			SetWorkspace(*((int32*)index));
			break;
		}
		
		// Theoretically, we could just call the driver directly, but we will
		// call the CursorManager's version to allow for future expansion
		case AS_SHOW_CURSOR:
		{
			cursormanager->ShowCursor();
			_cursorhidden=false;
			break;
		}
		case AS_HIDE_CURSOR:
		{
			cursormanager->HideCursor();
			_cursorhidden=true;
			break;
		}
		case AS_OBSCURE_CURSOR:
		{
			cursormanager->ObscureCursor();
			break;
		}
		case AS_QUERY_CURSOR_HIDDEN:
		{
			// Attached data
			// 1) int32 port to reply to
			write_port(*((port_id*)index),(_cursorhidden)?SERVER_TRUE:SERVER_FALSE,NULL,0);
			break;
		}
		case AS_SET_CURSOR_DATA:
		{
			// Attached data: 68 bytes of _appcursor data
			
			int8 cdata[68];
			memcpy(cdata, buffer, 68);

			// Because we don't want an overaccumulation of these particular
			// cursors, we will delete them if there is an existing one. It would
			// otherwise be easy to crash the server by calling SetCursor a
			// sufficient number of times
			if(_appcursor)
				cursormanager->DeleteCursor(_appcursor->ID());

			_appcursor=new ServerCursor(cdata);
			_appcursor->SetAppSignature(_signature.String());
			cursormanager->AddCursor(_appcursor);
			cursormanager->SetCursor(_appcursor->ID());
			break;
		}
		case AS_SET_CURSOR_BCURSOR:
		{
			// Attached data:
			// 1) int32 token ID of the cursor to set

			if(!index)
				cursormanager->SetCursor(*((int32*)index));
			break;
		}
		case AS_CREATE_BCURSOR:
		{
printf("Create BCursor\n");
			// Attached data:
			// 1) port_id reply port
			// 2) 68 bytes of _appcursor data
			
			port_id replyport=*((port_id*)index);
			index+=sizeof(port_id);
			
			int8 cdata[68];
			memcpy(cdata, index, 68);

			_appcursor=new ServerCursor(cdata);
			_appcursor->SetAppSignature(_signature.String());
			cursormanager->AddCursor(_appcursor);
			
			// Synchronous message - BApplication is waiting on the cursor's ID
			PortLink replylink(replyport);
			replylink.SetOpCode(AS_CREATE_BCURSOR);
			replylink.Attach(_appcursor->ID());
			replylink.Flush();
			break;
		}
		case AS_DELETE_BCURSOR:
		{
			// Attached data:
			// 1) int32 token ID of the cursor to delete
			if(_appcursor && _appcursor->ID()==*((int32*)index))
				_appcursor=NULL;
			
			if(!index)
				cursormanager->DeleteCursor(*((int32*)index));
			break;
		}
		case AS_GET_SCROLLBAR_INFO:
		{
			// Attached data:
			// 1) port_id reply port - synchronous message

			PortLink replylink( *((port_id*)index) );

			scroll_bar_info sbi=GetScrollBarInfo();
			
			replylink.SetOpCode(AS_GET_SCROLLBAR_INFO);
			replylink.Attach(&sbi,sizeof(scroll_bar_info));
			replylink.Flush();
			
			break;
		}
		case AS_SET_SCROLLBAR_INFO:
		{
			// Attached Data:
			// 1) scroll_bar_info scroll bar info structure
			SetScrollBarInfo(*((scroll_bar_info*)index));
			break;
		}
		case AS_FOCUS_FOLLOWS_MOUSE:
		{
			// Attached data:
			// 1) port_id reply port - synchronous message

			PortLink replylink( *((port_id*)index) );
			
			replylink.SetOpCode(AS_FOCUS_FOLLOWS_MOUSE);
			replylink.Attach(GetFFMouse());
			replylink.Flush();
			break;
		}
		case AS_SET_FOCUS_FOLLOWS_MOUSE:
		{
			// Attached Data:
			// 1) scroll_bar_info scroll bar info structure
			SetScrollBarInfo(*((scroll_bar_info*)index));
			break;
		}
		case AS_SET_MOUSE_MODE:
		{
			// Attached Data:
			// 1) enum mode_mouse FFM mouse mode
			SetFFMouseMode(*((mode_mouse*)index));
			break;
		}
		case AS_GET_MOUSE_MODE:
		{
			// Attached data:
			// 1) port_id reply port - synchronous message

			PortLink replylink( *((port_id*)index) );
			
			replylink.SetOpCode(AS_FOCUS_FOLLOWS_MOUSE);
			mode_mouse mode=GetFFMouseMode();
			replylink.Attach(&mode,sizeof(mode_mouse));
			replylink.Flush();
			break;
		}
		default:
		{
			STRACE(("ServerApp %s received unhandled message code offset %s\n",_signature.String(),MsgCodeToString(code)));

			break;
		}
	}
}

/*!
	\brief Looks up a ServerApp's ServerBitmap in its list
	\param token ID token of the bitmap to find
	\return The bitmap having that ID or NULL if not found
*/
ServerBitmap *ServerApp::_FindBitmap(int32 token)
{
	ServerBitmap *temp;
	for(int32 i=0; i<_bmplist->CountItems();i++)
	{
		temp=(ServerBitmap*)_bmplist->ItemAt(i);
		if(temp && temp->Token()==token)
			return temp;
	}
	return NULL;
}

