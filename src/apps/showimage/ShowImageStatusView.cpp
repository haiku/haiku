/*****************************************************************************/
// ShowImageStatusView
// Written by Fernando Francisco de Oliveira, Michael Wilber
//
// ShowImageStatusView.cpp
//
//
// Copyright (c) 2003 OpenBeOS Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/

#include "ShowImageStatusView.h"

ShowImageStatusView::ShowImageStatusView(BRect rect, const char* name,
	uint32 resizingMode, uint32 flags)
	: BView(rect, name, resizingMode, flags)
{
	SetViewColor(B_TRANSPARENT_32_BIT);
	SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetHighColor(0, 0, 0, 255);

	BFont font;
	GetFont(&font);
	font.SetSize(10.0);
	SetFont(&font);
}

void
ShowImageStatusView::Draw(BRect updateRect)
{
	rgb_color darkShadow = tint_color(LowColor(), B_DARKEN_2_TINT);
	rgb_color shadow = tint_color(LowColor(), B_DARKEN_1_TINT);
	rgb_color light = tint_color(LowColor(), B_LIGHTEN_MAX_TINT);

	BRect b(Bounds());

	BeginLineArray(5);
		AddLine(BPoint(b.left, b.top),
				BPoint(b.right, b.top), darkShadow);
		b.top += 1.0;
		AddLine(BPoint(b.left, b.top),
				BPoint(b.right, b.top), light);
		AddLine(BPoint(b.right, b.top + 1.0),
				BPoint(b.right, b.bottom), shadow);
		AddLine(BPoint(b.right - 1.0, b.bottom),
				BPoint(b.left + 1.0, b.bottom), shadow);
		AddLine(BPoint(b.left, b.bottom),
				BPoint(b.left, b.top + 1.0), light);
	EndLineArray();

	b.InsetBy(1.0, 1.0);

	// truncate and layout text
	BString truncated(fText);
	BFont font;
	GetFont(&font);
	font.TruncateString(&truncated, B_TRUNCATE_MIDDLE, b.Width() - 4.0);
	font_height fh;
	font.GetHeight(&fh);

	FillRect(b, B_SOLID_LOW);
	SetDrawingMode(B_OP_OVER);
	DrawString(truncated.String(), BPoint(b.left + 2.0,
										  floorf(b.top + b.Height() / 2.0
										  		+ fh.ascent / 2.0)));
}

void
ShowImageStatusView::SetText(BString &text)
{
	fText = text;
	Invalidate();
}
