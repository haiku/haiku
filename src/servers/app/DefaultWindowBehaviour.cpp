/*
 * Copyright 2001-2010, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Adi Oanca <adioanca@gmail.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Axel Dörfler <axeld@pinc-software.de>
 *		Brecht Machiels <brecht@mos6581.org>
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include "DefaultWindowBehaviour.h"

#include <WindowPrivate.h>

#include "Desktop.h"
#include "DrawingEngine.h"
#include "Window.h"


//#define DEBUG_WINDOW_CLICK
#ifdef DEBUG_WINDOW_CLICK
#	define STRACE_CLICK(x) printf x
#else
#	define STRACE_CLICK(x) ;
#endif


// The span between mouse down
static const bigtime_t kWindowActivationTimeout = 500000LL;


DefaultWindowBehaviour::DefaultWindowBehaviour(Window* window)
	:
	fWindow(window),

	fIsClosing(false),
	fIsMinimizing(false),
	fIsZooming(false),
	fIsSlidingTab(false),
	fActivateOnMouseUp(false),
	fMinimizeCheckOnMouseUp(false),

	fLastMousePosition(0.0f, 0.0f),
	fMouseMoveDistance(0.0f),
	fLastMoveTime(0),
	fLastSnapTime(0),
	fLastModifiers(0),
	fResetClickCount(0)
{
	fDesktop = fWindow->Desktop();
}


DefaultWindowBehaviour::~DefaultWindowBehaviour()
{
}


bool
DefaultWindowBehaviour::MouseDown(BMessage* message, BPoint where)
{
	Decorator* decorator = fWindow->Decorator();

	bool inBorderRegion = false;
	if (decorator != NULL)
		inBorderRegion = decorator->GetFootprint().Contains(where);

	int32 modifiers = message->FindInt32("modifiers");
	bool windowModifier = _IsWindowModifier(modifiers);

	// Get the click count and reset it, if the modifiers changed in the
	// meantime.
	// TODO: This should be done in a better place (e.g. the input server). It
	// should also reset clicks after mouse movement (which we don't do here
	// either -- though that's probably acceptable).
	int32 clickCount = message->FindInt32("clicks");
	if (clickCount <= 1) {
		fResetClickCount = 0;
	} else if (modifiers != fLastModifiers
		|| clickCount - fResetClickCount < 1) {
		fResetClickCount = clickCount - 1;
		clickCount = 1;
	} else
		clickCount -= fResetClickCount;
	fLastModifiers = modifiers;

	Region hitRegion = REGION_NONE;
	click_type action = CLICK_NONE;

	if (windowModifier || inBorderRegion) {
		// click on the window decorator or we have the window modifier keys
		// held

		// get the functional hit region
		if (windowModifier) {
			// click with window modifier keys -- let the whole window behave
			// like the border
			hitRegion = REGION_BORDER;
		} else {
			// click on the decorator -- get the exact region
			hitRegion = _RegionFor(message);
		}

		// translate the region into an action
		int32 buttons = message->FindInt32("buttons");
		bool leftButton = (buttons & B_PRIMARY_MOUSE_BUTTON) != 0;
		bool rightButton = (buttons & B_SECONDARY_MOUSE_BUTTON) != 0;
		uint32 flags = fWindow->Flags();

		switch (hitRegion) {
			case REGION_NONE:
				break;

			case REGION_TAB:
				// tab sliding in any case if either shift key is held down
				// except sliding up-down by moving mouse left-right would look
				// strange
				if (leftButton && (modifiers & B_SHIFT_KEY) != 0
					&& fWindow->Look() != kLeftTitledWindowLook) {
					action = CLICK_SLIDE_TAB;
					break;
				}
				// otherwise fall through -- same handling as for the border...

			case REGION_BORDER:
				if (leftButton)
					action = CLICK_DRAG;
				else if (rightButton)
					action = CLICK_MOVE_TO_BACK;
				break;

			case REGION_CLOSE_BUTTON:
				if (leftButton) {
					action = (flags & B_NOT_CLOSABLE) == 0
						? CLICK_CLOSE : CLICK_DRAG;
				} else if (rightButton)
					action = CLICK_MOVE_TO_BACK;
				break;

			case REGION_ZOOM_BUTTON:
				if (leftButton) {
					action = (flags & B_NOT_ZOOMABLE) == 0
						? CLICK_ZOOM : CLICK_DRAG;
				} else if (rightButton)
					action = CLICK_MOVE_TO_BACK;
				break;

			case REGION_MINIMIZE_BUTTON:
				if (leftButton) {
					action = (flags & B_NOT_MINIMIZABLE) == 0
						? CLICK_MINIMIZE : CLICK_DRAG;
				} else if (rightButton)
					action = CLICK_MOVE_TO_BACK;
				break;

			case REGION_RESIZE_CORNER:
				if (leftButton) {
					action = (flags & B_NOT_RESIZABLE) == 0
						? CLICK_RESIZE : CLICK_DRAG;
				} else if (rightButton)
					action = CLICK_MOVE_TO_BACK;
				break;
		}
	}

	if (action == CLICK_NONE) {
		// No action -- if this is a click inside the window's contents,
		// let it be forwarded to the window.
		return inBorderRegion;
	}

	DesktopSettings desktopSettings(fDesktop);
	if (!desktopSettings.AcceptFirstClick()) {
		// Ignore clicks on decorator buttons if the
		// non-floating window doesn't have focus
		if (!fWindow->IsFocus() && !fWindow->IsFloating()
			&& action != CLICK_MOVE_TO_BACK
			&& action != CLICK_RESIZE && action != CLICK_SLIDE_TAB)
			action = CLICK_DRAG;
	}

	// set decorator internals
	switch (action) {
		case CLICK_CLOSE:
			fIsClosing = true;
			STRACE_CLICK(("===> CLICK_CLOSE\n"));
			break;

		case CLICK_ZOOM:
			fIsZooming = true;
			STRACE_CLICK(("===> CLICK_ZOOM\n"));
			break;

		case CLICK_MINIMIZE:
			if ((fWindow->Flags() & B_NOT_MINIMIZABLE) == 0) {
				fIsMinimizing = true;
				STRACE_CLICK(("===> CLICK_MINIMIZE\n"));
			}
			break;

		case CLICK_DRAG:
			fIsDragging = true;
			fLastMousePosition = where;
			STRACE_CLICK(("===> CLICK_DRAG\n"));
			break;

		case CLICK_RESIZE:
			fIsResizing = true;
			fLastMousePosition = where;
			STRACE_CLICK(("===> CLICK_RESIZE\n"));
			break;

		case CLICK_SLIDE_TAB:
			fIsSlidingTab = true;
			fLastMousePosition = where;
			STRACE_CLICK(("===> CLICK_SLIDE_TAB\n"));
			break;

		default:
			break;
	}

	if (decorator != NULL) {
		// redraw decorator
		BRegion* visibleBorder = fWindow->RegionPool()->GetRegion();
		fWindow->GetBorderRegion(visibleBorder);
		visibleBorder->IntersectWith(&fWindow->VisibleRegion());

		DrawingEngine* engine = decorator->GetDrawingEngine();
		engine->LockParallelAccess();
		engine->ConstrainClippingRegion(visibleBorder);

		if (fIsZooming)
			decorator->SetZoom(true);
		else if (fIsClosing)
			decorator->SetClose(true);
		else if (fIsMinimizing)
			decorator->SetMinimize(true);

		engine->UnlockParallelAccess();

		fWindow->RegionPool()->Recycle(visibleBorder);
	}

	if (action == CLICK_MOVE_TO_BACK) {
		if (!fIsDragging || fWindow != fDesktop->BackWindow())
			fDesktop->SendWindowBehind(fWindow);
		else
			fDesktop->ActivateWindow(fWindow);
	} else {
		fDesktop->SetMouseEventWindow(fWindow);

		// activate window if in click to activate mode, else only focus it
		if (desktopSettings.MouseMode() == B_NORMAL_MOUSE)
			fDesktop->ActivateWindow(fWindow);
		else {
			fDesktop->SetFocusWindow(fWindow);

			if (action == CLICK_DRAG || action == CLICK_RESIZE)
				fActivateOnMouseUp = true;
		}

		if (fIsDragging && clickCount == 2)
			fMinimizeCheckOnMouseUp = true;

		fMouseMoveDistance = 0.0f;
		fLastMoveTime = system_time();
	}

	return true;
}


void
DefaultWindowBehaviour::MouseUp(BMessage* message, BPoint where)
{
	Decorator* decorator = fWindow->Decorator();

	int32 buttons = message->FindInt32("buttons");

	if (decorator != NULL) {
		// redraw decorator
		BRegion* visibleBorder = fWindow->RegionPool()->GetRegion();
		fWindow->GetBorderRegion(visibleBorder);
		visibleBorder->IntersectWith(&fWindow->VisibleRegion());

		DrawingEngine* engine = decorator->GetDrawingEngine();
		engine->LockParallelAccess();
		engine->ConstrainClippingRegion(visibleBorder);

		if (fIsZooming) {
			fIsZooming = false;
			decorator->SetZoom(false);
			if (_RegionFor(message) == REGION_ZOOM_BUTTON)
				fWindow->ServerWindow()->NotifyZoom();
		}
		if (fIsClosing) {
			fIsClosing = false;
			decorator->SetClose(false);
			if (_RegionFor(message) == REGION_CLOSE_BUTTON)
				fWindow->ServerWindow()->NotifyQuitRequested();
		}
		if (fIsMinimizing) {
			fIsMinimizing = false;
			decorator->SetMinimize(false);
			if (_RegionFor(message) == REGION_MINIMIZE_BUTTON)
				fWindow->ServerWindow()->NotifyMinimize(true);
		}

		engine->UnlockParallelAccess();

		fWindow->RegionPool()->Recycle(visibleBorder);
	}

	// if the primary mouse button is released, stop
	// dragging/resizing/sliding
	if ((buttons & B_PRIMARY_MOUSE_BUTTON) == 0) {
		if (fMinimizeCheckOnMouseUp) {
			// If the modifiers haven't changed in the meantime and not too
			// much time has elapsed, we're supposed to minimize the window.
			fMinimizeCheckOnMouseUp = false;
			if (message->FindInt32("modifiers") == fLastModifiers
				&& (fWindow->Flags() & B_NOT_MINIMIZABLE) == 0
				&& system_time() - fLastMoveTime < kWindowActivationTimeout) {
				fWindow->ServerWindow()->NotifyMinimize(true);
			}
		}

		// In FFM mode, activate the window and bring it to front in case this
		// was a drag click but the mouse was not moved.
		if (fActivateOnMouseUp) {
			fActivateOnMouseUp = false;
			// on R5, there is a time window for this feature
			// ie, click and press too long, nothing will happen
			if (system_time() - fLastMoveTime < kWindowActivationTimeout)
				fDesktop->ActivateWindow(fWindow);
		}

		fIsDragging = false;
		fIsResizing = false;
		fIsSlidingTab = false;
	}
}


void
DefaultWindowBehaviour::MouseMoved(BMessage *message, BPoint where, bool isFake)
{
	Decorator* decorator = fWindow->Decorator();

	#if 0
	if (decorator != NULL && fWindow->TopView() != NULL) {
		DrawingEngine* engine = decorator->GetDrawingEngine();
		engine->LockParallelAccess();
		engine->ConstrainClippingRegion(&fWindow->VisibleRegion());

		fWindow->TopView()->MarkAt(engine, where);
		engine->UnlockParallelAccess();
	}
	#endif
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
		if (fActivateOnMouseUp || fMinimizeCheckOnMouseUp) {
			if (now - fLastMoveTime >= kWindowActivationTimeout) {
				// This click is too long already for window activation/
				// minimizing.
				fActivateOnMouseUp = false;
				fMinimizeCheckOnMouseUp = false;
			}
		} else
			fLastMoveTime = now;
	}

	if (decorator != NULL) {
		BRegion* visibleBorder = fWindow->RegionPool()->GetRegion();
		fWindow->GetBorderRegion(visibleBorder);
		visibleBorder->IntersectWith(&fWindow->VisibleRegion());

		DrawingEngine* engine = decorator->GetDrawingEngine();
		engine->LockParallelAccess();
		engine->ConstrainClippingRegion(visibleBorder);

		Region hitRegion = _RegionFor(message);

		if (fIsZooming)
			decorator->SetZoom(hitRegion == REGION_ZOOM_BUTTON);
		else if (fIsClosing)
			decorator->SetClose(hitRegion == REGION_CLOSE_BUTTON);
		else if (fIsMinimizing)
			decorator->SetMinimize(hitRegion == REGION_MINIMIZE_BUTTON);

		engine->UnlockParallelAccess();
		fWindow->RegionPool()->Recycle(visibleBorder);
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
	if (fActivateOnMouseUp || fMinimizeCheckOnMouseUp) {
		fMouseMoveDistance += delta.x * delta.x + delta.y * delta.y;
		if (fMouseMoveDistance > 16.0f) {
			fActivateOnMouseUp = false;
			fMinimizeCheckOnMouseUp = false;
		} else
			delta = B_ORIGIN;
	}

	// moving
	if (fIsDragging) {
		if (!(fWindow->Flags() & B_NOT_MOVABLE)) {
			BPoint oldLeftTop = fWindow->Frame().LeftTop();

			_AlterDeltaForSnap(delta, now);
			fDesktop->MoveWindowBy(fWindow, delta.x, delta.y);

			// constrain delta to true change in position
			delta = fWindow->Frame().LeftTop() - oldLeftTop;
		} else
			delta = BPoint(0, 0);
	}
	// resizing
	if (fIsResizing) {
		if (!(fWindow->Flags() & B_NOT_RESIZABLE)) {
			if (fWindow->Flags() & B_NOT_V_RESIZABLE)
				delta.y = 0;
			if (fWindow->Flags() & B_NOT_H_RESIZABLE)
				delta.x = 0;

			BPoint oldRightBottom = fWindow->Frame().RightBottom();

			fDesktop->ResizeWindowBy(fWindow, delta.x, delta.y);

			// constrain delta to true change in size
			delta = fWindow->Frame().RightBottom() - oldRightBottom;
		} else
			delta = BPoint(0, 0);
	}
	// sliding tab
	if (fIsSlidingTab) {
		float loc = fWindow->TabLocation();
		// TODO: change to [0:1]
		loc += delta.x;
		if (fDesktop->SetWindowTabLocation(fWindow, loc))
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
		&& !fWindow->IsFocus() && !(fWindow->Flags() & B_AVOID_FOCUS)) {
		// If the mouse move is a fake one, we set the focus to NULL, which
		// will cause the window that had focus last to retrieve it again - this
		// makes FFM much nicer to use with the keyboard.
		fDesktop->SetFocusWindow(isFake ? NULL : fWindow);
	}
}


bool
DefaultWindowBehaviour::_IsWindowModifier(int32 modifiers) const
{
	return (fWindow->Flags() & B_NO_SERVER_SIDE_WINDOW_MODIFIERS) == 0
		&& (modifiers & (B_COMMAND_KEY | B_CONTROL_KEY | B_OPTION_KEY
				| B_SHIFT_KEY)) == (B_COMMAND_KEY | B_CONTROL_KEY);
}


DefaultWindowBehaviour::Region
DefaultWindowBehaviour::_RegionFor(const BMessage* message) const
{
	Decorator* decorator = fWindow->Decorator();
	if (decorator == NULL)
		return REGION_NONE;

	BPoint where;
	if (message->FindPoint("where", &where) != B_OK)
		return REGION_NONE;

	// translate the tab region into a functional region
	switch (decorator->RegionAt(where)) {
		case Decorator::REGION_NONE:
			return REGION_NONE;

		case Decorator::REGION_TAB:
			return REGION_TAB;

		case Decorator::REGION_CLOSE_BUTTON:
			return REGION_CLOSE_BUTTON;

		case Decorator::REGION_ZOOM_BUTTON:
			return REGION_ZOOM_BUTTON;

		case Decorator::REGION_MINIMIZE_BUTTON:
			return REGION_MINIMIZE_BUTTON;

		case Decorator::REGION_LEFT_BORDER:
		case Decorator::REGION_RIGHT_BORDER:
		case Decorator::REGION_TOP_BORDER:
		case Decorator::REGION_BOTTOM_BORDER:
			return REGION_BORDER;

		case Decorator::REGION_LEFT_TOP_CORNER:
		case Decorator::REGION_LEFT_BOTTOM_CORNER:
		case Decorator::REGION_RIGHT_TOP_CORNER:
			// not supported yet
			return REGION_BORDER;

		case Decorator::REGION_RIGHT_BOTTOM_CORNER:
			return REGION_RESIZE_CORNER;

		default:
			return REGION_NONE;
	}
}


void
DefaultWindowBehaviour::_AlterDeltaForSnap(BPoint& delta, bigtime_t now)
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

	BRect frame = fWindow->Frame();
	BPoint offsetWithinFrame;
	// TODO: Perhaps obtain the usable area (not covered by the Deskbar)?
	BRect screenFrame = fWindow->Screen()->Frame();

	Decorator* decorator = fWindow->Decorator();
	if (decorator) {
		frame = decorator->GetFootprint().Frame();
		offsetWithinFrame.x = fWindow->Frame().left - frame.left;
		offsetWithinFrame.y = fWindow->Frame().top - frame.top;
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

	delta.y = frame.top - fWindow->Frame().top;
	delta.x = frame.left - fWindow->Frame().left;
}
