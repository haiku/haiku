/*
 * Copyright 2004-2011 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Alexander von Gluck, kallisti5@unixzen.com
 */


#include "InterfaceWindow.h"

#include <Application.h>

#include <stdio.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "NetworkSetupWindow"


InterfaceWindow::InterfaceWindow(NetworkSettings* settings)
	:
	BWindow(BRect(50, 50, 370, 350), "Interface Settings",
		B_TITLED_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE,
		B_CURRENT_WORKSPACE)
{
	fNetworkSettings = settings;

	fTabView = new BTabView("settings_tabs");

	fApplyButton = new BButton("save", B_TRANSLATE("Save"),
		new BMessage(MSG_IP_SAVE));

	fRevertButton = new BButton("revert", B_TRANSLATE("Revert"),
		new BMessage(MSG_IP_REVERT));

	fTabView->SetResizingMode(B_FOLLOW_ALL);
		// ensure tab container matches window size

	_PopulateTabs();

	SetLayout(new BGroupLayout(B_VERTICAL));

	AddChild(BGroupLayoutBuilder(B_VERTICAL, 10)
		.Add(fTabView)
		.AddGroup(B_HORIZONTAL, 5)
			.AddGlue()
			.Add(fRevertButton)
			.Add(fApplyButton)
		.End()
		.SetInsets(10, 10, 10, 10)
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
			for (int index = 0; index < MAX_PROTOCOLS; index++)
			{
				if (supportedFamilies[index].present) {
					int inet_id = supportedFamilies[index].inet_id;
					fTabIPView[inet_id]->RevertFields();
				}
			}
			break;
		case MSG_IP_SAVE:
			for (int index = 0; index < MAX_PROTOCOLS; index++)
			{
				if (supportedFamilies[index].present) {
					int inet_id = supportedFamilies[index].inet_id;
					fTabIPView[inet_id]->SaveFields();
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

	BTab* hardwaretab = new BTab;
	fTabHardwareView = new InterfaceHardwareView(frame,
		fNetworkSettings);
	fTabView->AddTab(fTabHardwareView, hardwaretab);

	if (fNetworkSettings->IsEthernet())
		hardwaretab->SetLabel("Wired");
	else
		hardwaretab->SetLabel("Wirless");

	for (int index = 0; index < MAX_PROTOCOLS; index++)
	{
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

