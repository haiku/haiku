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
//	File Name:		ServerWindow.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Shadow BWindow class
//  
//------------------------------------------------------------------------------
#include <AppDefs.h>
#include <Rect.h>
#include <string.h>
#include <stdio.h>
#include <Debug.h>
#include <View.h>	// for B_XXXXX_MOUSE_BUTTON defines
#include "AppServer.h"
#include "Layer.h"
#include "PortLink.h"
#include "ServerWindow.h"
#include "ServerApp.h"
#include "ServerProtocol.h"
#include "WinBorder.h"
#include "Desktop.h"
#include "TokenHandler.h"

//! Handler to get BWindow tokens from
TokenHandler win_token_handler;

// defined in WinBorder.cpp. Locking is not necessary - the only
// _monitorthread which accesses them is the Poller _monitorthread

//! Used in window focus management
ServerWindow *active_serverwindow=NULL;

/*!
	\brief Contructor
	
	Sets up a lot of the stuff
*/
ServerWindow::ServerWindow(BRect rect, const char *string, uint32 wlook,
	uint32 wfeel, uint32 wflags, ServerApp *winapp,  port_id winport, uint32 index)
{
	_title=new BString;
	_title->SetTo( (string)?string:"Window" );

	// This must happen before the WinBorder object - it needs this object's _frame
	// to be valid
	_frame=rect;

	// set these early also - the _decorator will also need them
	_flags=wflags;
	_look=wlook;
	_feel=wfeel;

	_decorator=new_decorator(_frame,_title->String(),_look,_feel,_flags,GetGfxDriver());

	_winborder=new WinBorder(_frame,_title->String(),0,wflags,this);

	// _sender is the monitored _app's event port
	_sender=winport;
	_winlink=new PortLink(_sender);

	_applink= (winapp)? new PortLink(winapp->_receiver) : NULL;
	
	// _receiver is the port to which the _app sends messages for the server
	_receiver=create_port(30,_title->String());
	
	_active=false;
	_hidecount=0;

	// Spawn our message monitoring _monitorthread
	_monitorthread=spawn_thread(MonitorWin,_title->String(),B_NORMAL_PRIORITY,this);
	if(_monitorthread!=B_NO_MORE_THREADS && _monitorthread!=B_NO_MEMORY)
		resume_thread(_monitorthread);

	_workspace=index;

	AddWindowToDesktop(this,index);
}

ServerWindow::~ServerWindow(void)
{
	RemoveWindowFromDesktop(this);
	if(_applink)
	{
		delete _applink;
		_applink=NULL;
		delete _title;
		delete _winlink;
		delete _decorator;
//	TODO: uncomment this when we have WinBorder.h	
//		delete _winborder;
	}
	kill_thread(_monitorthread);
}

void ServerWindow::RequestDraw(BRect rect)
{
	_winlink->SetOpCode(LAYER_DRAW);
	_winlink->Attach(&rect,sizeof(BRect));
	_winlink->Flush();
}

void ServerWindow::ReplaceDecorator(void)
{
}

void ServerWindow::Quit(void)
{
}

const char *ServerWindow::GetTitle(void)
{
	return _title->String();
}

ServerApp *ServerWindow::GetApp(void)
{
	return _app;
}

void ServerWindow::Show(void)
{
	if(_winborder)
	{
//	TODO: uncomment this when we have WinBorder.h	
//		_winborder->ShowLayer();
		ActivateWindow(this);
	}
}

void ServerWindow::Hide(void)
{
//	TODO: uncomment this when we have WinBorder.h	
//	if(_winborder)
//		_winborder->HideLayer();
}

bool ServerWindow::IsHidden(void)
{
	return (_hidecount==0)?true:false;
}

void ServerWindow::SetFocus(bool value)
{
	if(_active!=value)
	{
		_active=value;
		_decorator->SetFocus(value);
		_decorator->Draw();
	}
}

bool ServerWindow::HasFocus(void)
{
	return _active;
}

void ServerWindow::WorkspaceActivated(int32 newworkspace, const BPoint reso, color_space cspace)
{
}

void ServerWindow::WindowActivated(bool active)
{
}

void ServerWindow::ScreenModeChanged(const BPoint res, color_space cspace)
{
}

void ServerWindow::SetFrame(const BRect &rect)
{
	_frame=rect;
}

BRect ServerWindow::Frame(void)
{
	return _frame;
}

status_t ServerWindow::Lock(void)
{
#ifdef DEBUG_SERVERWIN
printf("%s::Lock()\n",_title->String());
#endif
	return _locker.Lock();
}

void ServerWindow::Unlock(void)
{
#ifdef DEBUG_SERVERWIN
printf("%s::Unlock()\n",_title->String());
#endif
	_locker.Unlock();
}

bool ServerWindow::IsLocked(void)
{
	return _locker.IsLocked();
}

