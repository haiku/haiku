/*
 * Copyright (c) 2001-2005, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:  DarkWyrm <bpmagic@columbus.rr.com>
 *			Adi Oanca <adioanca@gmail.com>
 *			Stephan Aßmus <superstippi@gmx.de>
 *			Axel Dörfler, axeld@pinc-software.de
 */


#include "WindowLayer.h"

#include "DebugInfoManager.h"
#include "Decorator.h"
#include "DecorManager.h"
#include "Desktop.h"
#include "DrawingEngine.h"
#include "MessagePrivate.h"
#include "PortLink.h"
#include "ServerApp.h"
#include "ServerWindow.h"
#include "Workspace.h"

#include <WindowPrivate.h>

#include <Debug.h>
#include <DirectWindow.h>
#include <View.h>	// for mouse button defines

#include <new>
#include <stdio.h>


// Toggle debug output
//#define DEBUG_WINDOW_LAYER
//#define DEBUG_WINDOW_LAYER_CLICK

#ifdef DEBUG_WINDOW_LAYER
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif

#ifdef DEBUG_WINDOW_LAYER_CLICK
#	define STRACE_CLICK(x) printf x
#else
#	define STRACE_CLICK(x) ;
#endif

// if the background clearing is delayed until
// the client draws the view, we have less flickering
// when contents have to be redrawn because of resizing
// a window or because the client invalidates parts.
// when redrawing something that has been exposed from underneath
// other windows, the other window will be seen longer at
// its previous position though if the exposed parts are not
// cleared right away. maybe there ought to be a flag in
// the update session, which tells us the cause of the update
#define DELAYED_BACKGROUND_CLEARING 0

// IMPORTANT: nested ReadLockWindows()s are not supported (by MultiLocker)

using std::nothrow;

// constructor
WindowLayer::WindowLayer(const BRect& frame, const char *name,
						 window_look look, window_feel feel,
						 uint32 flags, uint32 workspaces,
						 ::ServerWindow* window,
						 DrawingEngine* drawingEngine)
	:
	fTitle(name),
	fFrame(frame),

	fVisibleRegion(),
	fVisibleContentRegion(),
	fVisibleContentRegionValid(false),
	fDirtyRegion(),

	fBorderRegion(),
	fBorderRegionValid(false),
	fContentRegion(),
	fContentRegionValid(false),
	fEffectiveDrawingRegion(),
	fEffectiveDrawingRegionValid(false),

	fIsClosing(false),
	fIsMinimizing(false),
	fIsZooming(false),
	fIsResizing(false),
	fIsSlidingTab(false),
	fIsDragging(false),

	fDecorator(NULL),
	fTopLayer(NULL),
	fWindow(window),
	fDrawingEngine(drawingEngine),
	fDesktop(window->Desktop()),

	fCurrentUpdateSession(),
	fPendingUpdateSession(),
	fUpdateRequested(false),
	fInUpdate(false),

	// windows start hidden
	fHidden(true),
	fIsFocus(false),

	fLook(look),
	fFeel(feel),
	fFlags(flags),
	fWorkspaces(workspaces),
	fCurrentWorkspace(-1),

	fMinWidth(1),
	fMaxWidth(32768),
	fMinHeight(1),
	fMaxHeight(32768),

	fReadLocked(false)
{
	// make sure our arguments are valid 
	if (!IsValidLook(fLook))
		fLook = B_TITLED_WINDOW_LOOK;
	if (!IsValidFeel(fFeel))
		fFeel = B_NORMAL_WINDOW_FEEL;
	fFlags &= ValidWindowFlags();

	if (fLook != B_NO_BORDER_WINDOW_LOOK) {
		fDecorator = gDecorManager.AllocateDecorator(fDesktop, frame, name, fLook, fFlags);
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
		if (fDesktop->ScreenAt(0)) {
			fDesktop->ScreenAt(0)->GetMode(width, height, colorSpace, frequency);
// TODO: MOVE THIS AWAY!!! RemoveBy contains calls to virtual methods! Also, there is not TopLayer()!
			fFrame.OffsetTo(B_ORIGIN);
//			ResizeBy(width - frame.Width(), height - frame.Height(), NULL);
		}
	}

	STRACE(("WindowLayer %p, %s:\n", this, Name()));
	STRACE(("\tFrame: (%.1f, %.1f, %.1f, %.1f)\n", fFrame.left, fFrame.top,
		fFrame.right, fFrame.bottom));
	STRACE(("\tWindow %s\n", window ? window->Title() : "NULL"));
}

