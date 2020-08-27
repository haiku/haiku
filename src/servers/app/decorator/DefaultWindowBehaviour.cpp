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

#include "ClickTarget.h"
#include "Desktop.h"
#include "DefaultDecorator.h"
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

	virtual void ExitState(State* nextState)
	{
	}

	virtual bool MouseDown(BMessage* message, BPoint where, bool& _unhandled)
	{
		return true;
	}

	virtual void MouseUp(BMessage* message, BPoint where)
	{
	}

	virtual void MouseMoved(BMessage* message, BPoint where, bool isFake)
	{
	}

	virtual void ModifiersChanged(BPoint where, int32 modifiers)
	{
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
				&& system_time() - fLastMoveTime < kWindowActivationTimeout) {
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

	virtual bool MouseDown(BMessage* message, BPoint where, bool& _unhandled)
	{
		// right-click while dragging shall bring the window to front
		int32 buttons = message->FindInt32("buttons");
		if ((buttons & B_SECONDARY_MOUSE_BUTTON) != 0) {
			if (fWindow == fDesktop->BackWindow())
				fDesktop->ActivateWindow(fWindow);
			else
				fDesktop->SendWindowBehind(fWindow);
			return true;
		}

		return MouseTrackingState::MouseDown(message, where, _unhandled);
	}

	virtual void MouseMovedAction(BPoint& delta, bigtime_t now)
	{
		if ((fWindow->Flags() & B_NOT_MOVABLE) == 0) {
			BPoint oldLeftTop = fWindow->Frame().LeftTop();

			fBehavior.AlterDeltaForSnap(fWindow, delta, now);
			fDesktop->MoveWindowBy(fWindow, delta.x, delta.y);

			// constrain delta to true change in position
			delta = fWindow->Frame().LeftTop() - oldLeftTop;
		} else
			delta = BPoint(0, 0);
	}
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
		if ((fWindow->Flags() & B_NOT_RESIZABLE) == 0) {
			if ((fWindow->Flags() & B_NOT_V_RESIZABLE) != 0)
				delta.y = 0;
			if ((fWindow->Flags() & B_NOT_H_RESIZABLE) != 0)
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

	virtual
	~SlideTabState()
	{
		fDesktop->SetWindowTabLocation(fWindow, fWindow->TabLocation(), false);
	}

	virtual void MouseMovedAction(BPoint& delta, bigtime_t now)
	{
		float location = fWindow->TabLocation();
		// TODO: change to [0:1]
		location += delta.x;
		AdjustMultiTabLocation(location, true);
		if (fDesktop->SetWindowTabLocation(fWindow, location, true))
			delta.y = 0;
		else
			delta = BPoint(0, 0);
	}

	void AdjustMultiTabLocation(float location, bool isShifting)
	{
		::Decorator* decorator = fWindow->Decorator();
		if (decorator == NULL || decorator->CountTabs() <= 1)
			return;

		// TODO does not work for none continuous shifts
		int32 windowIndex = fWindow->PositionInStack();
		DefaultDecorator::Tab*	movingTab = static_cast<DefaultDecorator::Tab*>(
			decorator->TabAt(windowIndex));
		int32 neighbourIndex = windowIndex;
		if (movingTab->tabOffset > location)
			neighbourIndex--;
		else
			neighbourIndex++;

		DefaultDecorator::Tab* neighbourTab
			= static_cast<DefaultDecorator::Tab*>(decorator->TabAt(
				neighbourIndex));
		if (neighbourTab == NULL)
			return;

		if (movingTab->tabOffset > location) {
			if (location > neighbourTab->tabOffset
					+ neighbourTab->tabRect.Width() / 2) {
				return;
			}
		} else {
			if (location + movingTab->tabRect.Width() < neighbourTab->tabOffset
					+ neighbourTab->tabRect.Width() / 2) {
				return;
			}
		}

		fWindow->MoveToStackPosition(neighbourIndex, isShifting);
	}
};


// #pragma mark - ResizeBorderState


struct DefaultWindowBehaviour::ResizeBorderState : MouseTrackingState {
	ResizeBorderState(DefaultWindowBehaviour& behavior, BPoint where,
		Decorator::Region region)
		:
		MouseTrackingState(behavior, where, true, false,
			B_SECONDARY_MOUSE_BUTTON),
		fHorizontal(NONE),
		fVertical(NONE)
	{
		switch (region) {
			case Decorator::REGION_TAB:
				// TODO: Handle like the border it is attached to (top/left)?
				break;
			case Decorator::REGION_LEFT_BORDER:
				fHorizontal = LEFT;
				break;
			case Decorator::REGION_RIGHT_BORDER:
				fHorizontal = RIGHT;
				break;
			case Decorator::REGION_TOP_BORDER:
				fVertical = TOP;
				break;
			case Decorator::REGION_BOTTOM_BORDER:
				fVertical = BOTTOM;
				break;
			case Decorator::REGION_LEFT_TOP_CORNER:
				fHorizontal = LEFT;
				fVertical = TOP;
				break;
			case Decorator::REGION_LEFT_BOTTOM_CORNER:
				fHorizontal = LEFT;
				fVertical = BOTTOM;
				break;
			case Decorator::REGION_RIGHT_TOP_CORNER:
				fHorizontal = RIGHT;
				fVertical = TOP;
				break;
			case Decorator::REGION_RIGHT_BOTTOM_CORNER:
				fHorizontal = RIGHT;
				fVertical = BOTTOM;
				break;
			default:
				break;
		}
	}

	ResizeBorderState(DefaultWindowBehaviour& behavior, BPoint where,
		int8 horizontal, int8 vertical)
		:
		MouseTrackingState(behavior, where, true, false,
			B_SECONDARY_MOUSE_BUTTON),
		fHorizontal(horizontal),
		fVertical(vertical)
	{
	}

	virtual void EnterState(State* previousState)
	{
		if ((fWindow->Flags() & B_NOT_RESIZABLE) != 0)
			fHorizontal = fVertical = NONE;
		else {
			if ((fWindow->Flags() & B_NOT_H_RESIZABLE) != 0)
				fHorizontal = NONE;

			if ((fWindow->Flags() & B_NOT_V_RESIZABLE) != 0)
				fVertical = NONE;
		}
		fBehavior._SetResizeCursor(fHorizontal, fVertical);
	}

	virtual void ExitState(State* nextState)
	{
		fBehavior._ResetResizeCursor();
	}

	virtual void MouseMovedAction(BPoint& delta, bigtime_t now)
	{
		if (fHorizontal == NONE)
			delta.x = 0;
		if (fVertical == NONE)
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
	int8	fHorizontal;
	int8	fVertical;
};


// #pragma mark - DecoratorButtonState


struct DefaultWindowBehaviour::DecoratorButtonState : State {
	DecoratorButtonState(DefaultWindowBehaviour& behavior,
		int32 tab, Decorator::Region button)
		:
		State(behavior),
		fTab(tab),
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

			int32 tab;
			switch (fButton) {
				case Decorator::REGION_CLOSE_BUTTON:
					decorator->SetClose(fTab, false);
					if (fBehavior._RegionFor(message, tab) == fButton)
						fWindow->ServerWindow()->NotifyQuitRequested();
					break;

				case Decorator::REGION_ZOOM_BUTTON:
					decorator->SetZoom(fTab, false);
					if (fBehavior._RegionFor(message, tab) == fButton)
						fWindow->ServerWindow()->NotifyZoom();
					break;

				case Decorator::REGION_MINIMIZE_BUTTON:
					decorator->SetMinimize(fTab, false);
					if (fBehavior._RegionFor(message, tab) == fButton)
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

			int32 tab;
			Decorator::Region hitRegion = message != NULL
				? fBehavior._RegionFor(message, tab) : fButton;

			switch (fButton) {
				case Decorator::REGION_CLOSE_BUTTON:
					decorator->SetClose(fTab, hitRegion == fButton);
					break;

				case Decorator::REGION_ZOOM_BUTTON:
					decorator->SetZoom(fTab, hitRegion == fButton);
					break;

				case Decorator::REGION_MINIMIZE_BUTTON:
					decorator->SetMinimize(fTab, hitRegion == fButton);
					break;

				default:
					break;
			}

			engine->UnlockParallelAccess();
			fWindow->RegionPool()->Recycle(visibleBorder);
		}
	}

protected:
	int32				fTab;
	Decorator::Region	fButton;
};


// #pragma mark - ManageWindowState


struct DefaultWindowBehaviour::ManageWindowState : State {
	ManageWindowState(DefaultWindowBehaviour& behavior, BPoint where)
		:
		State(behavior),
		fLastMousePosition(where),
		fHorizontal(NONE),
		fVertical(NONE)
	{
	}

	virtual void EnterState(State* previousState)
	{
		_UpdateBorders(fLastMousePosition);
	}

	virtual void ExitState(State* nextState)
	{
		fBehavior._SetBorderHighlights(fHorizontal, fVertical, false);
	}

	virtual bool MouseDown(BMessage* message, BPoint where, bool& _unhandled)
	{
		// We're only interested if the secondary mouse button was pressed,
		// otherwise let the caller handle the event.
		int32 buttons = message->FindInt32("buttons");
		if ((buttons & B_SECONDARY_MOUSE_BUTTON) == 0) {
			_unhandled = true;
			return true;
		}

		fBehavior._NextState(new (std::nothrow) ResizeBorderState(fBehavior,
			where, fHorizontal, fVertical));
		return true;
	}

	virtual void MouseMoved(BMessage* message, BPoint where, bool isFake)
	{
		// If the mouse is still over our window, update the borders. Otherwise
		// leave the state.
		if (fDesktop->WindowAt(where) == fWindow) {
			fLastMousePosition = where;
			_UpdateBorders(fLastMousePosition);
		} else
			fBehavior._NextState(NULL);
	}

	virtual void ModifiersChanged(BPoint where, int32 modifiers)
	{
		if (!fBehavior._IsWindowModifier(modifiers))
			fBehavior._NextState(NULL);
	}

private:
	void _UpdateBorders(BPoint where)
	{
		if ((fWindow->Flags() & B_NOT_RESIZABLE) != 0)
			return;

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

		// compute the resize direction
		int8 horizontal;
		int8 vertical;
		_ComputeResizeDirection(x, y, horizontal, vertical);

		if ((fWindow->Flags() & B_NOT_H_RESIZABLE) != 0)
			horizontal = NONE;
		if ((fWindow->Flags() & B_NOT_V_RESIZABLE) != 0)
			vertical = NONE;

		// update the highlight, if necessary
		if (horizontal != fHorizontal || vertical != fVertical) {
			fBehavior._SetBorderHighlights(fHorizontal, fVertical, false);
			fHorizontal = horizontal;
			fVertical = vertical;
			fBehavior._SetBorderHighlights(fHorizontal, fVertical, true);
		}
	}

private:
	BPoint	fLastMousePosition;
	int8	fHorizontal;
	int8	fVertical;
};


// #pragma mark - DefaultWindowBehaviour


DefaultWindowBehaviour::DefaultWindowBehaviour(Window* window)
	:
	fWindow(window),
	fDesktop(window->Desktop()),
	fLastModifiers(0)
{
}


DefaultWindowBehaviour::~DefaultWindowBehaviour()
{
}


bool
DefaultWindowBehaviour::MouseDown(BMessage* message, BPoint where,
	int32 lastHitRegion, int32& clickCount, int32& _hitRegion)
{
	fLastModifiers = message->FindInt32("modifiers");
	int32 buttons = message->FindInt32("buttons");

	int32 numButtons;
	if (get_mouse_type(&numButtons) == B_OK) {
		switch (numButtons) {
			case 1:
				// 1 button mouse
				if ((fLastModifiers & B_CONTROL_KEY) != 0) {
					// emulate right click by holding control
					buttons = B_SECONDARY_MOUSE_BUTTON;
					message->ReplaceInt32("buttons", buttons);
				}
				break;

			case 2:
				// TODO: 2 button mouse, pressing both buttons simultaneously
				// emulates middle click

			default:
				break;
		}
	}

	// if a state is active, let it do the job
	if (fState.Get() != NULL) {
		bool unhandled = false;
		bool result = fState->MouseDown(message, where, unhandled);
		if (!unhandled)
			return result;
	}

	// No state active yet, or it wants us to handle the event -- determine the
	// click region and decide what to do.

	Decorator* decorator = fWindow->Decorator();

	Decorator::Region hitRegion = Decorator::REGION_NONE;
	int32 tab = -1;
	Action action = ACTION_NONE;

	bool inBorderRegion = false;
	if (decorator != NULL)
		inBorderRegion = decorator->GetFootprint().Contains(where);

	bool windowModifier = _IsWindowModifier(fLastModifiers);

	if (windowModifier || inBorderRegion) {
		// click on the window decorator or we have the window modifier keys
		// held

		// get the functional hit region
		if (windowModifier) {
			// click with window modifier keys -- let the whole window behave
			// like the border
			hitRegion = Decorator::REGION_LEFT_BORDER;
		} else {
			// click on the decorator -- get the exact region
			hitRegion = _RegionFor(message, tab);
		}

		// translate the region into an action
		uint32 flags = fWindow->Flags();

		if ((buttons & B_PRIMARY_MOUSE_BUTTON) != 0) {
			// left mouse button
			switch (hitRegion) {
				case Decorator::REGION_TAB: {
					// tab sliding in any case if either shift key is held down
					// except sliding up-down by moving mouse left-right would
					// look strange
					if ((fLastModifiers & B_SHIFT_KEY) != 0
						&& fWindow->Look() != kLeftTitledWindowLook) {
						action = ACTION_SLIDE_TAB;
						break;
					}
					action = ACTION_DRAG;
					break;
				}

				case Decorator::REGION_LEFT_BORDER:
				case Decorator::REGION_RIGHT_BORDER:
				case Decorator::REGION_TOP_BORDER:
				case Decorator::REGION_BOTTOM_BORDER:
					action = ACTION_DRAG;
					break;

				case Decorator::REGION_CLOSE_BUTTON:
					action = (flags & B_NOT_CLOSABLE) == 0
						? ACTION_CLOSE : ACTION_DRAG;
					break;

				case Decorator::REGION_ZOOM_BUTTON:
					action = (flags & B_NOT_ZOOMABLE) == 0
						? ACTION_ZOOM : ACTION_DRAG;
					break;

				case Decorator::REGION_MINIMIZE_BUTTON:
					action = (flags & B_NOT_MINIMIZABLE) == 0
						? ACTION_MINIMIZE : ACTION_DRAG;
					break;

				case Decorator::REGION_LEFT_TOP_CORNER:
				case Decorator::REGION_LEFT_BOTTOM_CORNER:
				case Decorator::REGION_RIGHT_TOP_CORNER:
					// TODO: Handle correctly!
					action = ACTION_DRAG;
					break;

				case Decorator::REGION_RIGHT_BOTTOM_CORNER:
					action = (flags & B_NOT_RESIZABLE) == 0
						? ACTION_RESIZE : ACTION_DRAG;
					break;

				default:
					break;
			}
		} else if ((buttons & B_SECONDARY_MOUSE_BUTTON) != 0) {
			// right mouse button
			switch (hitRegion) {
				case Decorator::REGION_TAB:
				case Decorator::REGION_LEFT_BORDER:
				case Decorator::REGION_RIGHT_BORDER:
				case Decorator::REGION_TOP_BORDER:
				case Decorator::REGION_BOTTOM_BORDER:
				case Decorator::REGION_CLOSE_BUTTON:
				case Decorator::REGION_ZOOM_BUTTON:
				case Decorator::REGION_MINIMIZE_BUTTON:
				case Decorator::REGION_LEFT_TOP_CORNER:
				case Decorator::REGION_LEFT_BOTTOM_CORNER:
				case Decorator::REGION_RIGHT_TOP_CORNER:
				case Decorator::REGION_RIGHT_BOTTOM_CORNER:
					action = ACTION_RESIZE_BORDER;
					break;

				default:
					break;
			}
		}
	}

	_hitRegion = (int32)hitRegion;

	if (action == ACTION_NONE) {
		// No action -- if this is a click inside the window's contents,
		// let it be forwarded to the window.
		return inBorderRegion;
	}

	// reset the click count, if the hit region differs from the previous one
	if (hitRegion != lastHitRegion)
		clickCount = 1;

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
				new (std::nothrow) DecoratorButtonState(*this, tab, hitRegion));
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
			_NextState(new (std::nothrow) ResizeBorderState(*this, where,
				hitRegion));
			STRACE_CLICK(("===> ACTION_RESIZE_BORDER\n"));
			break;

		default:
			break;
	}

	return true;
}


