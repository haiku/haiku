/*
 * Copyright 2001-2009, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Adi Oanca <adioanca@gmail.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Axel Dörfler <axeld@pinc-software.de>
 *		Brecht Machiels <brecht@mos6581.org>
 */


#include "Window.h"

#include "Decorator.h"
#include "DecorManager.h"
#include "Desktop.h"
#include "DrawingEngine.h"
#include "HWInterface.h"
#include "MessagePrivate.h"
#include "PortLink.h"
#include "ServerApp.h"
#include "ServerWindow.h"
#include "Workspace.h"
#include "WorkspacesView.h"

#include <ViewPrivate.h>
#include <WindowPrivate.h>

#include <Debug.h>
#include <DirectWindow.h>
#include <PortLink.h>
#include <View.h>

#include <new>
#include <stdio.h>


// Toggle debug output
//#define DEBUG_WINDOW
//#define DEBUG_WINDOW_CLICK
//#define DEBUG_STACK_AND_TILE

#ifdef DEBUG_WINDOW
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif

#ifdef DEBUG_WINDOW_CLICK
#	define STRACE_CLICK(x) printf x
#else
#	define STRACE_CLICK(x) ;
#endif

#ifdef DEBUG_STACK_AND_TILE
#	define STRACE_SAT(x) debug_printf x
#else
#	define STRACE_SAT(x) ;
#endif

// IMPORTANT: nested LockSingleWindow()s are not supported (by MultiLocker)

using std::nothrow;

// if the background clearing is delayed until
// the client draws the view, we have less flickering
// when contents have to be redrawn because of resizing
// a window or because the client invalidates parts.
// when redrawing something that has been exposed from underneath
// other windows, the other window will be seen longer at
// its previous position though if the exposed parts are not
// cleared right away. maybe there ought to be a flag in
// the update session, which tells us the cause of the update


//static rgb_color sPendingColor = (rgb_color){ 255, 255, 0, 255 };
//static rgb_color sCurrentColor = (rgb_color){ 255, 0, 255, 255 };


Window::Window(const BRect& frame, const char *name,
		window_look look, window_feel feel, uint32 flags, uint32 workspaces,
		::ServerWindow* window, DrawingEngine* drawingEngine)
	:
	fTitle(name),
	fFrame(frame),
	fScreen(NULL),

	fVisibleRegion(),
	fVisibleContentRegion(),
	fDirtyRegion(),
	fDirtyCause(0),

	fBorderRegion(),
	fContentRegion(),
	fEffectiveDrawingRegion(),

	fVisibleContentRegionValid(false),
	fBorderRegionValid(false),
	fContentRegionValid(false),
	fEffectiveDrawingRegionValid(false),

	fRegionPool(),

	fIsClosing(false),
	fIsMinimizing(false),
	fIsZooming(false),
	fIsResizing(false),
	fIsSlidingTab(false),
	fIsDragging(false),
	fActivateOnMouseUp(false),

	fDecorator(NULL),
	fTopView(NULL),
	fWindow(window),
	fDrawingEngine(drawingEngine),
	fDesktop(window->Desktop()),

	fLastMousePosition(0.0f, 0.0f),
	fMouseMoveDistance(0.0f),
	fLastMoveTime(0),
	fLastSnapTime(0),

	fCurrentUpdateSession(&fUpdateSessions[0]),
	fPendingUpdateSession(&fUpdateSessions[1]),
	fUpdateRequested(false),
	fInUpdate(false),
	fUpdatesEnabled(true),

	// windows start hidden
	fHidden(true),
	fMinimized(false),
	fIsFocus(false),

	fLook(look),
	fFeel(feel),
	fWorkspaces(workspaces),
	fCurrentWorkspace(-1),

	fMinWidth(1),
	fMaxWidth(32768),
	fMinHeight(1),
	fMaxHeight(32768),

	fWorkspacesViewCount(0),

	fLeftVar(NULL),
	fTopVar(NULL),
	fRightVar(NULL),
	fBottomVar(NULL),
	fLeftConstraint(NULL),
	fTopConstraint(NULL),
	fMinWidthConstraint(NULL),
	fMinHeightConstraint(NULL),
	fWidthConstraint(NULL),
	fHeightConstraint(NULL),

	fLeftAdjacentWindows(NULL),
	fTopAdjacentWindows(NULL),
	fRightAdjacentWindows(NULL),
	fBottomAdjacentWindows(NULL),
	fSnappingConstraints(NULL),

	fLeft2LeftSnappingWindowIds(NULL),
	fLeft2RightSnappingWindowIds(NULL),
	fRight2RightSnappingWindowIds(NULL),
	fRight2LeftSnappingWindowIds(NULL),
	fTop2TopSnappingWindowIds(NULL),
	fTop2BottomSnappingWindowIds(NULL),
	fBottom2BottomSnappingWindowIds(NULL),
	fBottom2TopSnappingWindowIds(NULL),

	fWindowUnder(NULL),
	fStackedWindows(NULL),
	fPrevStackedWindows(NULL),
	fLeftStackingConstraint(NULL),
	fTopStackingConstraint(NULL),
	fRightStackingConstraint(NULL),
	fBottomStackingConstraint(NULL),
	fId(0),
	fStackedWindowIds(NULL)
{
	// make sure our arguments are valid
	if (!IsValidLook(fLook))
		fLook = B_TITLED_WINDOW_LOOK;
	if (!IsValidFeel(fFeel))
		fFeel = B_NORMAL_WINDOW_FEEL;

	SetFlags(flags, NULL);

	if (fLook != B_NO_BORDER_WINDOW_LOOK) {
		fDecorator = gDecorManager.AllocateDecorator(fDesktop, fDrawingEngine,
			frame, name, fLook, fFlags);
		if (fDecorator) {
			fDecorator->GetSizeLimits(&fMinWidth, &fMinHeight,
				&fMaxWidth, &fMaxHeight);
		}
	}

	// do we need to change our size to let the decorator fit?
	// _ResizeBy() will adapt the frame for validity before resizing
	if (feel == kDesktopWindowFeel) {
		// the desktop window spans over the whole screen
		// TODO: this functionality should be moved somewhere else
		//  (so that it is always used when the workspace is changed)
		uint16 width, height;
		uint32 colorSpace;
		float frequency;
		if (Screen() != NULL) {
			Screen()->GetMode(width, height, colorSpace, frequency);
// TODO: MOVE THIS AWAY!!! ResizeBy contains calls to virtual methods!
// Also, there is no TopView()!
			fFrame.OffsetTo(B_ORIGIN);
//			ResizeBy(width - frame.Width(), height - frame.Height(), NULL);
		}
	}

	STRACE(("Window %p, %s:\n", this, Name()));
	STRACE(("\tFrame: (%.1f, %.1f, %.1f, %.1f)\n", fFrame.left, fFrame.top,
		fFrame.right, fFrame.bottom));
	STRACE(("\tWindow %s\n", window ? window->Title() : "NULL"));
}


Window::~Window()
{
	// if the window still has stackedWindowIds but is not stacked anymore,
	// clean up the stackedWindowIds (they are not used by other windows
	// anymore)
	if (fStackedWindowIds
		&& (!fStackedWindows  || fStackedWindows->CountItems()==1)) {
		for (int i = 0; i < fStackedWindowIds->CountItems(); i++) {
			int32* id = static_cast<int32*>(fStackedWindowIds->ItemAt(i));
			free(id);
		}
		delete fStackedWindowIds;
		fStackedWindowIds = NULL;
	}

	// clean up the entries in the snapping lists for each combo of
	// snapping orientations
	_FreeUpSnappingList(SNAP_LEFT, SNAP_LEFT, false);
	_FreeUpSnappingList(SNAP_LEFT, SNAP_RIGHT, false);
	_FreeUpSnappingList(SNAP_RIGHT, SNAP_LEFT, false);
	_FreeUpSnappingList(SNAP_RIGHT, SNAP_RIGHT, false);
	_FreeUpSnappingList(SNAP_TOP, SNAP_TOP, false);
	_FreeUpSnappingList(SNAP_TOP, SNAP_BOTTOM, false);
	_FreeUpSnappingList(SNAP_BOTTOM, SNAP_TOP, false);
	_FreeUpSnappingList(SNAP_BOTTOM, SNAP_BOTTOM, false);

	// clean up all other stacking and snapping
	_RemoveStackingAndSnapping();
	FinishStackingAndSnapping();

	if (fTopView) {
		fTopView->DetachedFromWindow();
		delete fTopView;
	}

	delete fDecorator;

	delete fDrawingEngine;
}


status_t
Window::InitCheck() const
{
	if (!fDrawingEngine)
		return B_NO_MEMORY;
	// TODO: anything else?
	return B_OK;
}


void
Window::SetClipping(BRegion* stillAvailableOnScreen)
{
	// this function is only called from the Desktop thread

	// start from full region (as if the window was fully visible)
	GetFullRegion(&fVisibleRegion);
	// clip to region still available on screen
	fVisibleRegion.IntersectWith(stillAvailableOnScreen);

	fVisibleContentRegionValid = false;
	fEffectiveDrawingRegionValid = false;
}


void
Window::GetFullRegion(BRegion* region)
{
	// TODO: if someone needs to call this from
	// the outside, the clipping needs to be readlocked!

	// start from the decorator border, extend to use the frame
	GetBorderRegion(region);
	region->Include(fFrame);
}


void
Window::GetBorderRegion(BRegion* region)
{
	// TODO: if someone needs to call this from
	// the outside, the clipping needs to be readlocked!

	if (!fBorderRegionValid) {
		if (fDecorator)
			fDecorator->GetFootprint(&fBorderRegion);
		else
			fBorderRegion.MakeEmpty();

		fBorderRegionValid = true;
	}

	*region = fBorderRegion;
}


void
Window::GetContentRegion(BRegion* region)
{
	// TODO: if someone needs to call this from
	// the outside, the clipping needs to be readlocked!

	if (!fContentRegionValid) {
		_UpdateContentRegion();
	}

	*region = fContentRegion;
}


BRegion&
Window::VisibleContentRegion()
{
	// TODO: if someone needs to call this from
	// the outside, the clipping needs to be readlocked!

	// regions expected to be locked
	if (!fVisibleContentRegionValid) {
		GetContentRegion(&fVisibleContentRegion);
		fVisibleContentRegion.IntersectWith(&fVisibleRegion);
	}
	return fVisibleContentRegion;
}


// #pragma mark -


void
Window::_PropagatePosition()
{
	if ((fFlags & B_SAME_POSITION_IN_ALL_WORKSPACES) == 0)
		return;

	for (int32 i = 0; i < kListCount; i++) {
		Anchor(i).position = fFrame.LeftTop();
	}
}


void
Window::MoveBy(int32 x, int32 y)
{
	// this function is only called from the desktop thread

	if (x == 0 && y == 0)
		return;

	fFrame.OffsetBy(x, y);
	_PropagatePosition();

	// take along the dirty region which is not
	// processed yet
	fDirtyRegion.OffsetBy(x, y);

	if (fBorderRegionValid)
		fBorderRegion.OffsetBy(x, y);
	if (fContentRegionValid)
		fContentRegion.OffsetBy(x, y);

	if (fCurrentUpdateSession->IsUsed())
		fCurrentUpdateSession->MoveBy(x, y);
	if (fPendingUpdateSession->IsUsed())
		fPendingUpdateSession->MoveBy(x, y);

	fEffectiveDrawingRegionValid = false;

	if (fDecorator)
		fDecorator->MoveBy(x, y);

	if (fTopView != NULL) {
		fTopView->MoveBy(x, y, NULL);
		fTopView->UpdateOverlay();
	}

	// the desktop will take care of dirty regions

	// dispatch a message to the client informing about the changed size
	BMessage msg(B_WINDOW_MOVED);
	msg.AddInt64("when", system_time());
	msg.AddPoint("where", fFrame.LeftTop());
	fWindow->SendMessageToClient(&msg);
}


void
Window::ResizeBy(int32 x, int32 y, BRegion* dirtyRegion)
{
	// this function is only called from the desktop thread

	int32 wantWidth = fFrame.IntegerWidth() + x;
	int32 wantHeight = fFrame.IntegerHeight() + y;

	// enforce size limits
	if (wantWidth < fMinWidth)
		wantWidth = fMinWidth;
	if (wantWidth > fMaxWidth)
		wantWidth = fMaxWidth;

	if (wantHeight < fMinHeight)
		wantHeight = fMinHeight;
	if (wantHeight > fMaxHeight)
		wantHeight = fMaxHeight;

	x = wantWidth - fFrame.IntegerWidth();
	y = wantHeight - fFrame.IntegerHeight();

	if (x == 0 && y == 0)
		return;

	fFrame.right += x;
	fFrame.bottom += y;

	fBorderRegionValid = false;
	fContentRegionValid = false;
	fEffectiveDrawingRegionValid = false;

	if (fDecorator) {
		fDecorator->ResizeBy(x, y, dirtyRegion);
//if (dirtyRegion) {
//fDrawingEngine->FillRegion(*dirtyRegion, (rgb_color){ 255, 255, 0, 255 });
//snooze(40000);
//}
	}

	if (fTopView != NULL) {
		fTopView->ResizeBy(x, y, dirtyRegion);
		fTopView->UpdateOverlay();
	}

//if (dirtyRegion)
//fDrawingEngine->FillRegion(*dirtyRegion, (rgb_color){ 0, 255, 255, 255 });

	// send a message to the client informing about the changed size
	BRect frame(Frame());
	BMessage msg(B_WINDOW_RESIZED);
	msg.AddInt64("when", system_time());
	msg.AddInt32("width", frame.IntegerWidth());
	msg.AddInt32("height", frame.IntegerHeight());
	fWindow->SendMessageToClient(&msg);
}


void
Window::ScrollViewBy(View* view, int32 dx, int32 dy)
{
	// this is executed in ServerWindow with the Readlock
	// held

	if (!view || view == fTopView || (dx == 0 && dy == 0))
		return;

	BRegion* dirty = fRegionPool.GetRegion();
	if (!dirty)
		return;

	view->ScrollBy(dx, dy, dirty);

//fDrawingEngine->FillRegion(*dirty, (rgb_color){ 255, 0, 255, 255 });
//snooze(20000);

	if (!IsOffscreenWindow() && IsVisible() && view->IsVisible()) {
		dirty->IntersectWith(&VisibleContentRegion());
		_TriggerContentRedraw(*dirty);
	}

	fRegionPool.Recycle(dirty);
}


//! Takes care of invalidating parts that could not be copied
void
Window::CopyContents(BRegion* region, int32 xOffset, int32 yOffset)
{
	// executed in ServerWindow thread with the read lock held
	if (!IsVisible())
		return;

	BRegion* newDirty = fRegionPool.GetRegion(*region);

	// clip the region to the visible contents at the
	// source and destination location (note that VisibleContentRegion()
	// is used once to make sure it is valid, then fVisibleContentRegion
	// is used directly)
	region->IntersectWith(&VisibleContentRegion());
	if (region->CountRects() > 0) {
		// Constrain to content region at destination
		region->OffsetBy(xOffset, yOffset);
		region->IntersectWith(&fVisibleContentRegion);
		if (region->CountRects() > 0) {
			// if the region still contains any rects
			// offset to source location again
			region->OffsetBy(-xOffset, -yOffset);

			BRegion* allDirtyRegions = fRegionPool.GetRegion(fDirtyRegion);
			if (allDirtyRegions != NULL) {
				if (fPendingUpdateSession->IsUsed()) {
					allDirtyRegions->Include(
						&fPendingUpdateSession->DirtyRegion());
				}
				if (fCurrentUpdateSession->IsUsed()) {
					allDirtyRegions->Include(
						&fCurrentUpdateSession->DirtyRegion());
				}
				// Get just the part of the dirty regions which is semantically
				// copied along
				allDirtyRegions->IntersectWith(region);
			}

			BRegion* copyRegion = fRegionPool.GetRegion(*region);
			if (copyRegion != NULL) {
				// never copy what's already dirty
				if (allDirtyRegions != NULL)
					copyRegion->Exclude(allDirtyRegions);

				fDrawingEngine->CopyRegion(copyRegion, xOffset, yOffset);

				// Prevent those parts from being added to the dirty region...
				newDirty->Exclude(copyRegion);

				// The parts that could be copied are not dirty (at the
				// target location!)
				copyRegion->OffsetBy(xOffset, yOffset);
				// ... and even exclude them from the pending dirty region!
				if (fPendingUpdateSession->IsUsed())
					fPendingUpdateSession->DirtyRegion().Exclude(copyRegion);

				fRegionPool.Recycle(copyRegion);
			} else {
				// Fallback, should never be here.
				fDrawingEngine->CopyRegion(region, xOffset, yOffset);
			}

			if (allDirtyRegions != NULL)
				fRegionPool.Recycle(allDirtyRegions);
		}
	}
	// what is left visible from the original region
	// at the destination after the region which could be
	// copied has been excluded, is considered dirty
	// NOTE: it may look like dirty regions are not moved
	// if no region could be copied, but that's alright,
	// since these parts will now be in newDirty anyways
	// (with the right offset)
	newDirty->OffsetBy(xOffset, yOffset);
	newDirty->IntersectWith(&fVisibleContentRegion);
	if (newDirty->CountRects() > 0)
		ProcessDirtyRegion(*newDirty);

	fRegionPool.Recycle(newDirty);
}


