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
//					Stephan Aßmus <superstippi@gmx.de>
//	Description:	Layer subclass which handles window management
//  
//------------------------------------------------------------------------------
#include <Locker.h>
#include <Region.h>
#include <String.h>
#include <View.h>	// for mouse button defines

#include <Debug.h>
#include "DebugInfoManager.h"

#include "AppServer.h"	// for new_decorator()
#include "Decorator.h"
#include "Desktop.h"
#include "Globals.h"
#include "MessagePrivate.h"
#include "PortLink.h"
#include "RootLayer.h"
#include "ServerWindow.h"
#include "TokenHandler.h"
#include "Workspace.h"

#include "WinBorder.h"

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

WinBorder::WinBorder(const BRect &r,
					 const char *name,
					 const uint32 wlook,
					 const uint32 wfeel,
					 const uint32 wflags,
					 const uint32 wwksindex,
					 ServerWindow *win,
					 DisplayDriver *driver)
	: Layer(r, name, B_NULL_TOKEN, B_FOLLOW_NONE, 0UL, driver),
	  fDecorator(NULL),
	  fTopLayer(NULL),

	  zUpdateReg(),
	  yUpdateReg(),
	  fUpdateReg(),

	  fMouseButtons(0),
	  fKeyModifiers(0),
	  fLastMousePosition(-1.0, -1.0),

	  fIsClosing(false),
	  fIsMinimizing(false),
	  fIsZooming(false),

	  fIsDragging(false),
	  fBringToFrontOnRelease(false),

	  fIsResizing(false),

	  fInUpdate(false),
	  fRequestSent(false),

	  fLook(wlook),
	  fFeel(-1),
	  fLevel(-100),
	  fWindowFlags(wflags),
	  fWorkspaces(wwksindex),

	  fMinWidth(1.0),
	  fMaxWidth(10000.0),
	  fMinHeight(1.0),
	  fMaxHeight(10000.0),

	  cnt(0) // for debugging
{
	// unlike BViews, windows start off as hidden
	fHidden			= true;
	fServerWin		= win;
	fClassID		= AS_WINBORDER_CLASS;
	fAdFlags		= fAdFlags | B_LAYER_CHILDREN_DEPENDANT;
	fFlags			= B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE;
	fEventMask		= B_POINTER_EVENTS;

	QuietlySetFeel(wfeel);

	if (fFeel != B_NO_BORDER_WINDOW_LOOK) {
		fDecorator = new_decorator(r, name, fLook, fFeel, fWindowFlags, fDriver);
		if (fDecorator)
			fDecorator->GetSizeLimits(&fMinWidth, &fMinHeight, &fMaxWidth, &fMaxHeight);
	}

	RebuildFullRegion();

	gDesktop->AddWinBorder(this);

	STRACE(("WinBorder %s:\n", GetName()));
	STRACE(("\tFrame: (%.1f, %.1f, %.1f, %.1f)\n", r.left, r.top, r.right, r.bottom));
	STRACE(("\tWindow %s\n",win ? win->Title() : "NULL"));
}

WinBorder::~WinBorder()
{
	STRACE(("WinBorder(%s)::~WinBorder()\n",GetName()));

	gDesktop->RemoveWinBorder(this);

	delete fTopLayer;
	delete fDecorator;
}

//! Rebuilds the WinBorder's "fully-visible" region based on info from the decorator
void
WinBorder::RebuildFullRegion()
{
	STRACE(("WinBorder(%s)::RebuildFullRegion()\n",GetName()));

	fFull.MakeEmpty();

	// Winborder holds Decorator's full regions. if any...
	if (fDecorator)
		fDecorator->GetFootprint(&fFull);
}

