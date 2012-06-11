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
			owner->SetHighColor(tint_color(lowColor, B_DARKEN_2_TINT));
			owner->SetLowColor(owner->HighColor());
		} else
			owner->SetHighColor(lowColor);

		owner->FillRect(frame);
	}

	rgb_color black = {0, 0, 0, 255};

	BRect colorRect(frame);
	colorRect.InsetBy(2, 2);
	colorRect.right = colorRect.left + colorRect.Height();
	owner->SetHighColor(fColor);
	owner->FillRect(colorRect);
	owner->SetHighColor(black);
	owner->StrokeRect(colorRect);

	owner->MovePenTo(frame.left + colorRect.Width() + 8, frame.top
		+ BaselineOffset());

	if (!IsEnabled())
		owner->SetHighColor(tint_color(black, B_LIGHTEN_2_TINT));
	else
		owner->SetHighColor(black);

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

