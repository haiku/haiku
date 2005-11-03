/*
 * Copyright (c) 2001-2005, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:  DarkWyrm <bpmagic@columbus.rr.com>
 *			Adi Oanca <adioanca@cotty.iren.ro>
 *			Stephan Aßmus <superstippi@gmx.de>
 */


#include <Locker.h>
#include <Region.h>
#include <String.h>
#include <View.h>	// for mouse button defines

#include <Debug.h>
#include <WindowPrivate.h>
#include "DebugInfoManager.h"

#include "Decorator.h"
#include "DecorManager.h"
#include "Desktop.h"
#include "Globals.h"
#include "MessagePrivate.h"
#include "PortLink.h"
#include "RootLayer.h"
#include "ServerApp.h"
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

WinBorder::WinBorder(const BRect &frame,
					 const char *name,
					 const uint32 look,
					 const uint32 feel,
					 const uint32 flags,
					 const uint32 workspaces,
					 ServerWindow *window,
					 DisplayDriver *driver)
	: Layer(frame, name, B_NULL_TOKEN, B_FOLLOW_NONE, 0UL, driver),
	  fDecorator(NULL),
	  fTopLayer(NULL),

	  fCumulativeRegion(),
	  fInUpdateRegion(),

	  fMouseButtons(0),
	  fLastMousePosition(-1.0, -1.0),

	  fIsClosing(false),
	  fIsMinimizing(false),
	  fIsZooming(false),
	  fIsResizing(false),
	  fIsSlidingTab(false),
	  fIsDragging(false),

	  fUpdateRequestsEnabled(true),
	  fInUpdate(false),
	  fRequestSent(false),

	  fLook(look),
	  fLevel(-100),
	  fWindowFlags(flags),
	  fWorkspaces(workspaces),

	  fMinWidth(1.0),
	  fMaxWidth(32768.0),
	  fMinHeight(1.0),
	  fMaxHeight(32768.0),

	  cnt(0) // for debugging
{
	// unlike BViews, windows start off as hidden
	fHidden			= true;
	fServerWin		= window;
	fAdFlags		= fAdFlags | B_LAYER_CHILDREN_DEPENDANT;
	fFlags			= B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE;
#ifdef NEW_CLIPPING
	fRebuildDecRegion = true;
#endif
	QuietlySetFeel(feel);

	if (fFeel != B_NO_BORDER_WINDOW_LOOK) {
		fDecorator = gDecorManager.AllocateDecorator(window->App()->GetDesktop(), frame,
			name, fLook, fFeel,  fWindowFlags);
		if (fDecorator)
			fDecorator->GetSizeLimits(&fMinWidth, &fMinHeight, &fMaxWidth, &fMaxHeight);
	}

	// do we need to change our size to let the decorator fit?
	// _ResizeBy() will adapt the frame for validity before resizing
	if (feel == kDesktopWindowFeel) {
		// the desktop window spans over the whole screen
		// ToDo: this functionality should be moved somewhere else (so that it
		//	is always used when the workspace is changed)
		uint16 width, height;
		uint32 colorSpace;
		float frequency;
		GetRootLayer()->GetDesktop()->ScreenAt(0)->GetMode(width, height, colorSpace, frequency);
		_ResizeBy(width - frame.Width(), height - frame.Height());
	} else
		_ResizeBy(0, 0);

#ifndef NEW_CLIPPING
	RebuildFullRegion();
#endif

	STRACE(("WinBorder %p, %s:\n", this, Name()));
	STRACE(("\tFrame: (%.1f, %.1f, %.1f, %.1f)\n", fFrame.left, fFrame.top,
		fFrame.right, fFrame.bottom));
	STRACE(("\tWindow %s\n", window ? window->Title() : "NULL"));
}


WinBorder::~WinBorder()
{
	STRACE(("WinBorder(%s)::~WinBorder()\n", Name()));

	delete fTopLayer;
	delete fDecorator;
}

