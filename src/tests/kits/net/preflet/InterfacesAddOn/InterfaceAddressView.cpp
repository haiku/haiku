/*
 * Copyright 2004-2013 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck, kallisti5@unixzen.com
 *		John Scipione, jscipione@gmail.com
 */


#include "InterfaceAddressView.h"
#include "NetworkSettings.h"

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


// #pragma mark - InterfaceAddressView


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

	fModePopUpMenu->AddItem(new BMenuItem(B_TRANSLATE("DHCP"),
		new BMessage(M_MODE_AUTO)));
	fModePopUpMenu->AddItem(new BMenuItem(B_TRANSLATE("Static"),
		new BMessage(M_MODE_STATIC)));
	fModePopUpMenu->AddSeparatorItem();
	fModePopUpMenu->AddItem(new BMenuItem(B_TRANSLATE("Off"),
		new BMessage(M_MODE_OFF)));

	fModeField = new BMenuField(B_TRANSLATE("Mode:"), fModePopUpMenu);
	fModeField->SetToolTip(BString(B_TRANSLATE("The method for obtaining an IP address")));

	float minimumWidth = be_control_look->DefaultItemSpacing() * 16;

	fAddressField = new BTextControl(B_TRANSLATE("IP Address:"), NULL, NULL);
	fAddressField->SetToolTip(BString(B_TRANSLATE("Your IP address")));
	fAddressField->TextView()->SetExplicitMinSize(BSize(minimumWidth, B_SIZE_UNSET));
	fNetmaskField = new BTextControl(B_TRANSLATE("Netmask:"), NULL, NULL);
	fNetmaskField->SetToolTip(BString(B_TRANSLATE("Your netmask")));
	fNetmaskField->TextView()->SetExplicitMinSize(BSize(minimumWidth, B_SIZE_UNSET));
	fGatewayField = new BTextControl(B_TRANSLATE("Gateway:"), NULL, NULL);
	fGatewayField->SetToolTip(BString(B_TRANSLATE("Your gateway")));
	fGatewayField->TextView()->SetExplicitMinSize(BSize(minimumWidth, B_SIZE_UNSET));

	Revert();
		// Populate the fields

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
		case M_MODE_AUTO:
			_EnableFields(false);
			_ShowFields(true);
			break;

		case M_MODE_STATIC:
			_EnableFields(true);
			_ShowFields(true);
			break;

		case M_MODE_OFF:
			fAddressField->SetText("");
			fNetmaskField->SetText("");
			fGatewayField->SetText("");
			_EnableFields(false);
			_ShowFields(false);
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


void
InterfaceAddressView::_ShowFields(bool show)
{
	if (show) {
		if (fAddressField->IsHidden())
			fAddressField->Show();
		if (fNetmaskField->IsHidden())
			fNetmaskField->Show();
		if (fGatewayField->IsHidden())
			fGatewayField->Show();
	} else {
		if (!fAddressField->IsHidden())
			fAddressField->Hide();
		if (!fNetmaskField->IsHidden())
			fNetmaskField->Hide();
		if (!fGatewayField->IsHidden())
			fGatewayField->Hide();
	}
}


// #pragma mark - InterfaceAddressView public methods


status_t
InterfaceAddressView::Revert()
{
	// Populate address fields with current settings

	int32 mode;
	if (fSettings->AutoConfigure(fFamily)) {
		mode = M_MODE_AUTO;
		_EnableFields(false);
		_ShowFields(true);
	} else if (fSettings->IPAddr(fFamily).IsEmpty()) {
		mode = M_MODE_OFF;
		_EnableFields(false);
		_ShowFields(false);
	} else {
		mode = M_MODE_STATIC;
		_EnableFields(true);
		_ShowFields(true);
	}

	BMenuItem* item = fModePopUpMenu->FindItem(mode);
	if (item != NULL)
		item->SetMarked(true);

	if (!fSettings->IPAddr(fFamily).IsEmpty()) {
		fAddressField->SetText(fSettings->IP(fFamily));
		fNetmaskField->SetText(fSettings->Netmask(fFamily));
		fGatewayField->SetText(fSettings->Gateway(fFamily));
	}

	return B_OK;
}


status_t
InterfaceAddressView::Save()
{
	BMenuItem* item = fModePopUpMenu->FindMarked();
	if (item == NULL)
		return B_ERROR;

	fSettings->SetIP(fFamily, fAddressField->Text());
	fSettings->SetNetmask(fFamily, fNetmaskField->Text());
	fSettings->SetGateway(fFamily, fGatewayField->Text());
	fSettings->SetAutoConfigure(fFamily, item->Command() == M_MODE_AUTO);

	return B_OK;
}
