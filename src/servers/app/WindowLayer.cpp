/*
 * Copyright (c) 2001-2005, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:  DarkWyrm <bpmagic@columbus.rr.com>
 *			Adi Oanca <adioanca@gmail.com>
 *			Stephan Aßmus <superstippi@gmx.de>
 */


#include <DirectWindow.h>
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
#include "MessagePrivate.h"
#include "PortLink.h"
#include "RootLayer.h"
#include "ServerApp.h"
#include "ServerWindow.h"
#include "WindowLayer.h"
#include "Workspace.h"


// Toggle debug output
//#define DEBUG_WINBORDER
//#define DEBUG_WINBORDER_CLICK

#ifdef DEBUG_WINBORDER
#	include <stdio.h>
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif

#ifdef DEBUG_WINBORDER_CLICK
#	include <stdio.h>
#	define STRACE_CLICK(x) printf x
#else
#	define STRACE_CLICK(x) ;
#endif


WindowLayer::WindowLayer(const BRect &frame,
		const char *name, window_look look, window_feel feel,
		uint32 flags, uint32 workspaces,
		ServerWindow *window, DrawingEngine *driver)
	: Layer(frame, name, B_NULL_TOKEN, B_FOLLOW_NONE, 0UL, driver),
	  fDecorator(NULL),
	  fTopLayer(NULL),

	  fCumulativeRegion(),
	  fInUpdateRegion(),

	  fDecRegion(),
	  fRebuildDecRegion(true),

	  fMouseButtons(0),
	  fLastMousePosition(-1.0, -1.0),

	  fIsFocus(false),

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
	  fFeel(feel),
	  fWindowFlags(flags),
	  fWorkspaces(workspaces),

	  fMinWidth(1.0),
	  fMaxWidth(32768.0),
	  fMinHeight(1.0),
	  fMaxHeight(32768.0)
{
	// unlike BViews, windows start off as hidden
	fHidden = true;
	fWindow = window;
	fFlags = B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE;

	if (fLook != B_NO_BORDER_WINDOW_LOOK) {
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
		if (window->App()->GetDesktop()->ScreenAt(0)) {
			window->App()->GetDesktop()->ScreenAt(0)->GetMode(width, height, colorSpace, frequency);
// TODO: MOVE THIS AWAY!!! RemoveBy contains calls to virtual methods! Also, there is not TopLayer()!
			fFrame.OffsetTo(B_ORIGIN);
			WindowLayer::ResizeBy(width - frame.Width(), height - frame.Height());
		}
	}

	STRACE(("WindowLayer %p, %s:\n", this, Name()));
	STRACE(("\tFrame: (%.1f, %.1f, %.1f, %.1f)\n", fFrame.left, fFrame.top,
		fFrame.right, fFrame.bottom));
	STRACE(("\tWindow %s\n", window ? window->Title() : "NULL"));
}


WindowLayer::~WindowLayer()
{
	STRACE(("WindowLayer(%s)::~WindowLayer()\n", Name()));

	delete fDecorator;
}


//! redraws a certain section of the window border
void
WindowLayer::Draw(const BRect& updateRect)
{
	#ifdef DEBUG_WINBORDER
	printf("WindowLayer(%s)::Draw() : ", Name());
	r.PrintToStream();
	#endif
	
	// if we have a visible region, it is decorator's one.
	if (fDecorator) {
		fDecorator->SetFocus(IsFocus());
		fDecorator->Draw(updateRect);
	}
}


