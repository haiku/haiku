/*
 * Copyright 2004-2013 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck, kallisti5@unixzen.com
 *		John Scipione, jscipione@gmail.com
 */


#include "InterfaceHardwareView.h"
#include "NetworkSettings.h"

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

#include <stdio.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "IntefaceHardwareView"


// #pragma mark - InterfaceHardwareView


InterfaceHardwareView::InterfaceHardwareView(BRect frame,
	NetworkSettings* settings)
	:
	BGroupView(B_VERTICAL),
	fSettings(settings)
{
	SetLayout(new BGroupLayout(B_VERTICAL));

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

	Update();
		// Populate the fields

	BLayoutBuilder::Group<>(this)
		.AddGrid()
			.Add(status, 0, 0)
			.Add(fStatusField, 1, 0)
			.Add(macAddress, 0, 1)
			.Add(fMacAddressField, 1, 1)
			.Add(linkSpeed, 0, 2)
			.Add(fLinkSpeedField, 1, 2)
			.Add(linkTx, 0, 3)
			.Add(fLinkTxField, 1, 3)
			.Add(linkRx, 0, 4)
			.Add(fLinkRxField, 1, 4)
		.End()
		.AddGlue()
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

}


void
InterfaceHardwareView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		default:
			BView::MessageReceived(message);
	}
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

	return B_OK;
}


status_t
InterfaceHardwareView::Save()
{
	return B_OK;
}
