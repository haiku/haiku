/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Copyright 2008-2009, Pier Luigi Fiorini. All Rights Reserved.
 * Copyright 2004-2008, Michael Davidson. All Rights Reserved.
 * Copyright 2004-2007, Mikael Eiman. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Davidson, slaad@bong.com.au
 *		Mikael Eiman, mikael@eiman.tv
 *		Pier Luigi Fiorini, pierluigi.fiorini@gmail.com
 */

#include "BorderView.h"

const float kBorderWidth	= 2.0f;
const int32 kTitleSize		= 14;


BorderView::BorderView(BRect rect, const char* text)
	:
	BView(rect, "NotificationBorderView", B_FOLLOW_ALL,
		B_WILL_DRAW | B_FRAME_EVENTS),
	fTitle(text)
{
	SetViewColor(B_TRANSPARENT_COLOR);
}


BorderView::~BorderView()
{
}


void
BorderView::FrameResized(float, float)
{
	Invalidate();
}


void
BorderView::Draw(BRect rect)
{
	rgb_color col_bg = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color col_text = tint_color(col_bg, B_LIGHTEN_MAX_TINT);
	rgb_color col_text_shadow = tint_color(col_bg, B_DARKEN_MAX_TINT);

	// Background
	SetDrawingMode(B_OP_COPY);
	SetHighColor(col_bg);
	FillRect(rect);

	// Text
	SetLowColor(col_bg);
	SetDrawingMode(B_OP_ALPHA);
	SetFont(be_bold_font);
	SetFontSize(kTitleSize);

	BFont font;
	GetFont(&font);

	font_height fh;
	font.GetHeight(&fh);

	// float line_height = fh.ascent + fh.descent + fh.leading;

	float text_pos = fh.ascent;

	SetHighColor(col_text);
	DrawString(fTitle.String(), BPoint(kBorderWidth, text_pos));

	SetHighColor(col_text_shadow);
	DrawString(fTitle.String(), BPoint(kBorderWidth + 1, text_pos + 1));

	// Content border
	SetHighColor(tint_color(col_bg, B_DARKEN_2_TINT));

	BRect content = Bounds();
//	content.InsetBy(kBorderWidth, kBorderWidth);
	content.top += text_pos + fh.descent + 2;

	BRect content_line(content);
//	content_line.InsetBy(-1, -1);

	StrokeRect(content_line);
}


void
BorderView::GetPreferredSize(float* w, float* h)
{
	SetFont(be_bold_font);
	SetFontSize(kTitleSize);

	BFont font;
	GetFont(&font);

	font_height fh;
	font.GetHeight(&fh);

	float line_height = fh.ascent + fh.descent;

	*w = 2 * kBorderWidth + StringWidth(fTitle.String());
	*h = 2 * kBorderWidth + line_height + 3;
	*w = *h = 0;
}


float
BorderView::BorderSize()
{
	return 0;
}