//! Moves the winborder with redraw
void
WindowLayer::MoveBy(float x, float y)
{
	if (x == 0.0f && y == 0.0f)
		return;

	// lock here because we play with some regions
	if (GetRootLayer()) {
		// TODO: MoveBy and ResizeBy() are usually called
		// from the rootlayer's thread. HandleDirectConnection() could
		// block the calling thread for ~3 seconds in the worst case, 
		// and while it would be acceptable if called from the
		// ServerWindow's thread (only the window would be blocked), in this case
		// it's not, as also the mouse movement is driven by rootlayer.
		// Find some way to call DirectConnected() from the ServerWindow's thread,
		// by sending a message from here or whatever.
		// (Tested with BeSnes9x and works fine, though) 
		Window()->HandleDirectConnection(B_DIRECT_STOP);
		
		if (GetRootLayer()->Lock()) {
			fDecRegion.OffsetBy(x, y);

			fCumulativeRegion.OffsetBy(x, y);
			fInUpdateRegion.OffsetBy(x, y);

			if (fDecorator)
				fDecorator->MoveBy(x, y);

			Layer::MoveBy(x, y);

			GetRootLayer()->Unlock();
		}

		Window()->HandleDirectConnection(B_DIRECT_START | B_BUFFER_MOVED);
	} else {
		// just offset to the new position
		if (fDecorator)
			fDecorator->MoveBy(x, y);

		Layer::MoveBy(x, y);
	}

	// dispatch a message to the client informing about the changed size
	BMessage msg(B_WINDOW_MOVED);
	msg.AddInt64("when",  system_time());
	msg.AddPoint("where", Frame().LeftTop());
	Window()->SendMessageToClient(&msg);
}


void
WindowLayer::ResizeBy(float x, float y)
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
		return;

	// this method can be called from a ServerWindow thread or from a RootLayer one,
	// so lock
	if (GetRootLayer()) {
		Window()->HandleDirectConnection(B_DIRECT_STOP);
		
		if (GetRootLayer()->Lock()) {
			fRebuildDecRegion = true;

			if (fDecorator)
				fDecorator->ResizeBy(x, y);

			Layer::ResizeBy(x, y);

			GetRootLayer()->Unlock();
		}

		Window()->HandleDirectConnection(B_DIRECT_START | B_BUFFER_RESIZED);
	} else {
		if (fDecorator)
			fDecorator->ResizeBy(x, y);

		Layer::ResizeBy(x, y);
	}

	// send a message to the client informing about the changed size
	BRect frame(Frame());
	BMessage msg(B_WINDOW_RESIZED);
	msg.AddInt64("when", system_time());
	msg.AddInt32("width", frame.IntegerWidth());
	msg.AddInt32("height", frame.IntegerHeight());
	Window()->SendMessageToClient(&msg);
}


void
WindowLayer::SetName(const char* name)
{
	Layer::SetName(name);

	// rebuild the clipping for the title area
	// and redraw it.

	if (fDecorator) {
		BRegion updateRegion;
		fDecorator->SetTitle(name, &updateRegion);

		fRebuildDecRegion = true;
		GetRootLayer()->MarkForRebuild(updateRegion);
		GetRootLayer()->TriggerRebuild();

		GetRootLayer()->MarkForRedraw(updateRegion);
		GetRootLayer()->TriggerRedraw();
	}
}


void
WindowLayer::UpdateStart()
{
	// During updates we only want to draw what's in the update region
	fInUpdate = true;
	fRequestSent = false;
}


void
WindowLayer::UpdateEnd()
{
	// The usual case. Drawing is permitted in the whole visible area.

	fInUpdate = false;

	fInUpdateRegion.MakeEmpty();

	if (fCumulativeRegion.CountRects() > 0) {
		GetRootLayer()->MarkForRedraw(fCumulativeRegion);
		GetRootLayer()->TriggerRedraw();
	}
}


void
WindowLayer::EnableUpdateRequests()
{
	fUpdateRequestsEnabled = true;
	if (fCumulativeRegion.CountRects() > 0) {
		GetRootLayer()->MarkForRedraw(fCumulativeRegion);
		GetRootLayer()->TriggerRedraw();
	}
}


