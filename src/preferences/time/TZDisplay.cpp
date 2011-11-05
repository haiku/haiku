/*
 * Copyright 2004-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew McCall <mccall@@digitalparadise.co.uk>
 *		Mike Berg <mike@berg-net.us>
 *		Julun <host.haiku@gmx.de>
 *		Hamish Morrison <hamish@lavabit.com>
 */

#include "TZDisplay.h"

#include <stdio.h>

#include <LayoutUtils.h>



TTZDisplay::TTZDisplay(const char* name, const char* label)
	:
	BView(name, B_WILL_DRAW),
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
	BSize size = _CalcPrefSize();
	ResizeTo(size.width, size.height);
}


void
TTZDisplay::Draw(BRect)
{
	SetLowColor(ViewColor());

	BRect bounds = Bounds();
	FillRect(Bounds(), B_SOLID_LOW);

	font_height height;
	GetFontHeight(&height);
	float fontHeight = ceilf(height.descent + height.ascent +
		height.leading);

	BPoint pt(bounds.left, ceilf(bounds.top + height.ascent));
	DrawString(fLabel.String(), pt);

	pt.y += fontHeight;
	DrawString(fText.String(), pt);

	pt.y -= fontHeight;
	pt.x = bounds.right - StringWidth(fTime.String());
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
	InvalidateLayout();
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
	InvalidateLayout();
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
	InvalidateLayout();
}


BSize
TTZDisplay::MaxSize()
{
	BSize size = _CalcPrefSize();
	size.width = B_SIZE_UNLIMITED;
	
	return BLayoutUtils::ComposeSize(ExplicitMaxSize(),
		size);
}


BSize
TTZDisplay::MinSize()
{
	return BLayoutUtils::ComposeSize(ExplicitMinSize(),
		_CalcPrefSize());
}


BSize
TTZDisplay::PreferredSize()
{
	return BLayoutUtils::ComposeSize(ExplicitPreferredSize(),
		_CalcPrefSize());
}


BSize
TTZDisplay::_CalcPrefSize()
{
	font_height fontHeight;
	GetFontHeight(&fontHeight);
	
	BSize size;
	size.height = 2 * ceilf(fontHeight.ascent + fontHeight.descent +
		fontHeight.leading);
	
	// Add a little padding
	float padding = 10.0;
	float firstLine = ceilf(StringWidth(fLabel.String()) +
		StringWidth(" ") + StringWidth(fTime.String()) + padding);
	float secondLine = ceilf(StringWidth(fText.String()) + padding);
	size.width = firstLine > secondLine ? firstLine : secondLine;

	return size;
}
