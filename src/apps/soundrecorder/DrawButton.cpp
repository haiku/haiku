/*
 * Copyright 2005, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Reworked from DarkWyrm version in CDPlayer
 */
 
#include "DrawButton.h"
#include "DrawingTidbits.h"

DrawButton::DrawButton(BRect frame, const char *name, const unsigned char *on, 
	const unsigned char *off, BMessage *msg, int32 resize, int32 flags)
	: BControl(frame, name, "", msg, resize, flags | B_WILL_DRAW),
		fOn(frame, B_CMAP8),
		fOff(frame, B_CMAP8),
		fButtonState(false)
{
	fOff.SetBits(off, (frame.Width() + 1) * (frame.Height() + 1), 0, B_CMAP8);
	fOn.SetBits(on, (frame.Width() + 1) * (frame.Height() + 1), 0, B_CMAP8);
}


DrawButton::~DrawButton(void)
{
}


void
DrawButton::AttachedToWindow()
{
	SetViewColor(B_TRANSPARENT_COLOR);
	ReplaceTransparentColor(&fOn, Parent()->ViewColor());
	ReplaceTransparentColor(&fOff, Parent()->ViewColor());
}


void 
DrawButton::MouseUp(BPoint pt)
{
	fButtonState = fButtonState ? false : true;
	Invalidate();
	Invoke();
}


void
DrawButton::Draw(BRect update)
{	
	if (fButtonState) {
		DrawBitmap(&fOn, BPoint(0,0));
	} else {
		DrawBitmap(&fOff, BPoint(0,0));
	}
}
