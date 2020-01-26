/*
 * Copyright 2005, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Inspired by SoundCapture from Be newsletter (Media Kit Basics: Consumers 
 * and Producers)
 */
 
#include <stdio.h>
#include <string.h>
#include <Screen.h>

#include "TrackSlider.h"
#include "icon_button.h"

TrackSlider::TrackSlider(BRect rect, const char *title, BMessage *msg, 
	uint32 resizeFlags)
	: 
	BControl(rect, "slider", NULL, msg, resizeFlags, B_WILL_DRAW 
		| B_FRAME_EVENTS),
	fLeftTime(0),
	fRightTime(1000000),
	fMainTime(0),
	fTotalTime(1000000),
	fLeftTracking(false),
	fRightTracking(false), 
	fMainTracking(false),
	fBitmap(NULL),
	fBitmapView(NULL)
{
	fFont.SetSize(8.0);
	fFont.SetFlags(B_DISABLE_ANTIALIASING);
	
	int32 numFamilies = count_font_families(); 
	for (int32 i = 0; i < numFamilies; i++ ) { 
		font_family family; 
		uint32 flags; 
		if ((get_font_family(i, &family, &flags) == B_OK)
			&& (strcmp(family, "Baskerville") == 0)) {
			fFont.SetFamilyAndFace(family, B_REGULAR_FACE);
			break;
		}
	}
	
}


TrackSlider::~TrackSlider()
{
	delete fBitmap;
}


void
TrackSlider::AttachedToWindow()
{
	BControl::AttachedToWindow();
	SetViewColor(B_TRANSPARENT_COLOR);
	_InitBitmap();
	_RenderBitmap();
}


void
TrackSlider::_InitBitmap()
{
	if (fBitmap != NULL) {
		if (fBitmapView != NULL) {
			fBitmap->RemoveChild(fBitmapView);
			delete fBitmapView;
		}
		delete fBitmap;
	}

	BRect rect = Bounds();
	
	fBitmap = new BBitmap(rect, BScreen().ColorSpace(), true);
	
	fBitmapView = new SliderOffscreenView(rect.OffsetToSelf(B_ORIGIN), "bitmapView");
	fBitmap->AddChild(fBitmapView);
	
	fBitmapView->fRight = Bounds().right - kLeftRightTrackSliderWidth;
	if (fTotalTime == 0) {
		fBitmapView->fLeftX = 14;
		fBitmapView->fRightX = fBitmapView->fRight;
		fBitmapView->fPositionX = 15;
	} else {
		fBitmapView->fLeftX = 14 + (fBitmapView->fRight - 15) 
			* ((double)fLeftTime / fTotalTime);
		fBitmapView->fRightX = 15 + (fBitmapView->fRight - 16) 
			* ((double)fRightTime / fTotalTime);
		fBitmapView->fPositionX = 15 + (fBitmapView->fRight - 14) 
			* ((double)fMainTime / fTotalTime);
	}
}

#define SLIDER_BASE 10

void
TrackSlider::_RenderBitmap()
{
	/* rendering */
	if (fBitmap->Lock()) {
		fBitmapView->DrawX();
		fBitmap->Unlock();
	}
}


void
TrackSlider::Draw(BRect updateRect)
{	
	DrawBitmapAsync(fBitmap, BPoint(0,0));
		
	_DrawCounter(fMainTime, fBitmapView->fPositionX, fMainTracking);
	if (fLeftTracking)
		_DrawCounter(fLeftTime, fBitmapView->fLeftX, fLeftTracking);
	else if (fRightTracking)
		_DrawCounter(fRightTime, fBitmapView->fRightX, fRightTracking);
	
	_DrawMarker(fBitmapView->fPositionX);	
	
	Sync();
}


