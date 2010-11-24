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

#include <math.h>

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


// #pragma mark - State


struct DefaultWindowBehaviour::State {
	State(DefaultWindowBehaviour& behavior)
		:
		fBehavior(behavior),
		fWindow(behavior.fWindow),
		fDesktop(behavior.fDesktop)
	{
	}

	virtual ~State()
	{
	}

	virtual void EnterState(State* previousState)
	{
	}

	virtual bool MouseDown(BMessage* message, BPoint where)
	{
		return false;
	}

	virtual void MouseUp(BMessage* message, BPoint where)
	{
	}

	virtual void MouseMoved(BMessage* message, BPoint where, bool isFake)
	{
	}

	void UpdateFFMFocus(bool isFake)
	{
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

protected:
	DefaultWindowBehaviour&	fBehavior;
	Window*					fWindow;
	Desktop*				fDesktop;
};


// #pragma mark - MouseTrackingState


struct DefaultWindowBehaviour::MouseTrackingState : State {
	MouseTrackingState(DefaultWindowBehaviour& behavior, BPoint where,
		bool windowActionOnMouseUp, bool minimizeCheckOnMouseUp,
		int32 mouseButton = B_PRIMARY_MOUSE_BUTTON)
		:
		State(behavior),
		fMouseButton(mouseButton),
		fWindowActionOnMouseUp(windowActionOnMouseUp),
		fMinimizeCheckOnMouseUp(minimizeCheckOnMouseUp),
		fLastMousePosition(where),
		fMouseMoveDistance(0),
		fLastMoveTime(system_time())
	{
	}

	virtual void MouseUp(BMessage* message, BPoint where)
	{
		// ignore, if it's not our mouse button
		int32 buttons = message->FindInt32("buttons");
		if ((buttons & fMouseButton) != 0)
			return;

		if (fMinimizeCheckOnMouseUp) {
			// If the modifiers haven't changed in the meantime and not too
			// much time has elapsed, we're supposed to minimize the window.
			fMinimizeCheckOnMouseUp = false;
			if (message->FindInt32("modifiers") == fBehavior.fLastModifiers
				&& (fWindow->Flags() & B_NOT_MINIMIZABLE) == 0
				&& system_time() - fLastMoveTime
					< kWindowActivationTimeout) {
				fWindow->ServerWindow()->NotifyMinimize(true);
			}
		}

		// Perform the window action in case the mouse was not moved.
		if (fWindowActionOnMouseUp) {
			// There is a time window for this feature, i.e. click and press
			// too long, nothing will happen.
			if (system_time() - fLastMoveTime < kWindowActivationTimeout)
				MouseUpWindowAction();
		}

		fBehavior._NextState(NULL);
	}

	virtual void MouseMoved(BMessage* message, BPoint where, bool isFake)
	{
		// Limit the rate at which "mouse moved" events are handled that move
		// or resize the window. At the moment this affects also tab sliding,
		// but 1/75 s is a pretty fine granularity anyway, so don't bother.
		bigtime_t now = system_time();
		if (now - fLastMoveTime < 13333) {
			// TODO: add a "timed event" to query for
			// the then current mouse position
			return;
		}
		if (fWindowActionOnMouseUp || fMinimizeCheckOnMouseUp) {
			if (now - fLastMoveTime >= kWindowActivationTimeout) {
				// This click is too long already for window activation/
				// minimizing.
				fWindowActionOnMouseUp = false;
				fMinimizeCheckOnMouseUp = false;
				fLastMoveTime = now;
			}
		} else
			fLastMoveTime = now;

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
		if (fWindowActionOnMouseUp || fMinimizeCheckOnMouseUp) {
			fMouseMoveDistance += delta.x * delta.x + delta.y * delta.y;
			if (fMouseMoveDistance > 16.0f) {
				fWindowActionOnMouseUp = false;
				fMinimizeCheckOnMouseUp = false;
			} else
				delta = B_ORIGIN;
		}

		// perform the action (this also updates the delta)
		MouseMovedAction(delta, now);

		// set the new mouse position
		fLastMousePosition += delta;

		// update the FFM focus
		UpdateFFMFocus(isFake);
	}

	virtual void MouseMovedAction(BPoint& delta, bigtime_t now)
	{
	}