void
DefaultWindowBehaviour::MouseUp(BMessage* message, BPoint where)
{
	if (fState.Get() != NULL)
		fState->MouseUp(message, where);
}


void
DefaultWindowBehaviour::MouseMoved(BMessage* message, BPoint where, bool isFake)
{
	if (fState.Get() != NULL) {
		fState->MouseMoved(message, where, isFake);
	} else {
		// If the window modifiers are hold, enter the window management state.
		if (_IsWindowModifier(message->FindInt32("modifiers")))
			_NextState(new(std::nothrow) ManageWindowState(*this, where));
	}

	// change focus in FFM mode
	DesktopSettings desktopSettings(fDesktop);
	if (desktopSettings.FocusFollowsMouse()
		&& !fWindow->IsFocus() && (fWindow->Flags() & B_AVOID_FOCUS) == 0) {
		// If the mouse move is a fake one, we set the focus to NULL, which
		// will cause the window that had focus last to retrieve it again - this
		// makes FFM much nicer to use with the keyboard.
		fDesktop->SetFocusWindow(isFake ? NULL : fWindow);
	}
}


void
DefaultWindowBehaviour::ModifiersChanged(int32 modifiers)
{
	BPoint where;
	int32 buttons;
	fDesktop->GetLastMouseState(&where, &buttons);

	if (fState.Get() != NULL) {
		fState->ModifiersChanged(where, modifiers);
	} else {
		// If the window modifiers are hold, enter the window management state.
		if (_IsWindowModifier(modifiers))
			_NextState(new(std::nothrow) ManageWindowState(*this, where));
	}
}


