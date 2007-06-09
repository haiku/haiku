/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include <stdio.h>

#include <Alert.h>
#include <Entry.h>
#include <Message.h>
#include <String.h>

#include "support.h"

#include "MainWindow.h"

#include "App.h"

// constructor
App::App()
	: BApplication("application/x.vnd-YellowBites.LaunchBox")
{
}

// destructor
App::~App()
{
}

// QuitRequested
bool
App::QuitRequested()
{
	BMessage settings('sett');
	for (int32 i = 0; BWindow* window = WindowAt(i); i++) {
		if (MainWindow* padWindow = dynamic_cast<MainWindow*>(window)) {
			BMessage* windowSettings = padWindow->Settings();
			if (windowSettings && padWindow->Lock()) {
				padWindow->SaveSettings(windowSettings);
				padWindow->Unlock();
				settings.AddMessage("window", windowSettings);
			}
		}
	}
	save_settings(&settings, "main_settings", "LaunchBox");
	return true;
}

// ReadyToRun
void
App::ReadyToRun()
{
	bool windowAdded = false;
	BRect frame(50.0, 50.0, 65.0, 100.0);

	BMessage settings('sett');
	status_t status = load_settings(&settings, "main_settings", "LaunchBox");
	if (status >= B_OK) {
		BMessage windowMessage;
		for (int32 i = 0; settings.FindMessage("window", i, &windowMessage) >= B_OK; i++) {
			BString name("Pad ");
			name << i + 1;
			BMessage* windowSettings = new BMessage(windowMessage);
			MainWindow* window = new MainWindow(name.String(), frame, windowSettings);
			window->Show();
			windowAdded = true;
			frame.OffsetBy(10.0, 10.0);
			windowMessage.MakeEmpty();
		}
	}
	
	if (!windowAdded) {
		MainWindow* window = new MainWindow("Pad 1", frame);
		window->Show();
	}
}

// MessageReceived
void
App::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_ADD_WINDOW: {
			BMessage* settings = new BMessage('sett');
			message->FindMessage("window", settings);
			BString name("Pad ");
			name << CountWindows() + 1;
			MainWindow* window = new MainWindow(name.String(),
												BRect(50.0, 50.0, 65.0, 100.0), settings);
			window->Show();
			break;
		}
		default:
			BApplication::MessageReceived(message);
			break;
	}
}

// AboutRequested
void
App::AboutRequested()
{
	(new BAlert("about", "LaunchBox by stippi\n\n"
						 "for bonefish\n\n\n"
						 "v1.1.0",
						 "Neat", NULL, NULL))->Go(NULL);
}
