/*
 * Copyright 2005, Jérôme DUVAL. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <Alert.h>
#include <Application.h>
#include <Box.h>
#include <Directory.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Roster.h>
#include <string.h>
#include "InstallerWindow.h"
#include "PartitionMenuItem.h"

#define DRIVESETUP_SIG "application/x-vnd.Be-DRV$"

const uint32 BEGIN_MESSAGE = 'iBGN';
const uint32 SHOW_BOTTOM_MESSAGE = 'iSBT';
const uint32 SETUP_MESSAGE = 'iSEP';
const uint32 START_SCAN = 'iSSC';
const uint32 SRC_PARTITION = 'iSPT';
const uint32 DST_PARTITION = 'iDPT';

InstallerWindow::InstallerWindow(BRect frame_rect)
	: BWindow(frame_rect, "Installer", B_TITLED_WINDOW, B_NOT_ZOOMABLE | B_NOT_RESIZABLE),
	fDriveSetupLaunched(false)
{

	BRect bounds = Bounds();
	bounds.bottom += 1;
	bounds.right += 1;
	fBackBox = new BBox(bounds, NULL, B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS, B_FANCY_BORDER);
	AddChild(fBackBox);

	BRect statusRect(bounds.left+120, bounds.top+14, bounds.right-14, bounds.top+60);
	BRect textRect(statusRect);
	textRect.OffsetTo(B_ORIGIN);
	textRect.InsetBy(2,2);
	fStatusView = new BTextView(statusRect, "statusView", textRect,
		be_plain_font, NULL, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);
	fStatusView->MakeEditable(false);
	fStatusView->MakeSelectable(false);
	
	BScrollView *scroll = new BScrollView("statusScroll", fStatusView, B_FOLLOW_LEFT|B_FOLLOW_TOP, B_WILL_DRAW|B_FRAME_EVENTS);
        fBackBox->AddChild(scroll);

	fBeginButton = new BButton(BRect(bounds.right-90, bounds.bottom-35, bounds.right-10, bounds.bottom-11), 
		"begin_button", "Begin", new BMessage(BEGIN_MESSAGE), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	fBeginButton->MakeDefault(true);
	fBackBox->AddChild(fBeginButton);

	fSetupButton = new BButton(BRect(bounds.left+11, bounds.bottom-35, bounds.left+111, bounds.bottom-22),
		"setup_button", "Setup partitions" B_UTF8_ELLIPSIS, new BMessage(SETUP_MESSAGE), B_FOLLOW_LEFT|B_FOLLOW_BOTTOM);
	fBackBox->AddChild(fSetupButton);
	fSetupButton->Hide();

	fPackagesView = new PackagesView(BRect(bounds.left+12, bounds.top+4, bounds.right-15-B_V_SCROLL_BAR_WIDTH, bounds.bottom-61), "packages_view");
	fPackagesScrollView = new BScrollView("packagesScroll", fPackagesView, B_FOLLOW_LEFT | B_FOLLOW_BOTTOM, B_WILL_DRAW,
		false, true);
	fBackBox->AddChild(fPackagesScrollView);
	fPackagesScrollView->Hide();

	fDrawButton = new DrawButton(BRect(bounds.left+12, bounds.bottom-33, bounds.left+100, bounds.bottom-20),
		"options_button", "Fewer options", "More options", new BMessage(SHOW_BOTTOM_MESSAGE));
	fBackBox->AddChild(fDrawButton);

	fDestMenu = new BPopUpMenu("scanning" B_UTF8_ELLIPSIS);
	fSrcMenu = new BPopUpMenu("scanning" B_UTF8_ELLIPSIS);

	BRect fieldRect(bounds.left+50, bounds.top+70, bounds.right-13, bounds.top+90);
	fSrcMenuField = new BMenuField(fieldRect, "srcMenuField",
                "Install from: ", fSrcMenu);
        fSrcMenuField->SetDivider(70.0);
        fSrcMenuField->SetAlignment(B_ALIGN_RIGHT);
        fBackBox->AddChild(fSrcMenuField);

	fieldRect.OffsetBy(0,23);
	fDestMenuField = new BMenuField(fieldRect, "destMenuField",
		"Onto: ", fDestMenu);
	fDestMenuField->SetDivider(70.0);
	fDestMenuField->SetAlignment(B_ALIGN_RIGHT);
	fBackBox->AddChild(fDestMenuField);
	
	// finish creating window
	Show();

	fDriveSetupLaunched = be_roster->IsRunning(DRIVESETUP_SIG);
	be_roster->StartWatching(this);
	
	PostMessage(START_SCAN);
}

InstallerWindow::~InstallerWindow()
{
	be_roster->StopWatching(this);
}


void
InstallerWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case START_SCAN:
			StartScan();
			break;
		case BEGIN_MESSAGE:
			break;
		case SHOW_BOTTOM_MESSAGE:
			ShowBottom();
			break;
		case SRC_PARTITION:
			PublishPackages();
			break;
		case SETUP_MESSAGE:
			LaunchDriveSetup();
			break;
		case B_SOME_APP_LAUNCHED:
		case B_SOME_APP_QUIT:
		{
			const char *signature;
			if (msg->FindString("be:signature", &signature)==B_OK
				&& strcasecmp(signature, DRIVESETUP_SIG)==0) {
				DisableInterface(msg->what == B_SOME_APP_LAUNCHED);
                        }
                        break;
                }
		default:
			BWindow::MessageReceived(msg);
			break;
	}
}

bool
InstallerWindow::QuitRequested()
{
	if (fDriveSetupLaunched) {
		(new BAlert("driveSetup", 
			"Please close the DriveSetup window before closing the\nInstaller window.", "OK"))->Go();
		return false;
	}
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


void 
InstallerWindow::ShowBottom()
{
	if (fDrawButton->Value()) {
		ResizeTo(332,306);
		if (fSetupButton->IsHidden())
			fSetupButton->Show();
		if (fPackagesScrollView->IsHidden())
			fPackagesScrollView->Show();
	} else {
		if (!fSetupButton->IsHidden())
			fSetupButton->Hide();
		if (!fPackagesScrollView->IsHidden())
			fPackagesScrollView->Hide();
		ResizeTo(332,160);
	}
}


void
InstallerWindow::LaunchDriveSetup()
{
	if (be_roster->Launch(DRIVESETUP_SIG)!=B_OK)
		fprintf(stderr, "There was an error while launching DriveSetup\n");
}


void
InstallerWindow::DisableInterface(bool disable)
{
	if (!disable) {
		StartScan();
	}
	fDriveSetupLaunched = disable;
	fBeginButton->SetEnabled(!disable);
	fSetupButton->SetEnabled(!disable);
	fSrcMenuField->SetEnabled(!disable);
	fDestMenuField->SetEnabled(!disable);
	if (disable)
		fStatusView->SetText("Running DriveSetup" B_UTF8_ELLIPSIS "\nClose DriveSetup to continue with the\ninstallation.");
}


void
InstallerWindow::StartScan()
{
	fStatusView->SetText("Scanning for disks" B_UTF8_ELLIPSIS);

	BMenuItem *item;
	while ((item = fSrcMenu->RemoveItem((int32)0)))
		delete item;
	while ((item = fDestMenu->RemoveItem((int32)0)))
		delete item;

	fSrcMenu->AddItem(new PartitionMenuItem("BeOS 5 PE Max Edition V3.1 beta", 
		new BMessage(SRC_PARTITION), "/BeOS 5 PE Max Edition V3.1 beta"));

	if (fSrcMenu->ItemAt(0))
		fSrcMenu->ItemAt(0)->SetMarked(true);
	fStatusView->SetText("Choose the disk you want to install onto\nfrom the pop-up menu. Then click \"Begin\".");
}


void
InstallerWindow::PublishPackages()
{
	fPackagesView->Clean();
	PartitionMenuItem *item = (PartitionMenuItem *)fSrcMenu->FindMarked();
	if (!item)
		return;

	BPath directory(item->Path());
	directory.Append("_packages_");
	BDirectory dir(directory.Path());
	if (dir.InitCheck()!=B_OK)
		return;

	BEntry packageEntry;
	while (dir.GetNextEntry(&packageEntry)==B_OK) {
	}
}
