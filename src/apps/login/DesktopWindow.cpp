/*
 * Copyright 2008, François Revol, <revol@free.fr>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <string.h>
#include <stdio.h>

#include <Catalog.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Screen.h>
#include <View.h>

#include "LoginApp.h"
#include "DesktopWindow.h"

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "Desktop Window"

const window_feel kPrivateDesktopWindowFeel = window_feel(1024);
const window_look kPrivateDesktopWindowLook = window_look(4);
	// this is a mirror of an app server private values


DesktopWindow::DesktopWindow(BRect frame, bool editMode)
	: BWindow(frame, B_TRANSLATE("Desktop"), 
		kPrivateDesktopWindowLook, 
		kPrivateDesktopWindowFeel, 
		B_NOT_MOVABLE | B_NOT_CLOSABLE | B_NOT_ZOOMABLE
		 | B_NOT_MINIMIZABLE | B_NOT_RESIZABLE
		 | B_ASYNCHRONOUS_CONTROLS,
		editMode?B_CURRENT_WORKSPACE:B_ALL_WORKSPACES),
	  fEditShelfMode(editMode)
{
	BScreen screen;
	BView *desktop = new BView(Bounds(), "desktop", B_FOLLOW_NONE, 0);
	desktop->SetViewColor(screen.DesktopColor());
	AddChild(desktop);

	// load the shelf
	BPath path;
	status_t err;
	entry_ref ref;
	err = find_directory(B_COMMON_SETTINGS_DIRECTORY, &path, true);
	if (err >= B_OK) {
		BDirectory dir(path.Path());
		if (!dir.Contains("x-vnd.Haiku-Login", B_DIRECTORY_NODE))
			dir.CreateDirectory("x-vnd.Haiku-Login", NULL);
		path.Append("x-vnd.Haiku-Login");
		dir.SetTo(path.Path());
		if (!dir.Contains("Shelf", B_FILE_NODE))
			dir.CreateFile("Shelf", NULL);
		path.Append("Shelf");
		get_ref_for_path(path.Path(), &ref);
	}

	fDesktopShelf = new BShelf(&ref, desktop, fEditShelfMode, "DesktopShelf");
	if (fDesktopShelf)
		fDesktopShelf->SetDisplaysZombies(true);
}


DesktopWindow::~DesktopWindow()
{
	delete fDesktopShelf;
}


bool
DesktopWindow::QuitRequested()
{
	status_t err;
	err = fDesktopShelf->Save();
	printf(B_TRANSLATE_COMMENT("error %s\n",
		"A return message from fDesktopShelf->Save(). It can be \"B_OK\""),
		strerror(err));
	return BWindow::QuitRequested();
}


void
DesktopWindow::DispatchMessage(BMessage *message, BHandler *handler)
{
	switch (message->what) {
		case B_MOUSE_DOWN:
		case B_MOUSE_UP:
		case B_MOUSE_MOVED:
		case B_KEY_DOWN:
		case B_KEY_UP:
		case B_UNMAPPED_KEY_DOWN:
		case B_UNMAPPED_KEY_UP:
			/* don't allow interacting with the replicants */
			if (!fEditShelfMode)
				break;
		default:
		BWindow::DispatchMessage(message, handler);
	}
}


