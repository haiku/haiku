/*
 * Copyright 2004-2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck, kallisti5@unixzen.com
 *		John Scipione, jscipione@gmail.com
 */


#include "InterfaceAddressView.h"

#include <stdio.h>

#include <Catalog.h>
#include <ControlLook.h>
#include <LayoutBuilder.h>
#include <MenuItem.h>
#include <MenuField.h>
#include <PopUpMenu.h>
#include <Screen.h>
#include <Size.h>
#include <StringView.h>
#include <TextControl.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "IntefaceAddressView"


const uint32 kModeAuto = 'iato';
const uint32 kModeStatic = 'istc';
const uint32 kModeDisabled = 'ioff';


// #pragma mark - InterfaceAddressView


InterfaceAddressView::InterfaceAddressView(int family,
	const char* interface, BNetworkSettings& settings)
	:
	BGroupView(B_VERTICAL),
	fFamily(family),
	fInterface(interface),
	fSettings(settings)
{
	SetLayout(new BGroupLayout(B_VERTICAL));

	// Create our controls
	fModePopUpMenu = new BPopUpMenu("modes");

	if (fFamily == AF_INET) {
		fModePopUpMenu->AddItem(new BMenuItem(B_TRANSLATE("DHCP"),
			new BMessage(kModeAuto)));
	}

	if (fFamily == AF_INET6) {
		// Automatic can be DHCPv6 or Router Advertisements
		fModePopUpMenu->AddItem(new BMenuItem(B_TRANSLATE("Automatic"),
			new BMessage(kModeAuto)));
	}

	fModePopUpMenu->AddItem(new BMenuItem(B_TRANSLATE("Static"),
		new BMessage(kModeStatic)));
	fModePopUpMenu->AddSeparatorItem();
	fModePopUpMenu->AddItem(new BMenuItem(B_TRANSLATE("Disabled"),
		new BMessage(kModeDisabled)));

	fModeField = new BMenuField(B_TRANSLATE("Mode:"), fModePopUpMenu);
	fModeField->SetToolTip(
		B_TRANSLATE("The method for obtaining an IP address"));

	float minimumWidth = be_control_look->DefaultItemSpacing() * 16;

	fAddressField = new BTextControl(B_TRANSLATE("IP Address:"), NULL, NULL);
	fAddressField->SetToolTip(B_TRANSLATE("Your IP address"));
	fAddressField->TextView()->SetExplicitMinSize(BSize(minimumWidth, B_SIZE_UNSET));
	fNetmaskField = new BTextControl(B_TRANSLATE("Netmask:"), NULL, NULL);
	fNetmaskField->SetToolTip(B_TRANSLATE("The netmask defines your local network"));
	fNetmaskField->TextView()->SetExplicitMinSize(BSize(minimumWidth, B_SIZE_UNSET));
	fGatewayField = new BTextControl(B_TRANSLATE("Gateway:"), NULL, NULL);
	fGatewayField->SetToolTip(B_TRANSLATE("Your gateway to the internet"));
	fGatewayField->TextView()->SetExplicitMinSize(BSize(minimumWidth, B_SIZE_UNSET));

	fSettings.GetInterface(interface, fOriginalInterface);
	fInterfaceSettings = fOriginalInterface;
	_UpdateFields();

	BLayoutBuilder::Group<>(this)
		.AddGrid()
			.AddMenuField(fModeField, 0, 0, B_ALIGN_RIGHT)
			.AddTextControl(fAddressField, 0, 1, B_ALIGN_RIGHT)
			.AddTextControl(fNetmaskField, 0, 2, B_ALIGN_RIGHT)
			.AddTextControl(fGatewayField, 0, 3, B_ALIGN_RIGHT)
		.End()
		.AddGlue();
}


InterfaceAddressView::~InterfaceAddressView()
{
}


// #pragma mark - InterfaceAddressView virtual methods


void
InterfaceAddressView::AttachedToWindow()
{
	fModePopUpMenu->SetTargetForItems(this);
}


