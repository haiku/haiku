/*
 * Copyright 2004-2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 *		Philippe Houdoin
 * 		Fredrik Modéen
 *		John Scipione, jscipione@gmail.com
 */


#include "InterfaceListItem.h"

#include <algorithm>

#include <Application.h>
#include <Bitmap.h>
#include <Catalog.h>
#include <IconUtils.h>
#include <NetworkDevice.h>
#include <OutlineListView.h>
#include <Resources.h>
#include <String.h>


#define ICON_SIZE 37


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "InterfaceListItem"


InterfaceListItem::InterfaceListItem(const char* name)
	:
	BListItem(0, false),
	fIcon(NULL)
{
	fInterface.SetTo(name);
	_Init();
}


InterfaceListItem::~InterfaceListItem()
{
	delete fIcon;
}


// #pragma mark - InterfaceListItem public methods


void
InterfaceListItem::DrawItem(BView* owner, BRect /*bounds*/, bool complete)
{
	BOutlineListView* list = dynamic_cast<BOutlineListView*>(owner);
	if (list == NULL)
		return;

	owner->PushState();

	BRect bounds = list->ItemFrame(list->IndexOf(this));

	//rgb_color highColor = list->HighColor();
	rgb_color lowColor = list->LowColor();

	if (IsSelected() || complete) {
		if (IsSelected()) {
			list->SetHighColor(ui_color(B_LIST_SELECTED_BACKGROUND_COLOR));
			list->SetLowColor(list->HighColor());
		} else
			list->SetHighColor(lowColor);

		list->FillRect(bounds);
	}

	// TODO: only update periodically
	_UpdateState();

	BBitmap* stateIcon = _StateIcon();
	const char* stateText = _StateText();

	// Set the initial bounds of item contents
	BPoint iconPoint = bounds.LeftTop();
	BPoint namePoint = bounds.LeftTop();
	BPoint line2Point = bounds.LeftTop();
	BPoint line3Point = bounds.LeftTop();
	BPoint statePoint = bounds.RightTop();

	iconPoint += BPoint(4, 4);
	statePoint += BPoint(0, fFirstlineOffset);
	namePoint += BPoint(ICON_SIZE + 12, fFirstlineOffset);
	line2Point += BPoint(ICON_SIZE + 12, fSecondlineOffset);
	line3Point += BPoint(ICON_SIZE + 12, fThirdlineOffset);

	statePoint -= BPoint(be_plain_font->StringWidth(stateText) + 4.0f, 0);

	if (fDisabled) {
		list->SetDrawingMode(B_OP_ALPHA);
		list->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);
		list->SetHighColor(0, 0, 0, 32);
	} else
		list->SetDrawingMode(B_OP_OVER);

	list->DrawBitmapAsync(fIcon, iconPoint);
	list->DrawBitmapAsync(stateIcon, iconPoint);

	if (fDisabled) {
		rgb_color textColor;
		if (IsSelected())
			textColor = ui_color(B_LIST_SELECTED_ITEM_TEXT_COLOR);
		else
			textColor = ui_color(B_LIST_ITEM_TEXT_COLOR);

		if (textColor.red + textColor.green + textColor.blue > 128 * 3)
			list->SetHighColor(tint_color(textColor, B_DARKEN_1_TINT));
		else
			list->SetHighColor(tint_color(textColor, B_LIGHTEN_1_TINT));
	} else {
		if (IsSelected())
			list->SetHighColor(ui_color(B_LIST_SELECTED_ITEM_TEXT_COLOR));
		else
			list->SetHighColor(ui_color(B_LIST_ITEM_TEXT_COLOR));
	}

	list->SetFont(be_bold_font);

	list->DrawString(fDeviceName, namePoint);
	list->SetFont(be_plain_font);
	list->DrawString(stateText, statePoint);

// TODO!
/*	if (!disabled) {
		// Render IPv4 Address
		BString ipv4Str(B_TRANSLATE_COMMENT("IP:", "IPv4 address label"));
		if (fSettings->IPAddr(AF_INET).IsEmpty())
			ipv4Str << " " << B_TRANSLATE("None");
		else
			ipv4Str << " " << BString(fSettings->IP(AF_INET));

		list->DrawString(ipv4Str, line2Point);
	}

	// Render IPv6 Address (if present)
	if (!disabled && !fSettings->IPAddr(AF_INET6).IsEmpty()) {
		BString ipv6Str(B_TRANSLATE_COMMENT("IPv6:", "IPv6 address label"));
		ipv6Str << " " << BString(fSettings->IP(AF_INET6));

		list->DrawString(ipv6Str, line3Point);
	}
*/
	owner->PopState();
}


void
InterfaceListItem::Update(BView* owner, const BFont* font)
{
	BListItem::Update(owner, font);
	font_height height;
	font->GetHeight(&height);

	float lineHeight = ceilf(height.ascent) + ceilf(height.descent)
		+ ceilf(height.leading);

	fFirstlineOffset = 2 + ceilf(height.ascent + height.leading / 2);
	fSecondlineOffset = fFirstlineOffset + lineHeight;
	fThirdlineOffset = fFirstlineOffset + (lineHeight * 2);

	_UpdateState();

	SetWidth(fIcon->Bounds().Width() + 24
		+ be_bold_font->StringWidth(fDeviceName.String())
		+ owner->StringWidth(_StateText()));
	SetHeight(std::max(3 * lineHeight + 4, fIcon->Bounds().Height() + 8));
		// either to the text height or icon height, whichever is taller
}


