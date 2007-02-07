/*
 * Copyright (c) 2006-2007, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 */
#include "DrawButton.h"

DrawButton::DrawButton(BRect frame, const char *name, BBitmap *up, BBitmap *down,
						BMessage *msg, int32 resize, int32 flags)
 :	BButton(frame, name, "", msg, resize, flags)
{
	fUp = up;
	fDown = down;
	fDisabled = NULL;
}


DrawButton::~DrawButton(void)
{
	delete fUp;
	delete fDown;
	delete fDisabled;
}


void
DrawButton::SetBitmaps(BBitmap *up, BBitmap *down)
{
	delete fUp;
	delete fDown;
	
	fUp = up;
	fDown = down;
	
	if (IsEnabled())
		Invalidate();
}


void
DrawButton::SetDisabled(BBitmap *disabled)
{
	delete fDisabled;
	
	fDisabled = disabled;
	
	if (!IsEnabled())
		Invalidate();
}


void
DrawButton::Draw(BRect update)
{
	if (!IsEnabled()) {
		if (fDisabled)
			DrawBitmap(fDisabled, BPoint(0,0));
		else
			StrokeRect(Bounds());
		return;
	}
	
	if (Value() == B_CONTROL_ON) {
		if (fDown)
			DrawBitmap(fDown, BPoint(0,0));
		else
			StrokeRect(Bounds());
	} else {
		if (fUp)
			DrawBitmap(fUp, BPoint(0,0));
		else
			StrokeRect(Bounds());
	}
}


void
DrawButton::ResizeToPreferred(void)
{
	if (fUp)
		ResizeTo(fUp->Bounds().Width(),fUp->Bounds().Height());
	else if (fDown)
		ResizeTo(fDown->Bounds().Width(),fDown->Bounds().Height());
	else if (fDisabled)
		ResizeTo(fDisabled->Bounds().Width(),fDisabled->Bounds().Height());
}
