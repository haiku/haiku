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
//	File Name:		Window.cpp
//	Author:			John Hedditch (jhedditc@physics.adelaide.edu.au)
//					DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	BWindow is the base class for all windows (graphic areas
//					displayed on-screen).
//	Needs serverlink and msglink PortLink variables declared, but not sure whether
//	I'm allowed to do this in Window.h....
//------------------------------------------------------------------------------
//
// System Includes --------------------------------------------------------------
#include <Application.h>
#include <Window.h>
#include <Button.h>
#include <Point.h>
#include <Message.h>
#include <View.h>
#include <stdio.h>
// Project Includes -----------------------------------------------------------
#include <ServerProtocol.h>
#include <PropertyInfo.h> // Need this for GetSupportedSuites
#include <PortLink.h>
// Local Includes ---------------------------------------------------------------
/**********************************************
 * 	Public BWindow
 **********************************************/
//--------------------------------------------------------------------------------
BWindow::BWindow(BRect frame,
                 const char* title, 
                 window_type type,
                 uint32 flags,
                 uint32 workspace = B_CURRENT_WORKSPACE)
{
	Lock();
	fFrame = frame;
	SetTitle(title);
	decompose_type(type,&fLook,&fFeel);
	SetFlags(flags);
	SetWorkspaces(workspace);

	// MESSAGING STUFF - Adapted from DarkWyrms proto6 code
	// Names changed to match <Window.h>
	// Create a port on which to listen to messages
	receive_port=create_port(50,"msgport");
        if(receive_port==B_BAD_VALUE || receive_port==B_NO_MORE_PORTS)
                printf("BWindow: Couldn't create message port\n");

	// Notify app that we exist
	BWindow *win=this;
	serverlink=new PortLink(be_app->fServerFrom);
	serverlink->SetOpCode(AS_CREATE_WINDOW);
	serverlink->Attach(&win,sizeof(BWindow*));
	serverlink->Flush();
	
	 fUnused1 = true;	// Haven't been Show()n yet. Show() must UnLock().
	 			// Knew I'd find a use for fUnused1 :)
	 
	 fShowLevel = 0;	// Hidden to start with.

	// We create our serverlink utilizing our application's server port
        // because the server's ServerApp will handle window creation
	 serverlink->SetPort(be_app->fServerTo);

	 // make sure that the server port is valid. It will == B_NAME_NOT_FOUND if the
	 // OBApplication couldn't find the server
	if(be_app->fServerTo!=B_NAME_NOT_FOUND)
	 {
	   // Notify server of window's existence
	   serverlink->SetOpCode(AS_CREATE_WINDOW);
	   serverlink->Attach(fFrame);
	   serverlink->Attach((int32)fLook);
	   serverlink->Attach((int32)fFeel);
	   serverlink->Attach((int32)flags);
	   serverlink->Attach(&receive_port,sizeof(port_id));
	   serverlink->Attach((int32)workspace);
	   serverlink->Attach(fTitle,sizeof(fTitle));
	   
         // Send and wait for ServerWindow port. Necessary here so we can respond to
	 // messages as soon as Show() is called.
	 
	   int32 replycode;
	   status_t replystat;
	   ssize_t replysize;
	   int8 *replybuffer;
	   replybuffer=serverlink->FlushWithReply(&replycode,&replystat,&replysize);
	   send_port=*((port_id*)replybuffer);
	   serverlink->SetPort(send_port);
	   delete replybuffer;
	   serverlink->Flush();
      
	   top_view=new BView(frame.OffsetToCopy(0,0),"top_view",B_FOLLOW_ALL, B_WILL_DRAW);
	   top_view->owner=this;
	   top_view->top_level_view=true;
	   fFocus = top_view;		// top view gets the focus by default
      
	 }
	 else
	 {
		//	Not sure what to do here
	 }
      
}
//--------------------------------------------------------------------------------
BWindow::BWindow(BRect frame,
                 const char* title,                                            
                 window_look look,
		 window_feel feel,
                 uint32 flags,
                 uint32 workspace = B_CURRENT_WORKSPACE)
{
	Lock();
	fFrame = frame;
	SetTitle(title);
	SetLook(look);
	SetFeel(feel);
	SetFlags(flags);
	SetWorkspaces(workspace);

	// MESSAGING STUFF - Adapted from DarkWyrms proto6 code
	// Names changed to match <Window.h>
	// Create a port on which to listen to messages
	receive_port=create_port(50,"msgport");
        if(receive_port==B_BAD_VALUE || receive_port==B_NO_MORE_PORTS)
                printf("BWindow: Couldn't create message port\n");

	// Notify app that we exist
	BWindow *win=this;
	serverlink=new PortLink(be_app->fServerFrom);
	serverlink->SetOpCode(AS_CREATE_WINDOW);
	serverlink->Attach(&win,sizeof(BWindow*));
	serverlink->Flush();
	
	 fUnused1 = true;	// Haven't been Show()n yet. Show() must UnLock().
	 			// Knew I'd find a use for fUnused1 :)
	 
	 fShowLevel = 0;	// Hidden to start with.

	// We create our serverlink utilizing our application's server port
        // because the server's ServerApp will handle window creation
	 serverlink->SetPort(be_app->fServerTo);

	 // make sure that the server port is valid. It will == B_NAME_NOT_FOUND if the
	 // OBApplication couldn't find the server
     if(be_app->fServerTo!=B_NAME_NOT_FOUND)
	 {
	   // Notify server of window's existence
	   serverlink->SetOpCode(AS_CREATE_WINDOW);
	   serverlink->Attach(fFrame);
	   serverlink->Attach((int32)fLook);
	   serverlink->Attach((int32)fFeel);
	   serverlink->Attach((int32)flags);
	   serverlink->Attach(&receive_port,sizeof(port_id));
	   serverlink->Attach((int32)workspace);
	   serverlink->Attach(fTitle,sizeof(fTitle));
	   
         // Send and wait for ServerWindow port. Necessary here so we can respond to
	 // messages as soon as Show() is called.
	 
	   int32 replycode;
	   status_t replystat;
	   ssize_t replysize;
	   int8 *replybuffer;
	   replybuffer=serverlink->FlushWithReply(&replycode,&replystat,&replysize);
	   send_port=*((port_id*)replybuffer);
	   serverlink->SetPort(send_port);
	   delete replybuffer;
	   serverlink->Flush();
      
	   top_view=new BView(frame.OffsetToCopy(0,0),"top_view",B_FOLLOW_ALL, B_WILL_DRAW);
	   top_view->owner=this;
	   top_view->top_level_view=true;
	   fFocus = top_view;		// top view gets the focus by default
      
	 }
	 else
	 {
		//	Not sure what to do here
	 }
      
}       
//--------------------------------------------------------------------------------
BWindow::BWindow(BMessage* data)
{
}
//--------------------------------------------------------------------------------
/*  Virtual Destructor */
BWindow::~BWindow()
{
	Quit();
}
//--------------------------------------------------------------------------------
/* Virtual */
void
BWindow::Quit()
{
    
	// Straight from DarkWyrm
	// I hope this works. If it doesn't, we'll need to do a synchronous msg 
	BWindow *win=this;
	PortLink *link=new PortLink(be_app->fServerFrom);
	link->SetOpCode(AS_DELETE_WINDOW);
	link->Attach(&win,sizeof(BWindow *));
	link->Flush();
	delete link;
	// Server will need to be notified of window destruction
	serverlink->SetOpCode(AS_DELETE_WINDOW);
	// serverlink->Attach(&ID,sizeof(int32));   XXX What is ID???
	/* MAYBE */ 
	serverlink->Attach(&win,sizeof(BWindow *));
	serverlink->Flush();
	delete serverlink;

	// Quit our looper and delete our self :)
	Quit();

}
//--------------------------------------------------------------------------------
/* Static */
BArchivable*
BWindow::Instantiate(BMessage* data)
{
	if ( validate_instantiation(data, "BWindow"))
	{
                return new BWindow(data);
	}
        else
                return NULL;
}
//--------------------------------------------------------------------------------
/* Virtual */
status_t
BWindow::Archive(BMessage* data, bool deep = true) const
{
}
//--------------------------------------------------------------------------------
void
BWindow::AddChild(BView* child, BView* before = NULL)
{
	top_view->AddChild(child,before);
}
//--------------------------------------------------------------------------------
bool
BWindow::RemoveChild(BView* child)
{
	return top_view->RemoveChild(child);
}
//--------------------------------------------------------------------------------
int32
BWindow::CountChildren() const
{
	return top_view->CountChildren();
}
//--------------------------------------------------------------------------------
BView*
BWindow::ChildAt(int32 index) const
{
	return top_view->ChildAt(index);
}
//--------------------------------------------------------------------------------
/* Virtual */
void
BWindow::DispatchMessage(BMessage* message, BHandler* handler)
{
	
}
//--------------------------------------------------------------------------------
/* Virtual */
void
BWindow::MessageReceived(BMessage* message)
{
	/*	PARTIAL IMPLEMENTATION ONLY 	*/
	/*	Need to learn how to respond to GET messages */

	switch(message->what)
	{
		case B_WINDOW_MOVE_BY :
		{
			// Move the Window
		}

		case B_WINDOW_MOVE_TO :
		{
			// Move the Window
		}

		case B_QUIT_REQUESTED :
		{
		     // Send B_QUIT_REQUESTED to app_server
		}

		case B_SET_PROPERTY :
		{   
			// TODO etc
		}

		default :
		{
			BHandler::MessageReceived(message);
			break;
		}

	}
}
//--------------------------------------------------------------------------------
/* Virtual */
void
BWindow::FrameMoved(BPoint new_position)
{
	
}
//--------------------------------------------------------------------------------
/* Virtual */
void
BWindow::WorkspacesChanged(uint32 old_ws, uint32 new_ws)
{
	
}
//--------------------------------------------------------------------------------
/* Virtual */
void
BWindow::WorkspaceActivated(int32 ws, bool state)
{
	
}
//--------------------------------------------------------------------------------
/* Virtual */
void
BWindow::FrameResized(float new_width, float new_height)
{
	
}
//--------------------------------------------------------------------------------
/* Virtual */
void
BWindow::Minimize(bool minimize)
{
        /*
        serverlink->SetOpCode(MINIMIZE_WINDOW);
        serverlink->Flush();
        */
}
//--------------------------------------------------------------------------------
/* Virtual */
void
BWindow::Zoom(BPoint rec_position,
     float rec_width,
     float rec_height)
{
	
}
//--------------------------------------------------------------------------------
void
BWindow::Zoom()
{
	
}
//--------------------------------------------------------------------------------
void
BWindow::SetZoomLimits(float max_h, float max_v)
{
	
}
//--------------------------------------------------------------------------------
/* Virtual */
void
BWindow::ScreenChanged(BRect screen_size, color_space depth)
{
	
}
//--------------------------------------------------------------------------------
void
BWindow::SetPulseRate(bigtime_t rate)
{
	pulse_rate = rate;
}
//--------------------------------------------------------------------------------
bigtime_t
BWindow::PulseRate() const
{
	return pulse_rate;
}
//--------------------------------------------------------------------------------
void
AddShortcut(uint32 key,
	    uint32 modifiers,
	    BMessage* msg)
{
	
}
//--------------------------------------------------------------------------------
void
AddShortcut(uint32 key,
	    uint32 modifiers,
	    BMessage* msg,
	    BHandler* target)
{
	        
}
//--------------------------------------------------------------------------------
void
BWindow::RemoveShortcut(uint32 key, uint32 modifiers)
{
	
}
//--------------------------------------------------------------------------------
void
BWindow::SetDefaultButton(BButton* button)
{
	fDefaultButton = button;  /* Need app_server notification of this */
}
//--------------------------------------------------------------------------------
BButton*
BWindow::DefaultButton() const
{
	return fDefaultButton;
}
//--------------------------------------------------------------------------------
/* Virtual */
void
BWindow::MenusBeginning()
{
	
}
//--------------------------------------------------------------------------------
/* Virtual */
void
BWindow::MenusEnded()
{
	
}
//--------------------------------------------------------------------------------
bool
BWindow::NeedsUpdate() const
{
	
	return false;
}
//--------------------------------------------------------------------------------
void
BWindow::UpdateIfNeeded()
{
	
}
//--------------------------------------------------------------------------------
BView*
BWindow::FindView(const char* view_name) const
{
   	
	BView	*RandomView;
	return  RandomView;
}
//--------------------------------------------------------------------------------
BView*
BWindow::FindView(BPoint) const
{
}
//--------------------------------------------------------------------------------
BView*
BWindow::CurrentFocus() const
{
	return  fFocus;
}
//--------------------------------------------------------------------------------
void
BWindow::Activate(bool = true)
{
	
}
//--------------------------------------------------------------------------------
/* Virtual */
void
BWindow::WindowActivated(bool state)
{
	
}
//--------------------------------------------------------------------------------
void
BWindow::ConvertToScreen(BPoint* pt) const
{
	
}
//--------------------------------------------------------------------------------
BPoint
BWindow::ConvertToScreen(BPoint pt) const
{
	
	BPoint	RandomPoint;
	return RandomPoint;
}
//--------------------------------------------------------------------------------
void
BWindow::ConvertFromScreen(BPoint* pt) const
{
	
}
//--------------------------------------------------------------------------------
BPoint
BWindow::ConvertFromScreen(BPoint pt) const
{
	
	BPoint	RandomPoint;
	return RandomPoint;
}
//--------------------------------------------------------------------------------
void
BWindow::ConvertToScreen(BRect* rect) const
{
	
}
//--------------------------------------------------------------------------------
BRect
BWindow::ConvertToScreen(BRect rect) const
{
	
	BRect	RandomRect;
	return RandomRect;
}
//--------------------------------------------------------------------------------
void
BWindow::ConvertFromScreen(BRect* rect) const
{
	
}
//--------------------------------------------------------------------------------
BRect
BWindow::ConvertFromScreen(BRect rect) const
{
	
	BRect	RandomRect;
	return RandomRect;
}
//--------------------------------------------------------------------------------
void
BWindow::MoveBy(float dx, float dy) 
{
}
//--------------------------------------------------------------------------------
void
BWindow::MoveTo(BPoint) 
{
}
//--------------------------------------------------------------------------------
void
BWindow::MoveTo(float x, float y) 
{
}
//--------------------------------------------------------------------------------
void
BWindow::ResizeBy(float dx, float dy) 
{
	
}
//--------------------------------------------------------------------------------
void
BWindow::ResizeTo(float width, float height) 
{
	
}
//--------------------------------------------------------------------------------
/* Virtual */
void
BWindow::Show()
{
	if(fUnused1)	// This is the first Show() for this window, so Unlock.
	{
		fUnused1 = false;
		Unlock();
	}
	fShowLevel++;		
	if(fShowLevel>0)	// Transition between hidden and unhidden
	{
		// Notify App_server that it should show the window
		serverlink->SetOpCode(AS_SHOW_WINDOW);
		serverlink->Flush();
	}
}
//--------------------------------------------------------------------------------
/* Virtual */
void
BWindow::Hide()
{
	fShowLevel--;	
	if(fShowLevel==0)	// Transition between unhidden and hidden
	{
		// Notify App_server that it should hide this window
		serverlink->SetOpCode(AS_HIDE_WINDOW);
		serverlink->Flush();
	}
}
//--------------------------------------------------------------------------------
bool
BWindow::IsHidden() const
{
	return(fShowLevel<=0);
}
//--------------------------------------------------------------------------------
bool
BWindow::IsMinimized() const
{
	
}
//--------------------------------------------------------------------------------
void
BWindow::Flush() const
{
	
}
//--------------------------------------------------------------------------------
void
BWindow::Sync() const
{    
	// When attachment count inclusion is implemented, it needs to go
	// in this function

        int32 code;
        status_t status;
        ssize_t buffersize;

    PortLink *drawmsglink=new PortLink(be_app->fServerFrom);
        drawmsglink->SetOpCode(AS_SYNC);

        // Reply received:
        // Code: SYNC
	// Attached data: none
	// Buffersize: 0
	drawmsglink->FlushWithReply(&code,&status,&buffersize);
	//
	delete drawmsglink;
}
//--------------------------------------------------------------------------------
status_t
BWindow::SendBehind(const BWindow* window)
{
	
	return B_ERROR;
}
//--------------------------------------------------------------------------------
void
BWindow::DisableUpdates()
{
	
}
//--------------------------------------------------------------------------------
void
BWindow::EnableUpdates()
{
	
}
//--------------------------------------------------------------------------------
void
BWindow::BeginViewTransaction()
{
	
}
//--------------------------------------------------------------------------------
void
BWindow::EndViewTransaction()
{
	
}
//--------------------------------------------------------------------------------
BRect
BWindow::Bounds() const
{
	
	BRect	BoundingRectangle;
	return  BoundingRectangle;
}
//--------------------------------------------------------------------------------
BRect
BWindow::Frame() const
{
	
	BRect	BoundingRectangle;
	return  BoundingRectangle;
}
//--------------------------------------------------------------------------------
const char*
BWindow::Title() const
{
	return  fTitle;
}
//--------------------------------------------------------------------------------
void
BWindow::SetTitle(const char* title) 
{
		// need to notify app_server
	SetLocalTitle(title);
}
//--------------------------------------------------------------------------------
bool
BWindow::IsFront() const
{
	
	return false;
}
//--------------------------------------------------------------------------------
bool
BWindow::IsActive() const
{
	
	return false;
}
//--------------------------------------------------------------------------------
void
BWindow::SetKeyMenuBar(BMenuBar* bar)
{
		// need to notify app_server
	fKeyMenuBar = bar;
}
//--------------------------------------------------------------------------------
BMenuBar*
BWindow::KeyMenuBar() const
{
	return fKeyMenuBar;
}
//--------------------------------------------------------------------------------
void
BWindow::SetSizeLimits(float min_h,
		       float max_h,
		       float min_v,
		       float max_v)
{
		// need to notify app_server
	fMinWindH	= min_h;
	fMinWindV	= min_v;
	fMaxWindH	= max_h;
	fMaxWindV	= max_v;
}
//--------------------------------------------------------------------------------
void
BWindow::GetSizeLimits(float* min_h,
		       float* max_h,
		       float* min_v,
		       float* max_v)
{
	*min_h = fMinWindH;
	*min_v = fMinWindV;
	*max_h = fMaxWindH;
	*max_v = fMaxWindV;
}
//--------------------------------------------------------------------------------
uint32
BWindow::Workspaces() const
{
	
	return 0;
}
//--------------------------------------------------------------------------------
void
BWindow::SetWorkspaces(uint32) 
{
	
}
//--------------------------------------------------------------------------------
BView*
BWindow::LastMouseMovedView() const
{
	return  fLastMouseMovedView;
}
//--------------------------------------------------------------------------------
/* Virtual */
BHandler*
BWindow::ResolveSpecifier(BMessage* msg,
			   int32 index,
			   BMessage* specifier,
			   int32 form,
			   const char* property)
{
	
	BHandler	*RandomBHandler;
	return RandomBHandler;
}
//--------------------------------------------------------------------------------
/* Virtual */
status_t
BWindow::GetSupportedSuites(BMessage* data) 
{
      
      static property_info prop_list[] = {
      { "Feel", {B_GET_PROPERTY, B_SET_PROPERTY, 0},{B_DIRECT_SPECIFIER, 0}, "Get or Set the Feel"},
      { "Flags", {B_GET_PROPERTY, B_SET_PROPERTY,0}, {B_DIRECT_SPECIFIER, 0}, "Get or Set Flags"},
      { "Frame", {B_GET_PROPERTY, B_SET_PROPERTY, 0}, {B_DIRECT_SPECIFIER, 0}, "Get or Set Frame"},
      { "Hidden", {B_GET_PROPERTY, B_SET_PROPERTY, 0}, {B_DIRECT_SPECIFIER, 0}, "Get or Set Frame"},
      { "Look", {B_GET_PROPERTY, B_SET_PROPERTY, 0}, {B_DIRECT_SPECIFIER, 0}, "Get or Set Look"},
      { "MenuBar", {0}, {B_DIRECT_SPECIFIER, 0}, "Direct Message to Key Menu Bar"},
      { "Minimize", {B_SET_PROPERTY, 0}, {B_DIRECT_SPECIFIER, 0}, "Minim. if data true else restore"},
      { "Title", {B_GET_PROPERTY, B_SET_PROPERTY, 0}, {B_DIRECT_SPECIFIER, 0}, "Get or Set Title"},
      { "View" , {0}, {0}, "Direct message to top View, don't pop specifier" },
      { "Workspaces", {B_GET_PROPERTY, B_SET_PROPERTY, 0}, {B_DIRECT_SPECIFIER, 0}, "Get or Set WS"},
      0 }; 


      data->AddString("suites", "suite/vnd.Be-window");
      BPropertyInfo prop_info(prop_list);
      data->AddFlat("messages", &prop_info);
      return BHandler::GetSupportedSuites(data); 
}
//--------------------------------------------------------------------------------
/* Virtual */
status_t
BWindow::Perform(perform_code d, void* arg) 
{
	
 	return B_ERROR;	
}
//--------------------------------------------------------------------------------
status_t
BWindow::AddToSubset(BWindow* window) 
{
	
 	return B_ERROR;	
}
//--------------------------------------------------------------------------------
status_t
BWindow::RemoveFromSubset(BWindow* window) 
{
	
 	return B_ERROR;	
}
//--------------------------------------------------------------------------------
status_t
BWindow::SetType(window_type type) 
{
	 	// need to notify app_server
	return B_ERROR;
}
//--------------------------------------------------------------------------------
window_type
BWindow::Type() const
{
	 	// need to notify app_server
	window_type RandomWindowType;
	return RandomWindowType;
}
//--------------------------------------------------------------------------------
status_t
BWindow::SetLook(window_look look)
{
	 	// need to notify app_server
	fLook = look;
	return B_ERROR;
}
//--------------------------------------------------------------------------------
window_look
BWindow::Look() const
{
	return fLook;
}
//--------------------------------------------------------------------------------
status_t
BWindow::SetFeel(window_feel feel)
{
	 	// need to notify app_server
	fFeel = feel;
	return B_ERROR;
}
//--------------------------------------------------------------------------------
window_feel
BWindow::Feel() const
{
	return fFeel;
}
//--------------------------------------------------------------------------------
status_t
BWindow::SetFlags(uint32 flags)
{
	fFlags = flags;
	return B_OK;
}
//--------------------------------------------------------------------------------
uint32
BWindow::Flags() const
{
	return fFlags;
}
//--------------------------------------------------------------------------------
bool
BWindow::IsModal() const
{
	if(fFeel == B_MODAL_APP_WINDOW_FEEL)
		return true;
	else
		return false;
}
//--------------------------------------------------------------------------------
bool
BWindow::IsFloating() const
{
	if(fFeel == B_FLOATING_APP_WINDOW_FEEL) 
		return true;
	else
		return false;

}

