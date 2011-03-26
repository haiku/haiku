/*
 * Copyright 2004-2006, Jérôme DUVAL. All rights reserved.
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
ExpanderApp::ArgvReceived(int32 argc, char **argv)
{
	BMessage* msg = NULL;
	for (int32 i = 1; i < argc; i++) {
		entry_ref ref;
		status_t err = get_ref_for_path(argv[i], &ref);
		if (err == B_OK) {
			if (!msg) {
				msg = new BMessage;
				msg->what = B_REFS_RECEIVED;
			}
			msg->AddRef("refs", &ref);
		}
	}
	if (msg)
		RefsReceived(msg);
}


void
ExpanderApp::RefsReceived(BMessage* msg)
{
	BMessenger messenger(fWindow);
	msg->AddBool("fromApp", true);
	messenger.SendMessage(msg);
}


void
ExpanderApp::UpdateSettingsFrom(BMessage* message)
{
	fSettings.UpdateFrom(message);
}


//	#pragma mark -


int
main(int, char **)
{
	ExpanderApp theApp;
	theApp.Run();
	return 0;
}
