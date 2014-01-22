/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "App.h"

#include <stdio.h>

#include <Catalog.h>
#include <Entry.h>
#include <Message.h>
#include <String.h>

#include "support.h"

#include "MainWindow.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "App"


App::App()
	:
	BApplication("application/x-vnd.Haiku-HaikuDepot"),
	fMainWindow(NULL)
{
}


App::~App()
{
}


bool
App::QuitRequested()
{
	if (fMainWindow->LockLooperWithTimeout(1500000) == B_OK) {
		BMessage windowSettings;
		fMainWindow->StoreSettings(windowSettings);

		fMainWindow->UnlockLooper();

		_StoreSettings(windowSettings);
	}

	return true;
}


void
App::ReadyToRun()
{
	BRect frame(50.0, 50.0, 749.0, 549.0);

	BMessage settings;
	status_t status = load_settings(&settings, "main_settings", "HaikuDepot");
	if (status == B_OK) {
		settings.FindRect("window frame", &frame);
	}

	make_sure_frame_is_on_screen(frame);

	fMainWindow = new MainWindow(frame, settings);
	fMainWindow->Show();
}


void
App::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_MAIN_WINDOW_CLOSED:
		{
			BMessage windowSettings;
			if (message->FindMessage("window settings",
					&windowSettings) == B_OK) {
				_StoreSettings(windowSettings);
			}

			Quit();
			break;
		}

		default:
			BApplication::MessageReceived(message);
			break;
	}
}


void
App::_StoreSettings(const BMessage& windowSettings)
{
	save_settings(&windowSettings, "main_settings", "HaikuDepot");
}

