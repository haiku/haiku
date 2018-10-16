/*
 * Copyright 2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, <axeld@pinc-software.de>
 */


#include "ServiceListItem.h"

#include <Catalog.h>
#include <ControlLook.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ServiceListItem"


static const char* kEnabledState = B_TRANSLATE_MARK("on");
static const char* kDisabledState = B_TRANSLATE_MARK("off");


ServiceListItem::ServiceListItem(const char* name, const char* label,
	const BNetworkSettings& settings)
	:
	fName(name),
	fLabel(label),
	fSettings(settings),
	fOwner(NULL),
	fLineOffset(0),
	fEnabled(false)
{
}


ServiceListItem::~ServiceListItem()
{
}


void
ServiceListItem::DrawItem(BView* owner, BRect bounds, bool complete)
{
	owner->PushState();

	rgb_color lowColor = owner->LowColor();

	if (IsSelected() || complete) {
		if (IsSelected()) {
			owner->SetHighColor(ui_color(B_LIST_SELECTED_BACKGROUND_COLOR));
			owner->SetLowColor(owner->HighColor());
		} else
			owner->SetHighColor(lowColor);

		owner->FillRect(bounds);
	}

	const char* stateText = fEnabled ? B_TRANSLATE(kEnabledState)
		: B_TRANSLATE(kDisabledState);

	// Set the initial bounds of item contents
	BPoint statePoint = bounds.RightTop() + BPoint(0, fLineOffset)
		- BPoint(be_plain_font->StringWidth(stateText)
			+ be_control_look->DefaultLabelSpacing(), 0);
	BPoint namePoint = bounds.LeftTop()
		+ BPoint(be_control_look->DefaultLabelSpacing(), fLineOffset);

	owner->SetDrawingMode(B_OP_OVER);

	rgb_color textColor;
	if (IsSelected())
		textColor = ui_color(B_LIST_SELECTED_ITEM_TEXT_COLOR);
	else
		textColor = ui_color(B_LIST_ITEM_TEXT_COLOR);

	owner->SetHighColor(textColor);
	owner->DrawString(fLabel, namePoint);

	if (!fEnabled) {
		if (textColor.red + textColor.green + textColor.blue > 128 * 3)
			owner->SetHighColor(tint_color(textColor, B_DARKEN_1_TINT));
		else
			owner->SetHighColor(tint_color(textColor, B_LIGHTEN_1_TINT));
	}
	owner->DrawString(stateText, statePoint);

	owner->PopState();
}


void
ServiceListItem::Update(BView* owner, const BFont* font)
{
	fOwner = owner;
	fEnabled = IsEnabled();

	BListItem::Update(owner, font);
	font_height height;
	font->GetHeight(&height);

	fLineOffset = 2 + ceilf(height.ascent + height.leading / 2);

	float maxStateWidth = std::max(font->StringWidth(B_TRANSLATE(kEnabledState)),
		font->StringWidth(B_TRANSLATE(kDisabledState)));
	SetWidth(font->StringWidth(fLabel)
		+ 3 * be_control_look->DefaultLabelSpacing() + maxStateWidth);
	SetHeight(4 + ceilf(height.ascent + height.leading + height.descent));
}


void
ServiceListItem::SettingsUpdated(uint32 type)
{
	if (type == BNetworkSettings::kMsgServiceSettingsUpdated) {
		bool wasEnabled = fEnabled;
		fEnabled = IsEnabled();
		if (wasEnabled != fEnabled)
			fOwner->Invalidate();
	}
}


bool
ServiceListItem::IsEnabled()
{
	return fSettings.Service(fName).IsRunning();
}