// #pragma mark -


void
Window::SetTopView(View* topView)
{
	fTopView = topView;

	if (fTopView) {
		// the top view is special, it has a coordinate system
		// as if it was attached directly to the desktop, therefor,
		// the coordinate conversion through the view tree works
		// as expected, since the top view has no "parent" but has
		// fFrame as if it had

		// make sure the location of the top view on screen matches ours
		fTopView->MoveBy((int32)(fFrame.left - fTopView->Frame().left),
			(int32)(fFrame.top - fTopView->Frame().top), NULL);

		// make sure the size of the top view matches ours
		fTopView->ResizeBy((int32)(fFrame.Width() - fTopView->Frame().Width()),
			(int32)(fFrame.Height() - fTopView->Frame().Height()), NULL);

		fTopView->AttachedToWindow(this);
	}
}


View*
Window::ViewAt(const BPoint& where)
{
	return fTopView->ViewAt(where);
}


window_anchor&
Window::Anchor(int32 index)
{
	return fAnchor[index];
}


Window*
Window::NextWindow(int32 index) const
{
	return fAnchor[index].next;
}


Window*
Window::PreviousWindow(int32 index) const
{
	return fAnchor[index].previous;
}


void
Window::SetScreen(const ::Screen* screen)
{
	ASSERT_MULTI_WRITE_LOCKED(fDesktop->ScreenLocker());
	fScreen = screen;
}


const ::Screen*
Window::Screen() const
{
	ASSERT_MULTI_READ_LOCKED(fDesktop->ScreenLocker());
	return fScreen;
}


// #pragma mark -


void
Window::GetEffectiveDrawingRegion(View* view, BRegion& region)
{
	if (!fEffectiveDrawingRegionValid) {
		fEffectiveDrawingRegion = VisibleContentRegion();
		if (fUpdateRequested && !fInUpdate) {
			// We requested an update, but the client has not started it yet,
			// so it is only allowed to draw outside the pending update sessions
			// region
			fEffectiveDrawingRegion.Exclude(
				&fPendingUpdateSession->DirtyRegion());
		} else if (fInUpdate) {
			// enforce the dirty region of the update session
			fEffectiveDrawingRegion.IntersectWith(
				&fCurrentUpdateSession->DirtyRegion());
		} else {
			// not in update, the view can draw everywhere
//printf("Window(%s)::GetEffectiveDrawingRegion(for %s) - outside update\n", Title(), view->Name());
		}

		fEffectiveDrawingRegionValid = true;
	}

	// TODO: this is a region that needs to be cached later in the server
	// when the current view in ServerWindow is set, and we are currently
	// in an update (fInUpdate), than we can set this region and remember
	// it for the comming drawing commands until the current view changes
	// again or fEffectiveDrawingRegionValid is suddenly false.
	region = fEffectiveDrawingRegion;
	if (!fContentRegionValid)
		_UpdateContentRegion();

	region.IntersectWith(&view->ScreenAndUserClipping(&fContentRegion));
}


bool
Window::DrawingRegionChanged(View* view) const
{
	return !fEffectiveDrawingRegionValid || !view->IsScreenClippingValid();
}


void
Window::ProcessDirtyRegion(BRegion& region)
{
	// if this is exectuted in the desktop thread,
	// it means that the window thread currently
	// blocks to get the read lock, if it is
	// executed from the window thread, it should
	// have the read lock and the desktop thread
	// is blocking to get the write lock. IAW, this
	// is only executed in one thread.
	if (fDirtyRegion.CountRects() == 0) {
		// the window needs to be informed
		// when the dirty region was empty.
		// NOTE: when the window thread has processed
		// the dirty region in MessageReceived(),
		// it will make the region empty again,
		// when it is empty here, we need to send
		// the message to initiate the next update round.
		// Until the message is processed in the window
		// thread, the desktop thread can add parts to
		// the region as it likes.
		ServerWindow()->RequestRedraw();
	}

	fDirtyRegion.Include(&region);
	fDirtyCause |= UPDATE_EXPOSE;
}


void
Window::RedrawDirtyRegion()
{
	// executed from ServerWindow with the read lock held

	if (IsVisible()) {
		_DrawBorder();

		BRegion* dirtyContentRegion =
			fRegionPool.GetRegion(VisibleContentRegion());
		dirtyContentRegion->IntersectWith(&fDirtyRegion);

		_TriggerContentRedraw(*dirtyContentRegion);

		fRegionPool.Recycle(dirtyContentRegion);
	}

	// reset the dirty region, since
	// we're fully clean. If the desktop
	// thread wanted to mark something
	// dirty in the mean time, it was
	// blocking on the global region lock to
	// get write access, since we're holding
	// the read lock for the whole time.
	fDirtyRegion.MakeEmpty();
	fDirtyCause = 0;
}


void
Window::MarkDirty(BRegion& regionOnScreen)
{
	// for marking any part of the desktop dirty
	// this will get write access to the global
	// region lock, and result in ProcessDirtyRegion()
	// to be called for any windows affected
	if (fDesktop)
		fDesktop->MarkDirty(regionOnScreen);
}


void
Window::MarkContentDirty(BRegion& regionOnScreen)
{
	// for triggering AS_REDRAW
	// since this won't affect other windows, read locking
	// is sufficient. If there was no dirty region before,
	// an update message is triggered
	if (fHidden || IsOffscreenWindow())
		return;

	regionOnScreen.IntersectWith(&VisibleContentRegion());
	fDirtyCause |= UPDATE_REQUEST;
	_TriggerContentRedraw(regionOnScreen);
}


void
Window::MarkContentDirtyAsync(BRegion& regionOnScreen)
{
	// NOTE: see comments in ProcessDirtyRegion()
	if (fHidden || IsOffscreenWindow())
		return;

	regionOnScreen.IntersectWith(&VisibleContentRegion());

	if (fDirtyRegion.CountRects() == 0) {
		ServerWindow()->RequestRedraw();
	}

	fDirtyRegion.Include(&regionOnScreen);
	fDirtyCause |= UPDATE_REQUEST;
}


void
Window::InvalidateView(View* view, BRegion& viewRegion)
{
	if (view && IsVisible() && view->IsVisible()) {
		if (!fContentRegionValid)
			_UpdateContentRegion();

		view->ConvertToScreen(&viewRegion);
		viewRegion.IntersectWith(&VisibleContentRegion());
		if (viewRegion.CountRects() > 0) {
			viewRegion.IntersectWith(
				&view->ScreenAndUserClipping(&fContentRegion));

//fDrawingEngine->FillRegion(viewRegion, rgb_color{ 0, 255, 0, 255 });
//snooze(10000);
			fDirtyCause |= UPDATE_REQUEST;
			_TriggerContentRedraw(viewRegion);
		}
	}
}


// DisableUpdateRequests
void
Window::DisableUpdateRequests()
{
	fUpdatesEnabled = false;
}


// EnableUpdateRequests
void
Window::EnableUpdateRequests()
{
	fUpdatesEnabled = true;
	if (!fUpdateRequested && fPendingUpdateSession->IsUsed())
		_SendUpdateMessage();
}

// #pragma mark -


static const bigtime_t kWindowActivationTimeout = 500000LL;


