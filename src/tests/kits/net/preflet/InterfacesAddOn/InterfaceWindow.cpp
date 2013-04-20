/*
 * Copyright 2004-2013 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Alexander von Gluck, kallisti5@unixzen.com
 *		John Scipione, jscipione@gmail.com
 */


#include "InterfaceWindow.h"

#include <Application.h>
#include <Button.h>
#include <Catalog.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <TabView.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "InterfaceWindow"


InterfaceWindow::InterfaceWindow(NetworkSettings* settings)
	:
	BWindow(BRect(50, 50, 370, 350), "Interface Settings",
		B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE | B_NOT_RESIZABLE
			| B_AUTO_UPDATE_SIZE_LIMITS, B_CURRENT_WORKSPACE)
{
	fNetworkSettings = settings;

	fTabView = new BTabView("settings_tabs");
	fTabView->SetTabWidth(B_WIDTH_FROM_LABEL);

	fRevertButton = new BButton("revert", B_TRANSLATE("Revert"),
		new BMessage(MSG_IP_REVERT));

	fApplyButton = new BButton("save", B_TRANSLATE("Save"),
		new BMessage(MSG_IP_SAVE));
	SetDefaultButton(fApplyButton);

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


InterfaceWindow::~InterfaceWindow()
{
}


void
InterfaceWindow::MessageReceived(BMessage* message)
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
			this->Quit();
			break;
		default:
			BWindow::MessageReceived(message);
	}

}


status_t
InterfaceWindow::_PopulateTabs()
{
	BRect frame = fTabView->Bounds();
	protocols* supportedFamilies = fNetworkSettings->ProtocolVersions();

	BTab* hardwareTab = new BTab;
	fTabHardwareView = new InterfaceHardwareView(frame,
		fNetworkSettings);
	fTabView->AddTab(fTabHardwareView, hardwareTab);
	hardwareTab->SetLabel(B_TRANSLATE("Interface"));

	for (int index = 0; index < MAX_PROTOCOLS; index++) {
		if (supportedFamilies[index].present) {
			int inet_id = supportedFamilies[index].inet_id;
			fTabIPView[inet_id] = new InterfaceAddressView(frame,
				inet_id, fNetworkSettings);
			BTab* tab = new BTab;
			fTabView->AddTab(fTabIPView[inet_id], tab);
			tab->SetLabel(supportedFamilies[index].name);
		}
	}

	return B_OK;
}


bool
InterfaceWindow::QuitRequested()
{
	return true;
}