//! redraws a certain section of the window border
void
WinBorder::Draw(const BRect &r)
{
	#ifdef DEBUG_WINBORDER
	printf("WinBorder(%s)::Draw() : ", Name());
	r.PrintToStream();
	#endif
	
	// if we have a visible region, it is decorator's one.
	if (fDecorator) {
		WinBorder* wb = GetRootLayer()->Focus();
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
#ifndef NEW_CLIPPING

x = (float)int32(x);
y = (float)int32(y);

	if (x == 0.0 && y == 0.0)
		return;

	STRACE(("WinBorder(%s)::MoveBy(%.1f, %.1f) fDecorator: %p\n", Name(), x, y, fDecorator));
	if (fDecorator)
		fDecorator->MoveBy(x,y);

// NOTE: I moved this here from Layer::move_layer()
// Should this have any bad consequences I'm not aware of?
fCumulativeRegion.OffsetBy(x, y);
fInUpdateRegion.OffsetBy(x, y);

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
		Layer::move_layer(x, y);
	}

	if (Window()) {
		// dispatch a message to the client informing about the changed size
		BMessage msg(B_WINDOW_MOVED);
		msg.AddPoint("where", fFrame.LeftTop());
		Window()->SendMessageToClient(&msg, B_NULL_TOKEN, false);
	}

#else
	Layer::MoveBy(x, y);
#endif
}


void
WinBorder::ResizeBy(float x, float y)
{
	STRACE(("WinBorder(%s)::ResizeBy()\n", Name()));

	if (!_ResizeBy(x, y))
		return;

#ifndef NEW_CLIPPING
	if (Window()) {
		// send a message to the client informing about the changed size
		BMessage msg(B_WINDOW_RESIZED);
		msg.AddInt64("when", system_time());

		BRect frame(fTopLayer->Frame());
		msg.AddInt32("width", frame.IntegerWidth());
		msg.AddInt32("height", frame.IntegerHeight());

		Window()->SendMessageToClient(&msg, B_NULL_TOKEN, false);
	}
#endif
}


//! Resizes the winborder with redraw
bool
WinBorder::_ResizeBy(float x, float y)
{
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

	if (x == 0.0 && y == 0.0)
		return false;

#ifndef NEW_CLIPPING
	if (fDecorator)
		fDecorator->ResizeBy(x, y);

	if (IsHidden()) {
		// TODO: See large comment in MoveBy()
		fFrame.right += x;
		fFrame.bottom += y;

		if (fTopLayer)
			fTopLayer->resize_layer(x, y);
	} else {
		resize_layer(x, y);
	}
#else
	Layer::ResizeBy(x, y);
#endif

	return true;
}

// SetName
void
WinBorder::SetName(const char* name)
{
	Layer::SetName(name);

	// rebuild the clipping for the title area
	// and redraw it.

	// TODO: Adi, please have a look at this,
	// it doesn't work yet.
	if (fDecorator) {
		// before the change
		BRegion invalid(fDecorator->GetTabRect());

		fDecorator->SetTitle(name);

		// after the change
		invalid.Include(fDecorator->GetTabRect());

#ifndef NEW_CLIPPING
		RebuildFullRegion();
		fRootLayer->GoRedraw(this, invalid);
#else
		// TODO: ...
#endif
	}
}

// UpdateStart
void
WinBorder::UpdateStart()
{
	// During updates we only want to draw what's in the update region
	fInUpdate = true;
	fRequestSent = false;

cnt--;
if (cnt != 0)
	CRITICAL("Layer::UpdateStart(): wb->cnt != 0 -> Not Allowed!");
}

// UpdateEnd
void
WinBorder::UpdateEnd()
{
	// The usual case. Drawing is permitted in the whole visible area.

	fInUpdate = false;

	fInUpdateRegion.MakeEmpty();

	if (fCumulativeRegion.CountRects() > 0) {
		BRegion		reg(fCumulativeRegion);
		RequestDraw(reg, NULL);
	}
}

#ifndef NEW_CLIPPING

//! Rebuilds the WinBorder's "fully-visible" region based on info from the decorator
void
WinBorder::RebuildFullRegion()
{
	STRACE(("WinBorder(%s)::RebuildFullRegion()\n", Name()));

	fFull.MakeEmpty();

	// Winborder holds Decorator's full regions. if any...
	if (fDecorator)
		fDecorator->GetFootprint(&fFull);
}

#endif

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

	// Automatically resize the window to fit these new limits
	// if it does not already.

	// On R5, Windows don't automatically resize, but since
	// BWindow::ResizeTo() even honors the limits, I would guess
	// this is a bug that we don't have to adopt.
	// Note that most current apps will do unnecessary resizing
	// after having set the limits, but the overhead is neglible.

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
}


