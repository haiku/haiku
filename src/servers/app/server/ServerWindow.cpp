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
	cl			= NULL;

	_winborder=new WinBorder(_frame,_title->String(),wlook,wfeel,wflags,this);

	// _sender is the monitored window's event port
	_sender=winport;

	_winlink=new PortLink(_sender);
	_applink= (winapp)? new PortLink(winapp->_receiver) : NULL;
	
	// _receiver is the port to which the app sends messages for the server
	_receiver=create_port(30,_title->String());


	ses		= new BSession( _receiver, _sender );
		// Send a reply to our window - it is expecting _receiver port.
	ses->WriteData( &_receiver, sizeof(port_id) );
	ses->Sync();
	
	top_layer		= NULL;
	
	_active=false;

	// Spawn our message monitoring _monitorthread
	_monitorthread=spawn_thread(MonitorWin,_title->String(),B_NORMAL_PRIORITY,this);
	if(_monitorthread!=B_NO_MORE_THREADS && _monitorthread!=B_NO_MEMORY)
		resume_thread(_monitorthread);

	_workspace_index=index;
	_workspace=NULL;

	_token=win_token_handler.GetToken();

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
STRACE(("ServerWindow %s: Replace Decorator\n",_title->String()));
	_winborder->UpdateDecorator();
}

//! Requests that the ServerWindow's BWindow quit
void ServerWindow::Quit(void)
{
STRACE(("ServerWindow %s: Quit\n",_title->String()));
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
	_winlink->SetOpCode(AS_WORKSPACE_ACTIVATED);
	_winlink->Attach<int32>(workspace);
	_winlink->Attach<bool>(active);
	_winlink->Flush();
}

/*!
	\brief Notifies window of a workspace switch
	\param oldone index of the previous workspace
	\param newone index of the new workspace
*/
void ServerWindow::WorkspacesChanged(int32 oldone,int32 newone)
{
STRACE(("ServerWindow %s: WorkspacesChanged(%ld,%ld)\n",_title->String(),oldone,newone));
	_winlink->SetOpCode(AS_WORKSPACES_CHANGED);
	_winlink->Attach<int32>(oldone);
	_winlink->Attach<int32>(newone);
	_winlink->Flush();
}

/*!
	\brief Notifies window of a change in focus
	\param active New active status of the window
*/
void ServerWindow::WindowActivated(bool active)
{
STRACE(("ServerWindow %s: WindowActivated(%s)\n",_title->String(),(active)?"active":"inactive"));
	_winlink->SetOpCode(AS_WINDOW_ACTIVATED);
	_winlink->Attach<bool>(active);
	_winlink->Flush();
}

/*!
	\brief Notifies window of a change in screen resolution
	\param frame Size of the new resolution
	\param color_space Color space of the new screen mode
*/
void ServerWindow::ScreenModeChanged(const BRect frame, const color_space cspace)
{
STRACE(("ServerWindow %s: ScreenModeChanged\n",_title->String()));
	_winlink->SetOpCode(AS_SCREENMODE_CHANGED);
	_winlink->Attach<BRect>(frame);
	_winlink->Attach(&cspace,sizeof(color_space));
	_winlink->Flush();
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
				printf("!Server PANIC: window %s: cannot find Layer with ID %ld\n", _title->String(), token);

			STRACE(("ServerWindow %s: Message Set_Current_Layer: Layer ID: %ld\n", _title->String(), token));
			break;
		}
		case AS_LAYER_CREATE:
		{
			// Received when a view is attached to a window. This will require
			// us to attach a layer in the tree in the same manner and invalidate
			// the area in which the new layer resides assuming that it is
			// visible.
			Layer		*oldCL = cl;
			
			int32		token;
			BRect		frame;
			uint32		resizeMask;
			uint32		flags;
			int32		childCount;
			char*		name;
		
			ses->ReadInt32( &token );
			name		= ses->ReadString();
			ses->ReadRect( &frame );
			ses->ReadInt32( (int32*)&resizeMask );
			ses->ReadInt32( (int32*)&flags );
			ses->ReadInt32( &childCount );
			
			Layer		*newLayer;
			newLayer	= new Layer(frame, name, token, resizeMask, flags, this);
			
			// TODO: review Layer::AddChild
			cl->AddChild( newLayer );
			
			cl			= newLayer;
			
			int32		msgCode;
			ses->ReadInt32( &msgCode );		// this is AS_LAYER_SET_STATE
			DispatchMessage( msgCode );
			
			for(int i = 0; i < childCount; i++){
				ses->ReadInt32( &msgCode );		// this is AS_LAYER_CREATE
				DispatchMessage( msgCode );
			}
			
			cl			= oldCL;
			
			STRACE(("ServerWindow %s: Message Create_Layer unimplemented\n",_title->String()));
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
			STRACE(("ServerWindow %s: Message Delete_Layer unimplemented\n",_title->String()));

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
			
			cl->_layerdata->patt.Set( (int8*)&patt );
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

			STRACE(("ServerWindow %s: Message Layer_Set_State\n",_title->String()));
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
											
			STRACE(("ServerWindow %s: Message Create_Layer_Root\n",_title->String()));
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

Layer* ServerWindow::FindLayer(const Layer* start, int32 token) const
{
	if( !start )
		return NULL;

	Layer		*c = start->_topchild; //c = short for: current
	if( c != NULL )
		while( c != start ){
			if( c->_view_token == token )
				return c;

			if(	c->_topchild )
				c = c->_topchild;
			else
				if( c->_lowersibling )
					c = c->_lowersibling;
				else
					c = c->_parent;
		}
	return NULL;
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