void
Window::MouseDown(BMessage* message, BPoint where, int32* _viewToken)
{
	DesktopSettings desktopSettings(fDesktop);

	// TODO: move into Decorator
	if (!fBorderRegionValid)
		GetBorderRegion(&fBorderRegion);

	int32 modifiers = _ExtractModifiers(message);
	bool inBorderRegion = fBorderRegion.Contains(where);
	bool windowModifier = (fFlags & B_NO_SERVER_SIDE_WINDOW_MODIFIERS) == 0
		&& (modifiers & (B_COMMAND_KEY | B_CONTROL_KEY | B_OPTION_KEY
			| B_SHIFT_KEY)) == (B_COMMAND_KEY | B_CONTROL_KEY);

	// default action is to drag the Window
	if (windowModifier || inBorderRegion) {
		// clicking Window visible area

		click_type action = DEC_NONE;
		int32 buttons = _ExtractButtons(message);

		if (inBorderRegion && fDecorator != NULL)
			action = _ActionFor(message, buttons, modifiers);
		else {
			if ((buttons & B_SECONDARY_MOUSE_BUTTON) != 0)
				action = DEC_MOVETOBACK;
			else if ((fFlags & B_NOT_MOVABLE) == 0 && fDecorator != NULL)
				action = DEC_DRAG;
		}

		if (!desktopSettings.AcceptFirstClick()) {
			// ignore clicks on decorator buttons if the
			// non-floating window doesn't have focus
			if (!IsFocus() && !IsFloating() && action != DEC_MOVETOBACK
				&& action != DEC_RESIZE && action != DEC_SLIDETAB)
				action = DEC_DRAG;
		}

		// set decorator internals
		switch (action) {
			case DEC_CLOSE:
				fIsClosing = true;
				STRACE_CLICK(("===> DEC_CLOSE\n"));
				break;

			case DEC_ZOOM:
				fIsZooming = true;
				STRACE_CLICK(("===> DEC_ZOOM\n"));
				break;

			case DEC_MINIMIZE:
				fIsMinimizing = true;
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

		if (fDecorator != NULL) {
			// redraw decorator
			BRegion* visibleBorder = fRegionPool.GetRegion();
			GetBorderRegion(visibleBorder);
			visibleBorder->IntersectWith(&VisibleRegion());

			DrawingEngine* engine = fDecorator->GetDrawingEngine();
			engine->LockParallelAccess();
			engine->ConstrainClippingRegion(visibleBorder);

			if (fIsZooming) {
				fDecorator->SetZoom(true);
			} else if (fIsClosing) {
				fDecorator->SetClose(true);
			} else if (fIsMinimizing) {
				fDecorator->SetMinimize(true);
			}

			engine->UnlockParallelAccess();

			fRegionPool.Recycle(visibleBorder);
		}

		// Re-stack and re-snap the window if in stacking & snapping mode
		if (fDesktop->fIsStackingAndSnapping) {
			_RemoveStackingAndSnapping();
			_RemoveStackingPersistently();
			_RemoveSnappingPersistently();
			_CheckIfReadyToStack();
			_CheckIfReadyToSnap();
		}

		// based on what the Decorator returned, properly place this window.
		if (action == DEC_MOVETOBACK) {
			if (desktopSettings.MouseMode() == B_CLICK_TO_FOCUS_MOUSE) {
				bool covered = true;
				BRegion fullRegion;
				GetFullRegion(&fullRegion);
				if (fullRegion == VisibleRegion()) {
    				// window is overlapped.
    				covered = false;
				}
				if (this != fDesktop->FrontWindow() && covered)
					fDesktop->ActivateWindow(this);
				else
					fDesktop->SendWindowBehind(this);
			} else
				fDesktop->SendWindowBehind(this);
		} else {
			fDesktop->SetMouseEventWindow(this);

			// activate window if in click to activate mode, else only focus it
			if (desktopSettings.MouseMode() == B_NORMAL_MOUSE)
				fDesktop->ActivateWindow(this);
			else {
				fDesktop->SetFocusWindow(this);
				if (desktopSettings.MouseMode() == B_FOCUS_FOLLOWS_MOUSE
					&& action == DEC_DRAG) {
					fActivateOnMouseUp = true;
					fMouseMoveDistance = 0.0f;
					fLastMoveTime = system_time();
				}
			}
		}
	} else {
		// click was inside the window contents
		if (View* view = ViewAt(where)) {
			if (HasModal())
				return;

			// clicking a simple View
			if (!IsFocus()) {
				bool acceptFirstClick = desktopSettings.AcceptFirstClick()
					|| ((Flags() & B_WILL_ACCEPT_FIRST_CLICK) != 0);
				bool avoidFocus = (Flags() & B_AVOID_FOCUS) != 0;

				// Activate or focus the window in case it doesn't accept first
				// click, depending on the mouse mode
				if (desktopSettings.MouseMode() == B_NORMAL_MOUSE
					&& !acceptFirstClick)
					fDesktop->ActivateWindow(this);
				else if (!avoidFocus)
					fDesktop->SetFocusWindow(this);

				// Eat the click if we don't accept first click
				// (B_AVOID_FOCUS never gets the focus, so they always accept
				// the first click)
				// TODO: the latter is unlike BeOS - if we really wanted to
				// imitate this behaviour, we would need to check if we're
				// the front window instead of the focus window
				if (!acceptFirstClick && !avoidFocus)
					return;
			}

			// fill out view token for the view under the mouse
			*_viewToken = view->Token();
			view->MouseDown(message, where);
		}
	}
}


void
Window::MouseUp(BMessage* message, BPoint where, int32* _viewToken)
{
	bool invalidate = false;
	if (fDecorator) {
		click_type action = _ActionFor(message);

		// redraw decorator
		BRegion* visibleBorder = fRegionPool.GetRegion();
		GetBorderRegion(visibleBorder);
		visibleBorder->IntersectWith(&VisibleRegion());

		DrawingEngine* engine = fDecorator->GetDrawingEngine();
		engine->LockParallelAccess();
		engine->ConstrainClippingRegion(visibleBorder);

		if (fIsZooming) {
			fIsZooming = false;
			fDecorator->SetZoom(false);
			if (action == DEC_ZOOM) {
				invalidate = true;
				fWindow->NotifyZoom();
			}
		}
		if (fIsClosing) {
			fIsClosing = false;
			fDecorator->SetClose(false);
			if (action == DEC_CLOSE) {
				invalidate = true;
				fWindow->NotifyQuitRequested();
			}
		}
		if (fIsMinimizing) {
			fIsMinimizing = false;
			fDecorator->SetMinimize(false);
			if (action == DEC_MINIMIZE) {
				invalidate = true;
				fWindow->NotifyMinimize(true);
			}
		}

		// Snap or stack the window if in stack & tiling mode
		if (fDesktop->fIsStackingAndSnapping) {
			_RemoveStackingAndSnapping();
			_RemoveStackingPersistently();
			_RemoveSnappingPersistently();
			_CheckIfReadyToStack();
			_CheckIfReadyToSnap();
			_StackWindow();
			_SnapWindow();
			StackAndTile();
			fDesktop->FinishStackingAndSnapping();

			//Activate this window so that others stacked/tiled along with it
			//stay on top.
			fForceActivate = true;
		}

		engine->UnlockParallelAccess();

		fRegionPool.Recycle(visibleBorder);

		int32 buttons;
		if (message->FindInt32("buttons", &buttons) != B_OK)
			buttons = 0;

		// if the primary mouse button is released, stop
		// dragging/resizing/sliding
		if ((buttons & B_PRIMARY_MOUSE_BUTTON) == 0) {
			fIsDragging = false;
			fIsResizing = false;
			fIsSlidingTab = false;
		}
	}

	// This will be true right after stacking & tiling mode
	if (fForceActivate) {
		fDesktop->ActivateWindow(this);
		fForceActivate = false;
	}
	// in FFM mode, activate the window and bring it
	// to front in case this was a drag click but the
	// mouse was not moved
	else if (fActivateOnMouseUp) {
		fActivateOnMouseUp = false;
		// on R5, there is a time window for this feature
		// ie, click and press too long, nothing will happen
		if (system_time() - fLastMoveTime < kWindowActivationTimeout)
			fDesktop->ActivateWindow(this);
	}

	if (View* view = ViewAt(where)) {
		if (HasModal())
			return;

		*_viewToken = view->Token();
		view->MouseUp(message, where);
	}
}


void
Window::MouseMoved(BMessage *message, BPoint where, int32* _viewToken,
	bool isLatestMouseMoved, bool isFake)
{
#if 0
	if (fDecorator != NULL && fTopView != NULL) {
		DrawingEngine* engine = fDecorator->GetDrawingEngine();
		engine->LockParallelAccess();
		engine->ConstrainClippingRegion(&VisibleRegion());

		fTopView->MarkAt(engine, where);
		engine->UnlockParallelAccess();
	}
#endif

	View* view = ViewAt(where);
	if (view != NULL)
		*_viewToken = view->Token();

	// ignore pointer history
	if (!isLatestMouseMoved)
		return;

	// limit the rate at which "mouse moved" events
	// are handled that move or resize the window
	bigtime_t now = 0;
	if (fIsDragging || fIsResizing) {
		now = system_time();
		if (now - fLastMoveTime < 13333) {
			// TODO: add a "timed event" to query for
			// the then current mouse position
			return;
		}
		if (fActivateOnMouseUp) {
			if (now - fLastMoveTime >= kWindowActivationTimeout) {
				// This click is too long already for window activation.
				fActivateOnMouseUp = false;
			}
		} else
			fLastMoveTime = now;
	}

	if (fDecorator) {
		BRegion* visibleBorder = fRegionPool.GetRegion();
		GetBorderRegion(visibleBorder);
		visibleBorder->IntersectWith(&VisibleRegion());

		DrawingEngine* engine = fDecorator->GetDrawingEngine();
		engine->LockParallelAccess();
		engine->ConstrainClippingRegion(visibleBorder);

		if (fIsZooming) {
			fDecorator->SetZoom(_ActionFor(message) == DEC_ZOOM);
		} else if (fIsClosing) {
			fDecorator->SetClose(_ActionFor(message) == DEC_CLOSE);
		} else if (fIsMinimizing) {
			fDecorator->SetMinimize(_ActionFor(message) == DEC_MINIMIZE);
		}

		engine->UnlockParallelAccess();
		fRegionPool.Recycle(visibleBorder);
	}

	BPoint delta = where - fLastMousePosition;
	// NOTE: "delta" is later used to change fLastMousePosition.
	// If for some reason no change should take effect, delta
	// is to be set to (0, 0) so that fLastMousePosition is not
	// adjusted. This way the relative mouse position to the
	// item being changed (border during resizing, tab during
	// sliding...) stays fixed when the mouse is moved so that
	// changes are taking effect again.

	// If the window was moved enough, it doesn't come to
	// the front in FFM mode when the mouse is released.
	if (fActivateOnMouseUp) {
		fMouseMoveDistance += delta.x * delta.x + delta.y * delta.y;
		if (fMouseMoveDistance > 16.0f)
			fActivateOnMouseUp = false;
		else
			delta = B_ORIGIN;
	}

	// moving
	if (fIsDragging) {
		if (!(Flags() & B_NOT_MOVABLE)) {
			BPoint oldLeftTop = fFrame.LeftTop();

			_AlterDeltaForSnap(delta, now);
			fDesktop->MoveWindowBy(this, delta.x, delta.y);

			// Dragging in StackingAndSnapping mode means stacking and
			// snapping of this window is removed and can be changed.
			// Otherwise, dragging means the stack & tile constraints are
			// solved and windows adjusted accordingly.
			if (fDesktop->fIsStackingAndSnapping) {
				_RemoveStackingAndSnapping();
				_RemoveStackingPersistently();
				_RemoveSnappingPersistently();
				_CheckIfReadyToStack();
				_CheckIfReadyToSnap();
			} else
				StackAndTile();

			// constrain delta to true change in size
			delta = fFrame.LeftTop() - oldLeftTop;
		} else
			delta = BPoint(0, 0);
	}
	// resizing
	if (fIsResizing) {
		if (!(Flags() & B_NOT_RESIZABLE)) {
			if (Flags() & B_NOT_V_RESIZABLE)
				delta.y = 0;
			if (Flags() & B_NOT_H_RESIZABLE)
				delta.x = 0;

			BPoint oldRightBottom = fFrame.RightBottom();

			fDesktop->ResizeWindowBy(this, delta.x, delta.y);

			// Resizing in StackingAndSnapping mode means stacking and
			// snapping of this window is removed and can be changed.
			// Otherwise, resizing means the stack & tile constraints are
			// solved and windows adjusted accordingly.
			if (fDesktop->fIsStackingAndSnapping) {
				_RemoveStackingAndSnapping();
				_RemoveStackingPersistently();
				_RemoveSnappingPersistently();
				_CheckIfReadyToSnap();
			} else
				StackAndTile();

			// constrain delta to true change in size
			delta = fFrame.RightBottom() - oldRightBottom;
		} else
			delta = BPoint(0, 0);
	}
	// sliding tab
	if (fIsSlidingTab) {
		float loc = TabLocation();
		// TODO: change to [0:1]
		loc += delta.x;
		if (fDesktop->SetWindowTabLocation(this, loc))
			delta.y = 0;
		else
			delta = BPoint(0, 0);
	}

	// NOTE: fLastMousePosition is currently only
	// used for window moving/resizing/sliding the tab
	fLastMousePosition += delta;

	// change focus in FFM mode
	DesktopSettings desktopSettings(fDesktop);
	if (desktopSettings.FocusFollowsMouse()
		&& !IsFocus() && !(Flags() & B_AVOID_FOCUS)) {
		// If the mouse move is a fake one, we set the focus to NULL, which
		// will cause the window that had focus last to retrieve it again - this
		// makes FFM much nicer to use with the keyboard.
		fDesktop->SetFocusWindow(isFake ? NULL : this);
	}

	// mouse cursor

	if (view != NULL) {
		view->MouseMoved(message, where);

		// TODO: there is more for real cursor support, ie. if a window is closed,
		//		new app cursor shouldn't override view cursor, ...
		ServerWindow()->App()->SetCurrentCursor(view->Cursor());
	}
}


void
Window::_AlterDeltaForSnap(BPoint& delta, bigtime_t now)
{
	// Alter the delta (which is a proposed offset used while dragging a
	// window) so that the frame of the window 'snaps' to the edges of the
	// screen.

	const bigtime_t kSnappingDuration = 1500000LL;
	const bigtime_t kSnappingPause = 3000000LL;
	const float kSnapDistance = 8.0f;

	if (now - fLastSnapTime > kSnappingDuration
		&& now - fLastSnapTime < kSnappingPause) {
		// Maintain a pause between snapping.
		return;
	}

	BRect frame = fFrame;
	BPoint offsetWithinFrame;
	// TODO: Perhaps obtain the usable area (not covered by the Deskbar)?
	BRect screenFrame = Screen()->Frame();

	if (fDecorator) {
		BRegion reg;
		fDecorator->GetFootprint(&reg);
		frame = reg.Frame();
		offsetWithinFrame.x = fFrame.left - frame.left;
		offsetWithinFrame.y = fFrame.top - frame.top;
	}

	frame.OffsetBy(delta);

	float leftDist = fabs(frame.left - screenFrame.left);
	float topDist = fabs(frame.top - screenFrame.top);
	float rightDist = fabs(frame.right - screenFrame.right);
	float bottomDist = fabs(frame.bottom - screenFrame.bottom);

	bool snapped = false;
	if (leftDist < kSnapDistance || rightDist < kSnapDistance) {
		snapped = true;
		if (leftDist < rightDist) {
			frame.right -= frame.left;
			frame.left = 0.0f;
		} else {
			frame.left -= frame.right - screenFrame.right;
			frame.right = screenFrame.right;
		}
	}

	if (topDist < kSnapDistance || bottomDist < kSnapDistance) {
		snapped = true;
		if (topDist < bottomDist) {
			frame.bottom -= frame.top;
			frame.top = 0.0f;
		} else {
			frame.top -= frame.bottom - screenFrame.bottom;
			frame.bottom = screenFrame.bottom;
		}
	}
	if (snapped && now - fLastSnapTime > kSnappingPause)
		fLastSnapTime = now;


	frame.top += offsetWithinFrame.y;
	frame.left += offsetWithinFrame.x;

	delta.y = frame.top - fFrame.top;
	delta.x = frame.left - fFrame.left;
}


// #pragma mark -


void
Window::WorkspaceActivated(int32 index, bool active)
{
	BMessage activatedMsg(B_WORKSPACE_ACTIVATED);
	activatedMsg.AddInt64("when", system_time());
	activatedMsg.AddInt32("workspace", index);
	activatedMsg.AddBool("active", active);

	ServerWindow()->SendMessageToClient(&activatedMsg);
}


void
Window::WorkspacesChanged(uint32 oldWorkspaces, uint32 newWorkspaces)
{
	fWorkspaces = newWorkspaces;

	BMessage changedMsg(B_WORKSPACES_CHANGED);
	changedMsg.AddInt64("when", system_time());
	changedMsg.AddInt32("old", oldWorkspaces);
	changedMsg.AddInt32("new", newWorkspaces);

	ServerWindow()->SendMessageToClient(&changedMsg);
}


void
Window::Activated(bool active)
{
	BMessage msg(B_WINDOW_ACTIVATED);
	msg.AddBool("active", active);
	ServerWindow()->SendMessageToClient(&msg);
}


//# pragma mark -


void
Window::SetTitle(const char* name, BRegion& dirty)
{
	// rebuild the clipping for the title area
	// and redraw it.

	fTitle = name;

	if (fDecorator) {
		fDecorator->SetTitle(name, &dirty);

		fBorderRegionValid = false;
			// the border very likely changed
	}
}


void
Window::SetFocus(bool focus)
{
	// executed from Desktop thread
	// it holds the clipping write lock,
	// so the window thread cannot be
	// accessing fIsFocus

	BRegion* dirty = fRegionPool.GetRegion(fBorderRegion);
	if (dirty) {
		dirty->IntersectWith(&fVisibleRegion);
		fDesktop->MarkDirty(*dirty);
		fRegionPool.Recycle(dirty);
	}

	fIsFocus = focus;
	if (fDecorator)
		fDecorator->SetFocus(focus);

	Activated(focus);
}


void
Window::SetHidden(bool hidden)
{
	// the desktop takes care of dirty regions
	if (fHidden != hidden) {
		fHidden = hidden;

		fTopView->SetHidden(hidden);

		// TODO: anything else?
	}
}


void
Window::SetMinimized(bool minimized)
{
	if (minimized == fMinimized)
		return;

	fMinimized = minimized;
}


bool
Window::IsVisible() const
{
	if (IsOffscreenWindow())
		return true;

	if (IsHidden())
		return false;

/*
	if (fVisibleRegion.CountRects() == 0)
		return false;
*/
	return fCurrentWorkspace >= 0 && fCurrentWorkspace < kWorkingList;
}


void
Window::SetSizeLimits(int32 minWidth, int32 maxWidth, int32 minHeight,
	int32 maxHeight)
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
	if (fDecorator) {
		fDecorator->GetSizeLimits(&fMinWidth, &fMinHeight, &fMaxWidth,
			&fMaxHeight);
	}

	_ObeySizeLimits();
}


void
Window::GetSizeLimits(int32* minWidth, int32* maxWidth,
	int32* minHeight, int32* maxHeight) const
{
	*minWidth = fMinWidth;
	*maxWidth = fMaxWidth;
	*minHeight = fMinHeight;
	*maxHeight = fMaxHeight;
}


bool
Window::SetTabLocation(float location, BRegion& dirty)
{
	bool ret = false;
	if (fDecorator) {
		ret = fDecorator->SetTabLocation(location, &dirty);
		// the border region changed if ret is true
		fBorderRegionValid = fBorderRegionValid && !ret;
	}
	return ret;
}


float
Window::TabLocation() const
{
	if (fDecorator)
		return fDecorator->TabLocation();
	return 0.0;
}


bool
Window::SetDecoratorSettings(const BMessage& settings, BRegion& dirty)
{
	bool ret = false;
	if (fDecorator) {
		fDecorator->SetWindow(this);
		ret = fDecorator->SetSettings(settings, &dirty);
		// the border region changed if ret is true
		fBorderRegionValid = fBorderRegionValid && !ret;
	}
	return ret;
}


bool
Window::GetDecoratorSettings(BMessage* settings)
{
	if (fDecorator) {
		fDecorator->SetWindow(this);
		return fDecorator->GetSettings(settings);
	}

	return false;
}


void
Window::FontsChanged(BRegion* updateRegion)
{
	if (fDecorator != NULL) {
		DesktopSettings settings(fDesktop);
		fDecorator->FontsChanged(settings, updateRegion);
		fBorderRegionValid = false;
	}
}


void
Window::SetLook(window_look look, BRegion* updateRegion)
{
	if (fDecorator == NULL && look != B_NO_BORDER_WINDOW_LOOK) {
		// we need a new decorator
		fDecorator = gDecorManager.AllocateDecorator(fDesktop, fDrawingEngine,
			Frame(), Title(), fLook, fFlags);
		if (IsFocus())
			fDecorator->SetFocus(true);
	}

	fLook = look;

	fBorderRegionValid = false;
		// the border very likely changed
	fContentRegionValid = false;
		// mabye a resize handle was added...
	fEffectiveDrawingRegionValid = false;
		// ...and therefor the drawing region is
		// likely not valid anymore either

	if (fDecorator != NULL) {
		DesktopSettings settings(fDesktop);
		fDecorator->SetLook(settings, look, updateRegion);

		// we might need to resize the window!
		fDecorator->GetSizeLimits(&fMinWidth, &fMinHeight, &fMaxWidth,
			&fMaxHeight);
		_ObeySizeLimits();
	}

	if (look == B_NO_BORDER_WINDOW_LOOK) {
		// we don't need a decorator for this window
		delete fDecorator;
		fDecorator = NULL;
	}
}


void
Window::SetFeel(window_feel feel)
{
	// if the subset list is no longer needed, clear it
	if ((fFeel == B_MODAL_SUBSET_WINDOW_FEEL
			|| fFeel == B_FLOATING_SUBSET_WINDOW_FEEL)
		&& feel != B_MODAL_SUBSET_WINDOW_FEEL
		&& feel != B_FLOATING_SUBSET_WINDOW_FEEL)
		fSubsets.MakeEmpty();

	fFeel = feel;

	// having modal windows with B_AVOID_FRONT or B_AVOID_FOCUS doesn't
	// make that much sense, so we filter those flags out on demand
	fFlags = fOriginalFlags;
	fFlags &= ValidWindowFlags(fFeel);

	if (!IsNormal()) {
		fFlags |= B_SAME_POSITION_IN_ALL_WORKSPACES;
		_PropagatePosition();
	}
}


void
Window::SetFlags(uint32 flags, BRegion* updateRegion)
{
	fOriginalFlags = flags;
	fFlags = flags & ValidWindowFlags(fFeel);
	if (!IsNormal())
		fFlags |= B_SAME_POSITION_IN_ALL_WORKSPACES;

	if ((fFlags & B_SAME_POSITION_IN_ALL_WORKSPACES) != 0)
		_PropagatePosition();

	if (fDecorator == NULL)
		return;

	fDecorator->SetFlags(flags, updateRegion);

	fBorderRegionValid = false;
		// the border might have changed (smaller/larger tab)

	// we might need to resize the window!
	if (fDecorator) {
		fDecorator->GetSizeLimits(&fMinWidth, &fMinHeight, &fMaxWidth, &fMaxHeight);
		_ObeySizeLimits();
	}

// TODO: not sure if we want to do this
#if 0
	if ((fOriginalFlags & kWindowScreenFlag) != (flags & kWindowScreenFlag)) {
		// TODO: disabling needs to be nestable (or we might lose the previous
		// update state)
		if ((flags & kWindowScreenFlag) != 0)
			DisableUpdateRequests();
		else
			EnableUpdateRequests();
	}
#endif
}


/*!	Returns whether or not a window is in the workspace list with the
	specified \a index.
*/
bool
Window::InWorkspace(int32 index) const
{
	return (fWorkspaces & (1UL << index)) != 0;
}


bool
Window::SupportsFront()
{
	if (fFeel == kDesktopWindowFeel
		|| fFeel == kMenuWindowFeel
		|| (fFlags & B_AVOID_FRONT) != 0)
		return false;

	return true;
}


bool
Window::IsModal() const
{
	return IsModalFeel(fFeel);
}


bool
Window::IsFloating() const
{
	return IsFloatingFeel(fFeel);
}


bool
Window::IsNormal() const
{
	return !IsFloatingFeel(fFeel) && !IsModalFeel(fFeel);
}


bool
Window::HasModal() const
{
	for (Window* window = NextWindow(fCurrentWorkspace); window != NULL;
			window = window->NextWindow(fCurrentWorkspace)) {
		if (window->IsHidden() || !window->IsModal())
			continue;

		if (window->HasInSubset(this))
			return true;
	}

	return false;
}


/*!	\brief Returns the windows that's in behind of the backmost position
		this window can get.
	Returns NULL is this window can be the backmost window.

	\param workspace the workspace on which this check should be made. If
		the value is -1, the window's current workspace will be used.
*/
Window*
Window::Backmost(Window* window, int32 workspace)
{
	if (workspace == -1)
		workspace = fCurrentWorkspace;

	ASSERT(workspace != -1);
	if (workspace == -1)
		return NULL;

	// Desktop windows are always backmost
	if (fFeel == kDesktopWindowFeel)
		return NULL;

	if (window == NULL)
		window = PreviousWindow(workspace);

	for (; window != NULL; window = window->PreviousWindow(workspace)) {
		if (window->IsHidden() || window == this)
			continue;

		if (HasInSubset(window))
			return window;
	}

	return NULL;
}


