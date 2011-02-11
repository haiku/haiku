/*
 * Copyright 2004-2011 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Alexander von Gluck, kallisti5@unixzen.com
 */


#include "SettingsWindow.h"

#include <Application.h>

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "NetworkSetupWindow"


SettingsWindow::SettingsWindow(NetworkSettings* settings)
	: BWindow(BRect(50, 50, 400, 302), "Interface Settings",
		B_TITLED_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
		B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE
		| B_AUTO_UPDATE_SIZE_LIMITS, B_CURRENT_WORKSPACE)
{
	fNetworkSettings = settings;

	fTabView = new BTabView("settings_tabs");

	fApplyButton = new BButton("apply", B_TRANSLATE("Apply"),
		new BMessage(APPLY_MSG));

	fCancelButton = new BButton("cancel", B_TRANSLATE("Cancel"),
		new BMessage(CANCEL_MSG));

	fTabView->SetResizingMode(B_FOLLOW_ALL);
		// ensure tab container matches window size

	_PopulateTabs();

	SetLayout(new BGroupLayout(B_VERTICAL));

	AddChild(BGroupLayoutBuilder(B_VERTICAL, 10)
		.Add(fTabView)
		.AddGroup(B_HORIZONTAL, 5)
			.AddGlue()
			.Add(fCancelButton)
			.Add(fApplyButton)
		.End()
		.SetInsets(10, 10, 10, 10)
	);
}


SettingsWindow::~SettingsWindow()
{
}


void
SettingsWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		default:
			BWindow::MessageReceived(message);
	}

}


status_t
SettingsWindow::_PopulateTabs()
{
	// TODO : Temporary, just to get example data
	BRect frame = fTabView->Bounds();
	BView* view4 = new BView(frame, "net_settings_ipv4",
		B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
	BView* view6 = new BView(frame, "net_settings_ipv6",
		B_FOLLOW_ALL_SIDES, B_WILL_DRAW);

	BTab* tab4 = new BTab;
	BTab* tab6 = new BTab;

	fTabView->AddTab(view4, tab4);
	tab4->SetLabel("IPv4");

	fTabView->AddTab(view6, tab6);
	tab6->SetLabel("IPv6");

	return B_OK;
}


bool
SettingsWindow::QuitRequested()
{
	return true;
}

