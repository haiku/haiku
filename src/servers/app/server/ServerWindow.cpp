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
#include <View.h>	// for B_XXXXX_MOUSE_BUTTON defines
#include "AppServer.h"
#include "Layer.h"
#include "PortLink.h"
#include "ServerWindow.h"
#include "ServerApp.h"
#include "ServerProtocol.h"
#include "WinBorder.h"
#include "Desktop.h"
#include "DesktopClasses.h"
#include "TokenHandler.h"

//#define DEBUG_SERVERWINDOW
//#define DEBUG_SERVERWINDOW_MOUSE
//#define DEBUG_SERVERWINDOW_KEYBOARD

#ifdef DEBUG_SERVERWINDOW
#include <stdio.h>
#endif

//! Handler to get BWindow tokens from
TokenHandler win_token_handler;

/*!
	\brief Contructor
	
	Does a lot of stuff to set up for the window - new decorator, new winborder, spawn a 
	monitor thread.
*/
ServerWindow::ServerWindow(BRect rect, const char *string, uint32 wlook,
	uint32 wfeel, uint32 wflags, ServerApp *winapp,  port_id winport, uint32 index, int32 handlerID)
{
	_title=new BString;
	if(string)
		_title->SetTo(string);
	_frame=rect;
	_flags=wflags;
	_look=wlook;
	_feel=wfeel;
	_handlertoken=handlerID;

	_winborder=new WinBorder(_frame,_title->String(),wlook,wfeel,wflags,this);

	// _sender is the monitored window's event port
	_sender=winport;
	_winlink=new PortLink(_sender);

	_applink= (winapp)? new PortLink(winapp->_receiver) : NULL;
	
	// _receiver is the port to which the app sends messages for the server
	_receiver=create_port(30,_title->String());
	
	_active=false;

	// Spawn our message monitoring _monitorthread
	_monitorthread=spawn_thread(MonitorWin,_title->String(),B_NORMAL_PRIORITY,this);
	if(_monitorthread!=B_NO_MORE_THREADS && _monitorthread!=B_NO_MEMORY)
		resume_thread(_monitorthread);

	_workspace_index=index;
	_workspace=NULL;

	_token=win_token_handler.GetToken();

	AddWindowToDesktop(this,index,ActiveScreen());
#ifdef DEBUG_SERVERWINDOW
printf("ServerWindow %s:\n",_title->String());
printf("\tFrame (%.1f,%.1f,%.1f,%.1f)\n",rect.left,rect.top,rect.right,rect.bottom);
printf("\tPort: %ld\n",_receiver);
printf("\tWorkspace: %ld\n",index);
#endif
}

//!Tears down all connections with the user application, kills the monitoring thread.
ServerWindow::~ServerWindow(void)
{
#ifdef DEBUG_SERVERWINDOW
printf("ServerWindow %s:~ServerWindow()\n",_title->String());
#endif
	RemoveWindowFromDesktop(this);
	if(_applink)
	{
		delete _applink;
		_applink=NULL;
		delete _title;
		delete _winlink;
		delete _winborder;
	}
	kill_thread(_monitorthread);
}

/*!
	\brief Requests an update of the specified rectangle
	\param rect The area to update, in the parent's coordinates
	
	This could be considered equivalent to BView::Invalidate()
*/
void ServerWindow::RequestDraw(BRect rect)
{
#ifdef DEBUG_SERVERWINDOW
printf("ServerWindow %s: Request Draw\n",_title->String());
#endif
	_winlink->SetOpCode(AS_LAYER_DRAW);
	_winlink->Attach(&rect,sizeof(BRect));
	_winlink->Flush();
}

//! Requests an update for the entire window
void ServerWindow::RequestDraw(void)
{
	RequestDraw(_frame);
}

//! Forces the window border to update its decorator
void ServerWindow::ReplaceDecorator(void)
{
#ifdef DEBUG_SERVERWINDOW
printf("ServerWindow %s: Replace Decorator\n",_title->String());
#endif
	_winborder->UpdateDecorator();
}

//! Requests that the ServerWindow's BWindow quit
void ServerWindow::Quit(void)
{
#ifdef DEBUG_SERVERWINDOW
printf("ServerWindow %s: Quit\n",_title->String());
#endif
	_winlink->SetOpCode(B_QUIT_REQUESTED);
	_winlink->Flush();
}