/*!	\brief Returns the window that's in front of the frontmost position
		this window can get.
	Returns NULL if this window can be the frontmost window.

	\param workspace the workspace on which this check should be made. If
		the value is -1, the window's current workspace will be used.
*/
Window*
Window::Frontmost(Window* first, int32 workspace)
{
	if (workspace == -1)
		workspace = fCurrentWorkspace;

	ASSERT(workspace != -1);
	if (workspace == -1)
		return NULL;

	if (fFeel == kDesktopWindowFeel)
		return first ? first : NextWindow(workspace);

	if (first == NULL)
		first = NextWindow(workspace);

	for (Window* window = first; window != NULL;
			window = window->NextWindow(workspace)) {
		if (window->IsHidden() || window == this)
			continue;

		if (window->HasInSubset(this))
			return window;
	}

	return NULL;
}


bool
Window::AddToSubset(Window* window)
{
	return fSubsets.AddItem(window);
}


void
Window::RemoveFromSubset(Window* window)
{
	fSubsets.RemoveItem(window);
}


/*!	Returns whether or not a window is in the subset of this window.
	If a window is in the subset of this window, it means it should always
	appear behind this window.
*/
bool
Window::HasInSubset(const Window* window) const
{
	if (window == NULL || fFeel == window->Feel()
		|| fFeel == B_NORMAL_WINDOW_FEEL)
		return false;

	// Menus are a special case: they will always be on-top of every window
	// of their application
	if (fFeel == kMenuWindowFeel)
		return window->ServerWindow()->App() == ServerWindow()->App();
	if (window->Feel() == kMenuWindowFeel)
		return false;

	// we have a few special feels that have a fixed order

	const int32 kFeels[] = {kPasswordWindowFeel, kWindowScreenFeel,
		B_MODAL_ALL_WINDOW_FEEL, B_FLOATING_ALL_WINDOW_FEEL};

	for (uint32 order = 0;
			order < sizeof(kFeels) / sizeof(kFeels[0]); order++) {
		if (fFeel == kFeels[order])
			return true;
		if (window->Feel() == kFeels[order])
			return false;
	}

	if ((fFeel == B_FLOATING_APP_WINDOW_FEEL
			&& window->Feel() != B_MODAL_APP_WINDOW_FEEL)
		|| fFeel == B_MODAL_APP_WINDOW_FEEL)
		return window->ServerWindow()->App() == ServerWindow()->App();

	return fSubsets.HasItem(window);
}


/*!	\brief Collects all workspaces views in this window and puts it into \a list
*/
void
Window::FindWorkspacesViews(BObjectList<WorkspacesView>& list) const
{
	int32 count = fWorkspacesViewCount;
	fTopView->FindViews(kWorkspacesViewFlag, (BObjectList<View>&)list, count);
}


/*!	\brief Returns on which workspaces the window should be visible.

	A modal or floating window may be visible on a workspace if one
	of its subset windows is visible there. Floating windows also need
	to have a subset as front window to be visible.
*/
uint32
Window::SubsetWorkspaces() const
{
	if (fFeel == B_MODAL_ALL_WINDOW_FEEL
		|| fFeel == B_FLOATING_ALL_WINDOW_FEEL)
		return B_ALL_WORKSPACES;

	if (fFeel == B_FLOATING_APP_WINDOW_FEEL) {
		Window* front = fDesktop->FrontWindow();
		if (front != NULL && front->IsNormal()
			&& front->ServerWindow()->App() == ServerWindow()->App())
			return ServerWindow()->App()->Workspaces();

		return 0;
	}

	if (fFeel == B_MODAL_APP_WINDOW_FEEL) {
		uint32 workspaces = ServerWindow()->App()->Workspaces();
		if (workspaces == 0) {
			// The application doesn't seem to have any other windows
			// open or visible - but we'd like to see modal windows
			// anyway, at least when they are first opened.
			return 1UL << fDesktop->CurrentWorkspace();
		}
		return workspaces;
	}

	if (fFeel == B_MODAL_SUBSET_WINDOW_FEEL
		|| fFeel == B_FLOATING_SUBSET_WINDOW_FEEL) {
		uint32 workspaces = 0;
		bool hasNormalFront = false;
		for (int32 i = 0; i < fSubsets.CountItems(); i++) {
			Window* window = fSubsets.ItemAt(i);

			if (!window->IsHidden())
				workspaces |= window->Workspaces();
			if (window == fDesktop->FrontWindow() && window->IsNormal())
				hasNormalFront = true;
		}

		if (fFeel == B_FLOATING_SUBSET_WINDOW_FEEL && !hasNormalFront)
			return 0;

		return workspaces;
	}

	return 0;
}


/*!	Returns wether or not a window is in the subset workspace list with the
	specified \a index.
	See SubsetWorkspaces().
*/
bool
Window::InSubsetWorkspace(int32 index) const
{
	return (SubsetWorkspaces() & (1UL << index)) != 0;
}


// #pragma mark - static


/*static*/ bool
Window::IsValidLook(window_look look)
{
	return look == B_TITLED_WINDOW_LOOK
		|| look == B_DOCUMENT_WINDOW_LOOK
		|| look == B_MODAL_WINDOW_LOOK
		|| look == B_FLOATING_WINDOW_LOOK
		|| look == B_BORDERED_WINDOW_LOOK
		|| look == B_NO_BORDER_WINDOW_LOOK
		|| look == kDesktopWindowLook
		|| look == kLeftTitledWindowLook;
}


/*static*/ bool
Window::IsValidFeel(window_feel feel)
{
	return feel == B_NORMAL_WINDOW_FEEL
		|| feel == B_MODAL_SUBSET_WINDOW_FEEL
		|| feel == B_MODAL_APP_WINDOW_FEEL
		|| feel == B_MODAL_ALL_WINDOW_FEEL
		|| feel == B_FLOATING_SUBSET_WINDOW_FEEL
		|| feel == B_FLOATING_APP_WINDOW_FEEL
		|| feel == B_FLOATING_ALL_WINDOW_FEEL
		|| feel == kDesktopWindowFeel
		|| feel == kMenuWindowFeel
		|| feel == kWindowScreenFeel
		|| feel == kPasswordWindowFeel;
}


/*static*/ bool
Window::IsModalFeel(window_feel feel)
{
	return feel == B_MODAL_SUBSET_WINDOW_FEEL
		|| feel == B_MODAL_APP_WINDOW_FEEL
		|| feel == B_MODAL_ALL_WINDOW_FEEL;
}


/*static*/ bool
Window::IsFloatingFeel(window_feel feel)
{
	return feel == B_FLOATING_SUBSET_WINDOW_FEEL
		|| feel == B_FLOATING_APP_WINDOW_FEEL
		|| feel == B_FLOATING_ALL_WINDOW_FEEL;
}


/*static*/ uint32
Window::ValidWindowFlags()
{
	return B_NOT_MOVABLE
		| B_NOT_CLOSABLE
		| B_NOT_ZOOMABLE
		| B_NOT_MINIMIZABLE
		| B_NOT_RESIZABLE
		| B_NOT_H_RESIZABLE
		| B_NOT_V_RESIZABLE
		| B_AVOID_FRONT
		| B_AVOID_FOCUS
		| B_WILL_ACCEPT_FIRST_CLICK
		| B_OUTLINE_RESIZE
		| B_NO_WORKSPACE_ACTIVATION
		| B_NOT_ANCHORED_ON_ACTIVATE
		| B_ASYNCHRONOUS_CONTROLS
		| B_QUIT_ON_WINDOW_CLOSE
		| B_SAME_POSITION_IN_ALL_WORKSPACES
		| B_AUTO_UPDATE_SIZE_LIMITS
		| B_CLOSE_ON_ESCAPE
		| B_NO_SERVER_SIDE_WINDOW_MODIFIERS
		| kWindowScreenFlag
		| kAcceptKeyboardFocusFlag;
}


/*static*/ uint32
Window::ValidWindowFlags(window_feel feel)
{
	uint32 flags = ValidWindowFlags();
	if (IsModalFeel(feel))
		return flags & ~(B_AVOID_FOCUS | B_AVOID_FRONT);

	return flags;
}


// #pragma mark - private


void
Window::_ShiftPartOfRegion(BRegion* region, BRegion* regionToShift,
	int32 xOffset, int32 yOffset)
{
	BRegion* common = fRegionPool.GetRegion(*regionToShift);
	if (!common)
		return;
	// see if there is a common part at all
	common->IntersectWith(region);
	if (common->CountRects() > 0) {
		// cut the common part from the region,
		// offset that to destination and include again
		region->Exclude(common);
		common->OffsetBy(xOffset, yOffset);
		region->Include(common);
	}
	fRegionPool.Recycle(common);
}


void
Window::_TriggerContentRedraw(BRegion& dirtyContentRegion)
{
	if (!IsVisible() || dirtyContentRegion.CountRects() == 0
		|| (fFlags & kWindowScreenFlag) != 0)
		return;

	// put this into the pending dirty region
	// to eventually trigger a client redraw
	bool wasExpose = fPendingUpdateSession->IsExpose();
	BRegion* backgroundClearingRegion = &dirtyContentRegion;

	_TransferToUpdateSession(&dirtyContentRegion);

	if (fPendingUpdateSession->IsExpose()) {
		if (!fContentRegionValid)
			_UpdateContentRegion();

		if (!wasExpose) {
			// there was suddenly added a dirty region
			// caused by exposing content, we need to clear
			// the entire background
			backgroundClearingRegion = &fPendingUpdateSession->DirtyRegion();
		}

		if (fDrawingEngine->LockParallelAccess()) {
			bool copyToFrontEnabled = fDrawingEngine->CopyToFrontEnabled();
			fDrawingEngine->SetCopyToFrontEnabled(true);
			fDrawingEngine->SuspendAutoSync();

//sCurrentColor.red = rand() % 255;
//sCurrentColor.green = rand() % 255;
//sCurrentColor.blue = rand() % 255;
//sPendingColor.red = rand() % 255;
//sPendingColor.green = rand() % 255;
//sPendingColor.blue = rand() % 255;
//fDrawingEngine->FillRegion(*backgroundClearingRegion, sCurrentColor);
//snooze(10000);

			fTopView->Draw(fDrawingEngine, backgroundClearingRegion,
				&fContentRegion, true);

			fDrawingEngine->Sync();
			fDrawingEngine->SetCopyToFrontEnabled(copyToFrontEnabled);
			fDrawingEngine->UnlockParallelAccess();
		}
	}
}


void
Window::_DrawBorder()
{
	// this is executed in the window thread, but only
	// in respond to a REDRAW message having been received, the
	// clipping lock is held for reading

	if (!fDecorator)
		return;

	// construct the region of the border that needs redrawing
	BRegion* dirtyBorderRegion = fRegionPool.GetRegion();
	if (!dirtyBorderRegion)
		return;
	GetBorderRegion(dirtyBorderRegion);
	// intersect with our visible region
	dirtyBorderRegion->IntersectWith(&fVisibleRegion);
	// intersect with the dirty region
	dirtyBorderRegion->IntersectWith(&fDirtyRegion);

	DrawingEngine* engine = fDecorator->GetDrawingEngine();
	if (dirtyBorderRegion->CountRects() > 0 && engine->LockParallelAccess()) {
		engine->ConstrainClippingRegion(dirtyBorderRegion);
		bool copyToFrontEnabled = engine->CopyToFrontEnabled();
		engine->SetCopyToFrontEnabled(true);

		fDecorator->Draw(dirtyBorderRegion->Frame());

		engine->SetCopyToFrontEnabled(copyToFrontEnabled);

// TODO: remove this once the DrawState stuff is handled
// more cleanly. The reason why this is needed is that
// when the decorator draws strings, a draw state is set
// on the Painter object, and this is were it might get
// out of sync with what the ServerWindow things is the
// current DrawState set on the Painter
fWindow->ResyncDrawState();

		engine->UnlockParallelAccess();
	}
	fRegionPool.Recycle(dirtyBorderRegion);
}


/*!	pre: the clipping is readlocked (this function is
	only called from _TriggerContentRedraw()), which
	in turn is only called from MessageReceived() with
	the clipping lock held
*/
void
Window::_TransferToUpdateSession(BRegion* contentDirtyRegion)
{
	if (contentDirtyRegion->CountRects() <= 0)
		return;

//fDrawingEngine->FillRegion(*contentDirtyRegion, sPendingColor);
//snooze(20000);

	// add to pending
	fPendingUpdateSession->SetUsed(true);
//	if (!fPendingUpdateSession->IsExpose())
	fPendingUpdateSession->AddCause(fDirtyCause);
	fPendingUpdateSession->Include(contentDirtyRegion);

	if (!fUpdateRequested) {
		// send this to client
		_SendUpdateMessage();
		// the pending region is now the current,
		// though the update does not start until
		// we received BEGIN_UPDATE from the client
	}
}


void
Window::_SendUpdateMessage()
{
	if (!fUpdatesEnabled)
		return;

	BMessage message(_UPDATE_);
	if (ServerWindow()->SendMessageToClient(&message) != B_OK) {
		// If sending the message failed, we'll just keep adding to the dirty
		// region until sending was successful.
		// TODO: we might want to automatically resend this message in this case
		return;
	}

	fUpdateRequested = true;
	fEffectiveDrawingRegionValid = false;
}


void
Window::BeginUpdate(BPrivate::PortLink& link)
{
	// NOTE: since we might "shift" parts of the
	// internal dirty regions from the desktop thread
	// in response to Window::ResizeBy(), which
	// might move arround views, the user of this function
	// needs to hold the global clipping lock so that the internal
	// dirty regions are not messed with from the Desktop thread
	// and ServerWindow thread at the same time.

	if (!fUpdateRequested) {
		link.StartMessage(B_ERROR);
		link.Flush();
		fprintf(stderr, "Window::BeginUpdate() - no update requested!\n");
		return;
	}

	// make the pending update session the current update session
	// (toggle the pointers)
	UpdateSession* temp = fCurrentUpdateSession;
	fCurrentUpdateSession = fPendingUpdateSession;
	fPendingUpdateSession = temp;
	fPendingUpdateSession->SetUsed(false);
	// all drawing command from the client
	// will have the dirty region from the update
	// session enforced
	fInUpdate = true;
	fEffectiveDrawingRegionValid = false;

	// TODO: each view could be drawn individually
	// right before carrying out the first drawing
	// command from the client during an update
	// (View::IsBackgroundDirty() can be used
	// for this)
	if (!fContentRegionValid)
		_UpdateContentRegion();

	BRegion* dirty = fRegionPool.GetRegion(
		fCurrentUpdateSession->DirtyRegion());
	if (!dirty) {
		link.StartMessage(B_ERROR);
		link.Flush();
		return;
	}

	dirty->IntersectWith(&VisibleContentRegion());

//if (!fCurrentUpdateSession->IsExpose()) {
////sCurrentColor.red = rand() % 255;
////sCurrentColor.green = rand() % 255;
////sCurrentColor.blue = rand() % 255;
////sPendingColor.red = rand() % 255;
////sPendingColor.green = rand() % 255;
////sPendingColor.blue = rand() % 255;
//fDrawingEngine->FillRegion(*dirty, sCurrentColor);
//snooze(10000);
//}

	link.StartMessage(B_OK);
	// append the current window geometry to the
	// message, the client will need it
	link.Attach<BPoint>(fFrame.LeftTop());
	link.Attach<float>(fFrame.Width());
	link.Attach<float>(fFrame.Height());
	// find and attach all views that intersect with
	// the dirty region
	fTopView->AddTokensForViewsInRegion(link, *dirty, &fContentRegion);
	// mark the end of the token "list"
	link.Attach<int32>(B_NULL_TOKEN);
	link.Flush();

	// supress back to front buffer copies in the drawing engine
	fDrawingEngine->SetCopyToFrontEnabled(false);

	if (!fCurrentUpdateSession->IsExpose()
		&& fDrawingEngine->LockParallelAccess()) {
		fDrawingEngine->SuspendAutoSync();

		fTopView->Draw(fDrawingEngine, dirty, &fContentRegion, true);

		fDrawingEngine->Sync();
		fDrawingEngine->UnlockParallelAccess();
	} // else the background was cleared already

	fRegionPool.Recycle(dirty);
}