bool
DefaultWindowBehaviour::AlterDeltaForSnap(Window* window, BPoint& delta,
	bigtime_t now)
{
	return fMagneticBorder.AlterDeltaForSnap(window, delta, now);
}


bool
DefaultWindowBehaviour::_IsWindowModifier(int32 modifiers) const
{
	return (fWindow->Flags() & B_NO_SERVER_SIDE_WINDOW_MODIFIERS) == 0
		&& (modifiers & (B_COMMAND_KEY | B_CONTROL_KEY | B_OPTION_KEY
				| B_SHIFT_KEY)) == (B_COMMAND_KEY | B_CONTROL_KEY);
}


Decorator::Region
DefaultWindowBehaviour::_RegionFor(const BMessage* message, int32& tab) const
{
	Decorator* decorator = fWindow->Decorator();
	if (decorator == NULL)
		return Decorator::REGION_NONE;

	BPoint where;
	if (message->FindPoint("where", &where) != B_OK)
		return Decorator::REGION_NONE;

	return decorator->RegionAt(where, tab);
}


void
DefaultWindowBehaviour::_SetBorderHighlights(int8 horizontal, int8 vertical,
	bool active)
{
	if (Decorator* decorator = fWindow->Decorator()) {
		uint8 highlight = active
			? Decorator::HIGHLIGHT_RESIZE_BORDER
			: Decorator::HIGHLIGHT_NONE;

		// set the highlights for the borders
		BRegion dirtyRegion;
		switch (horizontal) {
			case LEFT:
				decorator->SetRegionHighlight(Decorator::REGION_LEFT_BORDER,
					highlight, &dirtyRegion);
				break;
			case RIGHT:
				decorator->SetRegionHighlight(
					Decorator::REGION_RIGHT_BORDER, highlight,
					&dirtyRegion);
				break;
		}

		switch (vertical) {
			case TOP:
				decorator->SetRegionHighlight(Decorator::REGION_TOP_BORDER,
					highlight, &dirtyRegion);
				break;
			case BOTTOM:
				decorator->SetRegionHighlight(
					Decorator::REGION_BOTTOM_BORDER, highlight,
					&dirtyRegion);
				break;
		}

		// set the highlights for the corners
		if (horizontal != NONE && vertical != NONE) {
			if (horizontal == LEFT) {
				if (vertical == TOP) {
					decorator->SetRegionHighlight(
						Decorator::REGION_LEFT_TOP_CORNER, highlight,
						&dirtyRegion);
				} else {
					decorator->SetRegionHighlight(
						Decorator::REGION_LEFT_BOTTOM_CORNER, highlight,
						&dirtyRegion);
				}
			} else {
				if (vertical == TOP) {
					decorator->SetRegionHighlight(
						Decorator::REGION_RIGHT_TOP_CORNER, highlight,
						&dirtyRegion);
				} else {
					decorator->SetRegionHighlight(
						Decorator::REGION_RIGHT_BOTTOM_CORNER, highlight,
						&dirtyRegion);
				}
			}
		}

		// invalidate the affected regions
		fWindow->ProcessDirtyRegion(dirtyRegion);
	}
}


