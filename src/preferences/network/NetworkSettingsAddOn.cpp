/*
 * Copyright 2004-2015 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <NetworkSettingsAddOn.h>

#include <stdio.h>
#include <stdlib.h>

#include <ControlLook.h>
#include <NetworkAddress.h>
#include <NetworkInterface.h>
#include <NetworkSettings.h>

#include "NetworkWindow.h"


using namespace BNetworkKit;


BNetworkSettingsItem::BNetworkSettingsItem()
	:
	fProfile(NULL)
{
}


BNetworkSettingsItem::~BNetworkSettingsItem()
{
}


status_t
BNetworkSettingsItem::ProfileChanged(const BNetworkProfile* newProfile)
{
	fProfile = newProfile;
	return B_OK;
}


const BNetworkProfile*
BNetworkSettingsItem::Profile() const
{
	return fProfile;
}


void
BNetworkSettingsItem::SettingsUpdated(uint32 type)
{
}


void
BNetworkSettingsItem::ConfigurationUpdated(const BMessage& message)
{
}


void
BNetworkSettingsItem::NotifySettingsUpdated()
{
	// TODO: post to network window
	BMessage updated(kMsgSettingsItemUpdated);
	updated.AddPointer("item", this);
	gNetworkWindow.SendMessage(&updated);
}


// #pragma mark -


BNetworkSettingsInterfaceItem::BNetworkSettingsInterfaceItem(
	const char* interface)
	:
	fInterface(interface)
{
}


BNetworkSettingsInterfaceItem::~BNetworkSettingsInterfaceItem()
{
}


BNetworkSettingsType
BNetworkSettingsInterfaceItem::Type() const
{
	return B_NETWORK_SETTINGS_TYPE_INTERFACE;
}


const char*
BNetworkSettingsInterfaceItem::Interface() const
{
	return fInterface;
}


// #pragma mark -


BNetworkInterfaceListItem::BNetworkInterfaceListItem(int family,
	const char* interface, const char* label, BNetworkSettings& settings)
	:
	fSettings(settings),
	fFamily(family),
	fInterface(interface),
	fLabel(label),
	fDisabled(false),
	fLineOffset(0),
	fSpacing(0)
{
}


BNetworkInterfaceListItem::~BNetworkInterfaceListItem()
{
}


const char*
BNetworkInterfaceListItem::Label() const
{
	return fLabel;
}


void
BNetworkInterfaceListItem::DrawItem(BView* owner, BRect bounds, bool complete)
{
	owner->PushState();

	if (IsSelected() || complete) {
		if (IsSelected()) {
			owner->SetHighColor(ui_color(B_LIST_SELECTED_BACKGROUND_COLOR));
			owner->SetLowColor(owner->HighColor());
		} else
			owner->SetHighColor(owner->LowColor());

		owner->FillRect(bounds);
	}

	// Set the initial bounds of item contents
	BPoint labelLocation = bounds.LeftTop() + BPoint(fSpacing, fLineOffset);

	if (fDisabled) {
		rgb_color textColor;
		if (IsSelected())
			textColor = ui_color(B_LIST_SELECTED_ITEM_TEXT_COLOR);
		else
			textColor = ui_color(B_LIST_ITEM_TEXT_COLOR);

		if (textColor.red + textColor.green + textColor.blue > 128 * 3)
			owner->SetHighColor(tint_color(textColor, B_DARKEN_1_TINT));
		else
			owner->SetHighColor(tint_color(textColor, B_LIGHTEN_1_TINT));
	} else {
		if (IsSelected())
			owner->SetHighColor(ui_color(B_LIST_SELECTED_ITEM_TEXT_COLOR));
		else
			owner->SetHighColor(ui_color(B_LIST_ITEM_TEXT_COLOR));
	}

	owner->SetFont(be_plain_font);
	owner->DrawString(fLabel, labelLocation);

	if (!fAddress.IsEmpty()) {
		BFont font = _AddressFont();
		owner->MovePenBy(fSpacing, 0);
		owner->SetFont(&font);
		owner->DrawString(fAddress);
	}

	owner->PopState();
}


void
BNetworkInterfaceListItem::Update(BView* owner, const BFont* font)
{
	_UpdateState();

	fSpacing = be_control_look->DefaultLabelSpacing();

	BListItem::Update(owner, font);
	font_height height;
	font->GetHeight(&height);

	float lineHeight = ceilf(height.ascent) + ceilf(height.descent)
		+ ceilf(height.leading);
	fLineOffset = 2 + ceilf(height.ascent + height.leading / 2);

	SetWidth(owner->StringWidth(fLabel) + 2 * fSpacing
		+ _AddressFont().StringWidth(fAddress.String()));
	SetHeight(lineHeight + 4);
}


void
BNetworkInterfaceListItem::ConfigurationUpdated(const BMessage& message)
{
	_UpdateState();
}


BFont
BNetworkInterfaceListItem::_AddressFont()
{
	BFont font;
	font.SetFace(B_ITALIC_FACE);
	font.SetSize(font.Size() * 0.9f);
	return font;
}


void
BNetworkInterfaceListItem::_UpdateState()
{
	BNetworkInterfaceAddress address;
	BNetworkInterface interface(fInterface);

	bool autoConfigure = fSettings.Interface(fInterface).IsAutoConfigure(
		fFamily);

	fAddress = "";
	fDisabled = !autoConfigure;

	int32 index = interface.FindFirstAddress(fFamily);
	if (index < 0)
		return;

	interface.GetAddressAt(index, address);

	fDisabled = address.Address().IsEmpty() && !autoConfigure;
	if (!address.Address().IsEmpty())
		fAddress << "(" << address.Address().ToString() << ")";
}


// #pragma mark -


BNetworkSettingsAddOn::BNetworkSettingsAddOn(image_id image,
	BNetworkSettings& settings)
	:
	fImage(image),
	fResources(NULL),
	fSettings(settings)
{
}


BNetworkSettingsAddOn::~BNetworkSettingsAddOn()
{
	delete fResources;
}


BNetworkSettingsInterfaceItem*
BNetworkSettingsAddOn::CreateNextInterfaceItem(uint32& cookie,
	const char* interface)
{
	return NULL;
}


BNetworkSettingsItem*
BNetworkSettingsAddOn::CreateNextItem(uint32& cookie)
{
	return NULL;
}


image_id
BNetworkSettingsAddOn::Image()
{
	return fImage;
}


BResources*
BNetworkSettingsAddOn::Resources()
{
	if (fResources == NULL) {
		image_info info;
		if (get_image_info(fImage, &info) != B_OK)
			return NULL;

		BResources* resources = new BResources();
		BFile file(info.name, B_READ_ONLY);
		if (resources->SetTo(&file) == B_OK)
			fResources = resources;
		else
			delete resources;
	}
	return fResources;
}


BNetworkSettings&
BNetworkSettingsAddOn::Settings()
{
	return fSettings;
}
