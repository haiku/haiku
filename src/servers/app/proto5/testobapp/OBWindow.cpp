#include <Message.h>
#include <List.h>
#include <stdio.h>
#include "OBApplication.h"
#include "OBView.h"
#include "OBWindow.h"
#include "ServerProtocol.h"
#include "PortLink.h"

#define DEBUG_OBWINDOW

OBWindow::OBWindow(BRect frame,const char *title, window_type type,
uint32 flags,uint32 workspace = B_CURRENT_WORKSPACE)
{
}

OBWindow::OBWindow(BRect frame,const char *title, window_look look,window_feel feel,
uint32 flags,uint32 workspace = B_CURRENT_WORKSPACE)
{
#ifdef DEBUG_OBWINDOW
printf("OBWindow(%s)\n",title);
#endif
	Lock();

	// Window starts hidden by default
	hidelevel=1;

	wframe=frame;
	wlook=look;
	wfeel=feel;
	wflags=flags;
	wkspace=workspace;
	
	wtitle=new BString(title);
	
	// Receives messages from server and others
	inport=create_port(50,"msgport");
	if(inport==B_BAD_VALUE || inport==B_NO_MORE_PORTS)
		printf("OBWindow: Couldn't create message port\n");

	// Notify app that we exist
	OBWindow *win=this;
	PortLink *link=new PortLink(obe_app->messageport);
	link->SetOpCode(ADDWINDOW);
	link->Attach(&win,sizeof(OBWindow*));
	link->Flush();
	delete link;

	// we set this flag because the window starts locked. When Show() is
	// called it will unlock the looper and show the window
	startlocked=true;
	
	// We create our serverlink utilizing our application's server port
	// because the server's ServerApp will handle window creation
	serverlink=new PortLink(obe_app->serverport);

	// make sure that the server port is valid. It will == B_NAME_NOT_FOUND if the
	// OBApplication couldn't find the server
	
	if(obe_app->serverport!=B_NAME_NOT_FOUND)
	{
		// Notify server of window's existence
		serverlink->SetOpCode(CREATE_WINDOW);
		serverlink->Attach(wframe);
		serverlink->Attach((int32)wflags);
		serverlink->Attach(&inport,sizeof(port_id));
		serverlink->Attach((int32)wkspace);
		serverlink->Attach((char*)wtitle->String(),wtitle->Length());
	
		// Send and wait for ServerWindow port. Necessary here so we can respond to
		// messages as soon as Show() is called.
	
		int32 replycode;
		status_t replystat;
		ssize_t replysize;
		int8 *replybuffer;
	
		replybuffer=serverlink->FlushWithReply(&replycode,&replystat,&replysize);
		outport=*((port_id*)replybuffer);
		serverlink->SetPort(outport);
		delete replybuffer;

#ifdef DEBUG_OBWINDOW
printf("OBWindow(%s): ServerWindow Port %ld\n",wtitle->String(),outport);
#endif

		// Create and attach the top view
		top_view=new OBView(wframe.OffsetToCopy(0,0),"top_view",B_FOLLOW_ALL, B_WILL_DRAW);
		top_view->owner=this;
		top_view->topview=true;
	}
	else
		PostMessage(B_QUIT_REQUESTED);
}

OBWindow::~OBWindow()
{
#ifdef DEBUG_OBWINDOW
printf("~OBWindow(%s)\n",wtitle->String());
#endif
	// delete all children. How? OBView recursively deletes its children
	delete top_view;
	
	delete wtitle;
	delete serverlink;
	delete drawmsglink;

}

void OBWindow::Quit()
{
#ifdef DEBUG_OBWINDOW
printf("%s::Quit()\n",wtitle->String());
#endif
	// I hope this works. If it doesn't, we'll need to do a synchronous msg
	OBWindow *win=this;
	PortLink *link=new PortLink(obe_app->messageport);
	link->SetOpCode(REMOVEWINDOW);
	link->Attach(&win,sizeof(OBWindow *));
	link->Flush();
	delete link;

	// Server will need to be notified of window destruction	
	serverlink->SetOpCode(DELETE_WINDOW);
	serverlink->Attach(&ID,sizeof(int32));
	serverlink->Flush();

	// Quit our looper and delete our self :)
	Quit();
}

void OBWindow::Close()
{
	Quit();
}

void OBWindow::AddChild(OBView *child, OBView *before = NULL)
{
#ifdef DEBUG_OBWINDOW
printf("%s::AddChild()\n",wtitle->String());
#endif
	top_view->AddChild(child,before);
}

bool OBWindow::RemoveChild(OBView *child)
{
#ifdef DEBUG_OBWINDOW
printf("%s::RemoveChild()\n",wtitle->String());
#endif
	return top_view->RemoveChild(child);
}

int32 OBWindow::CountChildren() const
{
	return top_view->CountChildren();
}

OBView *OBWindow::ChildAt(int32 index) const
{
	return top_view->ChildAt(index);
}

