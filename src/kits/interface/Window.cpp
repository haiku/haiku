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
//	Author:			Adrian Oanca (adioanca@mymail.ro)
//	Description:	A BWindow object represents a window that can be displayed
//					on the screen, and that can be the target of user events
//------------------------------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <BeBuild.h>
#include <InterfaceDefs.h>
#include <PropertyInfo.h>
#include <Handler.h>
#include <Looper.h>
#include <Application.h>
#include <Window.h>
#include <View.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <String.h>
#include <Screen.h>
#include <Button.h>
#include <MessageQueue.h>
#include <MessageRunner.h>
#include <Roster.h>

// Project Includes ------------------------------------------------------------
#include <AppMisc.h>
#include <PortLink.h>
#include <ServerProtocol.h>
#include <TokenSpace.h>
#include <MessageUtils.h>
#include <WindowAux.h>

// Standard Includes -----------------------------------------------------------
#include <stdio.h>
#include <math.h>

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------
//#define DEBUG_WIN
#ifdef DEBUG_WIN
#	include <stdio.h>
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif

// Globals ---------------------------------------------------------------------
using BPrivate::gDefaultTokens;

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
void _set_menu_sem_(BWindow *window, sem_id sem)
{
	window->fMenuSem=sem;
}

// Constructors
//------------------------------------------------------------------------------
BWindow::BWindow(BRect frame, const char* title, window_type type,
	uint32 flags,uint32 workspace)
	: BLooper( title )
{
	#ifdef DEBUG_WIN
		printf("BWindow::BWindow()\n");
	#endif
	window_look look;
	window_feel feel;
	
	decomposeType(type, &look, &feel);
	
	InitData( frame, title, look, feel, flags, workspace);
}

//------------------------------------------------------------------------------

BWindow::BWindow(BRect frame, const char* title, window_look look, window_feel feel,
	uint32 flags,uint32 workspace)
	: BLooper( title )
{
	InitData( frame, title, look, feel, flags, workspace );
}

//------------------------------------------------------------------------------

BWindow::BWindow(BMessage* data)
	: BLooper(data)
{
	BMessage		msg;
	BArchivable		*obj;
	const char		*title;
	window_look		look;
	window_feel		feel;
	uint32			type;
	uint32			workspaces;

	data->FindRect("_frame", &fFrame);
	data->FindString("_title", &title);
	data->FindInt32("_wlook", (int32*)&look);
	data->FindInt32("_wfeel", (int32*)&feel);
	if ( data->FindInt32("_flags", (int32*)&fFlags) == B_OK )
		{ }
	else
		fFlags		= 0;
	data->FindInt32("_wspace", (int32*)&workspaces);

	if ( data->FindInt32("_type", (int32*)&type) == B_OK ){
		decomposeType( (window_type)type, &fLook, &fFeel );
	}

		// connect to app_server and initialize data
	InitData( fFrame, title, look, feel, fFlags, workspaces );

	if ( data->FindFloat("_zoom", 0, &fMaxZoomWidth) == B_OK )
		if ( data->FindFloat("_zoom", 1, &fMaxZoomHeight) == B_OK)
			SetZoomLimits( fMaxZoomWidth, fMaxZoomHeight );

	if (data->FindFloat("_sizel", 0, &fMinWindWidth) == B_OK )
		if (data->FindFloat("_sizel", 1, &fMinWindHeight) == B_OK )
			if (data->FindFloat("_sizel", 2, &fMaxWindWidth) == B_OK )
				if (data->FindFloat("_sizel", 3, &fMaxWindHeight) == B_OK )
					SetSizeLimits(	fMinWindWidth, fMaxWindWidth,
									fMinWindHeight, fMaxWindHeight );

	if (data->FindInt64("_pulse", &fPulseRate) == B_OK )
		SetPulseRate( fPulseRate );

	int				i = 0;
	while ( data->FindMessage("_views", i++, &msg) == B_OK){ 
		obj			= instantiate_object(&msg);

		BView		*child;
		child		= dynamic_cast<BView *>(obj);
		if (child)
			AddChild( child ); 
	}
}

//------------------------------------------------------------------------------

BWindow::~BWindow()
{
	// the following lines, remove all existing shortcuts and delete accelList
	int32			noOfItems;
	
	noOfItems		= accelList.CountItems();
	for ( int index = noOfItems-1;  index >= 0; index-- ) {
		_BCmdKey		*cmdKey;

		cmdKey			= (_BCmdKey*)accelList.ItemAt( index );
		
		accelList.RemoveItem(index);
		
		delete cmdKey->message;
		delete cmdKey;
	}

	// TODO: release other dynamically-allocated objects

	// disable pulsing
	SetPulseRate( 0 );

	delete fLink;
	delete_port( receive_port );
}

//------------------------------------------------------------------------------

BArchivable* BWindow::Instantiate(BMessage* data)
{
	if ( !validate_instantiation( data , "BWindow" ) ) 
		return NULL; 
	
	return new BWindow(data); 
}

//------------------------------------------------------------------------------

status_t BWindow::Archive(BMessage* data, bool deep) const{

	status_t		retval;

	retval		= BLooper::Archive( data, deep );
	if (retval != B_OK)
		return retval;

	data->AddRect("_frame", fFrame);
	data->AddString("_title", fTitle);
	data->AddInt32("_wlook", fLook);
	data->AddInt32("_wfeel", fFeel);
	if (fFlags)
		data->AddInt32("_flags", fFlags);
	data->AddInt32("_wspace", (uint32)Workspaces());

	if ( !composeType(fLook, fFeel) )
		data->AddInt32("_type", (uint32)Type());

	if (fMaxZoomWidth != 32768.0 || fMaxZoomHeight != 32768.0)
	{
		data->AddFloat("_zoom", fMaxZoomWidth);
		data->AddFloat("_zoom", fMaxZoomHeight);
	}

	if (fMinWindWidth != 0.0	 || fMinWindHeight != 0.0 ||
		fMaxWindWidth != 32768.0 || fMaxWindHeight != 32768.0)
	{
		data->AddFloat("_sizel", fMinWindWidth);
		data->AddFloat("_sizel", fMinWindHeight);
		data->AddFloat("_sizel", fMaxWindWidth);
		data->AddFloat("_sizel", fMaxWindHeight);
	}

	if (fPulseRate != 500000)
		data->AddInt64("_pulse", fPulseRate);

	if (deep)
	{
		int32		noOfViews = CountChildren();
		for (int i=0; i<noOfViews; i++){
			BMessage		childArchive;

			retval			= ChildAt(i)->Archive( &childArchive, deep );
			if (retval == B_OK)
				data->AddMessage( "_views", &childArchive );
		}
	}

	return B_OK;
}

//------------------------------------------------------------------------------

void BWindow::Quit(){

	if (!IsLocked())
	{
		const char* name = Name();
		if (!name)
			name = "no-name";

		printf("ERROR - you must Lock a looper before calling Quit(), "
			   "team=%ld, looper=%s\n", Team(), name);
	}

		// Try to lock
	if (!Lock()){
			// We're toast already
		return;
	}

	while (!IsHidden())	{ Hide(); }

		// ... also its children
	//detachTopView();
STRACE(("Trying to stop connection...\n"));
		// tell app_server, this window will finish execution
	stopConnection();
STRACE(("Connection stopped!\n"));
	if (fFlags & B_QUIT_ON_WINDOW_CLOSE)
		be_app->PostMessage( B_QUIT_REQUESTED );

	BLooper::Quit();
}

//------------------------------------------------------------------------------

void BWindow::AddChild(BView *child, BView *before){
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

	Lock();
	fLink->StartMessage( AS_WINDOW_MINIMIZE );
	fLink->Attach<bool>( minimize );
	fLink->Flush();
	Unlock();
}
//------------------------------------------------------------------------------

status_t BWindow::SendBehind(const BWindow* window){

	if (!window)
		return B_ERROR;
	
	int32		rCode;

	Lock();
	fLink->StartMessage( AS_SEND_BEHIND );
	fLink->Attach<int32>( _get_object_token_(window) );	
	fLink->Flush();
	fLink->GetNextReply( &rCode );
	Unlock();
	
	return rCode == SERVER_TRUE? B_OK : B_ERROR;
}

//------------------------------------------------------------------------------

void BWindow::Flush() const{
	const_cast<BWindow*>(this)->Lock();
	fLink->Flush();
	const_cast<BWindow*>(this)->Unlock();
}

//------------------------------------------------------------------------------

void BWindow::Sync() const{

	int32		rCode;

	const_cast<BWindow*>(this)->Lock();	
	fLink->StartMessage( AS_SYNC );
	fLink->Flush();
	fLink->GetNextReply( &rCode );
	const_cast<BWindow*>(this)->Unlock();
}

//------------------------------------------------------------------------------

void BWindow::DisableUpdates()
{
	Lock();	
	fLink->StartMessage( AS_DISABLE_UPDATES );
	fLink->Flush();
	Unlock();
}

//------------------------------------------------------------------------------

void BWindow::EnableUpdates()
{
	Lock();	
	fLink->StartMessage( AS_ENABLE_UPDATES );
	fLink->Flush();
	Unlock();
}

//------------------------------------------------------------------------------

void BWindow::BeginViewTransaction()
{
	if ( !fInTransaction )
	{
		Lock();
		fLink->StartMessage( AS_BEGIN_TRANSACTION );
		Unlock();
		
		fInTransaction		= true;
	}
}

//------------------------------------------------------------------------------

void BWindow::EndViewTransaction()
{
	if ( fInTransaction )
	{
		Lock();
		fLink->StartMessage( AS_END_TRANSACTION );
		fLink->Flush();
		Unlock();
		
		fInTransaction		= false;		
	}
}

//------------------------------------------------------------------------------

