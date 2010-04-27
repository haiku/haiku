/*
 * Copyright (c) 2005-2010, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		DarkWyrm <darkwyrm@gmail.com>
 */
#include "App.h"
#include "ResWindow.h"

#include <Entry.h>

int
main(void)
{
	App app;
	app.Run();
	return 0;
}


App::App(void)
  :	BApplication("application/x-vnd.Haiku-ResEdit")
{
	fOpenPanel = new BFilePanel();
}


App::~App(void)
{
	delete fOpenPanel;
}


void
App::ReadyToRun(void)
{
	// CountWindows() needs to be used instead of fWindowCount because the registration
	// message isn't processed in time. One of the windows belong to the BFilePanels and is
	// counted in CountWindows().
	if (CountWindows() < 2)
		new ResWindow(BRect(50, 100, 600, 400));
}


void
App::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		case M_SHOW_OPEN_PANEL: {
			// Don't do anything if it's already open
			if (fOpenPanel->IsShowing())
				break;
			fOpenPanel->Show();
			break;
		}
		default:
			BApplication::MessageReceived(msg);
	}
}


void
App::ArgvReceived(int32 argc, char** argv)
{
	for (int32 i = 1; i < argc; i++) {
		BEntry entry(argv[i]);
		entry_ref ref;
		if (entry.GetRef(&ref) < B_OK)
			continue;
		new ResWindow(BRect(50, 100, 600, 400), &ref);
	}
}


void
App::RefsReceived(BMessage *msg)
{
	entry_ref ref;
	int32 i = 0;
	while (msg->FindRef("refs", i++, &ref) == B_OK)
		new ResWindow(BRect(50, 100, 600, 400), &ref);
}