/*!
	\brief Handles B_MOUSE_DOWN events and takes appropriate actions
	\param evt PointerEvent object containing the info from the last B_MOUSE_DOWN message
	\param sendMessage flag to send a B_MOUSE_DOWN message to the client
	
	This function searches to see if the B_MOUSE_DOWN message is being sent to the window tab
	or frame. If it is not, the message is passed on to the appropriate view in the client
	BWindow. If the WinBorder is the target, then the proper action flag is set.
*/
click_type
WinBorder::MouseDown(const PointerEvent& event)
{
	click_type action = _ActionFor(event);

	if (fDecorator) {
		// find out where user clicked in Decorator
		switch(action) {
			case DEC_CLOSE:
				fIsClosing = true;
				fDecorator->SetClose(true);
				STRACE_CLICK(("===> DEC_CLOSE\n"));
				break;
	
			case DEC_ZOOM:
				fIsZooming = true;
				fDecorator->SetZoom(true);
				STRACE_CLICK(("===> DEC_ZOOM\n"));
				break;
	
			case DEC_MINIMIZE:
				fIsMinimizing = true;
				fDecorator->SetMinimize(true);
				STRACE_CLICK(("===> DEC_MINIMIZE\n"));
				break;

			case DEC_DRAG:
				fIsDragging = true;
				fBringToFrontOnRelease = true;
				fLastMousePosition = event.where;
				STRACE_CLICK(("===> DEC_DRAG\n"));
				break;

			case DEC_RESIZE:
				fIsResizing = true;
				fLastMousePosition = event.where;
				fResizingClickOffset = event.where - fFrame.RightBottom();
				STRACE_CLICK(("===> DEC_RESIZE\n"));
				break;

			default:
				break;
		}
	}
	return action;
}

/*!
	\brief Handles B_MOUSE_MOVED events and takes appropriate actions
	\param evt PointerEvent object containing the info from the last B_MOUSE_MOVED message
	
	This function doesn't do much except test continue any move/resize operations in progress 
	or check to see if the user clicked on a tab button (close, zoom, etc.) and then moused
	away to prevent the operation from occurring
*/
void
WinBorder::MouseMoved(const PointerEvent& event)
{
	if (fDecorator) {
		if (fIsZooming) {
			fDecorator->SetZoom(_ActionFor(event) == DEC_ZOOM);
		} else if (fIsClosing) {
			fDecorator->SetClose(_ActionFor(event) == DEC_CLOSE);
		} else if (fIsMinimizing) {
			fDecorator->SetMinimize(_ActionFor(event) == DEC_MINIMIZE);
		}
	}
	if (fIsDragging) {
		// we will not come to front if we ever actually moved
		fBringToFrontOnRelease = false;

		BPoint delta = event.where - fLastMousePosition;
		MoveBy(delta.x, delta.y);
	}
	if (fIsResizing) {
		BRect frame(fFrame.LeftTop(), event.where - fResizingClickOffset);

		BPoint delta = frame.RightBottom() - fFrame.RightBottom();
		ResizeBy(delta.x, delta.y);
	}
	fLastMousePosition = event.where;
}

/*!
	\brief Handles B_MOUSE_UP events and takes appropriate actions
	\param evt PointerEvent object containing the info from the last B_MOUSE_UP message
	
	This function resets any state objects (is_resizing flag and such) and if resetting a 
	button click flag, takes the appropriate action (i.e. clearing the close button flag also
	takes steps to close the window).
*/
void
WinBorder::MouseUp(const PointerEvent& event)
{
	if (fDecorator) {
		click_type action = _ActionFor(event);

		if (fIsZooming) {
			fIsZooming	= false;
			fDecorator->SetZoom(false);
			if (action == DEC_ZOOM)
				Window()->Zoom();
			return;
		}
		if (fIsClosing) {
			fIsClosing	= false;
			fDecorator->SetClose(false);
			if (action == DEC_CLOSE)
				Window()->Quit();
			return;
		}
		if (fIsMinimizing) {
			fIsMinimizing = false;
			fDecorator->SetMinimize(false);
			if (action == DEC_MINIMIZE)
				Window()->Minimize(true);
			return;
		}
	}
	if (fBringToFrontOnRelease) {
		// TODO: We would have dragged the window if
		// the mouse would have moved, but it didn't
		// move -> This will bring the window to the
		// front on R5 in FFM mode!
	}
	fIsDragging = false;
	fIsResizing = false;
	fBringToFrontOnRelease = false;
}

