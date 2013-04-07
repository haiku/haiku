/*
 * Copyright 2004-2013 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 *		Philippe Houdoin
 * 		Fredrik Modéen
 *		John Scipione, jscipione@gmail.com
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
#include <net_notifications.h>
#include <sys/socket.h>
#include <sys/sockio.h>

#include <Bitmap.h>
#include <Catalog.h>
#include <File.h>
#include <IconUtils.h>
#include <MenuItem.h>
#include <NetworkDevice.h>
#include <NetworkInterface.h>
#include <NetworkRoster.h>
#include <Point.h>
#include <PopUpMenu.h>
#include <Resources.h>
#include <String.h>
#include <SeparatorItem.h>
#include <Window.h>

#include <AutoDeleter.h>

#include "InterfacesAddOn.h"
#include "InterfaceWindow.h"


#define ICON_SIZE 37


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "InterfacesListView"


// #pragma mark - our_image function


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


// #pragma mark - InterfaceListItem


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


// #pragma mark - InterfaceListItem public methods


void
InterfaceListItem::DrawItem(BView* owner, BRect /*bounds*/, bool complete)
{
	BListView* list = dynamic_cast<BListView*>(owner);

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

	if (fSettings->IsDisabled()) {
		interfaceState << "disabled";
		stateIcon = fIconOffline;
	} else if (!fInterface.HasLink()) {
		interfaceState << "no link";
		stateIcon = fIconOffline;
	} else if ((fSettings->IPAddr(AF_INET).IsEmpty()
		&& fSettings->IPAddr(AF_INET6).IsEmpty())
		&& (fSettings->AutoConfigure(AF_INET)
		|| fSettings->AutoConfigure(AF_INET6))) {
		interfaceState << "connecting" B_UTF8_ELLIPSIS;
		stateIcon = fIconPending;
	} else {
		interfaceState << "connected";
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

	if (fSettings->IsDisabled()) {
		list->SetDrawingMode(B_OP_ALPHA);
		list->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);
		list->SetHighColor(0, 0, 0, 32);
	} else
		list->SetDrawingMode(B_OP_OVER);

	list->DrawBitmapAsync(fIcon, iconPt);
	list->DrawBitmapAsync(stateIcon, iconPt);

	if (fSettings->IsDisabled()) {
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
	list->DrawString(Name(), namePt);
	list->SetFont(be_plain_font);
	list->DrawString(interfaceState, statePt);
	
	if (!fSettings->IsDisabled()) {
		// Render IPv4 Address
		BString ipv4Str(B_TRANSLATE_COMMENT("IP:", "IPv4 address label"));
		if (fSettings->IPAddr(AF_INET).IsEmpty())
			ipv4Str << " " << B_TRANSLATE("None");
		else
			ipv4Str << " " << BString(fSettings->IP(AF_INET));

		list->DrawString(ipv4Str, line2Pt);
	}
	
	// Render IPv6 Address (if present)
	if (!fSettings->IsDisabled()
		&& !fSettings->IPAddr(AF_INET6).IsEmpty()) {
		BString ipv6Str(B_TRANSLATE_COMMENT("IPv6:", "IPv6 address label"));
		ipv6Str << " " << BString(fSettings->IP(AF_INET6));

		list->DrawString(ipv6Str, line3Pt);
	}

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
	fSettings = new NetworkSettings(Name());

	const char* mediaTypeName = NULL;

	BNetworkDevice device(Name());
	if (device.IsWireless())
		mediaTypeName = "wifi";
	else if (device.IsEthernet())
		mediaTypeName = "ether";

	printf("%s is a %s device\n", Name(), mediaTypeName);

	_PopulateBitmaps(mediaTypeName);
		// Load the interface icons
}


void
InterfaceListItem::_PopulateBitmaps(const char* mediaType) {

	const uint8* interfaceHVIF;
	const uint8* offlineHVIF;
	const uint8* pendingHVIF;
	const uint8* onlineHVIF;

	BBitmap* interfaceBitmap = NULL;

	/* Load interface icons */
	image_info info;
	if (our_image(info) != B_OK)
		return;

	BFile resourcesFile(info.name, B_READ_ONLY);
	if (resourcesFile.InitCheck() < B_OK)
		return;

	BResources addonResources(&resourcesFile);

	if (addonResources.InitCheck() < B_OK)
		return;

	size_t iconSize;

	// Try specific interface icon?
	interfaceHVIF = (const uint8*)addonResources.LoadResource(
		B_VECTOR_ICON_TYPE, Name(), &iconSize);

	if (interfaceHVIF == NULL && mediaType != NULL)
		// Not found, try interface media type?
		interfaceHVIF = (const uint8*)addonResources.LoadResource(
			B_VECTOR_ICON_TYPE, mediaType, &iconSize);
	if (interfaceHVIF == NULL)
		// Not found, try default interface icon?
		interfaceHVIF = (const uint8*)addonResources.LoadResource(
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
	offlineHVIF = (const uint8*)addonResources.LoadResource(
		B_VECTOR_ICON_TYPE, "offline", &iconSize);

	if (offlineHVIF) {
		fIconOffline = new BBitmap(BRect(0, 0, ICON_SIZE, ICON_SIZE),
			0, B_RGBA32);
		BIconUtils::GetVectorIcon(offlineHVIF, iconSize, fIconOffline);
	}

	pendingHVIF = (const uint8*)addonResources.LoadResource(
		B_VECTOR_ICON_TYPE, "pending", &iconSize);

	if (pendingHVIF) {
		fIconPending = new BBitmap(BRect(0, 0, ICON_SIZE, ICON_SIZE),
			0, B_RGBA32);
		BIconUtils::GetVectorIcon(pendingHVIF, iconSize, fIconPending);
	}

	onlineHVIF = (const uint8*)addonResources.LoadResource(
		B_VECTOR_ICON_TYPE, "online", &iconSize);

	if (onlineHVIF) {
		fIconOnline = new BBitmap(BRect(0, 0, ICON_SIZE, ICON_SIZE),
			0, B_RGBA32);
		BIconUtils::GetVectorIcon(onlineHVIF, iconSize, fIconOnline);
	}
}


// #pragma mark - InterfaceListView


InterfacesListView::InterfacesListView(const char* name)
	:
	BListView(name)
{
	fContextMenu = new BPopUpMenu("context menu", false, false);
}


InterfacesListView::~InterfacesListView()
{
}


// #pragma mark - InterfaceListView protected methods


void
InterfacesListView::AttachedToWindow()
{
	BListView::AttachedToWindow();

	_InitList();

	start_watching_network(
		B_WATCH_NETWORK_INTERFACE_CHANGES | B_WATCH_NETWORK_LINK_CHANGES, this);

	Select(0);
		// Select the first item in the list
}


void
InterfacesListView::FrameResized(float width, float height)
{
	BListView::FrameResized(width, height);
	Invalidate();
}


void
InterfacesListView::DetachedFromWindow()
{
	BListView::DetachedFromWindow();

	stop_watching_network(this);

	// free all items, they will be retrieved again in AttachedToWindow()
	for (int32 i = CountItems(); i-- > 0;)
		delete ItemAt(i);

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


void
InterfacesListView::MouseDown(BPoint where)
{
	int32 buttons = 0;
	Window()->CurrentMessage()->FindInt32("buttons", &buttons);

	if ((B_SECONDARY_MOUSE_BUTTON & buttons) == 0) {
		// If not secondary mouse button do the default
		BListView::MouseDown(where);
		return;
	}

	InterfaceListItem* item = FindItem(where);
	if (item == NULL)
		return;

	// Remove all items from the menu
	for (int32 i = fContextMenu->CountItems(); i >= 0; --i) {
		BMenuItem* menuItem = fContextMenu->RemoveItem(i);
		delete menuItem;
	}

	// Now add the ones we want
	if (item->GetSettings()->IsDisabled()) {
		fContextMenu->AddItem(new BMenuItem(B_TRANSLATE("Enable"),
			new BMessage(kMsgInterfaceToggle)));
	} else {
		fContextMenu->AddItem(new BMenuItem(
			B_TRANSLATE("Configure" B_UTF8_ELLIPSIS),
				new BMessage(kMsgInterfaceConfigure)));
		if (item->GetSettings()->AutoConfigure(AF_INET)
			|| item->GetSettings()->AutoConfigure(AF_INET6)) {
			fContextMenu->AddItem(new BMenuItem(
				B_TRANSLATE("Renegotiate Address"),
					new BMessage(kMsgInterfaceRenegotiate)));
		}
		fContextMenu->AddItem(new BSeparatorItem());
		fContextMenu->AddItem(new BMenuItem(B_TRANSLATE("Disable"),
			new BMessage(kMsgInterfaceToggle)));
	}

	fContextMenu->ResizeToPreferred();
	BMenuItem* selected = fContextMenu->Go(ConvertToScreen(where));
	if (selected == NULL)
		return;

	switch (selected->Message()->what) {
		case kMsgInterfaceConfigure:
		{
			InterfaceWindow* win = new InterfaceWindow(item->GetSettings());
			win->MoveTo(ConvertToScreen(where));
			win->Show();
			break;
		}

		case kMsgInterfaceToggle:
			item->SetDisabled(!item->IsDisabled());
			Invalidate();
			break;

		case kMsgInterfaceRenegotiate:
			item->GetSettings()->RenegotiateAddresses();
			break;
	}
}


// #pragma mark - InterfaceListView public methods


InterfaceListItem*
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


InterfaceListItem*
InterfacesListView::FindItem(BPoint where)
{
	for (int32 i = CountItems(); i-- > 0;) {
		InterfaceListItem* item = dynamic_cast<InterfaceListItem*>(ItemAt(i));
		if (item == NULL)
			continue;

		if (ItemFrame(i).Contains(where))
			return item;
	}

	return NULL;
}


status_t
InterfacesListView::SaveItems()
{
	// Grab each fSettings instance and write it's settings
	for (int32 i = CountItems(); i-- > 0;) {
		InterfaceListItem* item = dynamic_cast<InterfaceListItem*>(ItemAt(i));
		if (item == NULL)
			continue;

		if (strcmp(item->Name(), "loop") == 0)
			continue;

		NetworkSettings* ns = item->GetSettings();
		// TODO : SetConfiguration doesn't use the interface settings file
		ns->SetConfiguration();
	}

	return B_OK;
}


// #pragma mark - InterfaceListView private methods


status_t
InterfacesListView::_InitList()
{
	BNetworkRoster& roster = BNetworkRoster::Default();
	BNetworkInterface interface;
	uint32 cookie = 0;

	while (roster.GetNextInterface(&cookie, interface) == B_OK) {
		if (strncmp(interface.Name(), "loop", 4) && interface.Name()[0])
			AddItem(new InterfaceListItem(interface.Name()));
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

	if (strcmp(name, "loop") == 0)
		return;

	InterfaceListItem* item = FindItem(name);
	if (item == NULL)
		printf("InterfaceListItem %s not found!\n", name);

	switch (opcode) {
		case B_NETWORK_INTERFACE_CHANGED:
		case B_NETWORK_DEVICE_LINK_CHANGED:
			if (item != NULL)
				InvalidateItem(IndexOf(item));
			break;

		case B_NETWORK_INTERFACE_ADDED:
			if (item != NULL)
				InvalidateItem(IndexOf(item));
			else
				AddItem(new InterfaceListItem(name));
			break;

		case B_NETWORK_INTERFACE_REMOVED:
			if (item != NULL) {
				RemoveItem(item);
				delete item;
			}
			break;
	}
}
