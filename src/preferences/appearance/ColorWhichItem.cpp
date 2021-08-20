/*
 * Copyright 2001-2016 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm, darkwyrm@earthlink.net
 *		Rene Gollent, rene@gollent.com
 *		Ryan Leavengood, leavengood@gmail.com
 *		John Scipione, jscipione@gmail.com
 */


#include "ColorWhichItem.h"

#include <math.h>

#include <ControlLook.h>


// golden ratio
#ifdef M_PHI
#	undef M_PHI
#endif
#define M_PHI 1.61803398874989484820


ColorWhichItem::ColorWhichItem(const char* text, color_which which,
	rgb_color color)
	:
	BStringItem(text, 0, false),
	fColorWhich(which),
	fColor(color)
{
}


void
ColorWhichItem::DrawItem(BView* owner, BRect frame, bool complete)
{
	rgb_color highColor = owner->HighColor();
	rgb_color lowColor = owner->LowColor();

	if (IsSelected() || complete) {
		if (IsSelected()) {
			owner->SetHighUIColor(B_LIST_SELECTED_BACKGROUND_COLOR);
			owner->SetLowColor(owner->HighColor());
		} else
			owner->SetHighColor(lowColor);

		owner->FillRect(frame);
	}

	float spacer = ceilf(be_control_look->DefaultItemSpacing() / 2);

	BRect colorRect(frame);
	colorRect.InsetBy(2.0f, 2.0f);
	colorRect.left += spacer;
	colorRect.right = colorRect.left + floorf(colorRect.Height() * M_PHI);
	owner->SetHighColor(fColor);
	owner->FillRect(colorRect);
	owner->SetHighUIColor(B_CONTROL_BORDER_COLOR);
	owner->StrokeRect(colorRect);

	owner->MovePenTo(colorRect.right + spacer, frame.top + BaselineOffset());

	if (!IsEnabled()) {
		rgb_color textColor = ui_color(B_LIST_ITEM_TEXT_COLOR);
		if (textColor.red + textColor.green + textColor.blue > 128 * 3)
			owner->SetHighColor(tint_color(textColor, B_DARKEN_2_TINT));
		else
			owner->SetHighColor(tint_color(textColor, B_LIGHTEN_2_TINT));
	} else {
		if (IsSelected())
			owner->SetHighUIColor(B_LIST_SELECTED_ITEM_TEXT_COLOR);
		else
			owner->SetHighUIColor(B_LIST_ITEM_TEXT_COLOR);
	}

	owner->DrawString(Text());

	owner->SetHighColor(highColor);
	owner->SetLowColor(lowColor);
}