// destructor
WindowLayer::~WindowLayer()
{
	delete fTopLayer;
	delete fDecorator;
}
/*
// MessageReceived
void
WindowLayer::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_REDRAW: {
			// there is only one MSG_REDRAW in the queue at anytime
			if (fDesktop->ReadLockWindows()) {

				_DrawBorder();
				_TriggerContentRedraw();

				// reset the dirty region, since
				// we're fully clean. If the desktop
				// thread wanted to mark something
				// dirty in the mean time, it was
				// blocking on the global region lock to
				// get write access, since we held the
				// read lock for the whole time.
				fDirtyRegion.MakeEmpty();

				fDesktop->ReadUnlockWindows();
			} else {
//printf("%s MSG_REDRAW -> pending redraws\n", Name());
			}
			break;
		}
		case MSG_BEGIN_UPDATE:
			_BeginUpdate();
			break;
		case MSG_END_UPDATE:
			_EndUpdate();
			break;
		case MSG_DRAWING_COMMAND: {
			int32 token;
			if (message->FindInt32("token", &token) >= B_OK)
				_DrawClient(token);
			break;
		}
		case MSG_DRAW_POLYGON: {
			int32 token;
			BPoint polygon[4];
			if (message->FindInt32("token", &token) >= B_OK &&
				message->FindPoint("point", 0, &polygon[0]) >= B_OK &&
				message->FindPoint("point", 1, &polygon[1]) >= B_OK &&
				message->FindPoint("point", 2, &polygon[2]) >= B_OK &&
				message->FindPoint("point", 3, &polygon[3]) >= B_OK) {

				_DrawClientPolygon(token, polygon);
			}
			break;
			
		}

		case MSG_INVALIDATE_VIEW: {
			int32 token;
			if (message->FindInt32("token", &token) >= B_OK)
				InvalidateView(token);
			break;
		}

		case MSG_SHOW:
			if (IsHidden()) {
				fDesktop->ShowWindow(this);
			}
			break;
		default:
			BLooper::MessageReceived(message);
			break;
	}
}

// QuitRequested
bool
WindowLayer::QuitRequested()
{
	if (fDesktop && fDesktop->WriteLockWindows()) {
		fDesktop->WindowDied(this);

		fClient->Lock();
		fClient->Quit();

		fDesktop->WriteUnlockWindows();
	}
	return true;
}
*/

// ReadLockWindows
bool
WindowLayer::ReadLockWindows()
{
	if (fDesktop)
		fReadLocked = fDesktop->ReadLockWindows();
	else
		fReadLocked = true;

	return fReadLocked;
}

// ReadUnlockWindows
void
WindowLayer::ReadUnlockWindows()
{
	if (fReadLocked) {
		if (fDesktop)
			fDesktop->ReadUnlockWindows();
		fReadLocked = false;
	}
}

// SetClipping
void
WindowLayer::SetClipping(BRegion* stillAvailableOnScreen)
{
	// this function is only called from the Desktop thread

	// start from full region (as if the window was fully visible)
	GetFullRegion(&fVisibleRegion);
	// clip to region still available on screen
	fVisibleRegion.IntersectWith(stillAvailableOnScreen);

	fVisibleContentRegionValid = false;
	fEffectiveDrawingRegionValid = false;

	// TODO: Window()->HandleDirectConnection(B_DIRECT_STOP);
}

// GetFullRegion
void
WindowLayer::GetFullRegion(BRegion* region)
{
	// TODO: if someone needs to call this from
	// the outside, the clipping needs to be readlocked!

	GetBorderRegion(region);

	// start from the frame, extend to include decorator border
	region->Include(fFrame);

}

// GetBorderRegion
void
WindowLayer::GetBorderRegion(BRegion* region)
{
	// TODO: if someone needs to call this from
	// the outside, the clipping needs to be readlocked!

	if (!fBorderRegionValid) {
		// TODO: checkup Decorator::GetFootPrint() to see if it is as fast as this:
/*		fBorderRegion.Set(BRect(fFrame.left - 4, fFrame.top - 20,
							  	(fFrame.left + fFrame.right) / 2, fFrame.top - 5));
		fBorderRegion.Include(BRect(fFrame.left - 4, fFrame.top - 4,
									fFrame.right + 4, fFrame.top - 1));
		fBorderRegion.Include(BRect(fFrame.left - 4, fFrame.top,
									fFrame.left - 1, fFrame.bottom));
		fBorderRegion.Include(BRect(fFrame.right + 1, fFrame.top,
									fFrame.right + 4, fFrame.bottom - 11));
		fBorderRegion.Include(BRect(fFrame.left - 4, fFrame.bottom + 1,
									fFrame.right - 11, fFrame.bottom + 4));
		fBorderRegion.Include(BRect(fFrame.right - 10, fFrame.bottom - 10,
									fFrame.right + 4, fFrame.bottom + 4));*/

		// TODO: remove and use Decorator::GetFootPrint()
		// start from the frame, extend to include decorator border
		if (fDecorator) {
			fDecorator->GetFootprint(&fBorderRegion);
		}
		fBorderRegionValid = true;
	}

	*region = fBorderRegion;
}


void
WindowLayer::GetContentRegion(BRegion* region)
{
	// TODO: if someone needs to call this from
	// the outside, the clipping needs to be readlocked!

	if (!fContentRegionValid) {
		_UpdateContentRegion();
	}

	*region = fContentRegion;
}


BRegion&
WindowLayer::VisibleContentRegion()
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
WindowLayer::MoveBy(int32 x, int32 y)
{
	// this function is only called from the desktop thread

	if ((x == 0 && y == 0) || !ReadLockWindows())
		return;

	fWindow->HandleDirectConnection(B_DIRECT_STOP);

	fFrame.OffsetBy(x, y);

	fWindow->HandleDirectConnection(B_DIRECT_START | B_BUFFER_MOVED);

	// take along the dirty region which have not
	// processed yet
	fDirtyRegion.OffsetBy(x, y);

	if (fBorderRegionValid)
		fBorderRegion.OffsetBy(x, y);
	if (fContentRegionValid)
		fContentRegion.OffsetBy(x, y);

	if (fCurrentUpdateSession.IsUsed())
		fCurrentUpdateSession.MoveBy(x, y);
	if (fPendingUpdateSession.IsUsed())
		fPendingUpdateSession.MoveBy(x, y);

	fEffectiveDrawingRegionValid = false;

	if (fDecorator)
		fDecorator->MoveBy(x, y);

	if (fTopLayer != NULL)
		fTopLayer->MoveBy(x, y, NULL);

	// the desktop will take care of dirty regions

	// dispatch a message to the client informing about the changed size
	BMessage msg(B_WINDOW_MOVED);
	msg.AddInt64("when", system_time());
	msg.AddPoint("where", fFrame.LeftTop());
	fWindow->SendMessageToClient(&msg);

	ReadUnlockWindows();
}