void
TrackSlider::_DrawCounter(bigtime_t timestamp, float position, bool isTracking)
{
	// timecounter
	
	rgb_color gray = {128,128,128};
	rgb_color blue = {0,0,140};
	rgb_color blue2 = {146,146,214};
	rgb_color white = {255,255,255};
	
	char string[12];
	_TimeToString(timestamp, string);
	int32 halfwidth = ((int32)fFont.StringWidth(string)) / 2;
	
	float counterX = position;
	if (counterX < 39)
		counterX = 39;
	if (counterX > fBitmapView->fRight - 23)
		counterX = fBitmapView->fRight - 23;
	
	BeginLineArray(4);
	if (!isTracking) {
		AddLine(BPoint(counterX-halfwidth-3,SLIDER_BASE+1), 
			BPoint(counterX+halfwidth+3,SLIDER_BASE+1), gray);
		AddLine(BPoint(counterX+halfwidth+4,SLIDER_BASE+1), 
			BPoint(counterX+halfwidth+4,SLIDER_BASE-8), gray);
		AddLine(BPoint(counterX-halfwidth-4,SLIDER_BASE+1), 
			BPoint(counterX-halfwidth-4,SLIDER_BASE-9), white);
		AddLine(BPoint(counterX-halfwidth-3,SLIDER_BASE-9), 
			BPoint(counterX+halfwidth+4,SLIDER_BASE-9), white);
		SetHighColor(216,216,216);
	} else {
		AddLine(BPoint(counterX-halfwidth-3,SLIDER_BASE+1), 
			BPoint(counterX+halfwidth+3,SLIDER_BASE+1), blue);
		AddLine(BPoint(counterX+halfwidth+4,SLIDER_BASE+1), 
			BPoint(counterX+halfwidth+4,SLIDER_BASE-9), blue2);
		AddLine(BPoint(counterX-halfwidth-4,SLIDER_BASE+1), 
			BPoint(counterX-halfwidth-4,SLIDER_BASE-9), blue2);
		AddLine(BPoint(counterX-halfwidth-3,SLIDER_BASE-9), 
			BPoint(counterX+halfwidth+3,SLIDER_BASE-9), blue2);
		SetHighColor(48,48,241);
	}
	EndLineArray();
	FillRect(BRect(counterX-halfwidth-3,SLIDER_BASE-8,counterX+halfwidth+3,
		SLIDER_BASE));

#ifdef __HAIKU__
	SetDrawingMode(B_OP_OVER);
#else
	SetDrawingMode(B_OP_COPY);
#endif
	if (isTracking)
		SetHighColor(255,255,255);
	else
		SetHighColor(0,0,0);
	SetLowColor(ViewColor());
	
	SetFont(&fFont);
	DrawString(string, BPoint(counterX-halfwidth, SLIDER_BASE-1));

}


void
TrackSlider::_DrawMarker(float position)
{
	rgb_color black = {0,0,0};
	rgb_color rose = {255,152,152};
	rgb_color red = {255,0,0};
	rgb_color bordeau = {178,0,0};
	rgb_color white = {255,255,255};
		
	BeginLineArray(30);
	AddLine(BPoint(position,SLIDER_BASE+7), BPoint(position-4,SLIDER_BASE+3), 
		black);
	AddLine(BPoint(position-4,SLIDER_BASE+3), BPoint(position-4,SLIDER_BASE+1),
		black);
	AddLine(BPoint(position-4,SLIDER_BASE+1), BPoint(position+4,SLIDER_BASE+1),
		black);
	AddLine(BPoint(position+4,SLIDER_BASE+1), BPoint(position+4,SLIDER_BASE+3),
		black);
	AddLine(BPoint(position+4,SLIDER_BASE+3), BPoint(position,SLIDER_BASE+7),
		black);
	
	
	AddLine(BPoint(position-3,SLIDER_BASE+2), BPoint(position+3,SLIDER_BASE+2),
		rose);
	AddLine(BPoint(position-3,SLIDER_BASE+3), BPoint(position-1,SLIDER_BASE+5),
		rose);
	
	AddLine(BPoint(position-2,SLIDER_BASE+3), BPoint(position+2,SLIDER_BASE+3),
		red);
	AddLine(BPoint(position-1,SLIDER_BASE+4), BPoint(position+1,SLIDER_BASE+4),
		red);
	AddLine(BPoint(position,SLIDER_BASE+5), BPoint(position,SLIDER_BASE+5),
		red);
	
	AddLine(BPoint(position,SLIDER_BASE+6), BPoint(position+3,SLIDER_BASE+3),
		bordeau);
	
	AddLine(BPoint(position,SLIDER_BASE+12), BPoint(position-4,SLIDER_BASE+16),
		black);
	AddLine(BPoint(position-4,SLIDER_BASE+16), BPoint(position-4,
		SLIDER_BASE+17), black);
	AddLine(BPoint(position-4,SLIDER_BASE+17), BPoint(position+4,
		SLIDER_BASE+17), black);
	AddLine(BPoint(position+4,SLIDER_BASE+17), BPoint(position+4,
		SLIDER_BASE+16), black);
	AddLine(BPoint(position+4,SLIDER_BASE+16), BPoint(position,SLIDER_BASE+12),
		black);
	AddLine(BPoint(position-4,SLIDER_BASE+18), BPoint(position+4,
		SLIDER_BASE+18), white);
	
	AddLine(BPoint(position-3,SLIDER_BASE+16), BPoint(position,SLIDER_BASE+13),
		rose);
	
	AddLine(BPoint(position-2,SLIDER_BASE+16), BPoint(position+2,
		SLIDER_BASE+16), red);
	AddLine(BPoint(position-1,SLIDER_BASE+15), BPoint(position+1,
		SLIDER_BASE+15), red);
	AddLine(BPoint(position,SLIDER_BASE+14), BPoint(position,
		SLIDER_BASE+14), red);
	
	AddLine(BPoint(position+1,SLIDER_BASE+14), BPoint(position+3,
		SLIDER_BASE+16), bordeau);
	
	EndLineArray();
}


