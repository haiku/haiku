
#include <stdio.h>

#include <Bitmap.h>
#include <Message.h>
#include <MessageQueue.h>
#include <Messenger.h>
#include <Window.h>

#include "DrawingEngine.h"
#include "WindowLayer.h"

#include "Desktop.h"

// constructor
Desktop::Desktop(DrawView* drawView)
	: BLooper("desktop"),
	  fTracking(false),
	  fLastMousePos(-1.0, -1.0),
	  fClickedWindow(NULL),
	  fResizing(false),
	  fIs2ndButton(false),

	  fClippingLock("clipping lock"),
	  fDirtyRegion(),
	  fBackgroundRegion(),

	  fDrawView(drawView),
	  fDrawingEngine(fDrawView->GetDrawingEngine()),

	  fWindows(64),

	  fFocusFollowsMouse(true),
	  fFocusWindow(NULL)
{
	fDrawView->SetDesktop(this);

	BRegion stillAvailableOnScreen;
	_RebuildClippingForAllWindows(&stillAvailableOnScreen);
	_SetBackground(&stillAvailableOnScreen);
}

// destructor
Desktop::~Desktop()
{
	int32 count = CountWindows();
	for (int32 i = count - 1; i >= 0; i--) {
		WindowLayer* window = WindowAtFast(i);
		window->Lock();
		window->Quit();
	}
}

// Draw
void
Desktop::Draw(BRect updateRect)
{
#if !RUN_WITH_FRAME_BUFFER
	// since parts of the view might have been exposed,
	// we need a clipping rebuild
	if (LockClipping()) {
		BRegion background;
		_RebuildClippingForAllWindows(&background);
		_SetBackground(&background);

		UnlockClipping();
	}
#endif

	if (fDrawingEngine->Lock()) {
		fDrawingEngine->SetHighColor(51, 102, 152);
		fDrawingEngine->FillRegion(&fBackgroundRegion);
		fDrawingEngine->Unlock();
	}
	
#if !RUN_WITH_FRAME_BUFFER
	// trigger redrawing windows
	BRegion update(updateRect);
	update.Exclude(&fBackgroundRegion);
	MarkDirty(&update);
#endif
}

// MouseDown
void
Desktop::MouseDown(BPoint where, uint32 buttons)
{
	fLastMousePos = where;
	fClickedWindow = WindowAt(where);
	fClickTime = system_time();
	if (buttons == B_PRIMARY_MOUSE_BUTTON) {
		fTracking = true;
		if (fClickedWindow) {
			BRect frame(fClickedWindow->Frame());
			BRect resizeRect(frame.right - 10, frame.bottom - 10,
							 frame.right + 4, frame.bottom + 4);
			fResizing = resizeRect.Contains(where);
		}
	} else if (buttons == B_SECONDARY_MOUSE_BUTTON) {
		if (fClickedWindow)
			SendToBack(fClickedWindow);

		fIs2ndButton = true;
	} else if (buttons == B_TERTIARY_MOUSE_BUTTON) {
		if (modifiers() & B_SHIFT_KEY) {
			// render global dirty region
			if (fDrawingEngine->Lock()) {
				fDrawingEngine->SetHighColor(255, 0, 0);
				fDrawingEngine->FillRegion(&fDirtyRegion);
				fDrawingEngine->MarkDirty(&fDirtyRegion);
				fDrawingEngine->Unlock();
			}
		} else {
			// complete redraw
#if RUN_WITH_FRAME_BUFFER
			if (fDrawingEngine->Lock()) {
				fDirtyRegion.MakeEmpty();

				BRegion region(fDrawingEngine->Bounds());
				fDrawingEngine->Unlock();

				MarkDirty(&region);
				region = fBackgroundRegion;
				fBackgroundRegion.MakeEmpty();
				_SetBackground(&region);
			}
#else
			fDirtyRegion.MakeEmpty();
			fDrawingEngine->MarkDirty();
#endif
		}
	}
}

// MouseUp
void
Desktop::MouseUp(BPoint where)
{
	if (!fIs2ndButton && system_time() - fClickTime < 250000L && fClickedWindow) {
		BringToFront(fClickedWindow);
	}
	fTracking = false;
	fIs2ndButton = false;
	fClickedWindow = NULL;
}