void
WindowLayer::ResizeBy(int32 x, int32 y, BRegion* dirtyRegion)
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

	if ((x == 0 && y == 0) || !ReadLockWindows())
		return;

	fWindow->HandleDirectConnection(B_DIRECT_STOP);

	fFrame.right += x;
	fFrame.bottom += y;

	fWindow->HandleDirectConnection(B_DIRECT_START | B_BUFFER_RESIZED);

	// put the previous border region into the dirty region as well
	// to handle the part that was overlapping a layer
	if (dirtyRegion)
		dirtyRegion->Include(&fBorderRegion);

	fBorderRegionValid = false;
	fContentRegionValid = false;
	fEffectiveDrawingRegionValid = false;

	if (fDecorator)
		fDecorator->ResizeBy(x, y);

	// the border is dirty, put it into
	// dirtyRegion for a start
	BRegion newBorderRegion;
	GetBorderRegion(&newBorderRegion);
	if (dirtyRegion) {
		dirtyRegion->Include(&newBorderRegion);
	}

	if (fTopLayer != NULL)
		fTopLayer->ResizeBy(x, y, dirtyRegion);

	// send a message to the client informing about the changed size
	BRect frame(Frame());
	BMessage msg(B_WINDOW_RESIZED);
	msg.AddInt64("when", system_time());
	msg.AddInt32("width", frame.IntegerWidth());
	msg.AddInt32("height", frame.IntegerHeight());
	fWindow->SendMessageToClient(&msg);

	ReadUnlockWindows();
}


void
WindowLayer::ScrollViewBy(ViewLayer* view, int32 dx, int32 dy)
{
	// this can be executed from any thread, but if the
	// desktop thread is executing this, it should have
	// the write lock, otherwise it is not prevented
	// from executing this at the same time as the window
	// is doing something else here!

	if (!view || view == fTopLayer || (dx == 0 && dy == 0))
		return;

	if (fDesktop && fDesktop->ReadLockWindows()) {
		BRegion dirty;
		view->ScrollBy(dx, dy, &dirty);

		_MarkContentDirty(&dirty);

		fDesktop->ReadUnlockWindows();
	}
}


//! Takes care of invalidating parts that could not be copied
void
WindowLayer::CopyContents(BRegion* region, int32 xOffset, int32 yOffset)
{
	if (!fHidden && fDesktop && fDesktop->ReadLockWindows()) {
		BRegion newDirty(*region);

		// clip the region to the visible contents at the
		// source and destination location (not that VisibleContentRegion()
		// is used once to make sure it is valid, then fVisibleContentRegion
		// is used directly)
		region->IntersectWith(&VisibleContentRegion());
		if (region->CountRects() > 0) {
			region->OffsetBy(xOffset, yOffset);
			region->IntersectWith(&fVisibleContentRegion);
			if (region->CountRects() > 0) {
				// if the region still contains any rects
				// offset to source location again
				region->OffsetBy(-xOffset, -yOffset);
				// the part which we can copy is not dirty
				newDirty.Exclude(region);
		
				if (fDrawingEngine->Lock()) {
					fDrawingEngine->CopyRegion(region, xOffset, yOffset);
					fDrawingEngine->Unlock();
				}

				// move along the already dirty regions that are common
				// with the region that we could copy
				_ShiftPartOfRegion(&fDirtyRegion, region, xOffset, yOffset);
				if (fCurrentUpdateSession.IsUsed())
					_ShiftPartOfRegion(&fCurrentUpdateSession.DirtyRegion(), region, xOffset, yOffset);
				if (fPendingUpdateSession.IsUsed())
					_ShiftPartOfRegion(&fPendingUpdateSession.DirtyRegion(), region, xOffset, yOffset);
		
			}
		}
		// what is left visible from the original region
		// at the destination after the region which could be
		// copied has been excluded, is considered dirty
		// NOTE: it may look like dirty regions are not moved
		// if no region could be copied, but that's alright,
		// since these parts will now be in newDirty anyways
		// (with the right offset)
		newDirty.OffsetBy(xOffset, yOffset);
		newDirty.IntersectWith(&fVisibleContentRegion);
		if (newDirty.CountRects() > 0)
			ProcessDirtyRegion(newDirty);

		fDesktop->ReadUnlockWindows();
	}
}


// #pragma mark -


void
WindowLayer::SetTopLayer(ViewLayer* topLayer)
{
	// the top layer is special, it has a coordinate system
	// as if it was attached directly to the desktop, therefor,
	// the coordinate conversion through the layer tree works
	// as expected, since the top layer has no "parent" but has
	// fFrame as if it had

	fTopLayer = topLayer;

	// make sure the location of the top layer on screen matches ours
	fTopLayer->MoveBy(fFrame.left - fTopLayer->Frame().left,
		fFrame.top - fTopLayer->Frame().top, NULL);

	// make sure the size of the top layer matches ours
	fTopLayer->ResizeBy(fFrame.Width() - fTopLayer->Frame().Width(),
		fFrame.Height() - fTopLayer->Frame().Height(), NULL);

	fTopLayer->AttachedToWindow(this);
}


