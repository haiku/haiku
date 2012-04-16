/*
 * Copyright 2009 Ankur Sethi <get.me.ankur@gmail.com>
 * Copyright 2004-2011, Axel Dörfler, axeld@pinc-software.de.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "OpenWindow.h"

#include <Application.h>
#include <Button.h>
#include <Catalog.h>
#include <Directory.h>
#include <Entry.h>
#include <GroupLayout.h>
#include <GridLayoutBuilder.h>
#include <Locale.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Screen.h>

#include "DiskProbe.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "OpenWindow"

static const uint32 kMsgProbeFile = 'prDv';
static const uint32 kMsgProbeDevice = 'prFl';


OpenWindow::OpenWindow()
	:
	BWindow(BRect(0, 0, 35, 10), B_TRANSLATE_SYSTEM_NAME("DiskProbe"),
		B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE
		| B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS)
{
	fDevicesMenu = new BPopUpMenu("devices");
	CollectDevices(fDevicesMenu);
	if (BMenuItem *item = fDevicesMenu->ItemAt(0))
		item->SetMarked(true);

	BMenuField *field = new BMenuField(B_TRANSLATE("Examine device:"),
		fDevicesMenu);

	BButton *probeDeviceButton = new BButton("device",
		B_TRANSLATE("Probe device"), new BMessage(kMsgProbeDevice));
	probeDeviceButton->MakeDefault(true);

	BButton *probeFileButton = new BButton("file",
		B_TRANSLATE("Probe file" B_UTF8_ELLIPSIS),
		new BMessage(kMsgProbeFile));

	BButton *cancelButton = new BButton("cancel", B_TRANSLATE("Cancel"),
		new BMessage(B_QUIT_REQUESTED));

	SetLayout(new BGroupLayout(B_HORIZONTAL));

	AddChild(BGridLayoutBuilder(8, 8)
		.Add(field, 0, 0, 3)
		.Add(cancelButton, 0, 1)
		.Add(probeFileButton, 1, 1)
		.Add(probeDeviceButton, 2, 1)
		.SetInsets(8, 8, 8, 8)
	);

	CenterOnScreen();
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