void ServerWindow::Loop(void)
{
	// Message-dispatching loop for the ServerWindow
	int32 msgcode;
	int8 *msgbuffer=NULL;
	ssize_t buffersize,bytesread;
	
	for(;;)
	{
		buffersize=port_buffer_size(_receiver);
		
		if(buffersize>0)
		{
			msgbuffer=new int8[buffersize];
			bytesread=read_port(_receiver,&msgcode,msgbuffer,buffersize);
		}
		else
			bytesread=read_port(_receiver,&msgcode,NULL,0);

		if (bytesread != B_BAD_PORT_ID && bytesread != B_TIMED_OUT && bytesread != B_WOULD_BLOCK)
		{
			switch(msgcode)
			{
				case LAYER_CREATE:
				{
					// Received when a view is attached to a window. This will require
					// us to attach a layer in the tree in the same manner and invalidate
					// the area in which the new layer resides assuming that it is
					// visible.
				
					// Attached Data:
					// 1) (int32) id of the parent view
					// 2) (BRect) frame in parent's coordinates
					// 3) (int32) resize flags
					// 4) (int32) view flags
					// 5) (uint16) view's hide level
					
					// This is a synchronous call, so reply immediately with the ID of the layer
					// so the BView can identify itself

					break;
				}
				case LAYER_DELETE:
				{
					// Received when a view is detached from a window. This is definitely
					// the less taxing operation - we call PruneTree() on the removed
					// layer, detach the layer itself, delete it, and invalidate the
					// area assuming that the view was visible when removed
					
					// Attached Data:
					// 1) (int32) id of the removed view
					break;
				}
				case SHOW_WINDOW:
				{
					Show();
					break;
				}
				case HIDE_WINDOW:
				{
					Hide();
					break;
				}
				case DELETE_WINDOW:
				{
					// Received from our window when it is told to quit, so tell our
					// application to delete this object
					_applink->SetOpCode(DELETE_WINDOW);
					_applink->Attach((int32)_monitorthread);
					_applink->Flush();
					break;
				}
				default:
					DispatchMessage(msgcode,msgbuffer);
					break;
			}

		}
	
		if(buffersize>0)
			delete msgbuffer;

		if(msgcode==B_QUIT_REQUESTED)
			break;
	}
}

void ServerWindow::DispatchMessage(int32 code, int8 *msgbuffer)
{
	switch(code)
	{
		default:
		{
#ifdef DEBUG_SERVERWIN
printf("%s::ServerWindow(): Received unexpected code: ",_title->String());
PrintMessageCode(code);
#endif
			break;
		}
	}
}

int32 ServerWindow::MonitorWin(void *data)
{
	ServerWindow *win=(ServerWindow *)data;
	win->Loop();
	exit_thread(0);
	return 0;
}

void ServerWindow::HandleMouseEvent(int32 code, int8 *buffer)
{
/*	ServerWindow *mousewin=NULL;
	int8 *index=buffer;
	
	// Find the window which will receive our mouse event.
	Layer *root=GetRootLayer();
	WinBorder *_winborder;
	
	// activeborder is used to remember windows when resizing/moving windows
	// or sliding a tab
	ASSERT(root!=NULL);

	// Dispatch the mouse event to the proper window
	switch(code)
	{
		case B_MOUSE_DOWN:
		{
			// Attached data:
			// 1) int64 - time of mouse click
			// 2) float - x coordinate of mouse click
			// 3) float - y coordinate of mouse click
			// 4) int32 - modifier keys down
			// 5) int32 - buttons down
			// 6) int32 - clicks

//			int64 time=*((int64*)index);
			index+=sizeof(int64);
			float x=*((float*)index);
			index+=sizeof(float);
			float y=*((float*)index);
			index+=sizeof(float);
			int32 modifiers=*((int32*)index);
			index+=sizeof(uint32);
			uint32 buttons=*((uint32*)index);
			index+=sizeof(uint32);
//			int32 clicks=*((int32*)index);
			BPoint pt(x,y);

			// If we have clicked on a window, 			
			_winborder=(WinBorder*)root->GetChildAt(pt);
			if(_winborder)
			{
				mousewin=_winborder->Window();
				_winborder->MouseDown(pt,buttons,modifiers);
			}
			break;
		}
		case B_MOUSE_UP:
		{
			// Attached data:
			// 1) int64 - time of mouse click
			// 2) float - x coordinate of mouse click
			// 3) float - y coordinate of mouse click
			// 4) int32 - modifier keys down

//			int64 time=*((int64*)index);
			index+=sizeof(int64);
			float x=*((float*)index);
			index+=sizeof(float);
			float y=*((float*)index);
			index+=sizeof(float);
//			int32 modifiers=*((int32*)index);
			BPoint pt(x,y);
			
			set_is_moving_window(false);
			_winborder=(WinBorder*)root->GetChildAt(pt);
			if(_winborder)
			{
				mousewin=_winborder->Window();
				
				// Eventually, we will build in MouseUp messages with buttons specified
				// For now, we just "assume" no mouse specification with a 0.
//				_winborder->MouseUp(pt,0,0);
				
			}
			break;
		}
		case B_MOUSE_MOVED:
		{
			// Attached data:
			// 1) int64 - time of mouse click
			// 2) float - x coordinate of mouse click
			// 3) float - y coordinate of mouse click
			// 4) int32 - buttons down
//			int64 time=*((int64*)index);
			index+=sizeof(int64);
			float x=*((float*)index);
			index+=sizeof(float);
			float y=*((float*)index);
			index+=sizeof(float);
			uint32 buttons=*((uint32*)index);
			BPoint pt(x,y);

			if(is_moving_window() || is_resizing_window() || is_sliding_tab())
			{
				mousewin=active_serverwindow;
				mousewin->_winborder->MouseMoved(pt,buttons,0);
			}
			else
			{
				_winborder=(WinBorder*)root->GetChildAt(pt);
				if(_winborder)
				{
					mousewin=_winborder->Window();
					_winborder->MouseMoved(pt,buttons,0);
				}
			}				
			break;
		}
		default:
		{
			break;
		}
	}
*/
}

void ServerWindow::HandleKeyEvent(int32 code, int8 *buffer)
{
}

void ActivateWindow(ServerWindow *win)
{
	if(active_serverwindow==win)
		return;
	if(active_serverwindow)
		active_serverwindow->SetFocus(false);
	active_serverwindow=win;
	if(win)
		win->SetFocus(true);
}
