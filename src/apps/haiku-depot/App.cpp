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
	BApplication("application/x-vnd.Haiku-HaikuDepot")
{
}


App::~App()
{
}


bool
App::QuitRequested()
{
	if (fMainWindow->LockLooperWithTimeout(1500000) == B_OK) {
		BRect windowFrame = fMainWindow->Frame();
		fMainWindow->UnlockLooper();

		_StoreSettings(windowFrame);
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

	fMainWindow = new MainWindow(frame);
	fMainWindow->Show();
}


void
App::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_MAIN_WINDOW_CLOSED:
		{
			BRect windowFrame;
			if (message->FindRect("window frame", &windowFrame) == B_OK)
				_StoreSettings(windowFrame);

			Quit();
			break;
		}
		
		default:
			BApplication::MessageReceived(message);
			break;
	}
}


void
App::_StoreSettings(const BRect& windowFrame)
{
	BMessage settings;
	settings.AddRect("window frame", windowFrame);
	save_settings(&settings, "main_settings", "HaikuDepot");
}