ViewLayer*
WindowLayer::ViewAt(const BPoint& where)
{
	ViewLayer* view = NULL;

	if (ReadLockWindows()) {
		if (!fContentRegionValid)
			_UpdateContentRegion();

		view = fTopLayer->ViewAt(where, &fContentRegion);

		ReadUnlockWindows();
	}
	return view;
}


window_anchor&
WindowLayer::Anchor(int32 index)
{
	return fAnchor[index];
}


WindowLayer*
WindowLayer::NextWindow(int32 index)
{
	return fAnchor[index].next;
}


WindowLayer*
WindowLayer::PreviousWindow(int32 index)
{
	return fAnchor[index].previous;
}


// #pragma mark -


void
WindowLayer::GetEffectiveDrawingRegion(ViewLayer* layer, BRegion& region)
{
	if (!fEffectiveDrawingRegionValid) {
		fEffectiveDrawingRegion = VisibleContentRegion();
		if (fInUpdate) {
			// enforce the dirty region of the update session
			fEffectiveDrawingRegion.IntersectWith(&fCurrentUpdateSession.DirtyRegion());
		} else {
			// not in update, the view can draw everywhere
		}

		fEffectiveDrawingRegionValid = true;
	}

	// TODO: this is a region that needs to be cached later in the server
	// when the current layer in ServerWindow is set, and we are currently
	// in an update (fInUpdate), than we can set this region and remember
	// it for the comming drawing commands until the current layer changes
	// again or fEffectiveDrawingRegionValid is suddenly false.
	region = fEffectiveDrawingRegion;
	if (!fContentRegionValid)
		_UpdateContentRegion();

	region.IntersectWith(&layer->ScreenClipping(&fContentRegion));
}

bool
WindowLayer::DrawingRegionChanged(ViewLayer* layer) const
{
	return !fEffectiveDrawingRegionValid || !layer->IsScreenClippingValid();
}


void
WindowLayer::ProcessDirtyRegion(BRegion& region)
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
		ServerWindow()->PostMessage(AS_REDRAW);
	}

	// this is executed from the desktop thread
	fDirtyRegion.Include(&region);
}


void
WindowLayer::RedrawDirtyRegion()
{
	if (!fDesktop || !fDesktop->ReadLockWindows())
		return;

	if (IsVisible()) {
		_DrawBorder();
		_TriggerContentRedraw();
	}

	// reset the dirty region, since
	// we're fully clean. If the desktop
	// thread wanted to mark something
	// dirty in the mean time, it was
	// blocking on the global region lock to
	// get write access, since we held the
	// read lock for the whole time.
	fDirtyRegion.MakeEmpty();

	fDesktop->ReadUnlockWindows();
}


void
WindowLayer::MarkDirty(BRegion& regionOnScreen)
{
	// for marking any part of the desktop dirty
	// this will get write access to the global
	// region lock, and result in ProcessDirtyRegion()
	// to be called for any windows affected
	if (fDesktop)
		fDesktop->MarkDirty(regionOnScreen);
}


void
WindowLayer::MarkContentDirty(BRegion& regionOnScreen)
{
	// for triggering AS_REDRAW
	// since this won't affect other windows, read locking
	// is sufficient. If there was no dirty region before,
	// an update message is triggered
	if (!fHidden && fDesktop && fDesktop->ReadLockWindows()) {
		regionOnScreen.IntersectWith(&VisibleContentRegion());
		ProcessDirtyRegion(regionOnScreen);

		fDesktop->ReadUnlockWindows();
	}
}


void
WindowLayer::InvalidateView(ViewLayer* layer, BRegion& layerRegion)
{
	if (layer && !fHidden && fDesktop && fDesktop->ReadLockWindows()) {
		if (!layer->IsVisible()) {
			fDesktop->ReadUnlockWindows();
			return;
		}
		if (!fContentRegionValid)
			_UpdateContentRegion();

		layer->ConvertToScreen(&layerRegion);
		layerRegion.IntersectWith(&layer->ScreenClipping(&fContentRegion));
		// TODO: would be nice if this didn't fire off a REDRAW message,
		// but currently, it is cleaner to do it, so that backgrounds are
		// properly cleared in _TriggerContentRedraw()
		MarkContentDirty(layerRegion);

		fDesktop->ReadUnlockWindows();
	}
}

// EnableUpdateRequests
void
WindowLayer::EnableUpdateRequests()
{
//	fUpdateRequestsEnabled = true;
/*	if (fCumulativeRegion.CountRects() > 0) {
		GetRootLayer()->MarkForRedraw(fCumulativeRegion);
		GetRootLayer()->TriggerRedraw();
	}*/
}


// #pragma mark -


