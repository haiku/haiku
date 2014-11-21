/*
 * Copyright 2004-2013 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck, kallisti5@unixzen.com
 *		John Scipione, jscipione@gmail.com
 */


#include "InterfaceHardwareView.h"

#include "InterfaceView.h"
#include "NetworkSettings.h"
#include "WirelessNetworkMenuItem.h"

#include <Button.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <LayoutBuilder.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <NetworkAddress.h>
#include <Screen.h>
#include <Size.h>
#include <StringView.h>
#include <TextControl.h>

#include <set>
#include <stdio.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "IntefaceHardwareView"


// #pragma mark - InterfaceHardwareView


InterfaceHardwareView::InterfaceHardwareView(NetworkSettings* settings)
	:
	BGroupView(B_VERTICAL),
	fSettings(settings)
{
	SetLayout(new BGroupLayout(B_VERTICAL));

	SetFlags(Flags() | B_PULSE_NEEDED);

	// TODO : Small graph of throughput?

	float minimumWidth = be_control_look->DefaultItemSpacing() * 16;

	BStringView* status = new BStringView("status label", B_TRANSLATE("Status:"));
	status->SetAlignment(B_ALIGN_RIGHT);
	fStatusField = new BStringView("status field", "");
	fStatusField->SetExplicitMinSize(BSize(minimumWidth, B_SIZE_UNSET));
	BStringView* macAddress = new BStringView("mac address label",
		B_TRANSLATE("MAC address:"));
	macAddress->SetAlignment(B_ALIGN_RIGHT);
	fMacAddressField = new BStringView("mac address field", "");
	fMacAddressField->SetExplicitMinSize(BSize(minimumWidth, B_SIZE_UNSET));
	BStringView* linkSpeed = new BStringView("link speed label",
		B_TRANSLATE("Link speed:"));
	linkSpeed->SetAlignment(B_ALIGN_RIGHT);
	fLinkSpeedField = new BStringView("link speed field", "");
	fLinkSpeedField->SetExplicitMinSize(BSize(minimumWidth, B_SIZE_UNSET));

	// TODO: These metrics may be better in a BScrollView?
	BStringView* linkTx = new BStringView("tx label",
		B_TRANSLATE("Sent:"));
	linkTx->SetAlignment(B_ALIGN_RIGHT);
	fLinkTxField = new BStringView("tx field", "");
	fLinkTxField ->SetExplicitMinSize(BSize(minimumWidth, B_SIZE_UNSET));
	BStringView* linkRx = new BStringView("rx label",
		B_TRANSLATE("Received:"));
	linkRx->SetAlignment(B_ALIGN_RIGHT);
	fLinkRxField = new BStringView("rx field", "");
	fLinkRxField ->SetExplicitMinSize(BSize(minimumWidth, B_SIZE_UNSET));

	fNetworkMenuField = new BMenuField(B_TRANSLATE("Network:"), new BMenu(
		B_TRANSLATE("Choose automatically")));
	fNetworkMenuField->SetAlignment(B_ALIGN_RIGHT);
	fNetworkMenuField->Menu()->SetLabelFromMarked(true);

	// Construct the BButtons
	fOnOff = new BButton("onoff", B_TRANSLATE("Disable"),
		new BMessage(kMsgInterfaceToggle));

	fRenegotiate = new BButton("heal", B_TRANSLATE("Renegotiate"),
		new BMessage(kMsgInterfaceRenegotiate));
	fRenegotiate->SetEnabled(false);

	BLayoutBuilder::Group<>(this)
		.AddGrid()
			.Add(status, 0, 0)
			.Add(fStatusField, 1, 0)
			.Add(fNetworkMenuField->CreateLabelLayoutItem(), 0, 1)
			.Add(fNetworkMenuField->CreateMenuBarLayoutItem(), 1, 1)
			.Add(macAddress, 0, 2)
			.Add(fMacAddressField, 1, 2)
			.Add(linkSpeed, 0, 3)
			.Add(fLinkSpeedField, 1, 3)
			.Add(linkTx, 0, 4)
			.Add(fLinkTxField, 1, 4)
			.Add(linkRx, 0, 5)
			.Add(fLinkRxField, 1, 5)
		.End()
		.AddGlue()
		.AddGroup(B_HORIZONTAL)
			.Add(fOnOff)
			.AddGlue()
			.Add(fRenegotiate)
		.End()
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
			B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);
}


InterfaceHardwareView::~InterfaceHardwareView()
{

}


// #pragma mark - InterfaceHardwareView virtual methods


