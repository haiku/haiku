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

// TODO: include when we have WindowBorder.h
//#include "WindowBorder.h"
#include "Desktop.h"

// defined in WindowBorder.cpp locking not necessary - the only
// thread which accesses them is the Poller thread
extern bool is_moving_window,is_resizing_window,is_sliding_tab;

// Used in window focus management
ServerWindow *active_serverwindow=NULL;

ServerWindow::ServerWindow(BRect rect, const char *title, uint32 wlook, uint32 wfeel,
	uint32 wflags, int32 workspace, ServerApp *winapp,  port_id winport)
{
/*	title=new BString;
	if(string)
		title->SetTo(string);
	else
		title->SetTo("Window");

	// This must happen before the WindowBorder object - it needs this object's frame
	// to be valid
	frame=rect;

	// set these early also - the decorator will also need them
	winflags=wflags;
	winlook=wlook;
	winfeel=wfeel;
	

	winborder=new WindowBorder(this, title->String());

	// hard code this for now - window look also needs to be attached and sent to
	// server by BWindow constructor
	decorator=instantiate_decorator(frame,title->String(),winlook,winfeel,winflags,get_gfxdriver());

	winborder->SetDecorator(decorator);

	// sender is the monitored app's event port
	sender=winport;
	winlink=new PortLink(sender);

	if(winapp!=NULL)
		applink=new PortLink(winapp->receiver);
	else
		applink=NULL;
	
	// receiver is the port to which the app sends messages for the server
	receiver=create_port(30,title->String());
	
	active=false;
	hidecount=0;

	// Spawn our message monitoring thread
	thread=spawn_thread(MonitorWin,title->String(),B_NORMAL_PRIORITY,this);
	if(thread!=B_NO_MORE_THREADS && thread!=B_NO_MEMORY)
		resume_thread(thread);

	workspace=index;
	AddWindowToDesktop(this,index);
*/
}

ServerWindow::~ServerWindow(void)
{
/*	RemoveWindowFromDesktop(this);
	if(applink)
	{
		delete applink;
		applink=NULL;

		delete title;
		delete winlink;
		delete decorator;
		delete winborder;
	}
	kill_thread(thread);
*/
}

void ServerWindow::RequestDraw(BRect rect)
{
/*	winlink->SetOpCode(LAYER_DRAW);
	winlink->Attach(&rect,sizeof(BRect));
	winlink->Flush();
*/
}

void ServerWindow::Quit(void)
{
}

void ServerWindow::Show(void)
{
/*	if(winborder)
	{
		winborder->ShowLayer();
		ActivateWindow(this);
	}
*/
}

void ServerWindow::Hide(void)
{
//	if(winborder)
//		winborder->HideLayer();
}

void ServerWindow::SetFocus(bool value)
{
/*	if(active!=value)
	{
		active=value;
		decorator->SetFocus(value);
		decorator->Draw();
	}
*/
}

void ServerWindow::WorkspaceActivated(int32 NewWorkspace, const BPoint Resolution, color_space CSpace)
{
}

void ServerWindow::WindowActivated(bool Active)
{
}

void ServerWindow::ScreenModeChanged(const BPoint Resolustion, color_space CSpace)
{
}

void ServerWindow::DispatchMessage(int32 code, int8 *msgbuffer)
{
	switch(code)
	{
		default:
		{
			break;
		}
	}
}

int32 ServerWindow::MonitorWin(void *data)
{
/*	ServerWindow *win=(ServerWindow *)data;
	// Message-dispatching loop for the ServerWindow
	int32 msgcode;
	int8 *msgbuffer=NULL;
	ssize_t buffersize,bytesread;
	
	for(;;)
	{
		buffersize=port_buffer_size(receiver);
		
		if(buffersize>0)
		{
			msgbuffer=new int8[buffersize];
			bytesread=read_port(receiver,&msgcode,msgbuffer,buffersize);
		}
		else
			bytesread=read_port(receiver,&msgcode,NULL,0);

		if (bytesread != B_BAD_PORT_ID && bytesread != B_TIMED_OUT && bytesread != B_WOULD_BLOCK)
		{
			switch(msgcode)
			{
				case LAYER_CREATE:
				{
					// Received when a view is attached to a window. This will require
					// us to attach a layer in the tree in the same manner and invalidate
					// the area in which the new layer resides assuming that it is
					// visible
				
					// Attached Data:
					// 1) (int32) id of the parent view
					// 2) (int32) id of the child view
					// 3) (BRect) frame in parent's coordinates
					// 4) (int32) resize flags
					// 5) (int32) view flags
					// 6) (uint16) view's hide level

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
					applink->SetOpCode(DELETE_WINDOW);
					applink->Attach((int32)thread);
					applink->Flush();
					break;
				}
				case VIEW_GET_TOKEN:
				{
					// Request to get a view identifier - this is synchronous, so reply
					// immediately
					
					// Attached data:
					// 1) port_id reply port
					port_id reply_port=*((port_id*)msgbuffer);
					PortLink *link=new PortLink(reply_port);
					link->Attach(view_counter++);
					link->Flush();
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
*/
	return 0;
}

void ServerWindow::HandleMouseEvent(int32 code, int8 *buffer)
{
/*	ServerWindow *mousewin=NULL;
	int8 *index=buffer;
	
	// Find the window which will receive our mouse event.
	Layer *root=GetRootLayer();
	WindowBorder *winborder;
	
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
			winborder=(WindowBorder*)root->GetChildAt(pt);
			if(winborder)
			{
				mousewin=winborder->Window();
				ASSERT(mousewin!=NULL);
				winborder->MouseDown(pt,buttons,modifiers);
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
			
			is_moving_window=false;
			winborder=(WindowBorder*)root->GetChildAt(pt);
			if(winborder)
			{
				mousewin=winborder->Window();
				ASSERT(mousewin!=NULL);
				
				// Eventually, we will build in MouseUp messages with buttons specified
				// For now, we just "assume" no mouse specification with a 0.
				winborder->MouseUp(pt,0,0);
				
				// Do cool mouse stuff here
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

			if(is_moving_window || is_resizing_window || is_sliding_tab)
			{
				mousewin=active_serverwindow;
				ASSERT(mousewin!=NULL);

				mousewin->winborder->MouseMoved(pt,buttons,0);
			}
			else
			{
				winborder=(WindowBorder*)root->GetChildAt(pt);
				if(winborder)
				{
					mousewin=winborder->Window();
					ASSERT(mousewin!=NULL);
	
					winborder->MouseMoved(pt,buttons,0);
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

void ActivateWindow(ServerWindow *win)
{
/*	if(active_serverwindow==win)
		return;
	if(active_serverwindow)
		active_serverwindow->SetFocus(false);
	active_serverwindow=win;
	if(win)
		win->SetFocus(true);
*/
}
