/*
 * Copyright 2010, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "BootPrompt.h"

#include <stdlib.h>

#include <AboutWindow.h>
#include <Locale.h>

#include "BootPromptWindow.h"


int
main(int, char **)
{
	BootPromptApp app;
	app.Run();
	return 0;
}


// #pragma mark -


const char* kAppSignature = "application/x-vnd.Haiku-ReadOnlyBootPrompt";


BootPromptApp::BootPromptApp()
	:
	BApplication(kAppSignature)
{
}


void
BootPromptApp::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_BOOT_DESKTOP:
			// TODO: Exit with some value that the Bootscript can deal with.
			exit(1);
			break;
		case MSG_RUN_INSTALLER:
			// TODO: Exit with some value that the Bootscript can deal with.
			exit(0);
			break;

		default:
			BApplication::MessageReceived(message);
	}
}


void
BootPromptApp::AboutRequested()
{
	const char* kAuthors[] = {
		"Stephan Aßmus",
		NULL
	};

	BAboutWindow* aboutWindow = new BAboutWindow("ReadOnlyBootPrompt", 2010,
		kAuthors);

	aboutWindow->Show();
}


void
BootPromptApp::ReadyToRun()
{
	// Prompt the user to select his preferred language.
	new BootPromptWindow();
}