ServerCursor*
DefaultWindowBehaviour::_ResizeCursorFor(int8 horizontal, int8 vertical)
{
	// get the cursor ID corresponding to the border/corner
	BCursorID cursorID = B_CURSOR_ID_SYSTEM_DEFAULT;

	if (horizontal == LEFT) {
		if (vertical == TOP)
			cursorID = B_CURSOR_ID_RESIZE_NORTH_WEST;
		else if (vertical == BOTTOM)
			cursorID = B_CURSOR_ID_RESIZE_SOUTH_WEST;
		else
			cursorID = B_CURSOR_ID_RESIZE_WEST;
	} else if (horizontal == RIGHT) {
		if (vertical == TOP)
			cursorID = B_CURSOR_ID_RESIZE_NORTH_EAST;
		else if (vertical == BOTTOM)
			cursorID = B_CURSOR_ID_RESIZE_SOUTH_EAST;
		else
			cursorID = B_CURSOR_ID_RESIZE_EAST;
	} else {
		if (vertical == TOP)
			cursorID = B_CURSOR_ID_RESIZE_NORTH;
		else if (vertical == BOTTOM)
			cursorID = B_CURSOR_ID_RESIZE_SOUTH;
	}

	return fDesktop->GetCursorManager().GetCursor(cursorID);
}


