
#include <stdio.h>

#include <Application.h>
#include <Message.h>
#include <MessageQueue.h>
#include <Messenger.h>
#include <Window.h>

#include "DrawingEngine.h"
#include "DrawView.h"
#include "WindowLayer.h"

#include "Desktop.h"

// constructor
Desktop::Desktop(DrawView* drawView, DrawingEngine* engine)
	: BLooper("desktop", B_URGENT_DISPLAY_PRIORITY),
	  fTracking(false),
	  fLastMousePos(-1.0, -1.0),
	  fClickedWindow(NULL),
	  fScrollingView(NULL),
	  fResizing(false),
	  fIs2ndButton(false),

	  fClippingLock("clipping lock"),
	  fBackgroundRegion(),

	  fMasterClipping(),
	  fXOffset(0),
	  fYOffset(0),

	  fDrawView(drawView),
	  fDrawingEngine(engine),

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
}

// MouseDown
void
Desktop::MouseDown(BPoint where, uint32 buttons, int32 clicks)
{
	fLastMousePos = where;
	fClickedWindow = WindowAt(where);
	fClickTime = system_time();
	if (buttons == B_PRIMARY_MOUSE_BUTTON) {
		fTracking = true;
		if (fClickedWindow) {
			if (modifiers() & B_SHIFT_KEY) {
				fScrollingView = fClickedWindow->ViewAt(where);
			} else if (clicks >= 2) {
				HideWindow(fClickedWindow);
				fClickedWindow = NULL;
			} else {
				BRect frame(fClickedWindow->Frame());
				BRect resizeRect(frame.right - 10, frame.bottom - 10,
								 frame.right + 4, frame.bottom + 4);
				fResizing = resizeRect.Contains(where);
			}
		}
	} else if (buttons == B_SECONDARY_MOUSE_BUTTON) {
		if (fClickedWindow)
			SendToBack(fClickedWindow);

		fIs2ndButton = true;
	} else if (buttons == B_TERTIARY_MOUSE_BUTTON) {
		fDrawView->Invalidate();
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
	fScrollingView = NULL;
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
				if (fScrollingView) {
					if (LockClipping()) {
						fClickedWindow->ScrollViewBy(fScrollingView, -dx, -dy);
						UnlockClipping();
					}
				} else if (fResizing) {
//bigtime_t now = system_time();
					ResizeWindowBy(fClickedWindow, dx, dy);
//printf("resizing: %lld\n", system_time() - now);
				} else {
					MoveWindowBy(fClickedWindow, dx, dy);
//printf("moving: %lld\n", system_time() - now);
				}
			}
		}
	} else if (fIs2ndButton) {
		fDrawingEngine->StrokeLine(fLastMousePos, where, (rgb_color){ 0, 0, 0, 255 });
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
			int32 clicks;
			if (message->FindPoint("where", &where) >= B_OK &&
				message->FindInt32("buttons", (int32*)&buttons) >= B_OK &&
				message->FindInt32("clicks", &clicks) >= B_OK) {

				where.x += fXOffset;
				where.y += fYOffset;

				MouseDown(where, buttons, clicks);
			}
			break;
		}
		case B_MOUSE_UP: {
			BPoint where;
			if (message->FindPoint("where", &where) >= B_OK) {

				where.x += fXOffset;
				where.y += fYOffset;

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

					where.x += fXOffset;
					where.y += fYOffset;
	
					MouseMoved(where, transit, NULL);
				}
			}
			break;
		}

		case MSG_ADD_WINDOW: {
			WindowLayer* window;
			if (message->FindPointer("window", (void**)&window) >= B_OK)
				AddWindow(window);
			break;
		}

		case MSG_QUIT:
			if (LockClipping()) {
				int32 count = CountWindows();
				for (int32 i = 0; i < count; i++)
					WindowAtFast(i)->PostMessage(B_QUIT_REQUESTED);
				UnlockClipping();
			}
			break;

		default:
			BLooper::MessageReceived(message);
	}
}

// #pragma mark -

