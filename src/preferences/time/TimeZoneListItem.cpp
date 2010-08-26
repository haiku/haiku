/*
 * Copyright 2010, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrien Destugues <pulkomandy@pulkomandy.ath.cx>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Axel Dörfler, axeld@pinc-software.de
 *		Oliver Tappe <zooey@hirschkaefer.de>
*/


#include "TimeZoneListItem.h"

#include <new>

#include <Bitmap.h>
#include <Country.h>
#include <String.h>
#include <TimeZone.h>
#include <Window.h>


static const BString skDefaultString;


TimeZoneListItem::TimeZoneListItem(const char* text, BCountry* country,
	BTimeZone* timeZone)
	:
	BStringItem(text, 0, false),
	fIcon(NULL),
	fTimeZone(timeZone)
{
	if (country != NULL) {
		fIcon = new(std::nothrow) BBitmap(BRect(0, 0, 15, 15), B_RGBA32);
		if (fIcon != NULL && country->GetIcon(fIcon) != B_OK) {
			delete fIcon;
			fIcon = NULL;
		}
	}
}


TimeZoneListItem::~TimeZoneListItem()
{
	delete fIcon;
	delete fTimeZone;
}


void
TimeZoneListItem::DrawItem(BView* owner, BRect frame, bool complete)
{
	rgb_color kHighlight = {140, 140, 140, 0};
	rgb_color kBlack = {0, 0, 0, 0};

	if (IsSelected() || complete) {
		rgb_color color;
		if (IsSelected())
			color = kHighlight;
		else
			color = owner->ViewColor();
		owner->SetHighColor(color);
		owner->SetLowColor(color);
		owner->FillRect(frame);
		owner->SetHighColor(kBlack);
	} else
		owner->SetLowColor(owner->ViewColor());

	// Draw the text
	BString text = Text();

	BFont font = be_plain_font;
	font_height	finfo;
	font.GetHeight(&finfo);
	owner->SetFont(&font);
	// TODO: the position is unnecessarily complicated, and not correct either
	owner->MovePenTo(frame.left + 8, frame.top
		+ (frame.Height() - (finfo.ascent + finfo.descent + finfo.leading)) / 2
		+ (finfo.ascent + finfo.descent) - 1);
	owner->DrawString(text.String());

	// Draw the icon
	frame.left = frame.right - 16;
	BRect iconFrame(frame);
	iconFrame.Set(iconFrame.left, iconFrame.top + 1, iconFrame.left + 15,
		iconFrame.top + 16);

	if (fIcon != NULL && fIcon->IsValid()) {
		owner->SetDrawingMode(B_OP_OVER);
		owner->DrawBitmap(fIcon, iconFrame);
		owner->SetDrawingMode(B_OP_COPY);
	}
}


bool
TimeZoneListItem::HasTimeZone() const
{
	return fTimeZone != NULL;
}


const BTimeZone&
TimeZoneListItem::TimeZone() const
{
	return *fTimeZone;
}


const BString&
TimeZoneListItem::ID() const
{
	if (fTimeZone == NULL)
		return skDefaultString;

	return fTimeZone->ID();
}


const BString&
TimeZoneListItem::Name() const
{
	if (fTimeZone == NULL)
		return skDefaultString;

	return fTimeZone->Name();
}


int
TimeZoneListItem::OffsetFromGMT() const
{
	if (fTimeZone == NULL)
		return 0;

	return fTimeZone->OffsetFromGMT();
}
