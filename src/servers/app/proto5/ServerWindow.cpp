/*
	ServerWindow.cpp
		Class which works with a BWindow. Handles all messages coming from and
		going to its window plus all drawing calls.
*/
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
#include "WindowBorder.h"
#include "DebugTools.h"

//#define DEBUG_SERVERWIN

// Used for providing identifiers for views
int32 view_counter=0;
bool movewin=false;

ServerWindow::ServerWindow(BRect rect, const char *string, uint32 winflags,
	ServerApp *winapp,  port_id winport, uint32 index)
{
	title=new BString;
	if(string)
		title->SetTo(string);
	else
		title->SetTo("Window");
#ifdef DEBUG_SERVERWIN
printf("ServerWindow() %s\n",title->String());
#endif

	// This must happen before the WindowBorder object - it needs this object's frame
	// to be valid
	frame=rect;

	winborder=new WindowBorder(this, title->String());
	
	// hard code this for now - window look also needs to be attached and sent to
	// server by BWindow constructor
	decorator=instantiate_decorator(winborder, winflags, B_TITLED_WINDOW_LOOK);
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
}

ServerWindow::~ServerWindow(void)
{
#ifdef DEBUG_SERVERWIN
printf("%s::~ServerWindow()\n",title->String());
#endif
	RemoveWindowFromDesktop(this);
	if(applink!=NULL)
		delete applink;
	delete title;
	delete winlink;
	delete decorator;
	delete winborder;
	kill_thread(thread);
}

void ServerWindow::RequestDraw(BRect rect)
{
	winlink->SetOpCode(LAYER_DRAW);
	winlink->Attach(&rect,sizeof(BRect));
	winlink->Flush();
}

void ServerWindow::ReplaceDecorator(void)
{
#ifdef DEBUG_SERVERWIN
printf("%s::ReplaceDecorator()\n",title->String());
#endif
}

void ServerWindow::Quit(void)
{
#ifdef DEBUG_SERVERWIN
printf("%s::Quit()\n",title->String());
#endif
}

const char *ServerWindow::GetTitle(void)
{
	return title->String();
}

ServerApp *ServerWindow::GetApp(void)
{
	return app;
}

void ServerWindow::Show(void)
{
#ifdef DEBUG_SERVERWIN
printf("%s::Show()\n",title->String());
#endif
	if(winborder)
	{
		winborder->ShowLayer();
		winborder->Draw(frame);
	}
}

void ServerWindow::Hide(void)
{
#ifdef DEBUG_SERVERWIN
printf("%s::Hide()\n",title->String());
#endif
	if(winborder)
		winborder->HideLayer();
}

bool ServerWindow::IsHidden(void)
{
	return (hidecount==0)?true:false;
}

void ServerWindow::SetFocus(bool value)
{
#ifdef DEBUG_SERVERWIN
printf("%s::SetFocus(%s)\n",title->String(),(value==true)?"true":"false");
#endif
	active=value;
}

bool ServerWindow::HasFocus(void)
{
	return active;
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

void ServerWindow::SetFrame(const BRect &rect)
{
#ifdef DEBUG_SERVERWIN
printf("%s::SetFrame()",title->String()); rect.PrintToStream();
#endif
	frame=rect;
}

BRect ServerWindow::Frame(void)
{
	return frame;
}

status_t ServerWindow::Lock(void)
{
#ifdef DEBUG_SERVERWIN
printf("%s::Lock()\n",title->String());
#endif
	return locker.Lock();
}

void ServerWindow::Unlock(void)
{
#ifdef DEBUG_SERVERWIN
printf("%s::Unlock()\n",title->String());
#endif
	locker.Unlock();
}

bool ServerWindow::IsLocked(void)
{
	return locker.IsLocked();
}

void ServerWindow::Loop(void)
{
	// Message-dispatching loop for the ServerWindow

#ifdef DEBUG_SERVERWIN
printf("%s::Loop()\n",title->String());
#endif
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
}

void ServerWindow::DispatchMessage(int32 code, int8 *msgbuffer)
{
	switch(code)
	{
		default:
		{
#ifdef DEBUG_SERVERWIN
printf("%s::ServerWindow(): Received unexpected code: ",title->String());
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
	ServerWindow *mousewin=NULL;
	int8 *index=buffer;
	
	// Find the window which will receive our mouse event.
	Layer *root=GetRootLayer();
	WindowBorder *winborder;
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
//			int32 modifiers=*((int32*)index);
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
				
				winborder->MouseDown(pt,buttons);
				
#ifdef DEBUG_SERVERWIN
printf("ServerWindow() %s: MouseDown(%.1f,%.1f)\n",mousewin->title->String(),x,y);
#endif
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
				winborder->MouseUp(pt,0);
				
				// Do cool mouse stuff here
				
#ifdef DEBUG_SERVERWIN
printf("ServerWindow() %s: MouseUp(%.1f,%.1f)\n",mousewin->title->String(),x,y);
#endif
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

			winborder=(WindowBorder*)root->GetChildAt(pt);
			if(activeborder)
				winborder=activeborder;
			if(winborder)
			{
				mousewin=winborder->Window();
				ASSERT(mousewin!=NULL);

				winborder->MouseMoved(pt,buttons);

				// Do cool mouse stuff here
				
#ifdef DEBUG_SERVERWIN
printf("ServerWindow() %s: MouseMoved(%.1f,%.1f)\n",mousewin->title->String(),x,y);
#endif
			}
			break;
		}
		default:
		{
#ifdef DEBUG_SERVERWIN
printf("HandleMouseEvent() received an unrecognized mouse event");
PrintMessageCode(code);
#endif
			break;
		}
	}
}
