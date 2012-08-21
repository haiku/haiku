/*
 * Copyright 2002-2008, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm (darkwyrm@earthlink.net)
 *		Rene Gollent (rene@gollent.com)
 *		Ryan Leavengood <leavengood@gmail.com>
 */


#include "ColorWhichItem.h"

#include <stdio.h>


ColorWhichItem::ColorWhichItem(const char* text, color_which which,
		rgb_color color)
	:
	BStringItem(text, 0, false),
	fColorWhich(which),
	fColor(color)
{
}


void
ColorWhichItem::DrawItem(BView *owner, BRect frame, bool complete)
{
	rgb_color highColor = owner->HighColor();
	rgb_color lowColor = owner->LowColor();

	if (IsSelected() || complete) {
		if (IsSelected()) {
			owner->SetHighColor(ui_color(B_MENU_SELECTED_BACKGROUND_COLOR));
			owner->SetLowColor(owner->HighColor());
		} else
			owner->SetHighColor(lowColor);

		owner->FillRect(frame);
	}

	rgb_color border = (rgb_color){ 184, 184, 184, 255 };

	BRect colorRect(frame);
	colorRect.InsetBy(2, 2);
	colorRect.right = colorRect.left + colorRect.Height();
	owner->SetHighColor(fColor);
	owner->FillRect(colorRect);
	owner->SetHighColor(border);
	owner->StrokeRect(colorRect);

	owner->MovePenTo(frame.left + colorRect.Width() + 8, frame.top
		+ BaselineOffset());

	if (!IsEnabled()) {
		rgb_color textColor = ui_color(B_MENU_ITEM_TEXT_COLOR);
		if (textColor.red + textColor.green + textColor.blue > 128 * 3)
			owner->SetHighColor(tint_color(textColor, B_DARKEN_2_TINT));
		else
			owner->SetHighColor(tint_color(textColor, B_LIGHTEN_2_TINT));
	} else {
		if (IsSelected())
			owner->SetHighColor(ui_color(B_MENU_SELECTED_ITEM_TEXT_COLOR));
		else
			owner->SetHighColor(ui_color(B_MENU_ITEM_TEXT_COLOR));
	}

	owner->DrawString(Text());

	owner->SetHighColor(highColor);
	owner->SetLowColor(lowColor);
}


color_which
ColorWhichItem::ColorWhich(void)
{
	return fColorWhich;
}


void
ColorWhichItem::SetColor(rgb_color color)
{
	fColor = color;
}