void
WindowLayer::MouseDown(BMessage* msg, BPoint where, int32* _viewToken)
{
	// TODO: move into Decorator
	if (!fBorderRegionValid)
		GetBorderRegion(&fBorderRegion);

	// default action is to drag the WindowLayer
	if (fBorderRegion.Contains(where)) {
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
			fDesktop->SendWindowBehind(this);
		} else {
			fDesktop->SetMouseEventWindow(this);
			fDesktop->ActivateWindow(this);
		}
	} else {
		if (ViewLayer* view = ViewAt(where)) {
//printf("clicked: %s\n", view->Name());
//view->PrintToStream();
			// clicking a simple ViewLayer
			if (!IsFocus()) {
				DesktopSettings desktopSettings(fDesktop);

				// not in FFM mode?
				if (desktopSettings.MouseMode() == B_NORMAL_MOUSE)
					fDesktop->ActivateWindow(this);

				// eat the click if we don't accept first click
				if ((Flags() & (B_WILL_ACCEPT_FIRST_CLICK | B_AVOID_FOCUS)) != 0)
					*_viewToken = view->Token();
			} else {
				// always fill out view token if we already have focus
				*_viewToken = view->Token();
			}
		} else {
//printf("clicked: nowhere\n");
		}
	}
}


void
WindowLayer::MouseUp(BMessage* msg, BPoint where, int32* _viewToken)
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
				fWindow->NotifyZoom();
			}
		}
		if (fIsClosing) {
			fIsClosing	= false;
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
	}
	fIsDragging = false;
	fIsResizing = false;
	fIsSlidingTab = false;

	if (ViewLayer* view = ViewAt(where))
		*_viewToken = view->Token();
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
		fDesktop->MoveWindowBy(this, delta.x, delta.y);
	}
	if (fIsResizing) {
		BPoint delta = where - fLastMousePosition;
		if (Flags() & B_NOT_V_RESIZABLE)
			delta.y = 0;
		if (Flags() & B_NOT_H_RESIZABLE)
			delta.x = 0;

		fDesktop->ResizeWindowBy(this, delta.x, delta.y);
	}
	if (fIsSlidingTab) {
		// TODO: implement
	}

	fLastMousePosition = where;

	// change focus in FFM mode
	DesktopSettings desktopSettings(fDesktop);

	if (desktopSettings.MouseMode() != B_NORMAL_MOUSE && !IsFocus())
		fDesktop->SetFocusWindow(this);

	if (ViewLayer* view = ViewAt(where))
		*_viewToken = view->Token();
}


// #pragma mark -


void
WindowLayer::WorkspaceActivated(int32 index, bool active)
{
	BMessage activatedMsg(B_WORKSPACE_ACTIVATED);
	activatedMsg.AddInt64("when", system_time());
	activatedMsg.AddInt32("workspace", index);
	activatedMsg.AddBool("active", active);

	ServerWindow()->SendMessageToClient(&activatedMsg);
}


void
WindowLayer::WorkspacesChanged(uint32 oldWorkspaces, uint32 newWorkspaces)
{
	fWorkspaces = newWorkspaces;

	BMessage changedMsg(B_WORKSPACES_CHANGED);
	changedMsg.AddInt64("when", system_time());
	changedMsg.AddInt32("old", oldWorkspaces);
	changedMsg.AddInt32("new", newWorkspaces);

	ServerWindow()->SendMessageToClient(&changedMsg);
}


void
WindowLayer::Activated(bool active)
{
	BMessage msg(B_WINDOW_ACTIVATED);
	msg.AddBool("active", active);
	ServerWindow()->SendMessageToClient(&msg);
}


//# pragma mark -


void
WindowLayer::SetTitle(const char* name)
{
	// rebuild the clipping for the title area
	// and redraw it.

	fTitle = name;

/* TODO: SetTitle
	if (fDecorator) {
		// TODO: need locking here too
		BRegion updateRegion;
		fDecorator->SetTitle(name, &updateRegion);

		if (fVisible && fDesktop & fDesktop->WriteLockWindows()) {
			// ....
			fDesktop->WriteUnlockWindows();
		}
	}
*/
}


void
WindowLayer::SetFocus(bool focus)
{
	// executed from Desktop thread
	// it holds the clipping write lock,
	// so the window thread cannot be
	// accessing fIsFocus

	BRegion dirty(fBorderRegion);
	dirty.IntersectWith(&fVisibleRegion);
	fDesktop->MarkDirty(dirty);

	fIsFocus = focus;
	if (fDecorator)
		fDecorator->SetFocus(focus);

	Activated(focus);
}


void
WindowLayer::SetHidden(bool hidden)
{
	// the desktop takes care of dirty regions
	if (fHidden != hidden) {
		fHidden = hidden;

		fTopLayer->SetHidden(hidden);

		// TODO: anything else?
	}
}


bool
WindowLayer::IsVisible() const
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
WindowLayer::SetSizeLimits(int32 minWidth, int32 maxWidth,
	int32 minHeight, int32 maxHeight)
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
		fDecorator->GetSizeLimits(&fMinWidth, &fMinHeight,
								  &fMaxWidth, &fMaxHeight);
	}

	_ObeySizeLimits();
}


