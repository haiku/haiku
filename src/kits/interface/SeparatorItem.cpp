/*
 * Copyright (c) 2001-2014 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Stefano Ceccherini, burton666@libero.it
 *		Marc Flerackers, mflerackers@androme.be
 *		Bill Hayden, haydentech@users.sourceforge.net
 */


#include <SeparatorItem.h>

#include <Font.h>


BSeparatorItem::BSeparatorItem()
	:
	BMenuItem("", NULL)
{
	BMenuItem::SetEnabled(false);
}


BSeparatorItem::BSeparatorItem(BMessage* data)
	:
	BMenuItem(data)
{
	BMenuItem::SetEnabled(false);
}


BSeparatorItem::~BSeparatorItem()
{
}


status_t
BSeparatorItem::Archive(BMessage* data, bool deep) const
{
	return BMenuItem::Archive(data, deep);
}


BArchivable*
BSeparatorItem::Instantiate(BMessage* data)
{
	if (validate_instantiation(data, "BSeparatorItem"))
		return new BSeparatorItem(data);

	return NULL;
}


void
BSeparatorItem::SetEnabled(bool enable)
{
	// Don't do anything - we don't want to get enabled ever
}


void
BSeparatorItem::GetContentSize(float* _width, float* _height)
{
	if (Menu() != NULL && Menu()->Layout() == B_ITEMS_IN_ROW) {
		if (_width != NULL)
			*_width = 2.0;

		if (_height != NULL)
			*_height = 2.0;
	} else {
		if (_width != NULL)
			*_width = 2.0;

		if (_height != NULL) {
			BFont font(be_plain_font);
			if (Menu() != NULL)
				Menu()->GetFont(&font);

			const float height = floorf((font.Size() * 0.8) / 2) * 2;
			*_height = max_c(4, height);
		}
	}
}


void
BSeparatorItem::Draw()
{
	BMenu *menu = Menu();
	if (menu == NULL)
		return;

	BRect bounds = Frame();
	rgb_color oldColor = menu->HighColor();
	rgb_color lowColor = menu->LowColor();

	if (menu->Layout() == B_ITEMS_IN_ROW) {
		const float startLeft = bounds.left + (floor(bounds.Width())) / 2;
		menu->SetHighColor(tint_color(lowColor, B_DARKEN_1_TINT));
		menu->StrokeLine(BPoint(startLeft, bounds.top + 1.0f),
			BPoint(startLeft, bounds.bottom - 1.0f));
		menu->SetHighColor(tint_color(lowColor, B_LIGHTEN_2_TINT));
		menu->StrokeLine(BPoint(startLeft + 1.0f, bounds.top + 1.0f),
			BPoint(startLeft + 1.0f, bounds.bottom - 1.0f));
	} else {
		const float startTop = bounds.top + (floor(bounds.Height())) / 2;
		menu->SetHighColor(tint_color(lowColor, B_DARKEN_1_TINT));
		menu->StrokeLine(BPoint(bounds.left + 1.0f, startTop),
			BPoint(bounds.right - 1.0f, startTop));
		menu->SetHighColor(tint_color(lowColor, B_LIGHTEN_2_TINT));
		menu->StrokeLine(BPoint(bounds.left + 1.0f, startTop + 1.0f),
			BPoint(bounds.right - 1.0f, startTop + 1.0f));
	}
	menu->SetHighColor(oldColor);
}


//	#pragma mark - private


void BSeparatorItem::_ReservedSeparatorItem1() {}
void BSeparatorItem::_ReservedSeparatorItem2() {}


BSeparatorItem &
BSeparatorItem::operator=(const BSeparatorItem &)
{
	return *this;
}
