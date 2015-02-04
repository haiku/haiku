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

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_types.h>
#include <netinet/in.h>
#include <net_notifications.h>
#include <sys/socket.h>
#include <sys/sockio.h>

#include <Application.h>
#include <Bitmap.h>
#include <Catalog.h>
#include <File.h>
#include <IconUtils.h>
#include <MenuItem.h>
#include <NetworkDevice.h>
#include <NetworkInterface.h>
#include <NetworkRoster.h>
#include <OutlineListView.h>
#include <Point.h>
#include <PopUpMenu.h>
#include <Resources.h>
#include <String.h>
#include <SeparatorItem.h>
#include <Window.h>

#include <AutoDeleter.h>


#define ICON_SIZE 37


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "InterfacesListView"


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

	BString interfaceState;
	BBitmap* stateIcon(NULL);

	// TODO: only update periodically
	bool disabled = (fInterface.Flags() & IFF_UP) == 0;

	if (disabled) {
		interfaceState = "disabled";
		stateIcon = fIconOffline;
	} else if (!fInterface.HasLink()) {
		interfaceState = "no link";
		stateIcon = fIconOffline;
	// TODO!
//	} else if ((fSettings->IPAddr(AF_INET).IsEmpty()
//		&& fSettings->IPAddr(AF_INET6).IsEmpty())
//		&& (fSettings->AutoConfigure(AF_INET)
//		|| fSettings->AutoConfigure(AF_INET6))) {
//		interfaceState = "connecting" B_UTF8_ELLIPSIS;
//		stateIcon = fIconPending;
	} else {
		interfaceState = "connected";
		stateIcon = fIconOnline;
	}

	// Set the initial bounds of item contents
	BPoint iconPt = bounds.LeftTop();
	BPoint namePt = bounds.LeftTop();
	BPoint line2Pt = bounds.LeftTop();
	BPoint line3Pt = bounds.LeftTop();
	BPoint statePt = bounds.RightTop();

	iconPt += BPoint(4, 4);
	statePt += BPoint(0, fFirstlineOffset);
	namePt += BPoint(ICON_SIZE + 12, fFirstlineOffset);
	line2Pt += BPoint(ICON_SIZE + 12, fSecondlineOffset);
	line3Pt += BPoint(ICON_SIZE + 12, fThirdlineOffset);

	statePt -= BPoint(
		be_plain_font->StringWidth(interfaceState.String()) + 4.0f, 0);

	if (disabled) {
		list->SetDrawingMode(B_OP_ALPHA);
		list->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);
		list->SetHighColor(0, 0, 0, 32);
	} else
		list->SetDrawingMode(B_OP_OVER);

	list->DrawBitmapAsync(fIcon, iconPt);
	list->DrawBitmapAsync(stateIcon, iconPt);

	if (disabled) {
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

	BString name = Name();
	name.RemoveFirst("/dev/net/");

	list->DrawString(name, namePt);
	list->SetFont(be_plain_font);
	list->DrawString(interfaceState, statePt);

// TODO!
/*	if (!disabled) {
		// Render IPv4 Address
		BString ipv4Str(B_TRANSLATE_COMMENT("IP:", "IPv4 address label"));
		if (fSettings->IPAddr(AF_INET).IsEmpty())
			ipv4Str << " " << B_TRANSLATE("None");
		else
			ipv4Str << " " << BString(fSettings->IP(AF_INET));

		list->DrawString(ipv4Str, line2Pt);
	}

	// Render IPv6 Address (if present)
	if (!disabled && !fSettings->IPAddr(AF_INET6).IsEmpty()) {
		BString ipv6Str(B_TRANSLATE_COMMENT("IPv6:", "IPv6 address label"));
		ipv6Str << " " << BString(fSettings->IP(AF_INET6));

		list->DrawString(ipv6Str, line3Pt);
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
