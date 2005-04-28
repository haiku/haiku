//------------------------------------------------------------------------------
//	Copyright (c) 2001-2005, Haiku, Inc.
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
//	File Name:		WinBorder.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//					Adi Oanca <adioanca@cotty.iren.ro>
//	Description:	Layer subclass which handles window management
//  
//------------------------------------------------------------------------------
#include <Region.h>
#include <String.h>
#include <Locker.h>
#include <Debug.h>
#include "PortLink.h"
#include "View.h"	// for mouse button defines
#include "MessagePrivate.h"
#include "ServerWindow.h"
#include "Decorator.h"
#include "DisplayDriver.h"
#include "Desktop.h"
#include "WinBorder.h"
#include "AppServer.h"	// for new_decorator()
#include "TokenHandler.h"
#include "Globals.h"
#include "RootLayer.h"
#include "Workspace.h"

// Toggle general function call output
//#define DEBUG_WINBORDER

// toggle
//#define DEBUG_WINBORDER_MOUSE
//#define DEBUG_WINBORDER_CLICK

#ifdef DEBUG_WINBORDER
#	include <stdio.h>
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif

#ifdef DEBUG_WINBORDER_MOUSE
#	include <stdio.h>
#	define STRACE_MOUSE(x) printf x
#else
#	define STRACE_MOUSE(x) ;
#endif

#ifdef DEBUG_WINBORDER_CLICK
#	include <stdio.h>
#	define STRACE_CLICK(x) printf x
#else
#	define STRACE_CLICK(x) ;
#endif

//! TokenHandler object used to provide IDs for all WinBorder objects
TokenHandler border_token_handler;

bool gMouseDown = false;


WinBorder::WinBorder(	const BRect &r,
						const char *name,
						const uint32 wlook,
						const uint32 wfeel,
						const uint32 wflags,
						const uint32 wwksindex,
						ServerWindow *win,
						DisplayDriver *driver)
	: Layer(r, name, B_NULL_TOKEN, B_FOLLOW_NONE, 0UL, driver),
	fLook(wlook),
	fLevel(-100),
	fWindowFlags(wflags),
	fWorkspaces(wwksindex)
{
	// unlike BViews, windows start off as hidden
	fHidden			= true;
	fInUpdate		= false;
	fRequestSent	= false;
	fServerWin		= win;
	fClassID		= AS_WINBORDER_CLASS;
cnt = 0; // for debugging
	fMouseButtons	= 0;
	fKeyModifiers	= 0;
	fDecorator		= NULL;
	fTopLayer		= NULL;
	fAdFlags		= fAdFlags | B_LAYER_CHILDREN_DEPENDANT;
	fFlags			= B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE;
	fEventMask		= B_POINTER_EVENTS;

	fIsClosing		= false;
	fIsMinimizing	= false;
	fIsZooming		= false;

	fLastMousePosition.Set(-1,-1);
	QuietlySetFeel(wfeel);

	if (fFeel != B_NO_BORDER_WINDOW_LOOK)
		fDecorator = new_decorator(r, name, fLook, fFeel, fWindowFlags, fDriver);

	RebuildFullRegion();

	desktop->AddWinBorder(this);

	STRACE(("WinBorder %s:\n",GetName()));
	STRACE(("\tFrame: (%.1f,%.1f,%.1f,%.1f)\n",r.left,r.top,r.right,r.bottom));
	STRACE(("\tWindow %s\n",win?win->Title():"NULL"));
}

WinBorder::~WinBorder(void)
{
	STRACE(("WinBorder(%s)::~WinBorder()\n",GetName()));

	desktop->RemoveWinBorder(this);

	if (fTopLayer){
		delete fTopLayer;
		fTopLayer = NULL;
	}

	if (fDecorator)	
	{
		delete fDecorator;
		fDecorator = NULL;
	}
}

//! Rebuilds the WinBorder's "fully-visible" region based on info from the decorator
void WinBorder::RebuildFullRegion(void)
{
	STRACE(("WinBorder(%s)::RebuildFullRegion()\n",GetName()));

	fFull.MakeEmpty();

	// Winborder holds Decorator's full regions. if any...
	if (fDecorator)
		fDecorator->GetFootprint(&fFull);
}

