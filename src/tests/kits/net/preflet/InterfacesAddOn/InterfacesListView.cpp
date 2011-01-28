/*
 * Copyright 2004-2011 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin
 * 		Fredrik Modéen
 *		Alexander von Gluck IV, <kallisti5@unixzen.com>
 */


#include "InterfacesListView.h"

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/sockio.h>

#include <File.h>
#include <IconUtils.h>
#include <net_notifications.h>
#include <NetworkDevice.h>
#include <NetworkInterface.h>
#include <NetworkRoster.h>
#include <Resources.h>

#include <AutoDeleter.h>

#include "Settings.h"


// #pragma mark -


status_t
our_image(image_info& image)
{
	int32 cookie = 0;
	while (get_next_image_info(B_CURRENT_TEAM, &cookie, &image) == B_OK) {
		if ((char *)our_image >= (char *)image.text
			&& (char *)our_image <= (char *)image.text + image.text_size)
			return B_OK;
	}

	return B_ERROR;
}


// #pragma mark -


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
	delete fSettings;
}


void
InterfaceListItem::Update(BView* owner, const BFont* font)
{
	BListItem::Update(owner,font);
	font_height height;
	font->GetHeight(&height);

	float lineHeight = ceilf(height.ascent) + ceilf(height.descent)
		+ ceilf(height.leading);

	fFirstlineOffset = 2 + ceilf(height.ascent + height.leading / 2);
	fSecondlineOffset = fFirstlineOffset + lineHeight;
	fThirdlineOffset = fFirstlineOffset + (lineHeight * 2);

	SetHeight(3 * lineHeight + 4);
}


void
InterfaceListItem::DrawItem(BView* owner, BRect /*bounds*/, bool complete)
{
	BListView* list = dynamic_cast<BListView*>(owner);
	if (!list)
		return;

	owner->PushState();

	BRect bounds = list->ItemFrame(list->IndexOf(this));

	rgb_color black= { 0,0,0,255 };

	if ( IsSelected() || complete ) {
        if (IsSelected()) {
            owner->SetHighColor(tint_color(owner->ViewColor() , B_HIGHLIGHT_BACKGROUND_TINT));
        } else {
            owner->SetHighColor(owner->LowColor());
		}

		owner->FillRect(bounds);
	}

	BPoint iconPt = bounds.LeftTop() + BPoint(4,4);
	BPoint namePt = BPoint(32+12, fFirstlineOffset);
	BPoint v4addrPt = BPoint(32+12, fSecondlineOffset);
	BPoint v6addrPt = BPoint(32+12, fThirdlineOffset);

	if ( fSettings->IsDisabled() ) {
		owner->SetDrawingMode(B_OP_ALPHA);
		owner->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);
		owner->SetHighColor(0, 0, 0, 32);
	} else
		owner->SetDrawingMode(B_OP_OVER);

	owner->DrawBitmapAsync(fIcon, iconPt);

	if ( fSettings->IsDisabled() )
        owner->SetHighColor(tint_color(black, B_LIGHTEN_1_TINT));
	else
        owner->SetHighColor(black);
		
	owner->SetFont(be_bold_font);
	owner->DrawString(Name(), namePt);
	owner->SetFont(be_plain_font);

	if (fSettings->IsDisabled())
		owner->DrawString("Disabled", v4addrPt);
	else {
		BString v4str("IPv4: ");
		v4str << fSettings->IP();
		v4str << " (";
		if ( fSettings->AutoConfigure() )
			v4str << "DHCP";
		else
			v4str << "manual";
		v4str << ")";
		owner->DrawString(v4str.String(), v4addrPt);

		owner->DrawString("IPv6: none (auto)", v6addrPt);
			// TODO : where will we keep this?
	}

	owner->PopState();
}


