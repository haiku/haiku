/*
 * Copyright 2004-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "OpenWindow.h"
#include "DiskProbe.h"

#include <Application.h>
#include <Screen.h>
#include <MenuField.h>
#include <PopUpMenu.h>
#include <MenuItem.h>
#include <Button.h>
#include <Directory.h>
#include <Entry.h>
#include <Path.h>


static const uint32 kMsgProbeFile = 'prDv';
static const uint32 kMsgProbeDevice = 'prFl';


OpenWindow::OpenWindow()
	: BWindow(BRect(0, 0, 35, 10), "DiskProbe", B_TITLED_WINDOW,
			B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS)
{
	BView *view = new BView(Bounds(), B_EMPTY_STRING, B_FOLLOW_ALL, B_WILL_DRAW);
	view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(view);

	fDevicesMenu = new BPopUpMenu("devices");
	CollectDevices(fDevicesMenu);
	if (BMenuItem *item = fDevicesMenu->ItemAt(0))
		item->SetMarked(true);

	BRect rect = Bounds().InsetByCopy(8, 8);
	BMenuField *field = new BMenuField(rect, "devices", "Examine Device:", fDevicesMenu);
	field->ResizeToPreferred();
	field->SetDivider(field->StringWidth(field->Label()) + 8);
	view->AddChild(field);

	BButton *probeDeviceButton = new BButton(BRect(10, 10, 20, 20), "file", "Probe Device",
		new BMessage(kMsgProbeDevice), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	probeDeviceButton->ResizeToPreferred();
	rect = probeDeviceButton->Bounds();
	probeDeviceButton->MoveTo(Bounds().Width() - 8 - rect.Width(),
		Bounds().Height() - 8 - rect.Height());
	view->AddChild(probeDeviceButton);

	// MakeDefault() may change the size and location of the button
	rect = probeDeviceButton->Frame();
	float width = rect.Width() + 58.0f;
	probeDeviceButton->MakeDefault(true);

	BButton *button = new BButton(rect, "file", "Probe File" B_UTF8_ELLIPSIS,
		new BMessage(kMsgProbeFile), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	button->ResizeToPreferred();
	button->MoveBy(-button->Bounds().Width() - 8, 0);
	view->AddChild(button);
	width += button->Bounds().Width();

	button = new BButton(button->Frame(), "cancel", "Cancel",
		new BMessage(B_QUIT_REQUESTED), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	button->ResizeToPreferred();
	button->MoveBy(-button->Bounds().Width() - 8, 0);
	view->AddChild(button);
	width += button->Bounds().Width();

	// make sure the window is large enough

	if (width < Bounds().Width())
		width = Bounds().Width();
	float height = button->Bounds().Height() + 24.0f + field->Frame().bottom;
	if (height < Bounds().Height())
		height = Bounds().Height();

	ResizeTo(width, height);

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

