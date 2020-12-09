/*
 * Copyright 2001-2020 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Stephan Aßmus, superstippi@gmx.de
 *		Stefano Ceccherini, burton666@libero.it
 *		DarkWyrm, bpmagic@columbus.rr.com
 *		Marc Flerackers, mflerackers@androme.be
 *		John Scipione, jscipione@gmail.com
 */


#include <ScrollBar.h>

#include <algorithm>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ControlLook.h>
#include <LayoutUtils.h>
#include <Message.h>
#include <OS.h>
#include <Shape.h>
#include <Window.h>

#include <binary_compatibility/Interface.h>


//#define TRACE_SCROLLBAR
#ifdef TRACE_SCROLLBAR
#	define TRACE(x...) printf(x)
#else
#	define TRACE(x...)
#endif


typedef enum {
	ARROW_LEFT = 0,
	ARROW_RIGHT,
	ARROW_UP,
	ARROW_DOWN,
	ARROW_NONE
} arrow_direction;


#define SCROLL_BAR_MAXIMUM_KNOB_SIZE	50
#define SCROLL_BAR_MINIMUM_KNOB_SIZE	9

#define SBC_SCROLLBYVALUE	0
#define SBC_SETDOUBLE		1
#define SBC_SETPROPORTIONAL	2
#define SBC_SETSTYLE		3

// Quick constants for determining which arrow is down and are defined with
// respect to double arrow mode. ARROW1 and ARROW4 refer to the outer pair of
// arrows and ARROW2 and ARROW3 refer to the inner ones. ARROW1 points left/up
// and ARROW4 points right/down.
#define ARROW1	0
#define ARROW2	1
#define ARROW3	2
#define ARROW4	3
#define THUMB	4
#define NOARROW	-1


static const bigtime_t kRepeatDelay = 300000;


// Because the R5 version kept a lot of data on server-side, we need to kludge
// our way into binary compatibility
class BScrollBar::Private {
public:
	Private(BScrollBar* scrollBar)
	:
	fScrollBar(scrollBar),
	fEnabled(true),
	fRepeaterThread(-1),
	fExitRepeater(false),
	fRepeaterDelay(0),
	fThumbFrame(0.0, 0.0, -1.0, -1.0),
	fDoRepeat(false),
	fClickOffset(0.0, 0.0),
	fThumbInc(0.0),
	fStopValue(0.0),
	fUpArrowsEnabled(true),
	fDownArrowsEnabled(true),
	fBorderHighlighted(false),
	fButtonDown(NOARROW)
	{
#ifdef TEST_MODE
		fScrollBarInfo.proportional = true;
		fScrollBarInfo.double_arrows = true;
		fScrollBarInfo.knob = 0;
		fScrollBarInfo.min_knob_size = 15;
#else
		get_scroll_bar_info(&fScrollBarInfo);
#endif

		fScrollBarInfo.min_knob_size = (int32)(fScrollBarInfo.min_knob_size *
				(be_plain_font->Size() / 12.0f));
	}

	~Private()
	{
		if (fRepeaterThread >= 0) {
			status_t dummy;
			fExitRepeater = true;
			wait_for_thread(fRepeaterThread, &dummy);
		}
	}

	void DrawScrollBarButton(BScrollBar* owner, arrow_direction direction,
		BRect frame, bool down = false);

	static int32 button_repeater_thread(void* data);

	int32 ButtonRepeaterThread();

	BScrollBar*			fScrollBar;
	bool				fEnabled;

	// TODO: This should be a static, initialized by
	// _init_interface_kit() at application startup-time,
	// like BMenu::sMenuInfo
	scroll_bar_info		fScrollBarInfo;

	thread_id			fRepeaterThread;
	volatile bool		fExitRepeater;
	bigtime_t			fRepeaterDelay;

	BRect				fThumbFrame;
	volatile bool		fDoRepeat;
	BPoint				fClickOffset;

	float				fThumbInc;
	float				fStopValue;

	bool				fUpArrowsEnabled;
	bool				fDownArrowsEnabled;

	bool				fBorderHighlighted;

	int8				fButtonDown;
};


// This thread is spawned when a button is initially pushed and repeatedly scrolls
// the scrollbar by a little bit after a short delay
int32
BScrollBar::Private::button_repeater_thread(void* data)
{
	BScrollBar::Private* privateData = (BScrollBar::Private*)data;
	return privateData->ButtonRepeaterThread();
}


int32
BScrollBar::Private::ButtonRepeaterThread()
{
	// Wait a bit before auto scrolling starts. As long as the user releases
	// and presses the button again while the repeat delay has not yet
	// triggered, the value is pushed into the future, so we need to loop such
	// that repeating starts at exactly the correct delay after the last
	// button press.
	while (fRepeaterDelay > system_time() && !fExitRepeater)
		snooze_until(fRepeaterDelay, B_SYSTEM_TIMEBASE);

	// repeat loop
	while (!fExitRepeater) {
		if (fScrollBar->LockLooper()) {
			if (fDoRepeat) {
				float value = fScrollBar->Value() + fThumbInc;
				if (fButtonDown == NOARROW) {
					// in this case we want to stop when we're under the mouse
					if (fThumbInc > 0.0 && value <= fStopValue)
						fScrollBar->SetValue(value);
					if (fThumbInc < 0.0 && value >= fStopValue)
						fScrollBar->SetValue(value);
				} else
					fScrollBar->SetValue(value);
			}

			fScrollBar->UnlockLooper();
		}

		snooze(25000);
	}

	// tell scrollbar we're gone
	if (fScrollBar->LockLooper()) {
		fRepeaterThread = -1;
		fScrollBar->UnlockLooper();
	}

	return 0;
}