	virtual void MouseUpWindowAction()
	{
		// default is window activation
		fDesktop->ActivateWindow(fWindow);
	}

protected:
	int32				fMouseButton;
	bool				fWindowActionOnMouseUp : 1;
	bool				fMinimizeCheckOnMouseUp : 1;

	BPoint				fLastMousePosition;
	float				fMouseMoveDistance;
	bigtime_t			fLastMoveTime;
};


// #pragma mark - DragState


struct DefaultWindowBehaviour::DragState : MouseTrackingState {
	DragState(DefaultWindowBehaviour& behavior, BPoint where,
		bool activateOnMouseUp, bool minimizeCheckOnMouseUp)
		:
		MouseTrackingState(behavior, where, activateOnMouseUp,
			minimizeCheckOnMouseUp)
	{
	}

	virtual bool MouseDown(BMessage* message, BPoint where)
	{
		// right-click while dragging shall bring the window to front
		int32 buttons = message->FindInt32("buttons");
		if ((buttons & B_SECONDARY_MOUSE_BUTTON) != 0) {
			if (fWindow == fDesktop->BackWindow())
				fDesktop->ActivateWindow(fWindow);
			else
				fDesktop->SendWindowBehind(fWindow);
			return false;
		}

		return MouseTrackingState::MouseDown(message, where);
	}

	virtual void MouseMovedAction(BPoint& delta, bigtime_t now)
	{
		if (!(fWindow->Flags() & B_NOT_MOVABLE)) {
			BPoint oldLeftTop = fWindow->Frame().LeftTop();

			_AlterDeltaForSnap(delta, now);
			fDesktop->MoveWindowBy(fWindow, delta.x, delta.y);

			// constrain delta to true change in position
			delta = fWindow->Frame().LeftTop() - oldLeftTop;
		} else
			delta = BPoint(0, 0);
	}

private:
	void _AlterDeltaForSnap(BPoint& delta, bigtime_t now)
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

private:
	bigtime_t			fLastSnapTime;
};


// #pragma mark - ResizeState


struct DefaultWindowBehaviour::ResizeState : MouseTrackingState {
	ResizeState(DefaultWindowBehaviour& behavior, BPoint where,
		bool activateOnMouseUp)
		:
		MouseTrackingState(behavior, where, activateOnMouseUp, false)
	{
	}

	virtual void MouseMovedAction(BPoint& delta, bigtime_t now)
	{
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
};


// #pragma mark - SlideTabState


struct DefaultWindowBehaviour::SlideTabState : MouseTrackingState {
	SlideTabState(DefaultWindowBehaviour& behavior, BPoint where)
		:
		MouseTrackingState(behavior, where, false, false)
	{
	}

	virtual void MouseMovedAction(BPoint& delta, bigtime_t now)
	{
		float loc = fWindow->TabLocation();
		// TODO: change to [0:1]
		loc += delta.x;
		if (fDesktop->SetWindowTabLocation(fWindow, loc))
			delta.y = 0;
		else
			delta = BPoint(0, 0);
	}
};


// #pragma mark - ResizeBorderState


struct DefaultWindowBehaviour::ResizeBorderState : MouseTrackingState {
	ResizeBorderState(DefaultWindowBehaviour& behavior, BPoint where)
		:
		MouseTrackingState(behavior, where, true, false,
			B_SECONDARY_MOUSE_BUTTON),
		fHorizontal(NONE),
		fVertical(NONE)
	{
		// Compute the window center relative location of where. We divide by
		// the width respective the height, so we compensate for the window's
		// aspect ratio.
		BRect frame(fWindow->Frame());
		if (frame.Width() + 1 == 0 || frame.Height() + 1 == 0)
			return;

		float x = (where.x - (frame.left + frame.right) / 2)
			/ (frame.Width() + 1);
		float y = (where.y - (frame.top + frame.bottom) / 2)
			/ (frame.Height() + 1);

		// compute the angle
		if (x == 0 && y == 0)
			return;

		float angle = atan2f(y, x);

		// rotate by 22.5 degree to align our sectors with 45 degree multiples
		angle += M_PI / 8;

		// add 180 degree to the negative values, so we get a nice 0 to 360
		// degree range
		if (angle < 0)
			angle += M_PI * 2;

		switch (int(angle / M_PI_4)) {
			case 0:
				fHorizontal = RIGHT;
				break;
			case 1:
				fHorizontal = RIGHT;
				fVertical = BOTTOM;
				break;
			case 2:
				fVertical = BOTTOM;
				break;
			case 3:
				fHorizontal = LEFT;
				fVertical = BOTTOM;
				break;
			case 4:
				fHorizontal = LEFT;
				break;
			case 5:
				fHorizontal = LEFT;
				fVertical = TOP;
				break;
			case 6:
				fVertical = TOP;
				break;
			case 7:
			default:
				fHorizontal = RIGHT;
				fVertical = TOP;
				break;
		}
	}

