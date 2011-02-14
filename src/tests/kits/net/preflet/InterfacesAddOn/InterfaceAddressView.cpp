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
	SetLayout(new BGroupLayout(B_VERTICAL));

	// TODO : Draw address fields in a generic way
	BStringView* address = new BStringView(Bounds(), "address",
		fSettings->IP(family));

	AddChild(BGroupLayoutBuilder(B_VERTICAL, 10)
		.Add(address)
		.AddGlue()
		.SetInsets(10, 10, 10, 10)
	);

}


InterfaceAddressView::~InterfaceAddressView()
{

}