//	#pragma mark - BScrollBar


BScrollBar::BScrollBar(BRect frame, const char* name, BView* target,
	float min, float max, orientation direction)
	:
	BView(frame, name, B_FOLLOW_NONE,
		B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_FRAME_EVENTS),
	fMin(min),
	fMax(max),
	fSmallStep(1.0f),
	fLargeStep(10.0f),
	fValue(0),
	fProportion(0.0f),
	fTarget(NULL),
	fOrientation(direction)
{
	SetViewColor(B_TRANSPARENT_COLOR);

	fPrivateData = new BScrollBar::Private(this);

	SetTarget(target);
	SetEventMask(B_NO_POINTER_HISTORY);

	_UpdateThumbFrame();
	_UpdateArrowButtons();

	SetResizingMode(direction == B_VERTICAL
		? B_FOLLOW_TOP_BOTTOM | B_FOLLOW_RIGHT
		: B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM);
}


BScrollBar::BScrollBar(const char* name, BView* target,
	float min, float max, orientation direction)
	:
	BView(name, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_FRAME_EVENTS),
	fMin(min),
	fMax(max),
	fSmallStep(1.0f),
	fLargeStep(10.0f),
	fValue(0),
	fProportion(0.0f),
	fTarget(NULL),
	fOrientation(direction)
{
	SetViewColor(B_TRANSPARENT_COLOR);

	fPrivateData = new BScrollBar::Private(this);

	SetTarget(target);
	SetEventMask(B_NO_POINTER_HISTORY);

	_UpdateThumbFrame();
	_UpdateArrowButtons();
}


BScrollBar::BScrollBar(BMessage* data)
	:
	BView(data),
	fTarget(NULL)
{
	fPrivateData = new BScrollBar::Private(this);

	// TODO: Does the BeOS implementation try to find the target
	// by name again? Does it archive the name at all?
	if (data->FindFloat("_range", 0, &fMin) < B_OK)
		fMin = 0.0f;

	if (data->FindFloat("_range", 1, &fMax) < B_OK)
		fMax = 0.0f;

	if (data->FindFloat("_steps", 0, &fSmallStep) < B_OK)
		fSmallStep = 1.0f;

	if (data->FindFloat("_steps", 1, &fLargeStep) < B_OK)
		fLargeStep = 10.0f;

	if (data->FindFloat("_val", &fValue) < B_OK)
		fValue = 0.0;

	int32 orientation;
	if (data->FindInt32("_orient", &orientation) < B_OK) {
		fOrientation = B_VERTICAL;
	} else
		fOrientation = (enum orientation)orientation;

	if ((Flags() & B_SUPPORTS_LAYOUT) == 0) {
		// just to make sure
		SetResizingMode(fOrientation == B_VERTICAL
			? B_FOLLOW_TOP_BOTTOM | B_FOLLOW_RIGHT
			: B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM);
	}

	if (data->FindFloat("_prop", &fProportion) < B_OK)
		fProportion = 0.0;

	_UpdateThumbFrame();
	_UpdateArrowButtons();
}


BScrollBar::~BScrollBar()
{
	SetTarget((BView*)NULL);
	delete fPrivateData;
}


BArchivable*
BScrollBar::Instantiate(BMessage* data)
{
	if (validate_instantiation(data, "BScrollBar"))
		return new BScrollBar(data);
	return NULL;
}


status_t
BScrollBar::Archive(BMessage* data, bool deep) const
{
	status_t err = BView::Archive(data, deep);
	if (err != B_OK)
		return err;

	err = data->AddFloat("_range", fMin);
	if (err != B_OK)
		return err;

	err = data->AddFloat("_range", fMax);
	if (err != B_OK)
		return err;

	err = data->AddFloat("_steps", fSmallStep);
	if (err != B_OK)
		return err;

	err = data->AddFloat("_steps", fLargeStep);
	if (err != B_OK)
		return err;

	err = data->AddFloat("_val", fValue);
	if (err != B_OK)
		return err;

	err = data->AddInt32("_orient", (int32)fOrientation);
	if (err != B_OK)
		return err;

	err = data->AddFloat("_prop", fProportion);

	return err;
}


void
BScrollBar::AllAttached()
{
	BView::AllAttached();
}


void
BScrollBar::AllDetached()
{
	BView::AllDetached();
}


void
BScrollBar::AttachedToWindow()
{
	BView::AttachedToWindow();
}


void
BScrollBar::DetachedFromWindow()
{
	BView::DetachedFromWindow();
}