	virtual void MouseMovedAction(BPoint& delta, bigtime_t now)
	{
		if ((fWindow->Flags() & B_NOT_RESIZABLE) != 0) {
			delta = BPoint(0, 0);
			return;
		}

		if ((fWindow->Flags() & B_NOT_H_RESIZABLE) != 0 || fHorizontal == NONE)
			delta.x = 0;
		if ((fWindow->Flags() & B_NOT_V_RESIZABLE) != 0 || fVertical == NONE)
			delta.y = 0;

		if (delta.x == 0 && delta.y == 0)
			return;

		// Resize first -- due to the window size limits this is not unlikely
		// to turn out differently from what we request.
		BPoint oldRightBottom = fWindow->Frame().RightBottom();

		fDesktop->ResizeWindowBy(fWindow, delta.x * fHorizontal,
			delta.y * fVertical);

		// constrain delta to true change in size
		delta = fWindow->Frame().RightBottom() - oldRightBottom;
		delta.x *= fHorizontal;
		delta.y *= fVertical;

		// see, if we have to move, too
		float moveX = fHorizontal == LEFT ? delta.x : 0;
		float moveY = fVertical == TOP ? delta.y : 0;

		if (moveX != 0 || moveY != 0)
			fDesktop->MoveWindowBy(fWindow, moveX, moveY);
	}

	virtual void MouseUpWindowAction()
	{
		fDesktop->SendWindowBehind(fWindow);
	}

private:
	enum {
		// 1 for the "natural" resize border, -1 for the opposite,
		// so multiplying the movement delta by that value results in the size
		// change.
		LEFT	= -1,
		TOP		= -1,
		NONE	= 0,
		RIGHT	= 1,
		BOTTOM	= 1
	};

private:
	int8	fHorizontal;
	int8	fVertical;
};


// #pragma mark - DecoratorButtonState


struct DefaultWindowBehaviour::DecoratorButtonState : State {
	DecoratorButtonState(DefaultWindowBehaviour& behavior, Region button)
		:
		State(behavior),
		fButton(button)
	{
	}

	virtual void EnterState(State* previousState)
	{
		_RedrawDecorator(NULL);
	}

	virtual void MouseUp(BMessage* message, BPoint where)
	{
		// ignore, if it's not the primary mouse button
		int32 buttons = message->FindInt32("buttons");
		if ((buttons & B_PRIMARY_MOUSE_BUTTON) != 0)
			return;

		// redraw the decorator
		if (Decorator* decorator = fWindow->Decorator()) {
			BRegion* visibleBorder = fWindow->RegionPool()->GetRegion();
			fWindow->GetBorderRegion(visibleBorder);
			visibleBorder->IntersectWith(&fWindow->VisibleRegion());

			DrawingEngine* engine = decorator->GetDrawingEngine();
			engine->LockParallelAccess();
			engine->ConstrainClippingRegion(visibleBorder);

			switch (fButton) {
				case REGION_CLOSE_BUTTON:
					decorator->SetClose(false);
					if (fBehavior._RegionFor(message) == REGION_CLOSE_BUTTON)
						fWindow->ServerWindow()->NotifyQuitRequested();
					break;

				case REGION_ZOOM_BUTTON:
					decorator->SetZoom(false);
					if (fBehavior._RegionFor(message) == REGION_ZOOM_BUTTON)
						fWindow->ServerWindow()->NotifyZoom();
					break;

				case REGION_MINIMIZE_BUTTON:
					decorator->SetMinimize(false);
					if (fBehavior._RegionFor(message) == REGION_MINIMIZE_BUTTON)
						fWindow->ServerWindow()->NotifyMinimize(true);
					break;

				default:
					break;
			}

			engine->UnlockParallelAccess();

			fWindow->RegionPool()->Recycle(visibleBorder);
		}

		fBehavior._NextState(NULL);
	}