void
InterfaceHardwareView::AttachedToWindow()
{
	Update();
		// Populate the fields

	fOnOff->SetTarget(this);
	fRenegotiate->SetTarget(this);
}


void
InterfaceHardwareView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgNetwork:
		{
			fSettings->SetWirelessNetwork(message->FindString("name"));
			break;
		}
		case kMsgInterfaceToggle:
		{
			fSettings->SetDisabled(!fSettings->IsDisabled());
			Update();
			Window()->FindView("interfaces")->Invalidate();
			break;
		}

		case kMsgInterfaceRenegotiate:
		{
			fSettings->RenegotiateAddresses();
			break;
		}

		default:
			BView::MessageReceived(message);
	}
}


void
InterfaceHardwareView::Pulse()
{
	// TODO maybe not everything needs to be updated here.
	Update();
}


// #pragma mark - InterfaceHardwareView public methods


status_t
InterfaceHardwareView::Revert()
{
	Update();
	return B_OK;
}


status_t
InterfaceHardwareView::Update()
{
	// Populate fields with current settings
	if (fSettings->HasLink()) {
		if (fSettings->IsWireless()) {
			BString network = fSettings->WirelessNetwork();
			network.Prepend(" (");
			network.Prepend(B_TRANSLATE("connected"));
			network.Append(")");
			fStatusField->SetText(network.String());
		} else {
			fStatusField->SetText(B_TRANSLATE("connected"));
		}
	} else
		fStatusField->SetText(B_TRANSLATE("disconnected"));

	fMacAddressField->SetText(fSettings->HardwareAddress());

	// TODO : Find how to get link speed
	fLinkSpeedField->SetText("100 Mb/s");

	// Update Link stats
	ifreq_stats stats;
	char buffer[100];
	fSettings->Stats(&stats);
	snprintf(buffer, sizeof(buffer), B_TRANSLATE("%" B_PRIu64 " KBytes"),
		stats.send.bytes / 1024);
	fLinkTxField->SetText(buffer);

	snprintf(buffer, sizeof(buffer), B_TRANSLATE("%" B_PRIu64 " KBytes"),
		stats.receive.bytes / 1024);
	fLinkRxField->SetText(buffer);

	// TODO move the wireless info to a separate tab. We should have a
	// BListView of available networks, rather than a menu, to make them more
	// readable and easier to browse and select.
	if (fNetworkMenuField->IsHidden(fNetworkMenuField)
		&& fSettings->IsWireless()) {
		fNetworkMenuField->Show();
	} else if (!fNetworkMenuField->IsHidden(fNetworkMenuField)
		&& !fSettings->IsWireless()) {
		fNetworkMenuField->Hide();
	}

	if (fSettings->IsWireless()) {
		// Rebuild network menu
		BMenu* menu = fNetworkMenuField->Menu();
		menu->RemoveItems(0, menu->CountItems(), true);

		std::set<BNetworkAddress> associated;
		BNetworkAddress address;
		uint32 cookie = 0;
		while (fSettings->GetNextAssociatedNetwork(cookie, address) == B_OK)
			associated.insert(address);

		wireless_network network;
		int32 count = 0;
		cookie = 0;
		while (fSettings->GetNextNetwork(cookie, network) == B_OK) {
			BMessage* message = new BMessage(kMsgNetwork);

			message->AddString("device", fSettings->Name());
			message->AddString("name", network.name);

			BMenuItem* item = new WirelessNetworkMenuItem(network.name,
				network.signal_strength,
				network.authentication_mode, message);
			if (associated.find(network.address) != associated.end())
				item->SetMarked(true);
			menu->AddItem(item);

			count++;
		}
		if (count == 0) {
			BMenuItem* item = new BMenuItem(
				B_TRANSLATE("<no wireless networks found>"), NULL);
			item->SetEnabled(false);
			menu->AddItem(item);
		} else {
			BMenuItem* item = new BMenuItem(
				B_TRANSLATE("Choose automatically"), NULL);
			if (menu->FindMarked() == NULL)
				item->SetMarked(true);
			menu->AddItem(item, 0);
			menu->AddItem(new BSeparatorItem(), 1);
		}
		menu->SetTargetForItems(this);
	}

	fRenegotiate->SetEnabled(!fSettings->IsDisabled());
	fOnOff->SetLabel(fSettings->IsDisabled() ? "Enable" : "Disable");

	return B_OK;
}


status_t
InterfaceHardwareView::Save()
{
	return B_OK;
}
