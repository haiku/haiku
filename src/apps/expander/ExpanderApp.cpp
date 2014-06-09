/*
 * Copyright 2004-2006, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "ExpanderApp.h"

#include "ExpanderWindow.h"


ExpanderApp::ExpanderApp()
	:
	BApplication("application/x-vnd.Haiku-Expander")
{
	BPoint windowPosition = fSettings.Message().FindPoint("window_position");
	BRect windowFrame(0, 0, 450, 120);
	windowFrame.OffsetBy(windowPosition);
	BMessage settings(fSettings.Message());
	fWindow = new ExpanderWindow(windowFrame, NULL, &settings);
}


void
ExpanderApp::ArgvReceived(int32 argc, char** argv)
{
	BMessage* message = NULL;
	for (int32 i = 1; i < argc; i++) {
		entry_ref ref;
		status_t err = get_ref_for_path(argv[i], &ref);
		if (err == B_OK) {
			if (message == NULL) {
				message = new BMessage;
				message->what = B_REFS_RECEIVED;
			}
			message->AddRef("refs", &ref);
		}
	}

	if (message != NULL)
		RefsReceived(message);
}


void
ExpanderApp::RefsReceived(BMessage* message)
{
	BMessenger messenger(fWindow);
	message->AddBool("fromApp", true);
	messenger.SendMessage(message);
}


void
ExpanderApp::UpdateSettingsFrom(BMessage* message)
{
	fSettings.UpdateFrom(message);
}


//	#pragma mark - main method


int
main(int argc, char** argv)
{
	ExpanderApp theApp;
	theApp.Run();

	return 0;
}
