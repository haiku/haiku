/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "ProbeWindow.h"
#include "OpenWindow.h"
#include "DiskProbe.h"
#include "ProbeView.h"

#include <Application.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Entry.h>
#include <Path.h>

#include <string.h>
#include <stdio.h>


ProbeWindow::ProbeWindow(BRect rect, entry_ref *ref, const char *attribute)
	: BWindow(rect, ref->name, B_DOCUMENT_WINDOW, B_ASYNCHRONOUS_CONTROLS),
	fRef(*ref)
{
	// Set alternative title for certain cases

	if (attribute != NULL) {
		char buffer[256];
		snprintf(buffer, sizeof(buffer), "%s: %s", ref->name, attribute);
		SetTitle(buffer);
	} else {
		BPath path(ref);
		if (!strncmp("/dev/", path.Path(), 5))
			SetTitle(path.Path());
	}

	// add the menu

	BMenuBar *menuBar = new BMenuBar(BRect(0, 0, 0, 0), NULL);
	AddChild(menuBar);

	BMenu *menu = new BMenu("File");
	menu->AddItem(new BMenuItem("New" B_UTF8_ELLIPSIS,
					new BMessage(kMsgOpenOpenWindow), 'N', B_COMMAND_KEY));

	BMenu *devicesMenu = new BMenu("Open Device");
	OpenWindow::CollectDevices(devicesMenu);
	devicesMenu->SetTargetForItems(be_app);
	menu->AddItem(new BMenuItem(devicesMenu));

	menu->AddItem(new BMenuItem("Open File" B_UTF8_ELLIPSIS,
					new BMessage(kMsgOpenFilePanel), 'O', B_COMMAND_KEY));
	menu->AddSeparatorItem();

	// the ProbeView file menu items will be inserted here
	menu->AddSeparatorItem();

	menu->AddItem(new BMenuItem("About DiskProbe" B_UTF8_ELLIPSIS, new BMessage(B_ABOUT_REQUESTED)));
	menu->AddSeparatorItem();

	menu->AddItem(new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q', B_COMMAND_KEY));
	menu->SetTargetForItems(be_app);
	menuBar->AddItem(menu);

	// add our interface widgets

	BRect rect = Bounds();
	rect.top = menuBar->Bounds().Height() + 1;
	ProbeView *probeView = new ProbeView(rect, ref, attribute);
	probeView->AddFileMenuItems(menu, menu->CountItems() - 4);
	AddChild(probeView);
}


ProbeWindow::~ProbeWindow()
{
}


void 
ProbeWindow::MessageReceived(BMessage *message)
{
	BWindow::MessageReceived(message);
}


bool 
ProbeWindow::QuitRequested()
{
	be_app_messenger.SendMessage(kMsgWindowClosed);
	return true;
}

