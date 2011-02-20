/*
 * Copyright 2004-2011 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Alexander von Gluck, kallisti5@unixzen.com
 */


#include "InterfaceWindow.h"

#include <Application.h>


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "NetworkSetupWindow"


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
	int* supportedFamilies = fNetworkSettings->ProtocolVersions();
	unsigned int index;

	switch (message->what) {
		case MSG_IP_REVERT:
			for (index = 0; index < sizeof(supportedFamilies); index++)
			{
				int protocol = supportedFamilies[index];
				if (protocol > 0)
					fTabIPView[protocol]->RevertFields();
			}
			break;
		case MSG_IP_SAVE:
			for (index = 0; index < sizeof(supportedFamilies); index++)
			{
				int protocol = supportedFamilies[index];
				if (protocol > 0)
					fTabIPView[protocol]->SaveFields();
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
	int* supportedFamilies = fNetworkSettings->ProtocolVersions();

	unsigned int index;
	for (index = 0; index < sizeof(supportedFamilies); index++)
	{
		int protocol = supportedFamilies[index];
		if (protocol > 0)
		{
			fTabIPView[protocol] = new InterfaceAddressView(frame,
				protocol, fNetworkSettings);
			BTab* tab = new BTab;
			fTabView->AddTab(fTabIPView[protocol], tab);
			tab->SetLabel((protocol == AF_INET) ? "IPv4" : "IPv6");
				// TODO : find a better way
		}
	}

	return B_OK;
}


bool
InterfaceWindow::QuitRequested()
{
	return true;
}