void
BScrollBar::Draw(BRect updateRect)
{
	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);

	uint32 flags = 0;
	bool scrollingEnabled = fMin < fMax
		&& fProportion >= 0.0f && fProportion < 1.0f;
	if (scrollingEnabled)
		flags |= BControlLook::B_PARTIALLY_ACTIVATED;

	if (!fPrivateData->fEnabled || !scrollingEnabled)
		flags |= BControlLook::B_DISABLED;

	bool isFocused = fPrivateData->fBorderHighlighted;
	if (isFocused)
		flags |= BControlLook::B_FOCUSED;

	bool doubleArrows = _DoubleArrows();

	// draw border
	BRect rect = Bounds();
	be_control_look->DrawScrollBarBorder(this, rect, updateRect, base, flags,
		fOrientation);

	// inset past border
	rect.InsetBy(1, 1);

	// draw arrows buttons
	BRect thumbBG = rect;
	if (fOrientation == B_HORIZONTAL) {
		BRect buttonFrame(rect.left, rect.top,
			rect.left + rect.Height(), rect.bottom);

		be_control_look->DrawScrollBarButton(this, buttonFrame, updateRect,
			base, flags | (fPrivateData->fButtonDown == ARROW1
				? BControlLook::B_ACTIVATED : 0),
			BControlLook::B_LEFT_ARROW, fOrientation,
			fPrivateData->fButtonDown == ARROW1);

		if (doubleArrows) {
			buttonFrame.OffsetBy(rect.Height() + 1, 0.0f);
			be_control_look->DrawScrollBarButton(this, buttonFrame, updateRect,
				base, flags | (fPrivateData->fButtonDown == ARROW2
					? BControlLook::B_ACTIVATED : 0),
				BControlLook::B_RIGHT_ARROW, fOrientation,
				fPrivateData->fButtonDown == ARROW2);

			buttonFrame.OffsetTo(rect.right - ((rect.Height() * 2) + 1),
				rect.top);
			be_control_look->DrawScrollBarButton(this, buttonFrame, updateRect,
				base, flags | (fPrivateData->fButtonDown == ARROW3
					? BControlLook::B_ACTIVATED : 0),
				BControlLook::B_LEFT_ARROW, fOrientation,
				fPrivateData->fButtonDown == ARROW3);

			thumbBG.left += rect.Height() * 2 + 2;
			thumbBG.right -= rect.Height() * 2 + 2;
		} else {
			thumbBG.left += rect.Height() + 1;
			thumbBG.right -= rect.Height() + 1;
		}

		buttonFrame.OffsetTo(rect.right - rect.Height(), rect.top);
		be_control_look->DrawScrollBarButton(this, buttonFrame, updateRect,
			base, flags | (fPrivateData->fButtonDown == ARROW4
				? BControlLook::B_ACTIVATED : 0),
			BControlLook::B_RIGHT_ARROW, fOrientation,
			fPrivateData->fButtonDown == ARROW4);
	} else {
		BRect buttonFrame(rect.left, rect.top, rect.right,
			rect.top + rect.Width());

		be_control_look->DrawScrollBarButton(this, buttonFrame, updateRect,
			base, flags | (fPrivateData->fButtonDown == ARROW1
				? BControlLook::B_ACTIVATED : 0),
			BControlLook::B_UP_ARROW, fOrientation,
			fPrivateData->fButtonDown == ARROW1);

		if (doubleArrows) {
			buttonFrame.OffsetBy(0, rect.Width() + 1);
			be_control_look->DrawScrollBarButton(this, buttonFrame,
				updateRect, base, flags | (fPrivateData->fButtonDown == ARROW2
					? BControlLook::B_ACTIVATED : 0),
				BControlLook::B_DOWN_ARROW, fOrientation,
				fPrivateData->fButtonDown == ARROW2);

			buttonFrame.OffsetTo(rect.left, rect.bottom
				- ((rect.Width() * 2) + 1));
			be_control_look->DrawScrollBarButton(this, buttonFrame,
				updateRect, base, flags | (fPrivateData->fButtonDown == ARROW3
					? BControlLook::B_ACTIVATED : 0),
				BControlLook::B_UP_ARROW, fOrientation,
				fPrivateData->fButtonDown == ARROW3);

			thumbBG.top += rect.Width() * 2 + 2;
			thumbBG.bottom -= rect.Width() * 2 + 2;
		} else {
			thumbBG.top += rect.Width() + 1;
			thumbBG.bottom -= rect.Width() + 1;
		}

		buttonFrame.OffsetTo(rect.left, rect.bottom - rect.Width());
		be_control_look->DrawScrollBarButton(this, buttonFrame, updateRect,
			base, flags | (fPrivateData->fButtonDown == ARROW4
				? BControlLook::B_ACTIVATED : 0),
			BControlLook::B_DOWN_ARROW, fOrientation,
			fPrivateData->fButtonDown == ARROW4);
	}

	// fill background besides the thumb
	rect = fPrivateData->fThumbFrame;
	if (fOrientation == B_HORIZONTAL) {
		BRect leftOfThumb(thumbBG.left, thumbBG.top,
			rect.left - 1, thumbBG.bottom);
		BRect rightOfThumb(rect.right + 1, thumbBG.top,
			thumbBG.right, thumbBG.bottom);
		be_control_look->DrawScrollBarBackground(this, leftOfThumb,
			rightOfThumb, updateRect, base, flags, fOrientation);
	} else {
		BRect topOfThumb(thumbBG.left, thumbBG.top,
			thumbBG.right, rect.top - 1);
		BRect bottomOfThumb(thumbBG.left, rect.bottom + 1,
			thumbBG.right, thumbBG.bottom);
		be_control_look->DrawScrollBarBackground(this, topOfThumb,
			bottomOfThumb, updateRect, base, flags, fOrientation);
	}

	// draw thumb
	rect = fPrivateData->fThumbFrame;
	be_control_look->DrawScrollBarThumb(this, rect, updateRect,
		ui_color(B_SCROLL_BAR_THUMB_COLOR), flags, fOrientation,
		fPrivateData->fScrollBarInfo.knob);
}


void
BScrollBar::FrameMoved(BPoint newPosition)
{
	BView::FrameMoved(newPosition);
}


void
BScrollBar::FrameResized(float newWidth, float newHeight)
{
	_UpdateThumbFrame();
}


