/*
 * Copyright (c) 2008 Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT/X11 license.
 *
 * Copyright (c) 1999 Mike Steed. You are free to use and distribute this
 * software as long as it is accompanied by it's documentation and this
 * copyright notice. The software comes with no warranty, etc.
 */


#include "App.h"

#include <stdio.h>

#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <Node.h>
#include <Path.h>

#include "DiskUsage.h"
#include "MainWindow.h"


App::App()
	:
	BApplication(kAppSignature),
	fMainWindow(NULL),
	fSavedRefsReceived(NULL)
{
}


App::~App()
{
	delete fSavedRefsReceived;
}


void
App::ArgvReceived(int32 argc, char** argv)
{
	BMessage refsReceived(B_REFS_RECEIVED);
	for (int32 i = 1; i < argc; i++) {
		BEntry entry(argv[i], true);
		entry_ref ref;
		if (entry.GetRef(&ref) == B_OK)
			refsReceived.AddRef("refs", &ref);
	}
	if (refsReceived.HasRef("refs"))
		PostMessage(&refsReceived);
}


void
App::RefsReceived(BMessage* message)
{
	if (!message->HasRef("refs") && message->HasRef("dir_ref")) {
		entry_ref dirRef;
		if (message->FindRef("dir_ref", &dirRef) == B_OK)
			message->AddRef("refs", &dirRef);
	}

	if (fMainWindow == NULL) {
		// ReadyToRun() has not been called yet, this happens when someone
		// launches us with a B_REFS_RECEIVED message.
		delete fSavedRefsReceived;
		fSavedRefsReceived = new BMessage(*message);
	} else
		fMainWindow->PostMessage(message);
}


void
App::ReadyToRun()
{
	BRect frame;

	BPath path;
	BFile settingsFile;
	BMessage settings;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK
		|| path.Append("DiskUsage") != B_OK
		|| settingsFile.SetTo(path.Path(), B_READ_ONLY) != B_OK
		|| settings.Unflatten(&settingsFile) != B_OK
		|| settings.FindRect("window frame", &frame) != B_OK) {
		// use default window frame
		frame.Set(0, 0, kDefaultPieSize, kDefaultPieSize);
		frame.OffsetTo(50, 50);
	}

	fMainWindow = new MainWindow(frame);
	fMainWindow->Show();

	if (fSavedRefsReceived) {
		// RefsReceived() was called earlier than ReadyToRun()
		fMainWindow->PostMessage(fSavedRefsReceived);
		delete fSavedRefsReceived;
		fSavedRefsReceived = NULL;
	}
}


bool
App::QuitRequested()
{
	// Save the settings.
	BPath path;
	BFile settingsFile;
	BMessage settings;
	if (settings.AddRect("window frame", fMainWindow->Frame()) != B_OK
		|| find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK
		|| path.Append("DiskUsage") != B_OK
		|| settingsFile.SetTo(path.Path(),
			B_CREATE_FILE | B_WRITE_ONLY | B_ERASE_FILE) != B_OK
		|| settings.Flatten(&settingsFile) != B_OK) {
		fprintf(stderr, "Failed to write application settings.\n");
	}

	return BApplication::QuitRequested();
}