// MouseMoved
void
Desktop::MouseMoved(BPoint where, uint32 code, const BMessage* dragMessage)
{
	WindowLayer* window;
	if (!fTracking && fFocusFollowsMouse && (window = WindowAt(where))) {
		SetFocusWindow(window);
	}
	if (fTracking) {
		int32 dx = (int32)(where.x - fLastMousePos.x);
		int32 dy = (int32)(where.y - fLastMousePos.y);
		fLastMousePos = where;

		if (dx != 0 || dy != 0) {
			if (fClickedWindow) {
//bigtime_t now = system_time();
				if (fResizing) {
					ResizeWindowBy(fClickedWindow, dx, dy);
//printf("resizing: %lld\n", system_time() - now);
				} else {
					MoveWindowBy(fClickedWindow, dx, dy);
//printf("moving: %lld\n", system_time() - now);
				}
			}
		}
	} else if (fIs2ndButton) {
		if (fDrawingEngine->Lock()) {
			fDrawingEngine->SetHighColor(0, 0, 0);
			fDrawingEngine->StrokeLine(fLastMousePos, where);

			BRect dirty(fLastMousePos, where);
			if (dirty.left > dirty.right) {
				dirty.left = where.x;
				dirty.right = fLastMousePos.x;
			} 
			if (dirty.top > dirty.bottom) {
				dirty.top = where.y;
				dirty.bottom = fLastMousePos.y;
			} 
			fDrawingEngine->MarkDirty(dirty);

			fDrawingEngine->Unlock();
		}
		fLastMousePos = where;
	}
}

// MessageReceived
void
Desktop::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_MOUSE_DOWN: {
			BPoint where;
			uint32 buttons;
			if (message->FindPoint("where", &where) >= B_OK &&
				message->FindInt32("buttons", (int32*)&buttons) >= B_OK) {

				MouseDown(where, buttons);
			}
			break;
		}
		case B_MOUSE_UP: {
			BPoint where;
			if (message->FindPoint("where", &where) >= B_OK) {

				MouseUp(where);
			}
			break;
		}
		case B_MOUSE_MOVED: {
			if (!MessageQueue()->FindMessage(B_MOUSE_MOVED, 0)) {
				BPoint where;
				uint32 transit;
				if (message->FindPoint("where", &where) >= B_OK &&
					message->FindInt32("be:transit", (int32*)&transit) >= B_OK) {
	
					MouseMoved(where, transit, NULL);
				}
			}
			break;
		}
		case MSG_DRAW: {
			BRect area;
			if (message->FindRect("area", &area) >= B_OK)
				Draw(area);
		}
		case MSG_ADD_WINDOW: {
			WindowLayer* window;
			if (message->FindPointer("window", (void**)&window) >= B_OK)
				AddWindow(window);
			break;
		}
		default:
			BLooper::MessageReceived(message);
	}
}

#pragma mark -

// AddWindow
bool
Desktop::AddWindow(WindowLayer* window)
{
	bool success = false;
	if (fWindows.AddItem((void*)window)) {
		// rebuild the entire screen clipping and draw the new window
		if (LockClipping()) {
			BRegion background;
			_RebuildClippingForAllWindows(&background);
			fBackgroundRegion.Exclude(&window->VisibleRegion());
			MarkDirty(&window->VisibleRegion());
			_SetBackground(&background);

			UnlockClipping();
		}
		SetFocusWindow(window);

		success = true;
	}
	return success;
}

// RemoveWindow
bool
Desktop::RemoveWindow(WindowLayer* window)
{
	bool success = false;
	if (fWindows.RemoveItem((void*)window)) {
		// rebuild the entire screen clipping and redraw the exposed windows
		if (LockClipping()) {
			BRegion dirty = window->VisibleRegion();
			BRegion background;
			_RebuildClippingForAllWindows(&background);
			MarkDirty(&dirty);
			_SetBackground(&background);

			UnlockClipping();
		}
		success = true;
	}
	return success;
}

// IndexOf
int32
Desktop::IndexOf(WindowLayer* window) const
{
	return fWindows.IndexOf((void*)window);
}

// CountWindows
int32
Desktop::CountWindows() const
{
	return fWindows.CountItems();
}

// HasWindow
bool
Desktop::HasWindow(WindowLayer* window) const
{
	return fWindows.HasItem((void*)window);
}

// WindowAt
WindowLayer*
Desktop::WindowAt(int32 index) const
{
	return (WindowLayer*)fWindows.ItemAt(index);
}

// WindowAtFast
WindowLayer*
Desktop::WindowAtFast(int32 index) const
{
	return (WindowLayer*)fWindows.ItemAtFast(index);
}