click_type WinBorder::TellWhat(PointerEvent& evt) const
{
	if (fTopLayer->fFullVisible.Contains(evt.where))
		return DEC_NONE;
	else if (fDecorator)
		return fDecorator->Clicked(evt.where, evt.buttons, evt.modifiers);
	else
		return DEC_NONE;
}

/*!
	\brief Handles B_MOUSE_DOWN events and takes appropriate actions
	\param evt PointerEvent object containing the info from the last B_MOUSE_DOWN message
	\param sendMessage flag to send a B_MOUSE_DOWN message to the client
	
	This function searches to see if the B_MOUSE_DOWN message is being sent to the window tab
	or frame. If it is not, the message is passed on to the appropriate view in the client
	BWindow. If the WinBorder is the target, then the proper action flag is set.
*/
void WinBorder::MouseDown(click_type action)
{
	// find out where user clicked in Decorator
	switch(action)
	{
		case DEC_CLOSE:
		{
			fIsClosing = true;
			fDecorator->SetClose(true);
			fDecorator->DrawClose();
			STRACE_CLICK(("===> DEC_CLOSE\n"));
			break;
		}
		case DEC_ZOOM:
		{
			fIsZooming = true;
			fDecorator->SetZoom(true);
			fDecorator->DrawZoom();
			STRACE_CLICK(("===> DEC_ZOOM\n"));
			break;
		}
		case DEC_MINIMIZE:
		{
			fIsMinimizing = true;
			fDecorator->SetMinimize(true);
			fDecorator->DrawMinimize();
			STRACE_CLICK(("===> DEC_MINIMIZE\n"));
			break;
		}
		default:
		{
			debugger("WinBorder::MouseDown - default case - not allowed!\n");
			break;
		}
	}
}

/*!
	\brief Handles B_MOUSE_MOVED events and takes appropriate actions
	\param evt PointerEvent object containing the info from the last B_MOUSE_MOVED message
	
	This function doesn't do much except test continue any move/resize operations in progress 
	or check to see if the user clicked on a tab button (close, zoom, etc.) and then moused
	away to prevent the operation from occurring
*/
void WinBorder::MouseMoved(click_type action)
{
	if (fIsZooming && action!=DEC_ZOOM)
	{
		fDecorator->SetZoom(false);
		fDecorator->DrawZoom();
	}
	else
	if (fIsClosing && action!=DEC_CLOSE)
	{
		fDecorator->SetClose(false);
		fDecorator->DrawClose();
	}
	else
	if(fIsMinimizing && action!=DEC_MINIMIZE)
	{
		fDecorator->SetMinimize(false);
		fDecorator->DrawMinimize();
	}
}

/*!
	\brief Handles B_MOUSE_UP events and takes appropriate actions
	\param evt PointerEvent object containing the info from the last B_MOUSE_UP message
	
	This function resets any state objects (is_resizing flag and such) and if resetting a 
	button click flag, takes the appropriate action (i.e. clearing the close button flag also
	takes steps to close the window).
*/
void WinBorder::MouseUp(click_type action)
{
	if (fIsZooming)
	{
		fIsZooming	= false;
		fDecorator->SetZoom(false);
		fDecorator->DrawZoom();
		if(action==DEC_ZOOM)
			Window()->Zoom();
		return;
	}
	if(fIsClosing)
	{
		fIsClosing	= false;
		fDecorator->SetClose(false);
		fDecorator->DrawClose();
		if(action==DEC_CLOSE)
			Window()->Quit();
		return;
	}
	if(fIsMinimizing)
	{
		fIsMinimizing = false;
		fDecorator->SetMinimize(false);
		fDecorator->DrawMinimize();
		if(action==DEC_MINIMIZE)
			Window()->Minimize(true);
		return;
	}
}

//! Sets the decorator focus to active or inactive colors
void WinBorder::HighlightDecorator(const bool &active)
{
	STRACE(("Decorator->Highlight\n"));
	fDecorator->SetFocus(active);
}

//! redraws a certain section of the window border
void WinBorder::Draw(const BRect &r)
{
	#ifdef DEBUG_WINBORDER
	printf("WinBorder(%s)::Draw() : ", GetName());
	r.PrintToStream();
	#endif
	
	// if we have a visible region, it is decorator's one.
	if(fDecorator)
	{
		WinBorder	*wb = GetRootLayer()->FocusWinBorder();
		if (wb && wb == this)
			fDecorator->SetFocus(true);
		else
			fDecorator->SetFocus(false);
		fDecorator->Draw(r);
	}
}

