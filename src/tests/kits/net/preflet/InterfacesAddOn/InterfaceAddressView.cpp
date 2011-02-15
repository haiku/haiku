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
	fAddressField = new BTextControl(frame, "address", "IP Address:",
		NULL, NULL, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);
	fNetmaskField = new BTextControl(frame, "netmask", "Netmask:",
		NULL, NULL, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);

	fAddressField->GetPreferredSize(&textControlW, &textControlH);
	float labelSize = ( textControlW + 50 )
		- fAddressField->StringWidth("XXX.XXX.XXX.XXX");

	_RevertFields();
		// Do the initial field population

	fAddressField->SetDivider(labelSize);
	fNetmaskField->SetDivider(labelSize);

	AddChild(BGroupLayoutBuilder(B_VERTICAL, 10)
		.Add(fAddressField)
		.Add(fNetmaskField)
		.AddGlue()
		.SetInsets(10, 10, 10, 10)
	);
}


InterfaceAddressView::~InterfaceAddressView()
{

}


status_t
InterfaceAddressView::_RevertFields()
{
	// Populate address fields with current settings
	fAddressField->SetText(fSettings->IP(fFamily));
	fNetmaskField->SetText(fSettings->Netmask(fFamily));

	return B_OK;
}
