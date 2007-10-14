/*
 * Copyright 2004-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew McCall <mccall@@digitalparadise.co.uk>
 *		Mike Berg <mike@berg-net.us>
 *		Julun <host.haiku@gmx.de>
 *
 */

#include <stdio.h>

#include "TZDisplay.h"


namespace {
	float _FontHeight()
	{
		font_height fontHeight;
		be_plain_font->GetHeight(&fontHeight);
		float height = ceil(fontHeight.descent) + ceil(fontHeight.ascent) 
			+ ceil(fontHeight.leading);
		return height;
	}
}


TTZDisplay::TTZDisplay(BRect frame, const char *name, const char *label)
	: BView(frame, name, B_FOLLOW_NONE, B_WILL_DRAW),
	  fLabel(label),
	  fText(""),
	  fTime("")
{
}


TTZDisplay::~TTZDisplay()
{
}


void
TTZDisplay::AttachedToWindow()
{
	if (Parent())
		SetViewColor(Parent()->ViewColor());
}


void
TTZDisplay::ResizeToPreferred()
{
	float height = _FontHeight();
	ResizeTo(Bounds().Width(), height *2);
}


void
TTZDisplay::Draw(BRect /* updateRect */)
{
	BRect bounds(Bounds());
	SetLowColor(ViewColor());
	FillRect(bounds, B_SOLID_LOW);
	
	float height = _FontHeight();

	BPoint drawpt(bounds.left +2, height /2.0 +1);
	DrawString(fLabel.String(), drawpt);

	drawpt.y += height +2;
	DrawString(fText.String(), drawpt);
	
	drawpt.x = bounds.right -be_plain_font->StringWidth(fTime.String()) - 2;
	DrawString(fTime.String(), drawpt);
}


const char*
TTZDisplay::Label() const
{
	return fLabel.String();
}


void
TTZDisplay::SetLabel(const char *label)
{
	fLabel.SetTo(label);
	Draw(Bounds());
}


const char*
TTZDisplay::Text() const
{
	return fText.String();
}


void
TTZDisplay::SetText(const char *text)
{
	fText.SetTo(text);
	Draw(Bounds());
}


const char*
TTZDisplay::Time() const
{
	return fTime.String();
}


void
TTZDisplay::SetTime(int32 hour, int32 minute)
{
	int32 ahour = hour;
	if (hour > 12)
		ahour = hour -12;
		
	if (ahour == 0)
		ahour = 12;

	char *ap = "AM";
	if (hour > 11)
		ap = "PM";

	char buffer[32];
	snprintf(buffer, sizeof(buffer), "%ld:%02ld %s", ahour, minute, ap);

	fTime.SetTo(buffer);

	Invalidate();
}

