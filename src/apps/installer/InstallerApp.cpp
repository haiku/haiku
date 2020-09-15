/*
 * Copyright 2020, Panagiotis Vasilopoulos <hello@alwayslivid.com>
 * Copyright 2015, Axel Dörfler <axeld@pinc-software.de>
 * Copyright 2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2005, Jérôme DUVAL.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "InstallerApp.h"

#include <unistd.h>

#include <Alert.h>
#include <Roster.h>
#include <TextView.h>

#include <syscalls.h>

#include "tracker_private.h"
#include "Utility.h"


static const uint32 kMsgAgree = 'agre';
static const uint32 kMsgNext = 'next';

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "InstallerApp"


int main(int, char **)
{
	InstallerApp installer;
	installer.Run();
	return 0;
}


InstallerApp::InstallerApp()
	:
	BApplication("application/x-vnd.Haiku-Installer")
{
}


void
InstallerApp::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgAgree:
			fEULAWindow->PostMessage(B_QUIT_REQUESTED);
			break;
		case kMsgNext:
			new InstallerWindow();
			break;

		default:
			BApplication::MessageReceived(message);
	}
}


void
InstallerApp::ReadyToRun()
{
#if 1
	// Show the EULA first.
	fEULAWindow = new EULAWindow();
#else
	// Show the installer window without EULA.
	new InstallerWindow();
#endif
}


void
InstallerApp::Quit()
{
	BApplication::Quit();

	if (!be_roster->IsRunning(kDeskbarSignature)) {
		if (CurrentMessage()->GetBool("install_complete")) {
			// Synchronize disks
			sync();

			if (Utility::IsReadOnlyVolume("/boot")) {
				// Unblock CD tray, and eject the CD
				Utility::BlockMedia("/boot", false);
				Utility::EjectMedia("/boot");
			}

			// Quickly reboot without touching anything
			// on disk (which we might just have ejected)
			_kern_shutdown(true);
		} else {
			// Return to FirstBootPrompt if the user hasn't
			// installed Haiku yet
			be_roster->Launch("application/x-vnd.Haiku-FirstBootPrompt");
		}
	}
}