void
InterfaceListItem::_Init()
{
	fSettings = new Settings(Name());

	// Init icon
	BBitmap* icon = NULL;

	const char* mediaTypeName = NULL;

	int media = fInterface.Media();
	printf("%s media = 0x%x\n", Name(), media);
	switch (IFM_TYPE(media)) {
		case IFM_ETHER:
			mediaTypeName = "ether";
			break;
		case IFM_IEEE80211:
			mediaTypeName = "wifi";
			break;
		default: {
			BNetworkDevice device(Name());
			if (device.IsWireless())
				mediaTypeName = "wifi";
			else if (device.IsEthernet())
				mediaTypeName = "ether";
			break;
		}
	}

	image_info info;
	if (our_image(info) != B_OK)
		return;

	BFile file(info.name, B_READ_ONLY);
	if (file.InitCheck() < B_OK)
		return;

	BResources resources(&file);
	if (resources.InitCheck() < B_OK)
		return;

	size_t size;
	// Try specific interface icon?
	const uint8* rawIcon = (const uint8*)resources.LoadResource(B_VECTOR_ICON_TYPE, Name(), &size);
	if (rawIcon == NULL && mediaTypeName != NULL)
		// Not found, try interface media type?
		rawIcon = (const uint8*)resources.LoadResource(B_VECTOR_ICON_TYPE, mediaTypeName, &size);
	if (rawIcon == NULL)
		// Not found, try default interface icon?
		rawIcon = (const uint8*)resources.LoadResource(B_VECTOR_ICON_TYPE, "ether", &size);

	if (rawIcon) {
		// Now build the bitmap
		icon = new BBitmap(BRect(0, 0, 31, 31), 0, B_RGBA32);
		if (BIconUtils::GetVectorIcon(rawIcon, size, icon) == B_OK)
			fIcon = icon;
		else
			delete icon;
	}
}

// #pragma mark -


InterfacesListView::InterfacesListView(BRect rect, const char* name, uint32 resizingMode)
	: BListView(rect, name, B_SINGLE_SELECTION_LIST, resizingMode)
{
}


InterfacesListView::~InterfacesListView()
{
}


void
InterfacesListView::AttachedToWindow()
{
	BListView::AttachedToWindow();

	_InitList();

	start_watching_network(
		B_WATCH_NETWORK_INTERFACE_CHANGES | B_WATCH_NETWORK_LINK_CHANGES, this);
}


void
InterfacesListView::DetachedFromWindow()
{
	BListView::DetachedFromWindow();

	stop_watching_network(this);

	// free all items, they will be retrieved again in AttachedToWindow()
	for (int32 i = CountItems(); i-- > 0;) {
		delete ItemAt(i);
	}
	MakeEmpty();
}


void
InterfacesListView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_NETWORK_MONITOR:
			_HandleNetworkMessage(message);
			break;

		default:
			BListView::MessageReceived(message);
	}
}


InterfaceListItem *
InterfacesListView::FindItem(const char* name)
{
	for (int32 i = CountItems(); i-- > 0;) {
		InterfaceListItem* item = dynamic_cast<InterfaceListItem*>(ItemAt(i));
		if (item == NULL)
			continue;

		if (strcmp(item->Name(), name) == 0)
			return item;
	}

	return NULL;
}


status_t
InterfacesListView::_InitList()
{
	BNetworkRoster& roster = BNetworkRoster::Default();
	BNetworkInterface interface;
	uint32 cookie = 0;

	while (roster.GetNextInterface(&cookie, interface) == B_OK) {
		if (strncmp(interface.Name(), "loop", 4) && interface.Name()[0]) {
			AddItem(new InterfaceListItem(interface.Name()));
		}
	}

	return B_OK;
}


status_t
InterfacesListView::_UpdateList()
{
	// TODO
	return B_OK;
}

void
InterfacesListView::_HandleNetworkMessage(BMessage* message)
{
	const char* name;
	int32 opcode;

	message->PrintToStream();

	if (message->FindInt32("opcode", &opcode) != B_OK)
		return;

	if (message->FindString("interface", &name) != B_OK
		&& message->FindString("device", &name) != B_OK)
		return;

	InterfaceListItem* item = FindItem(name);
	if (!item)
		printf("InterfaceListItem %s not found!\n", name);

	switch (opcode) {
		case B_NETWORK_INTERFACE_CHANGED:
		case B_NETWORK_DEVICE_LINK_CHANGED:
			if (item)
				InvalidateItem(IndexOf(item));
			break;

		case B_NETWORK_INTERFACE_ADDED:
			if (item)
				InvalidateItem(IndexOf(item));
			else
				AddItem(new InterfaceListItem(name));
			break;

		case B_NETWORK_INTERFACE_REMOVED:
			if (item) {
				RemoveItem(item);
				delete item;
			}
			break;
	}
}