void
DefaultWindowBehaviour::_SetResizeCursor(int8 horizontal, int8 vertical)
{
	fDesktop->SetManagementCursor(_ResizeCursorFor(horizontal, vertical));
}


void
DefaultWindowBehaviour::_ResetResizeCursor()
{
	fDesktop->SetManagementCursor(NULL);
}


/*static*/ void
DefaultWindowBehaviour::_ComputeResizeDirection(float x, float y,
	int8& _horizontal, int8& _vertical)
{
	_horizontal = NONE;
	_vertical = NONE;

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
			_horizontal = RIGHT;
			break;
		case 1:
			_horizontal = RIGHT;
			_vertical = BOTTOM;
			break;
		case 2:
			_vertical = BOTTOM;
			break;
		case 3:
			_horizontal = LEFT;
			_vertical = BOTTOM;
			break;
		case 4:
			_horizontal = LEFT;
			break;
		case 5:
			_horizontal = LEFT;
			_vertical = TOP;
			break;
		case 6:
			_vertical = TOP;
			break;
		case 7:
		default:
			_horizontal = RIGHT;
			_vertical = TOP;
			break;
	}
}


void
DefaultWindowBehaviour::_NextState(State* state)
{
	// exit the old state
	if (fState.Get() != NULL)
		fState->ExitState(state);

	// set and enter the new state
	ObjectDeleter<State> oldState(fState.Detach());
	fState.SetTo(state);

	if (fState.Get() != NULL) {
		fState->EnterState(oldState.Get());
		fDesktop->SetMouseEventWindow(fWindow);
	} else if (oldState.Get() != NULL) {
		// no state anymore -- reset the mouse event window, if it's still us
		if (fDesktop->MouseEventWindow() == fWindow)
			fDesktop->SetMouseEventWindow(NULL);
	}
}
