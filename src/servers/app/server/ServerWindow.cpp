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
#include <Message.h>
#include <GraphicsDefs.h>
#include <PortLink.h>
#include "AppServer.h"
#include "Layer.h"
#include "RootLayer.h"
#include "ServerWindow.h"
#include "ServerApp.h"
#include "ServerProtocol.h"
#include "WinBorder.h"
#include "Desktop.h"
#include "DesktopClasses.h"
#include "TokenHandler.h"
#include "Utils.h"
#include "DisplayDriver.h"
#include "ServerPicture.h"

//#define DEBUG_SERVERWINDOW
//#define DEBUG_SERVERWINDOW_MOUSE
//#define DEBUG_SERVERWINDOW_KEYBOARD


#ifdef DEBUG_SERVERWINDOW
#	include <stdio.h>
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif

#ifdef DEBUG_SERVERWINDOW_KEYBOARD
#	include <stdio.h>
#	define STRACE_KEY(x) printf x
#else
#	define STRACE_KEY(x) ;
#endif

#ifdef DEBUG_SERVERWINDOW_MOUSE
#	include <stdio.h>
#	define STRACE_MOUSE(x) printf x
#else
#	define STRACE_MOUSE(x) ;
#endif

//! TokenHandler object used to provide IDs for all windows in the system
TokenHandler win_token_handler;

//! Active winborder - used for tracking windows during moves, resizes, and tab slides
WinBorder *active_winborder=NULL;

template<class Type> Type
read_from_buffer(int8 **_buffer)
{
	Type *typedBuffer = (Type *)(*_buffer);
	Type value = *typedBuffer;

	typedBuffer++;
	*_buffer = (int8 *)(typedBuffer);

	return value;
}

static int8 *read_pattern_from_buffer(int8 **_buffer)
{
	int8 *pattern = *_buffer;

	*_buffer += AS_PATTERN_SIZE;

	return pattern;
}

template<class Type> void
write_to_buffer(int8 **_buffer, Type value)
{
	Type *typedBuffer = (Type *)(*_buffer);

	*typedBuffer = value;
	typedBuffer++;

	*_buffer = (int8 *)(typedBuffer);
}

/*!
	\brief Contructor
	
	Does a lot of stuff to set up for the window - new decorator, new winborder, spawn a 
	monitor thread.
*/
ServerWindow::ServerWindow(BRect rect, const char *string, uint32 wlook,
	uint32 wfeel, uint32 wflags, ServerApp *winapp,  port_id winport,
	port_id looperPort, uint32 index, int32 handlerID)
{
	_title			= new BString;
	if( string )
		_title->SetTo(string);
	_frame			= rect;
	_flags			= wflags;
	_look			= wlook;
	_feel			= wfeel;
	_handlertoken	= handlerID;
	cl				= NULL;
	winLooperPort	= looperPort;

	_winborder		= new WinBorder(_frame,_title->String(),wlook,wfeel,wflags,this);

	// _sender is the monitored window's event port
	_sender			= winport;

	_winlink		= new PortLink(_sender);
	_applink		= (winapp)? new PortLink(winapp->_receiver) : NULL;
	
	// _receiver is the port to which the app sends messages for the server
	_receiver		= create_port(30,_title->String());


	ses				= new BSession( _receiver, _sender );
		// Send a reply to our window - it is expecting _receiver port.
	ses->WriteData( &_receiver, sizeof(port_id) );
	ses->Sync();
	
	top_layer		= NULL;
	

	_active			= false;

	// Spawn our message monitoring _monitorthread
	_monitorthread	= spawn_thread( MonitorWin, _title->String(),
									B_NORMAL_PRIORITY, this );
	if(_monitorthread != B_NO_MORE_THREADS &&
		_monitorthread != B_NO_MEMORY)
		resume_thread( _monitorthread );

	_workspace_index= index;
	_workspace		= NULL;
	_token			= win_token_handler.GetToken();

	AddWindowToDesktop(this,index,ActiveScreen());
STRACE(("ServerWindow %s:\n",_title->String() ));
STRACE(("\tFrame (%.1f,%.1f,%.1f,%.1f)\n",rect.left,rect.top,rect.right,rect.bottom));
STRACE(("\tPort: %ld\n",_receiver));
STRACE(("\tWorkspace: %ld\n",index));
}

