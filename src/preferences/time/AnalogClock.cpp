/*
 * Copyright 2004-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mike Berg <mike@berg-net.us>
 *		Julun <host.haiku@gmx.de>
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "AnalogClock.h"
#include "TimeMessages.h"


#include <Bitmap.h>
#include <Message.h>


class OffscreenClock : public BView {
	public:
				OffscreenClock(BRect frame, const char *name);
				~OffscreenClock();

		void 	SetTime(int32 hour, int32 minute, int32 second);
		bool	IsDirty() const	{	return fDirty;	}
		void 	DrawClock();

	private:
		void	_DrawHands(float x, float y, float radius,
					rgb_color hourMinuteColor, rgb_color secondsColor);

		int32	fHours;
		int32	fMinutes;
		int32	fSeconds;
		bool	fDirty;
};


OffscreenClock::OffscreenClock(BRect frame, const char *name)
	: BView(frame, name, B_FOLLOW_NONE, B_WILL_DRAW),
	  fHours(0),
	  fMinutes(0),
	  fSeconds(0),
	  fDirty(true)
{
	SetFlags(Flags() | B_SUBPIXEL_PRECISE);
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
OffscreenClock::DrawClock()
{
	if (!LockLooper())
		return;

	BRect bounds = Bounds();
	// clear background
	rgb_color background = ui_color(B_PANEL_BACKGROUND_COLOR);
	SetHighColor(background);
	FillRect(bounds);

	float radius = floorf((MIN(bounds.Width(), bounds.Height()) / 2.0)) - 2.5;
	float x = floorf((bounds.left + bounds.right) / 2 + 0.5) + 0.5;
	float y = floorf((bounds.top + bounds.bottom) / 2 + 0.5) + 0.5;
		// + 0.5 is for the offset to pixel centers
		// (important when drawing with B_SUBPIXEL_PRECISE)

	bounds.Set(x - radius, y - radius, x + radius, y + radius);

	SetPenSize(2.0);

	SetHighColor(tint_color(background, B_DARKEN_1_TINT));
	StrokeEllipse(bounds.OffsetByCopy(-1, -1));

	SetHighColor(tint_color(background, B_LIGHTEN_2_TINT));
	StrokeEllipse(bounds.OffsetByCopy(1, 1));

	SetHighColor(tint_color(background, B_DARKEN_3_TINT));
	StrokeEllipse(bounds);

	SetLowColor(255, 255, 255);
	FillEllipse(bounds, B_SOLID_LOW);
	radius -= 3;

	SetHighColor(tint_color(HighColor(), B_DARKEN_2_TINT));

	// minutes
	SetPenSize(1.0);
	SetLineMode(B_BUTT_CAP, B_MITER_JOIN);
	for (int32 minute = 1; minute < 60; minute++) {
		if (minute % 5 == 0)
			continue;
		float x1 = x + sinf(minute * PI / 30.0) * radius;
		float y1 = y + cosf(minute * PI / 30.0) * radius;
		float x2 = x + sinf(minute * PI / 30.0) * (radius * 0.95);
		float y2 = y + cosf(minute * PI / 30.0) * (radius * 0.95);
		StrokeLine(BPoint(x1, y1), BPoint(x2, y2));
	}

	SetHighColor(tint_color(HighColor(), B_DARKEN_1_TINT));

	// hours
	SetPenSize(2.0);
	SetLineMode(B_ROUND_CAP, B_MITER_JOIN);
	for (int32 hour = 0; hour < 12; hour++) {
		float x1 = x + sinf(hour * PI / 6.0) * radius;
		float y1 = y + cosf(hour * PI / 6.0) * radius;
		float x2 = x + sinf(hour * PI / 6.0) * (radius * 0.9);
		float y2 = y + cosf(hour * PI / 6.0) * (radius * 0.9);
		StrokeLine(BPoint(x1, y1), BPoint(x2, y2));
	}

	rgb_color hourMinutColor = tint_color(HighColor(), B_DARKEN_2_TINT);
	rgb_color secondsColor = (rgb_color){ 255, 0, 0, 255 };
	rgb_color shadowColor = tint_color(LowColor(),
		(B_DARKEN_1_TINT + B_DARKEN_2_TINT) / 2);

	_DrawHands(x + 1.5, y + 1.5, radius, shadowColor, shadowColor);
	_DrawHands(x, y, radius, hourMinutColor, secondsColor);

	Sync();

	UnlockLooper();
}


void
OffscreenClock::_DrawHands(float x, float y, float radius,
	rgb_color hourMinuteColor, rgb_color secondsColor)
{
	SetHighColor(hourMinuteColor);

	float offsetX;
	float offsetY;

	// calc, draw hour hand
	SetPenSize(4.0);
	float hours = fHours + float(fMinutes) / 60.0;
	offsetX = (radius * 0.7) * sinf((hours * PI) / 6.0);
	offsetY = (radius * 0.7) * cosf((hours * PI) / 6.0);
	StrokeLine(BPoint(x, y), BPoint(x + offsetX, y - offsetY));

	// calc, draw minute hand
	SetPenSize(3.0);
	float minutes = fMinutes + float(fSeconds) / 60.0;
	offsetX = (radius * 0.9) * sinf((minutes * PI) / 30.0);
	offsetY = (radius * 0.9) * cosf((minutes * PI) / 30.0);
	StrokeLine(BPoint(x, y), BPoint(x + offsetX, y - offsetY));

	SetHighColor(secondsColor);

	// calc, draw second hand
	SetPenSize(1.0);
	offsetX = (radius * 0.95) * sinf((fSeconds * PI) / 30.0);
	offsetY = (radius * 0.95) * cosf((fSeconds * PI) / 30.0);
	StrokeLine(BPoint(x, y), BPoint(x + offsetX, y - offsetY));

	// draw the center knob
	SetHighColor(hourMinuteColor);
	FillEllipse(BPoint(x, y), radius * 0.06, radius * 0.06);
}


//	#pragma mark -


TAnalogClock::TAnalogClock(BRect frame, const char *name)
	: BView(frame, name, B_FOLLOW_NONE, B_WILL_DRAW | B_DRAW_ON_CHILDREN),
	fBitmap(NULL),
	fClock(NULL)
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
	fClock = new OffscreenClock(Bounds(), "offscreen");
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
TAnalogClock::MessageReceived(BMessage *message)
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
	if (fClock)
		fClock->SetTime(hour, minute, second);
	
	Invalidate();
}