//! Sets the decorator focus to active or inactive colors
void
WinBorder::HighlightDecorator(const bool &active)
{
	STRACE(("Decorator->Highlight\n"));
	if (fDecorator)
		fDecorator->SetFocus(active);
}

//! redraws a certain section of the window border
void
WinBorder::Draw(const BRect &r)
{
	#ifdef DEBUG_WINBORDER
	printf("WinBorder(%s)::Draw() : ", GetName());
	r.PrintToStream();
	#endif
	
	// if we have a visible region, it is decorator's one.
	if (fDecorator) {
		WinBorder* wb = GetRootLayer()->FocusWinBorder();
		if (wb == this)
			fDecorator->SetFocus(true);
		else
			fDecorator->SetFocus(false);
		fDecorator->Draw(r);
	}
}

//! Moves the winborder with redraw
void
WinBorder::MoveBy(float x, float y)
{
	if (x == 0.0 && y == 0.0)
		return;

	STRACE(("WinBorder(%s)::MoveBy(%.1f, %.1f) fDecorator: %p\n", GetName(), x, y, fDecorator));
	if (fDecorator)
		fDecorator->MoveBy(x,y);

// NOTE: I moved this here from Layer::move_layer()
// Should this have any bad consequences I'm not aware of?
zUpdateReg.OffsetBy(x, y);
yUpdateReg.OffsetBy(x, y);
fUpdateReg.OffsetBy(x, y);

	if (IsHidden()) {
// TODO: This is a work around for a design issue:
// The actual movement of a layer is done during
// the region rebuild. The mechanism is somewhat
// complicated and scheduled for refractoring...
// The problem here for hidden layers is that 
// they seem *not* to be part of the layer tree.
// I don't think this is wrong as such, but of
// course the rebuilding of regions does not take
// place then. I don't understand yet the consequences
// for normal views, but this here fixes at least
// BWindows being MoveTo()ed before they are Show()n.
// In Layer::move_to, StartRebuildRegions() is called
// on fParent. But the rest of the this layers tree
// has not been added to fParent apperantly. So now
// you ask why fParent is even valid? Me too.
		fFrame.OffsetBy(x, y);
		fFull.OffsetBy(x, y);
		fTopLayer->move_layer(x, y);
		// ...and here we get really hacky...
		fTopLayer->fFrame.OffsetTo(0.0, 0.0);
	} else {
		move_layer(x, y);
	}

	if (Window()) {
		// dispatch a message to the client informing about the changed size
		BMessage msg(B_WINDOW_MOVED);
		msg.AddPoint("where", fFrame.LeftTop());
		Window()->SendMessageToClient(&msg, B_NULL_TOKEN, false);
	}
}

//! Resizes the winborder with redraw
void
WinBorder::ResizeBy(float x, float y)
{
	STRACE(("WinBorder(%s)::ResizeBy()\n", GetName()));

	float wantWidth = fFrame.Width() + x;
	float wantHeight = fFrame.Height() + y;

	// enforce size limits
	if (wantWidth < fMinWidth)
		wantWidth = fMinWidth;
	if (wantWidth > fMaxWidth)
		wantWidth = fMaxWidth;

	if (wantHeight < fMinHeight)
		wantHeight = fMinHeight;
	if (wantHeight > fMaxHeight)
		wantHeight = fMaxHeight;

	x = wantWidth - fFrame.Width();
	y = wantHeight - fFrame.Height();

	if (x != 0.0 || y != 0.0) {
		if (fDecorator)
			fDecorator->ResizeBy(x, y);
	
		if (IsHidden()) {
			// TODO: See large comment in MoveBy()
			fFrame.right += x;
			fFrame.bottom += y;

			fTopLayer->resize_layer(x, y);
		} else {
			resize_layer(x, y);
		}

		if (Window()) {
			// send a message to the client informing about the changed size
			BRect frame(fTopLayer->Frame());
			BMessage msg(B_WINDOW_RESIZED);
			msg.AddInt32("width", frame.Width());
			msg.AddInt32("height", frame.Height());
			Window()->SendMessageToClient(&msg, B_NULL_TOKEN, false);
		}
	}
}