void
WinBorder::GetSizeLimits(float* minWidth, float* maxWidth,
	float* minHeight, float* maxHeight) const
{
	*minWidth = fMinWidth;
	*maxWidth = fMaxWidth;
	*minHeight = fMinHeight;
	*maxHeight = fMaxHeight;
}


void
WinBorder::MouseDown(const BMessage *msg)
{
	DesktopSettings desktopSettings(GetRootLayer()->GetDesktop());
	BPoint where(0,0);
	
	msg->FindPoint("where", &where);

	// not in FFM mode?
	if (desktopSettings.MouseMode() == B_NORMAL_MOUSE) {
		// default action is to drag the WinBorder
		click_type action = DEC_DRAG;
		Layer *target = LayerAt(where);
		// clicking a simple Layer.
		if (target != this) {
			if (GetRootLayer()->ActiveWorkspace()->Active() == this) {
				target->MouseDown(msg);
			}
			else {
				if (WindowFlags() & B_WILL_ACCEPT_FIRST_CLICK)
					target->MouseDown(msg);
				else
					goto activateWindow;
			}
		}
		// clicking WinBorder visible area
		else {
			winBorderAreaHandle:

			if (fDecorator)
				action = _ActionFor(msg);

			// deactivate border buttons on first click(select)
			if (GetRootLayer()->Focus() != this && action != DEC_MOVETOBACK
					&& action != DEC_RESIZE && action != DEC_SLIDETAB)
				action = DEC_DRAG;

			// set decorator internals
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
					fLastMousePosition = where;
					STRACE_CLICK(("===> DEC_DRAG\n"));
					break;

				case DEC_RESIZE:
					fIsResizing = true;
					fLastMousePosition = where;
					STRACE_CLICK(("===> DEC_RESIZE\n"));
					break;

				case DEC_SLIDETAB:
					fIsSlidingTab = true;
					fLastMousePosition = where;
					STRACE_CLICK(("===> DEC_SLIDETAB\n"));
					break;

				default:
					break;
			}

			// based on what the Decorator returned, properly place this window.
			if (action == DEC_MOVETOBACK) {
				GetRootLayer()->SetActive(this, false);
			}
			else {
				GetRootLayer()->SetNotifyLayer(this, B_POINTER_EVENTS, 0UL);
	
				activateWindow:
				GetRootLayer()->SetActive(this);
			}
		}
	}
	// in FFM mode
	else {
		Layer *target = LayerAt(where);
		// clicking a simple Layer; forward event.
		if (target != this)
			target->MouseDown(msg);
		// clicking inside our visible area.
		else
			goto winBorderAreaHandle;
	}
}

void
WinBorder::MouseUp(const BMessage *msg)
{
	bool invalidate = false;
	if (fDecorator) {
		click_type action = _ActionFor(msg);
// TODO: present behavior is not fine!
//		Decorator's Set*() methods _actualy draw_! on screen, not
//		 taking into account if that region is visible or not!
//		Decorator redraw code should follow the same path as Layer's
//		 one!
		if (fIsZooming) {
			fIsZooming	= false;
			fDecorator->SetZoom(false);
			if (action == DEC_ZOOM) {
				invalidate = true;
				Window()->NotifyZoom();
			}
		}
		if (fIsClosing) {
			fIsClosing	= false;
			fDecorator->SetClose(false);
			if (action == DEC_CLOSE) {
				invalidate = true;
				Window()->NotifyQuitRequested();
			}
		}
		if (fIsMinimizing) {
			fIsMinimizing = false;
			fDecorator->SetMinimize(false);
			if (action == DEC_MINIMIZE) {
				invalidate = true;
				Window()->NotifyMinimize(true);
			}
		}
	}
	fIsDragging = false;
	fIsResizing = false;
	fIsSlidingTab = false;
}