void 
TrackSlider::MouseMoved(BPoint point, uint32 transit, const BMessage *message)
{
	if (!IsTracking())
		return;
		
	uint32 mouseButtons;
	BPoint where;
	GetMouse(&where, &mouseButtons, true);
	
	// button not pressed, exit
	if (! (mouseButtons & B_PRIMARY_MOUSE_BUTTON)) {
		Invoke();
		SetTracking(false);
	}
	
	_UpdatePosition(point);
}


void
TrackSlider::MouseDown(BPoint point)
{
	if (!Bounds().InsetBySelf(2,2).Contains(point))
		return;

	_UpdatePosition(point);
	SetTracking(true);
	SetMouseEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY 
		| B_LOCK_WINDOW_FOCUS);
}


void
TrackSlider::MouseUp(BPoint point)
{
	if (!IsTracking())
		return;
	if (Bounds().InsetBySelf(2,2).Contains(point)) {
		_UpdatePosition(point);
	}
	
	fLeftTracking = fRightTracking = fMainTracking = false;
	
	Invoke();
	SetTracking(false);
	Draw(Bounds());
	Flush();
}


void
TrackSlider::_UpdatePosition(BPoint point)
{
	BRect leftRect(fBitmapView->fLeftX-9, SLIDER_BASE+3, fBitmapView->fLeftX, 
		SLIDER_BASE+16);
	BRect rightRect(fBitmapView->fRightX, SLIDER_BASE+3, 
		fBitmapView->fRightX+9, SLIDER_BASE+16);
	
	if (!(fRightTracking || fMainTracking) && (fLeftTracking 
		|| ((point.x < fBitmapView->fPositionX-4) && leftRect.Contains(point)))) {
		if (!IsTracking())
			fBitmapView->fLastX = point.x - fBitmapView->fLeftX;
		fBitmapView->fLeftX = MIN(MAX(point.x - fBitmapView->fLastX, 15), 
			fBitmapView->fRight);
		fLeftTime = (bigtime_t)(MAX(MIN((fBitmapView->fLeftX - 15) 
			/ (fBitmapView->fRight - 14),1), 0) * fTotalTime);
		fLeftTracking = true;
		
		BMessage msg = *Message();
		msg.AddInt64("left", fLeftTime);
		
		if (fBitmapView->fPositionX < fBitmapView->fLeftX) {
			fBitmapView->fPositionX = fBitmapView->fLeftX + 1;
			fMainTime = fLeftTime;
			msg.AddInt64("main", fMainTime);
			if (fBitmapView->fRightX < fBitmapView->fPositionX) {
				fBitmapView->fRightX = fBitmapView->fPositionX;
				fRightTime = fMainTime;
				msg.AddInt64("right", fRightTime);
			}
		}
		
		Invoke(&msg);
		_RenderBitmap();
		
		//printf("fLeftPos : %Ld\n", fLeftTime);
	} else if (!fMainTracking && (fRightTracking 
		|| ((point.x > fBitmapView->fPositionX+4) 
		&& rightRect.Contains(point)))) {
		if (!IsTracking())
			fBitmapView->fLastX = point.x - fBitmapView->fRightX;
		fBitmapView->fRightX = MIN(MAX(point.x - fBitmapView->fLastX, 15), 
			fBitmapView->fRight);
		fRightTime = (bigtime_t)(MAX(MIN((fBitmapView->fRightX - 15) 
			/ (fBitmapView->fRight - 14),1), 0) * fTotalTime);
		fRightTracking = true;
		
		BMessage msg = *Message();
		msg.AddInt64("right", fRightTime);
		
		if (fBitmapView->fPositionX > fBitmapView->fRightX) {
			fBitmapView->fPositionX = fBitmapView->fRightX;
			fMainTime = fRightTime;
			msg.AddInt64("main", fMainTime);
			if (fBitmapView->fLeftX > fBitmapView->fPositionX) {
				fBitmapView->fLeftX = fBitmapView->fPositionX - 1;
				fLeftTime = fMainTime;
				msg.AddInt64("left", fLeftTime);
			}
		}
		
		Invoke(&msg);
		_RenderBitmap();
		
		//printf("fRightPos : %Ld\n", fRightTime);
	} else {
		fBitmapView->fPositionX = MIN(MAX(point.x, 15), fBitmapView->fRight);
		fMainTime = (bigtime_t)(MAX(MIN((fBitmapView->fPositionX - 15) 
			/ (fBitmapView->fRight - 14),1), 0) * fTotalTime);
		fMainTracking = true;
		
		BMessage msg = *Message();
		msg.AddInt64("main", fMainTime);
		
		if (fBitmapView->fRightX < fBitmapView->fPositionX) {
			fBitmapView->fRightX = fBitmapView->fPositionX;
			fRightTime = fMainTime;
			msg.AddInt64("right", fRightTime);
			_RenderBitmap();
		} else if (fBitmapView->fLeftX > fBitmapView->fPositionX) {
			fBitmapView->fLeftX = fBitmapView->fPositionX - 1;
			fLeftTime = fMainTime;
			msg.AddInt64("left", fLeftTime);
			_RenderBitmap();
		}
		
		Invoke(&msg);
		//printf("fPosition : %Ld\n", fMainTime);
	}
	Draw(Bounds());
	Flush();
}