// SetMasterClipping
void
Desktop::SetMasterClipping(BRegion* clipping)
{
	BRegion update = *clipping;
	update.Exclude(&fMasterClipping);

	fMasterClipping = *clipping;
	// since parts of the view might have been exposed,
	// we need a clipping rebuild
	BRegion background;
	_RebuildClippingForAllWindows(&background);
	_SetBackground(&background);

	fDrawingEngine->FillRegion(&fBackgroundRegion, (rgb_color){ 51, 102, 152, 255 });
	
	// trigger redrawing windows
	update.Exclude(&fBackgroundRegion);
	MarkDirty(&update);
}

// SetOffset
void
Desktop::SetOffset(int32 x, int32 y)
{
	fXOffset = x;
	fYOffset = y;
}

// #pragma mark -

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
		if (!window->IsHidden() && window->VisibleRegion().Contains(where))
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

		fDrawingEngine->CopyRegion(&copyRegion, x, y);

		copyRegion.OffsetBy(x, y);
		newDirtyRegion.Exclude(&copyRegion);

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

// ShowWindow
void
Desktop::ShowWindow(WindowLayer* window)
{
	SetWindowHidden(window, false);
}

// HideWindow
void
Desktop::HideWindow(WindowLayer* window)
{
	SetWindowHidden(window, true);
}

// SetWindowHidden
void
Desktop::SetWindowHidden(WindowLayer* window, bool hidden)
{
	// TODO: if in ffm mode, make sure to switch focus
	// if appropriate
	if (LockClipping()) {

		if (window->IsHidden() != hidden) {

			window->SetHidden(hidden);

			BRegion dirty;

			if (hidden) {
				// after rebuilding the clipping,
				// this window will not have a visible
				// region anymore, so we need to remember
				// it now
				// (actually that's not true, since
				// hidden windows are excluded from the
				// clipping calculation, but anyways)
				dirty = window->VisibleRegion();
			}

			BRegion background;
			_RebuildClippingForAllWindows(&background);
			_SetBackground(&background);

			if (!hidden) {
				// everything that is now visible in the
				// window needs a redraw, but other windows
				// are not affected, we can call ProcessDirtyRegion()
				// of the window, and don't have to use MarkDirty()
				dirty = window->VisibleRegion();
				window->ProcessDirtyRegion(&dirty);
			} else {
				// when the window was hidden, the dirty region
				// affects other windows
				MarkDirty(&dirty);
			}
		}

		UnlockClipping();
	}
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
	if (fFocusWindow == window)
		return;

	if (LockClipping()) {

		if (fFocusWindow)
			fFocusWindow->SetFocus(false);
	
		fFocusWindow = window;
	
		if (fFocusWindow)
			fFocusWindow->SetFocus(true);

		UnlockClipping();
	}
}



// #pragma mark -

// MarkDirty
void
Desktop::MarkDirty(BRegion* region)
{
	if (region->CountRects() == 0)
		return;
		
	if (LockClipping()) {
		// send redraw messages to all windows intersecting the dirty region
		_TriggerWindowRedrawing(region);

		UnlockClipping();
	}
}

// WindowDied
void
Desktop::WindowDied(WindowLayer* window)
{
	// thread is expected expected to have the
	// write lock!
	fWindows.RemoveItem(window);
	if (fWindows.CountItems() == 0)
		be_app->PostMessage(B_QUIT_REQUESTED);
}

// #pragma mark -

// _RebuildClippingForAllWindows
void
Desktop::_RebuildClippingForAllWindows(BRegion* stillAvailableOnScreen)
{
	// the available region on screen starts with the entire screen area
	// each window on the screen will take a portion from that area

	// figure out what the entire screen area is
	*stillAvailableOnScreen = fMasterClipping;

	// set clipping of each window
	int32 count = CountWindows();
	for (int32 i = count - 1; i >= 0; i--) {
		WindowLayer* window = WindowAtFast(i);
		if (!window->IsHidden()) {
			window->SetClipping(stillAvailableOnScreen);
			// that windows region is not available on screen anymore
			stillAvailableOnScreen->Exclude(&window->VisibleRegion());
		}
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
		if (!window->IsHidden() && newDirtyRegion->Intersects(window->VisibleRegion().Frame()))
			window->ProcessDirtyRegion(newDirtyRegion);
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
		fDrawingEngine->FillRegion(&dirtyBackground, (rgb_color){ 51, 102, 152, 255 });
	}
}