/*!
	\brief Gets the title for the window
	\return The title for the window
*/
const char *ServerWindow::GetTitle(void)
{
	return _title->String();
}

/*!
	\brief Gets the window's ServerApp
	\return The ServerApp for the window
*/
ServerApp *ServerWindow::GetApp(void)
{
	return _app;
}

//! Shows the window's WinBorder
void ServerWindow::Show(void)
{
#ifdef DEBUG_SERVERWINDOW
printf("ServerWindow %s: Show\n",_title->String());
#endif
	if(_winborder)
	{
		_winborder->Show();
		_winborder->UpdateRegions(true);
	}
}

//! Hides the window's WinBorder
void ServerWindow::Hide(void)
{
#ifdef DEBUG_SERVERWINDOW
printf("ServerWindow %s: Hide\n",_title->String());
#endif
	if(_winborder)
		_winborder->Hide();
}

/*
	\brief Determines whether the window is hidden or not
	\return true if hidden, false if not
*/
bool ServerWindow::IsHidden(void)
{
	if(_winborder)
		return _winborder->IsHidden();
	return true;
}

/*!
	\brief Handles focus and redrawing when changing focus states
	
	The ServerWindow is set to (in)active and its decorator is redrawn based on its active status
*/
void ServerWindow::SetFocus(bool value)
{
#ifdef DEBUG_SERVERWINDOW
printf("ServerWindow %s: Set Focus to %s\n",_title->String(),value?"true":"false");
#endif
	if(_active!=value)
	{
		_active=value;
		_winborder->RequestDraw();
	}
}

/*!
	\brief Determines whether or not the window is active
	\return true if active, false if not
*/
bool ServerWindow::HasFocus(void)
{
	return _active;
}

/*!
	\brief Notifies window of workspace (de)activation
	\param workspace Index of the workspace changed
	\param active New active status of the workspace
*/
void ServerWindow::WorkspaceActivated(int32 workspace, bool active)
{
#ifdef DEBUG_SERVERWINDOW
printf("ServerWindow %s: WorkspaceActivated unimplemented\n",_title->String());
#endif
	// TODO: implement
}

/*!
	\brief Notifies window of a workspace switch
	\param oldone index of the previous workspace
	\param newone index of the new workspace
*/
void ServerWindow::WorkspacesChanged(int32 oldone,int32 newone)
{
#ifdef DEBUG_SERVERWINDOW
printf("ServerWindow %s: WorkspaceChanged unimplemented\n",_title->String());
#endif
	// TODO: implement
}

/*!
	\brief Notifies window of a change in focus
	\param active New active status of the window
*/
void ServerWindow::WindowActivated(bool active)
{
#ifdef DEBUG_SERVERWINDOW
printf("ServerWindow %s: WindowActivated unimplemented\n",_title->String());
#endif
	// TODO: implement
}

/*!
	\brief Notifies window of a change in screen resolution
	\param frame Size of the new resolution
	\param color_space Color space of the new screen mode
*/
void ServerWindow::ScreenModeChanged(const BRect frame, const color_space cspace)
{
#ifdef DEBUG_SERVERWINDOW
printf("ServerWindow %s: ScreenModeChanged unimplemented\n",_title->String());
#endif
	// TODO: implement
}

/*
	\brief Sets the frame size of the window
	\rect New window size
*/
void ServerWindow::SetFrame(const BRect &rect)
{
#ifdef DEBUG_SERVERWINDOW
printf("ServerWindow %s: Set Frame to (%.1f,%.1f,%.1f,%.1f)\n",_title->String(),
	rect.left,rect.top,rect.right,rect.bottom);
#endif
	_frame=rect;
}

/*!
	\brief Returns the frame of the window in screen coordinates
	\return The frame of the window in screen coordinates
*/
BRect ServerWindow::Frame(void)
{
	return _frame;
}

/*!
	\brief Locks the window
	\return B_OK if everything is ok, B_ERROR if something went wrong
*/
status_t ServerWindow::Lock(void)
{
#ifdef DEBUG_SERVERWINDOW
printf("ServerWindow %s: Lock\n",_title->String());
#endif
	return (_locker.Lock())?B_OK:B_ERROR;
}

