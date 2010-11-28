/*
 * Copyright 2004-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mike Berg <mike@berg-net.us>
 *		Julun <host.haiku@gmx.de>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Clemens <mail@Clemens-Zeidler.de>
 */

#include "AnalogClock.h"

#include <math.h>
#include <stdio.h>

#include <Bitmap.h>
#include <Message.h>
#include <Window.h>

#include "TimeMessages.h"


#define DRAG_DELTA_PHI 0.2


class OffscreenClock : public BView {
public:
							OffscreenClock(BRect frame, const char* name,
								bool drawSecondHand = true);
	virtual					~OffscreenClock();

			void		 	SetTime(int32 hour, int32 minute, int32 second);
			void 			GetTime(int32* hour, int32* minute, int32* second);
			bool			IsDirty() const	{	return fDirty;	}
			void 			DrawClock();

			bool			InHourHand(BPoint point);
			bool			InMinuteHand(BPoint point);

			void			SetHourHand(BPoint point);
			void			SetMinuteHand(BPoint point);

			void			SetHourDragging(bool dragging);
			void			SetMinuteDragging(bool dragging);

private:
			float			_GetPhi(BPoint point);
			bool			_InHand(BPoint point, int32 ticks, float radius);
			void			_DrawHands(float x, float y, float radius,
								rgb_color hourHourColor,
								rgb_color hourMinuteColor,
								rgb_color secondsColor, rgb_color knobColor);

			int32			fHours;
			int32			fMinutes;
			int32			fSeconds;
			bool			fDirty;

			float			fCenterX;
			float			fCenterY;
			float			fRadius;

			bool			fHourDragging;
			bool			fMinuteDragging;
			
			bool			fDrawSecondHand;
};


OffscreenClock::OffscreenClock(BRect frame, const char* name,
	bool drawSecondHand)
	:
	BView(frame, name, B_FOLLOW_NONE, B_WILL_DRAW),
	fHours(0),
	fMinutes(0),
	fSeconds(0),
	fDirty(true),
	fHourDragging(false),
	fMinuteDragging(false),
	fDrawSecondHand(drawSecondHand)
{
	SetFlags(Flags() | B_SUBPIXEL_PRECISE);

	BRect bounds = Bounds();
	fCenterX = floorf((bounds.left + bounds.right) / 2 + 0.5) + 0.5;
	fCenterY = floorf((bounds.top + bounds.bottom) / 2 + 0.5) + 0.5;
		// + 0.5 is for the offset to pixel centers
		// (important when drawing with B_SUBPIXEL_PRECISE)

	fRadius = floorf((MIN(bounds.Width(), bounds.Height()) / 2.0)) - 2.5;
	fRadius -= 3;
}


OffscreenClock::~OffscreenClock()
{
}


void
OffscreenClock::SetTime(int32 hour, int32 minute, int32 second)
{
	if (fHours == hour && fMinutes == minute && fSeconds == second)
		return;

	fHours = hour;
	fMinutes = minute;
	fSeconds = second;

	fDirty = true;
}


void
OffscreenClock::GetTime(int32* hour, int32* minute, int32* second)
{
	*hour = fHours;
	*minute = fMinutes;
	*second = fSeconds;
}


