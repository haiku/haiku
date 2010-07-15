#include "DefaultWindowBehaviour.h"

#include "Desktop.h"
#include "DrawingEngine.h"
#include "Window.h"


//#define DEBUG_WINDOW_CLICK

#ifdef DEBUG_WINDOW_CLICK
#	define STRACE_CLICK(x) printf x
#else
#	define STRACE_CLICK(x) ;
#endif


DefaultWindowBehaviour::DefaultWindowBehaviour(Window* window,
	Decorator* decorator)
	:
	fWindow(window),
	fDecorator(decorator),

	fIsClosing(false),
	fIsMinimizing(false),
	fIsZooming(false),
	fIsSlidingTab(false),
	fActivateOnMouseUp(false),

	fLastMousePosition(0.0f, 0.0f),
	fMouseMoveDistance(0.0f),
	fLastMoveTime(0),
	fLastSnapTime(0)
{
	fDesktop = fWindow->Desktop();
}


DefaultWindowBehaviour::~DefaultWindowBehaviour()
{
	
}


static const bigtime_t kWindowActivationTimeout = 500000LL;


bool
DefaultWindowBehaviour::MouseDown(BMessage* message, BPoint where)
{
	int32 modifiers = _ExtractModifiers(message);
	bool inBorderRegion = false;
	if (fWindow->Decorator())
		inBorderRegion = fWindow->Decorator()->GetFootprint().Contains(where);
	bool windowModifier =
		(fWindow->Flags() & B_NO_SERVER_SIDE_WINDOW_MODIFIERS) == 0
		&& (modifiers & (B_COMMAND_KEY | B_CONTROL_KEY | B_OPTION_KEY
			| B_SHIFT_KEY)) == (B_COMMAND_KEY | B_CONTROL_KEY);
	click_type action = CLICK_NONE;

	if (windowModifier || inBorderRegion) {
		// clicking Window visible area

		int32 buttons = _ExtractButtons(message);

		if (inBorderRegion)
			action = _ActionFor(message, buttons, modifiers);
		else {
			if ((buttons & B_SECONDARY_MOUSE_BUTTON) != 0)
				action = CLICK_MOVE_TO_BACK;
			else if ((fWindow->Flags() & B_NOT_MOVABLE) == 0
					&& fDecorator != NULL)
				action = CLICK_DRAG;
			else {
				// pass click on to the application
				windowModifier = false;
			}
		}
	}

	DesktopSettings desktopSettings(fDesktop);
	if (windowModifier || inBorderRegion) {
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

		if (fDecorator != NULL) {
			// redraw decorator
			BRegion* visibleBorder = fWindow->RegionPool()->GetRegion();
			fWindow->GetBorderRegion(visibleBorder);
			visibleBorder->IntersectWith(&fWindow->VisibleRegion());

			DrawingEngine* engine = fDecorator->GetDrawingEngine();
			engine->LockParallelAccess();
			engine->ConstrainClippingRegion(visibleBorder);

			if (fIsZooming)
				fDecorator->SetZoom(true);
			else if (fIsClosing)
				fDecorator->SetClose(true);
			else if (fIsMinimizing)
				fDecorator->SetMinimize(true);

			engine->UnlockParallelAccess();

			fWindow->RegionPool()->Recycle(visibleBorder);
		}

		if (action == CLICK_MOVE_TO_BACK) {
			if (desktopSettings.MouseMode() == B_CLICK_TO_FOCUS_MOUSE) {
				bool covered = true;
				BRegion fullRegion;
				fWindow->GetFullRegion(&fullRegion);
				if (fullRegion == fWindow->VisibleRegion()) {
    				// window is overlapped.
    				covered = false;
				}
				if (fWindow != fDesktop->FrontWindow() && covered)
					fDesktop->ActivateWindow(fWindow);
				else
					fDesktop->SendWindowBehind(fWindow);
			} else
				fDesktop->SendWindowBehind(fWindow);
		} else {
			fDesktop->SetMouseEventWindow(fWindow);

			// activate window if in click to activate mode, else only focus it
			if (desktopSettings.MouseMode() == B_NORMAL_MOUSE)
				fDesktop->ActivateWindow(fWindow);
			else {
				fDesktop->SetFocusWindow(fWindow);
				if (desktopSettings.MouseMode() == B_FOCUS_FOLLOWS_MOUSE
					&& (action == CLICK_DRAG || action == CLICK_RESIZE)) {
					fActivateOnMouseUp = true;
					fMouseMoveDistance = 0.0f;
					fLastMoveTime = system_time();
				}
			}
		}

		return true;
	}
	return false;
}