//! Unlocks the window
void ServerWindow::Unlock(void)
{
#ifdef DEBUG_SERVERWINDOW
printf("ServerWindow %s: Unlock\n",_title->String());
#endif
	_locker.Unlock();
}

/*!
	\brief Determines whether or not the window is locked
	\return True if locked, false if not.
*/
bool ServerWindow::IsLocked(void)
{
	return _locker.IsLocked();
}

void ServerWindow::DispatchMessage(int32 code, int8 *msgbuffer)
{
	switch(code)
	{
		case AS_LAYER_CREATE:
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

			// TODO: Implement
#ifdef DEBUG_SERVERWINDOW
printf("ServerWindow %s: Message Create_Layer unimplemented\n",_title->String());
#endif

			break;
		}
		case AS_LAYER_DELETE:
		{
			// Received when a view is detached from a window. This is definitely
			// the less taxing operation - we call PruneTree() on the removed
			// layer, detach the layer itself, delete it, and invalidate the
			// area assuming that the view was visible when removed
			
			// Attached Data:
			// 1) (int32) id of the removed view

			// TODO: Implement
#ifdef DEBUG_SERVERWINDOW
printf("ServerWindow %s: Message Delete_Layer unimplemented\n",_title->String());
#endif

			break;
		}
		case AS_SHOW_WINDOW:
		{
			Show();
			break;
		}
		case AS_HIDE_WINDOW:
		{
			Hide();
			break;
		}
		case AS_SEND_BEHIND:
		{
			// TODO: Implement
#ifdef DEBUG_SERVERWINDOW
printf("ServerWindow %s: Message  Send_Behind unimplemented\n",_title->String());
#endif
			break;
		}
		case AS_ENABLE_UPDATES:
		{
			// TODO: Implement
#ifdef DEBUG_SERVERWINDOW
printf("ServerWindow %s: Message Enable_Updates unimplemented\n",_title->String());
#endif
			break;
		}
		case AS_DISABLE_UPDATES:
		{
			// TODO: Implement
#ifdef DEBUG_SERVERWINDOW
printf("ServerWindow %s: Message Disable_Updates unimplemented\n",_title->String());
#endif
			break;
		}
		case AS_NEEDS_UPDATE:
		{
			// TODO: Implement
#ifdef DEBUG_SERVERWINDOW
printf("ServerWindow %s: Message Needs_Update unimplemented\n",_title->String());
#endif
			break;
		}
		case AS_WINDOW_TITLE:
		{
			// TODO: Implement
#ifdef DEBUG_SERVERWINDOW
printf("ServerWindow %s: Message Set_Title unimplemented\n",_title->String());
#endif
			break;
		}
		case AS_ADD_TO_SUBSET:
		{
			// TODO: Implement
#ifdef DEBUG_SERVERWINDOW
printf("ServerWindow %s: Message Add_To_Subset unimplemented\n",_title->String());
#endif
			break;
		}
		case AS_REM_FROM_SUBSET:
		{
			// TODO: Implement
#ifdef DEBUG_SERVERWINDOW
printf("ServerWindow %s: Message Remove_From_Subset unimplemented\n",_title->String());
#endif
			break;
		}
		case AS_SET_LOOK:
		{
			// TODO: Implement
#ifdef DEBUG_SERVERWINDOW
printf("ServerWindow %s: Message Set_Look unimplemented\n",_title->String());
#endif
			break;
		}
		case AS_SET_FLAGS:
		{
			// TODO: Implement
#ifdef DEBUG_SERVERWINDOW
printf("ServerWindow %s: Message Set_Flags unimplemented\n",_title->String());
#endif
			break;
		}
		case AS_SET_FEEL:
		{
			// TODO: Implement
#ifdef DEBUG_SERVERWINDOW
printf("ServerWindow %s: Message Set_Feel unimplemented\n",_title->String());
#endif
			break;
		}
		case AS_SET_ALIGNMENT:
		{
			// TODO: Implement
#ifdef DEBUG_SERVERWINDOW
printf("ServerWindow %s: Message Set_Alignment unimplemented\n",_title->String());
#endif
			break;
		}
		case AS_GET_ALIGNMENT:
		{
			// TODO: Implement
#ifdef DEBUG_SERVERWINDOW
printf("ServerWindow %s: Message Get_Alignment unimplemented\n",_title->String());
#endif
			break;
		}
		case AS_GET_WORKSPACES:
		{
			// TODO: Implement
#ifdef DEBUG_SERVERWINDOW
printf("ServerWindow %s: Message Get_Workspaces unimplemented\n",_title->String());
#endif
			break;
		}
		case AS_SET_WORKSPACES:
		{
			// TODO: Implement
#ifdef DEBUG_SERVERWINDOW
printf("ServerWindow %s: Message Set_Workspaces unimplemented\n",_title->String());
#endif
			break;
		}
		case AS_WINDOW_RESIZEBY:
		{
			// TODO: Implement
#ifdef DEBUG_SERVERWINDOW
printf("ServerWindow %s: Message Resize_By unimplemented\n",_title->String());
#endif
			break;
		}
		case AS_WINDOW_RESIZETO:
		{
			// TODO: Implement
#ifdef DEBUG_SERVERWINDOW
printf("ServerWindow %s: Message Resize_To unimplemented\n",_title->String());
#endif
			break;
		}
		case B_MINIMIZE:
		{
			// TODO: Implement
#ifdef DEBUG_SERVERWINDOW
printf("ServerWindow %s: Message Minimize unimplemented\n",_title->String());
#endif
			break;
		}
		case B_WINDOW_ACTIVATED:
		{
			// TODO: Implement
#ifdef DEBUG_SERVERWINDOW
printf("ServerWindow %s: Message Window_Activated unimplemented\n",_title->String());
#endif
			break;
		}
		case B_ZOOM:
		{
			// TODO: Implement
#ifdef DEBUG_SERVERWINDOW
printf("ServerWindow %s: Message Zoom unimplemented\n",_title->String());
#endif
			break;
		}
		case B_WINDOW_MOVE_TO:
		{
			// TODO: Implement
#ifdef DEBUG_SERVERWINDOW
printf("ServerWindow %s: Message Move_To unimplemented\n",_title->String());
#endif
			break;
		}
		case B_WINDOW_MOVE_BY:
		{
			// TODO: Implement
#ifdef DEBUG_SERVERWINDOW
printf("ServerWindow %s: Message Move_By unimplemented\n",_title->String());
#endif
			break;
		}
		default:
		{
			printf("ServerWindow %s received unexpected code %lx",_title->String(),code);
			break;
		}
	}
}