bool BWindow::IsFront() const
{
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

	if (msg->HasSpecifiers()){

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
				if (strcmp(prop, "Feel") ==0 )
				{
					uint32			newFeel;
					if (msg->FindInt32( "data", (int32*)&newFeel ) == B_OK){
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
					uint32			newFlags;
					if (msg->FindInt32( "data", (int32*)&newFlags ) == B_OK){
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
					uint32			newLook;
					if (msg->FindInt32( "data", (int32*)&newLook ) == B_OK){
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
					const char		*newTitle = NULL;
					if (msg->FindString( "data", &newTitle ) == B_OK){
						SetTitle( newTitle );
						
						replyMsg.what		= B_NO_ERROR;
						replyMsg.AddInt32( "error", B_OK );
					}
					else{
						replyMsg.what		= B_MESSAGE_NOT_UNDERSTOOD;
						replyMsg.AddInt32( "error", B_BAD_SCRIPT_SYNTAX );
						replyMsg.AddString( "message", "Didn't understand the specifier(s)" );
					}
				}
				
				else if (strcmp(prop, "Workspaces") ==0 )
				{
					uint32			newWorkspaces;
					if (msg->FindInt32( "data", (int32*)&newWorkspaces ) == B_OK){
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

	} // END: if (msg->HasSpecifiers())
	else
		BLooper::MessageReceived( msg );
} 

//------------------------------------------------------------------------------

void BWindow::DispatchMessage(BMessage *msg, BHandler *target) 
{
	if (!msg)
		return;

	switch ( msg->what )
	{ 
		case B_ZOOM:
		{
			Zoom();
			break;
		}
		case B_MINIMIZE:
		{
			bool			minimize;
	
			msg->FindBool("minimize", &minimize);
	
			fMinimized		= minimize;
			Minimize( minimize );
			break;
		}
		case B_WINDOW_RESIZED:
		{
			float			width, height;
	
			msg->FindFloat("width", &width);
			msg->FindFloat("height", &height);

			fFrame.right	= fFrame.left + width;
			fFrame.bottom	= fFrame.top + height;

			FrameResized(width,height);
			break;
		}
		case B_WINDOW_MOVED:
		{
			BPoint			origin;
	
			msg->FindPoint("where", &origin);

			fFrame.OffsetTo(origin);

			FrameMoved( origin );
			break;
		}
		
		// this is NOT an app_server message and we have to be cautious
		case B_WINDOW_MOVE_BY:
		{
			BPoint			offset;
			
			if (msg->FindPoint("data", &offset) == B_OK)
				MoveBy( offset.x, offset.y );
			else
				msg->SendReply( B_MESSAGE_NOT_UNDERSTOOD );
			break;
		}
			// this is NOT an app_server message and we have to be cautious
		case B_WINDOW_MOVE_TO:
		{
			BPoint			origin;
			
			if (msg->FindPoint("data", &origin) == B_OK)
				MoveTo( origin );
			else
				msg->SendReply( B_MESSAGE_NOT_UNDERSTOOD );
			break;
		}
		case B_WINDOW_ACTIVATED:
		{
			bool			active;
			
			msg->FindBool("active", &active);
	
			fActive			= active; 
			handleActivation( active );
			break;
		}
		case B_SCREEN_CHANGED:
		{
			BRect			frame;
			uint32			mode;
	
			msg->FindRect("frame", &frame);
			msg->FindInt32("mode", (int32*)&mode);
			ScreenChanged( frame, (color_space)mode );
			break;
		}
		case B_WORKSPACE_ACTIVATED:
		{
			uint32			workspace;
			bool			active;
	
			msg->FindInt32( "workspace", (int32*)&workspace );
			msg->FindBool( "active", &active );
			WorkspaceActivated( workspace, active );
			break;
		}
		case B_WORKSPACES_CHANGED:
		{
			uint32			oldWorkspace;
			uint32			newWorkspace;
	
			msg->FindInt32( "old", (int32*)&oldWorkspace );
			msg->FindInt32( "new", (int32*)&newWorkspace );
			WorkspacesChanged( oldWorkspace, newWorkspace );
			break;
		}
		case B_KEY_DOWN:
		{
			uint32			modifiers;
			int32			raw_char;
			const char		*string=NULL;

			msg->FindInt32( "modifiers", (int32*)&modifiers );
			msg->FindInt32( "raw_char", &raw_char );
			msg->FindString( "bytes", &string );

// TODO: USE target !!!!
			if ( !handleKeyDown( string[0], (uint32)modifiers) )
			{
				if(fFocus)
					fFocus->KeyDown( string, strlen(string) );
				else
					printf("Adi: No Focus\n");
			}
			break;
		}
		case B_KEY_UP:
		{
			const char		*string=NULL;
	
			msg->FindString( "bytes", &string );

// TODO: USE target !!!!
			if(fFocus)
				fFocus->KeyUp( string, strlen(string) );

			break;
		}
		case B_UNMAPPED_KEY_DOWN:
		case B_UNMAPPED_KEY_UP:
		case B_MODIFIERS_CHANGED:
		{
			if (target != this && target != top_view)
				target->MessageReceived( msg );
			break;
		}
		case B_MOUSE_WHEEL_CHANGED:
		{
			if (target != this && target != top_view)
				target->MessageReceived( msg );
			break;
		}
		case B_MOUSE_DOWN:
		{
			BPoint			where;
			uint32			modifiers;
			uint32			buttons;
			int32			clicks;
			
			msg->FindPoint( "where", &where );
			msg->FindInt32( "modifiers", (int32*)&modifiers );
			msg->FindInt32( "buttons", (int32*)&buttons );
			msg->FindInt32( "clicks", &clicks );

			if (target && target != this && target != top_view)
			{
				((BView*)target)->ConvertFromScreen(&where);
				((BView*)target)->MouseDown(where);
			}

// TODO: use the following line later, instead of the above.
//			sendMessageUsingEventMask( B_MOUSE_DOWN, where );
			break;
		}
		case B_MOUSE_UP:
		{
			BPoint			where;
			uint32			modifiers;

			msg->FindPoint( "where", &where );
			msg->FindInt32( "modifiers", (int32*)&modifiers );

			if (target && target != this && target != top_view)
			{
				((BView*)target)->ConvertFromScreen(&where);
				((BView*)target)->MouseUp(where);
			}

// TODO: use the following line later, instead of the above.
//			sendMessageUsingEventMask( B_MOUSE_UP, where );
			break;
		}
		case B_MOUSE_MOVED:
		{
			BPoint			where;
			uint32			buttons;
			uint32			transit;
			
			msg->FindPoint( "where", &where );
			msg->FindInt32( "buttons", (int32*)&buttons );
			msg->FindInt32( "transit", (int32*)&transit );
			if (target && target != this && target != top_view)
			{
				((BView*)target)->ConvertFromScreen(&where);
				((BView*)target)->MouseMoved(where, transit, NULL);
			}
//			sendMessageUsingEventMask( B_MOUSE_MOVED, where );
			break;
		}
		case B_PULSE:
		{
			if (fPulseEnabled)
				sendPulse( top_view );
			break;
		}
		case B_QUIT_REQUESTED:
		{
			if (QuitRequested())
				Quit();
			break;
		}
		case _UPDATE_:
		{
			STRACE(("info:BWindow handling _UPDATE_.\n"));
			BRect updateRect;
			int32 token;
			
			msg->FindRect("_rect", &updateRect);
			msg->FindInt32("_token",&token);
			
			fLink->StartMessage(AS_BEGIN_UPDATE);
			DoUpdate(top_view, updateRect);
			fLink->StartMessage(AS_END_UPDATE);
			fLink->Flush();
			break;
		}
		case B_VIEW_RESIZED:
		case B_VIEW_MOVED:
		{
			// NOTE: The problem with this implementation is that BView::Window()->CurrentMessage()
			// will show this message, and not what it used to be on R5. This might break apps and
			// we need to fix this here or change the way this feature is implemented. However, this
			// implementation shows what has to be done when Layers are moved or resized inside the
			// app_server. This message is generated from Layer::move_by() and resize_by() in
			// Layer::AddToViewsWithInvalidCoords().
			int32 token;
			BPoint frameLeftTop;
			float width;
			float height;
			BView* view;
			for (int32 i = 0; msg->FindInt32("_token", i, &token) >= B_OK; i++) {
				if (token >= 0) {
					msg->FindPoint("where", i, &frameLeftTop);
					msg->FindFloat("width", i, &width);
					msg->FindFloat("height", i, &height);
					if ((view = findView(top_view, token))) {
						// update the views offset in parent
						if (view->originX != frameLeftTop.x || view->originY != frameLeftTop.y) {
//printf("updating position (%.1f, %.1f): %s\n", frameLeftTop.x, frameLeftTop.y, view->Name());
							view->originX = frameLeftTop.x;
							view->originY = frameLeftTop.y;
							// optionally call FrameMoved
							if (view->fFlags & B_FRAME_EVENTS) {
								STRACE(("Calling BView(%s)::FrameMoved( %.1f, %.1f )\n", view->Name(),
										frameLeftTop.x, frameLeftTop.y));
								view->FrameMoved(frameLeftTop);
							}
						}
						// update the views width and height
						if (view->fBounds.Width() != width || view->fBounds.Height() != height) {
//printf("updating size (%.1f, %.1f): %s\n", width, height, view->Name());
							// TODO: does this work when a views left/top side is resized?
							view->fBounds.right = view->fBounds.left + width;
							view->fBounds.bottom = view->fBounds.top + height;
							// optionally call FrameResized
							if (view->fFlags & B_FRAME_EVENTS) {
								STRACE(("Calling BView(%s)::FrameResized( %f, %f )\n", view->Name(), width, height));
								view->FrameResized(width, height);
							}
						}
					} else {
						fprintf(stderr, "***PANIC: BW: Can't find view with ID: %ld !***\n", token);
					}
				}
			}
			break;
		}
/*		case B_VIEW_MOVED:
		{
			// NOTE: This message only arrives if the
			// view has flags B_FRAME_EVENTS
			BPoint			where;
			int32			token = B_NULL_TOKEN;
			BView			*view;
			
			msg->FindPoint("where", &where);
			msg->FindInt32("_token", &token);
			msg->RemoveName("_token");
				
			view			= findView(top_view, token);
			if (view)
			{
				STRACE(("Calling BView(%s)::FrameMoved( %f, %f )\n", view->Name(), where.x, where.y));
				view->FrameMoved( where );
			}
			else
				printf("***PANIC: BW: Can't find view with ID: %ld !***\n", token);
			
			break;
		}	
		case B_VIEW_RESIZED:
		{
			// NOTE: This message only arrives if the
			// view has flags B_FRAME_EVENTS
			float			newWidth,
							newHeight;
			BPoint			where;
			int32			token = B_NULL_TOKEN;
			BView			*view;
	
			msg->FindFloat("width", &newWidth);
			msg->FindFloat("height", &newHeight);
			msg->FindPoint("where", &where);
			msg->FindInt32("_token", &token);
			msg->RemoveName("_token");
				
			view			= findView(top_view, token);
			if (view){
				STRACE(("Calling BView(%s)::FrameResized( %f, %f )\n", view->Name(), newWidth, newHeight));
				view->FrameResized( newWidth, newHeight );
			}
			else
				printf("***PANIC: BW: Can't find view with ID: %ld !***\n", token);
			break;
		}
*/

		case _MENUS_DONE_:
			MenusEnded();
			break;
			
		default:
		{
			BLooper::DispatchMessage(msg, target); 
			break;
		}
	} // end switch(msg->what)
}

//------------------------------------------------------------------------------

void BWindow::FrameMoved(BPoint new_position)
{
	// does nothing
	// Hook function
}

//------------------------------------------------------------------------------

void BWindow::FrameResized(float new_width, float new_height)
{
	// does nothing
	// Hook function
}

//------------------------------------------------------------------------------

void BWindow::WorkspacesChanged(uint32 old_ws, uint32 new_ws)
{
	// does nothing
	// Hook function
}

//------------------------------------------------------------------------------

void BWindow::WorkspaceActivated(int32 ws, bool state)
{
	// does nothing
	// Hook function
}

//------------------------------------------------------------------------------

void BWindow::MenusBeginning()
{
	// does nothing
	// Hook function
}

//------------------------------------------------------------------------------

void BWindow::MenusEnded()
{
	// does nothing
	// Hook function
}

//------------------------------------------------------------------------------

void BWindow::SetSizeLimits(float minWidth, float maxWidth, 
							float minHeight, float maxHeight)
{
	int32		rCode;

	if (minWidth > maxWidth)
		return;
	if (minHeight > maxHeight)
		return;
		
	Lock();
	fLink->StartMessage( AS_SET_SIZE_LIMITS );
	fLink->Attach<float>( fMinWindWidth );
	fLink->Attach<float>( fMaxWindWidth );
	fLink->Attach<float>( fMinWindHeight );
	fLink->Attach<float>( fMaxWindHeight );
	fLink->Flush();
	fLink->GetNextReply( &rCode );
	Unlock();

	if (rCode == SERVER_TRUE){
		fMinWindHeight		= minHeight;
		fMinWindWidth		= minWidth;
		fMaxWindHeight		= maxHeight;
		fMaxWindWidth		= maxWidth;
	}
}

//------------------------------------------------------------------------------

void BWindow::GetSizeLimits(float *minWidth, float *maxWidth, 
							float *minHeight, float *maxHeight)
{
	*minHeight			= fMinWindHeight;
	*minWidth			= fMinWindWidth;
	*maxHeight			= fMaxWindHeight;
	*maxWidth			= fMaxWindWidth;
}

//------------------------------------------------------------------------------

void BWindow::SetZoomLimits(float maxWidth, float maxHeight)
{
	if (maxWidth > fMaxWindWidth)
		maxWidth	= fMaxWindWidth;
	else
		fMaxZoomWidth		= maxWidth;

	if (maxHeight > fMaxWindHeight)
		maxHeight	= fMaxWindHeight;
	else
		fMaxZoomHeight		= maxHeight;
}

//------------------------------------------------------------------------------

void BWindow::Zoom(	BPoint rec_position, float rec_width, float rec_height)
{

	// this is also a Hook function!
		
	MoveTo( rec_position );
	ResizeTo( rec_width, rec_height );
}

//------------------------------------------------------------------------------

void BWindow::Zoom()
{
	float			minWidth, minHeight;
	BScreen			screen;

/*
	from BeBook:
	However, if the window's rectangle already matches these "zoom" dimensions
	(give or take a few pixels), Zoom() passes the window's previous
	("non-zoomed") size and location. (??????)
*/

	if (Frame().Width() == fMaxZoomWidth && Frame().Height() == fMaxZoomHeight)
	{
		BPoint position( Frame().left, Frame().top);
		Zoom( position, fMaxZoomWidth, fMaxZoomHeight );
		return;
	}
	
/*
	from BeBook:
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

//------------------------------------------------------------------------------

void BWindow::ScreenChanged(BRect screen_size, color_space depth)
{
	// Hook function
	// does nothing
}

//------------------------------------------------------------------------------

void BWindow::SetPulseRate(bigtime_t rate)
{
	if ( rate < 0 )
		return;

	if (fPulseRate == 0 && !fPulseEnabled){
		fPulseRunner	= new BMessageRunner(	BMessenger(this),
												new BMessage( B_PULSE ),
												rate);
		fPulseRate		= rate;
		fPulseEnabled	= true;

		return;
	}

	if (rate == 0 && fPulseEnabled){
		delete			fPulseRunner;
		fPulseRunner	= NULL;

		fPulseRate		= rate;
		fPulseEnabled	= false;

		return;
	}

	fPulseRunner->SetInterval( rate );
}

//------------------------------------------------------------------------------

bigtime_t
BWindow::PulseRate() const
{
	return fPulseRate;
}


void
BWindow::AddShortcut(uint32 key, uint32 modifiers, BMenuItem *item)
{
	AddShortcut(key, modifiers, item->Message(), this);
}


void
BWindow::AddShortcut(uint32 key, uint32 modifiers, BMessage *msg)
{
	AddShortcut(key, modifiers, msg, this);
}


void
BWindow::AddShortcut(uint32 key, uint32 modifiers, BMessage *msg, BHandler *target)
{
	// NOTE: I'm not sure if it is OK to use 'key'
	
	if (msg == NULL)
		return;

	int64				when;
	_BCmdKey			*cmdKey;

	when				= real_time_clock_usecs();
	msg->AddInt64("when", when);

	// TODO: make sure key is a lowercase char !!!

	modifiers			= modifiers | B_COMMAND_KEY;

	cmdKey				= new _BCmdKey;
	cmdKey->key			= key;
	cmdKey->modifiers	= modifiers;
	cmdKey->message		= msg;
	if (target == NULL)
		cmdKey->targetToken	= B_ANY_TOKEN;
	else
		cmdKey->targetToken	= _get_object_token_(target);

	// removes the shortcut from accelList if it exists!
	RemoveShortcut( key, modifiers );

	accelList.AddItem( cmdKey );

}

//------------------------------------------------------------------------------

void BWindow::RemoveShortcut(uint32 key, uint32 modifiers)
{
	int32				index;
	
	modifiers			= modifiers | B_COMMAND_KEY;
	
	index				= findShortcut( key, modifiers );
	if ( index >=0 ) 
	{
		_BCmdKey		*cmdKey;
		
		cmdKey			= (_BCmdKey*)accelList.ItemAt( index );
		
		accelList.RemoveItem(index);
		
		delete cmdKey->message;
		delete cmdKey;
	}
}

//------------------------------------------------------------------------------

BButton* BWindow::DefaultButton() const
{
	return fDefaultButton;
}

//------------------------------------------------------------------------------

void BWindow::SetDefaultButton(BButton* button)
{
/*
	Note: for developers:
	
	He he, if you really want to understand what is happens here, take a piece of
		paper and start taking possible values and then walk with them through
		the code.
*/
	BButton				*aux;

	if ( fDefaultButton == button )
		return;

	if ( fDefaultButton )
	{
		aux				= fDefaultButton;
		fDefaultButton	= NULL;
		aux->MakeDefault( false );
		aux->Invalidate();
	}
	
	if ( button == NULL )
	{
		fDefaultButton		= NULL;
		return;
	}
	
	fDefaultButton			= button;
	fDefaultButton->MakeDefault( true );
	fDefaultButton->Invalidate();
}

//------------------------------------------------------------------------------

bool BWindow::NeedsUpdate() const
{

	int32		rCode;

	const_cast<BWindow*>(this)->Lock();	
	fLink->StartMessage( AS_NEEDS_UPDATE );
	fLink->Flush();
	fLink->GetNextReply( &rCode );
	const_cast<BWindow*>(this)->Unlock();
	
	return rCode == SERVER_TRUE;
}

//------------------------------------------------------------------------------

void BWindow::UpdateIfNeeded()
{
	// works only from this thread
	if (find_thread(NULL) != Thread())
		return;

	BMessageQueue *queue;
	BMessage *msg;
	
	queue = MessageQueue();

	//process all _UPDATE_ BMessages in message queue
	//we lock the queue because we don't want to be stuck in this
	//function if there is a continuous stream of updates 
	queue->Lock();
	while ((msg = queue->FindMessage(_UPDATE_, 0)))
	{
		Lock();
		DispatchMessage( msg, this );
		Unlock();

		queue->RemoveMessage( msg );
		delete msg;
	}
	queue->Unlock();
}

//------------------------------------------------------------------------------

BView* BWindow::FindView(const char* viewName) const
{

	return findView( top_view, viewName );
}

//------------------------------------------------------------------------------

BView* BWindow::FindView(BPoint point) const
{

	return findView( top_view, point );
}

//------------------------------------------------------------------------------

BView* BWindow::CurrentFocus() const
{
	return fFocus;
}

//------------------------------------------------------------------------------

void BWindow::Activate(bool active)
{
	if (IsHidden())
		return;

	Lock();
	fLink->StartMessage( AS_ACTIVATE_WINDOW );
	fLink->Attach<bool>( active );
	fLink->Flush();
	Unlock();
}

//------------------------------------------------------------------------------

void BWindow::WindowActivated(bool state)
{
	// hook function
	// does nothing
}

//------------------------------------------------------------------------------

void BWindow::ConvertToScreen(BPoint* pt) const
{
	pt->x			+= fFrame.left;
	pt->y			+= fFrame.top;
}

//------------------------------------------------------------------------------

BPoint BWindow::ConvertToScreen(BPoint pt) const
{
	pt.x			+= fFrame.left;
	pt.y			+= fFrame.top;

	return pt;
}

//------------------------------------------------------------------------------

void BWindow::ConvertFromScreen(BPoint* pt) const
{
	pt->x			-= fFrame.left;
	pt->y			-= fFrame.top;
}

//------------------------------------------------------------------------------

BPoint BWindow::ConvertFromScreen(BPoint pt) const
{
	pt.x			-= fFrame.left;
	pt.y			-= fFrame.top;

	return pt;
}

//------------------------------------------------------------------------------

void BWindow::ConvertToScreen(BRect* rect) const
{
	rect->top			+= fFrame.top;
	rect->left			+= fFrame.left;
	rect->bottom		+= fFrame.top;
	rect->right			+= fFrame.left;
}

//------------------------------------------------------------------------------

BRect BWindow::ConvertToScreen(BRect rect) const
{
	rect.top			+= fFrame.top;
	rect.left			+= fFrame.left;
	rect.bottom			+= fFrame.top;
	rect.right			+= fFrame.left;

	return rect;
}

//------------------------------------------------------------------------------

void BWindow::ConvertFromScreen(BRect* rect) const
{
	rect->top			-= fFrame.top;
	rect->left			-= fFrame.left;
	rect->bottom		-= fFrame.top;
	rect->right			-= fFrame.left;
}

//------------------------------------------------------------------------------

BRect BWindow::ConvertFromScreen(BRect rect) const
{
	rect.top			-= fFrame.top;
	rect.left			-= fFrame.left;
	rect.bottom			-= fFrame.top;
	rect.right			-= fFrame.left;

	return rect;
}

//------------------------------------------------------------------------------

bool BWindow::IsMinimized() const
{
	// Hiding takes precendence over minimization!!!
	if ( IsHidden() )
		return false;

	return fMinimized;
}

//------------------------------------------------------------------------------

BRect BWindow::Bounds() const
{
	BRect	bounds( 0.0, 0.0, fFrame.Width(), fFrame.Height() );
	return bounds;
}

//------------------------------------------------------------------------------

BRect BWindow::Frame() const
{
	return fFrame;
}

//------------------------------------------------------------------------------

const char* BWindow::Title() const
{
	return fTitle;
}

//------------------------------------------------------------------------------

void BWindow::SetTitle(const char* title)
{
	if (!title)
		return;

	if (fTitle)
	{
		free(fTitle);
		fTitle = NULL;
	}
	
	fTitle = strdup( title );

	// we will change BWindow's thread name to "w>window_title"	
	int32		length;
	length		= strlen( fTitle );
	
	char threadName[B_OS_NAME_LENGTH];
	strcpy(threadName, "w>");
	length=min_c(length,B_OS_NAME_LENGTH-3);
	strncat(threadName, fTitle, length);
	threadName[B_OS_NAME_LENGTH-1] = '\0';

	// if the message loop has been started...
	if (Thread() != B_ERROR )
	{
		SetName( threadName );
		rename_thread( Thread(), threadName );
		
		// we notify the app_server so we can actually see the change
		Lock();
		fLink->StartMessage( AS_WINDOW_TITLE);
		fLink->AttachString( fTitle );
		fLink->Flush();
		Unlock();
	}
	else
		SetName( threadName );
}

//------------------------------------------------------------------------------

bool BWindow::IsActive() const
{
	return fActive;
}

//------------------------------------------------------------------------------

void BWindow::SetKeyMenuBar(BMenuBar* bar)
{
	fKeyMenuBar			= bar;
}

//------------------------------------------------------------------------------

BMenuBar* BWindow::KeyMenuBar() const
{
	return fKeyMenuBar;
}

//------------------------------------------------------------------------------

bool BWindow::IsModal() const
{
	if ( fFeel == B_MODAL_SUBSET_WINDOW_FEEL)
		return true;
	if ( fFeel == B_MODAL_APP_WINDOW_FEEL)
		return true;
	if ( fFeel == B_MODAL_ALL_WINDOW_FEEL)
		return true;

	return false;

}

//------------------------------------------------------------------------------

bool BWindow::IsFloating() const
{
	if ( fFeel == B_FLOATING_SUBSET_WINDOW_FEEL)
		return true;
	if ( fFeel == B_FLOATING_APP_WINDOW_FEEL)
		return true;
	if ( fFeel == B_FLOATING_ALL_WINDOW_FEEL)
		return true;

	return false;
}

//------------------------------------------------------------------------------

status_t BWindow::AddToSubset(BWindow* window)
{
	if ( !window )
			return B_ERROR;
	
	int32		rCode;

	if (window->Feel() == B_NORMAL_WINDOW_FEEL &&
			(fFeel == B_MODAL_SUBSET_WINDOW_FEEL ||
			fFeel == B_FLOATING_SUBSET_WINDOW_FEEL) )
	{
		team_id		team = Team();
		
		Lock();
		fLink->StartMessage( AS_ADD_TO_SUBSET );
		fLink->Attach<int32>( _get_object_token_(window) );
		fLink->Attach<team_id>( team );
		fLink->Flush();
		fLink->GetNextReply( &rCode );
		Unlock();

		return rCode == SERVER_TRUE? B_OK : B_ERROR;
	}

	return B_ERROR;
}

//------------------------------------------------------------------------------

status_t BWindow::RemoveFromSubset(BWindow* window)
{
	if ( !window )
			return B_ERROR;
	
	int32		rCode;

	if (window->Feel() == B_NORMAL_WINDOW_FEEL &&
			(fFeel == B_MODAL_SUBSET_WINDOW_FEEL ||
				fFeel == B_FLOATING_SUBSET_WINDOW_FEEL)){

		team_id		team = Team();

		Lock();
		fLink->StartMessage( AS_REM_FROM_SUBSET );
		fLink->Attach<int32>( _get_object_token_(window) );
		fLink->Attach<team_id>( team );
		fLink->Flush();
		fLink->GetNextReply( &rCode );
		Unlock();

		return rCode == SERVER_TRUE? B_OK : B_ERROR;
	}

	return B_ERROR;
}

//------------------------------------------------------------------------------

status_t BWindow::Perform(perform_code d, void* arg)
{
	return BLooper::Perform( d, arg );
}

//------------------------------------------------------------------------------

status_t BWindow::SetType(window_type type)
{
	decomposeType(type, &fLook, &fFeel);
	status_t stat1, stat2;
	
	stat1=SetLook( fLook );
	stat2=SetFeel( fFeel );

	if(stat1==B_OK && stat2==B_OK)
		return B_OK;
	return B_ERROR;
}

//------------------------------------------------------------------------------

window_type	BWindow::Type() const
{
	return composeType( fLook, fFeel );
}

//------------------------------------------------------------------------------

status_t BWindow::SetLook(window_look look)
{

	int32		rCode;

	Lock();
	fLink->StartMessage( AS_SET_LOOK );
	fLink->Attach<int32>( (int32)look );
	fLink->Flush();
	fLink->GetNextReply( &rCode );
	Unlock();
	
	if (rCode == SERVER_TRUE)
	{
		fLook		= look;
		return B_OK;
	}
	
	return B_ERROR;
}

//------------------------------------------------------------------------------

window_look	BWindow::Look() const
{
	return fLook;
}

//------------------------------------------------------------------------------

status_t BWindow::SetFeel(window_feel feel)
{
	if (!(	feel == B_NORMAL_WINDOW_FEEL
			|| feel == B_MODAL_SUBSET_WINDOW_FEEL
			|| feel == B_MODAL_APP_WINDOW_FEEL
			|| feel == B_MODAL_ALL_WINDOW_FEEL
			|| feel == B_FLOATING_SUBSET_WINDOW_FEEL
			|| feel == B_FLOATING_APP_WINDOW_FEEL
			|| feel == B_FLOATING_ALL_WINDOW_FEEL))
		return B_ERROR;
	
	Lock();
	fLink->StartMessage( AS_SET_FEEL );
	fLink->Attach<int32>( (int32)feel );
	fLink->Flush();
	Unlock();
	
	fFeel		= feel;

	return B_OK;
}

//------------------------------------------------------------------------------

window_feel	BWindow::Feel() const
{
	return fFeel;
}

//------------------------------------------------------------------------------

status_t BWindow::SetFlags(uint32 flags)
{

	int32		rCode;

	Lock();	
	fLink->StartMessage( AS_SET_FLAGS );
	fLink->Attach<uint32>( flags );
	fLink->Flush();
	fLink->GetNextReply( &rCode );
	Unlock();
	
	if (rCode == SERVER_TRUE)
	{
		fFlags		= flags;
		return B_OK;
	}

	return B_ERROR;
}

//------------------------------------------------------------------------------

uint32	BWindow::Flags() const{
	return fFlags;
}

//------------------------------------------------------------------------------

status_t BWindow::SetWindowAlignment(window_alignment mode,
											int32 h, int32 hOffset,
											int32 width, int32 widthOffset,
											int32 v, int32 vOffset,
											int32 height, int32 heightOffset)
{
	if ( !(	(mode && B_BYTE_ALIGNMENT) ||
			(mode && B_PIXEL_ALIGNMENT) ) )
	{
		return B_ERROR;
	}

	if ( 0 <= hOffset && hOffset <=h )
		return B_ERROR;

	if ( 0 <= vOffset && vOffset <=v )
		return B_ERROR;

	if ( 0 <= widthOffset && widthOffset <=width )
		return B_ERROR;

	if ( 0 <= heightOffset && heightOffset <=height )
		return B_ERROR;

	// TODO: test if hOffset = 0 and set it to 1 if true.
	int32		rCode;

	Lock();
	fLink->StartMessage( AS_SET_ALIGNMENT );
	fLink->Attach<int32>( (int32)mode );
	fLink->Attach<int32>( h );
	fLink->Attach<int32>( hOffset );
	fLink->Attach<int32>( width );
	fLink->Attach<int32>( widthOffset );
	fLink->Attach<int32>( v );
	fLink->Attach<int32>( vOffset );
	fLink->Attach<int32>( height );
	fLink->Attach<int32>( heightOffset );
	fLink->Flush();
	fLink->GetNextReply( &rCode );
	Unlock();
	
	if ( rCode == SERVER_TRUE)
	{
		return B_NO_ERROR;
	}

	return B_ERROR;
}

//------------------------------------------------------------------------------

status_t BWindow::GetWindowAlignment(window_alignment* mode,
											int32* h, int32* hOffset,
											int32* width, int32* widthOffset,
											int32* v, int32* vOffset,
											int32* height, int32* heightOffset) const
{
	int32		rCode;

	const_cast<BWindow*>(this)->Lock();
	fLink->StartMessage( AS_GET_ALIGNMENT );
	fLink->Flush();
	fLink->GetNextReply( &rCode );
	
	if (rCode == SERVER_TRUE)
	{
		fLink->Read<int32>( (int32*)mode );
		fLink->Read<int32>( h );
		fLink->Read<int32>( hOffset );
		fLink->Read<int32>( width );
		fLink->Read<int32>( widthOffset );
		fLink->Read<int32>( v );
		fLink->Read<int32>( hOffset );
		fLink->Read<int32>( height );
		rCode=fLink->Read<int32>( heightOffset );		

		return B_NO_ERROR;
	}
	const_cast<BWindow*>(this)->Unlock();
	
	if(rCode!=B_OK)
		return B_ERROR;
	
	return B_OK;
}

//------------------------------------------------------------------------------

uint32 BWindow::Workspaces() const
{
	int32		rCode;
	uint32		workspaces;

	const_cast<BWindow*>(this)->Lock();
	fLink->StartMessage( AS_GET_WORKSPACES );
	fLink->Flush();
	fLink->GetNextReply(&rCode);
	fLink->Read<uint32>(&workspaces);
	const_cast<BWindow*>(this)->Unlock();
// TODO: shouldn't we cache?
	return workspaces;
}

//------------------------------------------------------------------------------

void BWindow::SetWorkspaces(uint32 workspaces)
{
// TODO: don't forget about Tracker's background window.
	if (fFeel != B_NORMAL_WINDOW_FEEL)
		return;

	Lock();
	fLink->StartMessage(AS_SET_WORKSPACES);
	fLink->Attach<uint32>(workspaces);
	fLink->Flush();
	Unlock();
}

//------------------------------------------------------------------------------

BView* BWindow::LastMouseMovedView() const
{
	return fLastMouseMovedView;
}

//------------------------------------------------------------------------------

void BWindow::MoveBy(float dx, float dy)
{
	if (dx != 0.0 || dy != 0.0) {

		Lock();

		fLink->StartMessage(AS_WINDOW_MOVE);
		fLink->Attach<float>(dx);
		fLink->Attach<float>(dy);
		fLink->Flush();
	
		fFrame.OffsetBy(dx, dy);

		Unlock();
	}
}

//------------------------------------------------------------------------------

void BWindow::MoveTo( BPoint point )
{
	Lock();
	
	if (fFrame.left != point.x || fFrame.top != point.y) {
	
		float xOffset = point.x - fFrame.left;
		float yOffset = point.y - fFrame.top;
	
		MoveBy(xOffset, yOffset);
	}

	Unlock();
}

//------------------------------------------------------------------------------

void BWindow::MoveTo(float x, float y)
{
	MoveTo(BPoint(x, y));
}

//------------------------------------------------------------------------------

void BWindow::ResizeBy(float dx, float dy)
{
	Lock();
	// stay in minimum & maximum frame limits
	if (fFrame.Width() + dx < fMinWindWidth)
		dx = fMinWindWidth - fFrame.Width();
	if (fFrame.Width() + dx > fMaxWindWidth)
		dx = fMaxWindWidth - fFrame.Width();
	if (fFrame.Height() + dy < fMinWindHeight)
		dy = fMinWindHeight - fFrame.Height();
	if (fFrame.Height() + dy > fMaxWindHeight)
		dy = fMaxWindHeight - fFrame.Height();

	if (dx != 0.0 || dy != 0.0) {
	
		fLink->StartMessage(AS_WINDOW_RESIZE);
	//	fLink->Attach<float>( fFrame.Width() );
	//	fLink->Attach<float>( fFrame.Height() );
		fLink->Attach<float>(dx);
		fLink->Attach<float>(dy);
		fLink->Flush();
		
		fFrame.SetRightBottom( fFrame.RightBottom() + BPoint(dx, dy));
		top_view->ResizeBy(dx, dy);
	}
	Unlock();
}

//------------------------------------------------------------------------------

void BWindow::ResizeTo(float width, float height)
{
	Lock();
	ResizeBy(width - fFrame.Width(), height - fFrame.Height());
	Unlock();
}

//------------------------------------------------------------------------------

void BWindow::Show()
{
	bool	isLocked = this->IsLocked();
		
	fShowLevel--;

	if (fShowLevel == 0)
	{
		STRACE(("BWindow(%s): sending AS_SHOW_WINDOW message...\n", Name() ));
		if (Lock())
		{
			fLink->StartMessage( AS_SHOW_WINDOW );
			fLink->Flush();
			Unlock();
		}
	}

	// if it's the fist time Show() is called... start the Looper thread.
	if ( Thread() == B_ERROR )
	{
		// normally this won't happen, but I want to be sure!
		if ( !isLocked ) Lock();
		Run();
	}
	
}

//------------------------------------------------------------------------------

void BWindow::Hide()
{
	if (fShowLevel == 0)
	{
		Lock();
		fLink->StartMessage( AS_HIDE_WINDOW );
		fLink->Flush();
		Unlock();
	}
	fShowLevel++;
}

//------------------------------------------------------------------------------

bool BWindow::IsHidden() const
{
	return fShowLevel > 0; 
}

//------------------------------------------------------------------------------

bool BWindow::QuitRequested()
{
	return BLooper::QuitRequested();
}

//------------------------------------------------------------------------------

thread_id BWindow::Run()
{
	return BLooper::Run();
}

//------------------------------------------------------------------------------

status_t BWindow::GetSupportedSuites(BMessage* data)
{
	status_t err = B_OK;
	if (!data)
		err = B_BAD_VALUE;

	if (!err)
	{
		err = data->AddString("Suites", "suite/vnd.Be-window");
		if (!err)
		{
			BPropertyInfo propertyInfo(windowPropInfo);
			err = data->AddFlat("message", &propertyInfo);

			if (!err)
				err = BLooper::GetSupportedSuites(data);
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
		{
			break;
		}
		
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
		{
			return this;
		}
		
		case 14:
		{
			if (fKeyMenuBar)
			{
				msg->PopSpecifier();
				return fKeyMenuBar;
			}
			else
			{
				BMessage replyMsg(B_MESSAGE_NOT_UNDERSTOOD);
				replyMsg.AddInt32( "error", B_NAME_NOT_FOUND );
				replyMsg.AddString( "message", "This window doesn't have a main MenuBar");
				msg->SendReply( &replyMsg );
				return NULL;
			}
		}
		
		case 15:
		{
			// we will NOT pop the current specifier
			return top_view;
		}
		
		case 16:
		{
			return this;
		}
	}

	return BLooper::ResolveSpecifier(msg, index, specifier, what, property);
}

// PRIVATE
//--------------------Private Methods-------------------------------------------
// PRIVATE

void BWindow::InitData(	BRect frame, const char* title, window_look look,
	window_feel feel, uint32 flags,	uint32 workspace)
{

	STRACE(("BWindow::InitData(...)\n"));
	
	fTitle=NULL;
	if ( be_app == NULL )
	{
		debugger("You need a valid BApplication object before interacting with the app_server");
		return;
	}
	
	fFrame		= frame;

	if (title)
		SetTitle( title );
	else
		SetTitle("no_name_window");
	
	fFeel			= feel;
	fLook			= look;
	fFlags			= flags;
	

	fInTransaction	= false;
	fActive			= false;
	fShowLevel		= 1;

	top_view		= NULL;
	fFocus			= NULL;
	fLastMouseMovedView	= NULL;
	fKeyMenuBar		= NULL;
	fDefaultButton	= NULL;

//	accelList		= new BList( 10 );
	AddShortcut('X', B_COMMAND_KEY, new BMessage(B_CUT), NULL);
	AddShortcut('C', B_COMMAND_KEY, new BMessage(B_COPY), NULL);
	AddShortcut('V', B_COMMAND_KEY, new BMessage(B_PASTE), NULL);
	AddShortcut('A', B_COMMAND_KEY, new BMessage(B_SELECT_ALL), NULL);
	AddShortcut('W', B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));

	fPulseEnabled	= false;
	fPulseRate		= 0;
	fPulseRunner	= NULL;

	// TODO: is this correct??? should the thread loop be started???
	//SetPulseRate( 500000 );

	// TODO:  see if you can use 'fViewsNeedPulse'

	fIsFilePanel	= false;

	// TODO: see WHEN is this used!
	fMaskActivated	= false;

	// TODO: see WHEN is this used!
	fWaitingForMenu	= false;

	fMinimized		= false;

	// TODO:  see WHERE you can use 'fMenuSem'

	fMaxZoomHeight	= 32768.0;
	fMaxZoomWidth	= 32768.0;
	fMinWindHeight	= 0.0;
	fMinWindWidth	= 0.0;
	fMaxWindHeight	= 32768.0;
	fMaxWindWidth	= 32768.0;

	fLastViewToken	= B_NULL_TOKEN;
	
	// TODO: other initializations!

	// Here, we will contact app_server and let him know that a window has
	// been created
	
	receive_port = create_port( B_LOOPER_PORT_DEFAULT_CAPACITY ,"w_rcv_port");
	
	if (receive_port<0)
	{
		debugger("Could not create BWindow's receive port, used for interacting with the app_server!");
		delete this;
	}

	STRACE(("BWindow::InitData(): contacting app_server...\n"));

	// let app_server to know that a window has been created.
	fLink = new BPortLink(be_app->fServerFrom, receive_port);
	
	// HERE we are in BApplication's thread, so for locking we use be_app variable
	// we'll lock the be_app to be sure we're the only one writing at BApplication's server port
	bool locked = false;
	if ( !(be_app->IsLocked()) )
	{
		be_app->Lock();
		locked = true; 
	}
	
 	STRACE(("be_app->fServerTo is %ld\n", be_app->fServerFrom));
	
 	status_t err;
 	fLink->StartMessage(AS_CREATE_WINDOW);
 	fLink->Attach<BRect>( fFrame );
 	fLink->Attach<int32>( (int32)fLook );
 	fLink->Attach<int32>( (int32)fFeel );
 	fLink->Attach<uint32>( fFlags );
 	fLink->Attach<uint32>( workspace );
 	fLink->Attach<int32>( _get_object_token_(this) );
 	fLink->Attach<port_id>( receive_port );
 	fLink->Attach<port_id>( fMsgPort );
 	fLink->AttachString( title );
 	fLink->Flush();
 
 	send_port = -1;
 	int32 rCode = SERVER_FALSE;
 	err = fLink->GetNextReply( &rCode );
 	if (err == B_OK && rCode == SERVER_TRUE)
 		fLink->Read<port_id>(&send_port);
 	fLink->SetSendPort(send_port);
	
 	STRACE(("Server says that our send port is %ld\n", send_port));
 	
  	STRACE(("Window locked?: %s\n", IsLocked()?"True":"False"));
  
 	// build and register top_view with app_server
  	BuildTopView();
}


/**	Reads all pending messages from the window port and put them into the queue.
 */

void
BWindow::DequeueAll()
{
	//	Get message count from port
	int32 count = port_count(fMsgPort);

	for (int32 i = 0; i < count; i++) {
		BMessage *message = MessageFromPort(0);
		if (message != NULL)
			fQueue->AddMessage(message);
	}
}


// TODO: This here is a nearly full code duplication to BLooper::task_loop
// but with one little difference: It uses the determine_target function
// to tell what the later target of a message will be, if no explicit target
// is supplied. This is important because we need to call the right targets
// MessageFilter. For B_KEY_DOWN messages for example, not the BWindow but the
// focus view will be the target of the message. This means that also the
// focus views MessageFilters have to be checked before DispatchMessage and
// not the ones of this BWindow.

void BWindow::task_looper()
{
	STRACE(("info: BWindow::task_looper() started.\n"));
	
	//	Check that looper is locked (should be)
	AssertLocked();
	//	Unlock the looper
	Unlock();

	//	loop: As long as we are not terminating.
	while (!fTerminating)
	{
		// TODO: timeout determination algo
		//	Read from message port (how do we determine what the timeout is?)
		BMessage* msg = MessageFromPort();

		//	Did we get a message?
		if (msg)
		{
			//	Add to queue
			fQueue->AddMessage(msg);
		} else {
			continue;
		}
		
		//	Get message count from port
		int32 msgCount = port_count(fMsgPort);
		for (int32 i = 0; i < msgCount; ++i)
		{
			//	Read 'count' messages from port (so we will not block)
			//	We use zero as our timeout since we know there is stuff there
			msg = MessageFromPort(0);
			//	Add messages to queue
			if (msg)
			{
				fQueue->AddMessage(msg);
			}
		}

		//	loop: As long as there are messages in the queue and the port is
		//		  empty... and we are not terminating, of course.
		bool dispatchNextMessage = true;
		while (!fTerminating && dispatchNextMessage)
		{
			//	Get next message from queue (assign to fLastMessage)
			fLastMessage = fQueue->NextMessage();

			//	Lock the looper
			Lock();
			if (!fLastMessage)
			{
				// No more messages: Unlock the looper and terminate the
				// dispatch loop.
				dispatchNextMessage = false;
			}
			else
			{
				//	Get the target handler
				//	Use BMessage friend functions to determine if we are using the
				//	preferred handler, or if a target has been specified
				BHandler* handler;
				if (_use_preferred_target_(fLastMessage))
				{
					handler = fPreferred;
				}
				else
				{
					/**
						@note	Here is where all the token stuff starts to
								make sense.  How, exactly, do we determine
								what the target BHandler is?  If we look at
								BMessage, we see an int32 field, fTarget.
								Amazingly, we happen to have a global mapping
								of BHandler pointers to int32s!
					 */
					 gDefaultTokens.GetToken(_get_message_target_(fLastMessage),
					 						 B_HANDLER_TOKEN,
					 						 (void**)&handler);
				}

				if (!handler)
				{
					handler = determine_target(msg, handler, _use_preferred_target_(fLastMessage));
					if (!handler)
						handler = this;
				}

				//	Is this a scripting message? (BMessage::HasSpecifiers())
				if (fLastMessage->HasSpecifiers())
				{
					int32 index = 0;
					// Make sure the current specifier is kosher
					if (fLastMessage->GetCurrentSpecifier(&index) == B_OK)
					{
						handler = resolve_specifier(handler, fLastMessage);
					}
				}
				else
				{
				}
				
				if (handler)
				{
					//	Do filtering
					handler = top_level_filter(fLastMessage, handler);
					if (handler && handler->Looper() == this)
					{
						DispatchMessage(fLastMessage, handler);
					}
				}
			}

			//	Unlock the looper
			Unlock();

			//	Delete the current message (fLastMessage)
			delete fLastMessage;
			fLastMessage = NULL;

			//	Are any messages on the port?
			if (port_count(fMsgPort) > 0)
			{
				//	Do outer loop
				dispatchNextMessage = false;
			}
		}
	}
}

//------------------------------------------------------------------------------

window_type BWindow::composeType(window_look look,	
								 window_feel feel) const
{
	window_type returnValue = B_UNTYPED_WINDOW;

	switch(feel)
	{
		case B_NORMAL_WINDOW_FEEL:
		{
			switch (look)
			{
				case B_TITLED_WINDOW_LOOK:
				{
					returnValue = B_TITLED_WINDOW;
					break;
				}
				case B_DOCUMENT_WINDOW_LOOK:
				{
					returnValue = B_DOCUMENT_WINDOW;
					break;
				}
				case B_BORDERED_WINDOW_LOOK:
				{
					returnValue = B_BORDERED_WINDOW;
					break;
				}
				default:
				{
					returnValue = B_UNTYPED_WINDOW;
				}
			}
			
			break;
		}
		case B_MODAL_APP_WINDOW_FEEL:
		{
			if (look == B_MODAL_WINDOW_LOOK)
				returnValue = B_MODAL_WINDOW;
			break;
		}
		case B_FLOATING_APP_WINDOW_FEEL:
		{
			if (look == B_FLOATING_WINDOW_LOOK)
				returnValue = B_FLOATING_WINDOW;
			break;
		}	
		default:
		{
			returnValue = B_UNTYPED_WINDOW;
		}
	}
	
	return returnValue;
}

//------------------------------------------------------------------------------

void BWindow::decomposeType(window_type type, window_look* look,
		window_feel* feel) const
{
	switch (type)
	{
		case B_TITLED_WINDOW:
		{
			*look = B_TITLED_WINDOW_LOOK;
			*feel = B_NORMAL_WINDOW_FEEL;
			break;
		}
		case B_DOCUMENT_WINDOW:
		{
			*look = B_DOCUMENT_WINDOW_LOOK;
			*feel = B_NORMAL_WINDOW_FEEL;
			break;
		}
		case B_MODAL_WINDOW:
		{
			*look = B_MODAL_WINDOW_LOOK;
			*feel = B_MODAL_APP_WINDOW_FEEL;
			break;
		}
		case B_FLOATING_WINDOW:
		{
			*look = B_FLOATING_WINDOW_LOOK;
			*feel = B_FLOATING_APP_WINDOW_FEEL;
			break;
		}
		case B_BORDERED_WINDOW:
		{
			*look = B_BORDERED_WINDOW_LOOK;
			*feel = B_NORMAL_WINDOW_FEEL;
			break;
		}
		case B_UNTYPED_WINDOW:
		{
			*look = B_TITLED_WINDOW_LOOK;
			*feel = B_NORMAL_WINDOW_FEEL;
			break;
		}
		default:
		{
			*look = B_TITLED_WINDOW_LOOK;
			*feel = B_NORMAL_WINDOW_FEEL;
			break;
		}
	}
}

//------------------------------------------------------------------------------

void BWindow::BuildTopView()
{
 	STRACE(("BuildTopView(): enter\n"));
 	BRect frame = fFrame;
 	frame.OffsetTo(0,0);
 
 	top_view		= new BView( frame, "top_view",
  								 B_FOLLOW_ALL, B_WILL_DRAW);
  	top_view->top_level_view	= true;
  
 	//inhibit check_lock()
  	fLastViewToken		= _get_object_token_( top_view );
  
 	// set top_view's owner, add it to window's eligible handler list
 	// and also set its next handler to be this window.
 	
 	STRACE(("Calling setowner top_view=%08x this=%08x.\n", (unsigned)top_view,
 		(unsigned)this));
 	
 	top_view->setOwner(this);
 
 	//we can't use AddChild() because this is the top_view
  	top_view->attachView(top_view);
  
 	STRACE(("BuildTopView ended\n"));
}

//------------------------------------------------------------------------------

void BWindow::stopConnection()
{
	Lock();
	fLink->StartMessage( AS_DELETE_WINDOW );
	fLink->Flush();
	Unlock();
}

//------------------------------------------------------------------------------

void BWindow::prepareView(BView *aView)
{
	// TODO: implement
}

//------------------------------------------------------------------------------

void BWindow::attachView(BView *aView)
{
	// TODO: implement
}

//------------------------------------------------------------------------------

void BWindow::detachView(BView *aView)
{
	// TODO: implement
}
//------------------------------------------------------------------------------

void BWindow::setFocus(BView *focusView, bool notifyInputServer)
{
	BView* previousFocus = fFocus;

	if (previousFocus == focusView)
		return;

	if (focusView)
		focusView->MakeFocus(true);

	// TODO: find out why do we have to notify input server.
	if (notifyInputServer)
	{
		// what am I suppose to do here??
	}
}

//------------------------------------------------------------------------------

void BWindow::handleActivation( bool active ){

	if (active)
	{
		// TODO: talk to Ingo to make BWindow a friend for BRoster	
		// be_roster->UpdateActiveApp( be_app->Team() );
	}
	
	WindowActivated( active );
	
	// recursively call hook function 'WindowActivated(bool)'
	// for all views attached to this window.
	activateView( top_view, active );
}

//------------------------------------------------------------------------------

void BWindow::activateView( BView *aView, bool active ){

	aView->WindowActivated( active );

	BView		*child;
	if ( (child = aView->first_child) )
	{
		while ( child ) 
		{
			activateView( child, active );
			child 		= child->next_sibling; 
		}
	}
}

//------------------------------------------------------------------------------

BHandler *
BWindow::determine_target(BMessage *msg, BHandler *target, bool pref)
{
	// TODO: this is mostly guessed; check for correctness.
	// I think this function is used to determine if a BView will be
	// the target of a message. This is used in the BLooper::task_loop
	// to determine what BHandler will dispatch the message and what filters
	// should be checked before doing so.
	
	switch (msg->what) {
		case B_KEY_DOWN:
		case B_KEY_UP:
		case B_UNMAPPED_KEY_DOWN:
		case B_UNMAPPED_KEY_UP:
		case B_MODIFIERS_CHANGED:
		case B_MOUSE_WHEEL_CHANGED:
			// these messages will be dispatched by the focus view later
			return fFocus;
		
		case B_MOUSE_DOWN:
		case B_MOUSE_UP:
		case B_MOUSE_MOVED:
			// TODO: find out how to determine the target for these
			break;
		
		case B_PULSE:
		case B_QUIT_REQUESTED:
			// TODO: test wether R5 will let BView dispatch these messages
			break;
		
		case B_VIEW_RESIZED:
		case B_VIEW_MOVED: {
			int32			token = B_NULL_TOKEN;
			msg->FindInt32("_token", &token);
			BView *view = findView(top_view, token);
			if (view)
				return view;
		} break;
		default: break;
	}
	
	return target;
}

//------------------------------------------------------------------------------

bool BWindow::handleKeyDown( const char key, uint32 modifiers){

	// TODO: ask people if using 'raw_char' is OK ?

		// handle BMenuBar key
	if ( (key == B_ESCAPE) && (modifiers & B_COMMAND_KEY)
		&& fKeyMenuBar)
	{

		// TODO: ask Marc about 'fWaitingForMenu' member!

		// fWaitingForMenu		= true;
		fKeyMenuBar->StartMenuBar(0, true, false, NULL);
		return true;
	}

	// Command+q has been pressed, so, we will quit
	if ( (key == 'Q' || key == 'q') && modifiers & B_COMMAND_KEY)
	{
		be_app->PostMessage(B_QUIT_REQUESTED); 
		return true;
	}

	// Keyboard navigation through views!!!!
	// TODO: Not correct, only Option-Tab should be handled here.
	if ( key == B_TAB)
	{
		BView			*nextFocus;

		if (modifiers & B_CONTROL_KEY & B_SHIFT_KEY)
		{
			nextFocus		= findPrevView( fFocus, B_NAVIGABLE_JUMP );
		}
		else
			if (modifiers & B_CONTROL_KEY)
			{
				nextFocus		= findNextView( fFocus, B_NAVIGABLE_JUMP );
			}
			else
				if (modifiers & B_SHIFT_KEY)
				{
					nextFocus		= findPrevView( fFocus, B_NAVIGABLE );
				}
				else
					nextFocus		= findNextView( fFocus, B_NAVIGABLE );

		if ( nextFocus && nextFocus != fFocus)
			setFocus( nextFocus, false );

		return true;
	}

		// Handle shortcuts
	int			index;
	if ( (index = findShortcut(key, modifiers)) >=0)
	{
		_BCmdKey		*cmdKey;

		cmdKey			= (_BCmdKey*)accelList.ItemAt( index );

			// we'll give the message to the focus view
		if (cmdKey->targetToken == B_ANY_TOKEN)
		{
			fFocus->MessageReceived( cmdKey->message );
			return true;
		}
		else
		{
			BHandler		*handler;
			BHandler		*aHandler;
			int				noOfHandlers;

			// search for a match through BLooper's list of eligible handlers
			handler			= NULL;
			noOfHandlers	= CountHandlers();
			for( int i=0; i < noOfHandlers; i++ )
			
				// do we have a match?
				if ( _get_object_token_( aHandler = HandlerAt(i) )
					 == cmdKey->targetToken)
				{
					// yes, we do.
					handler		= aHandler;
					break;
				}

			if ( handler )
				handler->MessageReceived( cmdKey->message );
			else
				// if no handler was found, BWindow will handle the message
				MessageReceived( cmdKey->message );
		}
		return true;
	}

	// if <ENTER> is pressed and we have a default button
	if (DefaultButton() && (key == B_ENTER))
	{
		const char		*chars;		// just to be sure
		CurrentMessage()->FindString("bytes", &chars);

		DefaultButton()->KeyDown( chars, strlen(chars) );
		return true;
	}


	return false;
}

//------------------------------------------------------------------------------
BView* BWindow::sendMessageUsingEventMask2( BView* aView, int32 message, BPoint where )
{
	BView		*destView;
	destView	= NULL;

	STRACE(("info: BWindow::sendMessageUsingEventMask2() recursing to view %s with point %f,%f.\n",
	 	aView->Name() ? aView->Name() : "<no name>", aView->ConvertFromScreen(where).x, aView->ConvertFromScreen(where).y));

	if ( aView->fBounds.Contains( aView->ConvertFromScreen(where) ))
	{
		 destView = aView;	//this is the lower-most view under the mouse so far
		 STRACE(("info: BWindow::sendMessageUsingEventMask() targeted view %s.\n",
		 	aView->Name() ? aView->Name() : "<no name>"));
	}

	// Code for Event Masks
	BView *child = aView->first_child;
	while ( child )
	{ 
		// see if a BView registered for mouse events and it's not the current focus view
		if ( aView != fFocus  &&
			child->fEventMask & (B_POINTER_EVENTS | B_POINTER_EVENTS << 16))
		{
			switch (message)
			{
				case B_MOUSE_DOWN:
				{
					child->MouseDown( child->ConvertFromScreen( where ) );
				}
				break;
				
				case B_MOUSE_UP:
				{
					//clear MouseEventMask on MouseUp
					child->fEventMask &= 0x0000FFFF;
					child->MouseUp( child->ConvertFromScreen( where ) );
				}
				break;
			
				case B_MOUSE_MOVED:
				{
					BMessage	*dragMessage;

					// TODO: get the dragMessage if any for now...
					dragMessage	= NULL;

					// TODO: after you have an example working, see if a view that registered for such events,
					// does reveive B_MOUSE_MOVED with other options than B_OUTDIDE_VIEW !!!
					// like: B_INSIDE_VIEW, B_ENTERED_VIEW, B_EXITED_VIEW

					child->MouseMoved( child->ConvertFromScreen(where), B_OUTSIDE_VIEW , dragMessage);
				}
				break;
			}
		}
		BView *target = sendMessageUsingEventMask2( child, message, where );

		// one of the children contains the point
		if (target)
			destView = target;
		child = child->next_sibling;
	}
	
	return destView;
}

//------------------------------------------------------------------------------

void BWindow::sendMessageUsingEventMask( int32 message, BPoint where )
{
	BView *destView = NULL;

	destView = sendMessageUsingEventMask2(top_view, message, where);
	
	// I'm SURE this is NEVER going to happen, but, during development of 
	// BWindow, it may slip a NULL value
	if (!destView)
	{
		// debugger("There is no BView under the mouse;");
		return;
	}
	
	switch( message )
	{
		case B_MOUSE_DOWN:
		{
			setFocus( destView );
			destView->MouseDown( destView->ConvertFromScreen( where ) );
			break;
		}
		case B_MOUSE_UP:
		{
			destView->MouseUp( destView->ConvertFromScreen( where ) );
			break;
		}
		case B_MOUSE_MOVED:
		{
			BMessage	*dragMessage;

			// TODO: add code for drag and drop
			// for now...
			dragMessage	= NULL;

			if (destView != fLastMouseMovedView)
			{
				if(fLastMouseMovedView)
	 				fLastMouseMovedView->MouseMoved( destView->ConvertFromScreen( where ), B_EXITED_VIEW , dragMessage);
 				destView->MouseMoved( ConvertFromScreen( where ), B_ENTERED_VIEW, dragMessage);
 				fLastMouseMovedView		= destView;
 				break;
			}
			else
			{
 				destView->MouseMoved( ConvertFromScreen( where ), B_INSIDE_VIEW , dragMessage);
 				break;
			}

			// I'm guessing that B_OUTSIDE_VIEW is given to the view that has focus,
			// I'll have to check
			
			// TODO: Do research on mouse capturing -- maybe it has something to do 
			// with this
 			if (fFocus != destView)
 				fFocus->MouseMoved( ConvertFromScreen( where ), B_OUTSIDE_VIEW , dragMessage);
			break;}
	}
}

//------------------------------------------------------------------------------

BMessage* BWindow::ConvertToMessage(void* raw, int32 code)
{
	return BLooper::ConvertToMessage( raw, code );
}

//------------------------------------------------------------------------------

void BWindow::sendPulse( BView* aView )
{
	BView *child; 
	if ( (child = aView->first_child) )
	{
		while ( child )
		{ 
			if ( child->Flags() & B_PULSE_NEEDED )
				child->Pulse();
			sendPulse( child );
			child = child->next_sibling; 
		}
	}
}

//------------------------------------------------------------------------------

int32 BWindow::findShortcut( uint32 key, uint32 modifiers )
{
	int32			index,
					noOfItems;

	index			= -1;
	noOfItems		= accelList.CountItems();

	for ( int32 i = 0;  i < noOfItems; i++ )
	{
		_BCmdKey*		tempCmdKey;

		tempCmdKey		= (_BCmdKey*)accelList.ItemAt(i);
		if (tempCmdKey->key == key && tempCmdKey->modifiers == modifiers)
		{
			index		= i;
			break;
		}
	}
	
	return index;
}

//------------------------------------------------------------------------------

BView* BWindow::findView(BView* aView, int32 token)
{

	if ( _get_object_token_(aView) == token )
		return aView;

	BView			*child;
	if ( (child = aView->first_child) )
	{
		while ( child )
		{
			BView*		view;
			if ( (view = findView( child, token )) )
				return view;
			child 		= child->next_sibling; 
		}
	}

	return NULL;
}

//------------------------------------------------------------------------------

BView* BWindow::findView(BView* aView, const char* viewName) const
{

	if ( strcmp( viewName, aView->Name() ) == 0)
		return aView;

	BView			*child;
	if ( (child = aView->first_child) )
	{
		while ( child )
		{
			BView*		view;
			if ( (view = findView( child, viewName )) )
				return view;
			child 		= child->next_sibling; 
		}
	}

	return NULL;
}

//------------------------------------------------------------------------------

BView* BWindow::findView(BView* aView, BPoint point) const
{

	if ( aView->Bounds().Contains(point) && !aView->first_child )
		return aView;

	BView			*child;
	if ( (child = aView->first_child) )
	{
		while ( child )
		{
			BView*		view;
			if ( (view = findView( child, point )) )
				return view;
			child 		= child->next_sibling; 
		}
	}

	return NULL;
}

//------------------------------------------------------------------------------

BView* BWindow::findNextView( BView *focus, uint32 flags)
{
	if (focus == NULL)
		focus = top_view;

	bool		found = false;
	BView		*nextFocus = focus;

	// Ufff... this toked me some time... this is the best form I've reached.
	// This algorithm searches the tree for BViews that accept focus.
	while (!found)
	{
		if (nextFocus->first_child)
			nextFocus		= nextFocus->first_child;
		else
		{
			if (nextFocus->next_sibling)
				nextFocus		= nextFocus->next_sibling;
			else
			{
				while( !nextFocus->next_sibling && nextFocus->parent )
					nextFocus		= nextFocus->parent;

				if (nextFocus == top_view)
					nextFocus		= nextFocus->first_child;
				else
					nextFocus		= nextFocus->next_sibling;
			}
		}
		
		if (nextFocus->Flags() & flags)
			found = true;

		// It means that the hole tree has been searched and there is no
		// view with B_NAVIGABLE_JUMP flag set!
		if (nextFocus == focus)
			return NULL;
	}

	return nextFocus;
}

//------------------------------------------------------------------------------

BView* BWindow::findPrevView( BView *focus, uint32 flags)
{
	bool		found;
	found		= false;

	BView		*prevFocus;
	prevFocus	= focus;

	BView		*aView;

	while (!found)
	{
		if ( (aView = findLastChild(prevFocus)) )
			prevFocus		= aView;
		else
		{
			if (prevFocus->prev_sibling)
				prevFocus		= prevFocus->prev_sibling;
			else
			{
				while( !prevFocus->prev_sibling && prevFocus->parent )
					prevFocus		= prevFocus->parent;

				if (prevFocus == top_view)
					prevFocus		= findLastChild( prevFocus );
				else
					prevFocus		= prevFocus->prev_sibling;
			}
		}
		
		if (prevFocus->Flags() & flags)
			found = true;


		// It means that the hole tree has been searched and there is no
		// view with B_NAVIGABLE_JUMP flag set!
		if (prevFocus == focus)
			return NULL;
	}

	return prevFocus;
}

//------------------------------------------------------------------------------

BView* BWindow::findLastChild(BView *parent)
{
	BView		*aView;
	if ( (aView = parent->first_child) )
	{
		while (aView->next_sibling)
			aView		= aView->next_sibling;

		return aView;
	}
	else
		return NULL;
}

//------------------------------------------------------------------------------

void BWindow::drawAllViews(BView* aView)
{
	if(Lock())
	{
		top_view->Invalidate();
		Unlock();
	}
	Sync();
}

//------------------------------------------------------------------------------

void BWindow::DoUpdate(BView* aView, BRect& area)
{
 
 	STRACE(("info: BWindow::DoUpdate() BRect(%f,%f,%f,%f) called.\n",
 		area.left, area.top, area.right, area.bottom));
 
 	aView->check_lock();
 			
 	if (aView->Flags() & B_WILL_DRAW)
 		aView->Draw( area );
 	else
 	{
 		rgb_color c = aView->HighColor();
 		aView->SetHighColor(aView->ViewColor());
 		aView->FillRect(aView->Bounds(), B_SOLID_HIGH);
 		aView->SetHighColor(c);
 	}
 
	BView *child;
	if ( (child = aView->first_child) )
	{
		while ( child )
		{
			if ( area.Intersects( child->Frame() ) )
			{
				BRect		newArea;
				newArea		= area & child->Frame();
				child->ConvertFromParent( &newArea );
				DoUpdate( child, newArea );
			}
			child 		= child->next_sibling; 
		}
	}
}

//------------------------------------------------------------------------------

void BWindow::SetIsFilePanel(bool yes)
{
	// TODO: is this not enough?
	fIsFilePanel	= yes;
}

//------------------------------------------------------------------------------

bool BWindow::IsFilePanel() const
{
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

void BWindow::PrintToStream() const{
	printf("BWindow '%s' data:\
		Title			= %s\
		Token			= %ld\
		InTransaction 	= %s\
		Active 			= %s\
		fShowLevel		= %d\
		Flags			= %lx\
		send_port		= %ld\
		receive_port	= %ld\
		top_view name	= %s\
		focus view name	= %s\
		lastMouseMoved	= %s\
		fLink			= %s\
		KeyMenuBar name	= %s\
		DefaultButton	= %s\
		# of shortcuts	= %ld",
		Name(),
		fTitle!=NULL? fTitle:"NULL",
		_get_object_token_(this),		
		fInTransaction==true? "yes":"no",
		fActive==true? "yes":"no",
		fShowLevel,
		fFlags,
		send_port,
		receive_port,
		top_view!=NULL? top_view->Name():"NULL",
		fFocus!=NULL? fFocus->Name():"NULL",
		fLastMouseMovedView!=NULL? fLastMouseMovedView->Name():"NULL",
		fLink!=NULL? "In place":"NULL",
		fKeyMenuBar!=NULL? fKeyMenuBar->Name():"NULL",
		fDefaultButton!=NULL? fDefaultButton->Name():"NULL",
		accelList.CountItems());
/*
	for( int32 i=0; i<accelList.CountItems(); i++){
		_BCmdKey	*key = (_BCmdKey*)accelList.ItemAt(i);
		printf("\tShortCut %ld: char %s\n\t\t message: \n", i, (key->key > 127)?"ASCII":"UNICODE");
		key->message->PrintToStream();
	}
*/	
	printf("\
		topViewToken	= %ld\
		pluseEnabled	= %s\
		isFilePanel		= %s\
		MaskActivated	= %s\
		pulseRate		= %lld\
		waitingForMenu	= %s\
		minimized		= %s\
		Menu semaphore	= %ld\
		maxZoomHeight	= %f\
		maxZoomWidth	= %f\
		minWindHeight	= %f\
		minWindWidth	= %f\
		maxWindHeight	= %f\
		maxWindWidth	= %f\
		frame			= ( %f, %f, %f, %f )\
		look			= %d\
		feel			= %d\
		lastViewToken	= %ld\
		pulseRUNNER		= %s\n",
		fTopViewToken,
		fPulseEnabled==true?"Yes":"No",
		fIsFilePanel==true?"Yes":"No",
		fMaskActivated==true?"Yes":"No",
		fPulseRate,
		fWaitingForMenu==true?"Yes":"No",
		fMinimized==true?"Yes":"No",
		fMenuSem,
		fMaxZoomHeight,
		fMaxZoomWidth,
		fMinWindHeight,
		fMinWindWidth,
		fMaxWindHeight,
		fMaxWindWidth,
		fFrame.left, fFrame.top, fFrame.right, fFrame.bottom, 
		(int16)fLook,
		(int16)fFeel,
		fLastViewToken,
		fPulseRunner!=NULL?"In place":"NULL");
}

/*
TODO list:

	*) take care of temporarely events mask!!!
	*) what's with this flag B_ASYNCHRONOUS_CONTROLS ?
	*) test arguments for SetWindowAligment
	*) call hook functions: MenusBeginning, MenusEnded. Add menu activation code.
*/

/*
 @log
*/
