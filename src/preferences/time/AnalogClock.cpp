/*
 * Copyright 2004-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mike Berg (inseculous)
 */

/*! Notes: OffscreenClock borrows heavily from the clock source. */


#include "AnalogClock.h"
#include "Bitmaps.h"
#include "TimeMessages.h"

#include <Alert.h>
#include <Bitmap.h>
#include <Debug.h>
#include <Dragger.h>
#include <Window.h>


class OffscreenClock : public BView {
	public:
		OffscreenClock(BRect frame, const char *name);
		virtual ~OffscreenClock();

		void DrawClock();
		BPoint Position();

		void SetTo(int32 hour, int32 minute, int32 second);

	private:
		BBitmap *fBitmap;
		BBitmap *fCenterBitmap;
		BBitmap *fCapBitmap;
		BPoint fCenter;

		BPoint fMinutePoints[60];
		BPoint fHourPoints[60];
		short fHours;
		short fMinutes;
		short fSeconds;
};

const BRect kClockRect(0, 0, kClockFaceWidth -1, kClockFaceHeight -1);
const BRect kCenterRect(0, 0, kCenterWidth -1, kCenterHeight -1);
const BRect kCapRect(0, 0, kCapWidth -1, kCapHeight -1);

/*!
	Analog Clock face rendering view.
*/
OffscreenClock::OffscreenClock(BRect frame, const char *name)
	: BView(frame, name, B_NOT_RESIZABLE, B_WILL_DRAW)
{
	fBitmap = new BBitmap(kClockRect, kClockFaceColorSpace);
	fBitmap->SetBits(kClockFaceBits, (kClockFaceWidth) *(kClockFaceHeight +3), 0, kClockFaceColorSpace);

	ReplaceTransparentColor(fBitmap, ui_color(B_PANEL_BACKGROUND_COLOR));

	fCenterBitmap = new BBitmap(kCenterRect, kCenterColorSpace);
	fCenterBitmap->SetBits(kCenterBits, (kCenterWidth) *(kCenterHeight +1), 0, kCenterColorSpace);

	fCapBitmap = new BBitmap(kCapRect, kCapColorSpace);
	fCapBitmap->SetBits(kCapBits, (kCapWidth +1) *(kCapHeight +1) +1, 0, kCapColorSpace);
	
	fCenter = BPoint(42, 42);

	float counter;
	short index;
	float x, y, mRadius, hRadius;

	mRadius = fCenter.x - 12;
	hRadius = mRadius - 10;
	index = 0;
	
	//
	// Generate minutes/hours points array
	//
	for (counter = 90; counter >= 0; counter -= 6,index++) {
		x = mRadius * cos(((360 - counter)/180.0) * 3.1415);
		x += fCenter.x;
		y = mRadius * sin(((360 - counter)/180.0) * 3.1415);
		y += fCenter.x;
		fMinutePoints[index].Set(x,y);
		x = hRadius * cos(((360 - counter)/180.0) * 3.1415);
		x += fCenter.x;
		y = hRadius * sin(((360 - counter)/180.0) * 3.1415);
		y += fCenter.x;
		fHourPoints[index].Set(x,y);
	}

	for (counter = 354; counter > 90; counter -= 6,index++) {
		x = mRadius * cos(((360 - counter)/180.0) * 3.1415);
		x += fCenter.x;
		y = mRadius * sin(((360 - counter)/180.0) * 3.1415);
		y += fCenter.x;
		fMinutePoints[index].Set(x,y);
		x = hRadius * cos(((360 - counter)/180.0) * 3.1415);
		x += fCenter.x;
		y = hRadius * sin(((360 - counter)/180.0) * 3.1415);
		y += fCenter.x;
		fHourPoints[index].Set(x,y);
	}
}


OffscreenClock::~OffscreenClock()
{	
	delete fBitmap;
	delete fCenterBitmap;
	delete fCapBitmap;
}


void
OffscreenClock::SetTo(int32 hour, int32 minute, int32 second)
{
	if (fSeconds != second || fMinutes != minute || fHours != hour) {
		fHours = hour;
		fMinutes = minute;
		fSeconds = second;
	}
}


void
OffscreenClock::DrawClock()
{
	ASSERT(Window()->IsLocked());

	// draw clockface
	SetDrawingMode(B_OP_COPY);
	DrawBitmap(fBitmap, BPoint(0, 0));

	SetHighColor(0, 0, 0, 255);

	short hours = fHours;
	if (hours >= 12)
		hours -= 12;

	hours *= 5;
	hours += (fMinutes / 12);

	// draw center hub
	SetDrawingMode(B_OP_OVER);
	DrawBitmap(fCenterBitmap, fCenter - BPoint(kCenterWidth/2.0, kCenterHeight/2.0));

	// draw hands
	StrokeLine(fCenter, fHourPoints[hours]);
	StrokeLine(fCenter, fMinutePoints[fMinutes]);
	SetHighColor(tint_color(HighColor(), B_LIGHTEN_1_TINT));
	StrokeLine(fCenter, fMinutePoints[fSeconds]);

	// draw center cap
	DrawBitmap(fCapBitmap, fCenter -BPoint(kCapWidth/2.0, kCapHeight/2.0));

	Sync();
}


//	#pragma mark -


/*!
	BView to display clock face of current time.
*/
TAnalogClock::TAnalogClock(BRect frame, const char *name, uint32 resizingmode, uint32 flags)
	: BView(frame, name, resizingmode, flags | B_DRAW_ON_CHILDREN)
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
	fClock = new OffscreenClock(kClockRect, "offscreen");
	fBitmap = new BBitmap(kClockRect, B_COLOR_8_BIT, true);
	fBitmap->Lock();
	fBitmap->AddChild(fClock);
	fBitmap->Unlock();

	// offscreen clock is kClockFaceWidth by kClockFaceHeight
	// which might be smaller then TAnalogClock frame so "center" it.
	fClockLeftTop = BPoint((rect.Width() - kClockFaceWidth) / 2.0,
		(rect.Height() - kClockFaceHeight) / 2.0);	
}


void
TAnalogClock::AttachedToWindow()
{
	if (Parent())
		SetViewColor(Parent()->ViewColor());
	else
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
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
						SetTo(hour, minute, second);
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
	SetHighColor(0, 100, 10);

	if (fBitmap->Lock()) {
		fClock->DrawClock();
		DrawBitmap(fBitmap, fClockLeftTop);
		fBitmap->Unlock();
	}
}


void
TAnalogClock::SetTo(int32 hour, int32 minute, int32 second)
{
	fClock->SetTo(hour, minute, second);
	Invalidate();
}