void
BScrollBar::MessageReceived(BMessage* message)
{
	switch(message->what) {
		case B_VALUE_CHANGED:
		{
			int32 value;
			if (message->FindInt32("value", &value) == B_OK)
				ValueChanged(value);

			break;
		}

		case B_MOUSE_WHEEL_CHANGED:
		{
			// Must handle this here since BView checks for the existence of
			// scrollbars, which a scrollbar itself does not have
			float deltaX = 0.0f;
			float deltaY = 0.0f;
			message->FindFloat("be:wheel_delta_x", &deltaX);
			message->FindFloat("be:wheel_delta_y", &deltaY);

			if (deltaX == 0.0f && deltaY == 0.0f)
				break;

			if (deltaX != 0.0f && deltaY == 0.0f)
				deltaY = deltaX;

			ScrollWithMouseWheelDelta(this, deltaY);
		}

		default:
			BView::MessageReceived(message);
	}
}


void
BScrollBar::MouseDown(BPoint where)
{
	if (!fPrivateData->fEnabled || fMin == fMax)
		return;

	SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);

	int32 buttons;
	if (Looper() == NULL || Looper()->CurrentMessage() == NULL
		|| Looper()->CurrentMessage()->FindInt32("buttons", &buttons) != B_OK) {
		buttons = B_PRIMARY_MOUSE_BUTTON;
	}

	if (buttons & B_SECONDARY_MOUSE_BUTTON) {
		// special absolute scrolling: move thumb to where we clicked
		fPrivateData->fButtonDown = THUMB;
		fPrivateData->fClickOffset
			= fPrivateData->fThumbFrame.LeftTop() - where;
		if (Orientation() == B_HORIZONTAL) {
			fPrivateData->fClickOffset.x
				= -fPrivateData->fThumbFrame.Width() / 2;
		} else {
			fPrivateData->fClickOffset.y
				= -fPrivateData->fThumbFrame.Height() / 2;
		}

		SetValue(_ValueFor(where + fPrivateData->fClickOffset));
		return;
	}

	// hit test for the thumb
	if (fPrivateData->fThumbFrame.Contains(where)) {
		fPrivateData->fButtonDown = THUMB;
		fPrivateData->fClickOffset
			= fPrivateData->fThumbFrame.LeftTop() - where;
		Invalidate(fPrivateData->fThumbFrame);
		return;
	}

	// hit test for arrows or empty area
	float scrollValue = 0.0;

	// pressing the shift key scrolls faster
	float buttonStepSize
		= (modifiers() & B_SHIFT_KEY) != 0 ? fLargeStep : fSmallStep;

	fPrivateData->fButtonDown = _ButtonFor(where);
	switch (fPrivateData->fButtonDown) {
		case ARROW1:
			scrollValue = -buttonStepSize;
			break;

		case ARROW2:
			scrollValue = buttonStepSize;
			break;

		case ARROW3:
			scrollValue = -buttonStepSize;
			break;

		case ARROW4:
			scrollValue = buttonStepSize;
			break;

		case NOARROW:
			// we hit the empty area, figure out which side of the thumb
			if (fOrientation == B_VERTICAL) {
				if (where.y < fPrivateData->fThumbFrame.top)
					scrollValue = -fLargeStep;
				else
					scrollValue = fLargeStep;
			} else {
				if (where.x < fPrivateData->fThumbFrame.left)
					scrollValue = -fLargeStep;
				else
					scrollValue = fLargeStep;
			}
			_UpdateTargetValue(where);
			break;
	}
	if (scrollValue != 0.0) {
		SetValue(fValue + scrollValue);
		Invalidate(_ButtonRectFor(fPrivateData->fButtonDown));

		// launch the repeat thread
		if (fPrivateData->fRepeaterThread == -1) {
			fPrivateData->fExitRepeater = false;
			fPrivateData->fRepeaterDelay = system_time() + kRepeatDelay;
			fPrivateData->fThumbInc = scrollValue;
			fPrivateData->fDoRepeat = true;
			fPrivateData->fRepeaterThread = spawn_thread(
				fPrivateData->button_repeater_thread, "scroll repeater",
				B_NORMAL_PRIORITY, fPrivateData);
			resume_thread(fPrivateData->fRepeaterThread);
		} else {
			fPrivateData->fExitRepeater = false;
			fPrivateData->fRepeaterDelay = system_time() + kRepeatDelay;
			fPrivateData->fDoRepeat = true;
		}
	}
}


void
BScrollBar::MouseMoved(BPoint where, uint32 code, const BMessage* dragMessage)
{
	if (!fPrivateData->fEnabled || fMin >= fMax || fProportion >= 1.0f
		|| fProportion < 0.0f) {
		return;
	}

	if (fPrivateData->fButtonDown != NOARROW) {
		if (fPrivateData->fButtonDown == THUMB) {
			SetValue(_ValueFor(where + fPrivateData->fClickOffset));
		} else {
			// suspend the repeating if the mouse is not over the button
			bool repeat = _ButtonRectFor(fPrivateData->fButtonDown).Contains(
				where);
			if (fPrivateData->fDoRepeat != repeat) {
				fPrivateData->fDoRepeat = repeat;
				Invalidate(_ButtonRectFor(fPrivateData->fButtonDown));
			}
		}
	} else {
		// update the value at which we want to stop repeating
		if (fPrivateData->fDoRepeat) {
			_UpdateTargetValue(where);
			// we might have to turn arround
			if ((fValue < fPrivateData->fStopValue
					&& fPrivateData->fThumbInc < 0)
				|| (fValue > fPrivateData->fStopValue
					&& fPrivateData->fThumbInc > 0)) {
				fPrivateData->fThumbInc = -fPrivateData->fThumbInc;
			}
		}
	}
}


