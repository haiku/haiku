/*
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

//static const char* kEULAText =
//"NOTICE: READ THIS BEFORE INSTALLING OR USING HAIKU\n\n"
//
//"Copyright " B_UTF8_COPYRIGHT " 2001-2009 The Haiku Project. All rights "
//"reserved. The copyright to the Haiku code is property of Haiku, Inc. or of "
//"the respective authors where expressly noted in the source.\n\n"
//
//"Permission is hereby granted, free of charge, to any person obtaining a "
//"copy of this software and associated documentation files (the \"Software\"), "
//"to deal in the Software without restriction, including without limitation "
//"the rights to use, copy, modify, merge, publish, distribute, sublicense, "
//"and/or sell copies of the Software, and to permit persons to whom the "
//"Software is furnished to do so, subject to the following conditions:\n\n"
//"The above copyright notice and this permission notice shall be included in "
//"all copies or substantial portions of the Software.\n\n"
//
//"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS "
//"OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, "
//"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE "
//"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER "
//"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING "
//"FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS "
//"IN THE SOFTWARE.";


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "InstallerApp"


int main(int, char **)
{
	InstallerApp theApp;
	theApp.Run();
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
			fEULAWindow->Lock();
			fEULAWindow->Quit();
		case kMsgNext:
			new InstallerWindow();
			break;

		default:
			BApplication::MessageReceived(message);
	}
}


void
InstallerApp::AboutRequested()
{
	BAlert *alert = new BAlert("about", B_TRANSLATE("Installer\n"
		"\twritten by Jérôme Duval and Stephan Aßmus\n"
		"\tCopyright 2005-2010, Haiku.\n\n"), B_TRANSLATE("OK"));
	alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
	BTextView *view = alert->TextView();
	BFont font;

	view->SetStylable(true);

	view->GetFont(&font);
	font.SetSize(18);
	font.SetFace(B_BOLD_FACE);
	view->SetFontAndColor(0, 9, &font);

	alert->Go();
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
		// Synchronize disks, and reboot the system
		sync();

		if (Utility::IsReadOnlyVolume("/boot")) {
			// Unblock CD tray, and eject the CD
			Utility::BlockMedia("/boot", false);
			Utility::EjectMedia("/boot");
		}

		// Quickly reboot without possibly touching anything on disk
		// (which we might just have ejected)
		_kern_shutdown(true);
	}
}
