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
//	Author:			Adrian Oanca (oancaadrian@yahoo.com)
//	Description:	A BWindow object represents a window that can be displayed
//					on the screen, and that can be the target of user events
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <BeBuild.h>

// Project Includes ------------------------------------------------------------
#include <InterfaceDefs.h>
#include <Application.h>
#include <Looper.h>
#include <Handler.h>
#include <View.h>
#include <MenuBar.h>
#include <String.h>
#include <PropertyInfo.h>
#include <Window.h>
#include <Screen.h>
#include <Button.h>

// Local Includes --------------------------------------------------------------
#include <PortLink.h>
#include "WindowAux.h"
#include "ServerProtocol.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------
static property_info windowPropInfo[] =
{
	{ "Feel", { B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Returns the current feel of the window.",0 },

	{ "Feel", { B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Sets the feel of the window.",0 },

	{ "Flags", { B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Returns the current flags of the window.",0 },

	{ "Flags", { B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Sets the window flags.",0 },

	{ "Frame", { B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Returns the window's frame rectangle.",0},

	{ "Frame", { B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Sets the window's frame rectangle.",0 },

	{ "Hidden", { B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Returns true if the window is hidden; false otherwise.",0},

	{ "Hidden", { B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Hides or shows the window.",0 },

	{ "Look", { B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Returns the current look of the window.",0},

	{ "Look", { B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Sets the look of the window.",0 },

	{ "Title", { B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Returns a string containing the window title.",0},

	{ "Title", { B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Sets the window title.",0 },

	{ "Workspaces", { B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Returns int32 bitfield of the workspaces in which the window appears.",0},

	{ "Workspaces", { B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Sets the workspaces in which the window appears.",0 },

	{ "MenuBar", { 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Directs the scripting message to the key menu bar.",0 },

	{ "View", { 0 },
		{ 0 }, "Directs the scripting message to the top view without popping the current specifier.",0 },

	{ "Minimize", { B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Minimizes the window if \"data\" is true; restores otherwise.",0 },

	{ 0, { 0 }, { 0 }, 0, 0 }
}; 
//------------------------------------------------------------------------------

// Constructors
//------------------------------------------------------------------------------
BWindow::BWindow(BRect frame,
				const char* title, 
				window_type type,
				uint32 flags,
				uint32 workspace = B_CURRENT_WORKSPACE)
{
	window_look look;
	window_feel feel;
	
	decomposeType(type, &look, &feel);

	BWindow(frame, title, look, feel, flags, workspace);
}

//------------------------------------------------------------------------------

BWindow::BWindow(BRect frame,
				const char* title, 
				window_look look,
				window_feel feel,
				uint32 flags,
				uint32 workspace = B_CURRENT_WORKSPACE)
			: BLooper( title )
{
	if ( be_app == NULL ){
		//debugger("You need a valid BApplication object before interacting with the app_server");
		return;
	}
	fTitle=NULL;
	InitData( frame, title, look, feel, flags, workspace );

	receive_port	= create_port( B_LOOPER_PORT_DEFAULT_CAPACITY ,
						"OBOS: BWindow - receivePort");
	if (receive_port==B_BAD_VALUE || receive_port==B_NO_MORE_PORTS){
		//debugger("Could not create BWindow's receive port, used for interacting with the app_server!");
		return;
	}
	

		// let app_server to know that a window has been created.
	serverLink = new PortLink(be_app->fServerTo);
	serverLink->SetOpCode(AS_CREATE_WINDOW);
	serverLink->Attach(fFrame);
	serverLink->Attach((int32)WindowLookToInteger(fLook));
	serverLink->Attach((int32)WindowFeelToInteger(fFeel));
	serverLink->Attach((int32)fFlags);
	serverLink->Attach((int32)workspace);	
	serverLink->Attach(_get_object_token_(this));
	serverLink->Attach(&receive_port,sizeof(port_id));
		// We add one so that the string will end up NULL-terminated.
	serverLink->Attach( (char*)title, strlen(title)+1 );
	
		// Send and wait for ServerWindow port. Necessary here so we can respond to
		// messages as soon as Show() is called.
	PortLink::ReplyData replyData;
	status_t replyStat;

		// HERE we are in BApplication's thread, so for locking we use be_app variable
		// we'll lock the be_app to be sure we're the only one writing at BApplication's server port
	be_app->Lock();
	
	replyStat		= serverLink->FlushWithReply( &replyData );
	if ( replyStat != B_OK ){
		//debugger("First reply from app_server was not received.");
		return;
	}
	
		// unlock, so other threads can do their job.
	be_app->Unlock();
	
	send_port		= *((port_id*)replyData.buffer);

		// Set the port on witch app_server will listen for us	
	serverLink->SetPort(send_port);
		// Initialize a PortLink object for use with graphic calls.
		// They need to be sent separately because of the speed reasons.
	srvGfxLink		= new PortLink( send_port );	
	
	delete replyData.buffer;

		// Create and attach the top view
	top_view			= buildTopView();
		// Here we will register the top view with app_server
// TODO: implement the following function
	attachTopView( );
}

//------------------------------------------------------------------------------

BWindow::BWindow(BMessage* data)
	: BLooper(data)
{
// TODO: implement correctly
	BMessage msg;
	BArchivable *obj;

// TODO: Instantiate BWindow's data members
		//	like: data->FindInt32("MyClass::Property1", &property1);


		// I still have doubts about instantiating childrens in this way,
		// but I REALLY think it's OK
	if (data->FindMessage("children", &msg) == B_OK){
		obj			= instantiate_object(&msg);
//		top_view	= dynamic_cast<BView *>(obj);

//		top_view->Instantiate( &msg );
	} 
}

//------------------------------------------------------------------------------

BWindow::~BWindow(){

		// the following lines, remove all existing shortcuts and delete accelList
	int32			noOfItems;
	
	noOfItems		= accelList.CountItems();
	for ( int index = noOfItems;  index >= 0; index-- ) {
		_BCmdKey		*cmdKey;

		cmdKey			= (_BCmdKey*)accelList.ItemAt( index );
		
		accelList.RemoveItem(index);
		
		delete cmdKey->message;
		delete cmdKey;
	}
	
// TODO: release other dinamicaly alocated objects

		// topView has already been detached by BWindow::Quit()
	delete		top_view;

	delete		srvGfxLink;
	delete		serverLink;
	delete_port( receive_port );
}

//------------------------------------------------------------------------------

BArchivable* BWindow::Instantiate(BMessage* data){
   if ( !validate_instantiation( data , "BWindow" ) ) 
      return NULL; 

   return new BWindow(data); 
}

//------------------------------------------------------------------------------

status_t BWindow::Archive(BMessage* data, bool deep = true) const{

// TODO: implement correctly
	status_t		retval;
	retval			= B_OK;

	BLooper::Archive( data, deep );

// TODO: add BWindow's members to *data

	if (deep)
	{
		BMessage		childArchive;

		retval			= top_view->Archive( &childArchive, deep);
		if (retval != B_OK)
			return retval;

		data->AddMessage( "children", &childArchive );
	}


		// SHOULD WE ADD THIS ??????????
		//   in disassembly code this isn't called - that makes no sense
		//   because BWindow::Instantiate( BMessage* ) calls support kit's
		//   validate_instantiation( BMessage*, "BWindow" ). and if we do not
		//   put the following line, validate_instantiation(...) would not
		//   find "BWindow" in the message's "class" field
		// TODO: ask someone that works on Support Kit
	data->AddString("class", "BWindow");


		// SHOULD WE ADD THIS ??????????
		//   if we do, what is BWindow's signature???
	data->AddString("add_on", "application/Be-window");
	
	return retval;
}

//------------------------------------------------------------------------------

void BWindow::Quit(){
/*
	Quit() removes the window from the screen(1), deletes all the BViews in its view
 		hierarchy(2), destroys the window thread(3), removes the window's connection
		to the Application Server(4), and deletes the BWindow object(5). 
*/
		//		1
	while (!IsHidden())	{ Hide(); }

		//		2, 4
	detachTopView();	// AND kills app_server connection

		//		3
	Lock();
	BLooper::Quit();

		//		5
	delete this;		// is it good?? I have to check!

}

//------------------------------------------------------------------------------

void BWindow::AddChild(BView *child, BView *before = NULL){
	top_view->AddChild( child, before );
}

//------------------------------------------------------------------------------

bool BWindow::RemoveChild(BView *child){
	return top_view->RemoveChild( child );
}

//------------------------------------------------------------------------------

int32 BWindow::CountChildren() const{
	return top_view->CountChildren();
}

//------------------------------------------------------------------------------

BView* BWindow::ChildAt(int32 index) const{
	return top_view->ChildAt( index );
}

//------------------------------------------------------------------------------

void BWindow::Minimize(bool minimize){
	if (IsModal())
		return;

	if (IsFloating())
		return;		

	serverLink->SetOpCode( B_MINIMIZE );
	serverLink->Attach( minimize );

	Lock();
	serverLink->Flush( );
	Unlock();

	fMinimized		= minimize;
	
	if (minimize == false)
		Activate();
}
//------------------------------------------------------------------------------

status_t BWindow::SendBehind(const BWindow* window){

	PortLink::ReplyData		replyData;
	
	serverLink->SetOpCode( AS_SEND_BEHIND );
// TODO: check the order of the next 2 lines	
	//serverLink->Attach( getServerToken() );
	//serverLink->Attach( window->getServerToken() );	

	Lock();
	serverLink->FlushWithReply( &replyData );
	Unlock();
	
	delete replyData.buffer;

	return replyData.code == SERVER_TRUE? B_OK : B_ERROR;
}

//------------------------------------------------------------------------------

void BWindow::Flush() const{
	const_cast<BWindow*>(this)->Lock();
	srvGfxLink->Flush();
	const_cast<BWindow*>(this)->Unlock();
}

//------------------------------------------------------------------------------

void BWindow::Sync() const{
	PortLink::ReplyData		replyData;

	const_cast<BWindow*>(this)->Lock();	
	srvGfxLink->FlushWithReply( &replyData );
	const_cast<BWindow*>(this)->Unlock();
	
	delete replyData.buffer;
}

//------------------------------------------------------------------------------

void BWindow::DisableUpdates(){

	serverLink->SetOpCode( AS_DISABLE_UPDATES );
	
	Lock();	
	serverLink->Flush( );
	Unlock();
}

//------------------------------------------------------------------------------

void BWindow::EnableUpdates(){
	
	serverLink->SetOpCode( AS_ENABLE_UPDATES );

	Lock();
	serverLink->Flush( );
	Unlock();
}

//------------------------------------------------------------------------------

void BWindow::BeginViewTransaction(){
	if ( !fInTransaction ){
		srvGfxLink->SetOpCode( AS_BEGIN_UPDATE );
		fInTransaction		= true;
	}
}

//------------------------------------------------------------------------------

void BWindow::EndViewTransaction(){
	if ( fInTransaction ){
		srvGfxLink->Attach( (int32)AS_END_UPDATE );
		fInTransaction		= false;
		Flush();
	}
}

//------------------------------------------------------------------------------

bool BWindow::IsFront() const{
	if (IsActive())
		return true;

	if (IsModal())
		return true;

	return false;
}

//------------------------------------------------------------------------------

void BWindow::MessageReceived( BMessage *msg )
{ 
	BMessage			specifier;
	int32				what;
	const char*			prop;
	int32				index;
	status_t			err;

	err = msg->GetCurrentSpecifier(&index, &specifier, &what, &prop);
	if (err == B_OK)
	{
		BMessage			replyMsg;

		switch (msg->what)
		{
		case B_GET_PROPERTY:{
				replyMsg.what		= B_NO_ERROR;
				replyMsg.AddInt32( "error", B_OK );
				
				if (strcmp(prop, "Feel") ==0 )
				{
					replyMsg.AddInt32( "result", (uint32)Feel());
				}
				else if (strcmp(prop, "Flags") ==0 )
				{
					replyMsg.AddInt32( "result", Flags());
				}
				else if (strcmp(prop, "Frame") ==0 )
				{
					replyMsg.AddRect( "result", Frame());				
				}
				else if (strcmp(prop, "Hidden") ==0 )
				{
					replyMsg.AddBool( "result", IsHidden());				
				}
				else if (strcmp(prop, "Look") ==0 )
				{
					replyMsg.AddInt32( "result", (uint32)Look());				
				}
				else if (strcmp(prop, "Title") ==0 )
				{
					replyMsg.AddString( "result", Title());				
				}
				else if (strcmp(prop, "Workspaces") ==0 )
				{
					replyMsg.AddInt32( "result", Workspaces());				
				}
			}break;

		case B_SET_PROPERTY:{
// TODO: check if msg->FindInt32 affects our data!
//			we need a uint32 !!!		
				if (strcmp(prop, "Feel") ==0 )
				{
					int32			newFeel;
					if (msg->FindInt32( "data", &newFeel ) == B_OK){
						SetFeel( (window_feel)newFeel );
						
						replyMsg.what		= B_NO_ERROR;
						replyMsg.AddInt32( "error", B_OK );
					}
					else{
						replyMsg.what		= B_MESSAGE_NOT_UNDERSTOOD;
						replyMsg.AddInt32( "error", B_BAD_SCRIPT_SYNTAX );
						replyMsg.AddString( "message", "Didn't understand the specifier(s)" );
					}
				}
				
				else if (strcmp(prop, "Flags") ==0 )
				{
					int32			newFlags;
					if (msg->FindInt32( "data", &newFlags ) == B_OK){
						SetFlags( newFlags );
						
						replyMsg.what		= B_NO_ERROR;
						replyMsg.AddInt32( "error", B_OK );
					}
					else{
						replyMsg.what		= B_MESSAGE_NOT_UNDERSTOOD;
						replyMsg.AddInt32( "error", B_BAD_SCRIPT_SYNTAX );
						replyMsg.AddString( "message", "Didn't understand the specifier(s)" );
					}
				}
				
				else if (strcmp(prop, "Frame") ==0 )
				{
					BRect			newFrame;
					if (msg->FindRect( "data", &newFrame ) == B_OK){
						MoveTo( newFrame.LeftTop() );
						
						if ( newFrame.right < fMinWindWidth )
							newFrame.right			= fMinWindWidth;

						if ( newFrame.bottom < fMinWindHeight )
							newFrame.bottom		= fMinWindHeight;

						if ( newFrame.right > fMaxWindWidth )
							newFrame.right			= fMaxWindWidth;

						if ( newFrame.bottom > fMaxWindHeight )
							newFrame.bottom		= fMaxWindHeight;
							
						ResizeTo( newFrame.right, newFrame.bottom);
						
						replyMsg.what		= B_NO_ERROR;
						replyMsg.AddInt32( "error", B_OK );
					}
					else{
						replyMsg.what		= B_MESSAGE_NOT_UNDERSTOOD;
						replyMsg.AddInt32( "error", B_BAD_SCRIPT_SYNTAX );
						replyMsg.AddString( "message", "Didn't understand the specifier(s)" );
					}
				}
				
				else if (strcmp(prop, "Hidden") ==0 )
				{
					bool			newHiddenState;
					if (msg->FindBool( "data", &newHiddenState ) == B_OK){
						if ( !IsHidden() && newHiddenState == true ){
							Hide();
							
							replyMsg.what		= B_NO_ERROR;
							replyMsg.AddInt32( "error", B_OK );
							
						}
						else if ( IsHidden() && newHiddenState == false ){
							Show();
							
							replyMsg.what		= B_NO_ERROR;
							replyMsg.AddInt32( "error", B_OK );
						}
						else{
							replyMsg.what		= B_MESSAGE_NOT_UNDERSTOOD;
							replyMsg.AddInt32( "error", B_BAD_SCRIPT_SYNTAX );
							replyMsg.AddString( "message", "Didn't understand the specifier(s)" );
						}
					}
					else{
						replyMsg.what		= B_MESSAGE_NOT_UNDERSTOOD;
						replyMsg.AddInt32( "error", B_BAD_SCRIPT_SYNTAX );
						replyMsg.AddString( "message", "Didn't understand the specifier(s)" );
					}
				}
				
				else if (strcmp(prop, "Look") ==0 )
				{
					int32			newLook;
					if (msg->FindInt32( "data", &newLook ) == B_OK){
						SetLook( (window_look)newLook );
						
						replyMsg.what		= B_NO_ERROR;
						replyMsg.AddInt32( "error", B_OK );
					}
					else{
						replyMsg.what		= B_MESSAGE_NOT_UNDERSTOOD;
						replyMsg.AddInt32( "error", B_BAD_SCRIPT_SYNTAX );
						replyMsg.AddString( "message", "Didn't understand the specifier(s)" );
					}
				}
				
				else if (strcmp(prop, "Title") ==0 )
				{
					const char		**newTitle;
					if (msg->FindString( "data", newTitle ) == B_OK){
						SetTitle( *newTitle );
						
						replyMsg.what		= B_NO_ERROR;
						replyMsg.AddInt32( "error", B_OK );
					}
					else{
						replyMsg.what		= B_MESSAGE_NOT_UNDERSTOOD;
						replyMsg.AddInt32( "error", B_BAD_SCRIPT_SYNTAX );
						replyMsg.AddString( "message", "Didn't understand the specifier(s)" );
					}
					delete newTitle;
				}
				
				else if (strcmp(prop, "Workspaces") ==0 )
				{
					int32			newWorkspaces;
					if (msg->FindInt32( "data", &newWorkspaces ) == B_OK){
						SetWorkspaces( newWorkspaces );
						
						replyMsg.what		= B_NO_ERROR;
						replyMsg.AddInt32( "error", B_OK );
					}
					else{
						replyMsg.what		= B_MESSAGE_NOT_UNDERSTOOD;
						replyMsg.AddInt32( "error", B_BAD_SCRIPT_SYNTAX );
						replyMsg.AddString( "message", "Didn't understand the specifier(s)" );
					}
				}
				
			}break;
		}
		msg->SendReply( &replyMsg );
	}
	else{
		BMessage		replyMsg(B_MESSAGE_NOT_UNDERSTOOD);
		replyMsg.AddInt32( "error" , B_BAD_SCRIPT_SYNTAX );
		replyMsg.AddString( "message", "Didn't understand the specifier(s)" );
		
		msg->SendReply( &replyMsg );
	}
} 

//------------------------------------------------------------------------------

void BWindow::DispatchMessage(BMessage *msg, BHandler *target) 
{
		// should ve call Lock()???
	//Lock();
	switch ( msg->what ) { 
	case B_ZOOM:
		Zoom();
		break;

	case B_MINIMIZE:{
		bool minimize;
		msg->FindBool("minimize", &minimize);
		Minimize( minimize );
		break;
		}
	case B_WINDOW_RESIZED:{
		int32 width;
		int32 height;
		msg->FindInt32("width", &width);
		msg->FindInt32("height", &height);
		FrameResized( width, height );
		break;
		}
	case B_WINDOW_MOVED:{
		BPoint origin;
		msg->FindPoint("where", &origin);
		FrameMoved( origin );
		break;
		}
	case B_WINDOW_MOVE_BY:{
		MessageReceived( msg );
		break;
		}
	case B_WINDOW_MOVE_TO:{
		MessageReceived( msg );
		break;
		}
	case B_WINDOW_ACTIVATED:{
		bool activated;
		msg->FindBool("active", &activated);
		WindowActivated( activated );
// TODO: send this message to all attached BViews
		top_view->WindowActivated( activated );
		break;
		}
	case B_SCREEN_CHANGED:{
		BRect frame;
		int32 mode;
		msg->FindRect("frame", &frame);
		msg->FindInt32("mode", &mode);
		ScreenChanged( frame, (color_space)mode );
		break;
		}
	case B_WORKSPACE_ACTIVATED:{
		int32 workspace;
		bool active;
		msg->FindInt32( "workspace", &workspace );
		msg->FindBool( "active", &active );
		WorkspaceActivated( workspace, active );
		break;
		}
	case B_WORKSPACES_CHANGED:{
		int32 oldWorkspace;
		int32 newWorkspace;
		msg->FindInt32( "old", &oldWorkspace );
		msg->FindInt32( "new", &newWorkspace );
		WorkspacesChanged( oldWorkspace, newWorkspace );
		break;
		}
	case B_KEY_DOWN:{
		int32		keyRepeat;
		int32		modifiers;
		BString		string;

		msg->FindInt32( "be:key_repeat", &keyRepeat );
		msg->FindInt32( "modifiers", &modifiers );
		msg->FindString( "bytes", &string );

// TODO: !!! Implement handleBWindowKeys(...)
		if ( !handleBWindowKeys( string, modifiers) )
			fFocus->KeyDown( string.String(), keyRepeat );
		break;
		}
	case B_KEY_UP:{
		int32		keyRepeat;
		BString		string;

		msg->FindInt32( "be:key_repeat", &keyRepeat );
		msg->FindString( "bytes", &string );

		fFocus->KeyUp( string.String(), keyRepeat );
		break;
		}
	case B_MOUSE_DOWN:{
		BPoint		where;
		int32		modifiers;
		int32		buttons;
		int32		clicks;

		msg->FindPoint( "where", &where );
		msg->FindInt32( "modifiers", &modifiers );
		msg->FindInt32( "buttons", &buttons );
		msg->FindInt32( "clicks", &clicks );
		
// TODO: find the view for witch this message was sent!
// TODO: !!! change Focus !!!
// TODO: !!! Implement sendMessageUsingEventMask(...) !!!
		sendMessageUsingEventMask( B_MOUSE_DOWN, where );
		break;
		}
	case B_MOUSE_UP:{
		BPoint		where;
		int32		modifiers;

		msg->FindPoint( "where", &where );
		msg->FindInt32( "modifiers", &modifiers );
		
// TODO: !!! Implement sendMessageUsingEventMask(...) !!!
		sendMessageUsingEventMask( B_MOUSE_UP, where );
		break;
		}
	case B_MOUSE_MOVED:{
		BPoint		where;
		int32		buttons;

		msg->FindPoint( "where", &where );
		msg->FindInt32( "buttons", &buttons );
		
// TODO: !!! Implement sendMessageUsingEventMask(...) !!!
		sendMessageUsingEventMask( B_MOUSE_MOVED, where );
		break;
		}
	case B_VIEW_RESIZED:{
		int32 width;
		int32 height;

		msg->FindInt32("width", &width);
		msg->FindInt32("height", &height);

// TODO: find the view for witch this message was sent!
		top_view->FrameResized( width, height );
		break;
		}
	case B_VIEW_MOVED:{
		BPoint origin;

		msg->FindPoint("where", &origin);

// TODO: find the view for witch this message was sent!
		top_view->FrameMoved( origin );
		break;
		}
	case B_VALUE_CHANGED:{
		int32		newScrollValue;

		msg->FindInt32( "value" , &newScrollValue );

// TODO: make a research to see what is this all about!
		// oh shit... what scrollbar??? of an active BView???  have to make some tests!!!
		// call hook function BScrollBar::ValueChanged( (float)newScrollValue );
		break;
		}
/*
Note:	The next four messages are sent by handleBWindowKeys(...) function
			when it finds key combinations like Command+q, Command+c, Command+v, ...
			
*/
	case B_CUT:{
// TODO: find the view for witch this message was sent!
		top_view->MessageReceived( msg );
		break;
		}
	case B_COPY:{
// TODO: find the view for witch this message was sent!
		top_view->MessageReceived( msg );
		break;
		}
	case B_PASTE:{
// TODO: find the view for witch this message was sent!
		top_view->MessageReceived( msg );
	break;
		}
	case B_SELECT_ALL:{
// TODO: find the view for witch this message was sent!
		top_view->MessageReceived( msg );
		break;
		}
	case B_PULSE:{

		sendWillingBViewsPulseMsg( top_view );
		break;
		}
	case B_QUIT_REQUESTED:{
		Quit();
		break;
		}
	default:{
		MessageReceived( msg );
		//Unlock();
		BLooper::DispatchMessage(msg, target); 
		break; 
		}
   }
   //Unlock();
} 

//------------------------------------------------------------------------------

void BWindow::FrameMoved(BPoint new_position){
	// does nothing
	// Hook function
}

//------------------------------------------------------------------------------

void BWindow::FrameResized(float new_width, float new_height){
	// does nothing
	// Hook function
}

//------------------------------------------------------------------------------

void BWindow::WorkspacesChanged(uint32 old_ws, uint32 new_ws){
	// does nothing
	// Hook function
}

//------------------------------------------------------------------------------

void BWindow::WorkspaceActivated(int32 ws, bool state){
	// does nothing
	// Hook function
}

//------------------------------------------------------------------------------

void BWindow::MenusBeginning(){
	// does nothing
	// Hook function
}

//------------------------------------------------------------------------------

void BWindow::MenusEnded(){
	// does nothing
	// Hook function
}

//------------------------------------------------------------------------------

void BWindow::SetSizeLimits(float minWidth, float maxWidth, 
							float minHeight, float maxHeight){
	fMinWindHeight		= minHeight;
	fMinWindWidth		= minWidth;
	fMaxWindHeight		= maxHeight;
	fMaxWindWidth		= maxWidth;
	
// TODO: we should send those to app_server	
}

//------------------------------------------------------------------------------

void BWindow::GetSizeLimits(float *minWidth, float *maxWidth, 
							float *minHeight, float *maxHeight){
	*minHeight			= fMinWindHeight;
	*minWidth			= fMinWindWidth;
	*maxHeight			= fMaxWindHeight;
	*maxWidth			= fMaxWindWidth;
}

//------------------------------------------------------------------------------

void BWindow::SetZoomLimits(float maxWidth, float maxHeight){
	fMaxZoomWidth		= maxWidth;
	fMaxZoomHeight		= maxHeight;
	
// TODO: we should send those to app_server		
}

//------------------------------------------------------------------------------

void BWindow::Zoom(	BPoint rec_position, float rec_width, float rec_height){

		// this is also a Hook function!
		
	MoveTo( rec_position );
	ResizeTo( rec_width, rec_height );
}

//------------------------------------------------------------------------------

void BWindow::Zoom(){
	float			minWidth, minHeight;
	BScreen			screen;
/*	from BeBook:
	However, if the window's rectangle already matches these "zoom" dimensions
	(give or take a few pixels), Zoom() passes the window's previous
	("non-zoomed") size and location. 
*/
	if (Frame().Width() == fMaxZoomWidth && Frame().Height() == fMaxZoomHeight) {
		BPoint position( Frame().left, Frame().top);
		Zoom( position, fMaxZoomWidth, fMaxZoomHeight );
		return;
	}
	
/*	from BeBook:
	The dimensions that non-virtual Zoom() passes to hook Zoom() are deduced from
	the smallest of three rectangles: 3) the screen rectangle, 1) the rectangle
	defined by SetZoomLimits(), 2) the rectangle defined by SetSizeLimits()
*/
		// 1
	minHeight		= fMaxZoomHeight;
	minWidth		= fMaxZoomWidth;

		// 2
	if ( fMaxWindHeight < minHeight ) { minHeight		= fMaxWindHeight; }
	if ( fMaxWindWidth  < minWidth  ) { minWidth		= fMaxWindWidth; }

		// 3
	if ( screen.Frame().Width()  < minWidth )   { minWidth		= screen.Frame().Width(); }
	if ( screen.Frame().Height() < minHeight  ) { minHeight		= screen.Frame().Height(); }

	Zoom( Frame().LeftTop(), minWidth, minHeight );
}

void BWindow::ScreenChanged(BRect screen_size, color_space depth){
	// Hook function
	// does nothing
}

void BWindow::SetPulseRate(bigtime_t rate){
	if (fPulseEnabled){
		fPulseRate		= rate;

// TODO: notify somebudy that we want to receive pulsing messages!!!

	}
}

//------------------------------------------------------------------------------

bigtime_t BWindow::PulseRate() const{
	return fPulseRate;
}

//------------------------------------------------------------------------------

void BWindow::AddShortcut(	uint32 key,	uint32 modifiers, BMessage* msg){
	AddShortcut( key, modifiers, msg, this);
}

//------------------------------------------------------------------------------

void BWindow::AddShortcut(	uint32 key,	uint32 modifiers, BMessage* msg, BHandler* target){
	int64				when;
	_BCmdKey			*cmdKey;

		// if target is NULL, the focus view will receive the message
		//		if there is no focus view, I thought, BWindow should receive the message
		//		I have not tested this, but I think it is a good idea to do so.
	if (target == NULL)
		if (fFocus)
			target		= fFocus;
		else
			target		= this;
	
	if ( !findHandler( top_view, target ) )
		return;

	when				= real_time_clock_usecs();
	msg->AddInt64("when", when);

// TODO:	make sure key is a lowercase char !!!

	modifiers			= modifiers | B_COMMAND_KEY;

	cmdKey				= new _BCmdKey;
	cmdKey->key			= key;
	cmdKey->modifiers	= modifiers;
	cmdKey->message		= msg;
	cmdKey->target		= target;

		// removes the shortcut from accelList if it exists!
	RemoveShortcut( key, modifiers );

	accelList.AddItem( cmdKey );

}

//------------------------------------------------------------------------------

void BWindow::RemoveShortcut(uint32 key, uint32 modifiers){
	int32				index;
	
	modifiers			= modifiers | B_COMMAND_KEY;

	index				= findShortcut( key, modifiers );
	if ( index >=0 ) {
		_BCmdKey		*cmdKey;

		cmdKey			= (_BCmdKey*)accelList.ItemAt( index );

		accelList.RemoveItem(index);
		
		delete cmdKey->message;
		delete cmdKey;
	}
}

//------------------------------------------------------------------------------

BButton* BWindow::DefaultButton() const{
	return fDefaultButton;
}

//------------------------------------------------------------------------------

void BWindow::SetDefaultButton(BButton* button){
/*
Note: for developers!
	He he, if you realy want to understand what is happens here, take a piece of
		paper and start taking possible values and then walk with them through
		the code.
*/
	BButton				*aux;

	if ( fDefaultButton == button )
		return;

	if ( fDefaultButton ){
		aux				= fDefaultButton;
		fDefaultButton	= NULL;
		aux->MakeDefault( false );
		aux->Draw( aux->Bounds() );
	}
	
	if ( button == NULL ){
		fDefaultButton		= NULL;
		return;
	}
	
	fDefaultButton			= button;
	fDefaultButton->MakeDefault( true );
	fDefaultButton->Draw( aux->Bounds() );
}

//------------------------------------------------------------------------------

bool BWindow::NeedsUpdate() const{
	PortLink::ReplyData		replyData;

	serverLink->SetOpCode( AS_NEEDS_UPDATE );

	const_cast<BWindow*>(this)->Lock();	
	serverLink->FlushWithReply( &replyData );
	const_cast<BWindow*>(this)->Unlock();
	
	delete replyData.buffer;
	
	return replyData.code == SERVER_TRUE;
}

//------------------------------------------------------------------------------

void BWindow::UpdateIfNeeded(){
	if (Thread() != B_ERROR)
		drawAllViews( top_view );
}

//------------------------------------------------------------------------------

BView* BWindow::FindView(const char* viewName) const{

	return findView( top_view, viewName );
}

//------------------------------------------------------------------------------

BView* BWindow::FindView(BPoint point) const{

	return findView( top_view, point );
}

//------------------------------------------------------------------------------

BView* BWindow::CurrentFocus() const{
	return fFocus;
}

//------------------------------------------------------------------------------

void BWindow::Activate(bool yes = true){
	if (IsHidden())
		return;

	serverLink->SetOpCode( B_WINDOW_ACTIVATED );
	serverLink->Attach( yes );
	
	Lock();
	serverLink->Flush( );
	Unlock();
}

//------------------------------------------------------------------------------

void BWindow::WindowActivated(bool state){
	// hook function
	// does nothing
}

//------------------------------------------------------------------------------

void BWindow::ConvertToScreen(BPoint* pt) const{
	pt->x			+= fFrame.left;
	pt->y			+= fFrame.top;
}

//------------------------------------------------------------------------------

BPoint BWindow::ConvertToScreen(BPoint pt) const{
	pt.x			+= fFrame.left;
	pt.y			+= fFrame.top;

	return pt;
}

//------------------------------------------------------------------------------

void BWindow::ConvertFromScreen(BPoint* pt) const{
	pt->x			-= fFrame.left;
	pt->y			-= fFrame.top;
}

//------------------------------------------------------------------------------

BPoint BWindow::ConvertFromScreen(BPoint pt) const{
	pt.x			-= fFrame.left;
	pt.y			-= fFrame.top;

	return pt;
}

//------------------------------------------------------------------------------

void BWindow::ConvertToScreen(BRect* rect) const{
	rect->top			+= fFrame.top;
	rect->left			+= fFrame.left;
	rect->bottom		+= fFrame.top;
	rect->right			+= fFrame.left;
}

//------------------------------------------------------------------------------

BRect BWindow::ConvertToScreen(BRect rect) const{
	rect.top			+= fFrame.top;
	rect.left			+= fFrame.left;
	rect.bottom			+= fFrame.top;
	rect.right			+= fFrame.left;

	return rect;
}

//------------------------------------------------------------------------------

void BWindow::ConvertFromScreen(BRect* rect) const{
	rect->top			-= fFrame.top;
	rect->left			-= fFrame.left;
	rect->bottom		-= fFrame.top;
	rect->right			-= fFrame.left;
}

//------------------------------------------------------------------------------

BRect BWindow::ConvertFromScreen(BRect rect) const{
	rect.top			-= fFrame.top;
	rect.left			-= fFrame.left;
	rect.bottom			-= fFrame.top;
	rect.right			-= fFrame.left;

	return rect;
}

//------------------------------------------------------------------------------

bool BWindow::IsMinimized() const{
		// Hiding takes precendence over minimization!!!
	if ( IsHidden() )
		return false;

	return fMinimized;
}

//------------------------------------------------------------------------------

BRect BWindow::Bounds() const{
	BRect			bounds( 0.0, 0.0, fFrame.Width(), fFrame.Height() );
	return bounds;
}

//------------------------------------------------------------------------------

BRect BWindow::Frame() const{
	return fFrame;
}

//------------------------------------------------------------------------------

const char* BWindow::Title() const{
	return fTitle;
}

//------------------------------------------------------------------------------

void BWindow::SetTitle(const char* title){
	if (!title)
		return;

	if (fTitle){
		delete fTitle;
		fTitle = NULL;
	}
	
	fTitle		= strdup( title );

		// we will change BWindow's thread name to "w>window_title"	
	int32		length;
	length		= strlen( fTitle );
	
	char		*threadName;
	threadName	= new char[32];
	strcpy(threadName, "w>");
	strncat(threadName, fTitle, (length>=29) ? 29: length);

	if (Thread() != B_ERROR ){
		SetName( threadName );
		rename_thread( Thread(), threadName );
	}
	else
		SetName( threadName );

		// we notify the app_server so we can actualy see the change
	serverLink->SetOpCode( AS_WINDOW_TITLE);
	serverLink->Attach( fTitle, strlen(fTitle)+1 );
	
	Lock();
	serverLink->Flush( );
	Unlock();
}

//------------------------------------------------------------------------------

bool BWindow::IsActive() const{
	return fActive;
}

//------------------------------------------------------------------------------

void BWindow::SetKeyMenuBar(BMenuBar* bar){
// TODO: Do a research on menus and see what else you have to do
	fKeyMenuBar				= bar;
}

//------------------------------------------------------------------------------

BMenuBar* BWindow::KeyMenuBar() const{
	return fKeyMenuBar;
}

//------------------------------------------------------------------------------

bool BWindow::IsModal() const{
// TODO: check this behavior in BeOS
	if ( fFeel == B_MODAL_SUBSET_WINDOW_FEEL)
		return true;
	if ( fFeel == B_MODAL_APP_WINDOW_FEEL)
		return true;
	if ( fFeel == B_MODAL_ALL_WINDOW_FEEL)
		return true;

	return false;

}

//------------------------------------------------------------------------------

bool BWindow::IsFloating() const{
// TODO: check this behavior in BeOS
	if ( fFeel == B_FLOATING_SUBSET_WINDOW_FEEL)
		return true;
	if ( fFeel == B_FLOATING_APP_WINDOW_FEEL)
		return true;
	if ( fFeel == B_FLOATING_ALL_WINDOW_FEEL)
		return true;

	return false;
}

//------------------------------------------------------------------------------

status_t BWindow::AddToSubset(BWindow* window){
	if (window->Feel() == B_MODAL_SUBSET_WINDOW_FEEL ||
		window->Feel() == B_FLOATING_SUBSET_WINDOW_FEEL){
		
		PortLink::ReplyData		replyData;

		serverLink->SetOpCode( AS_ADD_TO_SUBSET );
// TODO: when IK Team decides about server tokens, update the following line 
		serverLink->Attach( (int32)window );
		
		Lock();
		serverLink->FlushWithReply( &replyData );
		Unlock();
		
		delete replyData.buffer;

		return replyData.code == SERVER_TRUE? B_OK : B_ERROR;
	}

	return B_ERROR;
}

//------------------------------------------------------------------------------

status_t BWindow::RemoveFromSubset(BWindow* window){

	PortLink::ReplyData		replyData;

	serverLink->SetOpCode( AS_REM_FROM_SUBSET );
// TODO: when IK Team decides about server tokens, update the following line 
	serverLink->Attach( (int32)window );
		
	Lock();
	serverLink->FlushWithReply( &replyData );
	Unlock();

	delete replyData.buffer;

	return replyData.code == SERVER_TRUE? B_OK : B_ERROR;
}

//------------------------------------------------------------------------------

status_t BWindow::Perform(perform_code d, void* arg){
	return BLooper::Perform( d, arg );
}

//------------------------------------------------------------------------------

status_t BWindow::SetType(window_type type){
	decomposeType(type, &fLook, &fFeel);
	SetLook( fLook );
	SetFeel( fFeel );
}

//------------------------------------------------------------------------------

window_type	BWindow::Type() const{
	return composeType( fLook, fFeel );
}

//------------------------------------------------------------------------------

status_t BWindow::SetLook(window_look look){
	uint32					uintLook;
	PortLink::ReplyData		replyData;
	
	uintLook		= WindowLookToInteger( look );

	serverLink->SetOpCode( AS_SET_LOOK );
	serverLink->Attach( &uintLook, sizeof( uint32 ) );
		
	Lock();
	serverLink->FlushWithReply( &replyData );
	Unlock();
	
	delete replyData.buffer;

	if (replyData.code == SERVER_TRUE){
		fLook		= look;
		return B_OK;
	}
	else
		return B_ERROR;
}

//------------------------------------------------------------------------------

window_look	BWindow::Look() const{
	return fLook;
}

//------------------------------------------------------------------------------

status_t BWindow::SetFeel(window_feel feel){
	uint32					uintFeel;
	PortLink::ReplyData		replyData;
	
	uintFeel		= WindowFeelToInteger( feel );

	serverLink->SetOpCode( AS_SET_FEEL );
	serverLink->Attach( &uintFeel, sizeof( uint32 ) );
		
	Lock();
	serverLink->FlushWithReply( &replyData );
	Unlock();
	
	delete replyData.buffer;

	if (replyData.code == SERVER_TRUE){
		fFeel		= feel;
		return B_OK;
	}
	else
		return B_ERROR;
}

//------------------------------------------------------------------------------

window_feel	BWindow::Feel() const{
	return fFeel;
}

//------------------------------------------------------------------------------

status_t BWindow::SetFlags(uint32 flags){
	PortLink::ReplyData		replyData;
	
	serverLink->SetOpCode( AS_SET_FLAGS );
	serverLink->Attach( &flags, sizeof( uint32 ) );
		
	Lock();
	serverLink->FlushWithReply( &replyData );
	Unlock();
	
	delete replyData.buffer;

	if (replyData.code == SERVER_TRUE){
		fFlags		= flags;
		return B_OK;
	}
	else
		return B_ERROR;
}

//------------------------------------------------------------------------------

uint32	BWindow::Flags() const{
	return fFlags;
}

//------------------------------------------------------------------------------

status_t BWindow::SetWindowAlignment(window_alignment mode,
											int32 h, int32 hOffset = 0,
											int32 width = 0, int32 widthOffset = 0,
											int32 v = 0, int32 vOffset = 0,
											int32 height = 0, int32 heightOffset = 0)
{
	PortLink::ReplyData		replyData;

	serverLink->SetOpCode( AS_SET_ALIGNMENT );
	serverLink->Attach( (int32)mode );
	serverLink->Attach( h );
	serverLink->Attach( hOffset );
	serverLink->Attach( width );
	serverLink->Attach( widthOffset );
	serverLink->Attach( v );
	serverLink->Attach( vOffset );
	serverLink->Attach( height );
	serverLink->Attach( heightOffset );

	Lock();
	serverLink->FlushWithReply( &replyData );
	Unlock();
	
	delete replyData.buffer;
	
	if ( replyData.code == SERVER_TRUE){
		return B_NO_ERROR;
	}

	return B_ERROR;
}

//------------------------------------------------------------------------------

status_t BWindow::GetWindowAlignment(window_alignment* mode = NULL,
											int32* h = NULL, int32* hOffset = NULL,
											int32* width = NULL, int32* widthOffset = NULL,
											int32* v = NULL, int32* vOffset = NULL,
											int32* height = NULL, int32* heightOffset = NULL) const
{
	PortLink::ReplyData		replyData;
	int8						*rb;		// short for: replybuffer

	serverLink->SetOpCode( AS_GET_ALIGNMENT );
	
	const_cast<BWindow*>(this)->Lock();
	serverLink->FlushWithReply( &replyData );
	const_cast<BWindow*>(this)->Unlock();
	
	if (replyData.code == SERVER_TRUE){
		rb				= replyData.buffer;

		*mode			= *((window_alignment*)*((int32*)rb));	rb += sizeof(int32);
		*h				= *((int32*)rb);		rb += sizeof(int32);
		*hOffset		= *((int32*)rb);		rb += sizeof(int32);
		*width			= *((int32*)rb);		rb += sizeof(int32);
		*widthOffset	= *((int32*)rb);		rb += sizeof(int32);
		*v				= *((int32*)rb);		rb += sizeof(int32);
		*vOffset		= *((int32*)rb);		rb += sizeof(int32);
		*height			= *((int32*)rb);		rb += sizeof(int32);
		*heightOffset	= *((int32*)rb);

		delete replyData.buffer;

		return B_NO_ERROR;
	}
	
	return B_ERROR;
}

//------------------------------------------------------------------------------

uint32 BWindow::Workspaces() const{
	PortLink::ReplyData	replyData;
	uint32					workspaces;

	serverLink->SetOpCode( AS_GET_WORKSPACES );
	
	const_cast<BWindow*>(this)->Lock();
	serverLink->FlushWithReply( &replyData );
	const_cast<BWindow*>(this)->Unlock();
	
	if (replyData.code == SERVER_TRUE){
		workspaces		= *((uint32*)replyData.buffer);
		
		delete replyData.buffer;

		return workspaces;
	}
	
	return B_CURRENT_WORKSPACE;
}

//------------------------------------------------------------------------------

void BWindow::SetWorkspaces(uint32 workspaces){
	PortLink::ReplyData			replyData;

	serverLink->SetOpCode( AS_SET_WORKSPACES );
	serverLink->Attach( (int32)workspaces );
	
	Lock();
	serverLink->FlushWithReply( &replyData );
	Unlock();

	delete replyData.buffer;
/*
Note:	In future versions of OBOS we should make SetWorkspaces(uint32) return
			a status value! The following code should be added
			
	if (replyData.code == SERVER_TRUE)
		return B_OK;
		
	return B_ERROR;
*/
}

//------------------------------------------------------------------------------

BView* BWindow::LastMouseMovedView() const{
	return fLastMouseMovedView;
}

//------------------------------------------------------------------------------

void BWindow::MoveBy(float dx, float dy){

	serverLink->SetOpCode( B_WINDOW_MOVE_BY );
	serverLink->Attach( dx );
	serverLink->Attach( dy );
	
	Lock();
	serverLink->Flush();
	Unlock();

	fFrame.left			+= dx;
	fFrame.top			+= dy;
}

//------------------------------------------------------------------------------

void BWindow::MoveTo( BPoint point ){
	MoveTo( point.x, point.y );
}

//------------------------------------------------------------------------------

void BWindow::MoveTo(float x, float y){

	serverLink->SetOpCode( B_WINDOW_MOVE_TO );
	serverLink->Attach( x );
	serverLink->Attach( y );
	
	Lock();
	serverLink->Flush();
	Unlock();

	fFrame.left			= x;
	fFrame.top			= y;
}

//------------------------------------------------------------------------------

void BWindow::ResizeBy(float dx, float dy){

	serverLink->SetOpCode( AS_WINDOW_RESIZEBY );
	serverLink->Attach( dx );
	serverLink->Attach( dy );
	
	Lock();
	serverLink->Flush();
	Unlock();

	fFrame.right		+= dx;
	fFrame.bottom		+= dy;
}

//------------------------------------------------------------------------------

void BWindow::ResizeTo(float width, float height){

	serverLink->SetOpCode( AS_WINDOW_RESIZETO );
	serverLink->Attach( width );
	serverLink->Attach( height );
	
	Lock();
	serverLink->Flush( );
	Unlock();

	fFrame.right		= width;
	fFrame.bottom		= height;
}

//------------------------------------------------------------------------------

void BWindow::Show(){
	if ( Thread() == B_ERROR )
		Run();

	fShowLevel--;

	if (fShowLevel == 0){
		serverLink->SetOpCode( AS_SHOW_WINDOW );
		
		Lock();
		serverLink->Flush( );
		Unlock();
	}
}

//------------------------------------------------------------------------------

void BWindow::Hide(){
	if (fShowLevel == 0){
		serverLink->SetOpCode( AS_HIDE_WINDOW );

		Lock();
		serverLink->Flush();
		Unlock();
	}
	fShowLevel++;
}

//------------------------------------------------------------------------------

bool BWindow::IsHidden() const{
	return fShowLevel > 0; 
}

//------------------------------------------------------------------------------

bool BWindow::QuitRequested(){
// TODO: see if there is anything else to do there
	return BLooper::QuitRequested();
}

//------------------------------------------------------------------------------

thread_id BWindow::Run(){
	return BLooper::Run();
}

//------------------------------------------------------------------------------

status_t BWindow::GetSupportedSuites(BMessage* data){
	status_t err = B_OK;
	if (!data)
		err = B_BAD_VALUE;

	if (!err){
		err = data->AddString("Suites", "suite/vnd.Be-window");
		if (!err){
			BPropertyInfo propertyInfo(windowPropInfo);
			err = data->AddFlat("message", &propertyInfo);
			if (!err){
				err = BLooper::GetSupportedSuites(data);
			}
		}
	}
	return err;
}

//------------------------------------------------------------------------------

BHandler* BWindow::ResolveSpecifier(BMessage* msg, int32 index,	BMessage* specifier,
										int32 what,	const char* property)
{
	if (msg->what == B_WINDOW_MOVE_BY)
		return this;
	if (msg->what == B_WINDOW_MOVE_TO)
		return this;

	BPropertyInfo propertyInfo(windowPropInfo);
	switch (propertyInfo.FindMatch(msg, index, specifier, what, property))
	{
		case B_ERROR:
			break;
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
			return this;
			
		case 14:
			if (fKeyMenuBar){
				msg->PopSpecifier();
				return fKeyMenuBar;
			}
			else{
				BMessage		replyMsg(B_MESSAGE_NOT_UNDERSTOOD);
				replyMsg.AddInt32( "error", B_NAME_NOT_FOUND );
				replyMsg.AddString( "message", "This window doesn't have a main MenuBar");
				msg->SendReply( &replyMsg );
				return NULL;
			}
		case 15:
			// we will NOT pop the current specifier
			return top_view;
			
		case 16:
			return this;
	}

	return BLooper::ResolveSpecifier(msg, index, specifier, what, property);
}

// PRIVATE
//--------------------Private Methods-------------------------------------------
// PRIVATE

void BWindow::InitData(	BRect frame,
						const char* title,
						window_look look,
						window_feel feel,
						uint32 flags,
						uint32 workspace){

	fFrame			= frame;
	
	if(fTitle)
	{
		delete fTitle;
		fTitle=NULL;
	}

	if(title)
	{
		fTitle=new char[strlen(title)];
		strcpy( fTitle, title );
	}
	fFeel			= feel;
	fLook			= look;
	fFlags			= flags;
	
	fShowLevel		= 1;

// TODO: set size an zoom limits

// TODO: initialize accelList

// TODO: Add the 5(6) know shortcuts to window's shortcuts list

// TODO: Other stuff
}

//------------------------------------------------------------------------------

void BWindow::task_looper(){

// TODO: Implement for REAL
}

//------------------------------------------------------------------------------

window_type BWindow::composeType(window_look look,	
								 window_feel feel) const
{
	window_type returnValue;

	switch(feel)
	{
	case B_NORMAL_WINDOW_FEEL:
		switch (look)
		{
		case B_TITLED_WINDOW_LOOK:
			returnValue = B_TITLED_WINDOW;
			break;

		case B_DOCUMENT_WINDOW_LOOK:
			returnValue = B_DOCUMENT_WINDOW;
			break;

		case B_BORDERED_WINDOW_LOOK:
			returnValue = B_BORDERED_WINDOW;
			break;
		
		default:
			returnValue = B_UNTYPED_WINDOW;
		}
		break;

	case B_MODAL_APP_WINDOW_FEEL:
		if (look == B_MODAL_WINDOW_LOOK)
			returnValue = B_MODAL_WINDOW;
		break;

	case B_FLOATING_APP_WINDOW_FEEL:
		if (look == B_FLOATING_WINDOW_LOOK)
			returnValue = B_FLOATING_WINDOW;
		break;

	default:
		returnValue = B_UNTYPED_WINDOW;
	}
	
	return returnValue;
}

//------------------------------------------------------------------------------

void BWindow::decomposeType(window_type type, 
							   window_look* look,
							   window_feel* feel) const
{
	switch (type)
	{
	case B_TITLED_WINDOW:
		*look = B_TITLED_WINDOW_LOOK;
		*feel = B_NORMAL_WINDOW_FEEL;
		break;
	case B_DOCUMENT_WINDOW:
		*look = B_DOCUMENT_WINDOW_LOOK;
		*feel = B_NORMAL_WINDOW_FEEL;
		break;
	case B_MODAL_WINDOW:
		*look = B_MODAL_WINDOW_LOOK;
		*feel = B_MODAL_APP_WINDOW_FEEL;
		break;
	case B_FLOATING_WINDOW:
		*look = B_FLOATING_WINDOW_LOOK;
		*feel = B_FLOATING_APP_WINDOW_FEEL;
		break;
	case B_BORDERED_WINDOW:
		*look = B_BORDERED_WINDOW_LOOK;
		*feel = B_NORMAL_WINDOW_FEEL;

	case B_UNTYPED_WINDOW:
		*look = B_TITLED_WINDOW_LOOK;
		*feel = B_NORMAL_WINDOW_FEEL;

	default:
		*look = B_TITLED_WINDOW_LOOK;
		*feel = B_NORMAL_WINDOW_FEEL;
	}
}

//------------------------------------------------------------------------------

uint32 BWindow::WindowLookToInteger(window_look wl)
{
	switch(wl)
	{
		case B_BORDERED_WINDOW_LOOK:
			return 1;
		case B_TITLED_WINDOW_LOOK:
			return 2;
		case B_DOCUMENT_WINDOW_LOOK:
			return 3;
		case B_MODAL_WINDOW_LOOK:
			return 4;
		case B_FLOATING_WINDOW_LOOK:
			return 5;
		case B_NO_BORDER_WINDOW_LOOK:
		default:
			return 0;
	}
}

//------------------------------------------------------------------------------

uint32 BWindow::WindowFeelToInteger(window_feel wf)
{
	switch(wf)
	{
		case B_MODAL_SUBSET_WINDOW_FEEL:
			return 1;
		case B_MODAL_APP_WINDOW_FEEL:
			return 2;
		case B_MODAL_ALL_WINDOW_FEEL:
			return 3;
		case B_FLOATING_SUBSET_WINDOW_FEEL:
			return 4;
		case B_FLOATING_APP_WINDOW_FEEL:
			return 5;
		case B_FLOATING_ALL_WINDOW_FEEL:
			return 6;

		case B_NORMAL_WINDOW_FEEL:
		default:
			return 0;
	}
}

//------------------------------------------------------------------------------

BView* BWindow::buildTopView(){
	BView			*topView;

	topView					= new BView( fFrame.OffsetToCopy(0,0), "top_view",
										 B_FOLLOW_ALL, B_WILL_DRAW);
	topView->owner			= this;
	topView->attached		= true;
	topView->top_level_view	= true;

// TODO: to be changed later after IK Team agrees on server tokening
		// why do we need this memeber? for cacheing?
	fTopViewToken	= topView->server_token;	

/*
Note:
		I don't think setting BView::parent is a good idea!
		Same goes for adding top_view to BLooper's list of eligible handlers.
		I'll have to think about those!
*/

// TODO: other initializations

	return topView;
}

//------------------------------------------------------------------------------

void BWindow::attachTopView(){

// TODO: implement after you have a messaging protocol with app_server

/*
	serverLink->SetOpCode( AS_ROOT_LAYER );
	serverLink->Attach( ... );
	serverLink->Flush();
*/
}

//------------------------------------------------------------------------------

void BWindow::detachTopView(){

	RemoveChild( top_view );

	serverLink->SetOpCode( AS_QUIT_WINDOW );

// TODO: wait until IK Team decides about server tokening
	//serverLink->Attach( &server_token, sizeof(int32) );

	Lock();
	serverLink->Flush( );
	Unlock();
}

//------------------------------------------------------------------------------

void BWindow::setFocus(BView *focusView, bool notifyInputServer){

	fFocus			= NULL;
	fFocus->Draw( fFocus->Bounds() );

	fFocus			= focusView;
	fFocus->Draw( fFocus->Bounds() );

// TODO: find out why do we have to notify input server.
	if (notifyInputServer){
		// what am I suppose to do here??
	}
}

//------------------------------------------------------------------------------

bool BWindow::handleBWindowKeys( BString string, uint32 modifiers){

// TODO: Handle key combinations like: Command+q, Command+c,...

// TODO: Handle shortcuts

// TODO: if we have a defalut button, send him a message when the user hits the Enter key
/*
Note: I might be doing this by constructing a BMessage with the msg->what=B_MOUSE_DOWN
		then send it to BWindow::MessageReceived( BMessage* ); Don't forget about the
		server token associated with this BButton!!!
*/
	return true;
}

//------------------------------------------------------------------------------

	// messages: B_MOUSE_UP, B_MOUSE_DOWN, B_MOUSE_MOVED
void BWindow::sendMessageUsingEventMask( int32 message, BPoint where ){

// TODO: Add code for Event Masks

// TODO: change after IK Team decides about server tokening; in the mean time...
	switch( message ){
		case B_MOUSE_DOWN:{
			BView		*destView;

			destView	= FindView( where );
			if (!destView)
				return;

			destView->MouseDown( destView->ConvertFromScreen( where ) );
			break;}

		case B_MOUSE_UP:{
			BView		*destView;

			destView	= FindView( where );
			if (!destView)
				return;

			destView->MouseUp( destView->ConvertFromScreen( where ) );
			break;}

		case B_MOUSE_MOVED:{
			BView		*destView;
			BMessage	*dragMessage;

			destView	= FindView( where );
			if (!destView)
				return;

				// for now...
			dragMessage	= NULL;

			if (destView != fLastMouseMovedView){
				fLastMouseMovedView->MouseMoved( destView->ConvertFromScreen( where ), B_EXITED_VIEW , dragMessage);
				destView->MouseMoved( destView->ConvertFromScreen( where ), B_ENTERED_VIEW, dragMessage);
				fLastMouseMovedView		= destView;
			}
			else{
				destView->MouseMoved( destView->ConvertFromScreen( where ), B_INSIDE_VIEW , dragMessage);
			}

				// I'm guessing that B_OUTSIDE_VIEW is given to the view that has focus, I'll have to check
				// Do a research on mouse capturing, maybe it has something to do with this
			fFocus->MouseMoved( destView->ConvertFromScreen( where ), B_OUTSIDE_VIEW , dragMessage);
			break;}
	}
}

//------------------------------------------------------------------------------

void BWindow::sendWillingBViewsPulseMsg( BView* aView ){
	BView *child; 
	if ( child = aView->ChildAt(0) ){
		while ( child ) { 
			if ( child->Flags() | B_PULSE_NEEDED ) child->Pulse();
			sendWillingBViewsPulseMsg( child );
			child = child->NextSibling(); 
		}
	}
}

//------------------------------------------------------------------------------

BMessage* BWindow::ReadMessageFromPort(bigtime_t tout)
{

// TODO: see what you must do here! Talk to Erik!
	return NULL;
}

//------------------------------------------------------------------------------

BMessage* BWindow::ConvertToMessage(void* raw1, int32 code){

	BMessage	*msg;

		// This is in case we receive a BMessage from another thread or application
	msg			= BLooper::ConvertToMessage( raw1, code );
	if (msg)
		return msg;

		// (ALL)This is in case we receive a message from app_server
	uint8		*raw;
	raw			= (uint8*)raw1;

// TODO: use an uint32 from BWindow's members, as the current server token.

	int32		serverToken;
	int64		when;	// time since 01/01/70

	msg			= new BMessage();

	switch(code){
		case B_WINDOW_ACTIVATED:{
			bool		active;

			when		= *((int64*)raw);	raw += sizeof(int64);
			active		= *((bool*)raw);

			msg->what	= B_WINDOW_ACTIVATED;
			msg->AddInt64("when", when);
			msg->AddBool("active", active);

			break;}

		case B_QUIT_REQUESTED:{

			msg->what	= B_QUIT_REQUESTED;
			msg->AddBool("shortcut", false);

			break;}

		case B_KEY_DOWN:{
			int32			physicalKeyCode,
							repeat,
							modifiers,
							ASCIIcode;
			char			*bytes;
			uint8			states;
			int8			UTF8_1, UTF8_2, UTF8_3;

			when			= *((int64*)raw);	raw += sizeof(int64);
			physicalKeyCode	= *((int32*)raw);	raw += sizeof(int32);
			repeat			= *((int32*)raw);	raw += sizeof(int32);
			modifiers		= *((int32*)raw);	raw += sizeof(int32);
			states			= *((uint8*)raw);	raw += sizeof(uint8);
			UTF8_1			= *((int8*)raw);	raw += sizeof(int8);
			UTF8_2			= *((int8*)raw);	raw += sizeof(int8);
			UTF8_3			= *((int8*)raw);	raw += sizeof(int8);
			ASCIIcode		= *((int32*)raw);	raw += sizeof(int32);

			int32			length;
			length			= strlen( (char*)raw );
			bytes			= new char[length+1];
			strcpy( bytes, (char*)raw );


			msg->what		= B_KEY_DOWN;
			msg->AddInt64("when", when);
			msg->AddInt32("key", physicalKeyCode);
			msg->AddInt32("be:key_repeat", repeat);
			msg->AddInt32("modifiers", modifiers);
			msg->AddInt8("states", states);
			msg->AddInt8("byte", UTF8_1);
			msg->AddInt8("byte", UTF8_2);
			msg->AddInt8("byte", UTF8_3);
			msg->AddInt32("raw_char", ASCIIcode);
			msg->AddString("bytes", bytes);

			break;}

		case B_KEY_UP:{
			int32			physicalKeyCode,
							modifiers,
							ASCIIcode;
			char			*bytes;
			uint8			states;
			int8			UTF8_1, UTF8_2, UTF8_3;

			when			= *((int64*)raw);	raw += sizeof(int64);
			physicalKeyCode	= *((int32*)raw);	raw += sizeof(int32);
			modifiers		= *((int32*)raw);	raw += sizeof(int32);
			states			= *((uint8*)raw);	raw += sizeof(uint8);
			UTF8_1			= *((int8*)raw);	raw += sizeof(int8);
			UTF8_2			= *((int8*)raw);	raw += sizeof(int8);
			UTF8_3			= *((int8*)raw);	raw += sizeof(int8);
			ASCIIcode		= *((int32*)raw);	raw += sizeof(int32);

			int32			length;
			length			= strlen( (char*)raw );
			bytes			= new char[length+1];
			strcpy( bytes, (char*)raw );


			msg->what		= B_KEY_UP;
			msg->AddInt64("when", when);
			msg->AddInt32("key", physicalKeyCode);
			msg->AddInt32("modifiers", modifiers);
			msg->AddInt8("states", states);
			msg->AddInt8("byte", UTF8_1);
			msg->AddInt8("byte", UTF8_2);
			msg->AddInt8("byte", UTF8_3);
			msg->AddInt32("raw_char", ASCIIcode);
			msg->AddString("bytes", bytes);

			break;}

		case B_UNMAPPED_KEY_DOWN:{
			int32			physicalKeyCode,
							modifiers;
			uint8			states;

			when			= *((int64*)raw);	raw += sizeof(int64);
			physicalKeyCode	= *((int32*)raw);	raw += sizeof(int32);
			modifiers		= *((int32*)raw);	raw += sizeof(int32);
			states			= *((uint8*)raw);

			msg->what		= B_UNMAPPED_KEY_DOWN;
			msg->AddInt64("when", when);
			msg->AddInt32("key", physicalKeyCode);
			msg->AddInt32("modifiers", modifiers);
			msg->AddInt8("states", states);

			break;}

		case B_UNMAPPED_KEY_UP:{
			int32			physicalKeyCode,
							modifiers;
			uint8			states;

			when			= *((int64*)raw);	raw += sizeof(int64);
			physicalKeyCode	= *((int32*)raw);	raw += sizeof(int32);
			modifiers		= *((int32*)raw);	raw += sizeof(int32);
			states			= *((uint8*)raw);

			msg->what		= B_UNMAPPED_KEY_UP;
			msg->AddInt64("when", when);
			msg->AddInt32("key", physicalKeyCode);
			msg->AddInt32("modifiers", modifiers);
			msg->AddInt8("states", states);

			break;}

		case B_MODIFIERS_CHANGED:{
			int32			modifiers,
							modifiersOld;
			uint8			states;

			when			= *((int64*)raw);	raw += sizeof(int64);
			modifiers		= *((int32*)raw);	raw += sizeof(int32);
			modifiersOld	= *((int32*)raw);	raw += sizeof(int32);
			states			= *((uint8*)raw);

			msg->what		= B_MODIFIERS_CHANGED;
			msg->AddInt64("when", when);
			msg->AddInt32("modifiers", modifiers);
			msg->AddInt32("be:old_modifiers", modifiersOld);
			msg->AddInt8("states", states);

			break;}

		case B_MINIMIZE:{
			bool			minimize;

			when			= *((int64*)raw);	raw += sizeof(int64);
			minimize		= *((bool*)raw);

			msg->what		= B_MINIMIZE;
			msg->AddInt64("when", when);
			msg->AddBool("minimize", minimize);

			break;}

		case B_MOUSE_DOWN:{
			float			mouseLocationX,
							mouseLocationY;
			int32			modifiers,
							buttons,
							noOfClicks;
			BPoint			where;

			when			= *((int64*)raw);	raw += sizeof(int64);
			serverToken		= *((int32*)raw);	raw += sizeof(int32);
			mouseLocationX	= *((float*)raw);	raw += sizeof(float);	// view's coordinate system
			mouseLocationY	= *((float*)raw);	raw += sizeof(float);	// view's coordinate system
			modifiers		= *((int32*)raw);	raw += sizeof(int32);
			buttons			= *((int32*)raw);	raw += sizeof(int32);
			noOfClicks		= *((int32*)raw);

			where.Set( mouseLocationX, mouseLocationY );

			msg->what		= B_MOUSE_DOWN;
			msg->AddInt64("when", when);
			msg->AddPoint("where", where);
			msg->AddInt32("modifiers", modifiers);
			msg->AddInt32("buttons", buttons);
			msg->AddInt32("clicks", noOfClicks);

			break;}

		case B_MOUSE_MOVED:{
			float			mouseLocationX,
							mouseLocationY;
			int32			buttons;
			int32			modifiers;		// added by OBOS Team

			BPoint			where;

			when			= *((int64*)raw);	raw += sizeof(int64);
			serverToken		= *((int32*)raw);	raw += sizeof(int32);
			mouseLocationX	= *((float*)raw);	raw += sizeof(float);	// windows's coordinate system
			mouseLocationY	= *((float*)raw);	raw += sizeof(float);	// windows's coordinate system
			modifiers		= *((int32*)raw);	raw += sizeof(int32);	// added by OBOS Team
			buttons			= *((int32*)raw);

			where.Set( mouseLocationX, mouseLocationY );

			msg->what		= B_MOUSE_MOVED;
			msg->AddInt64("when", when);
			msg->AddPoint("where", where);
			msg->AddInt32("buttons", buttons);

			break;}

// TODO: check out documentation
		case B_MOUSE_ENTER_EXIT:{
				// is this its place???
			break;}

		case B_MOUSE_UP:{
			float			mouseLocationX,
							mouseLocationY;
			int32			modifiers;

// TODO: ask  IK Team if we should add a field with the states of mouse buttons
			BPoint			where;

			when			= *((int64*)raw);	raw += sizeof(int64);
			serverToken		= *((int32*)raw);	raw += sizeof(int32);
			mouseLocationX	= *((float*)raw);	raw += sizeof(float);	// view's coordinate system
			mouseLocationY	= *((float*)raw);	raw += sizeof(float);	// view's coordinate system
			modifiers		= *((int32*)raw);

			where.Set( mouseLocationX, mouseLocationY );

			msg->what		= B_MOUSE_UP;
			msg->AddInt64("when", when);
			msg->AddPoint("where", where);
			msg->AddInt32("modifiers", modifiers);

			break;}

		case B_MOUSE_WHEEL_CHANGED:{
			float			whellChangeX,
							whellChangeY;

			when			= *((int64*)raw);	raw += sizeof(int64);
			whellChangeY	= *((float*)raw);	raw += sizeof(float);
			whellChangeY	= *((float*)raw);

			msg->what		= B_MOUSE_WHEEL_CHANGED;
			msg->AddInt64("when", when);
			msg->AddFloat("be:wheel_delta_x", whellChangeX);
			msg->AddFloat("be:wheel_delta_y", whellChangeY);

			break;}

/*		case B_PULSE:{

			msg->what		= B_PULSE;

			break;}
*/

// TODO: find out what there 2 messages do, and where they come form.
		case B_RELEASE_OVERLAY_LOCK:{	//
			break;}						//	are these for BWindow??? ...I... think so...
		case B_ACQUIRE_OVERLAY_LOCK:{	//
			break;}						//

		case B_SCREEN_CHANGED:{
			float			top,
							left,
							right,
							bottom;
			int32			colorSpace;
			BRect			frame;

			when			= *((int64*)raw);	raw += sizeof(int64);
			left			= *((float*)raw);	raw += sizeof(float);
			top				= *((float*)raw);	raw += sizeof(float);
			right			= *((float*)raw);	raw += sizeof(float);
			bottom			= *((float*)raw);	raw += sizeof(float);
			colorSpace		= *((int32*)raw);

			frame.Set( left, top, right, bottom );

			msg->what		= B_SCREEN_CHANGED;
			msg->AddInt64("when", when);
			msg->AddRect("frame", frame);
			msg->AddInt32("mode", colorSpace);

			break;}

		case B_VALUE_CHANGED:{
			int32			value;

			when			= *((int64*)raw);	raw += sizeof(int64);
			serverToken		= *((int32*)raw);	raw += sizeof(int32);
			value			= *((int32*)raw);

			msg->what		= B_VALUE_CHANGED;
			msg->AddInt64("when", when);
			msg->AddInt32("value", value);

			break;}

		case B_VIEW_MOVED:{
			float			xAxisNewOrigin,
							yAxisNewOrigin;
			BPoint			where;

			when			= *((int64*)raw);	raw += sizeof(int64);
			serverToken		= *((int32*)raw);	raw += sizeof(int32);
			xAxisNewOrigin	= *((float*)raw);	raw += sizeof(float);
			yAxisNewOrigin	= *((float*)raw);

			where.Set( xAxisNewOrigin, yAxisNewOrigin );

			msg->what		= B_VIEW_MOVED;
			msg->AddInt64("when", when);
			msg->AddPoint("where", where);
			
			break;}

		case B_VIEW_RESIZED:{
			int32			newWidth,
							newHeight;
			float			xAxisNewOrigin,
							yAxisNewOrigin;
			BPoint			where;

			when			= *((int64*)raw);	raw += sizeof(int64);
			serverToken		= *((int32*)raw);	raw += sizeof(int32);
			newWidth		= *((int32*)raw);	raw += sizeof(int32);
			newHeight		= *((int32*)raw);	raw += sizeof(int32);
			xAxisNewOrigin	= *((float*)raw);	raw += sizeof(float);
			yAxisNewOrigin	= *((float*)raw);

			where.Set( xAxisNewOrigin, yAxisNewOrigin );

			msg->what		= B_VIEW_RESIZED;
			msg->AddInt64("when", when);
			msg->AddInt32("width", newWidth);
			msg->AddInt32("height", newHeight);
			msg->AddPoint("where", where);		// it can always be ignored
												// you'll hear about it in a separate B_VIEW_MOVED

			break;}

		case B_WINDOW_MOVED:{
			float			xAxisNewOrigin,
							yAxisNewOrigin;
			BPoint			where;

			when			= *((int64*)raw);	raw += sizeof(int64);
			xAxisNewOrigin	= *((float*)raw);	raw += sizeof(float);
			yAxisNewOrigin	= *((float*)raw);

			where.Set( xAxisNewOrigin, yAxisNewOrigin );

			msg->what		= B_WINDOW_MOVED;
			msg->AddInt64("when", when);
			msg->AddPoint("where", where);

			break;}

		case B_WINDOW_RESIZED:{
			int32			newWidth,
							newHeight;
			BPoint			where;

			when			= *((int64*)raw);	raw += sizeof(int64);
			newWidth		= *((int32*)raw);	raw += sizeof(int32);
			newHeight		= *((int32*)raw);

			msg->what		= B_WINDOW_RESIZED;
			msg->AddInt64("when", when);
			msg->AddInt32("width", newWidth);
			msg->AddInt32("height", newHeight);

			break;}

		case B_WORKSPACES_CHANGED:{
			int32			newWorkSpace,
							oldWorkSpace;

			when			= *((int64*)raw);	raw += sizeof(int64);
			oldWorkSpace	= *((int32*)raw);	raw += sizeof(int32);
			newWorkSpace	= *((int32*)raw);

			msg->what		= B_WORKSPACES_CHANGED;
			msg->AddInt64("when", when);
			msg->AddInt32("old", oldWorkSpace);
			msg->AddInt32("new", newWorkSpace);

			break;}

		case B_WORKSPACE_ACTIVATED:{
			int32			workSpace;
			bool			active;

			when			= *((int64*)raw);	raw += sizeof(int64);
			workSpace		= *((int32*)raw);	raw += sizeof(int32);
			active			= *((bool*)raw);

			msg->what		= B_WORKSPACE_ACTIVATED;
			msg->AddInt64("when", when);
			msg->AddInt32("workspace", workSpace);
			msg->AddBool("active", active);

			break;}

		case B_ZOOM:{

			when			= *((int64*)raw);

			msg->what		= B_ZOOM;
			msg->AddInt64("when", when);

			break;}

		default:{
			// there is no need for default, but, just in case...
			}
	}
	
	return msg;
}

//------------------------------------------------------------------------------

int32 BWindow::findShortcut( int32 key, int32 modifiers ){

// TODO: Optimize;

	int32			index,
					noOfItems;

	index			= -1;
	noOfItems		= accelList.CountItems();

	for ( int i = 0;  i < noOfItems; i++ ) {
		_BCmdKey*		tempCmdKey;

		tempCmdKey		= (_BCmdKey*)accelList.ItemAt(i);
		if (tempCmdKey->key == key && tempCmdKey->modifiers == modifiers){
			index		= i;
			break;
		}
	}
	
	return index;
}

//------------------------------------------------------------------------------

bool BWindow::findHandler( BView* start, BHandler* handler ) {

// TODO: Optimize;

	if (((BHandler*)start) == handler)
		return true;

	int32			noOfChildren;

	noOfChildren	= start->CountChildren();
	for ( int i = 0; i<noOfChildren; i++ ) {
		if ( findHandler( start->ChildAt(i), handler ) )
			return true;
	}

	return false;
}

//------------------------------------------------------------------------------

BView* BWindow::findView(BView* aView, int32 token){

// TODO: Optimize;

	if ( aView->server_token == token )
		return aView;

	int32			noOfChildren;

	noOfChildren	= aView->CountChildren();
	for ( int i = 0; i<noOfChildren; i++ ) {
		BView*		view;
		if ( (view = findView( aView->ChildAt(i), token )) )
			return view;
	}

	return NULL;
}

//------------------------------------------------------------------------------

BView* BWindow::findView(BView* aView, const char* viewName) const{

// TODO: Optimize;

	if ( strcmp( viewName, aView->Name() ) == 0)		// check!
		return aView;

	int32			noOfChildren;

	noOfChildren	= aView->CountChildren();
	for ( int i = 0; i<noOfChildren; i++ ) {
		BView*		view;
		if ( (view = findView( aView->ChildAt(i), viewName )) )
			return view;
	}

	return NULL;
}

//------------------------------------------------------------------------------

BView* BWindow::findView(BView* aView, BPoint point) const{

// TODO: Optimize;

	int32			noOfChildren;
	noOfChildren	= aView->CountChildren();

	if ( aView->Bounds().Contains(point) && noOfChildren == 0 )
		return aView;

	for ( int i = 0; i<noOfChildren; i++ ) {
		BView*		view;
		if ( (view = findView( aView->ChildAt(i), point )) )
			return view;
	}

	return NULL;
}

//------------------------------------------------------------------------------

void BWindow::drawAllViews(BView* aView){

// TODO: Optimize;

	aView->Draw( fFocus->Bounds() );

	int32			noOfChildren;

	noOfChildren	= aView->CountChildren();
	for ( int i = 0; i<noOfChildren; i++ )
		drawAllViews( aView->ChildAt(i) );
}

//------------------------------------------------------------------------------

void BWindow::SetIsFilePanel(bool yes){

// TODO: I think this is not enough
	fIsFilePanel	= yes;
}

//------------------------------------------------------------------------------

bool BWindow::IsFilePanel() const{
	return fIsFilePanel;
}


//------------------------------------------------------------------------------
// Virtual reserved Functions

void BWindow::_ReservedWindow1() { }
//------------------------------------------------------------------------------
void BWindow::_ReservedWindow2() { }
//------------------------------------------------------------------------------
void BWindow::_ReservedWindow3() { }
//------------------------------------------------------------------------------
void BWindow::_ReservedWindow4() { }
//------------------------------------------------------------------------------
void BWindow::_ReservedWindow5() { }
//------------------------------------------------------------------------------
void BWindow::_ReservedWindow6() { }
//------------------------------------------------------------------------------
void BWindow::_ReservedWindow7() { }
//------------------------------------------------------------------------------
void BWindow::_ReservedWindow8() { }

/*
BWindow::Error(char *errMsg){
	printf("Error: %s\n", errMsg);
}
*/

/* ATTENTION
	1) Currently, FrameMoved() is also called when a hidden window is shown on-screen. 
 

*/

/*
TODO list:

	1) add code for menus
	2) at B_KEY_DOWN check shortcuts
	3) talk to Erik about BLooper, how can I still use BLooper::task_looper(), and
		still read messages sent by app_server. If I override task_looper(), I would
		have to implement BLooper facilities beside my code.
*/