//! Moves the winborder with redraw
void WinBorder::MoveBy(float x, float y)
{
	STRACE(("WinBorder(%s)::MoveBy(%.1f, %.1f) fDecorator: %p\n", GetName(), x, y, fDecorator));
	if(fDecorator)
		fDecorator->MoveBy(x,y);

	move_layer(x,y);
}

//! Resizes the winborder with redraw
void WinBorder::ResizeBy(float x, float y)
{
	// TODO: account for size limits
	// NOTE: size limits are also regarded in BWindow::ResizeXX()
	
	STRACE(("WinBorder(%s)::ResizeBy()\n", GetName()));
	if(fDecorator)
		fDecorator->ResizeBy(x,y);

	resize_layer(x,y);
}

//! Sets the minimum and maximum sizes of the window
void WinBorder::SetSizeLimits(float minwidth, float maxwidth, float minheight, float maxheight)
{
	if(minwidth<0)
		minwidth=0;

	if(minheight<0)
		minheight=0;
	
	if(maxwidth<=minwidth)
		maxwidth=minwidth+1;
	
	if(maxheight<=minheight)
		maxheight=minheight+1;
	
	fMinWidth=minwidth;
	fMaxWidth=maxwidth;
	fMinHeight=minheight;
	fMaxHeight=maxheight;
}

//! Returns true if the point is in the WinBorder's screen area
bool WinBorder::HasPoint(const BPoint& pt) const
{
	return fFullVisible.Contains(pt);
}

// Unimplemented. Hook function for handling when system GUI colors change
void WinBorder::UpdateColors(void)
{
	STRACE(("WinBorder %s: UpdateColors unimplemented\n",GetName()));
}

// Unimplemented. Hook function for handling when the system decorator changes
void WinBorder::UpdateDecorator(void)
{
	STRACE(("WinBorder %s: UpdateDecorator unimplemented\n",GetName()));
}

// Unimplemented. Hook function for handling when a system font changes
void WinBorder::UpdateFont(void)
{
	STRACE(("WinBorder %s: UpdateFont unimplemented\n",GetName()));
}

// Unimplemented. Hook function for handling when the screen resolution changes
void WinBorder::UpdateScreen(void)
{
	STRACE(("WinBorder %s: UpdateScreen unimplemented\n",GetName()));
}

void WinBorder::QuietlySetFeel(int32 feel)
{
	fFeel = feel;

	switch(fFeel)
	{
		case B_FLOATING_SUBSET_WINDOW_FEEL:
		case B_FLOATING_APP_WINDOW_FEEL:
			fLevel	= B_FLOATING_APP;
			break;

		case B_MODAL_SUBSET_WINDOW_FEEL:
		case B_MODAL_APP_WINDOW_FEEL:
			fLevel	= B_MODAL_APP;
			break;

		case B_NORMAL_WINDOW_FEEL:
			fLevel	= B_NORMAL;
			break;

		case B_FLOATING_ALL_WINDOW_FEEL:
			fLevel	= B_FLOATING_ALL;
			break;

		case B_MODAL_ALL_WINDOW_FEEL:
			fLevel	= B_MODAL_ALL;
			break;
			
		case B_SYSTEM_LAST:
			fLevel	= B_SYSTEM_LAST;
			break;

		case B_SYSTEM_FIRST:
			fLevel	= B_SYSTEM_FIRST;
			break;

		default:
			fLevel	= B_NORMAL;
	}

	// floating and modal windows must appear in every workspace where
	// their main window is present. Thus their wksIndex will be set to
	// '0x0' and they will be made visible when needed.
	switch (fFeel)
	{
		case B_MODAL_APP_WINDOW_FEEL:
			break;
		case B_MODAL_SUBSET_WINDOW_FEEL:
		case B_FLOATING_APP_WINDOW_FEEL:
		case B_FLOATING_SUBSET_WINDOW_FEEL:
			fWorkspaces = 0x0UL;
			break;
		case B_MODAL_ALL_WINDOW_FEEL:
		case B_FLOATING_ALL_WINDOW_FEEL:
		case B_SYSTEM_LAST:
		case B_SYSTEM_FIRST:
			fWorkspaces = 0xffffffffUL;
			break;
		case B_NORMAL_WINDOW_FEEL:
			break;
	}
}	
