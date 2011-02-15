/*
 * Copyright 2004-2011 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Alexander von Gluck, kallisti5@unixzen.com
 */


#include "InterfaceAddressView.h"
#include "NetworkSettings.h"

#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <GridLayoutBuilder.h>
#include <MenuItem.h>
#include <StringView.h>


InterfaceAddressView::InterfaceAddressView(BRect frame, const char* name,
	int family, NetworkSettings* settings)
	:
	BView(frame, name, B_FOLLOW_ALL_SIDES, 0),
	fSettings(settings),
	fFamily(family)
{
	SetLayout(new BGroupLayout(B_VERTICAL));

	// Create our controls
	fModePopUpMenu = new BPopUpMenu("modes");

	fModePopUpMenu->AddItem(new BMenuItem("Automatic",
		new BMessage(AUTOSEL_MSG)));
	fModePopUpMenu->AddItem(new BMenuItem("Static",
		new BMessage(STATICSEL_MSG)));
	fModePopUpMenu->AddSeparatorItem();
	fModePopUpMenu->AddItem(new BMenuItem("None",
		new BMessage(NONESEL_MSG)));

	fModeField = new BMenuField("Mode:", fModePopUpMenu);

	fAddressField = new BTextControl("IP Address:", NULL, NULL);
	fNetmaskField = new BTextControl("Netmask:", NULL, NULL);
	fGatewayField = new BTextControl("Gateway:", NULL, NULL);

	RevertFields();
		// Do the initial field population

	AddChild(BGroupLayoutBuilder(B_VERTICAL, 10)
		.Add(BGridLayoutBuilder()
			.Add(fModeField->CreateLabelLayoutItem(), 0, 0)
			.Add(fModeField->CreateMenuBarLayoutItem(), 1, 0)
			.Add(fAddressField->CreateLabelLayoutItem(), 0, 1)
			.Add(fAddressField->CreateTextViewLayoutItem(), 1, 1)
			.Add(fNetmaskField->CreateLabelLayoutItem(), 0, 2)
			.Add(fNetmaskField->CreateTextViewLayoutItem(), 1, 2)
			.Add(fGatewayField->CreateLabelLayoutItem(), 0, 3)
			.Add(fGatewayField->CreateTextViewLayoutItem(), 1, 3)
		)
		.AddGlue()
	);
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
		case AUTOSEL_MSG:
			_EnableFields(false);
			break;
		case STATICSEL_MSG:
			_EnableFields(true);
			break;
		case NONESEL_MSG:
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

	const char* currMode = fSettings->AutoConfigure() ? "Automatic" : "Static";

	_EnableFields(!fSettings->AutoConfigure());
		// if Autoconfigured, disable address fields until changed

	// TODO : AutoConfigure needs to be based on family
	if (fSettings->IPAddr(fFamily).IsEmpty()
		&& !fSettings->AutoConfigure())
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
