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
		"This is beta-quality software! It means there is a risk of "
		"losing important data. Make frequent backups! You have been "
		"warned.\n\n\n");
	infoText << B_TRANSLATE(
		"1)   If you are installing Haiku onto real hardware (not inside an "
		"emulator), you may want to prepare a hard disk partition from "
		"another OS (you could, for example, use a GParted Live-CD, which "
		"can also resize existing partitions to make room).\n"
		"You can also set up partitions by launching DriveSetup from "
		"Installer, but you won't be able to resize existing partitions with "
		"it. While DriveSetup has been quite thoroughly tested over the "
		"years, it's recommended to have up-to-date backups of the other "
		"partitions on your system. Just in case" B_UTF8_ELLIPSIS);
	infoText << "\n\n\n";
	infoText << B_TRANSLATE(
		"2)   The Installer will make the Haiku partition itself bootable, "
		"but takes no steps to integrate Haiku into an existing boot menu. "
		"If you have GRUB already installed, you can add Haiku to it.\n"
		"For details, please consult the guide on booting Haiku on our "
		"website at https://www.haiku-os.org/guides/booting.\n"
		"Or you can set up a boot menu from Installer's \"Tools\" menu, see "
		"the Haiku User Guide's topic on the application \"BootManager\"."
		"\n\n\n");
	infoText << B_TRANSLATE(
		"3)   When you successfully boot into Haiku for the first time, make "
		"sure to read our \"User Guide\" and take the \"Quick Tour\". There "
		"are links on the Desktop and in WebPositive's bookmarks.\n\n");
	infoText << B_TRANSLATE(
		"Have fun and thanks for trying out Haiku!");

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