void
Window::EndUpdate()
{
	// NOTE: see comment in _BeginUpdate()

	if (fInUpdate) {
		// reenable copy to front
		fDrawingEngine->SetCopyToFrontEnabled(true);

		BRegion* dirty = fRegionPool.GetRegion(
			fCurrentUpdateSession->DirtyRegion());

		if (dirty) {
			dirty->IntersectWith(&VisibleContentRegion());

			fDrawingEngine->CopyToFront(*dirty);
			fRegionPool.Recycle(dirty);
		}

		fCurrentUpdateSession->SetUsed(false);

		fInUpdate = false;
		fEffectiveDrawingRegionValid = false;
	}
	if (fPendingUpdateSession->IsUsed()) {
		// send this to client
		_SendUpdateMessage();
	} else {
		fUpdateRequested = false;
	}
}


void
Window::_UpdateContentRegion()
{
	fContentRegion.Set(fFrame);

	// resize handle
	if (fDecorator) {
		if (!fBorderRegionValid)
			GetBorderRegion(&fBorderRegion);

		fContentRegion.Exclude(&fBorderRegion);
	}

	fContentRegionValid = true;
}


int32
Window::_ExtractButtons(const BMessage* message) const
{
	int32 buttons;
	if (message->FindInt32("buttons", &buttons) != B_OK)
		buttons = 0;
	return buttons;
}


int32
Window::_ExtractModifiers(const BMessage* message) const
{
	int32 modifiers;
	if (message->FindInt32("modifiers", &modifiers) != B_OK)
		modifiers = 0;
	return modifiers;
}


click_type
Window::_ActionFor(const BMessage* message) const
{
	if (fDecorator == NULL)
		return DEC_NONE;

	int32 buttons = _ExtractButtons(message);
	int32 modifiers = _ExtractModifiers(message);
	return _ActionFor(message, buttons, modifiers);
}


click_type
Window::_ActionFor(const BMessage* message, int32 buttons,
	int32 modifiers) const
{
	if (fDecorator == NULL)
		return DEC_NONE;

	BPoint where;
	if (message->FindPoint("where", &where) != B_OK)
		return DEC_NONE;

	return fDecorator->Clicked(where, buttons, modifiers);
}


void
Window::_ObeySizeLimits()
{
	// make sure we even have valid size limits
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

	if (fDesktop)
		fDesktop->ResizeWindowBy(this, xDiff, yDiff);
	else
		ResizeBy((int32)xDiff, (int32)yDiff, NULL);
}


// #pragma mark - UpdateSession


Window::UpdateSession::UpdateSession()
	:
	fDirtyRegion(),
	fInUse(false),
	fCause(0)
{
}


Window::UpdateSession::~UpdateSession()
{
}


void
Window::UpdateSession::Include(BRegion* additionalDirty)
{
	fDirtyRegion.Include(additionalDirty);
}


void
Window::UpdateSession::Exclude(BRegion* dirtyInNextSession)
{
	fDirtyRegion.Exclude(dirtyInNextSession);
}


bool
Window::HighlightTab(bool active, BRegion& dirty)
{
	if (fDecorator) {
		fDecorator->HighlightTab(active, &dirty);
		fBorderRegionValid = false;
		return true;
	}
	return false;
}


bool
Window::HighlightBorders(bool active, BRegion& dirty)
{
	if (fDecorator) {
		fDecorator->HighlightBorders(active, &dirty);
		fBorderRegionValid = false;
		return true;
	}
	return false;
}


bool
Window::IsTabHighlighted()
{
	if (fDecorator)
		return fDecorator->IsTabHighlighted();
	return false;
}


bool
Window::IsBordersHighlighted()
{
	if (fDecorator)
		return fDecorator->IsBordersHighlighted();
	return false;
}


void
Window::_EnsureWindowWithinScreenBounds(Window* window, Window* detached)
{
	BRect screenFrame = Screen()->Frame();
	float deltaX = 0;
	float deltaY = 0;
	BList* windowsToRight = window->Right2LeftSnappingWindowIds();
	BList* windowsToLeft = window->Left2RightSnappingWindowIds();
	BList* windowsToTop = window->Bottom2TopSnappingWindowIds();
	BList* windowsToBottom = window->Top2BottomSnappingWindowIds();

	if (window->Frame().left < 0
		&& (!windowsToRight
			|| windowsToRight->IsEmpty()
			|| windowsToRight->CountItems() == 1
				&& *static_cast<int32*>(windowsToRight->ItemAt(0))
					== detached->WindowId())) {
		//If window is off-screen to the left and there's no window tiled
		//to its right that can still "save" it from going totally off-screen
		deltaX = 6 - window->Frame().left;
	}
	else if (window->Frame().right > screenFrame.right
		&& (!windowsToLeft
			|| windowsToLeft->IsEmpty()
			|| windowsToLeft->CountItems() == 1
				&& *static_cast<int32*>(windowsToLeft->ItemAt(0))
					== detached->WindowId())) {
		//If window is off-screen to the right and there's no window tiled
		//to its left that can still "save" it from going totally off-screen
		deltaX = screenFrame.right - window->Frame().right - 6;
	}
	if (window->Frame().top < 0
		&& (!windowsToTop
			|| windowsToTop->IsEmpty()
			|| windowsToTop->CountItems() == 1
				&& *static_cast<int32*>(windowsToTop->ItemAt(0))
					== detached->WindowId())) {
		//If window is off-screen to the top and there's no window tiled
		//to its bottom that can still "save" it from going totally off-screen
		deltaY = 27 - window->Frame().top;
	}
	else if (window->Frame().bottom > screenFrame.bottom
		&& (!windowsToBottom
			|| windowsToBottom->IsEmpty()
			|| windowsToBottom->CountItems() == 1
				&& *static_cast<int32*>(windowsToBottom->ItemAt(0))
					== detached->WindowId())) {
		//If window is off-screen to the bottom and there's no window tiled
		//to its top that can still "save" it from going totally off-screen
		deltaY = screenFrame.bottom - window->Frame().bottom - 6;
	}
	if (deltaX != 0 || deltaY != 0)
		fDesktop->MoveWindowBy(window, deltaX, deltaY);
}


void
Window::InitStackedWindowIds()
{
	if (!fStackedWindowIds)
		fStackedWindowIds = new BList();
}


/*!	\brief Initializes all the Stack & Tile specific data for this window.
		Unless this initialization is done, the methods for stacking and
		snapping will not work. If the window is already initialized, the
		method returns immediately.
*/
void
Window::_InitStackingAndSnapping()
{
	STRACE_SAT(("Window::_InitStackingAndSnapping() on %s, fLeftVar=%d\n",
		Title(), fLeftVar));

	// if there are variables for the sides, then it is already initialized
	if (fLeftVar)
		return;

	fLeftVar = fDesktop->fStackAndTileSpec->AddVariable();
	fTopVar = fDesktop->fStackAndTileSpec->AddVariable();
	fRightVar = fDesktop->fStackAndTileSpec->AddVariable();
	fBottomVar = fDesktop->fStackAndTileSpec->AddVariable();
	BString label(Title());
	label << ".fLeftVar";
	fLeftVar->SetLabel(label.String());
	label = Title();
	label << ".fTopVar";
	fTopVar->SetLabel(label.String());
	label = Title();
	label << ".fRightVar";
	fRightVar->SetLabel(label.String());
	label = Title();
	label << ".fBottomVar";
	fBottomVar->SetLabel(label.String());

	fLeftVar->SetRange(-DBL_MAX, DBL_MAX);
	fTopVar->SetRange(-DBL_MAX, DBL_MAX);
	fRightVar->SetRange(-DBL_MAX, DBL_MAX);
	fBottomVar->SetRange(-DBL_MAX, DBL_MAX);

	fLeftConstraint  = fDesktop->fStackAndTileSpec
		->AddConstraint(1.0, fLeftVar,
			OperatorType(EQ), fFrame.left, 1, 1);
	fTopConstraint  = fDesktop->fStackAndTileSpec
		->AddConstraint(1.0, fTopVar,
			OperatorType(EQ), fFrame.top, 1, 1);
	label = Title();
	label << ".fLeftConstraint";
	fLeftConstraint->SetLabel(label.String());
	label = Title();
	label << ".fTopConstraint";
	fTopConstraint->SetLabel(label.String());

	int32 minWidth, maxWidth, minHeight, maxHeight;
	GetSizeLimits(&minWidth, &maxWidth, &minHeight, &maxHeight);
	fMinWidthConstraint = fDesktop->fStackAndTileSpec
		->AddConstraint(1.0, fLeftVar, -1.0, fRightVar,
			OperatorType(LE), -minWidth);
	fMinHeightConstraint = fDesktop->fStackAndTileSpec
		->AddConstraint(1.0, fTopVar, -1.0, fBottomVar,
			OperatorType(LE), -minHeight);
	label = Title();
	label << ".fMinWidthConstraint";
	fMinWidthConstraint->SetLabel(label.String());
	label = Title();
	label << ".fMinHeightConstraint";
	fMinHeightConstraint->SetLabel(label.String());

	// The width and height constraints have higher penalties than the
	// position constraints (left, top), so a window will keep its size
	// unless explicitly resized.
	fWidthConstraint = fDesktop->fStackAndTileSpec
		->AddConstraint(-1.0, fLeftVar, 1.0, fRightVar,
			OperatorType(EQ), fFrame.Width(), 10, 10);
	fHeightConstraint = fDesktop->fStackAndTileSpec
		->AddConstraint(-1.0, fTopVar, 1.0, fBottomVar,
			OperatorType(EQ), fFrame.Height(), 10, 10);
	label = Title();
	label << ".fWidthConstraint";
	fWidthConstraint->SetLabel(label.String());
	label = Title();
	label << ".fHeightConstraint";
	fHeightConstraint->SetLabel(label.String());

	fSnappingConstraints = new BList();

	STRACE_SAT(("Finished Window::_InitStackingAndSnapping() on %s\n",
		Title()));
}


/*!	\brief Removes all the Stack & Tile specific data from this window, so that
		the window is not stacked or snapped anymore. This method is called
		before the Stack & Tile configuration mode, so that stacking and
		snapping of a window can be set up from scratch.
*/
void
Window::_RemoveStackingAndSnapping()
{
	STRACE_SAT(("Window::_RemoveStackingAndSnapping() on %s, fLeftVar=%d\n",
		Title(), fLeftVar));

	// if there are no variables for the sides, then it is not active
	if (!fLeftVar)
		return;

	delete fLeftConstraint;
	delete fTopConstraint;
	delete fMinWidthConstraint;
	delete fMinHeightConstraint;
	delete fWidthConstraint;
	delete fHeightConstraint;
	fLeftConstraint = NULL;
	fTopConstraint = NULL;
	fMinWidthConstraint = NULL;
	fMinHeightConstraint = NULL;
	fWidthConstraint = NULL;
	fHeightConstraint = NULL;

	// remove stacking specific data
	fPrevStackedWindows = fStackedWindows;
	if (fStackedWindows) {
		// remove constraints
		delete fLeftStackingConstraint;
		delete fTopStackingConstraint;
		delete fRightStackingConstraint;
		delete fBottomStackingConstraint;
		fLeftStackingConstraint = NULL;
		fTopStackingConstraint  = NULL;
		fRightStackingConstraint = NULL;
		fBottomStackingConstraint = NULL;

		// if this is the first window in the stack, then the stacking
		// constraints have to be changed to refer to the new first window
		if (this == fStackedWindows->FirstItem()) {
			Window* newFirstWindow = static_cast<Window*>(
				fStackedWindows->ItemAt(1));

			// the first window does not need stacking constraints
			delete newFirstWindow->fLeftStackingConstraint;
			delete newFirstWindow->fTopStackingConstraint;
			delete newFirstWindow->fRightStackingConstraint;
			delete newFirstWindow->fBottomStackingConstraint;
			newFirstWindow->fLeftStackingConstraint = NULL;
			newFirstWindow->fTopStackingConstraint = NULL;
			newFirstWindow->fRightStackingConstraint = NULL;
			newFirstWindow->fBottomStackingConstraint = NULL;

			// the following windows need stacking constraints referring to
			// the first window
			for (int i = 2; i < fStackedWindows->CountItems(); i++) {
				Window* window = static_cast<Window*>(
					fStackedWindows->ItemAt(i));
				window->fLeftStackingConstraint->SetLeftSide(
					1, window->fLeftVar, -1, newFirstWindow->fLeftVar);
				window->fTopStackingConstraint->SetLeftSide(
					1, window->fTopVar, -1, newFirstWindow->fTopVar);
				window->fRightStackingConstraint->SetLeftSide(
					1, window->fRightVar, -1, newFirstWindow->fRightVar);
				window->fBottomStackingConstraint->SetLeftSide(
					1, window->fBottomVar, -1, newFirstWindow->fBottomVar);
			}
		}

		// remove this window from the stack
		fStackedWindows->RemoveItem(this);
		fStackedWindows = NULL;
	}

	// remove snapping specific data
	if (fSnappingConstraints) {
		for (int i = 0; i < fSnappingConstraints->CountItems(); i++)
			delete static_cast<Constraint*>(fSnappingConstraints->ItemAt(i));
		fSnappingConstraints = NULL;
	}

	// deleting variables also invalidates their constraints
	// therefore we can delete them afterwards without harm
	delete fLeftVar;
	delete fTopVar;
	delete fRightVar;
	delete fBottomVar;
	fLeftVar = NULL;
	fTopVar = NULL;
	fRightVar = NULL;
	fBottomVar = NULL;

	//For each previously snapped window, make sure that it is within
	//screen bounds, so as to prevent it from being lost off-screen
	for (int i = 0; fLeft2LeftSnappingWindowIds
		&& i < fLeft2LeftSnappingWindowIds->CountItems(); i++) {
		int32* id = static_cast<int32*>(fLeft2LeftSnappingWindowIds->ItemAt(i));
		Window* win = fDesktop->FindWindow(*id);
		if (win) {
			_EnsureWindowWithinScreenBounds(win, this);
			win->StackAndTile();
		}
	}
	for (int i = 0; fLeft2RightSnappingWindowIds
		&& i < fLeft2RightSnappingWindowIds->CountItems(); i++) {
		int32* id = static_cast<int32*>(fLeft2RightSnappingWindowIds->ItemAt(i));
		Window* win = fDesktop->FindWindow(*id);
		if (win) {
			_EnsureWindowWithinScreenBounds(win, this);
			win->StackAndTile();
		}
	}
	for (int i = 0; fRight2LeftSnappingWindowIds
		&& i < fRight2LeftSnappingWindowIds->CountItems(); i++) {
		int32* id = static_cast<int32*>(fRight2LeftSnappingWindowIds->ItemAt(i));
		Window* win = fDesktop->FindWindow(*id);
		if (win) {
			_EnsureWindowWithinScreenBounds(win, this);
			win->StackAndTile();
		}
	}
	for (int i = 0; fRight2RightSnappingWindowIds
		&& i < fRight2RightSnappingWindowIds->CountItems(); i++) {
		int32* id = static_cast<int32*>(fRight2RightSnappingWindowIds->ItemAt(i));
		Window* win = fDesktop->FindWindow(*id);
		if (win) {
			_EnsureWindowWithinScreenBounds(win, this);
			win->StackAndTile();
		}
	}
	for (int i = 0; fTop2TopSnappingWindowIds
		&& i < fTop2TopSnappingWindowIds->CountItems(); i++) {
		int32* id = static_cast<int32*>(fTop2TopSnappingWindowIds->ItemAt(i));
		Window* win = fDesktop->FindWindow(*id);
		if (win) {
			_EnsureWindowWithinScreenBounds(win, this);
			win->StackAndTile();
		}
	}
	for (int i = 0; fTop2BottomSnappingWindowIds
		&& i < fTop2BottomSnappingWindowIds->CountItems(); i++) {
		int32* id = static_cast<int32*>(fTop2BottomSnappingWindowIds->ItemAt(i));
		Window* win = fDesktop->FindWindow(*id);
		if (win) {
			_EnsureWindowWithinScreenBounds(win, this);
			win->StackAndTile();
		}
	}
	for (int i = 0; fBottom2TopSnappingWindowIds
		&& i < fBottom2TopSnappingWindowIds->CountItems(); i++) {
		int32* id = static_cast<int32*>(fBottom2TopSnappingWindowIds->ItemAt(i));
		Window* win = fDesktop->FindWindow(*id);
		if (win) {
			_EnsureWindowWithinScreenBounds(win, this);
			win->StackAndTile();
		}
	}
	for (int i = 0; fBottom2BottomSnappingWindowIds
		&& i < fBottom2BottomSnappingWindowIds->CountItems(); i++) {
		int32* id = static_cast<int32*>(fBottom2BottomSnappingWindowIds->ItemAt(i));
		Window* win = fDesktop->FindWindow(*id);
		if (win) {
			_EnsureWindowWithinScreenBounds(win, this);
			win->StackAndTile();
		}
	}

	STRACE_SAT(("Finished Window::_RemoveStackingAndSnapping() on %s\n",
		Title()));
}