/*!
	\brief Iterator for graphics update messages
	\param msgsize Size of the buffer containing the graphics messages
	\param msgbuffer Buffer containing the graphics message
*/
void ServerWindow::DispatchGraphicsMessage(int32 msgsize, int8 *msgbuffer)
{
	
}

/*!
	\brief Message-dispatching loop for the ServerWindow

	MonitorWin() watches the ServerWindow's message port and dispatches as necessary
	\param data The thread's ServerWindow
	\return Throwaway code. Always 0.
*/
int32 ServerWindow::MonitorWin(void *data)
{
	ServerWindow *win=(ServerWindow *)data;

	int32 msgcode;
	int8 *msgbuffer=NULL;
	ssize_t buffersize,bytesread;
	
	for(;;)
	{
		buffersize=port_buffer_size(win->_receiver);
		
		if(buffersize>0)
		{
			msgbuffer=new int8[buffersize];
			bytesread=read_port(win->_receiver,&msgcode,msgbuffer,buffersize);
		}
		else
			bytesread=read_port(win->_receiver,&msgcode,NULL,0);

		if (bytesread != B_BAD_PORT_ID && bytesread != B_TIMED_OUT && bytesread != B_WOULD_BLOCK)
		{
			switch(msgcode)
			{
				case B_QUIT_REQUESTED:
				{
#ifdef DEBUG_SERVERWINDOW
printf("ServerWindow %s received Quit request\n",win->Title());
#endif
					// Our BWindow sent us this message when it quit.
					// We need to ask its ServerApp to delete our monitor
					win->_applink->SetOpCode(AS_DELETE_WINDOW);
					win->_applink->Attach((int32)win->_token);
					win->_applink->Flush();
					break;
				}
				case AS_BEGIN_UPDATE:
				{
					win->DispatchGraphicsMessage(buffersize,msgbuffer);
					break;
				}
				default:
					win->DispatchMessage(msgcode,msgbuffer);
					break;
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
	\brief Passes mouse event messages to the appropriate window
	\param code Message code of the mouse message
	\param buffer Attachment buffer for the mouse message
*/
void ServerWindow::HandleMouseEvent(int32 code, int8 *buffer)
{
#ifdef DEBUG_SERVERWINDOW_MOUSE
printf("ServerWindow::HandleMouseEvent unimplemented\n");
#endif
	ServerWindow *mousewin=NULL;
	int8 *index=buffer;
	
	// Find the window which will receive our mouse event.
	Layer *root=GetRootLayer(CurrentWorkspace(),ActiveScreen());
	WinBorder *_winborder;
	
	// activeborder is used to remember windows when resizing/moving windows
	// or sliding a tab
	if(!root)
	{
		printf("ERROR: HandleMouseEvent has NULL root layer!!!\n");
		return;
	}

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
			float x=*((float*)index); index+=sizeof(float);
			float y=*((float*)index); index+=sizeof(float);
//			int32 modifiers=*((int32*)index); index+=sizeof(uint32);
//			uint32 buttons=*((uint32*)index); index+=sizeof(uint32);
//			int32 clicks=*((int32*)index);
			BPoint pt(x,y);

			// If we have clicked on a window, 			
			_winborder=(WinBorder*)root->GetChildAt(pt);
			if(_winborder)
			{
				mousewin=_winborder->Window();
				_winborder->MouseDown(buffer);
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
			float x=*((float*)index); index+=sizeof(float);
			float y=*((float*)index); index+=sizeof(float);
//			int32 modifiers=*((int32*)index);
			BPoint pt(x,y);
			
			set_is_moving_window(false);
			_winborder=(WinBorder*)root->GetChildAt(pt);
			if(_winborder)
			{
				mousewin=_winborder->Window();
				
				// Eventually, we will build in MouseUp messages with buttons specified
				// For now, we just "assume" no mouse specification with a 0.
				_winborder->MouseUp(buffer);
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
			float x=*((float*)index); index+=sizeof(float);
			float y=*((float*)index); index+=sizeof(float);
//			uint32 buttons=*((uint32*)index);
			BPoint pt(x,y);

			// TODO: Fix
/*			if(is_moving_window() || is_resizing_window() || is_sliding_tab())
			{
				mousewin=active_serverwindow;
				mousewin->_winborder->MouseMoved(pt,buttons,0);
			}
			else
			{
*/				_winborder=(WinBorder*)root->GetChildAt(pt);
				if(_winborder)
				{
					mousewin=_winborder->Window();
					_winborder->MouseMoved(buffer);
//				}
			}				
			break;
		}
		default:
		{
			break;
		}
	}
}

/*!
	\brief Passes key event messages to the appropriate window
	\param code Message code of the key message
	\param buffer Attachment buffer for the key message
*/
void ServerWindow::HandleKeyEvent(int32 code, int8 *buffer)
{
#ifdef DEBUG_SERVERWINDOW_KEYBOARD
printf("ServerWindow::HandleKeyEvent unimplemented\n");
#endif
/*	ServerWindow *keywin=NULL;
	int8 *index=buffer;
	
	// Dispatch the key event to the active window
*/
}

/*!
	\brief Returns the Workspace object to which the window belongs
	
	If the window belongs to all workspaces, it returns the current workspace
*/
Workspace *ServerWindow::GetWorkspace(void)
{
	if(_workspace_index==B_ALL_WORKSPACES)
		return _workspace->GetScreen()->GetActiveWorkspace();

	return _workspace;
}

/*!
	\brief Assign the window to a particular workspace object
	\param The ServerWindow's new workspace
*/
void ServerWindow::SetWorkspace(Workspace *wkspc)
{
#ifdef DEBUG_SERVERWINDOW
printf("ServerWindow %s: Set Workspace\n",_title->String());
#endif
	_workspace=wkspc;
}

/*!
	\brief Handles window activation stuff. Called by Desktop functions
*/
void ActivateWindow(ServerWindow *oldwin,ServerWindow *newwin)
{
#ifdef DEBUG_SERVERWINDOW
printf("ActivateWindow: old=%s, new=%s\n",oldwin?oldwin->Title():"NULL",newwin?newwin->Title():"NULL");
#endif
	if(oldwin==newwin)
		return;

	if(oldwin)
		oldwin->SetFocus(false);

	if(newwin)
		newwin->SetFocus(true);
}