// WindowAt
WindowLayer*
Desktop::WindowAt(const BPoint& where) const
{
	// NOTE, since the clipping is only changed from this thread,
	// it is save to use it without locking
	int32 count = CountWindows();
	for (int32 i = count - 1; i >= 0; i--) {
		WindowLayer* window = WindowAtFast(i);
		if (window->VisibleRegion().Contains(where))
			return window;
	}
	return NULL;
}

// TopWindow
WindowLayer*
Desktop::TopWindow() const
{
	return (WindowLayer*)fWindows.LastItem();
}

// BottomWindow
WindowLayer*
Desktop::BottomWindow() const
{
	return (WindowLayer*)fWindows.FirstItem();
}

#pragma mark -

// MoveWindowBy
void
Desktop::MoveWindowBy(WindowLayer* window, int32 x, int32 y)
{
	if (!Lock())
		return;

	if (LockClipping()) {
		// the dirty region starts with the visible area of the window being moved
		BRegion newDirtyRegion(window->VisibleRegion());
		// we have to move along the part of the current dirty region
		// that intersects with the window being moved
		BRegion alreadyDirtyRegion(fDirtyRegion);
		alreadyDirtyRegion.IntersectWith(&window->VisibleRegion());

		window->MoveBy(x, y);

		BRegion background;
		_RebuildClippingForAllWindows(&background);
	
		// construct the region that is possible to be blitted
		// to move the contents of the window
		BRegion copyRegion(window->VisibleRegion());
		copyRegion.OffsetBy(-x, -y);
		copyRegion.IntersectWith(&newDirtyRegion);

		// include the the new visible region of the window being
		// moved into the dirty region (for now)
		newDirtyRegion.Include(&window->VisibleRegion());

		if (fDrawingEngine->Lock()) {
			fDrawingEngine->CopyRegion(&copyRegion, x, y);
	
			// in the dirty region, exclude the parts that we
			// could move by blitting
			copyRegion.OffsetBy(x, y);
			newDirtyRegion.Exclude(&copyRegion);
			MarkClean(&copyRegion);
			fDrawingEngine->MarkDirty(&copyRegion);
	
			fDrawingEngine->Unlock();
		}
		// include the moved peviously dirty region
		alreadyDirtyRegion.OffsetBy(x, y);
		alreadyDirtyRegion.IntersectWith(&window->VisibleRegion());
		newDirtyRegion.Include(&alreadyDirtyRegion);

		MarkDirty(&newDirtyRegion);
		_SetBackground(&background);
	
		UnlockClipping();
	}

	Unlock();
}

// ResizeWindowBy
void
Desktop::ResizeWindowBy(WindowLayer* window, int32 x, int32 y)
{
	if (!Lock())
		return;

	if (LockClipping()) {
		BRegion newDirtyRegion;
		BRegion previouslyOccupiedRegion(window->VisibleRegion());
		
		window->ResizeBy(x, y, &newDirtyRegion);

		BRegion background;
		_RebuildClippingForAllWindows(&background);

		previouslyOccupiedRegion.Exclude(&window->VisibleRegion());

		newDirtyRegion.IntersectWith(&window->VisibleRegion());
		newDirtyRegion.Include(&previouslyOccupiedRegion);

		MarkDirty(&newDirtyRegion);
		_SetBackground(&background);
	
		UnlockClipping();
	}

	Unlock();
}

// BringToFront
void
Desktop::BringToFront(WindowLayer* window)
{
	if (window == TopWindow())
		return;

	if (LockClipping()) {

		// we don't need to redraw what is currently
		// visible of the window
		BRegion clean(window->VisibleRegion());

		// detach window and re-atach at last position
		if (fWindows.RemoveItem((void*)window) &&
			fWindows.AddItem((void*)window)) {

			BRegion dummy;
			_RebuildClippingForAllWindows(&dummy);

			// redraw what became visible of the window
			BRegion dirty(window->VisibleRegion());
			dirty.Exclude(&clean);

			MarkDirty(&dirty);
		}

		UnlockClipping();
	}

	if (!fFocusFollowsMouse)
		SetFocusWindow(TopWindow());
}

// SendToBack
void
Desktop::SendToBack(WindowLayer* window)
{
	if (window == BottomWindow())
		return;

	if (LockClipping()) {

		// what is currently visible of the window
		// might be dirty after the window is send to back
		BRegion dirty(window->VisibleRegion());

		// detach window and re-atach at last position
		if (fWindows.RemoveItem((void*)window) &&
			fWindows.AddItem((void*)window, 0)) {

			BRegion dummy;
			_RebuildClippingForAllWindows(&dummy);

			// redraw what was previously visible of the window
			BRegion clean(window->VisibleRegion());
			dirty.Exclude(&clean);

			MarkDirty(&dirty);
		}

		UnlockClipping();
	}

	if (!fFocusFollowsMouse)
		SetFocusWindow(TopWindow());
}

