/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "App.h"

#include <stdio.h>

#include <AboutWindow.h>
#include <Entry.h>
#include <Message.h>
#include <String.h>

#include "support.h"

#include "MainWindow.h"


App::App()
	:
	BApplication("application/x-vnd.Haiku-LaunchBox"),
	fSettingsChanged(false)
{
	SetPulseRate(3000000);
}


App::~App()
{
}


bool
App::QuitRequested()
{
	_StoreSettingsIfNeeded();
	return true;
}


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
		MainWindow* window = new MainWindow("Pad 1", frame, true);
		window->Show();
	}
}


void
App::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_ADD_WINDOW: {
			BMessage* settings = new BMessage('sett');
			bool wasCloned = message->FindMessage("window", settings) == B_OK;
			BString name("Pad ");
			name << CountWindows() + 1;
			MainWindow* window = new MainWindow(name.String(),
				BRect(50.0, 50.0, 65.0, 100.0), settings);
			if (wasCloned)
				window->MoveBy(10, 10);
			window->Show();
			fSettingsChanged = true;
			break;
		}
		case MSG_SETTINGS_CHANGED:
			fSettingsChanged = true;
			break;
		default:
			BApplication::MessageReceived(message);
			break;
	}
}


void
App::AboutRequested()
{
	const char* authors[2];
	authors[0] = "Stephan Aßmus (aka stippi)";
	authors[1] = NULL;
	BAboutWindow("LaunchBox", 2004, authors).Show();
}


void
App::Pulse()
{
	_StoreSettingsIfNeeded();
}


void
App::_StoreSettingsIfNeeded()
{
	if (!fSettingsChanged)
		return;

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

	fSettingsChanged = false;
}
