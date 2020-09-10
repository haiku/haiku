/*
 * Copyright 2010, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2020, Panagiotis Vasilopoulos <hello@alwayslivid.com>
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "BootPrompt.h"

#include <stdlib.h>

#include <Catalog.h>
#include <LaunchRoster.h>
#include <Locale.h>

#include <syscalls.h>


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
const char* kDeskbarSignature = "application/x-vnd.Be-TSKB";


BootPromptApp::BootPromptApp()
	:
	BApplication(kAppSignature)
{
}


void
BootPromptApp::MessageReceived(BMessage* message)
{
	switch (message->what) {
		// Booting the desktop or running the installer both result
		// in sending a B_QUIT_REQUESTED message that ultimately
		// closes the FirstBootPrompt window. However, according to
		// BootPromptWindow::QuitRequested(), if the BootPromptWindow
		// is not running on a desktop and the user decides to close
		// the window using the button, the user will be asked if
		// they wish to reboot their system.
		//
		// Asking that in the former scenarios would not make much
		// sense. "dont_reboot" explicitly states that the user does
		// not wish to reboot their system, which suppresses the
		// confirm box.
		case MSG_BOOT_DESKTOP:
		{
			BLaunchRoster().Target("desktop");
			sExitValue = 1;

			BMessage* newMessage = new BMessage(B_QUIT_REQUESTED);
			newMessage->AddBool("dont_reboot", true);
			PostMessage(newMessage);
			break;
		}
		case MSG_RUN_INSTALLER:
		{
			BLaunchRoster().Target("installer");
			sExitValue = 0;

			BMessage* newMessage = new BMessage(B_QUIT_REQUESTED);
			newMessage->AddBool("dont_reboot", true);
			PostMessage(newMessage);
			break;
		}
		case MSG_REBOOT_REQUESTED:
		{
			_kern_shutdown(true);
			sExitValue = -1;
			break;
		}

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

