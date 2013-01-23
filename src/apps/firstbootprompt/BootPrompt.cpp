/*
 * Copyright 2010, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "BootPrompt.h"

#include <stdlib.h>

#include <Catalog.h>
#include <Locale.h>

#include "BootPromptWindow.h"


static int sExitValue;


int
main(int, char **)
{
	BootPromptApp app;
	app.Run();
	return sExitValue;
}


// #pragma mark -


const char* kAppSignature = "application/x-vnd.Haiku-FirstBootPrompt";


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
			sExitValue = 1;
			PostMessage(B_QUIT_REQUESTED);
			break;
		case MSG_RUN_INSTALLER:
			sExitValue = 0;
			PostMessage(B_QUIT_REQUESTED);
			break;

		default:
			BApplication::MessageReceived(message);
	}
}


void
BootPromptApp::ReadyToRun()
{
	// Prompt the user to select his preferred language.
	new BootPromptWindow();
}

