/*
 * Copyright 2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2005, Jérôme DUVAL.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "InstallerApp.h"

#include <Alert.h>
#include <Button.h>
#include <LayoutBuilder.h>
#include <Locale.h>
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
	BString infoText;
	infoText << B_TRANSLATE(
		"Welcome to the Haiku Installer!\n\n");
	infoText << B_TRANSLATE(
		"IMPORTANT INFORMATION BEFORE INSTALLING HAIKU\n\n");
	infoText << B_TRANSLATE(
		"This is alpha-quality software! It means there is a high risk of "
		"losing important data. Make frequent backups! You have been "
		"warned.\n\n\n");
	infoText << B_TRANSLATE(
		"1)   If you are installing Haiku onto real hardware (not inside an "
		"emulator) it is recommended that you have already prepared a hard "
		"disk partition. The Installer and the DriveSetup tool offer to "
		"initialize existing partitions with the Haiku native file system, "
		"but the options to change the actual partition layout may not have "
		"been tested on a sufficiently great variety of computer "
		"configurations so we do not recommend using it.\n");
	infoText << B_TRANSLATE(
		"If you have not created a partition yet, simply reboot, create the "
		"partition using whatever tool you feel most comfortable with, and "
		"reboot into Haiku to continue with the installation. You could for "
		"example use the GParted Live-CD, it can also resize existing "
		"partitions to make room.\n\n\n");
	infoText << B_TRANSLATE(
		"2)   The Installer will make the Haiku partition itself bootable, "
		"but takes no steps to integrate Haiku into an existing boot menu. "
		"If you have GRUB already installed, you can add Haiku to its boot "
		"menu. Depending on what version of GRUB you use, this is done "
		"differently.\n\n\n");
	infoText << B_TRANSLATE(
		"2.1) GRUB 1\n");
	infoText << B_TRANSLATE(
		"Starting with os-prober v1.44 (e.g. in Ubuntu 11.04 or later), Haiku "
		"should be recognized out of the box. To add Haiku to the GRUB menu, "
		"open a Terminal and enter:\n\n");
	infoText << B_TRANSLATE(
		"\tsudo update-grub\n\n\n");
	infoText << B_TRANSLATE(
		"2.2) GRUB 1\n");
	infoText << B_TRANSLATE(
		"Configure your /boot/grub/menu.lst by launching your favorite "
		"editor from a Terminal like this:\n\n");
	infoText << B_TRANSLATE(
		"\tsudo <your favorite text editor> /boot/grub/menu.lst\n\n");
	infoText << B_TRANSLATE(
		"You'll note that GRUB uses a different naming strategy for hard "
		"drives than Linux.\n\n");
	infoText << B_TRANSLATE(
		"With GRUB it's: (hdN,n)\n\n");
	infoText << B_TRANSLATE(
		"All hard disks start with \"hd\".\n");
	infoText << B_TRANSLATE(
		"\"N\" is the hard disk number, starting with \"0\".\n");
	infoText << B_TRANSLATE(
		"\"n\" is the partition number, also starting with \"0\".\n");
	infoText << B_TRANSLATE(
		"The first logical partition always has the number \"4\", regardless "
		"of the number of primary partitions.\n\n");
	infoText << B_TRANSLATE(
		"So behind the other menu entries towards the bottom of the file, add "
		"something similar to these lines:\n\n");
	infoText << B_TRANSLATE(
		"\t# Haiku on /dev/sda7\n");
	infoText << B_TRANSLATE(
		"\ttitle\t\t\t\tHaiku\n");
	infoText << B_TRANSLATE(
		"\trootnoverify\t\t(hd0,6)\n");
	infoText << B_TRANSLATE(
		"\tchainloader\t\t+1\n\n");
	infoText << B_TRANSLATE(
		"You can see the correct partition in GParted for example.\n\n\n");
	infoText << B_TRANSLATE(
		"2.3) GRUB 2\n");
	infoText << B_TRANSLATE(
		"Newer versions of GRUB use an extra configuration file to add "
		"custom entries to the boot menu. To add them to the top, you have "
		"to create/edit a file by launching your favorite editor from a "
		"Terminal like this:\n\n");
	infoText << B_TRANSLATE(
		"\tsudo <your favorite text editor> /etc/grub.d/40_custom\n\n");
	infoText << B_TRANSLATE(
		"NOTE: While the naming strategy for hard disks is still as described "
		"under 2.1) the naming scheme for partitions has changed.\n\n");
	infoText << B_TRANSLATE(		
		"GRUB's naming scheme is still: (hdN,n)\n\n");
	infoText << B_TRANSLATE(
		"All hard disks start with \"hd\".\n");
	infoText << B_TRANSLATE(
		"\"N\" is the hard disk number, starting with \"0\".\n");
	infoText << B_TRANSLATE(
		"\"n\" is the partition number, which for GRUB 2 starts with \"1\"\n");
	infoText << B_TRANSLATE(
		"With GRUB 2 the first logical partition always has the number \"5\", "
		"regardless of the number of primary partitions.\n\n");
	infoText << B_TRANSLATE(
		"So below the heading that must not be edited, add something similar "
		"to these lines:\n\n");
	infoText << B_TRANSLATE(
		"\t# Haiku on /dev/sda7\n");
	infoText << B_TRANSLATE(
		"\tmenuentry \"Haiku Alpha\" {\n");
	infoText << B_TRANSLATE(
		"\t\tset root=(hd0,7)\n");
	infoText << B_TRANSLATE(
		"\t\tchainloader +1\n");
	infoText << B_TRANSLATE(
		"\t}\n\n");
	infoText << B_TRANSLATE(
		"Additionally you have to edit another file to actually display the "
		"boot menu:\n\n");
	infoText << B_TRANSLATE(
		"\tsudo <your favorite text editor> /etc/default/grub\n\n");
	infoText << B_TRANSLATE(
		"Here you have to comment out the line \"GRUB_HIDDEN_TIMEOUT=0\" by "
		"putting a \"#\" in front of it in order to actually display the "
		"boot menu.\n\n");
	infoText << B_TRANSLATE(
		"Finally, you have to update the boot menu by entering:\n\n");
	infoText << B_TRANSLATE(
		"\tsudo update-grub\n\n\n");
	infoText << B_TRANSLATE(
		"3)   When you successfully boot into Haiku for the first time, make "
		"sure to read our \"Welcome\" and \"Userguide\" documentation. There "
		"are links on the Desktop and in WebPositive's bookmarks.\n\n");
	infoText << B_TRANSLATE(
		"Have fun and thanks a lot for trying out Haiku! We hope you like it!");

#if 1
	// Show the EULA first.
	BTextView* textView = new BTextView("eula", be_plain_font, NULL,
		B_WILL_DRAW);
	textView->SetInsets(10, 10, 10, 10);
	textView->MakeEditable(false);
	textView->MakeSelectable(false);
	textView->SetText(infoText);

	BScrollView* scrollView = new BScrollView("eulaScroll",
		textView, B_WILL_DRAW, false, true);

	BButton* cancelButton = new BButton(B_TRANSLATE("Quit"),
		new BMessage(B_QUIT_REQUESTED));
	cancelButton->SetTarget(this);

	BButton* continueButton = new BButton(B_TRANSLATE("Continue"),
		new BMessage(kMsgAgree));
	continueButton->SetTarget(this);
	continueButton->MakeDefault(true);

	BRect eulaFrame = BRect(0, 0, 600, 450);
	fEULAWindow = new BWindow(eulaFrame, B_TRANSLATE("README"),
		B_MODAL_WINDOW, B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE
			| B_AUTO_UPDATE_SIZE_LIMITS);

	BLayoutBuilder::Group<>(fEULAWindow, B_VERTICAL, 10)
		.SetInsets(10)
		.Add(scrollView)
		.AddGroup(B_HORIZONTAL, 10)
			.AddGlue()
			.Add(cancelButton)
			.Add(continueButton);

	fEULAWindow->CenterOnScreen();
	fEULAWindow->Show();
#else
	// Show the installer window without EULA.
	new InstallerWindow();
#endif
}