// #pragma mark - InterfaceListItem private methods


void
InterfaceListItem::_Init()
{
	const char* mediaTypeName = NULL;

	BNetworkDevice device(Name());
	if (device.IsWireless())
		mediaTypeName = "wifi";
	else if (device.IsEthernet())
		mediaTypeName = "ether";

	_PopulateBitmaps(mediaTypeName);
		// Load the interface icons
}


void
InterfaceListItem::_PopulateBitmaps(const char* mediaType)
{
	const uint8* interfaceHVIF;
	const uint8* offlineHVIF;
	const uint8* pendingHVIF;
	const uint8* onlineHVIF;

	BBitmap* interfaceBitmap = NULL;

	BResources* resources = BApplication::AppResources();

	size_t iconSize;

	// Try specific interface icon?
	interfaceHVIF = (const uint8*)resources->LoadResource(
		B_VECTOR_ICON_TYPE, Name(), &iconSize);

	if (interfaceHVIF == NULL && mediaType != NULL)
		// Not found, try interface media type?
		interfaceHVIF = (const uint8*)resources->LoadResource(
			B_VECTOR_ICON_TYPE, mediaType, &iconSize);
	if (interfaceHVIF == NULL)
		// Not found, try default interface icon?
		interfaceHVIF = (const uint8*)resources->LoadResource(
			B_VECTOR_ICON_TYPE, "ether", &iconSize);

	if (interfaceHVIF) {
		// Now build the bitmap
		interfaceBitmap = new BBitmap(BRect(0, 0, ICON_SIZE, ICON_SIZE),
			0, B_RGBA32);
		if (BIconUtils::GetVectorIcon(interfaceHVIF,
			iconSize, interfaceBitmap) == B_OK)
			fIcon = interfaceBitmap;
		else
			delete interfaceBitmap;
	}

	// Load possible state icons
	offlineHVIF = (const uint8*)resources->LoadResource(
		B_VECTOR_ICON_TYPE, "offline", &iconSize);

	if (offlineHVIF) {
		fIconOffline = new BBitmap(BRect(0, 0, ICON_SIZE, ICON_SIZE),
			0, B_RGBA32);
		BIconUtils::GetVectorIcon(offlineHVIF, iconSize, fIconOffline);
	}

	pendingHVIF = (const uint8*)resources->LoadResource(
		B_VECTOR_ICON_TYPE, "pending", &iconSize);

	if (pendingHVIF) {
		fIconPending = new BBitmap(BRect(0, 0, ICON_SIZE, ICON_SIZE),
			0, B_RGBA32);
		BIconUtils::GetVectorIcon(pendingHVIF, iconSize, fIconPending);
	}

	onlineHVIF = (const uint8*)resources->LoadResource(
		B_VECTOR_ICON_TYPE, "online", &iconSize);

	if (onlineHVIF) {
		fIconOnline = new BBitmap(BRect(0, 0, ICON_SIZE, ICON_SIZE),
			0, B_RGBA32);
		BIconUtils::GetVectorIcon(onlineHVIF, iconSize, fIconOnline);
	}
}


void
InterfaceListItem::_UpdateState()
{
	fDeviceName = Name();
	fDeviceName.RemoveFirst("/dev/net/");

	fDisabled = (fInterface.Flags() & IFF_UP) == 0;
	fHasLink = fInterface.HasLink();
	fConnecting = (fInterface.Flags() & IFF_CONFIGURING) != 0;

	int32 count = fInterface.CountAddresses();
	size_t addressIndex = 0;
	for (int32 index = 0; index < count; index++) {
		if (addressIndex >= sizeof(fAddress) / sizeof(fAddress[0]))
			break;

		BNetworkInterfaceAddress address;
		if (fInterface.GetAddressAt(index, address) == B_OK)
			fAddress[addressIndex++] = address.Address().ToString();
	}
}


BBitmap*
InterfaceListItem::_StateIcon() const
{
	if (fDisabled)
		return fIconOffline;
	if (!fHasLink)
		return fIconOffline;
	// TODO!
//	} else if ((fSettings->IPAddr(AF_INET).IsEmpty()
//		&& fSettings->IPAddr(AF_INET6).IsEmpty())
//		&& (fSettings->AutoConfigure(AF_INET)
//		|| fSettings->AutoConfigure(AF_INET6))) {
//		interfaceState = "connecting" B_UTF8_ELLIPSIS;
//		stateIcon = fIconPending;

	return fIconOnline;
}


const char*
InterfaceListItem::_StateText() const
{
	if (fDisabled)
		return B_TRANSLATE("disabled");
	if (!fInterface.HasLink())
		return B_TRANSLATE("no link");

	// TODO!
//	} else if ((fSettings->IPAddr(AF_INET).IsEmpty()
//		&& fSettings->IPAddr(AF_INET6).IsEmpty())
//		&& (fSettings->AutoConfigure(AF_INET)
//		|| fSettings->AutoConfigure(AF_INET6))) {
//		interfaceState = "connecting" B_UTF8_ELLIPSIS;
//		stateIcon = fIconPending;

	return B_TRANSLATE("connected");
}
