/*
 * Copyright 2016-2022 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Scipione, jscipione@gmail.com
 *
 * Based on ColorWhichItem by DarkWyrm (bpmagic@columbus.rr.com)
 */


#include "ColorItem.h"

#include <math.h>

#include <ControlLook.h>
#include <View.h>


// golden ratio
#ifdef M_PHI
#	undef M_PHI
#endif
#define M_PHI 1.61803398874989484820


//	#pragma mark - ColorItem


ColorItem::ColorItem(const char* string, rgb_color color)
	:
	BStringItem(string, 0, false),
	fColor(color)
{
}


void
ColorItem::DrawItem(BView* owner, BRect frame, bool complete)
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

	// draw the colored box
	owner->SetHighColor(fColor);
	owner->FillRect(colorRect);

	// draw the border
	owner->SetHighUIColor(B_CONTROL_BORDER_COLOR);
	owner->StrokeRect(colorRect);

	// draw the string
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