void
DefaultWindowBehaviour::MouseUp(BMessage* message, BPoint where)
{
	bool invalidate = false;
	if (fDecorator) {
		click_type action = _ActionFor(message);

		// redraw decorator
		BRegion* visibleBorder = fWindow->RegionPool()->GetRegion();
		fWindow->GetBorderRegion(visibleBorder);
		visibleBorder->IntersectWith(&fWindow->VisibleRegion());

		DrawingEngine* engine = fDecorator->GetDrawingEngine();
		engine->LockParallelAccess();
		engine->ConstrainClippingRegion(visibleBorder);

		if (fIsZooming) {
			fIsZooming = false;
			fDecorator->SetZoom(false);
			if (action == CLICK_ZOOM) {
				invalidate = true;
				fWindow->ServerWindow()->NotifyZoom();
			}
		}
		if (fIsClosing) {
			fIsClosing = false;
			fDecorator->SetClose(false);
			if (action == CLICK_CLOSE) {
				invalidate = true;
				fWindow->ServerWindow()->NotifyQuitRequested();
			}
		}
		if (fIsMinimizing) {
			fIsMinimizing = false;
			fDecorator->SetMinimize(false);
			if (action == CLICK_MINIMIZE) {
				invalidate = true;
				fWindow->ServerWindow()->NotifyMinimize(true);
			}
		}

		engine->UnlockParallelAccess();

		fWindow->RegionPool()->Recycle(visibleBorder);

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

	// in FFM mode, activate the window and bring it
	// to front in case this was a drag click but the
	// mouse was not moved
	if (fActivateOnMouseUp) {
		fActivateOnMouseUp = false;
		// on R5, there is a time window for this feature
		// ie, click and press too long, nothing will happen
		if (system_time() - fLastMoveTime < kWindowActivationTimeout)
			fDesktop->ActivateWindow(fWindow);
	}
}


void
DefaultWindowBehaviour::MouseMoved(BMessage *message, BPoint where, bool isFake)
{
	#if 0
	if (fDecorator != NULL && fWindow->TopView() != NULL) {
		DrawingEngine* engine = fDecorator->GetDrawingEngine();
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
		if (fActivateOnMouseUp) {
			if (now - fLastMoveTime >= kWindowActivationTimeout) {
				// This click is too long already for window activation.
				fActivateOnMouseUp = false;
			}
		} else
			fLastMoveTime = now;
	}

	if (fDecorator) {
		BRegion* visibleBorder = fWindow->RegionPool()->GetRegion();
		fWindow->GetBorderRegion(visibleBorder);
		visibleBorder->IntersectWith(&fWindow->VisibleRegion());

		DrawingEngine* engine = fDecorator->GetDrawingEngine();
		engine->LockParallelAccess();
		engine->ConstrainClippingRegion(visibleBorder);

		if (fIsZooming) {
			fDecorator->SetZoom(_ActionFor(message) == CLICK_ZOOM);
		} else if (fIsClosing) {
			fDecorator->SetClose(_ActionFor(message) == CLICK_CLOSE);
		} else if (fIsMinimizing) {
			fDecorator->SetMinimize(_ActionFor(message) == CLICK_MINIMIZE);
		}

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
	if (fActivateOnMouseUp) {
		fMouseMoveDistance += delta.x * delta.x + delta.y * delta.y;
		if (fMouseMoveDistance > 16.0f)
			fActivateOnMouseUp = false;
		else
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


int32
DefaultWindowBehaviour::_ExtractButtons(const BMessage* message) const
{
	int32 buttons;
	if (message->FindInt32("buttons", &buttons) != B_OK)
		buttons = 0;
	return buttons;
}


int32
DefaultWindowBehaviour::_ExtractModifiers(const BMessage* message) const
{
	int32 modifiers;
	if (message->FindInt32("modifiers", &modifiers) != B_OK)
		modifiers = 0;
	return modifiers;
}


click_type
DefaultWindowBehaviour::_ActionFor(const BMessage* message) const
{
	if (fDecorator == NULL)
		return CLICK_NONE;

	int32 buttons = _ExtractButtons(message);
	int32 modifiers = _ExtractModifiers(message);
	return _ActionFor(message, buttons, modifiers);
}


click_type
DefaultWindowBehaviour::_ActionFor(const BMessage* message, int32 buttons,
	int32 modifiers) const
{
	if (fDecorator == NULL)
		return CLICK_NONE;

	BPoint where;
	if (message->FindPoint("where", &where) != B_OK)
		return CLICK_NONE;

	return fDecorator->Clicked(where, buttons, modifiers);
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

	if (fDecorator) {
		frame = fDecorator->GetFootprint().Frame();
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