void
BScrollBar::MouseUp(BPoint where)
{
	if (fPrivateData->fButtonDown == THUMB)
		Invalidate(fPrivateData->fThumbFrame);
	else
		Invalidate(_ButtonRectFor(fPrivateData->fButtonDown));

	fPrivateData->fButtonDown = NOARROW;
	fPrivateData->fExitRepeater = true;
	fPrivateData->fDoRepeat = false;
}


void
BScrollBar::WindowActivated(bool active)
{
	fPrivateData->fEnabled = active;
	Invalidate();
}


void
BScrollBar::SetValue(float value)
{
	if (value > fMax)
		value = fMax;
	else if (value < fMin)
		value = fMin;
	else if (isnan(value) || isinf(value))
		return;

	value = roundf(value);
	if (value == fValue)
		return;

	TRACE("BScrollBar(%s)::SetValue(%.1f)\n", Name(), value);

	fValue = value;

	_UpdateThumbFrame();
	_UpdateArrowButtons();

	ValueChanged(fValue);
}


float
BScrollBar::Value() const
{
	return fValue;
}


void
BScrollBar::ValueChanged(float newValue)
{
	TRACE("BScrollBar(%s)::ValueChanged(%.1f)\n", Name(), newValue);

	if (fTarget != NULL) {
		// cache target bounds
		BRect targetBounds = fTarget->Bounds();
		// if vertical, check bounds top and scroll if different from newValue
		if (fOrientation == B_VERTICAL && targetBounds.top != newValue)
			fTarget->ScrollBy(0.0, newValue - targetBounds.top);

		// if horizontal, check bounds left and scroll if different from newValue
		if (fOrientation == B_HORIZONTAL && targetBounds.left != newValue)
			fTarget->ScrollBy(newValue - targetBounds.left, 0.0);
	}

	TRACE(" -> %.1f\n", newValue);

	SetValue(newValue);
}


void
BScrollBar::SetProportion(float value)
{
	if (value < 0.0f)
		value = 0.0f;
	else if (value > 1.0f)
		value = 1.0f;

	if (value == fProportion)
		return;

	TRACE("BScrollBar(%s)::SetProportion(%.1f)\n", Name(), value);

	bool oldEnabled = fPrivateData->fEnabled && fMin < fMax
		&& fProportion < 1.0f && fProportion >= 0.0f;

	fProportion = value;

	bool newEnabled = fPrivateData->fEnabled && fMin < fMax
		&& fProportion < 1.0f && fProportion >= 0.0f;

	_UpdateThumbFrame();

	if (oldEnabled != newEnabled)
		Invalidate();
}


float
BScrollBar::Proportion() const
{
	return fProportion;
}


void
BScrollBar::SetRange(float min, float max)
{
	if (min > max || isnanf(min) || isnanf(max)
		|| isinff(min) || isinff(max)) {
		min = 0.0f;
		max = 0.0f;
	}

	min = roundf(min);
	max = roundf(max);

	if (fMin == min && fMax == max)
		return;

	TRACE("BScrollBar(%s)::SetRange(min=%.1f, max=%.1f)\n", Name(), min, max);

	fMin = min;
	fMax = max;

	if (fValue < fMin || fValue > fMax)
		SetValue(fValue);
	else {
		_UpdateThumbFrame();
		Invalidate();
	}
}


void
BScrollBar::GetRange(float* min, float* max) const
{
	if (min != NULL)
		*min = fMin;

	if (max != NULL)
		*max = fMax;
}


void
BScrollBar::SetSteps(float smallStep, float largeStep)
{
	// Under R5, steps can be set only after being attached to a window,
	// probably because the data is kept server-side. We'll just remove
	// that limitation... :P

	// The BeBook also says that we need to specify an integer value even
	// though the step values are floats. For the moment, we'll just make
	// sure that they are integers
	smallStep = roundf(smallStep);
	largeStep = roundf(largeStep);
	if (fSmallStep == smallStep && fLargeStep == largeStep)
		return;

	TRACE("BScrollBar(%s)::SetSteps(small=%.1f, large=%.1f)\n", Name(),
		smallStep, largeStep);

	fSmallStep = smallStep;
	fLargeStep = largeStep;

	if (fProportion == 0.0) {
		// special case, proportion is based on fLargeStep if it was never
		// set, so it means we need to invalidate here
		_UpdateThumbFrame();
		Invalidate();
	}

	// TODO: test use of fractional values and make them work properly if
	// they don't
}


void
BScrollBar::GetSteps(float* smallStep, float* largeStep) const
{
	if (smallStep != NULL)
		*smallStep = fSmallStep;

	if (largeStep != NULL)
		*largeStep = fLargeStep;
}


void
BScrollBar::SetTarget(BView* target)
{
	if (fTarget) {
		// unset the previous target's scrollbar pointer
		if (fOrientation == B_VERTICAL)
			fTarget->fVerScroller = NULL;
		else
			fTarget->fHorScroller = NULL;
	}

	fTarget = target;
	if (fTarget) {
		if (fOrientation == B_VERTICAL)
			fTarget->fVerScroller = this;
		else
			fTarget->fHorScroller = this;
	}
}