void
WinBorder::MouseMoved(const BMessage *msg)
{
	BPoint where(0,0);

	msg->FindPoint("where", &where);

	if (fDecorator) {
// TODO: present behavior is not fine!
//		Decorator's Set*() methods _actualy draw_! on screen, not
//		 taking into account if that region is visible or not!
//		Decorator redraw code should follow the same path as Layer's
//		 one!
		if (fIsZooming) {
			fDecorator->SetZoom(_ActionFor(msg) == DEC_ZOOM);
		} else if (fIsClosing) {
			fDecorator->SetClose(_ActionFor(msg) == DEC_CLOSE);
		} else if (fIsMinimizing) {
			fDecorator->SetMinimize(_ActionFor(msg) == DEC_MINIMIZE);
		}
	}

	if (fIsDragging) {
		BPoint delta = where - fLastMousePosition;
#ifndef NEW_CLIPPING
		MoveBy(delta.x, delta.y);
#else
		do_MoveBy(delta.x, delta.y);
#endif
	}
	if (fIsResizing) {
		BPoint delta = where - fLastMousePosition;
#ifndef NEW_CLIPPING
		ResizeBy(delta.x, delta.y);
#else
		do_ResizeBy(delta.x, delta.y);
#endif
	}
	if (fIsSlidingTab) {
		// TODO: implement
	}

	fLastMousePosition = where;
}

void
WinBorder::WorkspaceActivated(int32 index, bool active)
{
	BMessage activatedMsg(B_WORKSPACE_ACTIVATED);
	activatedMsg.AddInt64("when", real_time_clock_usecs());
	activatedMsg.AddInt32("workspace", index);
	activatedMsg.AddBool("active", active);

	Window()->SendMessageToClient(&activatedMsg, B_NULL_TOKEN, false);
}

void
WinBorder::WorkspacesChanged(uint32 oldWorkspaces, uint32 newWorkspaces)
{
	fWorkspaces = newWorkspaces;

	BMessage changedMsg(B_WORKSPACES_CHANGED);
	changedMsg.AddInt64("when", real_time_clock_usecs());
	changedMsg.AddInt32("old", oldWorkspaces);
	changedMsg.AddInt32("new", newWorkspaces);

	Window()->SendMessageToClient(&changedMsg, B_NULL_TOKEN, false);
}

void
WinBorder::Activated(bool active)
{
	BMessage msg(B_WINDOW_ACTIVATED);
	msg.AddBool("active", false);
	Window()->SendMessageToClient(&msg, B_NULL_TOKEN, false);
}

// SetTabLocation
void
WinBorder::SetTabLocation(float location)
{
	if (fDecorator)
		fDecorator->SetTabLocation(location);
}

// TabLocation
float
WinBorder::TabLocation() const
{
	if (fDecorator)
		return fDecorator->TabLocation();
	return 0.0;
}

//! Sets the decorator focus to active or inactive colors
void
WinBorder::HighlightDecorator(bool active)
{
	STRACE(("Decorator->Highlight\n"));
	if (fDecorator)
		fDecorator->SetFocus(active);
}

// Unimplemented. Hook function for handling when system GUI colors change
void
WinBorder::UpdateColors()
{
	STRACE(("WinBorder %s: UpdateColors unimplemented\n", Name()));
}

// Unimplemented. Hook function for handling when the system decorator changes
void
WinBorder::UpdateDecorator()
{
	STRACE(("WinBorder %s: UpdateDecorator unimplemented\n", Name()));
}

// Unimplemented. Hook function for handling when a system font changes
void
WinBorder::UpdateFont()
{
	STRACE(("WinBorder %s: UpdateFont unimplemented\n", Name()));
}

// Unimplemented. Hook function for handling when the screen resolution changes
void
WinBorder::UpdateScreen()
{
	STRACE(("WinBorder %s: UpdateScreen unimplemented\n", Name()));
}

