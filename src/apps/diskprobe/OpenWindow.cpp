/*
 * Copyright 2009 Ankur Sethi <get.me.ankur@gmail.com>
 * Copyright 2004-2006, Axel Dörfler, axeld@pinc-software.de.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "OpenWindow.h"

#include <Application.h>
#include <Button.h>
#include <Directory.h>
#include <Entry.h>
#include <GroupLayout.h>
#include <GridLayoutBuilder.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Screen.h>

#include "DiskProbe.h"


static const uint32 kMsgProbeFile = 'prDv';
static const uint32 kMsgProbeDevice = 'prFl';


OpenWindow::OpenWindow()
	: BWindow(BRect(0, 0, 35, 10), "DiskProbe", B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS
			 | B_AUTO_UPDATE_SIZE_LIMITS)
{
	fDevicesMenu = new BPopUpMenu("devices");
	CollectDevices(fDevicesMenu);
	if (BMenuItem *item = fDevicesMenu->ItemAt(0))
		item->SetMarked(true);

	BMenuField *field = new BMenuField("Examine Device:", fDevicesMenu, NULL);

	BButton *probeDeviceButton = new BButton("device", "Probe Device",
		new BMessage(kMsgProbeDevice));
	probeDeviceButton->MakeDefault(true);

	BButton *probeFileButton = new BButton("file", "Probe File" B_UTF8_ELLIPSIS,
		new BMessage(kMsgProbeFile));

	BButton *cancelButton = new BButton("cancel", "Cancel",
		new BMessage(B_QUIT_REQUESTED));


	SetLayout(new BGroupLayout(B_HORIZONTAL));
	
	AddChild(BGridLayoutBuilder(8, 8)
		.Add(field, 0, 0, 3)
		.Add(cancelButton, 0, 1)
		.Add(probeFileButton, 1, 1)
		.Add(probeDeviceButton, 2, 1)
		.SetInsets(8, 8, 8, 8)
	);

	// TODO: This does not center the window, since with layout management,
	// the window will still have an invalid size at this point.
	BScreen screen(this);
	MoveTo(screen.Frame().left + (screen.Frame().Width() - Frame().Width()) / 2,
		screen.Frame().top + (screen.Frame().Height() - Frame().Height()) / 2);
}


OpenWindow::~OpenWindow()
{
}


void 
OpenWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kMsgProbeDevice: {
			BMenuItem *item = fDevicesMenu->FindMarked();
			if (item == NULL)
				break;

			be_app_messenger.SendMessage(item->Message());
			PostMessage(B_QUIT_REQUESTED);
			break;
		}

		case kMsgProbeFile:
			be_app_messenger.SendMessage(kMsgOpenFilePanel);
			PostMessage(B_QUIT_REQUESTED);
			break;

		case B_SIMPLE_DATA: {
			// if it's a file drop, open it
			entry_ref ref;
			if (message->FindRef("refs", 0, &ref) == B_OK) {
				BMessage openMessage(*message);
				openMessage.what = B_REFS_RECEIVED;

				be_app_messenger.SendMessage(&openMessage);
				PostMessage(B_QUIT_REQUESTED);
			}
			break;
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool 
OpenWindow::QuitRequested()
{
	be_app_messenger.SendMessage(kMsgOpenWindowClosed);
	return true;
}


void 
OpenWindow::CollectDevices(BMenu *menu, BEntry *startEntry)
{
	BDirectory directory;
	if (startEntry != NULL)
		directory.SetTo(startEntry);
	else
		directory.SetTo("/dev/disk");

	BEntry entry;
	while (directory.GetNextEntry(&entry) == B_OK) {
		if (entry.IsDirectory()) {
			CollectDevices(menu, &entry);
			continue;
		}

		entry_ref ref;
		if (entry.GetRef(&ref) != B_OK)
			continue;

		BPath path;
		if (entry.GetPath(&path) != B_OK)
			continue;

		BMessage *message = new BMessage(B_REFS_RECEIVED);
		message->AddRef("refs", &ref);

		menu->AddItem(new BMenuItem(path.Path(), message));
	}
}

