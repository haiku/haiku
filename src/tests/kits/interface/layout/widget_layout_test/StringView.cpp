/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "StringView.h"

#include <math.h>

#include <View.h>


StringView::StringView(const char* string)
	: View(),
	  fString(string),
	  fAlignment(B_ALIGN_LEFT),
	  fStringAscent(0),
	  fStringDescent(0),
	  fStringWidth(0),
	  fExplicitMinSize(B_SIZE_UNSET, B_SIZE_UNSET)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fTextColor = (rgb_color){ 0, 0, 0, 255 };
}


void
StringView::SetString(const char* string)
{
	fString = string;

	_UpdateStringMetrics();
	Invalidate();
	InvalidateLayout();
}


void
StringView::SetAlignment(alignment align)
{
	if (align != fAlignment) {
		fAlignment = align;
		Invalidate();
	}
}


void
StringView::SetTextColor(rgb_color color)
{
	fTextColor = color;
	Invalidate();
}


BSize
StringView::MinSize()
{
	BSize size(fExplicitMinSize);
	if (!size.IsWidthSet())
		size.width = fStringWidth - 1;
	if (!size.IsHeightSet())
		size.height = fStringAscent + fStringDescent - 1;
	return size;
}


void
StringView::SetExplicitMinSize(BSize size)
{
	fExplicitMinSize = size;
}


void
StringView::AddedToContainer()
{
	_UpdateStringMetrics();
}


void
StringView::Draw(BView* container, BRect updateRect)
{
	BSize size(Size());
	int widthDiff = (int)size.width + 1 - (int)fStringWidth;
	int heightDiff = (int)size.height + 1
		- (int)(fStringAscent + (int)fStringDescent);
	BPoint base;

	// horizontal alignment
	switch (fAlignment) {
		case B_ALIGN_RIGHT:
			base.x = widthDiff;
			break;
		case B_ALIGN_LEFT:
		default:
			base.x = 0;
			break;
	}

	base.y = heightDiff / 2 + fStringAscent;

	container->SetHighColor(fTextColor);
	container->DrawString(fString.String(), base);
}


void
StringView::_UpdateStringMetrics()
{
	BView* container = Container();
	if (!container)
		return;

	BFont font;
	container->GetFont(&font);

	font_height fh;
	font.GetHeight(&fh);

	fStringAscent = ceilf(fh.ascent);
	fStringDescent = ceilf(fh.descent);
	fStringWidth = font.StringWidth(fString.String());
}