void OBWindow::DispatchMessage(BMessage *message, BHandler *handler)
{
}

void OBWindow::MessageReceived(BMessage *message)
{
	// Not sure right now what needs to be handled.
	switch(message->what)
	{
		default:
			BLooper::MessageReceived(message);
	}
}

void OBWindow::FrameMoved(BPoint new_position)
{
}

void OBWindow::WorkspacesChanged(uint32 old_ws, uint32 new_ws)
{
}

void OBWindow::WorkspaceActivated(int32 ws, bool state)
{
}

void OBWindow::FrameResized(float new_width, float new_height)
{
}

void OBWindow::Minimize(bool minimize)
{
}

void OBWindow::Zoom(BPoint rec_position,float rec_width,float rec_height)
{
}

void OBWindow::Zoom()
{
}

void OBWindow::ScreenChanged(BRect screen_size, color_space depth)
{
}

bool OBWindow::NeedsUpdate() const
{
	return false;
}

void OBWindow::UpdateIfNeeded()
{
}

OBView *OBWindow::FindView(const char *view_name) const
{
	return top_view->FindView(view_name);
}

OBView *OBWindow::FindView(BPoint) const
{
	return NULL;
}

OBView *OBWindow::CurrentFocus() const
{
	return focusedview;
}

void OBWindow::Activate(bool=true)
{
}

void OBWindow::WindowActivated(bool state)
{
}

void OBWindow::ConvertToScreen(BPoint *pt) const
{
}

BPoint OBWindow::ConvertToScreen(BPoint pt) const
{
	return BPoint(-1,-1);
}

void OBWindow::ConvertFromScreen(BPoint *pt) const
{
}

BPoint OBWindow::ConvertFromScreen(BPoint pt) const
{
	return BPoint(-1,-1);
}

void OBWindow::ConvertToScreen(BRect *rect) const
{
}

BRect OBWindow::ConvertToScreen(BRect rect) const
{
	return BRect(0,0,0,0);
}

void OBWindow::ConvertFromScreen(BRect *rect) const
{
}

BRect OBWindow::ConvertFromScreen(BRect rect) const
{
	return BRect(0,0,0,0);
}

void OBWindow::MoveBy(float dx, float dy)
{
}

void OBWindow::MoveTo(BPoint)
{
}

void OBWindow::MoveTo(float x, float y)
{
}

void OBWindow::ResizeBy(float dx, float dy)
{
}

void OBWindow::ResizeTo(float width, float height)
{
}

void OBWindow::Show()
{
	if(startlocked)
	{
		startlocked=false;
		Unlock();
	}
	hidelevel--;
	if(hidelevel==0)
	{
		serverlink->SetOpCode(SHOW_WINDOW);
		serverlink->Flush();
	}
}

void OBWindow::Hide()
{
	if(hidelevel==0)
	{
		serverlink->SetOpCode(HIDE_WINDOW);
		serverlink->Flush();
	}
	hidelevel++;
}

bool OBWindow::IsHidden() const
{
	return false;
}

bool OBWindow::IsMinimized() const
{
	return false;
}

void OBWindow::Flush() const
{
	// When attachment count inclusion is implemented, it needs to go
	// in this function

	drawmsglink->SetOpCode(GFX_FLUSH);
	drawmsglink->Flush();
}

void OBWindow::Sync() const
{
	// When attachment count inclusion is implemented, it needs to go
	// in this function

	int32 code;
	status_t status;
	ssize_t buffersize;

	drawmsglink->SetOpCode(GFX_SYNC);

	// Reply received:

	// Code: SYNC
	// Attached data: none
	// Buffersize: 0
	drawmsglink->FlushWithReply(&code,&status,&buffersize);
}

status_t OBWindow::SendBehind(const OBWindow *window)
{
	return B_ERROR;
}

void OBWindow::DisableUpdates()
{
}

void OBWindow::EnableUpdates()
{
}

void OBWindow::BeginViewTransaction()
{
}

void OBWindow::EndViewTransaction()
{
}

BRect OBWindow::Bounds() const
{
	return BRect(0,0,0,0);
}

BRect OBWindow::Frame() const
{
	return BRect(0,0,0,0);
}

const char *OBWindow::Title() const
{
	return NULL;
}

void OBWindow::SetTitle(const char *title)
{
}

bool OBWindow::IsFront() const
{
	return false;
}

bool OBWindow::IsActive() const
{
	return false;
}

uint32 OBWindow::Workspaces() const
{
	return 0;
}

void OBWindow::SetWorkspaces(uint32)
{
}

OBView *OBWindow::LastMouseMovedView() const
{
	return NULL;
}

status_t OBWindow::AddToSubset(OBWindow *window)
{
	return B_ERROR;
}

status_t OBWindow::RemoveFromSubset(OBWindow *window)
{
	return B_ERROR;
}

bool OBWindow::QuitRequested()
{
	return false;
}

thread_id OBWindow::Run()
{
	return 0;
}