// SetFocusWindow
void
Desktop::SetFocusWindow(WindowLayer* window)
{
	// TODO: find bug, this invalidates too many regions

	if (fFocusWindow == window)
		return;

	if (fFocusWindow)
		fFocusWindow->SetFocus(false);

	fFocusWindow = window;

	if (fFocusWindow)
		fFocusWindow->SetFocus(true);
}



#pragma mark -

// MarkDirty
void
Desktop::MarkDirty(BRegion* region)
{
	if (region->CountRects() == 0)
		return;
		
	// NOTE: the idea is that for all dirty areas ever included
	// in the culmulative dirty region, redraw messages have been
	// sent to the windows affected by just the newly included
	// area. Therefor, _TriggerWindowRedrawing() is not called
	// with "fDirtyRegion", but with just the new "region" instead.
	// Whenever a window is actually carrying out a redraw request,
	// it is expected to remove the redrawn area from the dirty region.

	if (LockClipping()) {
		// add the new dirty region to the culmulative dirty region
		fDirtyRegion.Include(region);

#if SHOW_GLOBAL_DIRTY_REGION
if (fDrawingEngine->Lock()) {
	fDrawingEngine->SetHighColor(255, 0, 0);
	fDrawingEngine->FillRegion(region);
	fDrawingEngine->MarkDirty(region);
	fDrawingEngine->Unlock();
	snooze(100000);
}
#endif

		// send redraw messages to all windows intersecting the dirty region
		_TriggerWindowRedrawing(region);

		UnlockClipping();
	}
}

// MarkClean
void
Desktop::MarkClean(BRegion* region)
{
	if (LockClipping()) {
		// remove the clean region from the culmulative dirty region
		fDirtyRegion.Exclude(region);

		UnlockClipping();
	}
}

#pragma mark -

// _RebuildClippingForAllWindows
void
Desktop::_RebuildClippingForAllWindows(BRegion* stillAvailableOnScreen)
{
	// the available region on screen starts with the entire screen area
	// each window on the screen will take a portion from that area

	// figure out what the entire screen area is
	if (!fDrawView->Window())
		stillAvailableOnScreen->Set(fDrawView->Bounds());
	else {
		if (fDrawView->Window()->Lock()) {
			fDrawView->GetClippingRegion(stillAvailableOnScreen);
			fDrawView->Window()->Unlock();
		}
	}

	// set clipping of each window
	int32 count = CountWindows();
	for (int32 i = count - 1; i >= 0; i--) {
		WindowLayer* window = WindowAtFast(i);
		window->SetClipping(stillAvailableOnScreen);
		// that windows region is not available on screen anymore
		stillAvailableOnScreen->Exclude(&window->VisibleRegion());
	}
}

// _TriggerWindowRedrawing
void
Desktop::_TriggerWindowRedrawing(BRegion* newDirtyRegion)
{
	// send redraw messages to all windows intersecting the dirty region
	int32 count = CountWindows();
	for (int32 i = count - 1; i >= 0; i--) {
		WindowLayer* window = WindowAtFast(i);
		if (newDirtyRegion->Intersects(window->VisibleRegion().Frame()))
			window->PostMessage(MSG_REDRAW);
	}
}

// _SetBackground
void
Desktop::_SetBackground(BRegion* background)
{
	// NOTE: the drawing operation is caried out
	// in the clipping region rebuild, but it is
	// ok actually, because it also avoids trails on
	// moving windows

	// remember the region not covered by any windows
	// and redraw the dirty background 
	BRegion dirtyBackground(*background);
	dirtyBackground.Exclude(&fBackgroundRegion);
	dirtyBackground.IntersectWith(background);
	fBackgroundRegion = *background;
	if (dirtyBackground.Frame().IsValid()) {
		if (fDrawingEngine->Lock()) {
			fDrawingEngine->SetHighColor(51, 102, 152);
			fDrawingEngine->FillRegion(&dirtyBackground);
			fDrawingEngine->MarkDirty(&dirtyBackground);
	
			fDrawingEngine->Unlock();
		}
		MarkClean(&dirtyBackground);
	}
}