void
BScrollBar::SetTarget(const char* targetName)
{
	// NOTE 1: BeOS implementation crashes for targetName == NULL
	// NOTE 2: BeOS implementation also does not modify the target
	// if it can't be found
	if (targetName == NULL)
		return;

	if (Window() == NULL)
		debugger("Method requires window and doesn't have one");

	BView* target = Window()->FindView(targetName);
	if (target != NULL)
		SetTarget(target);
}


BView*
BScrollBar::Target() const
{
	return fTarget;
}


void
BScrollBar::SetOrientation(orientation direction)
{
	if (fOrientation == direction)
		return;

	fOrientation = direction;
	InvalidateLayout();
	Invalidate();
}


orientation
BScrollBar::Orientation() const
{
	return fOrientation;
}


status_t
BScrollBar::SetBorderHighlighted(bool highlight)
{
	if (fPrivateData->fBorderHighlighted == highlight)
		return B_OK;

	fPrivateData->fBorderHighlighted = highlight;

	BRect dirty(Bounds());
	if (fOrientation == B_HORIZONTAL)
		dirty.bottom = dirty.top;
	else
		dirty.right = dirty.left;

	Invalidate(dirty);

	return B_OK;
}


void
BScrollBar::GetPreferredSize(float* _width, float* _height)
{
	const float scale = std::max(be_plain_font->Size() / 12.0f, 1.0f);
	if (fOrientation == B_VERTICAL) {
		if (_width)
			*_width = B_V_SCROLL_BAR_WIDTH * scale;

		if (_height)
			*_height = _MinSize().Height();
	} else if (fOrientation == B_HORIZONTAL) {
		if (_width)
			*_width = _MinSize().Width();

		if (_height)
			*_height = B_H_SCROLL_BAR_HEIGHT * scale;
	}
}


void
BScrollBar::ResizeToPreferred()
{
	BView::ResizeToPreferred();
}



void
BScrollBar::MakeFocus(bool focus)
{
	BView::MakeFocus(focus);
}


BSize
BScrollBar::MinSize()
{
	return BLayoutUtils::ComposeSize(ExplicitMinSize(), _MinSize());
}


BSize
BScrollBar::MaxSize()
{
	BSize maxSize;
	GetPreferredSize(&maxSize.width, &maxSize.height);
	if (fOrientation == B_HORIZONTAL)
		maxSize.width = B_SIZE_UNLIMITED;
	else
		maxSize.height = B_SIZE_UNLIMITED;
	return BLayoutUtils::ComposeSize(ExplicitMaxSize(), maxSize);
}


BSize
BScrollBar::PreferredSize()
{
	BSize preferredSize;
	GetPreferredSize(&preferredSize.width, &preferredSize.height);
	return BLayoutUtils::ComposeSize(ExplicitPreferredSize(), preferredSize);
}


status_t
BScrollBar::GetSupportedSuites(BMessage* message)
{
	return BView::GetSupportedSuites(message);
}


BHandler*
BScrollBar::ResolveSpecifier(BMessage* message, int32 index,
	BMessage* specifier, int32 what, const char* property)
{
	return BView::ResolveSpecifier(message, index, specifier, what, property);
}


status_t
BScrollBar::Perform(perform_code code, void* _data)
{
	switch (code) {
		case PERFORM_CODE_MIN_SIZE:
			((perform_data_min_size*)_data)->return_value
				= BScrollBar::MinSize();

			return B_OK;

		case PERFORM_CODE_MAX_SIZE:
			((perform_data_max_size*)_data)->return_value
				= BScrollBar::MaxSize();

			return B_OK;

		case PERFORM_CODE_PREFERRED_SIZE:
			((perform_data_preferred_size*)_data)->return_value
				= BScrollBar::PreferredSize();

			return B_OK;

		case PERFORM_CODE_LAYOUT_ALIGNMENT:
			((perform_data_layout_alignment*)_data)->return_value
				= BScrollBar::LayoutAlignment();

			return B_OK;

		case PERFORM_CODE_HAS_HEIGHT_FOR_WIDTH:
			((perform_data_has_height_for_width*)_data)->return_value
				= BScrollBar::HasHeightForWidth();

			return B_OK;

		case PERFORM_CODE_GET_HEIGHT_FOR_WIDTH:
		{
			perform_data_get_height_for_width* data
				= (perform_data_get_height_for_width*)_data;
			BScrollBar::GetHeightForWidth(data->width, &data->min, &data->max,
				&data->preferred);

			return B_OK;
		}

		case PERFORM_CODE_SET_LAYOUT:
		{
			perform_data_set_layout* data = (perform_data_set_layout*)_data;
			BScrollBar::SetLayout(data->layout);

			return B_OK;
		}

		case PERFORM_CODE_LAYOUT_INVALIDATED:
		{
			perform_data_layout_invalidated* data
				= (perform_data_layout_invalidated*)_data;
			BScrollBar::LayoutInvalidated(data->descendants);

			return B_OK;
		}

		case PERFORM_CODE_DO_LAYOUT:
		{
			BScrollBar::DoLayout();

			return B_OK;
		}
	}

	return BView::Perform(code, _data);
}


void BScrollBar::_ReservedScrollBar1() {}
void BScrollBar::_ReservedScrollBar2() {}
void BScrollBar::_ReservedScrollBar3() {}
void BScrollBar::_ReservedScrollBar4() {}


BScrollBar&
BScrollBar::operator=(const BScrollBar&)
{
	return *this;
}


