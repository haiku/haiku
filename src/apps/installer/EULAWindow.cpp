/*
 * Copyright 2013,	Jérôme DUVAL.
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "EULAWindow.h"

#include <Application.h>
#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <LayoutUtils.h>
#include <Roster.h>
#include <ScrollView.h>
#include <SpaceLayoutItem.h>

#include "tracker_private.h"

static const uint32 kMsgAgree = 'agre';
static const uint32 kMsgNext = 'next';

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "InstallerApp"


EULAWindow::EULAWindow()
	:
	BWindow(BRect(0, 0, 600, 450), B_TRANSLATE("README"),
		B_MODAL_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, B_NOT_ZOOMABLE
		| B_NOT_MINIMIZABLE | B_AUTO_UPDATE_SIZE_LIMITS)
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
		"2.1) GRUB (since os-prober v1.44)\n");
	infoText << B_TRANSLATE(
		"Starting with os-prober v1.44 (e.g. in Ubuntu 11.04 or later), Haiku "
		"should be recognized out of the box. To add Haiku to the GRUB menu, "
		"open a Terminal and enter:\n\n");
	infoText << B_TRANSLATE(
		"\tsudo update-grub\n\n\n");
	infoText << B_TRANSLATE(
		"2.2) GRUB 2\n");
	infoText << B_TRANSLATE(
		"If the os-prober approach doesn't work for you, GRUB 2 uses an extra "
		"configuration file to add custom entries to the boot menu. To add "
		"them to the top, you have to create/edit a file by launching your "
		"favorite editor from a Terminal like this:\n\n");
	infoText << B_TRANSLATE(
		"\tsudo <your favorite text editor> /etc/grub.d/40_custom\n\n");
	infoText << B_TRANSLATE(
		"GRUB's naming scheme for partitions is: (hdN,n)\n\n");
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

	BTextView* textView = new BTextView("eula", be_plain_font, NULL, B_WILL_DRAW);
	textView->SetInsets(10, 10, 10, 10);
	textView->MakeEditable(false);
	textView->MakeSelectable(false);
	textView->SetText(infoText);

	BScrollView* scrollView = new BScrollView("eulaScroll",
		textView, B_WILL_DRAW, false, true);

	BButton* cancelButton = new BButton(B_TRANSLATE("Quit"),
		new BMessage(B_QUIT_REQUESTED));
	cancelButton->SetTarget(be_app);

	BButton* continueButton = new BButton(B_TRANSLATE("Continue"),
		new BMessage(kMsgAgree));
	continueButton->SetTarget(be_app);
	continueButton->MakeDefault(true);

	if (!be_roster->IsRunning(kTrackerSignature))
		SetWorkspaces(B_ALL_WORKSPACES);

	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(B_USE_WINDOW_SPACING)
		.Add(scrollView)
		.AddGroup(B_HORIZONTAL, B_USE_ITEM_SPACING)
			.AddGlue()
			.Add(cancelButton)
			.Add(continueButton);

	CenterOnScreen();
	Show();
}


bool
EULAWindow::QuitRequested()
{
	be_app->PostMessage(kMsgNext);
	return true;
}
