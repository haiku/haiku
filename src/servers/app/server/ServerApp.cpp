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
#include <stdio.h>
#include <string.h>

#include "CursorManager.h"
#include "Desktop.h"
#include "DisplayDriver.h"
#include "FontServer.h"
#include "ServerApp.h"
#include "ServerWindow.h"
#include "ServerCursor.h"
#include "ServerBitmap.h"
#include "ServerProtocol.h"
#include "ServerConfig.h"
#include "LayerData.h"

/*!
	\brief Constructor
	\param sendport port ID for the BApplication which will receive the
	ServerApp's messages
	\param rcvport port by which the ServerApp will receive messages from its
	BApplication.
	\param _signature NULL-terminated string which contains the BApplication's
	MIME _signature.
*/
ServerApp::ServerApp(port_id sendport, port_id rcvport, char *signature)
{
	// need to copy the _signature because the message buffer
	// owns the copy which we are passed as a parameter.
	_signature=(signature)?signature:"BeApplication";

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
	_isactive=false;

	ServerCursor *defaultc=cursormanager->GetCursor(B_CURSOR_DEFAULT);
	
	_appcursor=(defaultc)?new ServerCursor(defaultc):NULL;
	_lock=create_sem(1,"ServerApp sem");

	_driver=GetGfxDriver();
}