//! Sets the minimum and maximum sizes of the window
void
WinBorder::SetSizeLimits(float minWidth, float maxWidth,
						 float minHeight, float maxHeight)
{
	if (minWidth < 0)
		minWidth = 0;

	if (minHeight < 0)
		minHeight = 0;

	fMinWidth = minWidth;
	fMaxWidth = maxWidth;
	fMinHeight = minHeight;
	fMaxHeight = maxHeight;

	// give the Decorator a say in this too
	if (fDecorator)
		fDecorator->GetSizeLimits(&fMinWidth, &fMinHeight,
								  &fMaxWidth, &fMaxHeight);

	if (fMaxWidth < fMinWidth)
		fMaxWidth = fMinWidth;

	if (fMaxHeight < fMinHeight)
		fMaxHeight = fMinHeight;

#if 0 // On R5, Windows don't automatically resize
	// Automatically resize the window to fit these new limits
	// if it does not already.
	float minWidthDiff = fMinWidth - fFrame.Width();
	float minHeightDiff = fMinHeight - fFrame.Height();
	float maxWidthDiff = fMaxWidth - fFrame.Width();
	float maxHeightDiff = fMaxHeight - fFrame.Height();

	float xDiff = 0.0;
	if (minWidthDiff > 0.0)	// we're currently smaller than minWidth
		xDiff = minWidthDiff;
	else if (maxWidthDiff < 0.0) // we're currently larger than maxWidth
		xDiff = maxWidthDiff;

	float yDiff = 0.0;
	if (minHeightDiff > 0.0) // we're currently smaller than minHeight
		yDiff = minHeightDiff;
	else if (maxHeightDiff < 0.0) // we're currently larger than maxHeight
		yDiff = maxHeightDiff;

	ResizeBy(xDiff, yDiff);
#endif
}

// GetSizeLimits
void
WinBorder::GetSizeLimits(float* minWidth, float* maxWidth,
						 float* minHeight, float* maxHeight) const
{
	*minWidth = fMinWidth;
	*maxWidth = fMaxWidth;
	*minHeight = fMinHeight;
	*maxHeight = fMaxHeight;
}

//! Returns true if the point is in the WinBorder's screen area
bool
WinBorder::HasPoint(const BPoint& pt) const
{
	return fFullVisible.Contains(pt);
}

// Unimplemented. Hook function for handling when system GUI colors change
void
WinBorder::UpdateColors()
{
	STRACE(("WinBorder %s: UpdateColors unimplemented\n",GetName()));
}

// Unimplemented. Hook function for handling when the system decorator changes
void
WinBorder::UpdateDecorator()
{
	STRACE(("WinBorder %s: UpdateDecorator unimplemented\n",GetName()));
}

// Unimplemented. Hook function for handling when a system font changes
void
WinBorder::UpdateFont()
{
	STRACE(("WinBorder %s: UpdateFont unimplemented\n",GetName()));
}

// Unimplemented. Hook function for handling when the screen resolution changes
void
WinBorder::UpdateScreen()
{
	STRACE(("WinBorder %s: UpdateScreen unimplemented\n",GetName()));
}

// QuietlySetFeel
void
WinBorder::QuietlySetFeel(int32 feel)
{
	fFeel = feel;

	switch(fFeel) {
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
	switch (fFeel) {
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

// _ActionFor
click_type
WinBorder::_ActionFor(const PointerEvent& event) const
{
	if (fTopLayer->fFullVisible.Contains(event.where))
		return DEC_NONE;
	else if (fDecorator)
		return fDecorator->Clicked(event.where, event.buttons, event.modifiers);
	else
		return DEC_NONE;
}


