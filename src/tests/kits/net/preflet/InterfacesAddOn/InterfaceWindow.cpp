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
	switch (message->what) {
		case MSG_IP_REVERT:
			// RFC : we could check fTabView for Selection
			// here and only revert the selected tab.
			fIPv4TabView->RevertFields();
			fIPv6TabView->RevertFields();
			break;
		case MSG_IP_SAVE:
			fIPv4TabView->SaveFields();
			fIPv6TabView->SaveFields();
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
	fIPv4TabView = new InterfaceAddressView(frame, "net_settings_ipv4",
		AF_INET, fNetworkSettings);
	fIPv6TabView = new InterfaceAddressView(frame, "net_settings_ipv6",
		AF_INET6, fNetworkSettings);

	BTab* tab4 = new BTab;
	BTab* tab6 = new BTab;

	fTabView->AddTab(fIPv4TabView, tab4);
	tab4->SetLabel("IPv4");

	fTabView->AddTab(fIPv6TabView, tab6);
	tab6->SetLabel("IPv6");

	return B_OK;
}


bool
InterfaceWindow::QuitRequested()
{
	return true;
}