void
OffscreenClock::DrawClock()
{
	if (!LockLooper())
		return;

	BRect bounds = Bounds();
	// clear background
	rgb_color background = ui_color(B_PANEL_BACKGROUND_COLOR);
	SetHighColor(background);
	FillRect(bounds);


	bounds.Set(fCenterX - fRadius, fCenterY - fRadius,
		fCenterX + fRadius, fCenterY + fRadius);

	SetPenSize(2.0);

	SetHighColor(tint_color(background, B_DARKEN_1_TINT));
	StrokeEllipse(bounds.OffsetByCopy(-1, -1));

	SetHighColor(tint_color(background, B_LIGHTEN_2_TINT));
	StrokeEllipse(bounds.OffsetByCopy(1, 1));

	SetHighColor(tint_color(background, B_DARKEN_3_TINT));
	StrokeEllipse(bounds);

	SetLowColor(255, 255, 255);
	FillEllipse(bounds, B_SOLID_LOW);

	SetHighColor(tint_color(HighColor(), B_DARKEN_2_TINT));

	// minutes
	SetPenSize(1.0);
	SetLineMode(B_BUTT_CAP, B_MITER_JOIN);
	for (int32 minute = 1; minute < 60; minute++) {
		if (minute % 5 == 0)
			continue;
		float x1 = fCenterX + sinf(minute * M_PI / 30.0) * fRadius;
		float y1 = fCenterY + cosf(minute * M_PI / 30.0) * fRadius;
		float x2 = fCenterX + sinf(minute * M_PI / 30.0) * (fRadius * 0.95);
		float y2 = fCenterY + cosf(minute * M_PI / 30.0) * (fRadius * 0.95);
		StrokeLine(BPoint(x1, y1), BPoint(x2, y2));
	}

	SetHighColor(tint_color(HighColor(), B_DARKEN_1_TINT));

	// hours
	SetPenSize(2.0);
	SetLineMode(B_ROUND_CAP, B_MITER_JOIN);
	for (int32 hour = 0; hour < 12; hour++) {
		float x1 = fCenterX + sinf(hour * M_PI / 6.0) * fRadius;
		float y1 = fCenterY + cosf(hour * M_PI / 6.0) * fRadius;
		float x2 = fCenterX + sinf(hour * M_PI / 6.0) * (fRadius * 0.9);
		float y2 = fCenterY + cosf(hour * M_PI / 6.0) * (fRadius * 0.9);
		StrokeLine(BPoint(x1, y1), BPoint(x2, y2));
	}

	rgb_color knobColor = tint_color(HighColor(), B_DARKEN_2_TINT);;
	rgb_color hourColor;
	if (fHourDragging)
		hourColor = (rgb_color){ 0, 0, 255, 255 };
	else
	 	hourColor = tint_color(HighColor(), B_DARKEN_2_TINT);

	rgb_color minuteColor;
	if (fMinuteDragging)
		minuteColor = (rgb_color){ 0, 0, 255, 255 };
	else
	 	minuteColor = tint_color(HighColor(), B_DARKEN_2_TINT);

	rgb_color secondsColor = (rgb_color){ 255, 0, 0, 255 };
	rgb_color shadowColor = tint_color(LowColor(),
		(B_DARKEN_1_TINT + B_DARKEN_2_TINT) / 2);

	_DrawHands(fCenterX + 1.5, fCenterY + 1.5, fRadius,
		shadowColor, shadowColor, shadowColor, shadowColor);
	_DrawHands(fCenterX, fCenterY, fRadius,
		hourColor, minuteColor, secondsColor, knobColor);

	Sync();

	UnlockLooper();
}


bool
OffscreenClock::InHourHand(BPoint point)
{
	int32 ticks = fHours;
	if (ticks > 12)
		ticks -= 12;
	ticks *= 5;
	ticks += int32(5. * fMinutes / 60.0);
	if (ticks > 60)
		ticks -= 60;
	return _InHand(point, ticks, fRadius * 0.7);
}


bool
OffscreenClock::InMinuteHand(BPoint point)
{
	return _InHand(point, fMinutes, fRadius * 0.9);
}


void
OffscreenClock::SetHourHand(BPoint point)
{
	point.x -= fCenterX;
	point.y -= fCenterY;

	float pointPhi = _GetPhi(point);
	float hoursExact = 6.0 * pointPhi / M_PI;
	if (fHours >= 12)
		fHours = 12;
	else
		fHours = 0;
	fHours += int32(hoursExact);

	SetTime(fHours, fMinutes, fSeconds);
}


void
OffscreenClock::SetMinuteHand(BPoint point)
{
	point.x -= fCenterX;
	point.y -= fCenterY;

	float pointPhi = _GetPhi(point);
	float minutesExact = 30.0 * pointPhi / M_PI;
	fMinutes = int32(ceilf(minutesExact));

	SetTime(fHours, fMinutes, fSeconds);
}


