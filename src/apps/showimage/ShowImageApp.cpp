/*
 * Copyright 2003-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Fernando Francisco de Oliveira
 *		Michael Wilber
 *		Michael Pfeiffer
 *		Ryan Leavengood
 */


#include "ShowImageApp.h"

#include <stdio.h>

#include <Catalog.h>
#include <Clipboard.h>
#include <FilePanel.h>
#include <Locale.h>
#include <Path.h>
#include <Screen.h>
#include <String.h>

#include "ShowImageConstants.h"
#include "ShowImageWindow.h"
#include "ToolBarIcons.h"


const char* kApplicationSignature = "application/x-vnd.Haiku-ShowImage";
const int32 kWindowsToIgnore = 1;
	// ignore the always open file panel


ShowImageApp::ShowImageApp()
	:
	BApplication(kApplicationSignature),
	fOpenPanel(new BFilePanel(B_OPEN_PANEL)),
	fPulseStarted(false),
	fLastWindowFrame(BRect(30, 30, 430, 330))
{
	B_TRANSLATE_MARK_SYSTEM_NAME_VOID("ShowImage");
	_UpdateLastWindowFrame();
	// BBitmap can be created after there is a BApplication instance.
	init_tool_bar_icons();
}


ShowImageApp::~ShowImageApp()
{
	// BBitmap must be deleted while there is still a BApplication instance.
	uninit_tool_bar_icons();
}


void
ShowImageApp::ArgvReceived(int32 argc, char **argv)
{
	BMessage message;
	bool hasRefs = false;

	// get current working directory
	const char* cwd;
	if (CurrentMessage() == NULL
		|| CurrentMessage()->FindString("cwd", &cwd) != B_OK)
		cwd = "";

	for (int32 i = 1; i < argc; i++) {
		BPath path;
		if (argv[i][0] == '/') {
			// absolute path
			path.SetTo(argv[i]);
		} else {
			// relative path
			path.SetTo(cwd);
			path.Append(argv[i]);
		}

		entry_ref ref;
		status_t err = get_ref_for_path(path.Path(), &ref);
		if (err == B_OK) {
			message.AddRef("refs", &ref);
			hasRefs = true;
		}
	}

	if (hasRefs)
		RefsReceived(&message);
}


void
ShowImageApp::ReadyToRun()
{
	if (CountWindows() == kWindowsToIgnore)
		fOpenPanel->Show();
	else {
		// If image windows are already open
		// (paths supplied on the command line)
		// start checking the number of open windows
		_StartPulse();
	}

	be_clipboard->StartWatching(be_app_messenger);
		// tell the clipboard to notify this app when its contents change
}


void
ShowImageApp::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_FILE_OPEN:
			fOpenPanel->Show();
			break;

		case B_CANCEL:
			// File open panel was closed,
			// start checking count of open windows
			_StartPulse();
			break;

		case B_CLIPBOARD_CHANGED:
			_CheckClipboard();
			break;

		case MSG_WINDOW_HAS_QUIT:
			// Make sure that new windows open with the location/size of the
			// last closed window.
			_UpdateLastWindowFrame();
			break;

		default:
			BApplication::MessageReceived(message);
			break;
	}
}


void
ShowImageApp::Pulse()
{
	// Bug: The BFilePanel is automatically closed if the volume that
	// is displayed is unmounted.
	if (!IsLaunching() && CountWindows() <= kWindowsToIgnore) {
		// If the application is not launching and
		// all windows are closed except for the file open panel,
		// quit the application
		PostMessage(B_QUIT_REQUESTED);
	}
}


void
ShowImageApp::RefsReceived(BMessage* message)
{
	// If a tracker window opened me, get a messenger from it.
	BMessenger trackerMessenger;
	if (message->HasMessenger("TrackerViewToken"))
		message->FindMessenger("TrackerViewToken", &trackerMessenger);

	entry_ref ref;
	for (int32 i = 0; message->FindRef("refs", i, &ref) == B_OK; i++)
		_Open(ref, trackerMessenger);
}


bool
ShowImageApp::QuitRequested()
{
	// Give the windows a chance to prompt the user if there are changes
	bool result = BApplication::QuitRequested();
	if (result) {
		be_clipboard->StopWatching(be_app_messenger);
			// tell clipboard we don't want anymore notification
	}

	return result;
}


void
ShowImageApp::_StartPulse()
{
	if (!fPulseStarted) {
		// Tell the app to begin checking
		// for the number of open windows
		fPulseStarted = true;
		SetPulseRate(250000);
			// Set pulse to every 1/4 second
	}
}


void
ShowImageApp::_Open(const entry_ref& ref, const BMessenger& trackerMessenger)
{
	fLastWindowFrame.OffsetBy(20, 20);
	if (!BScreen(B_MAIN_SCREEN_ID).Frame().Contains(fLastWindowFrame))
		fLastWindowFrame.OffsetTo(50, 50);

	new ShowImageWindow(fLastWindowFrame, ref, trackerMessenger);
}


void
ShowImageApp::_BroadcastToWindows(BMessage* message)
{
	const int32 count = CountWindows();
	for (int32 i = 0; i < count; i++) {
		// BMessenger checks for us if BWindow is still a valid object
		BMessenger messenger(WindowAt(i));
		messenger.SendMessage(message);
	}
}


void
ShowImageApp::_CheckClipboard()
{
	// Determines if the contents of the clipboard contain
	// data that is useful to this application.
	// After checking the clipboard, a message is sent to
	// all windows indicating that the clipboard has changed
	// and whether or not the clipboard contains useful data.
	bool dataAvailable = false;

	if (be_clipboard->Lock()) {
		BMessage* clip = be_clipboard->Data();
		if (clip != NULL) {
			dataAvailable = clip->HasMessage("image/bitmap")
				|| clip->HasMessage("image/x-be-bitmap");
		}

		be_clipboard->Unlock();
	}

	BMessage msg(B_CLIPBOARD_CHANGED);
	msg.AddBool("data_available", dataAvailable);
	_BroadcastToWindows(&msg);
}


void
ShowImageApp::_UpdateLastWindowFrame()
{
	fLastWindowFrame = fSettings.GetRect("WindowFrame", fLastWindowFrame);
	// Compensate the offset which we always add to new windows.
	fLastWindowFrame.OffsetBy(-20, -20);
}


//	#pragma mark -


int
main(int, char**)
{
	ShowImageApp app;
	app.Run();
	return 0;
}