	virtual void MouseMoved(BMessage* message, BPoint where, bool isFake)
	{
		_RedrawDecorator(message);

		// update the FFM focus
		UpdateFFMFocus(isFake);
	}

private:
	void _RedrawDecorator(const BMessage* message)
	{
		if (Decorator* decorator = fWindow->Decorator()) {
			BRegion* visibleBorder = fWindow->RegionPool()->GetRegion();
			fWindow->GetBorderRegion(visibleBorder);
			visibleBorder->IntersectWith(&fWindow->VisibleRegion());

			DrawingEngine* engine = decorator->GetDrawingEngine();
			engine->LockParallelAccess();
			engine->ConstrainClippingRegion(visibleBorder);

			Region hitRegion = message != NULL
				? fBehavior._RegionFor(message) : fButton;

			switch (fButton) {
				case REGION_CLOSE_BUTTON:
					decorator->SetClose(hitRegion == REGION_CLOSE_BUTTON);
					break;

				case REGION_ZOOM_BUTTON:
					decorator->SetZoom(hitRegion == REGION_ZOOM_BUTTON);
					break;

				case REGION_MINIMIZE_BUTTON:
					decorator->SetMinimize(hitRegion == REGION_MINIMIZE_BUTTON);
					break;

				default:
					break;
			}

			engine->UnlockParallelAccess();
			fWindow->RegionPool()->Recycle(visibleBorder);
		}
	}

protected:
	Region	fButton;
};


// #pragma mark - DefaultWindowBehaviour


DefaultWindowBehaviour::DefaultWindowBehaviour(Window* window)
	:
	fWindow(window),
	fDesktop(window->Desktop()),
	fState(NULL),
	fLastModifiers(0),
	fLastMouseButtons(0),
	fLastRegion(REGION_NONE),
	fResetClickCount(0)
{
}


DefaultWindowBehaviour::~DefaultWindowBehaviour()
{
	delete fState;
}


bool
DefaultWindowBehaviour::MouseDown(BMessage* message, BPoint where)
{
	// Get the click count and reset it, if the modifiers changed in the
	// meantime. Do the same when this is not the button we've seen before.
	// TODO: At least the modifier check should be done in a better place
	// (e.g. the input server). It should also reset clicks after mouse
	// movement (which we don't do here either -- though that's probably
	// acceptable).
	int32 clickCount = message->FindInt32("clicks");
	int32 modifiers = message->FindInt32("modifiers");
	int32 buttons = message->FindInt32("buttons");

	if (clickCount <= 1) {
		fResetClickCount = 0;
	} else if (modifiers != fLastModifiers || buttons != fLastMouseButtons
		|| clickCount - fResetClickCount < 1) {
		fResetClickCount = clickCount - 1;
		clickCount = 1;
	} else
		clickCount -= fResetClickCount;

	fLastModifiers = modifiers;
	fLastMouseButtons = buttons;

	// if a state is active, let it do the job
	if (fState != NULL)
		return fState->MouseDown(message, where);

	// No state active yet -- determine the click region and decide what to do.

	Decorator* decorator = fWindow->Decorator();

	Region hitRegion = REGION_NONE;
	Action action = ACTION_NONE;

	bool inBorderRegion = false;
	if (decorator != NULL)
		inBorderRegion = decorator->GetFootprint().Contains(where);

	bool windowModifier = _IsWindowModifier(modifiers);

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
					action = ACTION_SLIDE_TAB;
					break;
				}
				// otherwise fall through -- same handling as for the border...

			case REGION_BORDER:
				if (leftButton)
					action = ACTION_DRAG;
				else if (rightButton)
					action = ACTION_RESIZE_BORDER;
				break;

			case REGION_CLOSE_BUTTON:
				if (leftButton) {
					action = (flags & B_NOT_CLOSABLE) == 0
						? ACTION_CLOSE : ACTION_DRAG;
				} else if (rightButton)
					action = ACTION_RESIZE_BORDER;
				break;

