/*
 * Copyright 2002-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mattias Sundblad
 *		Andrew Bachmann
 */

#include "ColorMenuItem.h"

#include <Message.h>
#include <Rect.h>


ColorMenuItem::ColorMenuItem(const char* label, rgb_color color,
	BMessage *message)
	:
	BMenuItem(label, message, 0, 0),
	fItemColor(color)
{
	message->AddData("color", B_RGB_COLOR_TYPE, &color, sizeof(rgb_color));
}


void
ColorMenuItem::DrawContent()
{
	BMenu* menu = Menu();
	if (menu) {
		rgb_color menuColor = menu->HighColor();
		BRect colorSquare(Frame());

		if (colorSquare.Width() > colorSquare.Height()) {
			// large button
			colorSquare.left += 8;
			colorSquare.top += 2;
			colorSquare.bottom -= 2;
		}
		colorSquare.right = colorSquare.left + colorSquare.Height();
		if (IsMarked()) {
			menu->SetHighColor(ui_color(B_NAVIGATION_BASE_COLOR));
			menu->StrokeRect(colorSquare);
		}
		colorSquare.InsetBy(1, 1);
		menu->SetHighColor(fItemColor);
		menu->FillRect(colorSquare);
		menu->SetHighColor(menuColor);
		menu->MovePenBy(colorSquare.right + 5.0f, 4.0f);
		BMenuItem::DrawContent();
	}
}