void
InterfaceAddressView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kModeAuto:
		case kModeStatic:
		case kModeDisabled:
			_SetModeField(message->what);
			_UpdateSettings();
			break;

		default:
			BView::MessageReceived(message);
	}
}


// #pragma mark - InterfaceAddressView private methods


void
InterfaceAddressView::_EnableFields(bool enable)
{
	fAddressField->SetEnabled(enable);
	fNetmaskField->SetEnabled(enable);
	fGatewayField->SetEnabled(enable);
}


// #pragma mark - InterfaceAddressView public methods


status_t
InterfaceAddressView::Revert()
{
	// Populate address fields with current settings

// TODO!
/*
	int32 mode;
	if (fSettings->AutoConfigure(fFamily)) {
		mode = kModeAuto;
		_EnableFields(false);
	} else if (fSettings->IPAddr(fFamily).IsEmpty()) {
		mode = kModeDisabled;
		_EnableFields(false);
	} else {
		mode = kModeStatic;
		_EnableFields(true);
	}

	BMenuItem* item = fModePopUpMenu->FindItem(mode);
	if (item != NULL)
		item->SetMarked(true);

	if (!fSettings->IPAddr(fFamily).IsEmpty()) {
		fAddressField->SetText(fSettings->IP(fFamily));
		fNetmaskField->SetText(fSettings->Netmask(fFamily));
		fGatewayField->SetText(fSettings->Gateway(fFamily));
	}
*/
	return B_OK;
}


status_t
InterfaceAddressView::Save()
{
	BMenuItem* item = fModePopUpMenu->FindMarked();
	if (item == NULL)
		return B_ERROR;
/*
	fSettings->SetIP(fFamily, fAddressField->Text());
	fSettings->SetNetmask(fFamily, fNetmaskField->Text());
	fSettings->SetGateway(fFamily, fGatewayField->Text());
	fSettings->SetAutoConfigure(fFamily, item->Command() == kModeAuto);
*/
	return B_OK;
}


void
InterfaceAddressView::_UpdateFields()
{
	bool autoConfigure = (fInterface.Flags()
		& (IFF_AUTO_CONFIGURED | IFF_CONFIGURING)) != 0;

	BNetworkInterfaceAddress address;
	status_t status = B_ERROR;

	int32 index = fInterface.FindFirstAddress(fFamily);
	if (index >= 0)
		status = fInterface.GetAddressAt(index, address);
	if (index < 0 || status != B_OK
		|| address.Address().IsEmpty() && !autoConfigure) {
		if (status == B_OK) {
			// Check persistent settings for the mode -- the address
			// can also be empty if the automatic configuration hasn't
			// started yet.
			autoConfigure = fInterfaceSettings.IsEmpty()
				|| fInterfaceSettings.GetBool("auto_config", false);
		}
		if (!autoConfigure) {
			_SetModeField(kModeDisabled);
			return;
		}
	}

	if (autoConfigure)
		_SetModeField(kModeAuto);
	else
		_SetModeField(kModeStatic);

	fAddressField->SetText(address.Address().ToString());
	fNetmaskField->SetText(address.Mask().ToString());

	BNetworkAddress gateway;
	if (fInterface.GetDefaultRoute(fFamily, gateway) == B_OK)
		fGatewayField->SetText(gateway.ToString());
	else
		fGatewayField->SetText(NULL);
}


void
InterfaceAddressView::_SetModeField(uint32 mode)
{
	BMenuItem* item = fModePopUpMenu->FindItem(mode);
	if (item != NULL)
		item->SetMarked(true);

	_EnableFields(mode == kModeStatic);

	if (mode == kModeDisabled) {
		fAddressField->SetText(NULL);
		fNetmaskField->SetText(NULL);
		fGatewayField->SetText(NULL);
	}
}


/*!	Updates the current settings from the controls. */
void
InterfaceAddressView::_UpdateSettings()
{
}
