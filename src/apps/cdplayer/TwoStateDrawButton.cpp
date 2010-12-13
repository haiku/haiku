/*
 * Copyright (c) 2006-2007, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		DarkWyrm, bpmagic@columbus.rr.com
 */


#include "TwoStateDrawButton.h"


TwoStateDrawButton::TwoStateDrawButton(BRect frame, const char *name,
	BBitmap *upone, BBitmap *downone, BBitmap *uptwo, BBitmap *downtwo,
	BMessage *msg, const int32 &resize, const int32 &flags)
	: BButton(frame, name, "", msg, resize, flags),
 	fUpOne(upone),
	fDownOne(downone),
	fUpTwo(uptwo),
	fDownTwo(downtwo),
	fDisabledOne(NULL),
	fDisabledTwo(NULL),
	fButtonState(true)

{
	fButtonState = false;
}


TwoStateDrawButton::~TwoStateDrawButton()
{
	delete fUpOne;
	delete fDownOne;
	delete fUpTwo;
	delete fDownTwo;
	delete fDisabledOne;
	delete fDisabledTwo;
}


void
TwoStateDrawButton::ResizeToPreferred()
{
	if (fUpOne)
		ResizeTo(fUpOne->Bounds().Width(), fUpOne->Bounds().Height());
	else if (fDownOne)
		ResizeTo(fDownOne->Bounds().Width(), fDownOne->Bounds().Height());
	else if (fUpTwo)
		ResizeTo(fUpTwo->Bounds().Width(),fUpTwo->Bounds().Height());
	else if (fDownTwo)
		ResizeTo(fDownTwo->Bounds().Width(), fDownTwo->Bounds().Height());
	else if (fDisabledOne)
		ResizeTo(fDisabledOne->Bounds().Width(), fDisabledOne->Bounds().Height());
	else if(fDisabledTwo)
		ResizeTo(fDisabledTwo->Bounds().Width(), fDisabledTwo->Bounds().Height());
}


void
TwoStateDrawButton::SetBitmaps(BBitmap *upone, BBitmap *downone, BBitmap *uptwo, 
	BBitmap *downtwo)
{	
	delete fUpOne;
	delete fDownOne;
	delete fUpTwo;
	delete fDownTwo;

	fUpOne = upone;
	fDownOne = downone;
	fUpTwo = uptwo;
	fDownTwo = downtwo;
}


void
TwoStateDrawButton::SetBitmaps(const int32 state, BBitmap *up, BBitmap *down)
{
	if (state == 0) {
		delete fUpOne;
		delete fDownOne;
		fUpOne = up;
		fDownOne = down;
	} else {
		delete fUpTwo;
		delete fDownTwo;
		fUpTwo = up;
		fDownTwo = down;
	}
}


void
TwoStateDrawButton::SetDisabled(BBitmap *disabledone, BBitmap *disabledtwo)
{
	delete fDisabledOne;
	delete fDisabledTwo;
	
	fDisabledOne = disabledone;
	fDisabledTwo = disabledtwo;
}


void
TwoStateDrawButton::MouseUp(BPoint pt)
{
	BButton::MouseUp(pt);
	fButtonState = fButtonState ? false : true;
	Invalidate();
}


void
TwoStateDrawButton::SetState(int32 value)
{
	if (fButtonState != value) {
		fButtonState = (value == 0) ? false : true;
		Invalidate();
	}
}


void
TwoStateDrawButton::Draw(BRect update)
{
	if (fButtonState) {
		if (!IsEnabled()) {
			if (fDisabledTwo)
				DrawBitmap(fDisabledTwo, BPoint(0,0));
			else
				StrokeRect(Bounds());
			return;
		}
		
		if (Value() == B_CONTROL_ON) {
			if (fDownTwo)
				DrawBitmap(fDownTwo, BPoint(0,0));
			else
				StrokeRect(Bounds());
		} else {
			if (fUpTwo)
				DrawBitmap(fUpTwo, BPoint(0,0));
			else
				StrokeRect(Bounds());
		}
	} else {
		if (!IsEnabled()) {
			if (fDisabledOne)
				DrawBitmap(fDisabledOne, BPoint(0,0));
			else
				StrokeRect(Bounds());
			return;
		}

		if (Value() == B_CONTROL_ON) {
			if (fDownOne)
				DrawBitmap(fDownOne, BPoint(0,0));
			else
				StrokeRect(Bounds());
		} else {
			if (fUpOne)
				DrawBitmap(fUpOne, BPoint(0,0));
			else
				StrokeRect(Bounds());
		}
	}
}