//! Sets the minimum and maximum sizes of the window
void
WindowLayer::SetSizeLimits(float minWidth, float maxWidth,
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
WindowLayer::GetSizeLimits(float* minWidth, float* maxWidth,
	float* minHeight, float* maxHeight) const
{
	*minWidth = fMinWidth;
	*maxWidth = fMaxWidth;
	*minHeight = fMinHeight;
	*maxHeight = fMaxHeight;
}


void
WindowLayer::MouseDown(BMessage *msg, BPoint where, int32* _viewToken)
{
	Desktop* desktop = Window()->App()->GetDesktop();

	// default action is to drag the WindowLayer
	Layer *target = LayerAt(where);
	if (target == this) {
		// clicking WindowLayer visible area

		click_type action = DEC_DRAG;

		if (fDecorator)
			action = _ActionFor(msg);

		// deactivate border buttons on first click (select)
		if (!IsFocus() && action != DEC_MOVETOBACK
			&& action != DEC_RESIZE && action != DEC_SLIDETAB)
			action = DEC_DRAG;

		// set decorator internals
		switch (action) {
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
			desktop->SendBehindWindow(this, NULL);
		} else {
			GetRootLayer()->SetMouseEventWindow(this);
			desktop->ActivateWindow(this);
		}
	} else if (target != NULL) {
		// clicking a simple Layer.
		if (!IsFocus()) {
			DesktopSettings desktopSettings(desktop);

			// not in FFM mode?
			if (desktopSettings.MouseMode() == B_NORMAL_MOUSE)
				desktop->ActivateWindow(this);

			if ((WindowFlags() & B_WILL_ACCEPT_FIRST_CLICK) == 0)
				return;
		}

		target->MouseDown(msg, where, _viewToken);
	}
}


void
WindowLayer::MouseUp(BMessage *msg, BPoint where, int32* _viewToken)
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

	Layer* target = LayerAt(where);
	if (target != NULL && target != this)
		target->MouseUp(msg, where, _viewToken);
}


void
WindowLayer::MouseMoved(BMessage *msg, BPoint where, int32* _viewToken)
{
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
		Window()->Desktop()->MoveWindowBy(this, delta.x, delta.y);
	}
	if (fIsResizing) {
		BPoint delta = where - fLastMousePosition;
		Window()->Desktop()->ResizeWindowBy(this, delta.x, delta.y);
	}
	if (fIsSlidingTab) {
		// TODO: implement
	}

	fLastMousePosition = where;

	// change focus in FFM mode
	Desktop* desktop = Window()->Desktop();
	DesktopSettings desktopSettings(desktop);

	if (desktopSettings.MouseMode() != B_NORMAL_MOUSE && !IsFocus())
		GetRootLayer()->SetFocus(this);

	Layer* target = LayerAt(where);
	if (target != NULL && target != this)
		target->MouseMoved(msg, where, _viewToken);
}


void
WindowLayer::WorkspaceActivated(int32 index, bool active)
{
	BMessage activatedMsg(B_WORKSPACE_ACTIVATED);
	activatedMsg.AddInt64("when", real_time_clock_usecs());
	activatedMsg.AddInt32("workspace", index);
	activatedMsg.AddBool("active", active);

	Window()->SendMessageToClient(&activatedMsg);
}


void
WindowLayer::WorkspacesChanged(uint32 oldWorkspaces, uint32 newWorkspaces)
{
	fWorkspaces = newWorkspaces;

	BMessage changedMsg(B_WORKSPACES_CHANGED);
	changedMsg.AddInt64("when", real_time_clock_usecs());
	changedMsg.AddInt32("old", oldWorkspaces);
	changedMsg.AddInt32("new", newWorkspaces);

	Window()->SendMessageToClient(&changedMsg);
}


void
WindowLayer::Activated(bool active)
{
	BMessage msg(B_WINDOW_ACTIVATED);
	msg.AddBool("active", active);
	Window()->SendMessageToClient(&msg);
}


void
WindowLayer::SetTabLocation(float location)
{
	if (fDecorator)
		fDecorator->SetTabLocation(location);
}


float
WindowLayer::TabLocation() const
{
	if (fDecorator)
		return fDecorator->TabLocation();
	return 0.0;
}


//! Sets the decorator focus to active or inactive colors
void
WindowLayer::HighlightDecorator(bool active)
{
	STRACE(("Decorator->Highlight\n"));
	if (fDecorator)
		fDecorator->SetFocus(active);
}