/*!	\brief Looks for a suitable stacking candidate by checking which window
		tabs overlap with this window's tab, and gives visual feedback by
		adjusting window tab highlights. This method is called during the
		Stack & Tile configuration mode.
*/
void
Window::_CheckIfReadyToStack()
{
	// stacking only works if this window has a tab
	if (!fDecorator)
		return;

	Window* prevWindowUnder = fWindowUnder;
	fWindowUnder = NULL;

	// search for stacking candidate
	BRect thisTabRect(
		fDecorator->TabRect().left, fDecorator->TabRect().top,
		fDecorator->TabRect().right + 1, fDecorator->TabRect().bottom);
	for (Window* window = fDesktop->GetWindows().LastWindow();
		window;
		window = window->PreviousWindow(fCurrentWorkspace)) {
		if (window != this
			&& !window->IsHidden()
			&& window->Decorator()
			&& thisTabRect.Intersects(window->Decorator()->TabRect())) {

			// remember window as the candidate for stacking
			fWindowUnder = window;
			break;
		}
	}

	// if the stacking candidate has changed, change tab highlights
	if(prevWindowUnder != fWindowUnder) {
		if(!prevWindowUnder) { // candidate found for the first time
			fDesktop->HighlightTab(this, true);
			fDesktop->HighlightTab(fWindowUnder, true);
		} else if(!fWindowUnder) { // no candidate after there was one
			fDesktop->HighlightTab(this, false);
			fDesktop->HighlightTab(prevWindowUnder, false);
		} else { // changing from one to another candidate
			fDesktop->HighlightTab(prevWindowUnder, false);
			fDesktop->HighlightTab(fWindowUnder, true);
		}
	}
}


/*!	\brief Stacks this window to the current candidate window for stacking
		and adjusts the window tabs of the stack. Afterwards the stacking
		candidate is cleared.
*/
void
Window::_StackWindow()
{
	STRACE_SAT(("Window::_StackWindow() on %s, fWindowUnder=%x\n",
		Title(), fWindowUnder));

	// stack only if there is a candidate window for stacking
	if (fWindowUnder == NULL)
		return;

	// align this window with the stack
	fDesktop->MoveWindowBy(this,
		fWindowUnder->Frame().left - Frame().left,
		fWindowUnder->Frame().top - Frame().top);
	fDesktop->ResizeWindowBy(this,
		fWindowUnder->Frame().right - Frame().right,
		fWindowUnder->Frame().bottom - Frame().bottom);

	// make sure there is a stacked window list
	BList* stack = fWindowUnder->fStackedWindows;
	if (!stack) {
		stack = new BList();
		stack->AddItem(fWindowUnder);
		fWindowUnder->fStackedWindows = stack;
	}

	// make sure there is a stacked window id list
	BList* stackIds = fWindowUnder->fStackedWindowIds;
	if (!stackIds) {
		stackIds = new BList();
		int32* id = static_cast<int32*>(malloc(sizeof(int32)));
		*id = fWindowUnder->WindowId();
		stackIds->AddItem(id);
		fWindowUnder->fStackedWindowIds = stackIds;
	}

	// add this window's id to the stacked window id list - ONLY
	// if not already contained
	bool idExists = false;
	for (int i = 0; !idExists && i < stackIds->CountItems(); i++) {
		int32* stackedId = static_cast<int32*>(stackIds->ItemAt(i));
		idExists = *stackedId == WindowId();
	}
	if (!idExists) {
		int32* id = static_cast<int32*>(malloc(sizeof(int32)));
		*id = WindowId();
		stackIds->AddItem(id);
	}
	fStackedWindowIds = stackIds;

	// add this window to the stack before the windowUnder,
	// rearrange the window tabs and initialize constraints
	stack->AddItem(this, stack->IndexOf(fWindowUnder));
	fStackedWindows = stack;
	_ArrangeStackedWindowTabs();
	_InitStackingAndSnapping();

	// set up stacking constraints
	Window* firstWindow = static_cast<Window*>(stack->FirstItem());
	if (this != firstWindow) {
		// if this is not the first window, then add stacking constraints
		// referring to the first window in the stack
		firstWindow->_InitStackingAndSnapping();
		fLeftStackingConstraint = fLeftVar->IsEqual(firstWindow->fLeftVar);
		fTopStackingConstraint = fTopVar->IsEqual(firstWindow->fTopVar);
		fRightStackingConstraint = fRightVar->IsEqual(firstWindow->fRightVar);
		fBottomStackingConstraint = fBottomVar->IsEqual(firstWindow->fBottomVar);
		BString label("stacking of ");
		label << Title();
		fLeftStackingConstraint->SetLabel(label.String());
		fTopStackingConstraint->SetLabel(label.String());
		fRightStackingConstraint->SetLabel(label.String());
		fBottomStackingConstraint->SetLabel(label.String());
	} else {
		// if this is the first window in the stack, then the stacking
		// constraints of the following windows have to be changed to refer
		// to this window
		for (int i = 1; i < fStackedWindows->CountItems(); i++) {
			Window* window = static_cast<Window*>(
				fStackedWindows->ItemAt(i));
			window->_InitStackingAndSnapping();

			// either create new stacking constraints
			// or just change the existing ones
			if (!window->fLeftStackingConstraint) {
				window->fLeftStackingConstraint =
					window->fLeftVar->IsEqual(fLeftVar);
				window->fTopStackingConstraint =
					window->fTopVar->IsEqual(fTopVar);
				window->fRightStackingConstraint =
					window->fRightVar->IsEqual(fRightVar);
				window->fBottomStackingConstraint =
					window->fBottomVar->IsEqual(fBottomVar);
				BString label("stacking of ");
				label << Title();
				window->fLeftStackingConstraint->SetLabel(label.String());
				window->fTopStackingConstraint->SetLabel(label.String());
				window->fRightStackingConstraint->SetLabel(label.String());
				window->fBottomStackingConstraint->SetLabel(label.String());
			} else {
				window->fLeftStackingConstraint->SetLeftSide(
					1, window->fLeftVar, -1, fLeftVar);
				window->fTopStackingConstraint->SetLeftSide(
					1, window->fTopVar, -1, fTopVar);
				window->fRightStackingConstraint->SetLeftSide(
					1, window->fRightVar, -1, fRightVar);
				window->fBottomStackingConstraint->SetLeftSide(
					1, window->fBottomVar, -1, fBottomVar);
			}
		}
	}

	// clear stacking candidate
	fWindowUnder = NULL;

	STRACE_SAT(("Finished Window::_StackWindow() on %s\n", Title()));
}


/*!	\brief Stacks this window onto the stack of the given window so that the
		tab of this window is before the tab of the given window.

	\param window the window with the stack where this window will be added.
*/
void
Window::StackWindowBefore(Window* window)
{
	fWindowUnder = window;
	_StackWindow();
}


/*!	\brief Arranges the tab locations of the windows that are in the stack of
		this window.
*/
void
Window::_ArrangeStackedWindowTabs()
{
	STRACE_SAT(("Window::_ArrangeStackedWindowTabs() on %s, fStackedWindows=%x\n",
		Title(), fStackedWindows));
	if (!fStackedWindows)
		return;

	// adjust tab locations of stacked windows
	float location = 0;
	for (int i = 0; i < fStackedWindows->CountItems(); i++) {
		Window* window = static_cast<Window*>(fStackedWindows->ItemAt(i));
		fDesktop->SetWindowTabLocation(window, location);
		STRACE_SAT(("\t%s.tabLocation = %f\n", window->Title(), location));
		location = window->TabLocation()
			+ window->Decorator()->TabRect().Width() + 1;
	}
	STRACE_SAT(("Finished Window::_ArrangeStackedWindowTabs() on %s\n",
		Title()));
}


/*!	\brief Compiles suitable snapping candidates by checking which windows
		are adjacent to this window, and gives visual feedback by adjusting
		window border highlights. This method is called during the Stack & Tile
		configuration mode.
*/
void
Window::_CheckIfReadyToSnap()
{
	if (!fLeftAdjacentWindows) {
		fLeftAdjacentWindows = new BList();
		fTopAdjacentWindows = new BList();
		fRightAdjacentWindows = new BList();
		fBottomAdjacentWindows = new BList();
	}

	// remember previous adjacent windows for changing highlights later
	BList prevLeftAdjacentWindows(*fLeftAdjacentWindows);
	BList prevTopAdjacentWindows(*fTopAdjacentWindows);
	BList prevRightAdjacentWindows(*fRightAdjacentWindows);
	BList prevBottomAdjacentWindows(*fBottomAdjacentWindows);

	// clear lists of old adjacent windows
	fLeftAdjacentWindows->MakeEmpty();
	fTopAdjacentWindows->MakeEmpty();
	fRightAdjacentWindows->MakeEmpty();
	fBottomAdjacentWindows->MakeEmpty();

	// go through all windows and look for snapping candidates
	for (Window* window2 = fDesktop->GetWindows().LastWindow();
		window2;
		window2 = window2->PreviousWindow(fCurrentWorkspace)) {
		if (!window2->IsHidden()
			&& this != window2	// don't snap window to itself
			&& window2 != fWindowUnder // don't snap with stacking candidate
			&& !(strcmp(window2->Title(), "Deskbar")==0)
			&& !(strcmp(window2->Title(), "Desktop")==0)) {

			// is window2 adjacent on the left of this window?
			BRect thisLeft(
				this->Frame().left - 12, this->Frame().top + 12,
				this->Frame().left + 6, this->Frame().bottom - 12);
			BRect window2Right(
				window2->Frame().right - 6, window2->Frame().top + 12,
				window2->Frame().right + 12, window2->Frame().bottom - 12);
			if (thisLeft.Intersects(window2Right))
				fLeftAdjacentWindows->AddItem(window2);

			// is window2 adjacent on the top of this window?
			BRect thisTop(
				this->Frame().left + 12, this->Frame().top - 22,
				this->Frame().right - 12, this->Frame().top + 6);
			BRect window2Bottom(
				window2->Frame().left + 12, window2->Frame().bottom - 6,
				window2->Frame().right - 12, window2->Frame().bottom + 12);
			if (thisTop.Intersects(window2Bottom))
				fTopAdjacentWindows->AddItem(window2);

			// is window2 adjacent on the right of this window?
			BRect thisRight(
				this->Frame().right - 6, this->Frame().top + 12,
				this->Frame().right + 12, this->Frame().bottom - 12);
			BRect window2Left(
				window2->Frame().left - 12, window2->Frame().top + 12,
				window2->Frame().left + 6, window2->Frame().bottom - 12);
			if (thisRight.Intersects(window2Left))
				fRightAdjacentWindows->AddItem(window2);

			// is window2 adjacent on the bottom of this window?
			BRect thisBottom(
				this->Frame().left + 12, this->Frame().bottom - 6,
				this->Frame().right - 12, this->Frame().bottom + 12);
			BRect window2Top(
				window2->Frame().left + 12, window2->Frame().top - 22,
				window2->Frame().right - 12, window2->Frame().top + 6);
			if (thisBottom.Intersects(window2Top))
				fBottomAdjacentWindows->AddItem(window2);
		}
	}

	// change window border highlight of this window
	if (!fLeftAdjacentWindows->IsEmpty()
		|| !fTopAdjacentWindows->IsEmpty()
		|| !fRightAdjacentWindows->IsEmpty()
		|| !fBottomAdjacentWindows->IsEmpty())
		fDesktop->HighlightBorders(this, true);
	else
		fDesktop->HighlightBorders(this, false);

	// switch off border highlights of windows that are not candidates any more
	for (int i = 0; i < prevLeftAdjacentWindows.CountItems(); i++) {
		Window* window = static_cast<Window*>(
			prevLeftAdjacentWindows.ItemAt(i));
		if (!fLeftAdjacentWindows->HasItem(window))
			fDesktop->HighlightBorders(window, false);
	}
	for (int i = 0; i < prevTopAdjacentWindows.CountItems(); i++) {
		Window* window = static_cast<Window*>(
			prevTopAdjacentWindows.ItemAt(i));
		if (!fTopAdjacentWindows->HasItem(window))
			fDesktop->HighlightBorders(window, false);
	}
	for (int i = 0; i < prevRightAdjacentWindows.CountItems(); i++) {
		Window* window = static_cast<Window*>(
			prevRightAdjacentWindows.ItemAt(i));
		if (!fRightAdjacentWindows->HasItem(window))
			fDesktop->HighlightBorders(window, false);
	}
	for (int i = 0; i < prevBottomAdjacentWindows.CountItems(); i++) {
		Window* window = static_cast<Window*>(
			prevBottomAdjacentWindows.ItemAt(i));
		if (!fBottomAdjacentWindows->HasItem(window))
			fDesktop->HighlightBorders(window, false);
	}

	// switch on border highlights of candidate windows
	// if they are already on, HighlightBorders will detect this
	for (int i = 0; i < fLeftAdjacentWindows->CountItems(); i++)
		fDesktop->HighlightBorders(
			static_cast<Window*>(fLeftAdjacentWindows->ItemAt(i)), true);
	for (int i = 0; i < fTopAdjacentWindows->CountItems(); i++)
		fDesktop->HighlightBorders(
			static_cast<Window*>(fTopAdjacentWindows->ItemAt(i)), true);
	for (int i = 0; i < fRightAdjacentWindows->CountItems(); i++)
		fDesktop->HighlightBorders(
			static_cast<Window*>(fRightAdjacentWindows->ItemAt(i)), true);
	for (int i = 0; i < fBottomAdjacentWindows->CountItems(); i++)
		fDesktop->HighlightBorders(
			static_cast<Window*>(fBottomAdjacentWindows->ItemAt(i)), true);
}


