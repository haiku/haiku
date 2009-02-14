/*
 * Copyright 2008-09, Oliver Ruiz Dorantes, <oliver.ruiz.dorantes_at_gmail.com>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "BluetoothWindow.h"


#include <Button.h>
#include <GroupLayoutBuilder.h>
#include <Messenger.h>
#include <SpaceLayoutItem.h>
#include <TabView.h>


#include "RemoteDevicesView.h"

//#include "ConnChanView.h"
#include "defs.h"

static const uint32 kMsgSetDefaults = 'dflt';
static const uint32 kMsgRevert = 'rvrt';

static const uint32 kMsgStartServices = 'SrSR';
static const uint32 kMsgStopServices = 'StST';
static const uint32 kMsgShowDebug = 'ShDG';


BluetoothWindow::BluetoothWindow(BRect frame)
 :	BWindow(frame, "Bluetooth", B_TITLED_WINDOW,
 		B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS,
 		B_ALL_WORKSPACES)
{
	SetLayout(new BGroupLayout(B_HORIZONTAL));

	fDefaultsButton = new BButton("defaults", "Defaults",
		new BMessage(kMsgSetDefaults), B_WILL_DRAW);

	fRevertButton = new BButton("revert", "Revert",
		new BMessage(kMsgRevert), B_WILL_DRAW);

	// Add the menu bar
	fMenubar = new BMenuBar(Bounds(), "menu_bar");

	// Add File menu to menu bar
	BMenu *menu = new BMenu("Server");
	menu->AddItem(new BMenuItem("Start Bluetooth Services" B_UTF8_ELLIPSIS, new BMessage(kMsgStartServices), 0));
	menu->AddItem(new BMenuItem("Stop Bluetooth Services" B_UTF8_ELLIPSIS, new BMessage(kMsgStopServices), 0));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("Show Bluetooth console" B_UTF8_ELLIPSIS, new BMessage(kMsgStartServices), 0));
	fMenubar->AddItem(menu);
	
	menu = new BMenu("View");
	menu->AddItem(new BMenuItem("Connections & Channels list" B_UTF8_ELLIPSIS, NULL, 0));
	menu->AddItem(new BMenuItem("Remote Devices List" B_UTF8_ELLIPSIS, NULL, 0));	
	fMenubar->AddItem(menu);
	
	menu = new BMenu("Help");
	menu->AddItem(new BMenuItem("About" B_UTF8_ELLIPSIS, new BMessage(B_ABOUT_REQUESTED), 0));
	fMenubar->AddItem(menu);

	BTabView* tabView = new BTabView("tabview", B_WIDTH_FROM_LABEL);

	fSettingsView = new BluetoothSettingsView("Settings");
//	fConnChan = new ConnChanView("Connections & Channels", B_WILL_DRAW);
	fRemoteDevices = new RemoteDevicesView("Remote Devices", B_WILL_DRAW);

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
BluetoothWindow::MessageReceived(BMessage *message)
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
