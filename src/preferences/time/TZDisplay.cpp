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
		float height = ceil(fontHeight.descent + fontHeight.ascent
			+ fontHeight.leading);
		return height;
	}
}


TTZDisplay::TTZDisplay(BRect frame, const char* name, const char* label)
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
	ResizeTo(Bounds().Width(), _FontHeight() * 2.0 + 4.0);
}


void
TTZDisplay::Draw(BRect /* updateRect */)
{
	SetLowColor(ViewColor());

	BRect bounds = Bounds();
	FillRect(Bounds(), B_SOLID_LOW);

	float fontHeight = _FontHeight();

	BPoint pt(bounds.left + 2.0, fontHeight / 2.0 + 2.0);
	DrawString(fLabel.String(), pt);

	pt.y += fontHeight;
	DrawString(fText.String(), pt);

	pt.y -= fontHeight;
	pt.x = bounds.right - StringWidth(fTime.String()) - 2.0;
	DrawString(fTime.String(), pt);
}


const char*
TTZDisplay::Label() const
{
	return fLabel.String();
}


void
TTZDisplay::SetLabel(const char* label)
{
	fLabel.SetTo(label);
	Invalidate();
}


const char*
TTZDisplay::Text() const
{
	return fText.String();
}


void
TTZDisplay::SetText(const char* text)
{
	fText.SetTo(text);
	Invalidate();
}


const char*
TTZDisplay::Time() const
{
	return fTime.String();
}


void
TTZDisplay::SetTime(const char* time)
{
	fTime.SetTo(time);
	Invalidate();
}

