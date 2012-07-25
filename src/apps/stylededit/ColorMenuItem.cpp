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

ColorMenuItem::ColorMenuItem(const char	*label, rgb_color color,
	BMessage *message)
	: BMenuItem(label, message, 0, 0),
	fItemColor(color)
{
}


void
ColorMenuItem::DrawContent()
{
	BMenu* menu = Menu();
	if (menu) {
		rgb_color menuColor = menu->HighColor();

		menu->SetHighColor(fItemColor);
		BMenuItem::DrawContent();
		menu->SetHighColor(menuColor);
	}
}