void
OffscreenClock::SetHourDragging(bool dragging)
{
	fHourDragging = dragging;
	fDirty = true;
}


void
OffscreenClock::SetMinuteDragging(bool dragging)
{
	fMinuteDragging = dragging;
	fDirty = true;
}


float
OffscreenClock::_GetPhi(BPoint point)
{
	if (point.x == 0 && point.y < 0)
		return 2 * M_PI;
	if (point.x == 0 && point.y > 0)
		return M_PI;
	if (point.y == 0 && point.x < 0)
		return M_PI * 3 / 2;
	if (point.y == 0 && point.x > 0)
		return M_PI / 2;

	float pointPhi = atanf(-1. * point.y / point.x);
	if (point.y < 0. && point.x > 0.)	// right upper corner
		pointPhi = M_PI / 2. - pointPhi;
	if (point.y > 0. && point.x > 0.)	// right lower corner
		pointPhi = M_PI / 2 - pointPhi;
	if (point.y > 0. && point.x < 0.)	// left lower corner
		pointPhi = (M_PI * 3. / 2. - pointPhi);
	if (point.y < 0. && point.x < 0.)	// left upper corner
		pointPhi = 3. / 2. * M_PI - pointPhi;
	return pointPhi;
}


bool
OffscreenClock::_InHand(BPoint point, int32 ticks, float radius)
{
	point.x -= fCenterX;
	point.y -= fCenterY;

	float pRadius = sqrt(pow(point.x, 2) + pow(point.y, 2));

	if (radius < pRadius)
		return false;

	float pointPhi = _GetPhi(point);
	float handPhi = M_PI / 30.0 * ticks;
	float delta = pointPhi - handPhi;
	if (fabs(delta) > DRAG_DELTA_PHI)
		return false;

	return true;
}


void
OffscreenClock::_DrawHands(float x, float y, float radius,
	rgb_color hourColor,
	rgb_color minuteColor,
	rgb_color secondsColor,
	rgb_color knobColor)
{
	float offsetX;
	float offsetY;

	// calc, draw hour hand
	SetHighColor(hourColor);
	SetPenSize(4.0);
	float hours = fHours + float(fMinutes) / 60.0;
	offsetX = (radius * 0.7) * sinf((hours * M_PI) / 6.0);
	offsetY = (radius * 0.7) * cosf((hours * M_PI) / 6.0);
	StrokeLine(BPoint(x, y), BPoint(x + offsetX, y - offsetY));

	// calc, draw minute hand
	SetHighColor(minuteColor);
	SetPenSize(3.0);
	float minutes = fMinutes + float(fSeconds) / 60.0;
	offsetX = (radius * 0.9) * sinf((minutes * M_PI) / 30.0);
	offsetY = (radius * 0.9) * cosf((minutes * M_PI) / 30.0);
	StrokeLine(BPoint(x, y), BPoint(x + offsetX, y - offsetY));

	if (fDrawSecondHand) {
		// calc, draw second hand
		SetHighColor(secondsColor);
		SetPenSize(1.0);
		offsetX = (radius * 0.95) * sinf((fSeconds * M_PI) / 30.0);
		offsetY = (radius * 0.95) * cosf((fSeconds * M_PI) / 30.0);
		StrokeLine(BPoint(x, y), BPoint(x + offsetX, y - offsetY));
	}
	
	// draw the center knob
	SetHighColor(knobColor);
	FillEllipse(BPoint(x, y), radius * 0.06, radius * 0.06);
}


//	#pragma mark -


TAnalogClock::TAnalogClock(BRect frame, const char* name,
	bool drawSecondHand, bool interactive)
	:
	BView(frame, name, B_FOLLOW_NONE, B_WILL_DRAW | B_DRAW_ON_CHILDREN),
	fBitmap(NULL),
	fClock(NULL),
	fDrawSecondHand(drawSecondHand),
	fInteractive(interactive),
	fDraggingHourHand(false),
	fDraggingMinuteHand(false),
	fTimeChangeIsOngoing(false)
{
	_InitView(frame);
}


