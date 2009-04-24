/*
 * Copyright 2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2005, Jérôme DUVAL.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "InstallerApp.h"

#include <Alert.h>
#include <Button.h>
#include <GroupLayoutBuilder.h>
#include <Screen.h>
#include <ScrollView.h>
#include <TextView.h>


static const uint32 kMsgAgree = 'agre';

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


static const char* kInfoText =
"IMPORTANT INFORMATION BEFORE INSTALLING HAIKU\n\n"

"This is alpha-quality software! It means there is a high risk of loosing "
"important data. Make frequent backups! You have been warned.\n\n"

"1)  If you are installing Haiku onto real hardware (not inside an emulator) "
"you need to have prepared a harddisk partition. The Installer and the "
"DriveSetup tool will only offer to initialize existing partitions with the "
"Haiku native filesystem. We recommend using the GParted Live-CD, it can also "
"resize partitions to make room. We do not feel confident in enabling our own "
"code to mess with your harddisk partition table. If you have not created a "
"partition yet, simply reboot, create the partition using whatever tool you "
"feel most comfortable with, and reboot into Haiku to continue with the "
"installation.\n\n"

"2)  The Installer will take no steps to integrate Haiku into an existig boot "
"menu. The Haiku partition itself will be made bootable. If you have GRUB "
"already installed, edit your /boot/grub/menu.1st by launching your favorite "
"editor from a Terminal like this:\n\n"

"\tsudo <your favorite text editor> /boot/grub/menu.lst\n\n"

"You'll note that GRUB uses a different naming strategy for hard drives than "
"Linux.\n\n"

"With GRUB it's: (hdN,n)\n\n"

"All harddisks start with \"hd\"\n"
"\"N\" is the hard disk number, starting with \"0\".\n"
"\"n\" is the partition number, also starting with \"0\".\n"
"The first logical partition always has the number 4, regardless of the "
"number of primary partitions.\n\n"

"So behind the other menu entries towards the bottom of the file, add "
"something similar to these lines:\n\n"

"\t# Haiku on /dev/sda7\n"
"\ttitle\t\t\t\tHaiku\n"
"\trootnoverify\t\t(hd0,6)\n"
"\tchainloader\t\t+1\n\n"

"You can see the correct partition in gparted for example.\n\n"

"3)  When you successfully boot into Haiku for the first time, make sure to read "
"our \"Welcome\" documentation, there is a link on the Desktop.\n\n"

"Have fun and thanks a lot for trying out Haiku! We hope you like it!"
;


int main(int, char **)
{
	InstallerApp theApp;
	theApp.Run();
	return 0;
}


InstallerApp::InstallerApp()
	: BApplication("application/x-vnd.Haiku-Installer")
{
}


void
InstallerApp::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgAgree:
			fEULAWindow->Lock();
			fEULAWindow->Quit();
			new InstallerWindow();
			break;

		default:
			BApplication::MessageReceived(message);
	}
}


void
InstallerApp::AboutRequested()
{
	BAlert *alert = new BAlert("about", "Installer\n"
		"\twritten by Jérôme Duval\n"
		"\tCopyright 2005, Haiku.\n\n", "Ok");
	BTextView *view = alert->TextView();
	BFont font;

	view->SetStylable(true);

	view->GetFont(&font);
	font.SetSize(18);
	font.SetFace(B_BOLD_FACE);
	view->SetFontAndColor(0, 14, &font);

	alert->Go();
}


void
InstallerApp::ReadyToRun()
{
#if 1
	// Show the EULA first.
	BTextView* textView = new BTextView("eula", be_plain_font, NULL,
		B_WILL_DRAW);
	textView->SetInsets(10, 10, 10, 10);
	textView->MakeEditable(false);
	textView->MakeSelectable(false);
	textView->SetText(kInfoText);

	BScrollView* scrollView = new BScrollView("eulaScroll",
		textView, B_WILL_DRAW, false, true);

	BButton* cancelButton = new BButton("Quit",
		new BMessage(B_QUIT_REQUESTED));
	cancelButton->SetTarget(this);

	BButton* continueButton = new BButton("Continue",
		new BMessage(kMsgAgree));
	continueButton->SetTarget(this);
	continueButton->MakeDefault(true);

	BRect eulaFrame = BRect(0, 0, 600, 450);
	fEULAWindow = new BWindow(eulaFrame, "README",
		B_MODAL_WINDOW, B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE
			| B_AUTO_UPDATE_SIZE_LIMITS);

	fEULAWindow->SetLayout(new BGroupLayout(B_HORIZONTAL));
	fEULAWindow->AddChild(BGroupLayoutBuilder(B_VERTICAL, 10)
		.Add(scrollView)
		.Add(BGroupLayoutBuilder(B_HORIZONTAL, 10)
			.AddGlue()
			.Add(cancelButton)
			.Add(continueButton)
		)
		.SetInsets(10, 10, 10, 10)
	);

	BRect frame = BScreen().Frame();
	fEULAWindow->MoveTo(frame.left + (frame.Width() - eulaFrame.Width()) / 2,
		frame.top + (frame.Height() - eulaFrame.Height()) / 2);

	fEULAWindow->Show();
#else
	// Show the installer window without EULA.
	new InstallerWindow();
#endif
}

