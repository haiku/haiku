/*
 * Copyright 2004-2011 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Alexander von Gluck, kallisti5@unixzen.com
 */


#include "InterfaceAddressView.h"
#include "NetworkSettings.h"

#include <LayoutBuilder.h>
#include <MenuItem.h>
#include <StringView.h>


InterfaceAddressView::InterfaceAddressView(BRect frame, int family,
	NetworkSettings* settings)
	:
	BGroupView(B_VERTICAL),
	fSettings(settings),
	fFamily(family)
{
	SetLayout(new BGroupLayout(B_VERTICAL));

	// Create our controls
	fModePopUpMenu = new BPopUpMenu("modes");

	fModePopUpMenu->AddItem(new BMenuItem("Automatic",
		new BMessage(M_MODE_AUTO)));
	fModePopUpMenu->AddItem(new BMenuItem("Static",
		new BMessage(M_MODE_STATIC)));
	fModePopUpMenu->AddSeparatorItem();
	fModePopUpMenu->AddItem(new BMenuItem("None",
		new BMessage(M_MODE_NONE)));

	fModeField = new BMenuField("Mode:", fModePopUpMenu);
	fModeField->SetToolTip(BString("The method for obtaining an IP address"));

	fAddressField = new BTextControl("IP Address:", NULL, NULL);
	fAddressField->SetToolTip(BString("Your internet protocol address"));
	fNetmaskField = new BTextControl("Netmask:", NULL, NULL);
	fNetmaskField->SetToolTip(BString("Your netmask (subnet)"));
	fGatewayField = new BTextControl("Gateway:", NULL, NULL);
	fGatewayField->SetToolTip(BString("Your gateway (router)"));

	RevertFields();
		// Do the initial field population

	BLayoutBuilder::Group<>(this)
		.AddGrid()
			.AddMenuField(fModeField, 0, 0, B_ALIGN_RIGHT)
			.AddTextControl(fAddressField, 0, 1, B_ALIGN_RIGHT)
			.AddTextControl(fNetmaskField, 0, 2, B_ALIGN_RIGHT)
			.AddTextControl(fGatewayField, 0, 3, B_ALIGN_RIGHT)
		.End()
		.AddGlue()
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
			B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);
}


InterfaceAddressView::~InterfaceAddressView()
{

}


void
InterfaceAddressView::AttachedToWindow()
{
	fModePopUpMenu->SetTargetForItems(this);
}


void
InterfaceAddressView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case M_MODE_AUTO:
			_EnableFields(false);
			break;
		case M_MODE_STATIC:
			_EnableFields(true);
			break;
		case M_MODE_NONE:
			_EnableFields(false);
			fAddressField->SetText("");
			fNetmaskField->SetText("");
			fGatewayField->SetText("");
			break;
		default:
			BView::MessageReceived(message);
	}
}


void
InterfaceAddressView::_EnableFields(bool enabled)
{
	fAddressField->SetEnabled(enabled);
	fNetmaskField->SetEnabled(enabled);
	fGatewayField->SetEnabled(enabled);
}


status_t
InterfaceAddressView::RevertFields()
{
	// Populate address fields with current settings

	const char* currMode = fSettings->AutoConfigure(fFamily)
		? "Automatic" : "Static";

	_EnableFields(!fSettings->AutoConfigure(fFamily));
		// if Autoconfigured, disable address fields until changed

	if (fSettings->IPAddr(fFamily).IsEmpty()
		&& !fSettings->AutoConfigure(fFamily))
	{
		currMode = "None";
		_EnableFields(false);
	}

	BMenuItem* item = fModePopUpMenu->FindItem(currMode);
	if (item)
		item->SetMarked(true);

	fAddressField->SetText(fSettings->IP(fFamily));
	fNetmaskField->SetText(fSettings->Netmask(fFamily));
	fGatewayField->SetText(fSettings->Gateway(fFamily));

	return B_OK;
}


status_t
InterfaceAddressView::SaveFields()
{
	fSettings->SetIP(fFamily, fAddressField->Text());
	fSettings->SetNetmask(fFamily, fNetmaskField->Text());
	fSettings->SetGateway(fFamily, fGatewayField->Text());

	BMenuItem* item = fModePopUpMenu->FindItem("Automatic");
	fSettings->SetAutoConfigure(fFamily, item->IsMarked());

	return B_OK;
}

