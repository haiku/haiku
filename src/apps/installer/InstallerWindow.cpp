/*
 * Copyright 2005, Jérôme DUVAL. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <Alert.h>
#include <Application.h>
#include <Autolock.h>
#include <Box.h>
#include <ClassInfo.h>
#include <Directory.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Roster.h>
#include <string.h>
#include <String.h>
#include <TranslationUtils.h>
#include "InstallerWindow.h"
#include "PartitionMenuItem.h"

#define DRIVESETUP_SIG "application/x-vnd.Be-DRV$"

const uint32 BEGIN_MESSAGE = 'iBGN';
const uint32 SHOW_BOTTOM_MESSAGE = 'iSBT';
const uint32 SETUP_MESSAGE = 'iSEP';
const uint32 START_SCAN = 'iSSC';
const uint32 PACKAGE_CHECKBOX = 'iPCB';

class LogoView : public BBox {
	public:
			LogoView(const BRect &r);
			~LogoView(void);
		virtual void Draw(BRect update);
	private:
		BBitmap			*fLogo;
		BPoint			fDrawPoint;
};


LogoView::LogoView(const BRect &r)
	: BBox(r, "logoview", B_FOLLOW_LEFT|B_FOLLOW_TOP, B_WILL_DRAW, B_NO_BORDER)
{
	fLogo = BTranslationUtils::GetBitmap('PNG ', "haikulogo.png");
	if (fLogo) {
		fDrawPoint.x = (r.Width() - fLogo->Bounds().Width()) / 2;
		fDrawPoint.y = 0;
	}
}


LogoView::~LogoView(void)
{
	delete fLogo;
}


void
LogoView::Draw(BRect update)
{
	if (fLogo)
		DrawBitmap(fLogo, fDrawPoint);
}


InstallerWindow::InstallerWindow(BRect frame_rect)
	: BWindow(frame_rect, "Installer", B_TITLED_WINDOW, B_NOT_ZOOMABLE | B_NOT_RESIZABLE),
	fDriveSetupLaunched(false),
	fLastSrcItem(NULL),
	fLastTargetItem(NULL)
{
	fCopyEngine = new CopyEngine(this);
	
	BRect bounds = Bounds();
	bounds.bottom += 1;
	bounds.right += 1;
	fBackBox = new BBox(bounds, NULL, B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS, B_FANCY_BORDER);
	AddChild(fBackBox);
	
	BRect logoRect = fBackBox->Bounds();
	logoRect.left += 1;
	logoRect.top = 12;
	logoRect.right -= 226;
	logoRect.bottom = logoRect.top + 46 + B_H_SCROLL_BAR_HEIGHT;
	LogoView *logoView = new LogoView(logoRect);
	fBackBox->AddChild(logoView);

	BRect statusRect(bounds.right-222, logoRect.top+2, bounds.right-14, logoRect.bottom - B_H_SCROLL_BAR_HEIGHT+4);
	BRect textRect(statusRect);
	textRect.OffsetTo(B_ORIGIN);
	textRect.InsetBy(2,2);
	fStatusView = new BTextView(statusRect, "statusView", textRect,
		be_plain_font, NULL, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);
	fStatusView->MakeEditable(false);
	fStatusView->MakeSelectable(false);
	
	BScrollView *scroll = new BScrollView("statusScroll", fStatusView, B_FOLLOW_LEFT|B_FOLLOW_TOP, B_WILL_DRAW|B_FRAME_EVENTS);
        fBackBox->AddChild(scroll);

	fBeginButton = new BButton(BRect(bounds.right-90, bounds.bottom-35, bounds.right-11, bounds.bottom-11), 
		"begin_button", "Begin", new BMessage(BEGIN_MESSAGE), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	fBeginButton->MakeDefault(true);
	fBackBox->AddChild(fBeginButton);

	fSetupButton = new BButton(BRect(bounds.left+11, bounds.bottom-35, 
		bounds.left + be_plain_font->StringWidth("Setup partitions") + 36, bounds.bottom-22),
		"setup_button", "Setup partitions" B_UTF8_ELLIPSIS, new BMessage(SETUP_MESSAGE), B_FOLLOW_LEFT|B_FOLLOW_BOTTOM);
	fBackBox->AddChild(fSetupButton);
	fSetupButton->Hide();

	fPackagesView = new PackagesView(BRect(bounds.left+12, bounds.top+4, bounds.right-15-B_V_SCROLL_BAR_WIDTH, bounds.bottom-61), "packages_view");
	fPackagesScrollView = new BScrollView("packagesScroll", fPackagesView, B_FOLLOW_LEFT | B_FOLLOW_BOTTOM, B_WILL_DRAW,
		false, true);
	fBackBox->AddChild(fPackagesScrollView);
	fPackagesScrollView->Hide();

	fDrawButton = new DrawButton(BRect(bounds.left+12, bounds.bottom-33, bounds.left+120, bounds.bottom-20),
		"options_button", "Fewer options", "More options", new BMessage(SHOW_BOTTOM_MESSAGE));
	fBackBox->AddChild(fDrawButton);

	fDestMenu = new BPopUpMenu("scanning" B_UTF8_ELLIPSIS, true, false);
	fSrcMenu = new BPopUpMenu("scanning" B_UTF8_ELLIPSIS, true, false);

	BRect fieldRect(bounds.left+50, bounds.top+70, bounds.right-13, bounds.top+90);
	fSrcMenuField = new BMenuField(fieldRect, "srcMenuField",
                "Install from: ", fSrcMenu);
	fSrcMenuField->SetDivider(bounds.right-274);
	fSrcMenuField->SetAlignment(B_ALIGN_RIGHT);
	fBackBox->AddChild(fSrcMenuField);

	fieldRect.OffsetBy(0,23);
	fDestMenuField = new BMenuField(fieldRect, "destMenuField",
		"Onto: ", fDestMenu);
	fDestMenuField->SetDivider(bounds.right-274);
	fDestMenuField->SetAlignment(B_ALIGN_RIGHT);
	fBackBox->AddChild(fDestMenuField);

	BRect sizeRect = fBackBox->Bounds();
	sizeRect.top = 105;
	sizeRect.bottom = sizeRect.top + 15;
	sizeRect.right -= 12;
	sizeRect.left = sizeRect.right - be_plain_font->StringWidth("Disk space required: 0.0 KB") - 40;
	fSizeView = new BStringView(sizeRect, "size_view", "Disk space required: 0.0 KB", B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	fSizeView->SetAlignment(B_ALIGN_RIGHT);
	fBackBox->AddChild(fSizeView);
	fSizeView->Hide();

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
		{
			BList *list = new BList();
			int32 size = 0;
			fPackagesView->GetPackagesToInstall(list, &size);
			fCopyEngine->SetPackagesList(list);
			fCopyEngine->SetSpaceRequired(size);
			BMessenger(fCopyEngine).SendMessage(ENGINE_START);
			DisableInterface(true);
			break;
		}
		case SHOW_BOTTOM_MESSAGE:
			ShowBottom();
			break;
		case SRC_PARTITION:
			if (fLastSrcItem == fSrcMenu->FindMarked())
				break;
			fLastSrcItem = fSrcMenu->FindMarked();
			PublishPackages();
			AdjustMenus();
			break;
		case TARGET_PARTITION:
			if (fLastTargetItem == fDestMenu->FindMarked())
				break;
			fLastTargetItem = fDestMenu->FindMarked();
			AdjustMenus();
			break;
		case SETUP_MESSAGE:
			LaunchDriveSetup();
			break;
		case PACKAGE_CHECKBOX: {
			char buffer[15];
			fPackagesView->GetTotalSizeAsString(buffer);
			char string[255];
			sprintf(string, "Disk space required: %s", buffer);
			fSizeView->SetText(string);
			break;
		}
		case STATUS_MESSAGE: {
			const char *status;
			if (msg->FindString("status", &status) == B_OK) {
				fLastStatus = fStatusView->Text();
				SetStatusMessage(status);
			} else
				SetStatusMessage(fLastStatus.String());
		}
		case INSTALL_FINISHED:
			DisableInterface(false);
			break;
		case B_SOME_APP_LAUNCHED:
		case B_SOME_APP_QUIT:
		{
			const char *signature;
			if (msg->FindString("be:signature", &signature)==B_OK
				&& strcasecmp(signature, DRIVESETUP_SIG)==0) {
				fDriveSetupLaunched = msg->what == B_SOME_APP_LAUNCHED;
				DisableInterface(fDriveSetupLaunched);
				if (fDriveSetupLaunched)
					SetStatusMessage("Running DriveSetup" B_UTF8_ELLIPSIS "\nClose DriveSetup to continue with the\ninstallation.");
				else
					StartScan();
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
	fCopyEngine->PostMessage(B_QUIT_REQUESTED);
	return true;
}


void 
InstallerWindow::ShowBottom()
{
	if (fDrawButton->Value()) {
		ResizeTo(INSTALLER_RIGHT, 306);
		if (fSetupButton->IsHidden())
			fSetupButton->Show();
		if (fPackagesScrollView->IsHidden())
			fPackagesScrollView->Show();
		if (fSizeView->IsHidden())
			fSizeView->Show();
	} else {
		if (!fSetupButton->IsHidden())
			fSetupButton->Hide();
		if (!fPackagesScrollView->IsHidden())
			fPackagesScrollView->Hide();
		if (!fSizeView->IsHidden())
			fSizeView->Hide();
		ResizeTo(INSTALLER_RIGHT, 160);
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
	fBeginButton->SetEnabled(!disable);
	fSetupButton->SetEnabled(!disable);
	fSrcMenuField->SetEnabled(!disable);
	fDestMenuField->SetEnabled(!disable);
}


void
InstallerWindow::StartScan()
{
	SetStatusMessage("Scanning for disks" B_UTF8_ELLIPSIS);

	BMenuItem *item;
	while ((item = fSrcMenu->RemoveItem((int32)0)))
		delete item;
	while ((item = fDestMenu->RemoveItem((int32)0)))
		delete item;

	fCopyEngine->ScanDisksPartitions(fSrcMenu, fDestMenu);

	if (fSrcMenu->ItemAt(0)) {
		PublishPackages();
	}
	AdjustMenus();
	SetStatusMessage("Choose the disk you want to install onto from the pop-up menu. Then click \"Begin\".");
}


void
InstallerWindow::AdjustMenus()
{
	PartitionMenuItem *item1 = (PartitionMenuItem *)fSrcMenu->FindMarked();
	if (item1) {
		fSrcMenuField->MenuItem()->SetLabel(item1->MenuLabel());
	} else {
		if (fSrcMenu->CountItems() == 0)
			fSrcMenuField->MenuItem()->SetLabel("<none>");
		else
			fSrcMenuField->MenuItem()->SetLabel(((PartitionMenuItem *)fSrcMenu->ItemAt(0))->MenuLabel());
	}
	
	PartitionMenuItem *item2 = (PartitionMenuItem *)fDestMenu->FindMarked();
	if (item2) {
		fDestMenuField->MenuItem()->SetLabel(item2->MenuLabel());
	} else {
		if (fDestMenu->CountItems() == 0)
			fDestMenuField->MenuItem()->SetLabel("<none>");
		else
			fDestMenuField->MenuItem()->SetLabel(((PartitionMenuItem *)fDestMenu->ItemAt(0))->MenuLabel());
	}
	char message[255];
	sprintf(message, "Press the Begin button to install from '%s' onto '%s'", 
		item1 ? item1->Name() : "null", item2 ? item2->Name() : "null");
	SetStatusMessage(message);
}


void
InstallerWindow::PublishPackages()
{
	fPackagesView->Clean();
	PartitionMenuItem *item = (PartitionMenuItem *)fSrcMenu->FindMarked();
	if (!item)
		return;

#ifdef __HAIKU__
	BPath directory;
	BDiskDeviceRoster roster;
	BDiskDevice device;
	BPartition *partition;
	if (roster.GetPartitionWithID(item->ID(), &device, &partition) == B_OK) {
		if (partition->GetMountPoint(&directory)!=B_OK)
			return;
	} else if (roster.GetDeviceWithID(item->ID(), &device) == B_OK) {
		if (device.GetMountPoint(&directory)!=B_OK)
			return;
	} else 
		return; // shouldn't happen
#else
	BPath directory = "/BeOS 5 PE Max Edition V3.1 beta";
#endif
	
	directory.Append(PACKAGES_DIRECTORY);
	BDirectory dir(directory.Path());
	if (dir.InitCheck()!=B_OK)
		return;

	BEntry packageEntry;
	BList packages;
	while (dir.GetNextEntry(&packageEntry)==B_OK) {
		Package *package = Package::PackageFromEntry(packageEntry);
		if (package) {
			packages.AddItem(package);
		}
	}
	packages.SortItems(ComparePackages);

	fPackagesView->AddPackages(packages, new BMessage(PACKAGE_CHECKBOX));
	PostMessage(PACKAGE_CHECKBOX);	
}


int 
InstallerWindow::ComparePackages(const void *firstArg, const void *secondArg)
{
	const Group *group1 = *static_cast<const Group * const *>(firstArg);
	const Group *group2 = *static_cast<const Group * const *>(secondArg);
	const Package *package1 = dynamic_cast<const Package *>(group1);
	const Package *package2 = dynamic_cast<const Package *>(group2);
	int sameGroup = strcmp(group1->GroupName(), group2->GroupName());
	if (sameGroup != 0)
		return sameGroup;
	if (!package2)
		return -1;
	if (!package1)
		return 1;
	return strcmp(package1->Name(), package2->Name());
}


void
InstallerWindow::SetStatusMessage(const char *text)
{
	BAutolock(this);
	fStatusView->SetText(text);
}