// QuietlySetFeel
void
WinBorder::QuietlySetFeel(int32 feel)
{
	fFeel = feel;

	switch (fFeel) {
		case B_FLOATING_SUBSET_WINDOW_FEEL:
		case B_FLOATING_APP_WINDOW_FEEL:
			fLevel = B_FLOATING_APP;
			break;

		case B_MODAL_SUBSET_WINDOW_FEEL:
		case B_MODAL_APP_WINDOW_FEEL:
			fLevel = B_MODAL_APP;
			break;

		case B_NORMAL_WINDOW_FEEL:
			fLevel = B_NORMAL;
			break;

		case B_FLOATING_ALL_WINDOW_FEEL:
			fLevel = B_FLOATING_ALL;
			break;

		case B_MODAL_ALL_WINDOW_FEEL:
			fLevel = B_MODAL_ALL;
			break;
			
// TODO: This case is bogus, since I'm sure "feel"
// is being represented by uint32 somewhere before
// this function is used. And B_SYSTEM_LAST is defined -10. -Stephan
		case B_SYSTEM_LAST:
		case kDesktopWindowFeel:
			fLevel = B_SYSTEM_LAST;
			break;

		case B_SYSTEM_FIRST:
			fLevel = B_SYSTEM_FIRST;
			break;

		default:
			fLevel = B_NORMAL;
	}

	// floating and modal windows must appear in every workspace where
	// their main window is present. Thus their fWorkspaces will be set to
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
WinBorder::_ActionFor(const BMessage *msg) const
{
	BPoint where(0,0);
	int32 buttons = 0;
	int32 modifiers = 0;

	msg->FindPoint("where", &where);
	msg->FindInt32("buttons", &buttons);
	msg->FindInt32("modifiers", &modifiers);

#ifndef NEW_INPUT_HANDING
#ifndef NEW_CLIPPING
	if (fTopLayer->fFullVisible.Contains(where))
		return DEC_NONE;
	else
#else
	if (fTopLayer->fFullVisible2.Contains(where))
		return DEC_NONE;
	else
#endif
#endif
	if (fDecorator)
		return fDecorator->Clicked(where, buttons, modifiers);
	else
		return DEC_NONE;
}

#ifdef NEW_CLIPPING
void WinBorder::MovedByHook(float dx, float dy)
{
	STRACE(("WinBorder(%s)::MovedByHook(%.1f, %.1f) fDecorator: %p\n", Name(), x, y, fDecorator));

	fDecRegion.OffsetBy(dx, dy);

	if (fDecorator)
		fDecorator->MoveBy(dx, dy);

	fCumulativeRegion.OffsetBy(dx, dy);
	fInUpdateRegion.OffsetBy(dx, dy);

	// dispatch a message to the client informing about the changed size
	BMessage msg(B_WINDOW_MOVED);
	msg.AddInt64("when",  system_time());
	msg.AddPoint("where", Frame().LeftTop());
	Window()->SendMessageToClient(&msg, B_NULL_TOKEN, false);
}

void WinBorder::ResizedByHook(float dx, float dy, bool automatic)
{
	STRACE(("WinBorder(%s)::ResizedByHook(%.1f, %.1f, %s) fDecorator: %p\n", Name(), x, y, automatic?"true":"false", fDecorator));
	fRebuildDecRegion = true;

	if (fDecorator)
		fDecorator->ResizeBy(dx, dy);

	// send a message to the client informing about the changed size
	BRect frame(fTopLayer->Frame());
	BMessage msg(B_WINDOW_RESIZED);
	msg.AddInt64("when", system_time());
	msg.AddInt32("width", frame.IntegerWidth());
	msg.AddInt32("height", frame.IntegerHeight());
	Window()->SendMessageToClient(&msg, B_NULL_TOKEN, false);
}

void WinBorder::set_decorator_region(BRect bounds)
{
	fRebuildDecRegion = false;

	if (fDecorator)
	{
		fDecRegion.MakeEmpty();
		fDecorator->GetFootprint(&fDecRegion);
	}
}

void
WinBorder::_ReserveRegions(BRegion &reg)
{
	BRegion reserve(reg);
	reserve.IntersectWith(&fDecRegion);
	fVisible2.Include(&reserve);
	reg.Exclude(&reserve);
}

void
WinBorder::_GetWantedRegion(BRegion &reg)
{
	if (fRebuildDecRegion)
	{
		set_decorator_region(Bounds());
		// TODO? the decorator should be in WinBorder coordinates?? It's easier not to.
		//ConvertToScreen2(&fDecRegion);
	}

	BRect			screenFrame(Bounds());
	ConvertToScreen2(&screenFrame);
	reg.Set(screenFrame);

	BRegion			screenReg(GetRootLayer()->Bounds());

	reg.IntersectWith(&screenReg);

	reg.Include(&fDecRegion);
}
#endif

// SetTopLayer
void
WinBorder::SetTopLayer(Layer* layer)
{
	if (layer) {
		fTopLayer = layer;
		fTopLayer->SetAsTopLayer(true);
	
		// connect decorator and top layer. (?)
		AddChild(fTopLayer, NULL);
	}
}