//!Tears down all connections with the user application, kills the monitoring thread.
ServerWindow::~ServerWindow(void)
{
STRACE(("ServerWindow %s:~ServerWindow()\n",_title->String()));
	RemoveWindowFromDesktop(this);
	if(_applink)
	{
		delete _applink;
		_applink=NULL;
		delete _title;
		delete _winlink;
		delete _winborder;
		
		cl		= NULL;
		if ( top_layer )
			delete top_layer;
		delete	ses;
		ses		= NULL;
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
STRACE(("ServerWindow %s: Request Draw\n",_title->String()));
	BMessage		msg;
	
	msg.what		= _UPDATE_;
	msg.AddRect("_rect", rect);
	
	SendMessageToClient( &msg );
}

//! Requests an update for the entire window
void ServerWindow::RequestDraw(void)
{
	RequestDraw(_frame);
}

//! Forces the window border to update its decorator
void ServerWindow::ReplaceDecorator(void)
{
STRACE(("ServerWindow %s: Replace Decorator\n",_title->String()));
	_winborder->UpdateDecorator();
}

//! Requests that the ServerWindow's BWindow quit
void ServerWindow::Quit(void)
{
STRACE(("ServerWindow %s: Quit\n",_title->String()));
	BMessage		msg;
	
	msg.what		= B_QUIT_REQUESTED;
	
	SendMessageToClient( &msg );
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
STRACE(("ServerWindow %s: Show\n",_title->String()));
	if(_winborder)
	{
		_winborder->Show();
		_winborder->SetFocus(true);
		_winborder->UpdateRegions(true);
	}
}

//! Hides the window's WinBorder
void ServerWindow::Hide(void)
{
STRACE(("ServerWindow %s: Hide\n",_title->String()));
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
STRACE(("ServerWindow %s: Set Focus to %s\n",_title->String(),value?"true":"false"));
	if(_active!=value)
	{
		_active=value;
		_winborder->SetFocus(value);
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
STRACE(("ServerWindow %s: WorkspaceActivated(%ld,%s)\n",_title->String(),workspace,(active)?"active":"inactive"));
	BMessage		msg;
	
	msg.what		= B_WORKSPACE_ACTIVATED;
	msg.AddInt32("workspace", workspace);
	msg.AddBool("active", active);
	
	SendMessageToClient( &msg );
}

/*!
	\brief Notifies window of a workspace switch
	\param oldone index of the previous workspace
	\param newone index of the new workspace
*/
void ServerWindow::WorkspacesChanged(int32 oldone,int32 newone)
{
STRACE(("ServerWindow %s: WorkspacesChanged(%ld,%ld)\n",_title->String(),oldone,newone));
	BMessage		msg;
	
	msg.what		= B_WORKSPACES_CHANGED;
	msg.AddInt32("old", oldone);
	msg.AddInt32("new", newone);
	
	SendMessageToClient( &msg );
}

/*!
	\brief Notifies window of a change in focus
	\param active New active status of the window
*/
void ServerWindow::WindowActivated(bool active)
{
STRACE(("ServerWindow %s: WindowActivated(%s)\n",_title->String(),(active)?"active":"inactive"));
	BMessage		msg;
	
	msg.what		= B_WINDOW_ACTIVATED;
	msg.AddBool("active", active);
	
	SendMessageToClient( &msg );
}

/*!
	\brief Notifies window of a change in screen resolution
	\param frame Size of the new resolution
	\param color_space Color space of the new screen mode
*/
void ServerWindow::ScreenModeChanged(const BRect frame, const color_space cspace)
{
STRACE(("ServerWindow %s: ScreenModeChanged\n",_title->String()));
	BMessage		msg;
	
	msg.what		= B_SCREEN_CHANGED;
	msg.AddRect("frame", frame);
	msg.AddInt32("mode", (int32)cspace);
	
	SendMessageToClient( &msg );
}

/*
	\brief Sets the frame size of the window
	\rect New window size
*/
void ServerWindow::SetFrame(const BRect &rect)
{
STRACE(("ServerWindow %s: Set Frame to (%.1f,%.1f,%.1f,%.1f)\n",_title->String(),
			rect.left,rect.top,rect.right,rect.bottom));
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
STRACE(("ServerWindow %s: Lock\n",_title->String()));
	return (_locker.Lock())?B_OK:B_ERROR;
}

//! Unlocks the window
void ServerWindow::Unlock(void)
{
STRACE(("ServerWindow %s: Unlock\n",_title->String()));
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

void ServerWindow::DispatchMessage( int32 code )
{
	switch( code )
	{
	/********** BView Messages ***********/
		case AS_SET_CURRENT_LAYER:
		{
			int32		token;
			
			ses->ReadInt32( &token );
			
			Layer		*current = FindLayer( top_layer, token );
			if (current)
				cl		= current;
			else // hope this NEVER happens! :-)
				printf("Server PANIC: window %s: cannot find Layer with ID %ld\n", _title->String(), token);

			STRACE(("ServerWindow %s: Message AS_SET_CURRENT_LAYER: Layer name: %s\n", _title->String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_CREATE:
		{
			// Received when a view is attached to a window. This will require
			// us to attach a layer in the tree in the same manner and invalidate
			// the area in which the new layer resides assuming that it is
			// visible.
			STRACE(("SW %s: AS_LAYER_CREATE...\n", _title->String()));
			Layer		*oldCL = cl;
			
			int32		token;
			BRect		frame;
			uint32		resizeMask;
			uint32		flags;
			bool		hidden;
			int32		childCount;
			char*		name;
		
			ses->ReadInt32( &token );
			name		= ses->ReadString();
			ses->ReadRect( &frame );
			ses->ReadInt32( (int32*)&resizeMask );
			ses->ReadInt32( (int32*)&flags );
			ses->ReadBool( &hidden );
			ses->ReadInt32( &childCount );
			
				// view's visible area is invalidated here
			Layer		*newLayer;
			newLayer	= new Layer(frame, name, token, resizeMask, flags, this);
				// there is no way of setting this, other than manual. :-)
			newLayer->_hidden	= hidden;
			
			// TODO: 'before' Layer support???
			
			// TODO: review Layer::AddChild
				// newLayer's parent will invalidate the area that its new child
				//    occupies, here in AddChild()
			cl->AddChild( newLayer );
			cl->PrintTree();
			
				// we set Layer attributes (BView's state)...
			cl			= newLayer;
			
			int32		msgCode;
			ses->ReadInt32( &msgCode );		// this is AS_LAYER_SET_FONT_STATE
			DispatchMessage( msgCode );

			ses->ReadInt32( &msgCode );		// this is AS_LAYER_SET_STATE
			DispatchMessage( msgCode );
			
				// ... and attach its children.
			for(int i = 0; i < childCount; i++){
				ses->ReadInt32( &msgCode );		// this is AS_LAYER_CREATE
				DispatchMessage( msgCode );
			}
			
			cl			= oldCL;
			
			STRACE(("DONE: ServerWindow %s: Message AS_CREATE_LAYER: Parent: %s, Child: %s\n", _title->String(), cl->_name->String(), name));
			break;
		}
		case AS_LAYER_DELETE:
		{
			// Received when a view is detached from a window. This is definitely
			// the less taxing operation - we call PruneTree() on the removed
			// layer, detach the layer itself, delete it, and invalidate the
			// area assuming that the view was visible when removed
			STRACE(("SW %s: AS_LAYER_DELETE(self)...\n", _title->String()));			
			Layer		*parent;
			parent		= cl->_parent;
			
				// here we remove current layer from list.
			cl->RemoveSelf( false );
			// TODO: invalidate the region occupied by this view.
			//			Should be done in Layer::RemoveChild() though!
			cl->PruneTree();

			parent->PrintTree();
			STRACE(("DONE: ServerWindow %s: Message AS_DELETE_LAYER: Parent: %s Layer: %s\n", _title->String(), parent->_name->String(), cl->_name->String()));

			delete cl;

			cl 			= parent;
			break;
		}
		case AS_LAYER_SET_STATE:
		{
			rgb_color		highColor,
							lowColor,
							viewColor;
			pattern			patt;
			int32			clippRegRects;

			ses->ReadPoint( &(cl->_layerdata->penlocation) );
			ses->ReadFloat( &(cl->_layerdata->pensize) );
			ses->ReadData( &highColor, sizeof(rgb_color) );
			ses->ReadData( &lowColor, sizeof(rgb_color) );
			ses->ReadData( &viewColor, sizeof(rgb_color) );
			ses->ReadData( &patt, sizeof(pattern) );	
			ses->ReadInt8( (int8*)&(cl->_layerdata->draw_mode) );
			ses->ReadPoint( &(cl->_layerdata->coordOrigin) );
			ses->ReadInt8( (int8*)&(cl->_layerdata->lineJoin) );
			ses->ReadInt8( (int8*)&(cl->_layerdata->lineCap) );
			ses->ReadFloat( &(cl->_layerdata->miterLimit) );
			ses->ReadInt8( (int8*)&(cl->_layerdata->alphaSrcMode) );
			ses->ReadInt8( (int8*)&(cl->_layerdata->alphaFncMode) );
			ses->ReadFloat( &(cl->_layerdata->scale) );
			ses->ReadBool( &(cl->_layerdata->fontAliasing) );
			ses->ReadInt32( &clippRegRects );
			
			cl->_layerdata->patt.Set( *((uint64*)&patt) );
			cl->_layerdata->highcolor.SetColor( highColor );
			cl->_layerdata->lowcolor.SetColor( lowColor );
			cl->_layerdata->viewcolor.SetColor( viewColor );

			if( clippRegRects != 0 ){
				if( cl->_layerdata->clippReg == NULL)
					cl->_layerdata->clippReg = new BRegion();
				else
					cl->_layerdata->clippReg->MakeEmpty();

				BRect		rect;
				
				for( int32 i = 0; i < clippRegRects; i++){
					ses->ReadRect( &rect );
					cl->_layerdata->clippReg->Include( rect );
				}
			}
			else{
				if ( cl->_layerdata->clippReg ){
					delete cl->_layerdata->clippReg;
					cl->_layerdata->clippReg = NULL;
				}
			}

			STRACE(("ServerWindow %s: Message AS_LAYER_SET_STATE: Layer: %s\n",_title->String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_SET_FONT_STATE:
		{
			uint16			mask;

			ses->ReadInt16( (int16*)&mask );
			
			if ( mask & B_FONT_FAMILY_AND_STYLE ){
				uint32		fontID;
				ses->ReadInt32( (int32*)&fontID );
				// TODO: implement later. Currently there is no SetFamAndStyle(uint32)
				//   in ServerFont class. DW, could you add one?
				//cl->_layerdata->font->
			}
	
			if ( mask & B_FONT_SIZE ){
				float		size;
				ses->ReadFloat( &size );
				cl->_layerdata->font.SetSize( size );
			}

			if ( mask & B_FONT_SHEAR ){
				float		shear;
				ses->ReadFloat( &shear );
				cl->_layerdata->font.SetShear( shear );
			}

			if ( mask & B_FONT_ROTATION ){
				float		rotation;
				ses->ReadFloat( &rotation );
				cl->_layerdata->font.SetRotation( rotation );
			}

			if ( mask & B_FONT_SPACING ){
				uint8		spacing;
				ses->ReadInt8( (int8*)&spacing );	// uint8
				cl->_layerdata->font.SetSpacing( spacing );
			}

			if ( mask & B_FONT_ENCODING ){
				uint8		encoding;
				ses->ReadInt8( (int8*)&encoding ); // uint8
				cl->_layerdata->font.SetEncoding( encoding );
			}

			if ( mask & B_FONT_FACE ){
				uint16		face;
				ses->ReadInt16( (int16*)&face );	// uint16
				cl->_layerdata->font.SetFace( face );
			}
	
			if ( mask & B_FONT_FLAGS ){
				uint32		flags;
				ses->ReadInt32( (int32*)&flags ); // uint32
				cl->_layerdata->font.SetFlags( flags );
			}

			STRACE(("ServerWindow %s: Message AS_LAYER_SET_FONT_STATE: Layer: %s\n",_title->String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_GET_STATE:
		{
			LayerData	*ld;
				// these 4 are here because of a compiler warning. Maybe he's right... :-)
			rgb_color	hc, lc, vc; // high, low and view colors
			uint64		patt;		// current pattern as a uint64

			ld			= cl->_layerdata; // now we write fewer characters. :-)
			hc			= ld->highcolor.GetColor32();
			lc			= ld->lowcolor.GetColor32();
			vc			= ld->viewcolor.GetColor32();
			patt		= ld->patt.GetInt64();
			
			// TODO: DW implement such a method in ServerFont class!
			ses->WriteUInt32( 0UL /*uint32 ld->font.GetFamAndStyle()*/ );
			ses->WriteFloat( ld->font.Size() );
			ses->WriteFloat( ld->font.Shear() );
			ses->WriteFloat( ld->font.Rotation() );
			ses->WriteUInt8( ld->font.Spacing() );
			ses->WriteUInt8( ld->font.Encoding() );
			ses->WriteUInt16( ld->font.Face() );
			ses->WriteUInt32( ld->font.Flags() );
			
			ses->WritePoint( ld->penlocation );
			ses->WriteFloat( ld->pensize );
			ses->WriteData( &hc, sizeof(rgb_color)/* 4*8 */ );
			ses->WriteData( &lc, sizeof(rgb_color)/* 4*8 */ );
			ses->WriteData( &vc, sizeof(rgb_color)/* 4*8 */ );
			ses->WriteData( &patt, sizeof(pattern)/* 8*8 */ );
			ses->WritePoint( ld->coordOrigin );
			ses->WriteUInt8( (uint8)(ld->draw_mode) );
			ses->WriteUInt8( (uint8)(ld->lineCap) );
			ses->WriteUInt8( (uint8)(ld->lineJoin) );
			ses->WriteFloat( ld->miterLimit );
			ses->WriteUInt8( (uint8)(ld->alphaSrcMode) );
			ses->WriteUInt8( (uint8)(ld->alphaFncMode) );
			ses->WriteFloat( ld->scale );
			ses->WriteBool( ld->fontAliasing );
			
			int32		noOfRects = 0;
			if ( ld->clippReg )
				noOfRects = ld->clippReg->CountRects();

			ses->WriteInt32( noOfRects );
			
			for( int i = 0; i < noOfRects; i++ ){
				ses->WriteRect( ld->clippReg->RectAt(i) );
			}
			
			ses->WriteFloat( cl->_frame.left );
			ses->WriteFloat( cl->_frame.top );
			ses->WriteRect( cl->_frame.OffsetToCopy( cl->_boundsLeftTop ) );
			
			ses->Sync();

			STRACE(("ServerWindow %s: Message AS_LAYER_GET_STATE: Layer: %s\n",_title->String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_MOVETO:
		{
			float		x, y;
			
			ses->ReadFloat( &x );
			ses->ReadFloat( &y );
			
			cl->DoMoveTo(x, y);

			STRACE(("ServerWindow %s: Message AS_LAYER_MOVETO: Layer: %s\n",_title->String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_RESIZETO:
		{
			float		newWidth, newHeight;
			
			ses->ReadFloat( &newWidth );
			ses->ReadFloat( &newHeight );
		/* TODO: check for minimum alowed. WinBorder should provide such
		 *	a method, based on its decorator.
		 */
			cl->DoResizeTo( newWidth, newHeight );

			STRACE(("ServerWindow %s: Message AS_LAYER_RESIZETO: Layer: %s\n",_title->String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_GET_COORD:
		{
			ses->WriteFloat( cl->_frame.left );
			ses->WriteFloat( cl->_frame.top );
			ses->WriteRect( cl->_frame.OffsetToCopy( cl->_boundsLeftTop ) );
			ses->Sync();

			STRACE(("ServerWindow %s: Message AS_LAYER_GET_COORD: Layer: %s\n",_title->String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_SET_FLAGS:
		{
			ses->ReadUInt32( &(cl->_flags) );
			
			STRACE(("ServerWindow %s: Message AS_LAYER_SET_FLAGS: Layer: %s\n",_title->String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_HIDE:
		{
			cl->Hide();
			
			STRACE(("ServerWindow %s: Message AS_LAYER_HIDE: Layer: %s\n",_title->String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_SHOW:
		{
			cl->Show();
			
			STRACE(("ServerWindow %s: Message AS_LAYER_SHOW: Layer: %s\n",_title->String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_SET_LINE_MODE:
		{
			int8		lineCap, lineJoin;
/* TODO: speak with DW! Shouldn't we lock before modifying certain memebers?
 *	Because redraw code might use an updated value instead of one for witch
 *		it was called. e.g.: different lineCap or lineJoin. Strange result
 *		would appear.
 */
			ses->ReadInt8( &lineCap );
			ses->ReadInt8( &lineJoin );
			ses->ReadFloat( &(cl->_layerdata->miterLimit) );
			
			cl->_layerdata->lineCap		= (cap_mode)lineCap;
			cl->_layerdata->lineJoin	= (join_mode)lineJoin;
		
			STRACE(("ServerWindow %s: Message AS_LAYER_SET_LINE_MODE: Layer: %s\n",_title->String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_GET_LINE_MODE:
		{
			ses->WriteInt8( (int8)(cl->_layerdata->lineCap) );
			ses->WriteInt8( (int8)(cl->_layerdata->lineJoin) );
			ses->WriteFloat( cl->_layerdata->miterLimit );
			ses->Sync();
		
			STRACE(("ServerWindow %s: Message AS_LAYER_GET_LINE_MODE: Layer: %s\n",_title->String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_PUSH_STATE:
		{
			LayerData		*ld = new LayerData();
			ld->prevState	= cl->_layerdata;
			cl->_layerdata	= ld;
			
			STRACE(("ServerWindow %s: Message AS_LAYER_PUSH_STATE: Layer: %s\n",_title->String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_POP_STATE:
		{
			if ( !(cl->_layerdata->prevState) ) {
				STRACE(("WARNING: SW(%s): User called BView(%s)::PopState(), but there is NO state on stack!\n", _title->String(), cl->_name->String()));
				break;
			}

			LayerData		*ld = cl->_layerdata;
			cl->_layerdata	= cl->_layerdata->prevState;
			delete ld;
		
			STRACE(("ServerWindow %s: Message AS_LAYER_POP_STATE: Layer: %s\n",_title->String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_SET_SCALE:
		{
			ses->ReadFloat( &(cl->_layerdata->scale) );
		
			STRACE(("ServerWindow %s: Message AS_LAYER_SET_SCALE: Layer: %s\n",_title->String(), cl->_name->String()));		
			break;
		}
		case AS_LAYER_GET_SCALE:
		{
			LayerData		*ld = cl->_layerdata;
			float			scale = ld->scale;

			while( (ld = ld->prevState) )
				scale		*= ld->scale;
			
			ses->WriteFloat( scale );
			ses->Sync();
		
			STRACE(("ServerWindow %s: Message AS_LAYER_SET_SCALE: Layer: %s\n",_title->String(), cl->_name->String()));		
			break;
		}
		case AS_LAYER_SET_PEN_LOC:
		{
			float		x, y;
			
			ses->ReadFloat( &x );
			ses->ReadFloat( &y );
			
			cl->_layerdata->penlocation.Set( x, y );
		
			STRACE(("ServerWindow %s: Message AS_LAYER_SET_PEN_LOC: Layer: %s\n",_title->String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_GET_PEN_LOC:
		{
			ses->WritePoint( cl->_layerdata->penlocation );
			ses->Sync();
		
			STRACE(("ServerWindow %s: Message AS_LAYER_GET_PEN_LOC: Layer: %s\n",_title->String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_SET_PEN_SIZE:
		{
			ses->ReadFloat( &(cl->_layerdata->pensize) );
		
			STRACE(("ServerWindow %s: Message AS_LAYER_SET_PEN_SIZE: Layer: %s\n",_title->String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_GET_PEN_SIZE:
		{
			ses->WriteFloat( cl->_layerdata->pensize );
			ses->Sync();
		
			STRACE(("ServerWindow %s: Message AS_LAYER_GET_PEN_SIZE: Layer: %s\n",_title->String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_SET_HIGH_COLOR:
		{
			rgb_color		c;
			
			ses->ReadData( &c, sizeof(rgb_color) );
			
			cl->_layerdata->highcolor.SetColor( c );
		
			STRACE(("ServerWindow %s: Message AS_LAYER_SET_HIGH_COLOR: Layer: %s\n",_title->String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_SET_LOW_COLOR:
		{
			rgb_color		c;
			
			ses->ReadData( &c, sizeof(rgb_color) );
			
			cl->_layerdata->lowcolor.SetColor( c );
		
			STRACE(("ServerWindow %s: Message AS_LAYER_SET_LOW_COLOR: Layer: %s\n",_title->String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_SET_VIEW_COLOR:
		{
			rgb_color		c;
			
			ses->ReadData( &c, sizeof(rgb_color) );
			
			cl->_layerdata->viewcolor.SetColor( c );
		
			STRACE(("ServerWindow %s: Message AS_LAYER_SET_VIEW_COLOR: Layer: %s\n",_title->String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_GET_COLORS:
		{
			rgb_color		highColor, lowColor, viewColor;
			
			highColor		= cl->_layerdata->highcolor.GetColor32();
			lowColor		= cl->_layerdata->lowcolor.GetColor32();
			viewColor		= cl->_layerdata->viewcolor.GetColor32();
			
			ses->WriteData( &highColor, sizeof(rgb_color) );
			ses->WriteData( &lowColor, sizeof(rgb_color) );
			ses->WriteData( &viewColor, sizeof(rgb_color) );
			ses->Sync();

			STRACE(("ServerWindow %s: Message AS_LAYER_GET_COLORS: Layer: %s\n",_title->String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_SET_BLEND_MODE:
		{
			int8		srcAlpha, alphaFunc;
			
			ses->ReadInt8( &srcAlpha );
			ses->ReadInt8( &alphaFunc );
			
			cl->_layerdata->alphaSrcMode	= (source_alpha)srcAlpha;
			cl->_layerdata->alphaFncMode	= (alpha_function)alphaFunc;

			STRACE(("ServerWindow %s: Message AS_LAYER_SET_BLEND_MODE: Layer: %s\n",_title->String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_GET_BLEND_MODE:
		{
			ses->WriteInt8( (int8)(cl->_layerdata->alphaSrcMode) );
			ses->WriteInt8( (int8)(cl->_layerdata->alphaFncMode) );
			ses->Sync();

			STRACE(("ServerWindow %s: Message AS_LAYER_GET_BLEND_MODE: Layer: %s\n",_title->String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_SET_DRAW_MODE:
		{
			int8		drawingMode;
			
			ses->ReadInt8( &drawingMode );
			
			cl->_layerdata->draw_mode	= (drawing_mode)drawingMode;

			STRACE(("ServerWindow %s: Message AS_LAYER_SET_DRAW_MODE: Layer: %s\n",_title->String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_GET_DRAW_MODE:
		{
			ses->WriteInt8( (int8)(cl->_layerdata->draw_mode) );
			ses->Sync();
		
			STRACE(("ServerWindow %s: Message AS_LAYER_GET_DRAW_MODE: Layer: %s\n",_title->String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_PRINT_ALIASING:
		{
			ses->ReadBool( &(cl->_layerdata->fontAliasing) );
		
			STRACE(("ServerWindow %s: Message AS_LAYER_PRINT_ALIASING: Layer: %s\n",_title->String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_CLIP_TO_PICTURE:
		{
			int32		pictureToken;
			BPoint		where;
			
			ses->ReadInt32( &pictureToken );
			ses->ReadPoint( &where );

			ServerPicture	*sp = NULL;
			int32			i = 0;

			for(;;){
				sp		= static_cast<ServerPicture*>(cl->_serverwin->_app->_piclist->ItemAt(i++));
				if (!sp)
					break;
					
				if( sp->GetToken() == pictureToken ){
					cl->_layerdata->clippPicture	= sp;
// TODO: increase that picture's reference count.( ~ allocate a picture )
					break;
				}
			}
				// avoid compiler warning
			i = 0;

			cl->_layerdata->clippInverse	= false;

// TODO: redraw.
		
			STRACE(("ServerWindow %s: Message AS_LAYER_CLIP_TO_PICTURE: Layer: %s\n",_title->String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_CLIP_TO_INVERSE_PICTURE:
		{
			int32		pictureToken;
			BPoint		where;
			
			ses->ReadInt32( &pictureToken );
			ses->ReadPoint( &where );

			ServerPicture	*sp = NULL;
			int32			i = 0;

			for(;;){
				sp		= static_cast<ServerPicture*>(cl->_serverwin->_app->_piclist->ItemAt(i++));
				if (!sp)
					break;
					
				if( sp->GetToken() == pictureToken ){
					cl->_layerdata->clippPicture	= sp;
// TODO: increase that picture's reference count.( ~ allocate a picture )
					break;
				}
			}
				// avoid compiler warning
			i = 0;
			
			cl->_layerdata->clippInverse	= true;

// TODO: redraw.

			STRACE(("ServerWindow %s: Message AS_LAYER_CLIP_TO_INVERSE_PICTURE: Layer: %s\n",_title->String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_GET_CLIP_REGION:
		{
			BRegion		reg;
			LayerData	*ld;
			int32		noOfRects;
		// TODO: I think locking is necesary
			ld			= cl->_layerdata;
			reg			= cl->ConvertFromParent( cl->_visible );

			if( ld->clippReg )
				reg.IntersectWith( ld->clippReg );

			while( (ld = ld->prevState) )
				if( ld->clippReg )
					reg.IntersectWith( ld->clippReg );

			noOfRects	= reg.CountRects();
			ses->WriteInt32( noOfRects );

			for( int i = 0; i < noOfRects; i++){
				ses->WriteRect( reg.RectAt(i) );
			}

			ses->Sync();
		
			STRACE(("ServerWindow %s: Message AS_LAYER_GET_CLIP_REGION: Layer: %s\n",_title->String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_SET_CLIP_REGION:
		{
			int32		noOfRects;
			BRect		r;
		// TODO: I think locking is necesary
			if( cl->_layerdata->clippReg )
				cl->_layerdata->clippReg->MakeEmpty();
			else
				cl->_layerdata->clippReg = new BRegion();
			
			ses->ReadInt32( &noOfRects );
			
			for( int i = 0; i < noOfRects; i++ ){
				ses->ReadRect( &r );
				cl->_layerdata->clippReg->Include( r );
			}
			
// TODO: redraw.

			STRACE(("ServerWindow %s: Message AS_LAYER_SET_CLIP_REGION: Layer: %s\n",_title->String(), cl->_name->String()));
			break;
		}

		
	/********** END: BView Messages ***********/
	
	/********** BWindow Messages ***********/
		case AS_LAYER_CREATE_ROOT:
		{
			// Received when a window creates its internal top view
			int32			token;
			BRect			frame;
			uint32			resizeMode;
			uint32			flags;
			char*			name = NULL;
			
			ses->ReadInt32( &token );
			ses->ReadRect( &frame );
			ses->ReadInt32( (int32*)&resizeMode );
			ses->ReadInt32( (int32*)&flags );
			name			= ses->ReadString();
			
			top_layer		= new Layer( frame, name, token, resizeMode,
											flags, this );
			cl				= top_layer;
			
			STRACE(("ServerWindow %s: Setting currentLayer to: '%s'\n", _title->String(), name ));
			STRACE(("ServerWindow %s: Message Create_Layer_Root\n",_title->String()));
			delete name;
			break;
		}
		case AS_LAYER_DELETE_ROOT:
		{
			// Received when a window deletes its internal top view
			
			// TODO: Implement
			STRACE(("ServerWindow %s: Message Delete_Layer_Root unimplemented\n",_title->String()));

			break;
		}
		case AS_SHOW_WINDOW:
		{
			STRACE(("ServerWindow %s: Message AS_SHOW\n",_title->String()));
			Show();
			break;
		}
		case AS_HIDE_WINDOW:
		{
			STRACE(("ServerWindow %s: Message AS_HIDE\n",_title->String()));		
			Hide();
			break;
		}
		case AS_SEND_BEHIND:
		{
			// TODO: Implement
			STRACE(("ServerWindow %s: Message  Send_Behind unimplemented\n",_title->String()));
			break;
		}
		case AS_ENABLE_UPDATES:
		{
			// TODO: Implement
			STRACE(("ServerWindow %s: Message Enable_Updates unimplemented\n",_title->String()));
			break;
		}
		case AS_DISABLE_UPDATES:
		{
			// TODO: Implement
			STRACE(("ServerWindow %s: Message Disable_Updates unimplemented\n",_title->String()));
			break;
		}
		case AS_NEEDS_UPDATE:
		{
			// TODO: Implement
			STRACE(("ServerWindow %s: Message Needs_Update unimplemented\n",_title->String()));
			break;
		}
		case AS_WINDOW_TITLE:
		{
			// TODO: Implement
			STRACE(("ServerWindow %s: Message Set_Title unimplemented\n",_title->String()));
			break;
		}
		case AS_ADD_TO_SUBSET:
		{
			// TODO: Implement
			STRACE(("ServerWindow %s: Message Add_To_Subset unimplemented\n",_title->String()));
			break;
		}
		case AS_REM_FROM_SUBSET:
		{
			// TODO: Implement
			STRACE(("ServerWindow %s: Message Remove_From_Subset unimplemented\n",_title->String()));
			break;
		}
		case AS_SET_LOOK:
		{
			// TODO: Implement
			STRACE(("ServerWindow %s: Message Set_Look unimplemented\n",_title->String()));
			break;
		}
		case AS_SET_FLAGS:
		{
			// TODO: Implement
			STRACE(("ServerWindow %s: Message Set_Flags unimplemented\n",_title->String()));
			break;
		}
		case AS_SET_FEEL:
		{
			// TODO: Implement
			STRACE(("ServerWindow %s: Message Set_Feel unimplemented\n",_title->String()));
			break;
		}
		case AS_SET_ALIGNMENT:
		{
			// TODO: Implement
			STRACE(("ServerWindow %s: Message Set_Alignment unimplemented\n",_title->String()));
			break;
		}
		case AS_GET_ALIGNMENT:
		{
			// TODO: Implement
			STRACE(("ServerWindow %s: Message Get_Alignment unimplemented\n",_title->String()));
			break;
		}
		case AS_GET_WORKSPACES:
		{
			// TODO: Implement
			STRACE(("ServerWindow %s: Message Get_Workspaces unimplemented\n",_title->String()));
			break;
		}
		case AS_SET_WORKSPACES:
		{
			// TODO: Implement
			STRACE(("ServerWindow %s: Message Set_Workspaces unimplemented\n",_title->String()));
			break;
		}
		case AS_WINDOW_RESIZE:
		{
			// TODO: Implement
			STRACE(("ServerWindow %s: Message Resize unimplemented\n",_title->String()));
			break;
		}
		case B_MINIMIZE:
		{
			// TODO: Implement
			STRACE(("ServerWindow %s: Message Minimize unimplemented\n",_title->String()));
			break;
		}
		case B_WINDOW_ACTIVATED:
		{
			// TODO: Implement
			STRACE(("ServerWindow %s: Message Window_Activated unimplemented\n",_title->String()));
			break;
		}
		case B_ZOOM:
		{
			// TODO: Implement
			STRACE(("ServerWindow %s: Message Zoom unimplemented\n",_title->String()));
			break;
		}
		case B_WINDOW_MOVE_TO:
		{
			// TODO: Implement
			STRACE(("ServerWindow %s: Message Move_To unimplemented\n",_title->String()));
			break;
		}
		case B_WINDOW_MOVE_BY:
		{
			// TODO: Implement
			STRACE(("ServerWindow %s: Message Move_By unimplemented\n",_title->String()));
			break;
		}
		default:
		{
			printf("ServerWindow %s received unexpected code - message offset %lx\n",_title->String(), code - SERVER_TRUE);
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
	Layer *layer;
	LayerData *layerdata;
	int32 code;
	int32 view_token;
	uint32 sizeRemaining = (uint32)msgsize;
	BRegion WindowClipRegion;
	BRegion LayerClipRegion;
	Layer *sibling;
	int32 numRects = 0;
	
	if ( !msgsize || !msgbuffer )
		return;
	if ( IsHidden() )
		return;

	WindowClipRegion.Set(_winborder->Frame());
	sibling = _winborder->GetUpperSibling();
	while ( sibling )
	{
		WindowClipRegion.Exclude(sibling->Frame());
		sibling = sibling->GetUpperSibling();
	}

	if ( !WindowClipRegion.Frame().IsValid() )
		return;
	
	// We need to decide whether coordinates are specified in view or root coordinates.
	// For now, we assume root level coordinates.
	code = AS_BEGIN_UPDATE;
	while ((sizeRemaining > 2*sizeof(int32)) && (code != AS_END_UPDATE))
	{
		code = read_from_buffer<int32>(&msgbuffer);
		view_token = read_from_buffer<int32>(&msgbuffer);
		layer = _workspace->GetRoot()->FindLayer(view_token);
		if ( layer )
		{
			layerdata = layer->GetLayerData();
			LayerClipRegion.Set(layer->Frame());
			LayerClipRegion.IntersectWith(&WindowClipRegion);
			numRects = LayerClipRegion.CountRects();
		}
		else
		{
			layerdata = NULL;
			printf("ServerWindow %s received invalid view token %lx",_title->String(),view_token);
		}
		switch (code)
		{
			case AS_SET_HIGH_COLOR:
			{
				if ( sizeRemaining >= AS_SET_COLOR_MSG_SIZE )
				{
					uint8 r, g, b, a;
					r = read_from_buffer<uint8>(&msgbuffer);
					g = read_from_buffer<uint8>(&msgbuffer);
					b = read_from_buffer<uint8>(&msgbuffer);
					a = read_from_buffer<uint8>(&msgbuffer);
					if ( layerdata )
						layerdata->highcolor.SetColor(r,g,b,a);
					sizeRemaining -= AS_SET_COLOR_MSG_SIZE;
				}
				else
				{
					printf("ServerWindow %s received truncated graphics code %lx",_title->String(),code);
					sizeRemaining = 0;
				}
				break;
			}
			case AS_SET_LOW_COLOR:
			{
				if ( sizeRemaining >= AS_SET_COLOR_MSG_SIZE )
				{
					uint8 r, g, b, a;
					r = read_from_buffer<uint8>(&msgbuffer);
					g = read_from_buffer<uint8>(&msgbuffer);
					b = read_from_buffer<uint8>(&msgbuffer);
					a = read_from_buffer<uint8>(&msgbuffer);
					if ( layerdata )
						layerdata->lowcolor.SetColor(r,g,b,a);
					sizeRemaining -= AS_SET_COLOR_MSG_SIZE;
				}
				else
				{
					printf("ServerWindow %s received truncated graphics code %lx",_title->String(),code);
					sizeRemaining = 0;
				}
				break;
			}
			case AS_SET_VIEW_COLOR:
			{
				if ( sizeRemaining >= AS_SET_COLOR_MSG_SIZE )
				{
					uint8 r, g, b, a;
					r = read_from_buffer<uint8>(&msgbuffer);
					g = read_from_buffer<uint8>(&msgbuffer);
					b = read_from_buffer<uint8>(&msgbuffer);
					a = read_from_buffer<uint8>(&msgbuffer);
					if ( layerdata )
						layerdata->viewcolor.SetColor(r,g,b,a);
					sizeRemaining -= AS_SET_COLOR_MSG_SIZE;
				}
				else
				{
					printf("ServerWindow %s received truncated graphics code %lx",_title->String(),code);
					sizeRemaining = 0;
				}
				break;
			}
			case AS_STROKE_ARC:
			{
				// TODO:: Add clipping
				if ( sizeRemaining >= AS_STROKE_ARC_MSG_SIZE )
				{
					float left, top, right, bottom, angle, span;
					int8 *pattern;

					left = read_from_buffer<float>(&msgbuffer);
					top = read_from_buffer<float>(&msgbuffer);
					right = read_from_buffer<float>(&msgbuffer);
					bottom = read_from_buffer<float>(&msgbuffer);
					angle = read_from_buffer<float>(&msgbuffer);
					span = read_from_buffer<float>(&msgbuffer);
					pattern = read_pattern_from_buffer(&msgbuffer);
					BRect rect(left,top,right,bottom);
					if ( layerdata )
						_app->_driver->StrokeArc(rect,angle,span,layerdata,pattern);
					sizeRemaining -= AS_STROKE_ARC_MSG_SIZE;
				}
				else
				{
					printf("ServerWindow %s received truncated graphics code %lx",_title->String(),code);
					sizeRemaining = 0;
				}
				break;
			}
			case AS_STROKE_BEZIER:
			{
				// TODO:: Add clipping
				if ( sizeRemaining >= AS_STROKE_BEZIER_MSG_SIZE )
				{
					BPoint *pts;
					int8 *pattern;
					int i;
					pts = new BPoint[4];
					for (i=0; i<4; i++)
					{
						pts[i].x = read_from_buffer<float>(&msgbuffer);
						pts[i].y = read_from_buffer<float>(&msgbuffer);
					}
					pattern = read_pattern_from_buffer(&msgbuffer);
					if ( layerdata )
						_app->_driver->StrokeBezier(pts,layerdata,pattern);
					delete[] pts;
					sizeRemaining -= AS_STROKE_BEZIER_MSG_SIZE;
				}
				else
				{
					printf("ServerWindow %s received truncated graphics code %lx",_title->String(),code);
					sizeRemaining = 0;
				}
				break;
			}
			case AS_STROKE_ELLIPSE:
			{
				// TODO:: Add clipping
				if ( sizeRemaining >= AS_STROKE_ELLIPSE_MSG_SIZE )
				{
					float left, top, right, bottom;
					int8 *pattern;
					left = read_from_buffer<float>(&msgbuffer);
					top = read_from_buffer<float>(&msgbuffer);
					right = read_from_buffer<float>(&msgbuffer);
					bottom = read_from_buffer<float>(&msgbuffer);
					pattern = read_pattern_from_buffer(&msgbuffer);
					BRect rect(left,top,right,bottom);
					if ( layerdata )
						_app->_driver->StrokeEllipse(rect,layerdata,pattern);
					sizeRemaining -= AS_STROKE_ELLIPSE_MSG_SIZE;
				}
				else
				{
					printf("ServerWindow %s received truncated graphics code %lx",_title->String(),code);
					sizeRemaining = 0;
				}
				break;
			}
			case AS_STROKE_LINE:
			{
				// TODO:: Add clipping
				if ( sizeRemaining >= AS_STROKE_LINE_MSG_SIZE )
				{
					float x1, y1, x2, y2;
					int8 *pattern;
					x1 = read_from_buffer<float>(&msgbuffer);
					y1 = read_from_buffer<float>(&msgbuffer);
					x2 = read_from_buffer<float>(&msgbuffer);
					y2 = read_from_buffer<float>(&msgbuffer);
					pattern = read_pattern_from_buffer(&msgbuffer);
					BPoint p1(x1,y1);
					BPoint p2(x2,y2);
					if ( layerdata )
						_app->_driver->StrokeLine(p1,p2,layerdata,pattern);
					sizeRemaining -= AS_STROKE_LINE_MSG_SIZE;
				}
				else
				{
					printf("ServerWindow %s received truncated graphics code %lx",_title->String(),code);
					sizeRemaining = 0;
				}
				break;
			}
			case AS_STROKE_LINEARRAY:
			{
				// TODO: Implement
				break;
			}
			case AS_STROKE_POLYGON:
			{
				// TODO: Implement
				break;
			}
			case AS_STROKE_RECT:
			{
				// TODO:: Add clipping
				if ( sizeRemaining >= AS_STROKE_RECT_MSG_SIZE )
				{
					float left, top, right, bottom;
					int8 *pattern;
					left = read_from_buffer<float>(&msgbuffer);
					top = read_from_buffer<float>(&msgbuffer);
					right = read_from_buffer<float>(&msgbuffer);
					bottom = read_from_buffer<float>(&msgbuffer);
					pattern = read_pattern_from_buffer(&msgbuffer);
					BRect rect(left,top,right,bottom);
					if ( layerdata )
						_app->_driver->StrokeRect(rect,layerdata,pattern);
					sizeRemaining -= AS_STROKE_RECT_MSG_SIZE;
				}
				else
				{
					printf("ServerWindow %s received truncated graphics code %lx",_title->String(),code);
					sizeRemaining = 0;
				}
				break;
			}
			case AS_STROKE_ROUNDRECT:
			{
				// TODO:: Add clipping
				if ( sizeRemaining >= AS_STROKE_ROUNDRECT_MSG_SIZE )
				{
					float left, top, right, bottom, xrad, yrad;
					int8 *pattern;
					left = read_from_buffer<float>(&msgbuffer);
					top = read_from_buffer<float>(&msgbuffer);
					right = read_from_buffer<float>(&msgbuffer);
					bottom = read_from_buffer<float>(&msgbuffer);
					xrad = read_from_buffer<float>(&msgbuffer);
					yrad = read_from_buffer<float>(&msgbuffer);
					pattern = read_pattern_from_buffer(&msgbuffer);
					BRect rect(left,top,right,bottom);
					if ( layerdata )
						_app->_driver->StrokeRoundRect(rect,xrad,yrad,layerdata,pattern);
					sizeRemaining -= AS_STROKE_ROUNDRECT_MSG_SIZE;
				}
				else
				{
					printf("ServerWindow %s received truncated graphics code %lx",_title->String(),code);
					sizeRemaining = 0;
				}
				break;
			}
			case AS_STROKE_SHAPE:
			{
				// TODO: Implement
				break;
			}
			case AS_STROKE_TRIANGLE:
			{
				// TODO:: Add clipping
				if ( sizeRemaining >= AS_STROKE_TRIANGLE_MSG_SIZE )
				{
					BPoint *pts;
					int8 *pattern;
					float left, top, right, bottom;
					int i;
					pts = new BPoint[3];
					for (i=0; i<3; i++)
					{
						pts[i].x = read_from_buffer<float>(&msgbuffer);
						pts[i].y = read_from_buffer<float>(&msgbuffer);
					}
					left = read_from_buffer<float>(&msgbuffer);
					top = read_from_buffer<float>(&msgbuffer);
					right = read_from_buffer<float>(&msgbuffer);
					bottom = read_from_buffer<float>(&msgbuffer);
					pattern = read_pattern_from_buffer(&msgbuffer);
					BRect rect(left,top,right,bottom);
					if ( layerdata )
						_app->_driver->StrokeTriangle(pts,rect,layerdata,pattern);
					delete[] pts;
					sizeRemaining -= AS_STROKE_TRIANGLE_MSG_SIZE;
				}
				else
				{
					printf("ServerWindow %s received truncated graphics code %lx",_title->String(),code);
					sizeRemaining = 0;
				}
				break;
			}
			case AS_FILL_ARC:
			{
				// TODO:: Add clipping
				if ( sizeRemaining >= AS_FILL_ARC_MSG_SIZE )
				{
					float left, top, right, bottom, angle, span;
					int8 *pattern;
					left = read_from_buffer<float>(&msgbuffer);
					top = read_from_buffer<float>(&msgbuffer);
					right = read_from_buffer<float>(&msgbuffer);
					bottom = read_from_buffer<float>(&msgbuffer);
					angle = read_from_buffer<float>(&msgbuffer);
					span = read_from_buffer<float>(&msgbuffer);
					pattern = read_pattern_from_buffer(&msgbuffer);
					BRect rect(left,top,right,bottom);
					if ( layerdata )
						_app->_driver->FillArc(rect,angle,span,layerdata,pattern);
					sizeRemaining -= AS_FILL_ARC_MSG_SIZE;
				}
				else
				{
					printf("ServerWindow %s received truncated graphics code %lx",_title->String(),code);
					sizeRemaining = 0;
				}
				break;
			}
			case AS_FILL_BEZIER:
			{
				// TODO:: Add clipping
				if ( sizeRemaining >= AS_FILL_BEZIER_MSG_SIZE )
				{
					BPoint *pts;
					int8 *pattern;
					int i;
					pts = new BPoint[4];
					for (i=0; i<4; i++)
					{
						pts[i].x = read_from_buffer<float>(&msgbuffer);
						pts[i].y = read_from_buffer<float>(&msgbuffer);
					}
					pattern = read_pattern_from_buffer(&msgbuffer);
					if ( layerdata )
						_app->_driver->FillBezier(pts,layerdata,pattern);
					delete[] pts;
					sizeRemaining -= AS_FILL_BEZIER_MSG_SIZE;
				}
				else
				{
					printf("ServerWindow %s received truncated graphics code %lx",_title->String(),code);
					sizeRemaining = 0;
				}
				break;
			}
			case AS_FILL_ELLIPSE:
			{
				// TODO:: Add clipping
				if ( sizeRemaining >= AS_FILL_ELLIPSE_MSG_SIZE )
				{
					float left, top, right, bottom;
					int8 *pattern;
					left = read_from_buffer<float>(&msgbuffer);
					top = read_from_buffer<float>(&msgbuffer);
					right = read_from_buffer<float>(&msgbuffer);
					bottom = read_from_buffer<float>(&msgbuffer);
					pattern = read_pattern_from_buffer(&msgbuffer);
					BRect rect(left,top,right,bottom);
					if ( layerdata )
						_app->_driver->FillEllipse(rect,layerdata,pattern);
					sizeRemaining -= AS_FILL_ELLIPSE_MSG_SIZE;
				}
				else
				{
					printf("ServerWindow %s received truncated graphics code %lx",_title->String(),code);
					sizeRemaining = 0;
				}
				break;
			}
			case AS_FILL_POLYGON:
			{
				// TODO: Implement
				break;
			}
			case AS_FILL_RECT:
			{
				if ( sizeRemaining >= AS_FILL_RECT_MSG_SIZE )
				{
					float left, top, right, bottom;
					int8 *pattern;
					left = read_from_buffer<float>(&msgbuffer);
					top = read_from_buffer<float>(&msgbuffer);
					right = read_from_buffer<float>(&msgbuffer);
					bottom = read_from_buffer<float>(&msgbuffer);
					pattern = read_pattern_from_buffer(&msgbuffer);
					BRect rect(left,top,right,bottom);
					if ( layerdata && numRects )
						if ( numRects == 1 )
							_app->_driver->FillRect(rect,layerdata,pattern);
						else
						{
							int i;
							for (i=0; i<numRects; i++)
								_app->_driver->FillRect(LayerClipRegion.RectAt(i),layerdata,pattern);
						}
					sizeRemaining -= AS_FILL_RECT_MSG_SIZE;
				}
				else
				{
					printf("ServerWindow %s received truncated graphics code %lx",_title->String(),code);
					sizeRemaining = 0;
				}
				break;
			}
			case AS_FILL_REGION:
			{
				// TODO: Implement
				break;
			}
			case AS_FILL_ROUNDRECT:
			{
				// TODO:: Add clipping
				if ( sizeRemaining >= AS_FILL_ROUNDRECT_MSG_SIZE )
				{
					float left, top, right, bottom, xrad, yrad;
					int8 *pattern;
					left = read_from_buffer<float>(&msgbuffer);
					top = read_from_buffer<float>(&msgbuffer);
					right = read_from_buffer<float>(&msgbuffer);
					bottom = read_from_buffer<float>(&msgbuffer);
					xrad = read_from_buffer<float>(&msgbuffer);
					yrad = read_from_buffer<float>(&msgbuffer);
					pattern = read_pattern_from_buffer(&msgbuffer);
					BRect rect(left,top,right,bottom);
					if ( layerdata )
						_app->_driver->FillRoundRect(rect,xrad,yrad,layerdata,pattern);
					sizeRemaining -= AS_FILL_ROUNDRECT_MSG_SIZE;
				}
				else
				{
					printf("ServerWindow %s received truncated graphics code %lx",_title->String(),code);
					sizeRemaining = 0;
				}
				break;
			}
			case AS_FILL_SHAPE:
			{
				// TODO: Implement
				break;
			}
			case AS_FILL_TRIANGLE:
			{
				// TODO:: Add clipping
				if ( sizeRemaining >= AS_FILL_TRIANGLE_MSG_SIZE )
				{
					BPoint *pts;
					int8 *pattern;
					float left, top, right, bottom;
					int i;
					pts = new BPoint[3];
					for (i=0; i<3; i++)
					{
						pts[i].x = read_from_buffer<float>(&msgbuffer);
						pts[i].y = read_from_buffer<float>(&msgbuffer);
					}
					left = read_from_buffer<float>(&msgbuffer);
					top = read_from_buffer<float>(&msgbuffer);
					right = read_from_buffer<float>(&msgbuffer);
					bottom = read_from_buffer<float>(&msgbuffer);
					pattern = read_pattern_from_buffer(&msgbuffer);
					BRect rect(left,top,right,bottom);
					if ( layerdata )
						_app->_driver->FillTriangle(pts,rect,layerdata,pattern);
					delete[] pts;
					sizeRemaining -= AS_FILL_TRIANGLE_MSG_SIZE;
				}
				else
				{
					printf("ServerWindow %s received truncated graphics code %lx",_title->String(),code);
					sizeRemaining = 0;
				}
				break;
			}
			case AS_MOVEPENBY:
			{
				// TODO: Implement
				break;
			}
			case AS_MOVEPENTO:
			{
				// TODO: Implement
				break;
			}
			case AS_SETPENSIZE:
			{
				// TODO: Implement
				break;
			}
			case AS_DRAW_STRING:
			{
				// TODO: Implement
				break;
			}
			case AS_SET_FONT:
			{
				// TODO: Implement
				break;
			}
			case AS_SET_FONT_SIZE:
			{
				// TODO: Implement
				break;
			}
			default:
			{
				sizeRemaining -= sizeof(int32);
				printf("ServerWindow %s received unexpected graphics code %lx",_title->String(),code);
				break;
			}
		}
	}
}

/*!
	\brief Message-dispatching loop for the ServerWindow

	MonitorWin() watches the ServerWindow's message port and dispatches as necessary
	\param data The thread's ServerWindow
	\return Throwaway code. Always 0.
*/
int32 ServerWindow::MonitorWin(void *data)
{
	ServerWindow 	*win = (ServerWindow *)data;
	status_t		rv;
	int32			code;
	
	for(;;)
	{
		rv = win->ses->ReadInt32( &code );
		
		if ( rv != B_BAD_PORT_ID ){
			switch( code ){
				case B_QUIT_REQUESTED:{
					STRACE(("ServerWindow %s received Quit request\n",win->Title()));
					// Our BWindow sent us this message when it quit.
					// We need to ask its ServerApp to delete our monitor
					win->_applink->SetOpCode(AS_DELETE_WINDOW);
					win->_applink->Attach<int32>(win->_token);
					win->_applink->Flush();
					break;
				}
				default:{
					win->DispatchMessage( code );
					break;
				}
			}
		}
		else{
			exit_thread(1);
			return 1;
		}
	
		if( code == B_QUIT_REQUESTED )
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
//void ServerWindow::HandleMouseEvent(int32 code, int8 *buffer)
void ServerWindow::HandleMouseEvent(PortMessage *msg)
{
	ServerWindow *mousewin=NULL;
//	int8 *index=buffer;
	
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
//	switch(code)
	switch(msg->Code())
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

//			index+=sizeof(int64);
//			float x=*((float*)index); index+=sizeof(float);
//			float y=*((float*)index); index+=sizeof(float);
//			BPoint pt(x,y);
			float x;
			float y;
			int64 dummy;
			msg->Read<int64>(&dummy);
			msg->Read<float>(&x);
			msg->Read<float>(&y);
			BPoint pt(x,y);
			
			// If we have clicked on a window, 			
			active_winborder = _winborder = WindowContainsPoint(pt);
			if(_winborder)
			{
				mousewin=_winborder->Window();
				_winborder->MouseDown((int8*)msg->Buffer());
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

//			index+=sizeof(int64);
//			float x=*((float*)index); index+=sizeof(float);
//			float y=*((float*)index); index+=sizeof(float);
//			BPoint pt(x,y);
			float x;
			float y;
			int64 dummy;
			msg->Read(&dummy);
			msg->Read(&x);
			msg->Read(&y);
			BPoint pt(x,y);
			
			set_is_sliding_tab(false);
			set_is_moving_window(false);
			set_is_resizing_window(false);
			_winborder	= WindowContainsPoint(pt);
			active_winborder=NULL;
			if(_winborder)
			{
				mousewin=_winborder->Window();
				
				// Eventually, we will build in MouseUp messages with buttons specified
				// For now, we just "assume" no mouse specification with a 0.
				_winborder->MouseUp((int8*)msg->Buffer());
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
//			index+=sizeof(int64);
//			float x=*((float*)index); index+=sizeof(float);
//			float y=*((float*)index); index+=sizeof(float);
//			BPoint pt(x,y);
			float x;
			float y;
			int64 dummy;
			msg->Read(&dummy);
			msg->Read(&x);
			msg->Read(&y);
			BPoint pt(x,y);

			if(is_moving_window() || is_resizing_window() || is_sliding_tab())
			{
				active_winborder->MouseMoved((int8*)msg->Buffer());
			}
			else
			{
				_winborder = WindowContainsPoint(pt);
				if(_winborder)
				{
					mousewin=_winborder->Window();
					_winborder->MouseMoved((int8*)msg->Buffer());
				}
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
STRACE_KEY(("ServerWindow::HandleKeyEvent unimplemented\n"));
/*	ServerWindow *keywin=NULL;
	
	// Dispatch the key event to the active window
	keywin=GetActiveWindow();
	if(keywin)
	{
		keywin->Lock();
		keywin->_winlink->SetOpCode(code);
		keywin->_winlink
		keywin->Unlock();
	}
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
STRACE(("ServerWindow %s: Set Workspace\n",_title->String()));
	_workspace=wkspc;
}

//-----------------------------------------------------------------------

Layer* ServerWindow::FindLayer(const Layer* start, int32 token) const
{
	if( !start )
		return NULL;
	
		// see if we're looking for 'start'
	if( start->_view_token == token )
		return const_cast<Layer*>(start);

	Layer		*c = start->_topchild; //c = short for: current
	if( c != NULL )
		while( true ){
			// action block
			{
				if( c->_view_token == token )
					return c;
			}

				// go deep
			if(	c->_topchild ){
				c = c->_topchild;
			}
				// go right or up
			else
					// go right
				if( c->_lowersibling ){
					c = c->_lowersibling;
				}
					// go up
				else{
					while( !c->_parent->_lowersibling && c->_parent != start ){
						c = c->_parent;
					}
						// that enough! We've reached the start layer.
					if( c->_parent == start )
						break;
						
					c = c->_parent->_lowersibling;
				}
		}

	return NULL;
}

//-----------------------------------------------------------------------

void ServerWindow::SendMessageToClient( const BMessage* msg ) const{
	ssize_t		size;
	char		*buffer;
	
	size		= msg->FlattenedSize();
	buffer		= new char[size];
	if ( msg->Flatten( buffer, size ) == B_OK ){
		write_port( winLooperPort, msg->what, buffer, size );
	}
	else
		printf("PANIC: SW: '%s': can't flatten message in 'SendMessageToClient()'\n", _title->String() );

	delete buffer;
}

//-----------------------------------------------------------------------

/*!
	\brief Handles window activation stuff. Called by Desktop functions
*/
void ActivateWindow(ServerWindow *oldwin,ServerWindow *newwin)
{
STRACE(("ActivateWindow: old=%s, new=%s\n",oldwin?oldwin->Title():"NULL",newwin?newwin->Title():"NULL"));
	if(oldwin==newwin)
		return;

	if(oldwin)
		oldwin->SetFocus(false);

	if(newwin)
		newwin->SetFocus(true);
}

/*
 @log
 	*added handlers for AS_LAYER_(MOVE/RESIZE)TO messages
 	*added AS_LAYER_GET_COORD handler
 	*changed some methods to use BMessage class for sending messages to BWindow
*/