void
WindowLayer::GetSizeLimits(int32* minWidth, int32* maxWidth,
	int32* minHeight, int32* maxHeight) const
{
	*minWidth = fMinWidth;
	*maxWidth = fMaxWidth;
	*minHeight = fMinHeight;
	*maxHeight = fMaxHeight;
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


void
WindowLayer::SetLook(window_look look, BRegion* updateRegion)
{
	if (fDecorator == NULL && look != B_NO_BORDER_WINDOW_LOOK) {
		// we need a new decorator
		fDecorator = gDecorManager.AllocateDecorator(fDesktop, Frame(),
			Title(), fLook, fFlags);
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
		fDecorator->GetSizeLimits(&fMinWidth, &fMinHeight, &fMaxWidth, &fMaxHeight);
		_ObeySizeLimits();
	}

	if (look == B_NO_BORDER_WINDOW_LOOK) {
		// we don't need a decorator for this window
		delete fDecorator;
		fDecorator = NULL;
	}
}


void
WindowLayer::SetFeel(window_feel feel)
{
	// if the subset list is no longer needed, clear it
	if ((fFeel == B_MODAL_SUBSET_WINDOW_FEEL || fFeel == B_FLOATING_SUBSET_WINDOW_FEEL)
		&& (feel != B_MODAL_SUBSET_WINDOW_FEEL && feel != B_FLOATING_SUBSET_WINDOW_FEEL))
		fSubsets.MakeEmpty();

	fFeel = feel;

	// having modal windows with B_AVOID_FRONT or B_AVOID_FOCUS doesn't
	// make that much sense, so we filter those flags out on demand
	fFlags &= ~ValidWindowFlags(fFeel);
}


void
WindowLayer::SetFlags(uint32 flags, BRegion* updateRegion)
{
	fFlags = flags & ~ValidWindowFlags(fFeel);

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
}

/*!
	\brief Returns wether or not the window is visible on the specified
		workspace.

	A modal or floating window may be visible on a workscreen if one
	of its subset windows is visible there.
*/
bool
WindowLayer::OnWorkspace(int32 index) const
{
	if ((fWorkspaces & (1UL << index)) != 0
		|| fFeel == B_MODAL_ALL_WINDOW_FEEL
		|| fFeel == B_FLOATING_ALL_WINDOW_FEEL)
		return true;

	if (fFeel == B_MODAL_APP_WINDOW_FEEL
		|| fFeel == B_FLOATING_APP_WINDOW_FEEL)
		return ServerWindow()->App()->OnWorkspace(index);

	if (fFeel == B_MODAL_SUBSET_WINDOW_FEEL
		|| fFeel == B_FLOATING_SUBSET_WINDOW_FEEL) {
		for (int32 i = 0; i < fSubsets.CountItems(); i++) {
			WindowLayer* window = fSubsets.ItemAt(i);
			if (window->OnWorkspace(index))
				return true;
		}
	}

	return false;
}


bool
WindowLayer::SupportsFront()
{
	if (fFeel == kDesktopWindowFeel)
		return false;

	return true;
}


bool
WindowLayer::IsModal() const
{
	return IsModalFeel(fFeel);
}


bool
WindowLayer::IsFloating() const
{
	return IsFloatingFeel(fFeel);
}


bool
WindowLayer::IsNormal() const
{
	return !IsFloatingFeel(fFeel) && !IsModalFeel(fFeel);
}


/*!
	\brief Returns the windows that's in behind of the backmost position
		this window can get.
		Returns NULL is this window can be the backmost window.
*/
WindowLayer*
WindowLayer::Backmost(WindowLayer* window, int32 workspace)
{
	if (workspace == -1)
		workspace = fCurrentWorkspace;

	if (fFeel == kDesktopWindowFeel)
		return NULL;
	else if (fFeel == B_FLOATING_ALL_WINDOW_FEEL)
		return window ? window : PreviousWindow(workspace);

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


/*!
	\brief Returns the windows that's in front of the frontmost position
		this window can get.
		Returns NULL is this window can be the frontmost window.
*/
WindowLayer*
WindowLayer::Frontmost(WindowLayer* first, int32 workspace)
{
	if (workspace == -1)
		workspace = fCurrentWorkspace;

	if (fFeel == kDesktopWindowFeel)
		return first ? first : NextWindow(workspace);

	if (fFeel == B_FLOATING_ALL_WINDOW_FEEL)
		return NULL;

	if (first == NULL)
		first = NextWindow(workspace);

	for (WindowLayer* window = first; window != NULL;
			window = window->NextWindow(workspace)) {
		if (window->IsHidden() || window == this)
			continue;

		// no one can be in front of a floating all window
		if (window->Feel() == B_FLOATING_ALL_WINDOW_FEEL)
			return window;

		if (window->HasInSubset(this))
			return window;
	}

	return NULL;
}


bool
WindowLayer::AddToSubset(WindowLayer* window)
{
	return fSubsets.AddItem(window);
}


void
WindowLayer::RemoveFromSubset(WindowLayer* window)
{
	fSubsets.RemoveItem(window);
}


bool
WindowLayer::HasInSubset(WindowLayer* window)
{
	if (fFeel == B_MODAL_APP_WINDOW_FEEL && window->Feel() == B_MODAL_ALL_WINDOW_FEEL
		|| fFeel == B_NORMAL_WINDOW_FEEL
		|| fFeel == window->Feel())
		return false;

	if (fFeel == B_FLOATING_ALL_WINDOW_FEEL
		|| fFeel == B_MODAL_ALL_WINDOW_FEEL)
		return true;

	if (fFeel == B_FLOATING_APP_WINDOW_FEEL
		|| fFeel == B_MODAL_APP_WINDOW_FEEL)
		return window->ServerWindow()->App() == ServerWindow()->App();

	return fSubsets.HasItem(window);
}


bool
WindowLayer::SameSubset(WindowLayer* window)
{
	// TODO: this is probably not needed at all, but it doesn't hurt to have it in svn
	if (fFeel == B_MODAL_ALL_WINDOW_FEEL || window->Feel() == B_MODAL_ALL_WINDOW_FEEL)
		return fFeel == window->Feel();

	if (fFeel == B_MODAL_APP_WINDOW_FEEL || window->Feel() == B_MODAL_APP_WINDOW_FEEL)
		return ServerWindow()->App() == window->ServerWindow()->App();

	if (fFeel == B_MODAL_SUBSET_WINDOW_FEEL) {
		// we basically need to check if the subsets have anything in common
		for (int32 i = fSubsets.CountItems(); i-- > 0;) {
			if (window->HasInSubset(fSubsets.ItemAt(i)))
				return true;
		}
	}
	if (window->Feel() == B_MODAL_SUBSET_WINDOW_FEEL) {
		for (int32 i = window->fSubsets.CountItems(); i-- > 0;) {
			if (HasInSubset(window->fSubsets.ItemAt(i)))
				return true;
		}
	}

	return false;
}


// #pragma mark - static


/*static*/
bool
WindowLayer::IsValidLook(window_look look)
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


/*static*/
bool
WindowLayer::IsValidFeel(window_feel feel)
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
		|| feel == kWindowScreenFeel;
}


/*static*/
bool
WindowLayer::IsModalFeel(window_feel feel)
{
	return feel == B_MODAL_SUBSET_WINDOW_FEEL
		|| feel == B_MODAL_APP_WINDOW_FEEL
		|| feel == B_MODAL_ALL_WINDOW_FEEL;
}


/*static*/
bool
WindowLayer::IsFloatingFeel(window_feel feel)
{
	return feel == B_FLOATING_SUBSET_WINDOW_FEEL
		|| feel == B_FLOATING_APP_WINDOW_FEEL
		|| feel == B_FLOATING_ALL_WINDOW_FEEL;
}


/*static*/
uint32
WindowLayer::ValidWindowFlags()
{
	return B_NOT_MOVABLE | B_NOT_CLOSABLE | B_NOT_ZOOMABLE
		| B_NOT_MINIMIZABLE | B_NOT_RESIZABLE
		| B_NOT_H_RESIZABLE | B_NOT_V_RESIZABLE
		| B_AVOID_FRONT | B_AVOID_FOCUS
		| B_WILL_ACCEPT_FIRST_CLICK | B_OUTLINE_RESIZE
		| B_NO_WORKSPACE_ACTIVATION
		| B_NOT_ANCHORED_ON_ACTIVATE
		| B_ASYNCHRONOUS_CONTROLS
		| B_QUIT_ON_WINDOW_CLOSE
		| kWorkspacesWindowFlag
		| kWindowScreenFlag;
}


/*static*/
uint32
WindowLayer::ValidWindowFlags(window_feel feel)
{
	uint32 flags = ValidWindowFlags();
	if (IsModalFeel(feel))
		return flags & ~(B_AVOID_FOCUS | B_AVOID_FRONT);

	return flags;
}



// #pragma mark - private

// _ShiftPartOfRegion
void
WindowLayer::_ShiftPartOfRegion(BRegion* region, BRegion* regionToShift,
								int32 xOffset, int32 yOffset)
{
	BRegion common(*regionToShift);
	// see if there is a common part at all
	common.IntersectWith(region);
	if (common.CountRects() > 0) {
		// cut the common part from the region,
		// offset that to destination and include again
		region->Exclude(&common);
		common.OffsetBy(xOffset, yOffset);
		region->Include(&common);
	}
}


void
WindowLayer::_TriggerContentRedraw()
{
	BRegion dirtyContentRegion(VisibleContentRegion());
	dirtyContentRegion.IntersectWith(&fDirtyRegion);

	if (dirtyContentRegion.CountRects() > 0) {
		// send UPDATE message to the client
		_MarkContentDirty(&dirtyContentRegion);

		if (!fContentRegionValid)
			_UpdateContentRegion();

		if (fDrawingEngine->Lock()) {
			fDrawingEngine->ConstrainClippingRegion(&dirtyContentRegion);
#if DELAYED_BACKGROUND_CLEARING
			fTopLayer->Draw(fDrawingEngine, &dirtyContentRegion,
							&fContentRegion, false);
#else
			fTopLayer->Draw(fDrawingEngine, &dirtyContentRegion,
							&fContentRegion, true);
#endif
			fDrawingEngine->Unlock();
		}
	}
}


void
WindowLayer::_DrawBorder()
{
	// this is executed in the window thread, but only
	// in respond to a REDRAW message having been received, the
	// clipping lock is held for reading

	if (!fDecorator)
		return;

	// construct the region of the border that needs redrawing
	BRegion dirtyBorderRegion;
	GetBorderRegion(&dirtyBorderRegion);
	// intersect with our visible region
	dirtyBorderRegion.IntersectWith(&fVisibleRegion);
	// intersect with the dirty region
	dirtyBorderRegion.IntersectWith(&fDirtyRegion);

	if (dirtyBorderRegion.CountRects() > 0) {
// TODO: decorator drawing with update region...
		fDrawingEngine->ConstrainClippingRegion(&dirtyBorderRegion);
		fDecorator->Draw(dirtyBorderRegion.Frame());
		fDrawingEngine->ConstrainClippingRegion(NULL);
	}
}


/*!
	pre: the clipping is readlocked (this function is
	only called from _TriggerContentRedraw()), which
	in turn is only called from MessageReceived() with
	the clipping lock held
*/
void
WindowLayer::_MarkContentDirty(BRegion* contentDirtyRegion)
{
	if (contentDirtyRegion->CountRects() <= 0)
		return;

	// add to pending
	fPendingUpdateSession.SetUsed(true);
	fPendingUpdateSession.Include(contentDirtyRegion);

	// clip pending update session from current
	// update session, it makes no sense to draw stuff
	// already needing a redraw anyways. Theoretically,
	// this could be done smarter (clip layers from pending
	// that have not yet been redrawn in the current update
	// session)
	if (fCurrentUpdateSession.IsUsed()) {
		fCurrentUpdateSession.Exclude(contentDirtyRegion);
		fEffectiveDrawingRegionValid = false;
	}

	if (!fUpdateRequested) {
		// send this to client
		_SendUpdateMessage();
		// as long as we have not received
		// the "begin update" command, the
		// pending session does not become the
		// current
// TODO: problem: since we sent the update regions frame,
// we should not add to the pending session after we
// sent the update message!!!
	} else {
		if (!fCurrentUpdateSession.IsUsed())
			fprintf(stderr, "WindowLayer(%s)::_MarkContentDirty() - pending region changed before BeginUpdate()!\n", Title());
	}
}

// _SendUpdateMessage
void
WindowLayer::_SendUpdateMessage()
{
	BMessage message(_UPDATE_);
	BRect updateRect = fPendingUpdateSession.DirtyRegion().Frame();
	updateRect.OffsetBy(-fFrame.left, -fFrame.top);
	message.AddRect("_rect", updateRect);
	ServerWindow()->SendMessageToClient(&message);
	fUpdateRequested = true;

	// TODO: the toggling between the update sessions is too
	// expensive, optimize with some pointer tricks
	fCurrentUpdateSession = fPendingUpdateSession;
	fPendingUpdateSession.SetUsed(false);
}


void
WindowLayer::BeginUpdate()
{
	// NOTE: since we might "shift" parts of the
	// internal dirty regions from the desktop thread
	// in response to WindowLayer::ResizeBy(), which
	// might move arround views, this function needs to block
	// on the global clipping lock so that the internal
	// dirty regions are not messed with from the Desktop thread
	// and ServerWindow thread at the same time.
	if (!fDesktop->ReadLockWindows())
		return;

	if (fUpdateRequested && fCurrentUpdateSession.IsUsed()) {
		// all drawing command from the client
		// will have the dirty region from the update
		// session enforced
		fInUpdate = true;
		fEffectiveDrawingRegionValid = false;
	}

	fDesktop->ReadUnlockWindows();
}


void
WindowLayer::EndUpdate()
{
	// NOTE: see comment in _BeginUpdate()
	if (!fDesktop->ReadLockWindows())
		return;

	if (fInUpdate) {
		fCurrentUpdateSession.SetUsed(false);

		fInUpdate = false;
		fEffectiveDrawingRegionValid = false;
	}
	if (fPendingUpdateSession.IsUsed()) {
		// send this to client
		_SendUpdateMessage();
	} else {
		fUpdateRequested = false;
	}

	fDesktop->ReadUnlockWindows();
}


void
WindowLayer::_UpdateContentRegion()
{
	// TODO: speed up by avoiding "Exclude()"
	// start from the frame, extend to include decorator border
	fContentRegion.Set(fFrame);

	// resize handle
	if (fDecorator) {
		if (!fBorderRegionValid)
			GetBorderRegion(&fBorderRegion);

		fContentRegion.Exclude(&fBorderRegion);
	}

	fContentRegionValid = true;
}


click_type
WindowLayer::_ActionFor(const BMessage* msg) const
{
	BPoint where(0, 0);
	int32 buttons = 0;
	int32 modifiers = 0;

	msg->FindPoint("where", &where);
	msg->FindInt32("buttons", &buttons);
	msg->FindInt32("modifiers", &modifiers);

	if (fDecorator)
		return fDecorator->Clicked(where, buttons, modifiers);

	return DEC_NONE;
}

// _ObeySizeLimits
void
WindowLayer::_ObeySizeLimits()
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
		ResizeBy(xDiff, yDiff, NULL);
}

// #pragma mark - UpdateSession

// constructor
WindowLayer::UpdateSession::UpdateSession()
	: fDirtyRegion(),
	  fInUse(false)
{
}

// destructor
WindowLayer::UpdateSession::~UpdateSession()
{
}

// Include
void
WindowLayer::UpdateSession::Include(BRegion* additionalDirty)
{
	fDirtyRegion.Include(additionalDirty);
}


void
WindowLayer::UpdateSession::Exclude(BRegion* dirtyInNextSession)
{
	fDirtyRegion.Exclude(dirtyInNextSession);
}


void
WindowLayer::UpdateSession::MoveBy(int32 x, int32 y)
{
	fDirtyRegion.OffsetBy(x, y);
}


void
WindowLayer::UpdateSession::SetUsed(bool used)
{
	fInUse = used;
	if (!fInUse)
		fDirtyRegion.MakeEmpty();
}


WindowLayer::UpdateSession&
WindowLayer::UpdateSession::operator=(const WindowLayer::UpdateSession& other)
{
	fDirtyRegion = other.fDirtyRegion;
	fInUse = other.fInUse;
	return *this;
}



