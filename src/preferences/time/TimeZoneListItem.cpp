/*
 * Copyright 2010-2013 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus, superstippi@gmx.de
 *		Adrien Destugues, pulkomandy@pulkomandy.ath.cx
 *		Axel Dörfler, axeld@pinc-software.de
 *		John Scipione, jscipione@gmail.com
 *		Oliver Tappe, zooey@hirschkaefer.de
 */


#include "TimeZoneListItem.h"

#include <new>

#include <Bitmap.h>
#include <Country.h>
#include <ControlLook.h>
#include <String.h>
#include <TimeZone.h>
#include <Window.h>


static const BString skDefaultString;


TimeZoneListItem::TimeZoneListItem(const char* text, BCountry* country,
	BTimeZone* timeZone)
	:
	BStringItem(text, 0, false),
	fCountry(country),
	fTimeZone(timeZone),
	fIcon(NULL)
{
}


TimeZoneListItem::~TimeZoneListItem()
{
	delete fCountry;
	delete fTimeZone;
	delete fIcon;
}


void
TimeZoneListItem::DrawItem(BView* owner, BRect frame, bool complete)
{
	if (fIcon != NULL && fIcon->IsValid()) {
		float iconSize = fIcon->Bounds().Width();
		_DrawItemWithTextOffset(owner, frame, complete,
			iconSize + be_control_look->DefaultLabelSpacing());

		BRect iconFrame(frame.left + be_control_look->DefaultLabelSpacing(),
			frame.top,
			frame.left + iconSize - 1 + be_control_look->DefaultLabelSpacing(),
			frame.top + iconSize - 1);
		owner->SetDrawingMode(B_OP_OVER);
		owner->DrawBitmap(fIcon, iconFrame);
		owner->SetDrawingMode(B_OP_COPY);
	} else
		_DrawItemWithTextOffset(owner, frame, complete, 0);
}


void
TimeZoneListItem::Update(BView* owner, const BFont* font)
{
	float oldIconSize = Height();
	BStringItem::Update(owner, font);
	if (!HasCountry())
		return;

	float iconSize = Height();
	if (iconSize == oldIconSize && fIcon != NULL)
		return;

	SetWidth(Width() + iconSize + be_control_look->DefaultLabelSpacing());

	delete fIcon;
	fIcon = new(std::nothrow) BBitmap(BRect(0, 0, iconSize - 1, iconSize - 1),
		B_RGBA32);
	if (fIcon != NULL && fCountry->GetIcon(fIcon) != B_OK) {
		delete fIcon;
		fIcon = NULL;
	}
}


void
TimeZoneListItem::SetCountry(BCountry* country)
{
	delete fCountry;
	fCountry = country;
}


void
TimeZoneListItem::SetTimeZone(BTimeZone* timeZone)
{
	delete fTimeZone;
	fTimeZone = timeZone;
}


const BString&
TimeZoneListItem::ID() const
{
	if (!HasTimeZone())
		return skDefaultString;

	return fTimeZone->ID();
}


const BString&
TimeZoneListItem::Name() const
{
	if (!HasTimeZone())
		return skDefaultString;

	return fTimeZone->Name();
}


int
TimeZoneListItem::OffsetFromGMT() const
{
	if (!HasTimeZone())
		return 0;

	return fTimeZone->OffsetFromGMT();
}


void
TimeZoneListItem::_DrawItemWithTextOffset(BView* owner, BRect frame,
	bool complete, float textOffset)
{
	rgb_color highColor = owner->HighColor();
	rgb_color lowColor = owner->LowColor();

	if (IsSelected() || complete) {
		rgb_color color;
		if (IsSelected())
			color = ui_color(B_LIST_SELECTED_BACKGROUND_COLOR);
		else
			color = owner->ViewColor();

		owner->SetHighColor(color);
		owner->SetLowColor(color);
		owner->FillRect(frame);
	} else
		owner->SetLowColor(owner->ViewColor());

	if (!IsEnabled()) {
		rgb_color textColor = ui_color(B_LIST_ITEM_TEXT_COLOR);
		if (textColor.red + textColor.green + textColor.blue > 128 * 3)
			owner->SetHighColor(tint_color(textColor, B_DARKEN_2_TINT));
		else
			owner->SetHighColor(tint_color(textColor, B_LIGHTEN_2_TINT));
	} else {
		if (IsSelected())
			owner->SetHighColor(ui_color(B_LIST_SELECTED_ITEM_TEXT_COLOR));
		else
			owner->SetHighColor(ui_color(B_LIST_ITEM_TEXT_COLOR));
	}

	owner->MovePenTo(
		frame.left + be_control_look->DefaultLabelSpacing() + textOffset,
		frame.top + BaselineOffset());
	owner->DrawString(Text());

	owner->SetHighColor(highColor);
	owner->SetLowColor(lowColor);
}