void
WindowLayer::UpdateColors()
{
	// Unimplemented. Hook function for handling when system GUI colors change
	STRACE(("WindowLayer %s: UpdateColors unimplemented\n", Name()));
}


void
WindowLayer::UpdateDecorator()
{
	// Unimplemented. Hook function for handling when the system decorator changes
	STRACE(("WindowLayer %s: UpdateDecorator unimplemented\n", Name()));
}


void
WindowLayer::UpdateFont()
{
	// Unimplemented. Hook function for handling when a system font changes
	STRACE(("WindowLayer %s: UpdateFont unimplemented\n", Name()));
}


void
WindowLayer::UpdateScreen()
{
	// Unimplemented. Hook function for handling when the screen resolution changes
	STRACE(("WindowLayer %s: UpdateScreen unimplemented\n", Name()));
}


bool
WindowLayer::SupportsFront()
{
	if (fFeel == kDesktopWindowFeel)
		return false;

	return true;
}


void
WindowLayer::SetFeel(window_feel feel)
{
	fFeel = feel;

	// TODO: this shouldn't be necessary, but we'll see :)

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
			fWorkspaces = 0xffffffffUL;
			break;
		case B_NORMAL_WINDOW_FEEL:
			break;
	}
}


click_type
WindowLayer::_ActionFor(const BMessage *msg) const
{
	BPoint where(0,0);
	int32 buttons = 0;
	int32 modifiers = 0;

	msg->FindPoint("where", &where);
	msg->FindInt32("buttons", &buttons);
	msg->FindInt32("modifiers", &modifiers);

	if (fDecorator)
		return fDecorator->Clicked(where, buttons, modifiers);

	return DEC_NONE;
}


void
WindowLayer::set_decorator_region(BRect bounds)
{
	fRebuildDecRegion = false;

	if (fDecorator) {
		fDecRegion.MakeEmpty();
		fDecorator->GetFootprint(&fDecRegion);
	}
}


void
WindowLayer::_ReserveRegions(BRegion &reg)
{
	BRegion reserve(reg);
	reserve.IntersectWith(&fDecRegion);
	fVisible.Include(&reserve);
	reg.Exclude(&reserve);
}


void
WindowLayer::GetOnScreenRegion(BRegion& region)
{
	if (fRebuildDecRegion)
		set_decorator_region(Bounds());

	BRect frame(Bounds());
	ConvertToScreen(&frame);
	region.Set(frame);

	region.Include(&fDecRegion);

	BRegion screenRegion(GetRootLayer()->Bounds());
	region.IntersectWith(&screenRegion);
}


void
WindowLayer::RequestClientRedraw(const BRegion &invalid)
{
	BRegion	updateReg(fTopLayer->FullVisible());
	updateReg.IntersectWith(&invalid);

	if (updateReg.CountRects() == 0)
		return;

	fCumulativeRegion.Include(&updateReg);			

	if (fUpdateRequestsEnabled && !InUpdate() && !fRequestSent) {
		fInUpdateRegion = fCumulativeRegion;
		fRequestSent = true; // this is here to avoid a possible de-synchronization

		BRect rect(fInUpdateRegion.Frame());
		ConvertFromScreen(&rect);

		BMessage msg(_UPDATE_);
		msg.AddRect("_rect", rect);
		msg.AddRect("debug_rect", fInUpdateRegion.Frame());

		if (Window()->SendMessageToClient(&msg) == B_OK) {
			fCumulativeRegion.MakeEmpty();
		} else {
			fRequestSent = false;
			fInUpdateRegion.MakeEmpty();
		}
	}
}


void
WindowLayer::_AllRedraw(const BRegion &invalid)
{
	// send _UPDATE_ message to client
	RequestClientRedraw(invalid);

	Layer::_AllRedraw(invalid);
}


void
WindowLayer::SetTopLayer(Layer* layer)
{
	if (fTopLayer != NULL) {
		RemoveChild(fTopLayer);
		fTopLayer->SetAsTopLayer(false);
	}

	fTopLayer = layer;

	if (layer != NULL) {
		AddChild(fTopLayer, Window());
		fTopLayer->SetAsTopLayer(true);
	}
}