/*!	\brief Snaps this window to the current candidate windows for snapping
	on each side. Afterwards snapping candidates are cleared.
*/
void Window::_SnapWindow()
{
	BRect bounds;
	Window* leftmostWindow;
	Window* topmostWindow;
	Window* rightmostWindow;
	Window* bottommostWindow;

	// snap other windows to the left border
	if (!fLeftAdjacentWindows->IsEmpty()) {
		bounds = _BoundingRectAndWindows(fLeftAdjacentWindows,
			&leftmostWindow, &topmostWindow,
			&rightmostWindow, &bottommostWindow);

		// adjust window position & initialize snapping
		fDesktop->MoveWindowBy(this, bounds.right - Frame().left + 11, 0);
		_InitStackingAndSnapping();
		topmostWindow->_InitStackingAndSnapping();
		bottommostWindow->_InitStackingAndSnapping();

		// adjust top border
		if (fTopAdjacentWindows->IsEmpty()) {
			fDesktop->MoveWindowBy(this, 0, bounds.top - Frame().top);
			Constraint* topSnapping = fTopVar->IsEqual(topmostWindow->fTopVar);
			BString label("topSnapping of ");
			label << Title();
			topSnapping->SetLabel(label.String());
			fSnappingConstraints->AddItem(topSnapping);

			AddToSnappingList(topmostWindow, SNAP_TOP, SNAP_TOP);
			AddToSnappingList(topmostWindow, SNAP_LEFT, SNAP_RIGHT);
			topmostWindow->AddToSnappingList(this, SNAP_TOP, SNAP_TOP);
			topmostWindow->AddToSnappingList(this, SNAP_RIGHT, SNAP_LEFT);

			STRACE_SAT(("Adding %s[%d] to %s[%d]'s T2T & L2R list\n",
						topmostWindow->Title(), topmostWindow->WindowId(),
						Title(), WindowId()));
		}

		// adjust bottom border
		if (fBottomAdjacentWindows->IsEmpty()) {
			fDesktop->ResizeWindowBy(this, 0, bounds.bottom - Frame().bottom);
			Constraint* bottomSnapping =
				fBottomVar->IsEqual(bottommostWindow->fBottomVar);
			BString label("bottomSnapping of ");
			label << Title();
			bottomSnapping->SetLabel(label.String());
			fSnappingConstraints->AddItem(bottomSnapping);

			AddToSnappingList(bottommostWindow, SNAP_BOTTOM, SNAP_BOTTOM);
			AddToSnappingList(bottommostWindow, SNAP_LEFT, SNAP_RIGHT);
			bottommostWindow->AddToSnappingList(this, SNAP_BOTTOM, SNAP_BOTTOM);
			bottommostWindow->AddToSnappingList(this, SNAP_RIGHT, SNAP_LEFT);

			STRACE_SAT(("Adding %s[%d] to %s[%d]'s B2B & L2R list\n",
						bottommostWindow->Title(), bottommostWindow->WindowId(),
						Title(), WindowId()));
		}

		// adjust other windows
		for (int i = 0; i < fLeftAdjacentWindows->CountItems(); i++) {
			Window* window = static_cast<Window*>(
				fLeftAdjacentWindows->ItemAt(i));
			window->_InitStackingAndSnapping();
			Constraint* leftSnapping =
				fDesktop->fStackAndTileSpec->AddConstraint(
					-1, window->fRightVar, 1, fLeftVar,
					OperatorType(EQ), 11);
			BString label("leftSnapping of ");
			label << Title();
			leftSnapping->SetLabel(label.String());
			window->fSnappingConstraints->AddItem(leftSnapping);

			AddToSnappingList(window, SNAP_LEFT, SNAP_RIGHT);
			window->AddToSnappingList(this, SNAP_RIGHT, SNAP_LEFT);

			STRACE_SAT(("Adding %s[%d] to %s[%d]'s L2R list\n",
						window->Title(), window->WindowId(), Title(),
						WindowId()));
		}
	}

	// snap other windows to the top border
	if (!fTopAdjacentWindows->IsEmpty()) {
		bounds = _BoundingRectAndWindows(fTopAdjacentWindows,
			&leftmostWindow, &topmostWindow,
			&rightmostWindow, &bottommostWindow);

		// adjust window position & initialize snapping
		fDesktop->MoveWindowBy(this, 0, bounds.bottom - Frame().top + 32);
		_InitStackingAndSnapping();
		leftmostWindow->_InitStackingAndSnapping();
		rightmostWindow->_InitStackingAndSnapping();

		// adjust left border
		if (fLeftAdjacentWindows->IsEmpty()) {
			fDesktop->MoveWindowBy(this, bounds.left - Frame().left, 0);
			Constraint* leftSnapping =
				fLeftVar->IsEqual(leftmostWindow->fLeftVar);
			BString label("leftSnapping of ");
			label << Title();
			leftSnapping->SetLabel(label.String());
			fSnappingConstraints->AddItem(leftSnapping);

			AddToSnappingList(leftmostWindow, SNAP_LEFT, SNAP_LEFT);
			AddToSnappingList(leftmostWindow, SNAP_TOP, SNAP_BOTTOM);
			leftmostWindow->AddToSnappingList(this, SNAP_LEFT, SNAP_LEFT);
			leftmostWindow->AddToSnappingList(this, SNAP_BOTTOM, SNAP_TOP);

			STRACE_SAT(("Adding %s[%d] to %s[%d]'s L2L & T2B list\n",
						leftmostWindow->Title(), leftmostWindow->WindowId(),
						Title(), WindowId()));
		}

		// adjust right border
		if (fRightAdjacentWindows->IsEmpty()) {
			fDesktop->ResizeWindowBy(this, bounds.right - Frame().right, 0);
			Constraint* rightSnapping =
				fRightVar->IsEqual(rightmostWindow->fRightVar);
			BString label("rightSnapping of ");
			label << Title();
			rightSnapping->SetLabel(label.String());
			fSnappingConstraints->AddItem(rightSnapping);

			AddToSnappingList(rightmostWindow, SNAP_RIGHT, SNAP_RIGHT);
			AddToSnappingList(rightmostWindow, SNAP_TOP, SNAP_BOTTOM);
			rightmostWindow->AddToSnappingList(this, SNAP_RIGHT, SNAP_RIGHT);
			rightmostWindow->AddToSnappingList(this, SNAP_BOTTOM, SNAP_TOP);

			STRACE_SAT(("Adding %s[%d] to %s[%d]'s R2R & T2B list\n",
						rightmostWindow->Title(), rightmostWindow->WindowId(),
						Title(), WindowId()));
		}

		// adjust other windows
		for (int i = 0; i < fTopAdjacentWindows->CountItems(); i++) {
			Window* window = static_cast<Window*>(
				fTopAdjacentWindows->ItemAt(i));
			window->_InitStackingAndSnapping();
			Constraint* topSnapping =
				fDesktop->fStackAndTileSpec->AddConstraint(
					-1, window->fBottomVar, 1, fTopVar,
					OperatorType(EQ), 32);
			BString label("topSnapping of ");
			label << Title();
			topSnapping->SetLabel(label.String());
			window->fSnappingConstraints->AddItem(topSnapping);

			AddToSnappingList(window, SNAP_TOP, SNAP_BOTTOM);
			window->AddToSnappingList(this, SNAP_BOTTOM, SNAP_TOP);

			STRACE_SAT(("Adding %s[%d] to %s[%d]'s T2B list\n",
						window->Title(), window->WindowId(), Title(),
						WindowId()));
		}
	}

	// snap other windows to the right border
	if (!fRightAdjacentWindows->IsEmpty()) {
		bounds = _BoundingRectAndWindows(fRightAdjacentWindows,
			&leftmostWindow, &topmostWindow,
			&rightmostWindow, &bottommostWindow);

		// adjust window position & initialize snapping
		fDesktop->MoveWindowBy(this, bounds.left - Frame().right - 11, 0);
		_InitStackingAndSnapping();
		topmostWindow->_InitStackingAndSnapping();
		bottommostWindow->_InitStackingAndSnapping();

		// adjust top border
		if (fTopAdjacentWindows->IsEmpty()) {
			fDesktop->MoveWindowBy(this, 0, bounds.top - Frame().top);
			Constraint* topSnapping =
				fTopVar->IsEqual(topmostWindow->fTopVar);
			BString label("topSnapping of ");
			label << Title();
			topSnapping->SetLabel(label.String());
			fSnappingConstraints->AddItem(topSnapping);

			AddToSnappingList(topmostWindow, SNAP_TOP, SNAP_TOP);
			AddToSnappingList(topmostWindow, SNAP_RIGHT, SNAP_LEFT);
			topmostWindow->AddToSnappingList(this, SNAP_TOP, SNAP_TOP);
			topmostWindow->AddToSnappingList(this, SNAP_LEFT, SNAP_RIGHT);

			STRACE_SAT(("Adding %s[%d] to %s[%d]'s T2T & R2L list\n",
						topmostWindow->Title(), topmostWindow->WindowId(),
						Title(), WindowId()));
		}

		// adjust bottom border
		if (fBottomAdjacentWindows->IsEmpty()) {
			fDesktop->ResizeWindowBy(this, 0, bounds.bottom - Frame().bottom);
			Constraint* bottomSnapping =
				fBottomVar->IsEqual(bottommostWindow->fBottomVar);
			BString label("bottomSnapping of ");
			label << Title();
			bottomSnapping->SetLabel(label.String());
			fSnappingConstraints->AddItem(bottomSnapping);

			AddToSnappingList(bottommostWindow, SNAP_BOTTOM, SNAP_BOTTOM);
			AddToSnappingList(bottommostWindow, SNAP_RIGHT, SNAP_LEFT);
			bottommostWindow->AddToSnappingList(this, SNAP_BOTTOM, SNAP_BOTTOM);
			bottommostWindow->AddToSnappingList(this, SNAP_LEFT, SNAP_RIGHT);

			STRACE_SAT(("Adding %s[%d] to %s[%d]'s B2B & R2L list\n",
						bottommostWindow->Title(), bottommostWindow->WindowId(),
						Title(), WindowId()));
		}

		// adjust other windows
		for (int i = 0; i < fRightAdjacentWindows->CountItems(); i++) {
			Window* window = static_cast<Window*>(
				fRightAdjacentWindows->ItemAt(i));
			window->_InitStackingAndSnapping();
			Constraint* rightSnapping =
				fDesktop->fStackAndTileSpec->AddConstraint(
					-1, fRightVar, 1, window->fLeftVar,
					OperatorType(EQ), 11);
			BString label("rightSnapping of ");
			label << Title();
			rightSnapping->SetLabel(label.String());
			window->fSnappingConstraints->AddItem(rightSnapping);

			AddToSnappingList(window, SNAP_RIGHT, SNAP_LEFT);
			window->AddToSnappingList(this, SNAP_LEFT, SNAP_RIGHT);

			STRACE_SAT(("Adding %s[%d] to %s[%d]'s R2L list\n",
						window->Title(), window->WindowId(), Title(),
						WindowId()));
		}
	}

	// snap other windows to the bottom border
	if (!fBottomAdjacentWindows->IsEmpty()) {
		bounds = _BoundingRectAndWindows(fBottomAdjacentWindows,
			&leftmostWindow, &topmostWindow,
			&rightmostWindow, &bottommostWindow);

		// adjust window position & initialize snapping
		fDesktop->MoveWindowBy(this, 0, bounds.top - Frame().bottom - 32);
		_InitStackingAndSnapping();
		leftmostWindow->_InitStackingAndSnapping();
		rightmostWindow->_InitStackingAndSnapping();

		// adjust left border
		if (fLeftAdjacentWindows->IsEmpty()) {
			fDesktop->MoveWindowBy(this, bounds.left - Frame().left, 0);
			Constraint* leftSnapping =
				fLeftVar->IsEqual(leftmostWindow->fLeftVar);
			BString label("leftSnapping of ");
			label << Title();
			leftSnapping->SetLabel(label.String());
			fSnappingConstraints->AddItem(leftSnapping);

			AddToSnappingList(leftmostWindow, SNAP_LEFT, SNAP_LEFT);
			AddToSnappingList(leftmostWindow, SNAP_BOTTOM, SNAP_TOP);
			leftmostWindow->AddToSnappingList(this, SNAP_LEFT, SNAP_LEFT);
			leftmostWindow->AddToSnappingList(this, SNAP_TOP, SNAP_BOTTOM);

			STRACE_SAT(("Adding %s[%d] to %s[%d]'s L2L & B2T list\n",
						leftmostWindow->Title(), leftmostWindow->WindowId(),
						Title(), WindowId()));
		}

		// adjust right border
		if (fRightAdjacentWindows->IsEmpty()) {
			fDesktop->ResizeWindowBy(this, bounds.right - Frame().right, 0);
			Constraint* rightSnapping =
				fRightVar->IsEqual(rightmostWindow->fRightVar);
			BString label("rightSnapping of ");
			label << Title();
			rightSnapping->SetLabel(label.String());
			fSnappingConstraints->AddItem(rightSnapping);

			AddToSnappingList(rightmostWindow, SNAP_RIGHT, SNAP_RIGHT);
			AddToSnappingList(rightmostWindow, SNAP_BOTTOM, SNAP_TOP);
			rightmostWindow->AddToSnappingList(this, SNAP_RIGHT, SNAP_RIGHT);
			rightmostWindow->AddToSnappingList(this, SNAP_TOP, SNAP_BOTTOM);

			STRACE_SAT(("Adding %s[%d] to %s[%d]'s R2R & B2T list\n",
						rightmostWindow->Title(), rightmostWindow->WindowId(),
						Title(), WindowId()));
		}

		// adjust other windows
		for (int i = 0; i < fBottomAdjacentWindows->CountItems(); i++) {
			Window* window = static_cast<Window*>(
				fBottomAdjacentWindows->ItemAt(i));
			window->_InitStackingAndSnapping();
			Constraint* bottomSnapping =
				fDesktop->fStackAndTileSpec->AddConstraint(
					-1, fBottomVar, 1, window->fTopVar,
					OperatorType(EQ), 32);
			BString label("bottomSnapping of ");
			label << Title();
			bottomSnapping->SetLabel(label.String());
			window->fSnappingConstraints->AddItem(bottomSnapping);

			AddToSnappingList(window, SNAP_BOTTOM, SNAP_TOP);
			window->AddToSnappingList(this, SNAP_TOP, SNAP_BOTTOM);

			STRACE_SAT(("Adding %s[%d] to %s[%d]'s B2T list\n",
						window->Title(), window->WindowId(), Title(),
						WindowId()));
		}
	}

	// clear snapping candidates
	delete fLeftAdjacentWindows;
	delete fTopAdjacentWindows;
	delete fRightAdjacentWindows;
	delete fBottomAdjacentWindows;
	fLeftAdjacentWindows = NULL;
	fTopAdjacentWindows = NULL;
	fRightAdjacentWindows = NULL;
	fBottomAdjacentWindows = NULL;
}


void
Window::AddToSnappingList(Window* window,
		SnapOrientation thisSnapOrientation,
		SnapOrientation otherSnapOrientation) {
	AddToSnappingList(window->WindowId(), thisSnapOrientation,
			otherSnapOrientation);
}


/*!	\brief Adds the given window's id to current window's snapping list,
		according to given combo of snapping orientations. For example, if
		thisSnapOrientation is SNAP_LEFT and otherSnapOrientation is
		SNAP_RIGHT, then add the window's id to the left2right snapping list.
		Discards duplicate id's

	\param thisSnapOrientation snapping orientation for current window
	\param otherSnapOrientation snapping orientation for given window
 */
void
Window::AddToSnappingList(int32 windowId,
		SnapOrientation thisSnapOrientation,
		SnapOrientation otherSnapOrientation) {

	BList* windowIdList = GetSnappingList(thisSnapOrientation,
		otherSnapOrientation, true);
	//This may be the case if invalid snap orientation combo is given
	//(e.g. top to left)
	if (!windowIdList)
		return;

	//don't add id if already contained
	for (int i = 0; i < windowIdList->CountItems(); i++) {
		int32* id = static_cast<int32*>(windowIdList->ItemAt(i));
		if (*id == windowId) {
			return;
		}
	}
	int32* id = static_cast<int32*>(malloc(sizeof(int32)));
	*id = windowId;
	windowIdList->AddItem(id);

	char snapStr[][7] = {"left", "right", "top", "bottom"};
	STRACE_SAT(("\tAdded %s to %s's %s 2 %s list\n",
				windowId, Title(),
				snapStr[thisSnapOrientation],
				snapStr[otherSnapOrientation]));
}


/*!	\brief Removes given window from the snapping list according to the
		combination of given snapping orientations (e.g. left to left,
		top to bottom, etc.)

	\param window the window to remove
	\param thisSnapOrientation snapping orientation for this window
	\param otherSnapOrientation snapping orientation for other window
 */
void
Window::RemoveFromSnappingList(int32 windowId,
		SnapOrientation thisSnapOrientation,
		SnapOrientation otherSnapOrientation) {
	BList* windowIdList = GetSnappingList(thisSnapOrientation,
		otherSnapOrientation, false);

	// This may be the case if invalid snap orientation combo is given
	//(e.g. top to left)
	if (!windowIdList)
		return;

	for (int i = 0; i < windowIdList->CountItems(); i++) {
		int32* id = static_cast<int32*>(windowIdList->ItemAt(i));
		if (*id == windowId) {
			windowIdList->RemoveItem(i);
			free(id);

			char snapStr[][7] = {"left", "right", "top", "bottom"};
			STRACE_SAT(("\tRemoved %x from %s's %s 2 %s list\n",
						windowId, Title(),
						snapStr[thisSnapOrientation],
						snapStr[otherSnapOrientation]));
			return;
		}
	}
}


/*!	\brief Returns a reference to the snapping list according to the
		combination of given snapping orientations (e.g. left to left,
		top to bottom, etc.). If the snapping list is not initialised,
		then creates it before returning its reference.

	\param thisSnapOrientation snapping orientation for this window
	\param otherSnapOrientation snapping orientation for other window
	\param createIfNull whether to create the list if null
 */
BList*
Window::GetSnappingList(SnapOrientation thisSnapOrientation,
		SnapOrientation otherSnapOrientation,
		bool createIfNull)
{
	BList** windowIdListRef = _GetSnappingListRef(thisSnapOrientation,
			otherSnapOrientation, createIfNull);

	if (windowIdListRef)
		return *windowIdListRef;
	return NULL;
}


BList**
Window::_GetSnappingListRef(SnapOrientation thisSnapOrientation,
		SnapOrientation otherSnapOrientation,
		bool createIfNull)
{
	BList** windowIdListRef = NULL;
	if (thisSnapOrientation == SNAP_LEFT
		&& otherSnapOrientation == SNAP_LEFT)
		windowIdListRef = &fLeft2LeftSnappingWindowIds;
	if (thisSnapOrientation == SNAP_LEFT
		&& otherSnapOrientation == SNAP_RIGHT)
		windowIdListRef = &fLeft2RightSnappingWindowIds;
	if (thisSnapOrientation == SNAP_RIGHT
		&& otherSnapOrientation == SNAP_LEFT)
		windowIdListRef = &fRight2LeftSnappingWindowIds;
	if (thisSnapOrientation == SNAP_RIGHT
		&& otherSnapOrientation == SNAP_RIGHT)
		windowIdListRef = &fRight2RightSnappingWindowIds;
	if (thisSnapOrientation == SNAP_TOP
		&& otherSnapOrientation == SNAP_TOP)
		windowIdListRef = &fTop2TopSnappingWindowIds;
	if (thisSnapOrientation == SNAP_TOP
		&& otherSnapOrientation == SNAP_BOTTOM)
		windowIdListRef = &fTop2BottomSnappingWindowIds;
	if (thisSnapOrientation == SNAP_BOTTOM
		&& otherSnapOrientation == SNAP_TOP)
		windowIdListRef = &fBottom2TopSnappingWindowIds;
	if (thisSnapOrientation == SNAP_BOTTOM
		&& otherSnapOrientation == SNAP_BOTTOM)
		windowIdListRef = &fBottom2BottomSnappingWindowIds;

	//This will be the case if invalid snap orientation combo is given
	//(e.g. top to left)
	if (!windowIdListRef) {
		return NULL;
	}

	//Create snapping list if not already initialised.
	if (!(*windowIdListRef) && createIfNull) {
		*windowIdListRef = new BList();
	}

	return windowIdListRef;
}


