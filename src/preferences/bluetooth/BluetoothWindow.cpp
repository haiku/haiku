/*
 * Copyright 2008-10, Oliver Ruiz Dorantes, <oliver.ruiz.dorantes_at_gmail.com>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "BluetoothWindow.h"


#include <Button.h>
#include <Catalog.h>
#include <GroupLayoutBuilder.h>
#include <Messenger.h>
#include <SpaceLayoutItem.h>
#include <TabView.h>
#include <Roster.h>
#include <stdio.h>

#include <bluetooth/LocalDevice.h>
#include "RemoteDevicesView.h"

#include "defs.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Window"

static const uint32 kMsgSetDefaults = 'dflt';
static const uint32 kMsgRevert = 'rvrt';

static const uint32 kMsgStartServices = 'SrSR';
static const uint32 kMsgStopServices = 'StST';
static const uint32 kMsgShowDebug = 'ShDG';

LocalDevice* ActiveLocalDevice = NULL;


BluetoothWindow::BluetoothWindow(BRect frame)
	:
	BWindow(frame, B_TRANSLATE_SYSTEM_NAME("Bluetooth"), B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS)
{
	SetLayout(new BGroupLayout(B_HORIZONTAL));

	fDefaultsButton = new BButton("defaults", B_TRANSLATE("Defaults"),
		new BMessage(kMsgSetDefaults), B_WILL_DRAW);

	fRevertButton = new BButton("revert", B_TRANSLATE("Revert"),
		new BMessage(kMsgRevert), B_WILL_DRAW);

	// Add the menu bar
	fMenubar = new BMenuBar(Bounds(), "menu_bar");

	// Add File menu to menu bar
	BMenu* menu = new BMenu(B_TRANSLATE("Server"));
	menu->AddItem(new BMenuItem(
		B_TRANSLATE("Start bluetooth services" B_UTF8_ELLIPSIS),
		new BMessage(kMsgStartServices), 0));
	menu->AddItem(new BMenuItem(
		B_TRANSLATE("Stop bluetooth services" B_UTF8_ELLIPSIS),
		new BMessage(kMsgStopServices), 0));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(
		B_TRANSLATE("Show bluetooth console" B_UTF8_ELLIPSIS),
		new BMessage(kMsgStartServices), 0));
	menu->AddItem(new BMenuItem(
		B_TRANSLATE("Refresh LocalDevices" B_UTF8_ELLIPSIS),
		new BMessage(kMsgRefresh), 0));
	fMenubar->AddItem(menu);

	menu = new BMenu(B_TRANSLATE("View"));
	menu->AddItem(new BMenuItem(
		B_TRANSLATE("Connections & channels" B_UTF8_ELLIPSIS), NULL, 0));
	menu->AddItem(new BMenuItem(
		B_TRANSLATE("Remote devices list" B_UTF8_ELLIPSIS), NULL, 0));
	fMenubar->AddItem(menu);

	menu = new BMenu(B_TRANSLATE("Help"));
	menu->AddItem(new BMenuItem(B_TRANSLATE("About Bluetooth" B_UTF8_ELLIPSIS),
		new BMessage(B_ABOUT_REQUESTED), 0));
	fMenubar->AddItem(menu);

	BTabView* tabView = new BTabView("tabview", B_WIDTH_FROM_LABEL);

	fSettingsView = new BluetoothSettingsView(B_TRANSLATE("Settings"));
//	fConnChan = new ConnChanView("Connections & Channels", B_WILL_DRAW);
	fRemoteDevices = new RemoteDevicesView(
		B_TRANSLATE("Remote devices"), B_WILL_DRAW);

	tabView->AddTab(fRemoteDevices);
//	tabView->AddTab(fConnChan);
	tabView->AddTab(fSettingsView);


	fRevertButton->SetEnabled(false);

	AddChild(BGroupLayoutBuilder(B_VERTICAL, 0)
		.Add(fMenubar)
		.Add(BSpaceLayoutItem::CreateVerticalStrut(5))
		.Add(tabView)
		.Add(BSpaceLayoutItem::CreateVerticalStrut(5))
		.Add(BGroupLayoutBuilder(B_HORIZONTAL, 0)
			.Add(fRevertButton)
			.AddGlue()
			.Add(fDefaultsButton)
		)
		.SetInsets(5, 5, 5, 5)
	);
}


void
BluetoothWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgSetDefaults:
/*			fColorsView -> MessageReceived(new BMessage(DEFAULT_SETTINGS));
			fAntialiasingSettings->SetDefaults();
			fDefaultsButton->SetEnabled(false);
			fRevertButton->SetEnabled(true);
*/			break;

		case kMsgRevert:
/*			fColorsView -> MessageReceived(new BMessage(REVERT_SETTINGS));
			fAntialiasingSettings->Revert();
			fDefaultsButton->SetEnabled(fColorsView->IsDefaultable()
								|| fAntialiasingSettings->IsDefaultable());
			fRevertButton->SetEnabled(false);
*/			break;
		case kMsgStartServices:
			printf("kMsgStartServices\n");
			if (!be_roster->IsRunning(BLUETOOTH_SIGNATURE)) {
				status_t error;

				error = be_roster->Launch(BLUETOOTH_SIGNATURE);
				printf("kMsgStartServices: %s\n", strerror(error));
			}
			break;

		case kMsgStopServices:
			printf("kMsgStopServices\n");
			if (be_roster->IsRunning(BLUETOOTH_SIGNATURE)) {
				status_t error;

				error = BMessenger(BLUETOOTH_SIGNATURE).SendMessage(B_QUIT_REQUESTED);
				printf("kMsgStopServices: %s\n", strerror(error));
			}
			break;

		case kMsgAddToRemoteList:
			PostMessage(message, fRemoteDevices);
			break;
		case kMsgRefresh:
			fSettingsView->MessageReceived(message);
			break;
		case B_ABOUT_REQUESTED:
			be_app->PostMessage(message);
			break;
		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool
BluetoothWindow::QuitRequested(void)
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}