bool
BScrollBar::_DoubleArrows() const
{
	if (!fPrivateData->fScrollBarInfo.double_arrows)
		return false;

	// if there is not enough room, switch to single arrows even though
	// double arrows is specified
	if (fOrientation == B_HORIZONTAL) {
		return Bounds().Width() > (Bounds().Height() + 1) * 4
			+ fPrivateData->fScrollBarInfo.min_knob_size * 2;
	} else {
		return Bounds().Height() > (Bounds().Width() + 1) * 4
			+ fPrivateData->fScrollBarInfo.min_knob_size * 2;
	}
}


void
BScrollBar::_UpdateThumbFrame()
{
	BRect bounds = Bounds();
	bounds.InsetBy(1.0, 1.0);

	BRect oldFrame = fPrivateData->fThumbFrame;
	fPrivateData->fThumbFrame = bounds;
	float minSize = fPrivateData->fScrollBarInfo.min_knob_size;
	float maxSize;
	float buttonSize;

	// assume square buttons
	if (fOrientation == B_VERTICAL) {
		maxSize = bounds.Height();
		buttonSize = bounds.Width() + 1.0;
	} else {
		maxSize = bounds.Width();
		buttonSize = bounds.Height() + 1.0;
	}

	if (_DoubleArrows()) {
		// subtract the size of four buttons
		maxSize -= buttonSize * 4;
	} else {
		// subtract the size of two buttons
		maxSize -= buttonSize * 2;
	}
	// visual adjustments (room for darker line between thumb and buttons)
	maxSize--;

	float thumbSize;
	if (fPrivateData->fScrollBarInfo.proportional) {
		float proportion = fProportion;
		if (fMin >= fMax || proportion > 1.0 || proportion < 0.0)
			proportion = 1.0;

		if (proportion == 0.0) {
			// Special case a proportion of 0.0, use the large step value
			// in that case (NOTE: fMin == fMax already handled above)
			// This calculation is based on the assumption that "large step"
			// scrolls by one "page size".
			proportion = fLargeStep / (2 * (fMax - fMin));
			if (proportion > 1.0)
				proportion = 1.0;
		}
		thumbSize = maxSize * proportion;
		if (thumbSize < minSize)
			thumbSize = minSize;
	} else
		thumbSize = minSize;

	thumbSize = floorf(thumbSize + 0.5);
	thumbSize--;

	// the thumb can be scrolled within the remaining area "maxSize - thumbSize - 1.0"
	float offset = 0.0;
	if (fMax > fMin) {
		offset = floorf(((fValue - fMin) / (fMax - fMin))
			* (maxSize - thumbSize - 1.0));
	}

	if (_DoubleArrows()) {
		offset += buttonSize * 2;
	} else
		offset += buttonSize;

	// visual adjustments (room for darker line between thumb and buttons)
	offset++;

	if (fOrientation == B_VERTICAL) {
		fPrivateData->fThumbFrame.bottom = fPrivateData->fThumbFrame.top
			+ thumbSize;
		fPrivateData->fThumbFrame.OffsetBy(0.0, offset);
	} else {
		fPrivateData->fThumbFrame.right = fPrivateData->fThumbFrame.left
			+ thumbSize;
		fPrivateData->fThumbFrame.OffsetBy(offset, 0.0);
	}

	if (Window() != NULL) {
		BRect invalid = oldFrame.IsValid()
			? oldFrame | fPrivateData->fThumbFrame
			: fPrivateData->fThumbFrame;
		// account for those two dark lines
		if (fOrientation == B_HORIZONTAL)
			invalid.InsetBy(-2.0, 0.0);
		else
			invalid.InsetBy(0.0, -2.0);

		Invalidate(invalid);
	}
}


float
BScrollBar::_ValueFor(BPoint where) const
{
	BRect bounds = Bounds();
	bounds.InsetBy(1.0f, 1.0f);

	float offset;
	float thumbSize;
	float maxSize;
	float buttonSize;

	if (fOrientation == B_VERTICAL) {
		offset = where.y;
		thumbSize = fPrivateData->fThumbFrame.Height();
		maxSize = bounds.Height();
		buttonSize = bounds.Width() + 1.0f;
	} else {
		offset = where.x;
		thumbSize = fPrivateData->fThumbFrame.Width();
		maxSize = bounds.Width();
		buttonSize = bounds.Height() + 1.0f;
	}

	if (_DoubleArrows()) {
		// subtract the size of four buttons
		maxSize -= buttonSize * 4;
		// convert point to inside of area between buttons
		offset -= buttonSize * 2;
	} else {
		// subtract the size of two buttons
		maxSize -= buttonSize * 2;
		// convert point to inside of area between buttons
		offset -= buttonSize;
	}
	// visual adjustments (room for darker line between thumb and buttons)
	maxSize--;
	offset++;

	return roundf(fMin + (offset / (maxSize - thumbSize)
		* (fMax - fMin + 1.0f)));
}


