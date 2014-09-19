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
	BView("Interface Settings", 0, 0)
{
	fNetworkSettings = settings;

	fTabView = new BTabView("settings_tabs");
	fTabView->SetTabWidth(B_WIDTH_FROM_LABEL);

	fRevertButton = new BButton("revert", B_TRANSLATE("Revert"),
		new BMessage(MSG_IP_REVERT));

	fApplyButton = new BButton("save", B_TRANSLATE("Save"),
		new BMessage(MSG_IP_SAVE));

	_PopulateTabs();

	SetLayout(new BGroupLayout(B_VERTICAL));

	AddChild(BGroupLayoutBuilder(B_VERTICAL, B_USE_SMALL_SPACING)
		.Add(fTabView)
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.Add(fRevertButton)
			.AddGlue()
			.Add(fApplyButton)
		.End()
		.SetInsets(B_USE_SMALL_SPACING, B_USE_SMALL_SPACING,
			B_USE_SMALL_SPACING, B_USE_SMALL_SPACING)
	);
}


InterfaceView::~InterfaceView()
{
}


void
InterfaceView::AttachedToWindow()
{
	Window()->SetDefaultButton(fApplyButton);
}


void
InterfaceView::MessageReceived(BMessage* message)
{
	protocols* supportedFamilies = fNetworkSettings->ProtocolVersions();

	switch (message->what) {
		case MSG_IP_REVERT:
			for (int index = 0; index < MAX_PROTOCOLS; index++) {
				if (supportedFamilies[index].present) {
					int inet_id = supportedFamilies[index].inet_id;
					fTabIPView[inet_id]->Revert();
				}
			}
			break;
		case MSG_IP_SAVE:
			for (int index = 0; index < MAX_PROTOCOLS; index++) {
				if (supportedFamilies[index].present) {
					int inet_id = supportedFamilies[index].inet_id;
					fTabIPView[inet_id]->Save();
				}
			}
			break;
		default:
			BView::MessageReceived(message);
	}

}


status_t
InterfaceView::_PopulateTabs()
{
	protocols* supportedFamilies = fNetworkSettings->ProtocolVersions();

	BTab* hardwareTab = new BTab;
	fTabHardwareView = new InterfaceHardwareView(fNetworkSettings);
	fTabView->AddTab(fTabHardwareView, hardwareTab);
	hardwareTab->SetLabel(B_TRANSLATE("Interface"));

	for (int index = 0; index < MAX_PROTOCOLS; index++) {
		if (supportedFamilies[index].present) {
			int inet_id = supportedFamilies[index].inet_id;
			fTabIPView[inet_id] = new InterfaceAddressView(inet_id,
				fNetworkSettings);
			BTab* tab = new BTab;
			fTabView->AddTab(fTabIPView[inet_id], tab);
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