//--------------------------------------------------------------------------------
status_t
BWindow::SetWindowAlignment(window_alignment mode,
		int32 h,
		int32 hOffset = 0,
		int32 width = 0,
		int32 widthOffset = 0,
		int32 v = 0,
		int32 vOffset = 0,
		int32 height = 0,
		int32 heightOffset = 0)
{
	
	return B_ERROR;
}
//--------------------------------------------------------------------------------
status_t
BWindow::GetWindowAlignment(window_alignment* mode = 0,
		int32* h =0 ,
		int32* hOffset = 0,
		int32* width = 0,
		int32* widthOffset = 0,
		int32* v = 0,
		int32* vOffset = 0,
		int32* height = 0,
		int32* heightOffset = 0) const
{
	
	return B_ERROR;
}
//---------------------------------------------------------------------------------
/* Virtual */
bool
BWindow::QuitRequested()
{
	
	return false;
}
//---------------------------------------------------------------------------------
/* Virtual */
thread_id
BWindow::Run()
{
	
	thread_id	RandomThreadId;
	return RandomThreadId;
}

/******************************************************
 *	Private BWindow
 ******************************************************/

//---------------------------------------------------------------------------------
BWindow::BWindow()
{
	
}
//---------------------------------------------------------------------------------
BWindow::BWindow(BWindow&)
{
	
}
//---------------------------------------------------------------------------------
BWindow::BWindow(BRect frame, color_space depth, uint32 bitmapFlags, int32 rowBytes)
{
	
}
//------------------------------------------------------------------------------
BWindow&
BWindow::operator=(BWindow&)
{
        return *this;
}
//---------------------------------------------------------------------------------
void 
BWindow::InitData(BRect frame,
		  const char* title,
		  window_look look,
		  window_feel feel,
		  uint32 flags,
		  uint32 workspace)
{
	
}
//---------------------------------------------------------------------------------
status_t	
BWindow::ArchiveChildren(BMessage* data, bool deep) const
{
	
	return B_ERROR;
}
//---------------------------------------------------------------------------------
status_t	
BWindow::UnarchiveChildren(BMessage* data)
{
	
	return B_ERROR;
}
//---------------------------------------------------------------------------------
void	
BWindow::BitmapClose()
{
	
}
//---------------------------------------------------------------------------------
/* Virtual */
void	
BWindow::task_looper()
{
	
}
//---------------------------------------------------------------------------------
void	
BWindow::start_drag(BMessage* msg,
		    int32 token,
		    BPoint offset,
		    BRect track_rect,
		    BHandler* reply_to)
{
	
}
//---------------------------------------------------------------------------------
void	
BWindow::start_drag(BMessage* msg,
		    int32 token,
		    BPoint offset,
		    int32 bitmap_token,
		    drawing_mode dragMode,
		    BHandler* reply_to)
{
	
}
//---------------------------------------------------------------------------------
void	
BWindow::view_builder(BView* a_view)
{
	
}
//---------------------------------------------------------------------------------
void	
BWindow::attach_builder(BView* a_view)
{
	
}
//---------------------------------------------------------------------------------
void	
BWindow::detach_builder(BView* a_view)
{
	
}
//---------------------------------------------------------------------------------
int32	
BWindow::get_server_token() const
{
	
	return 0;
}
//---------------------------------------------------------------------------------
BMessage*	
BWindow::extract_drop(BMessage* an_event, BHandler* *target) 
{
	
	return an_event;
}
//---------------------------------------------------------------------------------
BMessage*	
BWindow::ReadMessageFromPort(bigtime_t tout = B_INFINITE_TIMEOUT)
{
	return BLooper::ReadMessageFromPort(tout);	// Is this right?
}
//---------------------------------------------------------------------------------
int32	
BWindow::MessagesWaiting()
{
	
	return 0;
}
//---------------------------------------------------------------------------------
void
BWindow::movesize(uint32 opcode, float h, float v)
{
	
}
//---------------------------------------------------------------------------------
void
BWindow::handle_activate(BMessage* an_event)
{
	
}
//---------------------------------------------------------------------------------
void
BWindow::do_view_frame(BMessage* an_event)
{
	
}
//---------------------------------------------------------------------------------
void
BWindow::do_value_change(BMessage* an_event, BHandler* handler)
{
	
}
//---------------------------------------------------------------------------------
void
BWindow::do_mouse_down(BMessage* an_event, BView* target)
{
	
}
//---------------------------------------------------------------------------------
void
BWindow::do_mouse_moved(BMessage* an_event, BView* target)
{
	
}
//---------------------------------------------------------------------------------
void
BWindow::do_key_down(BMessage* an_event, BHandler* handler)
{
	
}
//---------------------------------------------------------------------------------
void
BWindow::do_key_up(BMessage* an_event, BHandler* handler)
{
	
}
//---------------------------------------------------------------------------------
void
BWindow::do_menu_event(BMessage* an_event)
{
	
}
//---------------------------------------------------------------------------------
void
BWindow::do_draw_views()
{
	
}
//---------------------------------------------------------------------------------
/* Virtual */
BMessage*
BWindow::ConvertToMessage(void* raw, int32 code)
{
	BLooper::ConvertToMessage(raw,code);		// Is This right?
}
//---------------------------------------------------------------------------------
_cmd_key_*
BWindow::allocShortcut(uint32 key, uint32 modifiers)
{
	
}
//---------------------------------------------------------------------------------
_cmd_key_*
BWindow::FindShortcut(uint32 key, uint32 modifiers)
{
	
}
//---------------------------------------------------------------------------------
void
BWindow::AddShortcut(uint32 key, uint32 modifiers, BMenuItem* item)
{
	
}
//---------------------------------------------------------------------------------
void
BWindow::post_message(BMessage* message)
{
	
}
//---------------------------------------------------------------------------------
void
BWindow::SetLocalTitle(const char* new_title)
{
	*fTitle = *new_title;
}
//---------------------------------------------------------------------------------
void
BWindow::enable_pulsing(bool enable)
{
	pulse_enabled = enable;
}
//---------------------------------------------------------------------------------
BHandler*
BWindow::determine_target(BMessage* msg, BHandler* target, bool pref)
{
	
	BHandler	*RandomTarget;
	return RandomTarget;
}
//---------------------------------------------------------------------------------
void
BWindow::kb_navigate()
{
	
}
//---------------------------------------------------------------------------------
void
BWindow::navigate_to_next(int32 direction, bool group = false)
{
	
}
//---------------------------------------------------------------------------------
void
BWindow::set_focus(BView* focus, bool notify_input_server)
{
	
}
//---------------------------------------------------------------------------------
bool
BWindow::InUpdate()
{
	
	return false;
}
//---------------------------------------------------------------------------------
void
BWindow::DequeueAll()
{
	
}
//---------------------------------------------------------------------------------
bool
BWindow::find_token_and_handler(BMessage* msg, int32* token, BHandler* *handler)
{
	
	return false;
}
//---------------------------------------------------------------------------------
window_type
BWindow::compose_type(window_look look, window_feel feel) const
{
	
	return B_UNTYPED_WINDOW;
}
//---------------------------------------------------------------------------------
void
BWindow::decompose_type(window_type type, window_look* look, window_feel* feel) const
{
	switch(type)
	{	
	  case B_TITLED_WINDOW :
	       {
		  *look = B_TITLED_WINDOW_LOOK;
		  *feel = B_NORMAL_WINDOW_FEEL;
	       }	    
	  case B_DOCUMENT_WINDOW :
	       {
		  *look = B_DOCUMENT_WINDOW_LOOK;
		  *feel = B_NORMAL_WINDOW_FEEL;
	       }
	  case B_MODAL_WINDOW :
	       {
		  *look = B_MODAL_WINDOW_LOOK;
		  *feel = B_MODAL_APP_WINDOW_FEEL;
	       }
	  case B_FLOATING_WINDOW :
	       {
		  *look = B_FLOATING_WINDOW_LOOK;
		  *feel = B_FLOATING_APP_WINDOW_FEEL;
	       }
	  case B_BORDERED_WINDOW :
	       {
		  *look = B_BORDERED_WINDOW_LOOK;
		  *feel = B_NORMAL_WINDOW_FEEL;
	       }

	  default :
	       {
		  *look = B_TITLED_WINDOW_LOOK;
		  *feel = B_NORMAL_WINDOW_FEEL;
	       }
	}
}
//---------------------------------------------------------------------------------
void
BWindow::SetIsFilePanel(bool panel)
{
	
}
//---------------------------------------------------------------------------------
/* DEPRECATED DEPRECATED DEPRECATED */
void
BWindow::AddFloater(BWindow* a_floating_window)
{
	
}
//---------------------------------------------------------------------------------
/* DEPRECATED DEPRECATED DEPRECATED */
void
BWindow::RemoveFloater(BWindow* a_floating_window)
{
	
}
//---------------------------------------------------------------------------------
/* DEPRECATED DEPRECATED DEPRECATED */
window_type
BWindow::WindowType() const
{
	
}
//---------------------------------------------------------------------------------
void BWindow::_ReservedWindow1()
{
	
}
//---------------------------------------------------------------------------------
void BWindow::_ReservedWindow2()
{
	
}
//---------------------------------------------------------------------------------
void BWindow::_ReservedWindow3()
{
	
}
//---------------------------------------------------------------------------------
void BWindow::_ReservedWindow4()
{
	
}
//---------------------------------------------------------------------------------
void BWindow::_ReservedWindow5()
{
	
}
//---------------------------------------------------------------------------------
void BWindow::_ReservedWindow6()
{
	
}
//---------------------------------------------------------------------------------
void BWindow::_ReservedWindow7()
{
	
}
//---------------------------------------------------------------------------------
void BWindow::_ReservedWindow8()
{
	
}


