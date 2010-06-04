/*
 * Copyright 2005, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Inspired by SoundCapture from Be newsletter (Media Kit Basics: Consumers 
 * and Producers)
 */
#include "UpDownButton.h"
#include "icon_button.h"

UpDownButton::UpDownButton(BRect _rect, BMessage *msg, uint32 resizeFlags)
	: BControl(_rect, "button", NULL, msg, resizeFlags, B_WILL_DRAW),
	fLastValue(B_CONTROL_ON)
{
	BRect rect = BRect(0, 0, kUpDownButtonWidth - 1, kUpDownButtonHeight - 1);
	fBitmapUp = new BBitmap(rect, B_CMAP8);
	fBitmapUp->SetBits(kButtonUpBits, kUpDownButtonWidth * kUpDownButtonHeight,
		0, B_CMAP8);
	fBitmapDown = new BBitmap(rect, B_CMAP8);
	fBitmapDown->SetBits(kButtonDownBits, kUpDownButtonWidth 
		* kUpDownButtonHeight, 0, B_CMAP8);
	fBitmapMiddle = new BBitmap(rect, B_CMAP8);
	fBitmapMiddle->SetBits(kButtonMiddleBits, kUpDownButtonWidth 
		* kUpDownButtonHeight, 0, B_CMAP8);
}


UpDownButton::~UpDownButton()
{
	delete fBitmapUp;
	delete fBitmapDown;
	delete fBitmapMiddle;
}


void
UpDownButton::Draw(BRect updateRect)
{
	SetDrawingMode(B_OP_OVER);
	
	if(IsTracking()) {
		if((Bounds().top + Bounds().Height()/2) > (fTrackingY + 3))
			DrawBitmap(fBitmapUp);
		else if((Bounds().top + Bounds().Height()/2) < (fTrackingY - 3))
			DrawBitmap(fBitmapDown);
		else
			DrawBitmap(fBitmapMiddle);
	} else {
		if(Value()==B_CONTROL_OFF)
			DrawBitmap(fBitmapUp);
		else
			DrawBitmap(fBitmapDown);	
	}
	
	SetDrawingMode(B_OP_COPY);
}


void
UpDownButton::MouseDown(BPoint point)
{
	if(!IsEnabled())
		return;
	
	fLastValue = Value();
	fTrackingY = (Bounds().top + Bounds().Height()/2);
	
	SetTracking(true);
	SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
	
	SetValue(Value() == B_CONTROL_ON ? B_CONTROL_OFF : B_CONTROL_ON);	
}


void 
UpDownButton::MouseMoved(BPoint point, uint32 transit, const BMessage *message)
{
	if (!IsTracking())
		return;
		
	fTrackingY = point.y;
	Draw(Bounds());
	Flush();
}


void
UpDownButton::MouseUp(BPoint point)
{
	if (!IsTracking())
		return;
	
	if((Bounds().top + Bounds().Height()/2) > (fTrackingY + 3))
		SetValue(B_CONTROL_ON);
	else if((Bounds().top + Bounds().Height()/2) < (fTrackingY - 3))
		SetValue(B_CONTROL_OFF);
	
	if(Value()!=fLastValue)
		Invoke();
	SetTracking(false);
	Draw(Bounds());
	Flush();
	fLastValue = Value();
}