void 
TrackSlider::_TimeToString(bigtime_t timestamp, char *string)
{
	uint32 hours = timestamp / 3600000000LL;
	timestamp -= hours * 3600000000LL;
	uint32 minutes = timestamp / 60000000LL;
	timestamp -= minutes * 60000000LL;
	uint32 seconds = timestamp / 1000000LL;
	timestamp -= seconds * 1000000LL;
	uint32 centiseconds = timestamp / 10000LL;
	sprintf(string, "%02" B_PRId32 ":%02" B_PRId32 ":%02" B_PRId32 ":%02"
		B_PRId32, hours, minutes, seconds, centiseconds);
}


void 
TrackSlider::SetMainTime(bigtime_t timestamp, bool reset)
{
	fMainTime = timestamp;
	fBitmapView->fPositionX = 15 + (fBitmapView->fRight - 14) 
		* ((double)fMainTime / fTotalTime);
	if (reset) {
		fRightTime = fTotalTime;
		fLeftTime = 0;
		fBitmapView->fLeftX = 14 + (fBitmapView->fRight - 15) 
			* ((double)fLeftTime / fTotalTime);
		fBitmapView->fRightX = 15 + (fBitmapView->fRight - 16) 
			* ((double)fRightTime / fTotalTime);
		_RenderBitmap();
	}
	Invalidate();
}

void 
TrackSlider::SetTotalTime(bigtime_t timestamp, bool reset)
{
	fTotalTime = timestamp;
	if (reset) {
		fMainTime = 0;
		fRightTime = fTotalTime;
		fLeftTime = 0;
	}
	fBitmapView->fPositionX = 15 + (fBitmapView->fRight - 14) 
		* ((double)fMainTime / fTotalTime);
	fBitmapView->fLeftX = 14 + (fBitmapView->fRight - 15) 
		* ((double)fLeftTime / fTotalTime);
	fBitmapView->fRightX = 15 + (fBitmapView->fRight - 16) 
		* ((double)fRightTime / fTotalTime);
	_RenderBitmap();
	Invalidate();
}


void
TrackSlider::ResetMainTime()
{
	fMainTime = fLeftTime;
	fBitmapView->fPositionX = 15 + (fBitmapView->fRight - 14) 
		* ((double)fMainTime / fTotalTime);
	Invalidate();
}