int32
BScrollBar::_ButtonFor(BPoint where) const
{
	BRect bounds = Bounds();
	bounds.InsetBy(1.0f, 1.0f);

	float buttonSize = fOrientation == B_VERTICAL
		? bounds.Width() + 1.0f
		: bounds.Height() + 1.0f;

	BRect rect(bounds.left, bounds.top,
		bounds.left + buttonSize, bounds.top + buttonSize);

	if (fOrientation == B_VERTICAL) {
		if (rect.Contains(where))
			return ARROW1;

		if (_DoubleArrows()) {
			rect.OffsetBy(0.0, buttonSize);
			if (rect.Contains(where))
				return ARROW2;

			rect.OffsetTo(bounds.left, bounds.bottom - 2 * buttonSize);
			if (rect.Contains(where))
				return ARROW3;
		}
		rect.OffsetTo(bounds.left, bounds.bottom - buttonSize);
		if (rect.Contains(where))
			return ARROW4;
	} else {
		if (rect.Contains(where))
			return ARROW1;

		if (_DoubleArrows()) {
			rect.OffsetBy(buttonSize, 0.0);
			if (rect.Contains(where))
				return ARROW2;

			rect.OffsetTo(bounds.right - 2 * buttonSize, bounds.top);
			if (rect.Contains(where))
				return ARROW3;
		}
		rect.OffsetTo(bounds.right - buttonSize, bounds.top);
		if (rect.Contains(where))
			return ARROW4;
	}

	return NOARROW;
}


BRect
BScrollBar::_ButtonRectFor(int32 button) const
{
	BRect bounds = Bounds();
	bounds.InsetBy(1.0f, 1.0f);

	float buttonSize = fOrientation == B_VERTICAL
		? bounds.Width() + 1.0f
		: bounds.Height() + 1.0f;

	BRect rect(bounds.left, bounds.top,
		bounds.left + buttonSize - 1.0f, bounds.top + buttonSize - 1.0f);

	if (fOrientation == B_VERTICAL) {
		switch (button) {
			case ARROW1:
				break;

			case ARROW2:
				rect.OffsetBy(0.0, buttonSize);
				break;

			case ARROW3:
				rect.OffsetTo(bounds.left, bounds.bottom - 2 * buttonSize + 1);
				break;

			case ARROW4:
				rect.OffsetTo(bounds.left, bounds.bottom - buttonSize + 1);
				break;
		}
	} else {
		switch (button) {
			case ARROW1:
				break;

			case ARROW2:
				rect.OffsetBy(buttonSize, 0.0);
				break;

			case ARROW3:
				rect.OffsetTo(bounds.right - 2 * buttonSize + 1, bounds.top);
				break;

			case ARROW4:
				rect.OffsetTo(bounds.right - buttonSize + 1, bounds.top);
				break;
		}
	}

	return rect;
}


void
BScrollBar::_UpdateTargetValue(BPoint where)
{
	if (fOrientation == B_VERTICAL) {
		fPrivateData->fStopValue = _ValueFor(BPoint(where.x, where.y
			- fPrivateData->fThumbFrame.Height() / 2.0));
	} else {
		fPrivateData->fStopValue = _ValueFor(BPoint(where.x
			- fPrivateData->fThumbFrame.Width() / 2.0, where.y));
	}
}


void
BScrollBar::_UpdateArrowButtons()
{
	bool upEnabled = fValue > fMin;
	if (fPrivateData->fUpArrowsEnabled != upEnabled) {
		fPrivateData->fUpArrowsEnabled = upEnabled;
		Invalidate(_ButtonRectFor(ARROW1));
		if (_DoubleArrows())
			Invalidate(_ButtonRectFor(ARROW3));
	}

	bool downEnabled = fValue < fMax;
	if (fPrivateData->fDownArrowsEnabled != downEnabled) {
		fPrivateData->fDownArrowsEnabled = downEnabled;
		Invalidate(_ButtonRectFor(ARROW4));
		if (_DoubleArrows())
			Invalidate(_ButtonRectFor(ARROW2));
	}
}


status_t
control_scrollbar(scroll_bar_info* info, BScrollBar* bar)
{
	if (bar == NULL || info == NULL)
		return B_BAD_VALUE;

	if (bar->fPrivateData->fScrollBarInfo.double_arrows
			!= info->double_arrows) {
		bar->fPrivateData->fScrollBarInfo.double_arrows = info->double_arrows;

		int8 multiplier = (info->double_arrows) ? 1 : -1;

		if (bar->fOrientation == B_VERTICAL) {
			bar->fPrivateData->fThumbFrame.OffsetBy(0, multiplier
				* B_H_SCROLL_BAR_HEIGHT);
		} else {
			bar->fPrivateData->fThumbFrame.OffsetBy(multiplier
				* B_V_SCROLL_BAR_WIDTH, 0);
		}
	}

	bar->fPrivateData->fScrollBarInfo.proportional = info->proportional;

	// TODO: Figure out how proportional relates to the size of the thumb

	// TODO: Add redraw code to reflect the changes

	if (info->knob >= 0 && info->knob <= 2)
		bar->fPrivateData->fScrollBarInfo.knob = info->knob;
	else
		return B_BAD_VALUE;

	if (info->min_knob_size >= SCROLL_BAR_MINIMUM_KNOB_SIZE
			&& info->min_knob_size <= SCROLL_BAR_MAXIMUM_KNOB_SIZE) {
		bar->fPrivateData->fScrollBarInfo.min_knob_size = info->min_knob_size;
	} else
		return B_BAD_VALUE;

	return B_OK;
}


BSize
BScrollBar::_MinSize() const
{
	BSize minSize;
	if (fOrientation == B_HORIZONTAL) {
		minSize.width = 2 * B_V_SCROLL_BAR_WIDTH
			+ 2 * fPrivateData->fScrollBarInfo.min_knob_size;
		minSize.height = B_H_SCROLL_BAR_HEIGHT;
	} else {
		minSize.width = B_V_SCROLL_BAR_WIDTH;
		minSize.height = 2 * B_H_SCROLL_BAR_HEIGHT
			+ 2 * fPrivateData->fScrollBarInfo.min_knob_size;
	}

	return minSize;
}
