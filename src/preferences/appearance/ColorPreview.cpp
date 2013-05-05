/*
 * Copyright 2002-2013 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm, darkwyrm@earthlink.net
 *		John Scipione, jscipione@gmail.com
 */


#include "ColorPreview.h"


ColorPreview::ColorPreview(BRect frame, BMessage* message,
	uint32 resizingMode, uint32 flags)
	:
	BView(frame,"ColorPreview", resizingMode, flags | B_WILL_DRAW)
{
	SetViewColor(B_TRANSPARENT_COLOR);
	SetLowColor(0, 0, 0);
	invoker = new BInvoker(message, this);
	disabledcol.red = 128;
	disabledcol.green = 128;
	disabledcol.blue = 128;
	disabledcol.alpha = 255;
	is_enabled = true;
	is_rect = true;
}


ColorPreview::~ColorPreview(void)
{
	delete invoker;
}


void
ColorPreview::Draw(BRect update)
{
	rgb_color color;
	if (is_enabled)
		color = currentcol;
	else
		color = disabledcol;

	if (is_rect) {
		if (is_enabled) {
			rgb_color background = ui_color(B_PANEL_BACKGROUND_COLOR);
			rgb_color shadow = tint_color(background, B_DARKEN_1_TINT);
			rgb_color darkShadow = tint_color(background, B_DARKEN_3_TINT);
			rgb_color light = tint_color(background, B_LIGHTEN_MAX_TINT);

			BRect bounds(Bounds());

			BeginLineArray(4);
			AddLine(BPoint(bounds.left, bounds.bottom),
			BPoint(bounds.left, bounds.top), shadow);
			AddLine(BPoint(bounds.left + 1.0, bounds.top),
			BPoint(bounds.right, bounds.top), shadow);
			AddLine(BPoint(bounds.right, bounds.top + 1.0),
			BPoint(bounds.right, bounds.bottom), light);
			AddLine(BPoint(bounds.right - 1.0, bounds.bottom),
			BPoint(bounds.left + 1.0, bounds.bottom), light);
			EndLineArray();
			bounds.InsetBy(1.0, 1.0);

			BeginLineArray(4);
			AddLine(BPoint(bounds.left, bounds.bottom),
			BPoint(bounds.left, bounds.top), darkShadow);
			AddLine(BPoint(bounds.left + 1.0, bounds.top),
			BPoint(bounds.right, bounds.top), darkShadow);
			AddLine(BPoint(bounds.right, bounds.top + 1.0),
			BPoint(bounds.right, bounds.bottom), background);
			AddLine(BPoint(bounds.right - 1.0, bounds.bottom),
			BPoint(bounds.left + 1.0, bounds.bottom), background);
			EndLineArray();
			bounds.InsetBy(1.0, 1.0);

			SetHighColor(color);
			FillRect(bounds);
		} else {
			SetHighColor(color);
			FillRect(Bounds());
		}
	} else {
		// fill background
		SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		FillRect(update);

		SetHighColor(color);
		FillEllipse(Bounds());
		if (is_enabled)
			StrokeEllipse(Bounds(), B_SOLID_LOW);
	}
}


void
ColorPreview::MessageReceived(BMessage* message)
{
	// If we received a dropped message, see if it contains color data
	if (message->WasDropped()) {
		rgb_color* col;
		uint8* ptr;
		ssize_t size;
		if (message->FindData("RGBColor", (type_code)'RGBC',
				(const void**)&ptr,&size) == B_OK) {
			col = (rgb_color*)ptr;
			SetHighColor(*col);
		}
	}

	BView::MessageReceived(message);
}


void
ColorPreview::SetEnabled(bool value)
{
	if (is_enabled != value) {
		is_enabled = value;
		Invalidate();
	}
}


void
ColorPreview::SetTarget(BHandler* target)
{
	invoker->SetTarget(target);
}


rgb_color
ColorPreview::Color(void) const
{
	return currentcol;
}


void
ColorPreview::SetColor(rgb_color col)
{
	SetHighColor(col);
	currentcol = col;
	Draw(Bounds());
	invoker->Invoke();
}


void
ColorPreview::SetColor(uint8 r,uint8 g, uint8 b)
{
	SetHighColor(r,g,b);
	currentcol.red = r;
	currentcol.green = g;
	currentcol.blue = b;
	Draw(Bounds());
	invoker->Invoke();
}


void
ColorPreview::SetMode(bool is_rectangle)
{
	is_rect = is_rectangle;
}