void 
TrackSlider::FrameResized(float width, float height)
{
	fBitmapView->fRight = Bounds().right - kLeftRightTrackSliderWidth;
	fBitmapView->fPositionX = 15 + (fBitmapView->fRight - 14) 
		* ((double)fMainTime / fTotalTime);
	_InitBitmap();
	fBitmapView->fLeftX = 14 + (fBitmapView->fRight - 15) 
		* ((double)fLeftTime / fTotalTime);
	fBitmapView->fRightX = 15 + (fBitmapView->fRight - 16) 
		* ((double)fRightTime / fTotalTime);
	_RenderBitmap();
	Invalidate();
}


SliderOffscreenView::SliderOffscreenView(BRect frame, const char *name)
	: BView(frame, name, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW),
	leftBitmap(BRect(BPoint(0,0), kLeftRightTrackSliderSize), B_CMAP8),
	rightBitmap(BRect(BPoint(0,0), kLeftRightTrackSliderSize), B_CMAP8),
	leftThumbBitmap(BRect(0, 0, kLeftRightThumbWidth - 1, 
		kLeftRightThumbHeight - 1), B_CMAP8),
	rightThumbBitmap(BRect(0, 0, kLeftRightThumbWidth - 1, 
		kLeftRightThumbHeight - 1), B_CMAP8)
{
	leftBitmap.SetBits(kLeftTrackSliderBits, 
		kLeftRightTrackSliderWidth * kLeftRightTrackSliderHeight, 0, B_CMAP8);
	rightBitmap.SetBits(kRightTrackSliderBits, 
		kLeftRightTrackSliderWidth * kLeftRightTrackSliderHeight, 0, B_CMAP8);
	leftThumbBitmap.SetBits(kLeftThumbBits, 
		kLeftRightThumbWidth * kLeftRightThumbHeight, 0, B_CMAP8);
	rightThumbBitmap.SetBits(kRightThumbBits, 
		kLeftRightThumbWidth * kLeftRightThumbHeight, 0, B_CMAP8);
}


SliderOffscreenView::~SliderOffscreenView()
{

}