TAnalogClock::~TAnalogClock()
{
	delete fBitmap;
}


void
TAnalogClock::_InitView(BRect rect)
{
	fClock = new OffscreenClock(Bounds(), "offscreen", fDrawSecondHand);
	fBitmap = new BBitmap(Bounds(), B_RGB32, true);
	fBitmap->Lock();
	fBitmap->AddChild(fClock);
	fBitmap->Unlock();
}


void
TAnalogClock::AttachedToWindow()
{
	SetViewColor(B_TRANSPARENT_COLOR);
}


void
TAnalogClock::MessageReceived(BMessage* message)
{
	int32 change;
	switch (message->what) {
		case B_OBSERVER_NOTICE_CHANGE:
			message->FindInt32(B_OBSERVE_WHAT_CHANGE, &change);
			switch (change) {
				case H_TM_CHANGED:
				{
					int32 hour = 0;
					int32 minute = 0;
					int32 second = 0;
					if (message->FindInt32("hour", &hour) == B_OK
					 && message->FindInt32("minute", &minute) == B_OK
					 && message->FindInt32("second", &second) == B_OK)
						SetTime(hour, minute, second);
					break;
				}
				default:
					BView::MessageReceived(message);
					break;
			}
		break;
		default:
			BView::MessageReceived(message);
			break;
	}
}


void
TAnalogClock::MouseDown(BPoint point)
{
	if(!fInteractive) {
		BView::MouseDown(point);
		return;
	}
	
	fDraggingMinuteHand = fClock->InMinuteHand(point);
	if (fDraggingMinuteHand) {
		fClock->SetMinuteDragging(true);
		SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
		Invalidate();
		return;
	}
	fDraggingHourHand = fClock->InHourHand(point);
	if (fDraggingHourHand) {
		fClock->SetHourDragging(true);
		SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
		Invalidate();
		return;
	}
}


void
TAnalogClock::MouseUp(BPoint point)
{
	if(!fInteractive) {
		BView::MouseUp(point);
		return;
	}
	
	if (fDraggingHourHand || fDraggingMinuteHand) {
		int32 hour, minute, second;
		fClock->GetTime(&hour, &minute, &second);
		BMessage message(H_USER_CHANGE);
		message.AddBool("time", true);
		message.AddInt32("hour", hour);
		message.AddInt32("minute", minute);
		Window()->PostMessage(&message);
		fTimeChangeIsOngoing = true;
	}
	fDraggingHourHand = false;
	fDraggingMinuteHand = false;
	fClock->SetMinuteDragging(false);
	fClock->SetHourDragging(false);
}


void
TAnalogClock::MouseMoved(BPoint point, uint32 transit, const BMessage* message)
{
	if(!fInteractive) {
		BView::MouseMoved(point, transit, message);
		return;
	}

	if (fDraggingMinuteHand)
		fClock->SetMinuteHand(point);
	if (fDraggingHourHand)
		fClock->SetHourHand(point);

	Invalidate();
}


void
TAnalogClock::Draw(BRect /*updateRect*/)
{
	if (fBitmap) {
		if (fClock->IsDirty())
			fClock->DrawClock();
		DrawBitmap(fBitmap, BPoint(0, 0));
	}
}


void
TAnalogClock::SetTime(int32 hour, int32 minute, int32 second)
{
	// don't set the time if the hands are in a drag action
	if (fDraggingHourHand || fDraggingMinuteHand || fTimeChangeIsOngoing)
		return;

	if (fClock)
		fClock->SetTime(hour, minute, second);

	BWindow* window = Window();
	if (window && window->Lock()) {
		Invalidate();
		Window()->Unlock();
	}
}


void
TAnalogClock::ChangeTimeFinished()
{
	fTimeChangeIsOngoing = false;
}