//! Does all necessary teardown for application
ServerApp::~ServerApp(void)
{
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
		_applink->SetOpCode(DELETE_APP);
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
	\brief The thread function ServerApps use to monitor messages
	\param data Pointer to the thread's ServerApp object
	\return Throwaway value - always 0
*/
int32 ServerApp::MonitorApp(void *data)
{
	ServerApp *app=(ServerApp *)data;

	// Message-dispatching loop for the ServerApp
	int32 msgcode;
	int8 *msgbuffer=NULL;
	ssize_t buffersize,bytesread;
	
	for(;;)
	{
		buffersize=port_buffer_size(app->_receiver);
		
		if(buffersize>0)
		{
			// buffers are PortLink messages. Allocate necessary buffer and
			// we'll cast it as a BMessage.
			msgbuffer=new int8[buffersize];
			bytesread=read_port(app->_receiver,&msgcode,msgbuffer,buffersize);
		}
		else
			bytesread=read_port(app->_receiver,&msgcode,NULL,0);
		if (bytesread != B_BAD_PORT_ID && bytesread != B_TIMED_OUT && bytesread != B_WOULD_BLOCK)
		{
			switch(msgcode)
			{
// -------------- Messages received from the Server ------------------------
				// Mouse messages simply get passed onto the active app for now
				case B_MOUSE_UP:
				case B_MOUSE_DOWN:
				case B_MOUSE_MOVED:
				{
					// everything is formatted as it should be, so just call
					// write_port.
					write_port(app->_sender, msgcode, msgbuffer, buffersize);
					break;
				}
				case QUIT_APP:
				{
					// If we are using the real, accelerated version of the
					// DisplayDriver, we do NOT want the user to be able shut down
					// the server. The results would NOT be pretty
					if(DISPLAYDRIVER==HWDRIVER)
					{
						// This message is received from the app_server thread
						// because the server was asked to quit. Thus, we
						// ask all apps to quit. This is NOT the same as system
						// shutdown and will happen only in testing
						app->_applink->SetOpCode(B_QUIT_REQUESTED);
						app->_applink->Flush();
					}
					break;
				}
// -------------- Messages received from the Application ------------------
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
					app->_applink->SetOpCode(DELETE_APP);
					app->_applink->Attach(&app->_monitor_thread,sizeof(thread_id));
					app->_applink->Flush();
					break;
				}
				default:
				{
					app->DispatchMessage(msgcode, msgbuffer);
					break;
				}
			}

		}
	
		if(buffersize>0)
			delete msgbuffer;

		if(msgcode==B_QUIT_REQUESTED)
			break;
	}

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
void ServerApp::DispatchMessage(int32 code, int8 *buffer)
{
	int8 *index=buffer;
	LayerData ld;

	switch(code)
	{
		case UPDATED_CLIENT_FONTLIST:
		{
			// received when the client-side global font list has been
			// refreshed
			fontserver->Lock();
			fontserver->FontsUpdated();
			fontserver->Unlock();
			break;
		}
		case CREATE_WINDOW:
		{
			// Create the ServerWindow to node monitor a new OBWindow
			
			// Attached data:
			// 1) port_id reply port
			// 2) BRect window frame
			// 3) uint32 window flags
			// 4) port_id window's message port
			// 5) uint32 workspace index
			// 6) const char * title

			// Find the necessary data
			port_id reply_port=*((port_id*)index); index+=sizeof(port_id);
			BRect rect=*((BRect*)index); index+=sizeof(BRect);

			uint32 winlook=*((uint32*)index); index+=sizeof(uint32);
			uint32 winfeel=*((uint32*)index); index+=sizeof(uint32);
			uint32 winflags=*((uint32*)index); index+=sizeof(uint32);

			port_id win_port=*((port_id*)index); index+=sizeof(port_id);
			uint32 workspace=*((uint32*)index); index+=sizeof(uint32);

			// Create the ServerWindow object for this window
			ServerWindow *newwin=new ServerWindow(rect,(const char *)index,
				winlook, winfeel, winflags,this,win_port,workspace);
			_winlist->AddItem(newwin);

			// Window looper is waiting for our reply. Send back the
			// ServerWindow's message port
			PortLink *replylink=new PortLink(reply_port);
			replylink->SetOpCode(SET_SERVER_PORT);
			replylink->Attach((int32)newwin->_receiver);
			replylink->Flush();

			delete replylink;
			break;
		}
		case DELETE_WINDOW:
		{
			// Received from a ServerWindow when its window quits
			
			// Attached data:
			// 1) thread_id  ServerWindow ID
			thread_id winid;
			ServerWindow *w;
			for(int32 i=0;i<_winlist->CountItems();i++)
			{
				w=(ServerWindow*)_winlist->ItemAt(i);
				if(w->_monitorthread==winid)
				{
					_winlist->RemoveItem(w);
					delete w;
					break;
				}
			}
			break;
		}
		case GFX_SET_SCREEN_MODE:
		{
			// Attached data
			// 1) int32 workspace #
			// 2) uint32 screen mode
			// 3) bool make default
			int32 workspace=*((int32*)index); index+=sizeof(int32);
			uint32 mode=*((uint32*)index); index+=sizeof(uint32);

			SetSpace(workspace,mode,*((bool*)index));

			break;
		}
		case GFX_ACTIVATE_WORKSPACE:
		{
			// Attached data
			// 1) int32 workspace index
			
			// Error-checking is done in ActivateWorkspace, so this is a safe call
			SetWorkspace(*((int32*)index));
			break;
		}
		
		// Theoretically, we could just call the driver directly, but we will
		// call the CursorManager's version to allow for future expansion
		case SHOW_CURSOR:
		{
			cursormanager->ShowCursor();
			break;
		}
		case HIDE_CURSOR:
		{
			cursormanager->HideCursor();
			break;
		}
		case OBSCURE_CURSOR:
		{
			cursormanager->ObscureCursor();
			break;
		}
		
		case SET_CURSOR_DATA:
		{
			// Attached data: 68 bytes of _appcursor data
			
			// Get the data, update the app's _appcursor, and update the
			// app's _appcursor if active.
			int8 cdata[68];
			memcpy(cdata, buffer, 68);

			// Because we don't want an overaccumulation of these particular
			// cursors, we will delete them if there is an existing one. It would
			// otherwise be easy to crash the server by calling SetCursor a
			// sufficient number of times
			if(_appcursor)
				cursormanager->DeleteCursor(_appcursor->ID());

			_appcursor=new ServerCursor(cdata);
			cursormanager->AddCursor(_appcursor);
			cursormanager->SetCursor(_appcursor->ID());
			break;
		}
		case SET_CURSOR_BCURSOR:
		{
			// TODO: Implement
			break;
		}
		default:
		{
			printf("ServerApp %s received unhandled message code %lx\n",
				_signature.String(),code);
			break;
		}
	}
}