			case REGION_ZOOM_BUTTON:
				if (leftButton) {
					action = (flags & B_NOT_ZOOMABLE) == 0
						? ACTION_ZOOM : ACTION_DRAG;
				} else if (rightButton)
					action = ACTION_RESIZE_BORDER;
				break;

			case REGION_MINIMIZE_BUTTON:
				if (leftButton) {
					action = (flags & B_NOT_MINIMIZABLE) == 0
						? ACTION_MINIMIZE : ACTION_DRAG;
				} else if (rightButton)
					action = ACTION_RESIZE_BORDER;
				break;

			case REGION_RESIZE_CORNER:
				if (leftButton) {
					action = (flags & B_NOT_RESIZABLE) == 0
						? ACTION_RESIZE : ACTION_DRAG;
				} else if (rightButton)
					action = ACTION_RESIZE_BORDER;
				break;
		}
	}

	// The hit region changed since the last the click. Reset the click count.
	if (hitRegion != fLastRegion) {
		fLastRegion = hitRegion;
		clickCount = 1;

		fResetClickCount = message->FindInt32("clicks") - 1;
		if (fResetClickCount < 0)
			fResetClickCount = 0;
	}

	if (action == ACTION_NONE) {
		// No action -- if this is a click inside the window's contents,
		// let it be forwarded to the window.
		return inBorderRegion;
	}

	DesktopSettings desktopSettings(fDesktop);
	if (!desktopSettings.AcceptFirstClick()) {
		// Ignore clicks on decorator buttons if the
		// non-floating window doesn't have focus
		if (!fWindow->IsFocus() && !fWindow->IsFloating()
			&& action != ACTION_RESIZE_BORDER
			&& action != ACTION_RESIZE && action != ACTION_SLIDE_TAB)
			action = ACTION_DRAG;
	}

	bool activateOnMouseUp = false;
	if (action != ACTION_RESIZE_BORDER) {
		// activate window if in click to activate mode, else only focus it
		if (desktopSettings.MouseMode() == B_NORMAL_MOUSE) {
			fDesktop->ActivateWindow(fWindow);
		} else {
			fDesktop->SetFocusWindow(fWindow);
			activateOnMouseUp = true;
		}
	}

	// switch to the new state
	switch (action) {
		case ACTION_CLOSE:
		case ACTION_ZOOM:
		case ACTION_MINIMIZE:
			_NextState(
				new (std::nothrow) DecoratorButtonState(*this, hitRegion));
			STRACE_CLICK(("===> ACTION_CLOSE/ZOOM/MINIMIZE\n"));
			break;

		case ACTION_DRAG:
			_NextState(new (std::nothrow) DragState(*this, where,
				activateOnMouseUp, clickCount == 2));
			STRACE_CLICK(("===> ACTION_DRAG\n"));
			break;

		case ACTION_RESIZE:
			_NextState(new (std::nothrow) ResizeState(*this, where,
				activateOnMouseUp));
			STRACE_CLICK(("===> ACTION_RESIZE\n"));
			break;

		case ACTION_SLIDE_TAB:
			_NextState(new (std::nothrow) SlideTabState(*this, where));
			STRACE_CLICK(("===> ACTION_SLIDE_TAB\n"));
			break;

		case ACTION_RESIZE_BORDER:
			_NextState(new (std::nothrow) ResizeBorderState(*this, where));
			STRACE_CLICK(("===> ACTION_RESIZE_BORDER\n"));
			break;

		default:
			break;
	}

	// If we have a state now, it surely wants all further mouse events.
	if (fState != NULL)
		fDesktop->SetMouseEventWindow(fWindow);

	return true;
}


void
DefaultWindowBehaviour::MouseUp(BMessage* message, BPoint where)
{
	if (fState != NULL)
		fState->MouseUp(message, where);
}


void
DefaultWindowBehaviour::MouseMoved(BMessage *message, BPoint where, bool isFake)
{
	if (fState != NULL)
		fState->MouseMoved(message, where, isFake);
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
DefaultWindowBehaviour::_NextState(State* state)
{
	State* oldState = fState;
	fState = state;

	if (fState != NULL)
		fState->EnterState(oldState);

	delete oldState;
}
