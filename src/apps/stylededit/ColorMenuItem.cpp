/*
 * Copyright 2002-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mattias Sundblad
 *		Andrew Bachmann
 */

#include "ColorMenuItem.h"

#include <ControlLook.h>
#include <Message.h>
#include <Rect.h>


ColorMenuItem::ColorMenuItem(const char* label, rgb_color color, BMessage* message)
	:
	BMenuItem(label, message),
	fItemColor(color)
{
	message->AddData("color", B_RGB_COLOR_TYPE, &color, sizeof(rgb_color));
}


void
ColorMenuItem::Draw()
{
	BMenu* menu = Menu();
	if (menu == NULL)
		return;

	// save parent menu colors
	const color_which lowColor = menu->LowUIColor();
	const color_which highColor = menu->HighUIColor();

	// draw content
	menu->MovePenTo(ContentLocation());
	DrawContent();

	// restore the parent menu colors
	menu->SetLowUIColor(lowColor);
	menu->SetHighUIColor(highColor);
}


void
ColorMenuItem::DrawContent()
{
	BMenu* menu = Menu();
	if (menu == NULL)
		return;

	BRect colorSquare(Frame());
	if (colorSquare.Width() > colorSquare.Height()) {
		// large button
		colorSquare.left += 8;
		colorSquare.top += 2;
		colorSquare.bottom -= 2;
	}

	// make square
	colorSquare.right = colorSquare.left + colorSquare.Height();

	// push square down by 2px
	colorSquare.OffsetBy(0, 2);

	// draw border
	if (IsMarked()) {
		// extra border for selected color
		menu->SetHighColor(ui_color(B_NAVIGATION_BASE_COLOR));
		menu->StrokeRect(colorSquare);
		colorSquare.InsetBy(1, 1);
		menu->SetHighColor(ui_color(B_MENU_BACKGROUND_COLOR));
		menu->StrokeRect(colorSquare);
	} else {
		float tint = ui_color(B_MENU_ITEM_TEXT_COLOR).IsDark() ? 0.65 : B_DARKEN_2_TINT;
		menu->SetHighColor(tint_color(ui_color(B_MENU_ITEM_TEXT_COLOR), tint));
		menu->StrokeRect(colorSquare);
	}

	// fill the square with color
	colorSquare.InsetBy(1, 1);
	menu->SetHighColor(fItemColor);
	menu->FillRect(colorSquare);

	// setup label drawing state
	menu->SetHighColor(ui_color(B_MENU_ITEM_TEXT_COLOR));
	float spacing = roundf(be_control_look->DefaultLabelSpacing() / 2);
	float leftOffset = colorSquare.Width() + spacing;
	font_height fontHeight;
	menu->GetFontHeight(&fontHeight);
	float topOffset = fontHeight.ascent + 2; // push label down by 2px too
	menu->MovePenBy(leftOffset, topOffset);

	// draw label
	menu->DrawString(Label());
}
