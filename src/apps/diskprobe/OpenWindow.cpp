/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
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
static const uint32 kMsgCancel = 'Canc';


OpenWindow::OpenWindow()
	: BWindow(BRect(0, 0, 350, 100), "DiskProbe", B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
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
	field->SetDivider(field->StringWidth(field->Label()) + 8);
	field->ResizeToPreferred();
	view->AddChild(field);

	BButton *button = new BButton(BRect(10, 10, 20, 20), "file", "Probe Device", new BMessage(kMsgProbeDevice));
	button->ResizeToPreferred();
	rect = button->Bounds();
	button->MoveTo(Bounds().Width() - 8 - rect.Width(), Bounds().Height() - 8 - rect.Height());
	view->AddChild(button);

	// MakeDefault() may change the size and location of the button
	rect = button->Frame();
	button->MakeDefault(true);

	button = new BButton(rect, "file", "Probe File" B_UTF8_ELLIPSIS, new BMessage(kMsgProbeFile));
	button->ResizeToPreferred();
	button->MoveBy(-button->Bounds().Width() - 8, 0);
	view->AddChild(button);

	button = new BButton(button->Frame(), "cancel", "Cancel", new BMessage(kMsgCancel));
	button->ResizeToPreferred();
	button->MoveBy(-button->Bounds().Width() - 8, 0);
	view->AddChild(button);

	BScreen screen;
	MoveTo((screen.Frame().Width() - Frame().Width()) / 2,
		(screen.Frame().Height() - Frame().Height()) / 2);
}


OpenWindow::~OpenWindow()
{
}


void 
OpenWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kMsgProbeDevice:
		{
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

		case kMsgCancel:
			if (QuitRequested())
				Quit();
			break;

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