/*! \brief Snaps current window's side (left, right, top or bottom
		according to given orientation) to given window's side (again
		according to given orientation). Returns the object representing
		this snapping constraint.

		TODO some of the logic is duplicative w.r.t _SnapWindow - consider
		refactoring

	\param otherWindow the window to snap to
	\param thisSnap snapping orientation for current window
	\param otherSnap snapping orientation for given window
*/
Constraint*
Window::SnapToWindow(Window* otherWindow,
	SnapOrientation thisSnap,
	SnapOrientation otherSnap)
{
	Variable* thisSnappingVar;
	Variable* otherSnappingVar;
	BString snappingLabel;

	int snapDistance = 0;
	float moveDeltaX = 0;
	float moveDeltaY = 0;

	BRect thisFrame = Frame();
	BRect otherFrame = otherWindow->Frame();

	_InitStackingAndSnapping();
	otherWindow->_InitStackingAndSnapping();

	//For each possible pair of snapping orientations, work out the
	//necessary updates to make to the linear constraints
	switch(thisSnap) {
	case SNAP_LEFT:
		thisSnappingVar = fLeftVar;
		snappingLabel << "leftSnapping of ";
		switch(otherSnap) {
		case SNAP_LEFT:
			moveDeltaX = otherFrame.left - thisFrame.left;
			otherSnappingVar = otherWindow->fLeftVar;
			break;
		case SNAP_RIGHT:
			snapDistance = -11;
			moveDeltaX = otherFrame.right - thisFrame.left - snapDistance;
			otherSnappingVar = otherWindow->fRightVar;
			break;
		default:
			return NULL;
		}
		break;
	case SNAP_RIGHT:
		thisSnappingVar = fRightVar;
		snappingLabel << "rightSnapping of ";
		switch(otherSnap) {
		case SNAP_LEFT:
			snapDistance = 11;
			moveDeltaX = otherFrame.left - thisFrame.right - snapDistance;
			otherSnappingVar = otherWindow->fLeftVar;
			break;
		case SNAP_RIGHT:
			moveDeltaX = otherFrame.right - thisFrame.right;
			otherSnappingVar = otherWindow->fRightVar;
			break;
		default:
			return NULL;
		}
		break;
	case SNAP_TOP:
		thisSnappingVar = fTopVar;
		snappingLabel << "topSnapping of ";
		switch(otherSnap) {
		case SNAP_TOP:
			moveDeltaY = otherFrame.top - thisFrame.top;
			otherSnappingVar = otherWindow->fTopVar;
			break;
		case SNAP_BOTTOM:
			snapDistance = -32;
			moveDeltaY = otherFrame.bottom - thisFrame.top - snapDistance;
			otherSnappingVar = otherWindow->fBottomVar;
			break;
		default:
			return NULL;
		}
		break;
	default: //SNAP_BOTTOM
		thisSnappingVar = fBottomVar;
		snappingLabel << "bottomSnapping of ";
		switch(otherSnap) {
		case SNAP_TOP:
			snapDistance = 32;
			moveDeltaY = otherFrame.top - thisFrame.bottom - snapDistance;
			otherSnappingVar = otherWindow->fTopVar;
			break;
		case SNAP_BOTTOM:
			moveDeltaY = otherFrame.bottom - thisFrame.bottom;
			otherSnappingVar = otherWindow->fBottomVar;
			break;
		default:
			return NULL;
		}
		break;
	}

	fDesktop->MoveWindowBy(this, moveDeltaX, moveDeltaY, 0);

	Constraint* snappingConstraint;
	if (snapDistance == 0)
		snappingConstraint = thisSnappingVar->IsEqual(otherSnappingVar);
	else
		snappingConstraint = fDesktop->fStackAndTileSpec->AddConstraint(
				-1, thisSnappingVar, 1, otherSnappingVar,
				OperatorType(EQ), snapDistance);

	snappingLabel << Title();
	snappingConstraint->SetLabel(snappingLabel.String());
	fSnappingConstraints->AddItem(snappingConstraint);

	AddToSnappingList(otherWindow, thisSnap, otherSnap);
	otherWindow->AddToSnappingList(this, otherSnap, thisSnap);

	return snappingConstraint;
}


/*!	\brief Determines the overall bounding rectangle and the leftmost, topmost,
		rightmost and bottommost window of a list of windows. The bounding
		rectangle is returned.

	\param windows the list of windows for which to do the calculations.
	\param leftmostWindow address where the pointer to the leftmost window
			should be stored.
	\param topmostWindow address where the pointer to the topmost window
			should be stored.
	\param rightmostWindow address where the pointer to the rightmost window
			should be stored.
	\param bottommostWindow address where the pointer to the bottommost window
			should be stored.
*/
BRect
Window::_BoundingRectAndWindows(BList* windows,
	Window** leftmostWindow, Window** topmostWindow,
	Window** rightmostWindow, Window** bottommostWindow)
{
	ASSERT(!windows->IsEmpty());

	Window* firstWindow = static_cast<Window*>(windows->FirstItem());
	BRect bounds = firstWindow->fFrame;
	*leftmostWindow = firstWindow;
	*topmostWindow = firstWindow;
	*rightmostWindow = firstWindow;
	*bottommostWindow = firstWindow;

	// relax result by processing all other windows
	for (int i = 1; i < windows->CountItems(); i++) {
		Window* window = static_cast<Window*>(windows->ItemAt(i));
		if (window->fFrame.left < bounds.left) {
			bounds.left = window->fFrame.left;
			*leftmostWindow = window;
		}
		if (window->fFrame.top < bounds.top) {
			bounds.top = window->fFrame.top;
			*topmostWindow = window;
		}
		if (window->fFrame.right > bounds.right) {
			bounds.right = window->fFrame.right;
			*rightmostWindow = window;
		}
		if (window->fFrame.bottom > bounds.bottom) {
			bounds.bottom = window->fFrame.bottom;
			*bottommostWindow = window;
		}
	}
	return bounds;
}


/*!	\brief Cleans up after the Stack & Tile configuration mode of the window
		manager was exited. The candidates are cleared, tab locations are
		readjusted and unnecessary stack window lists are removed.
*/
void
Window::FinishStackingAndSnapping()
{
	// clear stacking candidate
	fWindowUnder = NULL;

	// clear snapping candidates
	delete fLeftAdjacentWindows;
	delete fTopAdjacentWindows;
	delete fRightAdjacentWindows;
	delete fBottomAdjacentWindows;
	fLeftAdjacentWindows = NULL;
	fTopAdjacentWindows = NULL;
	fRightAdjacentWindows = NULL;
	fBottomAdjacentWindows = NULL;

	// if the window was removed from a stack then rearrange tabs in the stack,
	// remove the stack if it contains only one window
	// and reset tab location of this window if it is not stacked again
	if (fPrevStackedWindows) {
		Window* firstWindow = static_cast<Window*>(
			fPrevStackedWindows->FirstItem());
		firstWindow->_ArrangeStackedWindowTabs();

		// remove stacks with only one window
		if (fPrevStackedWindows->CountItems() == 1) {
			firstWindow->fStackedWindows = NULL;
			delete fPrevStackedWindows;
		}

		if (!fStackedWindows)
			fDesktop->SetWindowTabLocation(this, 0);
		fPrevStackedWindows = NULL;
	}

	// if the window was added to a stack then rearrange tabs in that stack
	_ArrangeStackedWindowTabs();
}


/*!	\brief Solves the Stack & Tile constraints, if the window uses them, sets
	 	window locations and sizes accordingly, and readjusts window tabs of
	 	the windows that are stacked with this window.
*/
void
Window::StackAndTile()
{
	STRACE_SAT(("Window::StackAndTile() on %s, fDesktop->fStackAndTileSpec=%x\n",
		Title(), fDesktop->fStackAndTileSpec));

	// works only if stack and tile is active for this window
	if (!fLeftVar)
		return;

	// adjust window size soft constraints
	fWidthConstraint->SetRightSide(fFrame.Width());
	fHeightConstraint->SetRightSide(fFrame.Height());
	fWidthConstraint->SetPenaltyNeg(110);
	fWidthConstraint->SetPenaltyPos(110);
	fHeightConstraint->SetPenaltyNeg(110);
	fHeightConstraint->SetPenaltyPos(110);

	// adjust window position soft constraints
	// (a bit more penalty for them so they take precedence)
	fLeftConstraint->SetRightSide(fFrame.left);
	fTopConstraint->SetRightSide(fFrame.top);
	fLeftConstraint->SetPenaltyNeg(100);
	fLeftConstraint->SetPenaltyPos(100);
	fTopConstraint->SetPenaltyNeg(100);
	fTopConstraint->SetPenaltyPos(100);

	// solve Stack & Tile specification
	ResultType result;
	for (int32 tries = 0; tries < 15; tries++) {
		result = fDesktop->fStackAndTileSpec->Solve();
		if (result == OPTIMAL || result == INFEASIBLE)
			break;
	}

	// set window locations and sizes
	for (Window* window = fDesktop->GetWindows().LastWindow();
		window;
		window = window->PreviousWindow(fCurrentWorkspace)) {

		if (window->fLeftVar) {
			fDesktop->MoveWindowBy(window,
				window->fLeftVar->Value() - window->fFrame.left,
				window->fTopVar->Value() - window->fFrame.top);
			fDesktop->ResizeWindowBy(window,
				window->fRightVar->Value() - window->fFrame.right,
				window->fBottomVar->Value() - window->fFrame.bottom);
		}
	}

	// set penalties back to normal
	fWidthConstraint->SetPenaltyNeg(10);
	fWidthConstraint->SetPenaltyPos(10);
	fHeightConstraint->SetPenaltyNeg(10);
	fHeightConstraint->SetPenaltyPos(10);
	fLeftConstraint->SetPenaltyNeg(1);
	fLeftConstraint->SetPenaltyPos(1);
	fTopConstraint->SetPenaltyNeg(1);
	fTopConstraint->SetPenaltyPos(1);

	_ArrangeStackedWindowTabs();

	#ifdef DEBUG_STACK_AND_TILE
	// debug_printf crashes if given too large strings
	// therefore we cut our specification string up into smaller chunks
	BString* specStr = fDesktop->fStackAndTileSpec->ToBString();
	char line[71];
	int32 i = specStr->FindFirst("\n");
	while(i != B_ERROR) {
		if (i < 70) {
			specStr->MoveInto(line, 0, i + 1);
			line[i + 1] = '\0';
		} else {
			specStr->MoveInto(line, 0, 70);
			line[70] = '\0';
		}
		debug_printf("%s", line);
		i = specStr->FindFirst("\n");
	}
	while (specStr->Length() > 0) {
		int length = specStr->Length();
		if (length < 70) {
			specStr->MoveInto(line, 0, length);
			line[length] = '\0';
		} else {
			specStr->MoveInto(line, 0, 70);
			line[70] = '\0';
		}
		debug_printf("%s", line);
	}
	debug_printf("\n");
	delete specStr;
	#endif
	STRACE_SAT(("Finished Window::StackAndTile() on %s\n", Title()));
}


void
Window::_BoundWindowByWorkspace()
{
	BRect workspaceRect = fDesktop->WorkspaceFrame(Workspaces());

	_InitStackingAndSnapping();
	fLeftVar->SetRange(workspaceRect.left, workspaceRect.right);
	fTopVar->SetRange(workspaceRect.top, workspaceRect.bottom);
	fRightVar->SetRange(workspaceRect.left, workspaceRect.right);
	fBottomVar->SetRange(workspaceRect.top, workspaceRect.bottom);
}


void
Window::_UnboundWindowByWorkspace()
{
	if (!fLeftVar)
		return;

	fLeftVar->SetRange(-DBL_MAX, DBL_MAX);
	fTopVar->SetRange(-DBL_MAX, DBL_MAX);
	fRightVar->SetRange(-DBL_MAX, DBL_MAX);
	fBottomVar->SetRange(-DBL_MAX, DBL_MAX);
}


/*!	\brief Gets the id of this window. A new id is created randomly if the
		window does not have an id yet.
*/
int32
Window::WindowId()
{
	if(!fId)
		fId = rand() + 1;

	return fId;
}


void
Window::SetWindowId(int32 windowId)
{
	fId = windowId;
}


/*!	\brief Removes the id of this window from the list of stacked window ids
		if there is such a list for this window. The list is made persistent
		through DefaultDecorator::GetSettings().
*/
void
Window::_RemoveStackingPersistently()
{
	STRACE_SAT(("Window::_RemoveStackingPersistently() on %s, fStackedWindowIds=%x\n",
		Title(), fStackedWindowIds));
	if (!fStackedWindowIds)
		return;

	for (int i = 0; i < fStackedWindowIds->CountItems(); i++) {
		int32* id = static_cast<int32*>(fStackedWindowIds->ItemAt(i));
		if (*id == fId) {
			fStackedWindowIds->RemoveItem(i);
			free(id);
		}
	}

	fStackedWindowIds = NULL;
	STRACE_SAT(("Finished Window::_RemoveStackingPersistently() on %s\n",
		Title()));
}


/*!	\brief Removes information about snapped windows that would otherwise be
		made persistent through DefaultDecorator::GetSettings().
*/
void
Window::_RemoveSnappingPersistently()
{
	STRACE_SAT(("Window::_RemoveSnappingPersistently() on %s\n", Title()));

	_FreeUpSnappingList(SNAP_LEFT, SNAP_LEFT, true);
	_FreeUpSnappingList(SNAP_LEFT, SNAP_RIGHT, true);
	_FreeUpSnappingList(SNAP_RIGHT, SNAP_LEFT, true);
	_FreeUpSnappingList(SNAP_RIGHT, SNAP_RIGHT, true);
	_FreeUpSnappingList(SNAP_TOP, SNAP_TOP, true);
	_FreeUpSnappingList(SNAP_TOP, SNAP_BOTTOM, true);
	_FreeUpSnappingList(SNAP_BOTTOM, SNAP_TOP, true);
	_FreeUpSnappingList(SNAP_BOTTOM, SNAP_BOTTOM, true);

	STRACE_SAT(("Finished Window::_RemoveSnappingPersistently() on %s\n",
		Title()));
}


/*!	\brief Deletes the snapping list that corresponds to given snapping
		orientations. Also optionally deletes the current window's id from the
		snapping list of other relevant windows. For exmaple, if current window
		(W1) is snapped to other window (W2) via left 2 right, then removes
		W1 from W2's right2left snapping list.

	\param thisSnap snapping orientation of current window
	\param otherSnap snapping orientation of other window(s)
	\param deleteFromOtherWindowsList whether to delete the current window's
		id from the snapping list of other relevant windows
*/
void
Window::_FreeUpSnappingList(SnapOrientation thisSnap,
		SnapOrientation otherSnap,
		bool deleteFromOtherWindowsList) {
	BList** idListRef = _GetSnappingListRef(thisSnap, otherSnap, false);

	if (!idListRef)
		return;

	BList* idList = *idListRef;
	if (!idList)
		return;

	for (int i = 0; i < idList->CountItems(); i++) {
		int32* id = static_cast<int32*>(idList->ItemAt(i));
		if (deleteFromOtherWindowsList) {
			Window* otherWindow = fDesktop->FindWindow(*id);
			//Remove this window from the other window's snapping list.
			//Note the order of orientation is reversed.
			if (otherWindow)
				otherWindow->RemoveFromSnappingList(WindowId(), otherSnap, thisSnap);
		}
		free(id);
	}
	delete idList;
	*idListRef = NULL;
}


void
Window::UpdateSession::MoveBy(int32 x, int32 y)
{
	fDirtyRegion.OffsetBy(x, y);
}


void
Window::UpdateSession::SetUsed(bool used)
{
	fInUse = used;
	if (!fInUse) {
		fDirtyRegion.MakeEmpty();
		fCause = 0;
	}
}


void
Window::UpdateSession::AddCause(uint8 cause)
{
	fCause |= cause;
}


