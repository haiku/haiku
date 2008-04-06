/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "ActivityWindow.h"

#include <stdio.h>

#include <Application.h>
#include <File.h>
#include <FindDirectory.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Path.h>
#include <Roster.h>

#include "ActivityMonitor.h"
#include "ActivityView.h"


ActivityWindow::ActivityWindow()
	: BWindow(BRect(100, 100, 500, 250), "ActivityMonitor", B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE)
{
	BMessage settings;
	_LoadSettings(settings);

	BRect frame;
	if (settings.FindRect("window frame", &frame) == B_OK) {
		MoveTo(frame.LeftTop());
		ResizeTo(frame.Width(), frame.Height());
		frame.OffsetTo(B_ORIGIN);
	} else
		frame = Bounds();

	// create GUI

	BMenuBar* menuBar = new BMenuBar(Bounds(), "menu");
	AddChild(menuBar);

	frame.top = menuBar->Frame().bottom;

	BView* top = new BView(frame, NULL, B_FOLLOW_ALL, B_WILL_DRAW);
	top->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(top);

	fActivityView = new ActivityView(top->Bounds().InsetByCopy(10, 10),
		"ActivityMonitor", settings, B_FOLLOW_ALL);
	top->AddChild(fActivityView);

	// add menu

	// "File" menu
	BMenu* menu = new BMenu("File");
	BMenuItem* item;

	menu->AddItem(item = new BMenuItem("About ActivityMonitor" B_UTF8_ELLIPSIS,
		new BMessage(B_ABOUT_REQUESTED)));
	menu->AddSeparatorItem();

	menu->AddItem(new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q'));
	menu->SetTargetForItems(this);
	item->SetTarget(be_app);
	menuBar->AddItem(menu);
}


ActivityWindow::~ActivityWindow()
{
}


status_t
ActivityWindow::_OpenSettings(BFile& file, uint32 mode)
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return B_ERROR;

	path.Append("ActivityMonitor settings");

	return file.SetTo(path.Path(), mode);
}


status_t
ActivityWindow::_LoadSettings(BMessage& settings)
{
	BFile file;
	status_t status = _OpenSettings(file, B_READ_ONLY);
	if (status < B_OK)
		return status;

	return settings.Unflatten(&file);
}


status_t
ActivityWindow::_SaveSettings()
{
	BFile file;
	status_t status = _OpenSettings(file, B_WRITE_ONLY | B_CREATE_FILE
		| B_ERASE_FILE);
	if (status < B_OK)
		return status;

	BMessage settings('actm');
	status = settings.AddRect("window frame", Frame());
	if (status == B_OK)
		status = settings.Flatten(&file);

	return status;
}


void
ActivityWindow::_MessageDropped(BMessage* message)
{
	entry_ref ref;
	if (message->FindRef("refs", &ref) != B_OK) {
		// TODO: If app, then launch it, and add ActivityView for this one?
	}
}


void
ActivityWindow::MessageReceived(BMessage* message)
{
	if (message->WasDropped()) {
		_MessageDropped(message);
		return;
	}

	switch (message->what) {
		case B_REFS_RECEIVED:
		case B_SIMPLE_DATA:
			_MessageDropped(message);
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool
ActivityWindow::QuitRequested()
{
	_SaveSettings();
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}