void
SliderOffscreenView::DrawX()
{
	SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	FillRect(Bounds());
	
	SetHighColor(189, 186, 189);
	StrokeLine(BPoint(11, SLIDER_BASE + 1), BPoint(fRight, SLIDER_BASE + 1));
	SetHighColor(0, 0, 0);
	StrokeLine(BPoint(11, SLIDER_BASE + 2), BPoint(fRight, SLIDER_BASE + 2));
	SetHighColor(255, 255, 255);
	StrokeLine(BPoint(11, SLIDER_BASE + 17), BPoint(fRight, SLIDER_BASE + 17));
	SetHighColor(231, 227, 231);
	StrokeLine(BPoint(11, SLIDER_BASE + 18), BPoint(fRight, SLIDER_BASE + 18));
	
	SetDrawingMode(B_OP_OVER);
	SetLowColor(HighColor());

	BPoint leftPoint(5, SLIDER_BASE + 1);
	DrawBitmapAsync(&leftBitmap, BRect(BPoint(0, 0), 
		kLeftRightTrackSliderSize - BPoint(5, 0)),
		BRect(leftPoint, leftPoint + kLeftRightTrackSliderSize - BPoint(5, 0)));
	BPoint rightPoint(fRight + 1, SLIDER_BASE + 1);
	DrawBitmapAsync(&rightBitmap, BRect(BPoint(5, 0), kLeftRightTrackSliderSize), 
		BRect(rightPoint, rightPoint + kLeftRightTrackSliderSize-BPoint(5, 0)));
		
	SetHighColor(153, 153, 153);
	FillRect(BRect(11, SLIDER_BASE + 3, fLeftX - 9, SLIDER_BASE + 16));
	FillRect(BRect(fRightX + 9, SLIDER_BASE + 3, fRight, SLIDER_BASE + 16));
	if (fLeftX > 19) {
		StrokeLine(BPoint(fLeftX - 9, SLIDER_BASE + 3), BPoint(fLeftX - 6, 
			SLIDER_BASE + 3));
		StrokeLine(BPoint(fLeftX - 9, SLIDER_BASE + 4), BPoint(fLeftX - 7, 
			SLIDER_BASE + 4));
		StrokeLine(BPoint(fLeftX - 9, SLIDER_BASE + 5), BPoint(fLeftX - 8, 
			SLIDER_BASE + 5));
		StrokeLine(BPoint(fLeftX - 9, SLIDER_BASE + 16), BPoint(fLeftX - 6, 
			SLIDER_BASE + 16));
		StrokeLine(BPoint(fLeftX - 9, SLIDER_BASE + 15), BPoint(fLeftX - 7, 
			SLIDER_BASE + 15));
		StrokeLine(BPoint(fLeftX - 9, SLIDER_BASE + 14), BPoint(fLeftX - 8, 
			SLIDER_BASE + 14));
	}
	if (fRightX < fRight - 5) {
		StrokeLine(BPoint(fRightX + 5, SLIDER_BASE + 3), BPoint(fRightX + 8, 
			SLIDER_BASE + 3));
		StrokeLine(BPoint(fRightX + 7, SLIDER_BASE + 4), BPoint(fRightX + 8, 
			SLIDER_BASE + 4));
		StrokeLine(BPoint(fRightX + 8, SLIDER_BASE + 5), BPoint(fRightX + 8, 
			SLIDER_BASE + 6));
		StrokeLine(BPoint(fRightX + 8, SLIDER_BASE + 13), BPoint(fRightX + 8, 
			SLIDER_BASE + 14));
		StrokeLine(BPoint(fRightX + 5, SLIDER_BASE + 16), BPoint(fRightX + 8, 
			SLIDER_BASE + 16));
		StrokeLine(BPoint(fRightX + 7, SLIDER_BASE + 15), BPoint(fRightX + 8, 
			SLIDER_BASE + 15));
	}
	SetHighColor(144, 186, 136);
	FillRect(BRect(fLeftX + 1, SLIDER_BASE + 3, fRightX, SLIDER_BASE + 4));
	FillRect(BRect(fLeftX + 1, SLIDER_BASE + 5, fLeftX + 2, SLIDER_BASE + 16));
	SetHighColor(171, 221, 161);
	FillRect(BRect(fLeftX + 3, SLIDER_BASE + 5, fRightX, SLIDER_BASE + 16));
	
	int i = 17;
	int j = 18;
	SetHighColor(128, 128, 128);
	for (; i < fLeftX - 9; i += 6) {
		StrokeLine(BPoint(i, SLIDER_BASE + 7), BPoint(i, SLIDER_BASE + 13));
	}
	SetHighColor(179, 179, 179);
	for (; j < fLeftX - 9; j += 6) {
		StrokeLine(BPoint(j, SLIDER_BASE + 7), BPoint(j, SLIDER_BASE + 13));
	}
	
	while (i <= fLeftX)
		i += 6;
	while (j <= fLeftX)
		j += 6;
	
	SetHighColor(144, 186, 136);
	for (; i <= fRightX; i += 6) {
		StrokeLine(BPoint(i, SLIDER_BASE + 7), BPoint(i, SLIDER_BASE + 13));
	}		
	SetHighColor(189, 244, 178);
	for (; j <= fRightX; j += 6) {
		StrokeLine(BPoint(j, SLIDER_BASE + 7), BPoint(j, SLIDER_BASE + 13));
	}
	
	while (i <= fRightX + 9)
		i += 6;
	while (j <= fRightX + 9)
		j += 6;
	
	SetHighColor(128, 128, 128);
	for (; i <= fRight + 1; i += 6) {
		StrokeLine(BPoint(i, SLIDER_BASE + 7), BPoint(i, SLIDER_BASE + 13));
	}
	SetHighColor(179, 179, 179);
	for (; j <= fRight + 1; j += 6) {
		StrokeLine(BPoint(j, SLIDER_BASE + 7), BPoint(j, SLIDER_BASE + 13));
	}
	
	SetLowColor(HighColor());
	
	BPoint leftThumbPoint(fLeftX - 8, SLIDER_BASE + 3);
	DrawBitmapAsync(&leftThumbBitmap, BRect(BPoint(0, 0), 
		kLeftRightThumbSize - BPoint(7, 0)),
		BRect(leftThumbPoint, leftThumbPoint 
			+ kLeftRightThumbSize - BPoint(7, 0)));
	
	BPoint rightThumbPoint(fRightX, SLIDER_BASE + 3);
	DrawBitmapAsync(&rightThumbBitmap, BRect(BPoint(6, 0), 
		kLeftRightThumbSize), 
		BRect(rightThumbPoint, rightThumbPoint 
			+ kLeftRightThumbSize-BPoint(6, 0)));
	
	Sync();
}
