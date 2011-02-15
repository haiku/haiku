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
#include <MenuItem.h>
#include <StringView.h>


InterfaceAddressView::InterfaceAddressView(BRect frame, const char* name,
	int family, NetworkSettings* settings)
	:
	BView(frame, name, B_FOLLOW_ALL_SIDES, 0),
	fSettings(settings),
	fFamily(family)
{
	float textControlW;
	float textControlH;

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

	fModeField = new BMenuField(frame, "mode", "Mode:",
		fModePopUpMenu, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP, B_WILL_DRAW);

	fAddressField = new BTextControl(frame, "address", "IP Address:",
		NULL, NULL, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);
	fNetmaskField = new BTextControl(frame, "netmask", "Netmask:",
		NULL, NULL, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);
	fGatewayField = new BTextControl(frame, "gateway", "Gateway:",
		NULL, NULL, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);

	fAddressField->GetPreferredSize(&textControlW, &textControlH);
	float labelSize = ( textControlW + 50 )
		- fAddressField->StringWidth("XXX.XXX.XXX.XXX");

	RevertFields();
		// Do the initial field population

	fModeField->SetDivider(labelSize);
	fAddressField->SetDivider(labelSize);
	fNetmaskField->SetDivider(labelSize);
	fGatewayField->SetDivider(labelSize);

	AddChild(BGroupLayoutBuilder(B_VERTICAL, 10)
		.Add(fModeField)
		.Add(fAddressField)
		.Add(fNetmaskField)
		.Add(fGatewayField)
		.AddGlue()
		.SetInsets(10, 10, 10, 10)
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
