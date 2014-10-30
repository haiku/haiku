/*
 * Copyright 2004-2013 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Alexander von Gluck, kallisti5@unixzen.com
 *		John Scipione, jscipione@gmail.com
 */


#include "InterfaceView.h"

#include <Application.h>
#include <Button.h>
#include <Catalog.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <TabView.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "InterfaceWindow"


InterfaceView::InterfaceView(NetworkSettings* settings)
	:
	BTabView("settings_tabs")
{
	fNetworkSettings = settings;
	SetTabWidth(B_WIDTH_FROM_LABEL);
	_PopulateTabs();
}


InterfaceView::~InterfaceView()
{
}


void
InterfaceView::Revert()
{
	protocols* supportedFamilies = fNetworkSettings->ProtocolVersions();

	for (int index = 0; index < MAX_PROTOCOLS; index++) {
		if (supportedFamilies[index].present) {
			int inet_id = supportedFamilies[index].inet_id;
			fTabIPView[inet_id]->Revert();
		}
	}
}


void
InterfaceView::Apply()
{
	protocols* supportedFamilies = fNetworkSettings->ProtocolVersions();

	for (int index = 0; index < MAX_PROTOCOLS; index++) {
		if (supportedFamilies[index].present) {
			int inet_id = supportedFamilies[index].inet_id;
			fTabIPView[inet_id]->Save();
		}
	}
}


status_t
InterfaceView::_PopulateTabs()
{
	protocols* supportedFamilies = fNetworkSettings->ProtocolVersions();

	BTab* hardwareTab = new BTab;
	fTabHardwareView = new InterfaceHardwareView(fNetworkSettings);
	AddTab(fTabHardwareView, hardwareTab);
	hardwareTab->SetLabel(B_TRANSLATE("Interface"));

	for (int index = 0; index < MAX_PROTOCOLS; index++) {
		if (supportedFamilies[index].present) {
			int inet_id = supportedFamilies[index].inet_id;
			fTabIPView[inet_id] = new InterfaceAddressView(inet_id,
				fNetworkSettings);
			BTab* tab = new BTab;
			AddTab(fTabIPView[inet_id], tab);
			tab->SetLabel(supportedFamilies[index].name);
		}
	}

	return B_OK;
}


bool
InterfaceView::QuitRequested()
{
	return true;
}
