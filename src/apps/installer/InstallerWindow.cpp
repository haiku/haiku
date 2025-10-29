/*
 * Copyright 2020-2023, Panagiotis "Ivory" Vasilopoulos <git@n0toose.net>
 * Copyright 2009-2010, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2005-2008, Jérôme DUVAL
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "InstallerWindow.h"

#include <stdio.h>
#include <strings.h>

#include <Alert.h>
#include <Application.h>
#include <Autolock.h>
#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <ColorConversion.h>
#include <ControlLook.h>
#include <Directory.h>
#include <FindDirectory.h>
#include <LayoutBuilder.h>
#include <LayoutUtils.h>
#include <Locale.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Roster.h>
#include <Screen.h>
#include <ScrollView.h>
#include <SeparatorView.h>
#include <SpaceLayoutItem.h>
#include <StatusBar.h>
#include <String.h>
#include <TextView.h>
#include <TranslationUtils.h>
#include <TranslatorFormats.h>

#include "tracker_private.h"

#include "DialogPane.h"
#include "InstallerDefs.h"
#include "PackageViews.h"
#include "PartitionMenuItem.h"
#include "WorkerThread.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "InstallerWindow"


static const char* kDriveSetupSignature = "application/x-vnd.Haiku-DriveSetup";
static const char* kBootManagerSignature = "application/x-vnd.Haiku-BootManager";

const uint32 BEGIN_MESSAGE = 'iBGN';
const uint32 SHOW_BOTTOM_MESSAGE = 'iSBT';
const uint32 LAUNCH_DRIVE_SETUP = 'iSEP';
const uint32 LAUNCH_BOOTMAN = 'iWBM';
const uint32 START_SCAN = 'iSSC';
const uint32 PACKAGE_CHECKBOX = 'iPCB';
const uint32 ENCOURAGE_DRIVESETUP = 'iENC';


class LogoView : public BView {
public:
								LogoView(const BRect& frame);
								LogoView();
	virtual						~LogoView();

	virtual	void				Draw(BRect update);

	virtual	void				GetPreferredSize(float* _width,
									float* _height);

private:
			void				_Init();

			BBitmap*			fLogo;
};


LogoView::LogoView(const BRect& frame)
	:
	BView(frame, "logoview", B_FOLLOW_LEFT | B_FOLLOW_TOP,
		B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE)
{
	_Init();
}


LogoView::LogoView()
	:
	BView("logoview", B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE)
{
	_Init();
}


LogoView::~LogoView(void)
{
	delete fLogo;
}


void
LogoView::Draw(BRect update)
{
	BRect bounds(Bounds());
	SetLowColor(ui_color(B_DOCUMENT_BACKGROUND_COLOR));
	FillRect(bounds, B_SOLID_LOW);

	if (fLogo == NULL)
		return;

	BPoint placement;
	placement.x = (bounds.left + bounds.right - fLogo->Bounds().Width()) / 2;
	placement.y = (bounds.top + bounds.bottom - fLogo->Bounds().Height()) / 2;

	DrawBitmap(fLogo, placement);
}


void
LogoView::GetPreferredSize(float* _width, float* _height)
{
	float width = 0.0;
	float height = 0.0;
	if (fLogo) {
		width = fLogo->Bounds().Width();
		height = fLogo->Bounds().Height();
	}
	if (_width)
		*_width = width;
	if (_height)
		*_height = height;
}


void
LogoView::_Init()
{
	SetDrawingMode(B_OP_OVER);

#ifdef HAIKU_DISTRO_COMPATIBILITY_OFFICIAL
	rgb_color bgColor = ui_color(B_DOCUMENT_BACKGROUND_COLOR);

	if (bgColor.IsLight())
		fLogo = BTranslationUtils::GetBitmap(B_PNG_FORMAT, "logo.png");
	else
		fLogo = BTranslationUtils::GetBitmap(B_PNG_FORMAT, "logo_dark.png");
#else
	fLogo = BTranslationUtils::GetBitmap(B_PNG_FORMAT, "walter_logo.png");
#endif
}


// #pragma mark -


static BLayoutItem*
layout_item_for(BView* view)
{
	BLayout* layout = view->Parent()->GetLayout();
	int32 index = layout->IndexOfView(view);
	return layout->ItemAt(index);
}


InstallerWindow::InstallerWindow()
	:
	BWindow(BRect(-2400, -2000, -1800, -1800),
		B_TRANSLATE_SYSTEM_NAME("Installer"), B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS),
	fEncouragedToSetupPartitions(false),
	fDriveSetupLaunched(false),
	fBootManagerLaunched(false),
	fInstallStatus(kReadyForInstall),
	fWorkerThread(new WorkerThread(this)),
	fCopyEngineCancelSemaphore(-1)
{
	if (!be_roster->IsRunning(kTrackerSignature))
		SetWorkspaces(B_ALL_WORKSPACES);

	LogoView* logoView = new LogoView();

	rgb_color baseColor = ui_color(B_DOCUMENT_TEXT_COLOR);
	fStatusView = new BTextView("statusView", be_plain_font, &baseColor,
		B_WILL_DRAW);
	fStatusView->SetViewUIColor(B_DOCUMENT_BACKGROUND_COLOR);
	fStatusView->MakeEditable(false);
	fStatusView->MakeSelectable(false);

	BSize logoSize = logoView->MinSize();
	logoView->SetExplicitMaxSize(logoSize);

	// In the status view, make sure that we can display 5 lines of text of ~28 characters each
	font_height height;
	fStatusView->GetFontHeight(&height);
	float fontHeight = height.ascent + height.descent + height.leading;
	fStatusView->SetExplicitMinSize(BSize(fStatusView->StringWidth("W") * 28,
		fontHeight * 5 + 8));

	// Create a group view with a white background since the logo and status text won't have the
	// same height, this background will show in the remaining space
	fLogoGroup = new BGroupView(B_HORIZONTAL, 10);
	fLogoGroup->SetViewUIColor(B_DOCUMENT_BACKGROUND_COLOR);
	fLogoGroup->GroupLayout()->SetInsets(0, 0, 10, 0);
	fLogoGroup->AddChild(logoView);
	fLogoGroup->AddChild(fStatusView);

	fDestMenu = new BPopUpMenu(B_TRANSLATE("scanning" B_UTF8_ELLIPSIS),
		true, false);
	fSrcMenu = new BPopUpMenu(B_TRANSLATE("scanning" B_UTF8_ELLIPSIS),
		true, false);

	fSrcMenuField = new BMenuField("srcMenuField",
		B_TRANSLATE("Install from:"), fSrcMenu);
	fSrcMenuField->SetAlignment(B_ALIGN_RIGHT);

	fDestMenuField = new BMenuField("destMenuField", B_TRANSLATE("Onto:"),
		fDestMenu);
	fDestMenuField->SetAlignment(B_ALIGN_RIGHT);

	fPackagesSwitch = new PaneSwitch("options_button");
	fPackagesSwitch->SetLabels(B_TRANSLATE("Hide optional packages"),
		B_TRANSLATE("Show optional packages"));
	fPackagesSwitch->SetMessage(new BMessage(SHOW_BOTTOM_MESSAGE));
	fPackagesSwitch->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED,
		B_SIZE_UNSET));
	fPackagesSwitch->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT,
		B_ALIGN_TOP));

	fPackagesView = new PackagesView("packages_view");
	BScrollView* packagesScrollView = new BScrollView("packagesScroll",
		fPackagesView, B_WILL_DRAW, false, true);

	const char* requiredDiskSpaceString
		= B_TRANSLATE("Additional disk space required: 0.0 KiB");
	fSizeView = new BStringView("size_view", requiredDiskSpaceString);
	fSizeView->SetAlignment(B_ALIGN_RIGHT);
	fSizeView->SetExplicitAlignment(
		BAlignment(B_ALIGN_RIGHT, B_ALIGN_TOP));

	fProgressBar = new BStatusBar("progress",
		B_TRANSLATE("Install progress:  "));
	fProgressBar->SetMaxValue(100.0);

	fBeginButton = new BButton("begin_button", B_TRANSLATE("Begin"),
		new BMessage(BEGIN_MESSAGE));
	fBeginButton->MakeDefault(true);
	fBeginButton->SetEnabled(false);

	fLaunchDriveSetupButton = new BButton("setup_button",
		B_TRANSLATE("Set up partitions" B_UTF8_ELLIPSIS),
		new BMessage(LAUNCH_DRIVE_SETUP));

	fLaunchBootManagerItem = new BMenuItem(B_TRANSLATE("Set up boot menu" B_UTF8_ELLIPSIS),
		new BMessage(LAUNCH_BOOTMAN));
	fLaunchBootManagerItem->SetEnabled(false);

	fMakeBootableItem = new BMenuItem(B_TRANSLATE("Write boot sector"),
		new BMessage(MSG_WRITE_BOOT_SECTOR));
	fMakeBootableItem->SetEnabled(false);
	BMenuBar* mainMenu = new BMenuBar("main menu");
	BMenu* toolsMenu = new BMenu(B_TRANSLATE("Tools"));
	toolsMenu->AddItem(fLaunchBootManagerItem);
	toolsMenu->AddItem(fMakeBootableItem);
	mainMenu->AddItem(toolsMenu);

	BGroupView* packagesGroup = new BGroupView(B_VERTICAL, B_USE_ITEM_SPACING);
	packagesGroup->AddChild(fPackagesSwitch);
	packagesGroup->AddChild(packagesScrollView);
	packagesGroup->AddChild(fProgressBar);
	packagesGroup->AddChild(fSizeView);

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.Add(mainMenu)
		.Add(fLogoGroup)
		.Add(new BSeparatorView(B_HORIZONTAL, B_PLAIN_BORDER))
		.AddGroup(B_VERTICAL, B_USE_ITEM_SPACING)
			.SetInsets(B_USE_WINDOW_SPACING)
			.AddGrid(new BGridView(B_USE_ITEM_SPACING, B_USE_ITEM_SPACING))
				.AddMenuField(fSrcMenuField, 0, 0)
				.AddMenuField(fDestMenuField, 0, 1)
				.AddGlue(2, 0, 1, 2)
				.Add(BSpaceLayoutItem::CreateVerticalStrut(5), 0, 2, 3)
			.End()
			.Add(packagesGroup)
			.AddGroup(B_HORIZONTAL, B_USE_WINDOW_SPACING)
				.Add(fLaunchDriveSetupButton)
				.AddGlue()
				.Add(fBeginButton)
			.End()
		.End()
	.End();

	// Make the optional packages and progress bar invisible on start
	fPackagesLayoutItem = layout_item_for(packagesScrollView);
	fPkgSwitchLayoutItem = layout_item_for(fPackagesSwitch);
	fSizeViewLayoutItem = layout_item_for(fSizeView);
	fProgressLayoutItem = layout_item_for(fProgressBar);

	fPackagesLayoutItem->SetVisible(false);
	fSizeViewLayoutItem->SetVisible(false);
	fProgressLayoutItem->SetVisible(false);

	// finish creating window
	if (!be_roster->IsRunning(kDeskbarSignature))
		SetFlags(Flags() | B_NOT_MINIMIZABLE);

	CenterOnScreen();
	Show();

	// Register to receive notifications when apps launch or quit...
	be_roster->StartWatching(this);
	// ... and check the two we are interested in.
	fDriveSetupLaunched = be_roster->IsRunning(kDriveSetupSignature);
	fBootManagerLaunched = be_roster->IsRunning(kBootManagerSignature);

	if (Lock()) {
		fLaunchDriveSetupButton->SetEnabled(!fDriveSetupLaunched);
		fLaunchBootManagerItem->SetEnabled(!fBootManagerLaunched);
		Unlock();
	}

	PostMessage(START_SCAN);
}


InstallerWindow::~InstallerWindow()
{
	_SetCopyEngineCancelSemaphore(-1);
	be_roster->StopWatching(this);
}


void
InstallerWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case MSG_RESET:
		{
			_SetCopyEngineCancelSemaphore(-1);

			status_t error;
			if (msg->FindInt32("error", &error) == B_OK) {
				char errorMessage[2048];
				snprintf(errorMessage, sizeof(errorMessage),
					B_TRANSLATE("An error was encountered and the "
					"installation was not completed:\n\n"
					"Error:  %s"), strerror(error));
				BAlert* alert = new BAlert("error", errorMessage, B_TRANSLATE("OK"));
				alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
				alert->Go();
			}

			_DisableInterface(false);

			fProgressLayoutItem->SetVisible(false);
			fPkgSwitchLayoutItem->SetVisible(true);
			_ShowOptionalPackages();
			_UpdateControls();
			break;
		}
		case START_SCAN:
			_ScanPartitions();
			break;
		case BEGIN_MESSAGE:
			switch (fInstallStatus) {
				case kReadyForInstall:
				{
					// get source and target
					PartitionMenuItem* targetItem
						= (PartitionMenuItem*)fDestMenu->FindMarked();
					PartitionMenuItem* srcItem
						= (PartitionMenuItem*)fSrcMenu->FindMarked();
					if (srcItem == NULL || targetItem == NULL)
						break;

					_SetCopyEngineCancelSemaphore(create_sem(1,
						"copy engine cancel"));

					BList* list = new BList();
					int32 size = 0;
					fPackagesView->GetPackagesToInstall(list, &size);
					fWorkerThread->SetLock(fCopyEngineCancelSemaphore);
					fWorkerThread->SetPackagesList(list);
					fWorkerThread->SetSpaceRequired(size);
					fInstallStatus = kInstalling;
					fWorkerThread->StartInstall(srcItem->ID(),
						targetItem->ID());
					fBeginButton->SetLabel(B_TRANSLATE("Stop"));
					_DisableInterface(true);

					fProgressBar->SetTo(0.0, NULL, NULL);

					fPkgSwitchLayoutItem->SetVisible(false);
					fPackagesLayoutItem->SetVisible(false);
					fSizeViewLayoutItem->SetVisible(false);
					fProgressLayoutItem->SetVisible(true);
					break;
				}
				case kInstalling:
				{
					_QuitCopyEngine(true);
					break;
				}
				case kFinished:
					PostMessage(B_QUIT_REQUESTED);
					break;
				case kCancelled:
					break;
			}
			break;
		case SHOW_BOTTOM_MESSAGE:
			_ShowOptionalPackages();
			break;
		case SOURCE_PARTITION:
			_PublishPackages();
			_UpdateControls();
			break;
		case TARGET_PARTITION:
			_UpdateControls();
			break;
		case LAUNCH_DRIVE_SETUP:
			_LaunchDriveSetup();
			break;
		case LAUNCH_BOOTMAN:
			_LaunchBootManager();
			break;
		case PACKAGE_CHECKBOX:
		{
			char buffer[15];
			fPackagesView->GetTotalSizeAsString(buffer, sizeof(buffer));
			char string[256];
			snprintf(string, sizeof(string),
				B_TRANSLATE("Additional disk space required: %s"), buffer);
			fSizeView->SetText(string);
			fSizeView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
			break;
		}
		case ENCOURAGE_DRIVESETUP:
		{
			BAlert* alert = new BAlert("use drive setup", B_TRANSLATE("No partitions have "
				"been found that are suitable for installation. Please set "
				"up partitions and format at least one partition with the "
				"Be File System."), B_TRANSLATE("OK"));
			alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
			alert->Go();
			break;
		}
		case MSG_STATUS_MESSAGE:
		{
			float progress;
			if (msg->FindFloat("progress", &progress) == B_OK) {
				const char* currentItem;
				if (msg->FindString("item", &currentItem) != B_OK) {
					currentItem = B_TRANSLATE_COMMENT("???",
						"Unknown currently copied item");
				}
				BString trailingLabel;
				int32 currentCount;
				int32 maximumCount;
				if (msg->FindInt32("current", &currentCount) == B_OK
					&& msg->FindInt32("maximum", &maximumCount) == B_OK) {
					char buffer[64];
					snprintf(buffer, sizeof(buffer),
						B_TRANSLATE_COMMENT("%1ld of %2ld", "number of files copied"),
						(long int)currentCount, (long int)maximumCount);
					trailingLabel << buffer;
				} else {
					trailingLabel <<
						B_TRANSLATE_COMMENT("?? of ??", "Unknown progress");
				}
				fProgressBar->SetTo(progress, currentItem,
					trailingLabel.String());
			} else {
				const char *status;
				if (msg->FindString("status", &status) == B_OK) {
					fLastStatus = fStatusView->Text();
					_SetStatusMessage(status);
				} else
					_SetStatusMessage(fLastStatus.String());
			}
			break;
		}
		case MSG_INSTALL_FINISHED:
		{

			_SetCopyEngineCancelSemaphore(-1);

			PartitionMenuItem* dstItem
				= (PartitionMenuItem*)fDestMenu->FindMarked();

			BString status;
			if (be_roster->IsRunning(kDeskbarSignature)) {
				fBeginButton->SetLabel(B_TRANSLATE("Quit"));
				status.SetToFormat(B_TRANSLATE("Installation "
					"completed. Boot sector has been written to '%s'. Press "
					"'Quit' to leave the Installer or choose a new target "
					"volume to perform another installation."),
					dstItem ? dstItem->Name() : B_TRANSLATE_COMMENT("???",
						"Unknown partition name"));
			} else {
				fBeginButton->SetLabel(B_TRANSLATE("Restart"));
				status.SetToFormat(B_TRANSLATE("Installation "
					"completed. Boot sector has been written to '%s'. Press "
					"'Restart' to restart the computer or choose a new target "
					"volume to perform another installation."),
					dstItem ? dstItem->Name() : B_TRANSLATE_COMMENT("???",
						"Unknown partition name"));
			}

			_SetStatusMessage(status.String());
			fInstallStatus = kFinished;

			_DisableInterface(false);
			fProgressLayoutItem->SetVisible(false);
			fPkgSwitchLayoutItem->SetVisible(true);
			_ShowOptionalPackages();
			break;
		}
		case B_SOME_APP_LAUNCHED:
		case B_SOME_APP_QUIT:
		{
			const char *signature;
			if (msg->FindString("be:signature", &signature) != B_OK)
				break;
			bool isDriveSetup = !strcasecmp(signature, kDriveSetupSignature);
			bool isBootManager = !strcasecmp(signature, kBootManagerSignature);
			if (isDriveSetup || isBootManager) {
				bool scanPartitions = false;
				if (isDriveSetup) {
					bool launched = msg->what == B_SOME_APP_LAUNCHED;
					// We need to scan partitions if DriveSetup has quit.
					scanPartitions = fDriveSetupLaunched && !launched;
					fDriveSetupLaunched = launched;
				}
				if (isBootManager)
					fBootManagerLaunched = msg->what == B_SOME_APP_LAUNCHED;

				fBeginButton->SetEnabled(
					!fDriveSetupLaunched && !fBootManagerLaunched);
				_DisableInterface(fDriveSetupLaunched || fBootManagerLaunched);
				if (fDriveSetupLaunched && fBootManagerLaunched) {
					_SetStatusMessage(B_TRANSLATE("Running BootManager and "
						"DriveSetup" B_UTF8_ELLIPSIS
						"\n\nClose both applications to continue with the "
						"installation."));
				} else if (fDriveSetupLaunched) {
					_SetStatusMessage(B_TRANSLATE("Running DriveSetup"
						B_UTF8_ELLIPSIS
						"\n\nClose DriveSetup to continue with the "
						"installation."));
				} else if (fBootManagerLaunched) {
					_SetStatusMessage(B_TRANSLATE("Running BootManager"
						B_UTF8_ELLIPSIS
						"\n\nClose BootManager to continue with the "
						"installation."));
				} else {
					// If neither DriveSetup nor Bootman is running, we need
					// to scan partitions in case DriveSetup has quit, or
					// we need to update the guidance message, unless install
					// was already finished.
					if (scanPartitions)
						_ScanPartitions();
					else if (fInstallStatus != kFinished)
						_UpdateControls();
					else
						PostMessage(MSG_INSTALL_FINISHED);
				}
			}
			break;
		}
		case MSG_WRITE_BOOT_SECTOR:
			fWorkerThread->WriteBootSector(fDestMenu);
			break;

		default:
			BWindow::MessageReceived(msg);
			break;
	}
}


bool
InstallerWindow::QuitRequested()
{
	if ((Flags() & B_NOT_MINIMIZABLE) != 0) {
		// This means Deskbar is not running, i.e. Installer is the only
		// thing on the screen and we will reboot the machine once it quits.

		if (fDriveSetupLaunched && fBootManagerLaunched) {
			BAlert* alert = new BAlert(B_TRANSLATE("Quit BootManager and "
				"DriveSetup"),	B_TRANSLATE("Please close the BootManager "
				"and DriveSetup windows before closing the Installer window."),
				B_TRANSLATE("OK"));
			alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
			alert->Go();
			return false;
		}
		if (fDriveSetupLaunched) {
			BAlert* alert = new BAlert(B_TRANSLATE("Quit DriveSetup"),
				B_TRANSLATE("Please close the DriveSetup window before "
				"closing the Installer window."), B_TRANSLATE("OK"));
			alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
			alert->Go();
			return false;
		}
		if (fBootManagerLaunched) {
			BAlert* alert = new BAlert(B_TRANSLATE("Quit BootManager"),
				B_TRANSLATE("Please close the BootManager window before "
				"closing the Installer window."), B_TRANSLATE("OK"));
			alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
			alert->Go();
			return false;
		}
		if (fInstallStatus != kFinished) {
			BAlert* alert = new BAlert(B_TRANSLATE_SYSTEM_NAME("Installer"),
				B_TRANSLATE("Are you sure you want to stop the installation?"),
				B_TRANSLATE("Cancel"), B_TRANSLATE("Stop"), NULL,
				B_WIDTH_AS_USUAL, B_STOP_ALERT);
			alert->SetShortcut(0, B_ESCAPE);
			if (alert->Go() == 0)
				return false;
		}
	} else if (fInstallStatus == kInstalling) {
			BAlert* alert = new BAlert(B_TRANSLATE_SYSTEM_NAME("Installer"),
				B_TRANSLATE("The installation is not complete yet!\n"
                                "Are you sure you want to stop it?"),
				B_TRANSLATE("Cancel"), B_TRANSLATE("Stop"), NULL,
				B_WIDTH_AS_USUAL, B_STOP_ALERT);
			alert->SetShortcut(0, B_ESCAPE);
			if (alert->Go() == 0)
				return false;
	}

	_QuitCopyEngine(false);

	BMessage quitWithInstallStatus(B_QUIT_REQUESTED);
	quitWithInstallStatus.AddBool("install_complete",
		fInstallStatus == kFinished);

	fWorkerThread->PostMessage(&quitWithInstallStatus);
	be_app->PostMessage(&quitWithInstallStatus);
	return true;
}


// #pragma mark -


void
InstallerWindow::_ShowOptionalPackages()
{
	if (fPackagesLayoutItem && fSizeViewLayoutItem) {
		fPackagesLayoutItem->SetVisible(fPackagesSwitch->Value());
		fSizeViewLayoutItem->SetVisible(fPackagesSwitch->Value());
	}
}


void
InstallerWindow::_LaunchDriveSetup()
{
	if (be_roster->Launch(kDriveSetupSignature) != B_OK) {
		// Try really hard to launch it. It's very likely that this fails,
		// when we run from the CD and there is only an incomplete mime
		// database for example...
		BPath path;
		if (find_directory(B_SYSTEM_APPS_DIRECTORY, &path) != B_OK
			|| path.Append("DriveSetup") != B_OK) {
			path.SetTo("/boot/system/apps/DriveSetup");
		}
		BEntry entry(path.Path());
		entry_ref ref;
		if (entry.GetRef(&ref) != B_OK || be_roster->Launch(&ref) != B_OK) {
			BAlert* alert = new BAlert("error", B_TRANSLATE("DriveSetup, the "
				"application to configure disk partitions, could not be "
				"launched."), B_TRANSLATE("OK"));
			alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
			alert->Go();
		}
	}
}


void
InstallerWindow::_LaunchBootManager()
{
	// TODO: Currently BootManager always tries to install to the "first"
	// harddisk. If/when it later supports being installed to a certain
	// harddisk, we would have to pass it the disk that contains the target
	// partition here.
	if (be_roster->Launch(kBootManagerSignature) != B_OK) {
		// Try really hard to launch it. It's very likely that this fails,
		// when we run from the CD and there is only an incomplete mime
		// database for example...
		BPath path;
		if (find_directory(B_SYSTEM_APPS_DIRECTORY, &path) != B_OK
			|| path.Append("BootManager") != B_OK) {
			path.SetTo("/boot/system/apps/BootManager");
		}
		BEntry entry(path.Path());
		entry_ref ref;
		if (entry.GetRef(&ref) != B_OK || be_roster->Launch(&ref) != B_OK) {
			BAlert* alert = new BAlert(
				B_TRANSLATE("Failed to launch BootManager"),
				B_TRANSLATE("BootManager, the application to configure the "
					"Haiku boot menu, could not be launched."),
				B_TRANSLATE("OK"), NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
			alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
			alert->Go();
		}
	}
}


void
InstallerWindow::_DisableInterface(bool disable)
{
	fLaunchDriveSetupButton->SetEnabled(!disable);
	fLaunchBootManagerItem->SetEnabled(!disable);
	fMakeBootableItem->SetEnabled(!disable);
	fSrcMenuField->SetEnabled(!disable);
	fDestMenuField->SetEnabled(!disable);
}


void
InstallerWindow::_ScanPartitions()
{
	_SetStatusMessage(B_TRANSLATE("Scanning for disks" B_UTF8_ELLIPSIS));

	BMenuItem *item;
	while ((item = fSrcMenu->RemoveItem((int32)0)))
		delete item;
	while ((item = fDestMenu->RemoveItem((int32)0)))
		delete item;

	fWorkerThread->ScanDisksPartitions(fSrcMenu, fDestMenu);

	if (fSrcMenu->ItemAt(0) != NULL)
		_PublishPackages();

	// If the install is already finished, keep the button as is.
	if (fInstallStatus != kFinished)
		_UpdateControls();
	else
		PostMessage(MSG_INSTALL_FINISHED);
}


void
InstallerWindow::_UpdateControls()
{
	PartitionMenuItem* srcItem = (PartitionMenuItem*)fSrcMenu->FindMarked();
	BString label;
	if (srcItem) {
		label = srcItem->MenuLabel();
	} else {
		if (fSrcMenu->CountItems() == 0)
			label = B_TRANSLATE_COMMENT("<none>", "No partition available");
		else
			label = ((PartitionMenuItem*)fSrcMenu->ItemAt(0))->MenuLabel();
	}
	fSrcMenuField->MenuItem()->SetLabel(label.String());

	// Disable any unsuitable target items, check if at least one partition
	// is suitable.
	bool foundOneSuitableTarget = false;
	for (int32 i = fDestMenu->CountItems() - 1; i >= 0; i--) {
		PartitionMenuItem* dstItem
			= (PartitionMenuItem*)fDestMenu->ItemAt(i);
		if (srcItem != NULL && dstItem->ID() == srcItem->ID()) {
			// Prevent the user from having picked the same partition as source
			// and destination.
			dstItem->SetEnabled(false);
			dstItem->SetMarked(false);
		} else
			dstItem->SetEnabled(dstItem->IsValidTarget());

		if (dstItem->IsEnabled())
			foundOneSuitableTarget = true;
	}

	PartitionMenuItem* dstItem = (PartitionMenuItem*)fDestMenu->FindMarked();
	if (dstItem) {
		label = dstItem->MenuLabel();
	} else {
		if (fDestMenu->CountItems() == 0)
			label = B_TRANSLATE_COMMENT("<none>", "No partition available");
		else
			label = B_TRANSLATE("Please choose target");
	}
	fDestMenuField->MenuItem()->SetLabel(label.String());

	BString statusText;
	if (srcItem != NULL && dstItem != NULL) {
		statusText.SetToFormat(B_TRANSLATE("Press the 'Begin' button to install "
			"from '%1s' onto '%2s'."), srcItem->Name(), dstItem->Name());
	} else if (srcItem != NULL) {
		BString partitionRequiredHaiku = B_TRANSLATE(
			"Haiku has to be installed on a partition that uses "
			"the Be File System, but there are currently no such "
			"partitions available on your system.");

		BString partitionRequiredDebranded = B_TRANSLATE(
			"This operating system has to be installed on a partition "
			"that uses the Be File System, but there are currently "
			"no such partitions available on your system.");

		if (!foundOneSuitableTarget) {
#ifdef HAIKU_DISTRO_COMPATIBILITY_OFFICIAL
			statusText.Append(partitionRequiredHaiku);
#else
			statusText.Append(partitionRequiredDebranded);
#endif
			statusText.Append(" ");
			statusText.Append(B_TRANSLATE(
				"Click on 'Set up partitions" B_UTF8_ELLIPSIS
				"' to create one."));
		} else {
			statusText = B_TRANSLATE(
				"Choose the disk you want to install "
				"onto from the pop-up menu. Then click 'Begin'.");
		}
	} else if (dstItem != NULL) {
		statusText = B_TRANSLATE("Choose the source disk from the "
			"pop-up menu. Then click 'Begin'.");
	} else {
		statusText = B_TRANSLATE("Choose the source and destination disk "
			"from the pop-up menus. Then click 'Begin'.");
	}

	_SetStatusMessage(statusText.String());

	fInstallStatus = kReadyForInstall;
	fBeginButton->SetLabel(B_TRANSLATE("Begin"));
	fBeginButton->SetEnabled(srcItem && dstItem);

	// adjust "Write Boot Sector" and "Set up boot menu" buttons
	if (dstItem != NULL) {
		char buffer[256];
		snprintf(buffer, sizeof(buffer), B_TRANSLATE("Write boot sector to '%s'"),
			dstItem->Name());
		label = buffer;
	} else
		label = B_TRANSLATE("Write boot sector");
	fMakeBootableItem->SetEnabled(dstItem != NULL);
	fMakeBootableItem->SetLabel(label.String());
// TODO: Once bootman support writing to specific disks, enable this, since
// we would pass it the disk which contains the target partition.
//	fLaunchBootManagerItem->SetEnabled(dstItem != NULL);

	if (!fEncouragedToSetupPartitions && !foundOneSuitableTarget) {
		// Focus the users attention on the DriveSetup button
		fEncouragedToSetupPartitions = true;
		PostMessage(ENCOURAGE_DRIVESETUP);
	}
}


void
InstallerWindow::_PublishPackages()
{
	fPackagesView->Clean();
	PartitionMenuItem *item = (PartitionMenuItem *)fSrcMenu->FindMarked();
	if (item == NULL)
		return;

	BPath directory;
	BDiskDeviceRoster roster;
	BDiskDevice device;
	BPartition *partition;
	if (roster.GetPartitionWithID(item->ID(), &device, &partition) == B_OK) {
		if (partition->GetMountPoint(&directory) != B_OK)
			return;
	} else if (roster.GetDeviceWithID(item->ID(), &device) == B_OK) {
		if (device.GetMountPoint(&directory) != B_OK)
			return;
	} else
		return; // shouldn't happen

	directory.Append(kPackagesDirectoryPath);
	BDirectory dir(directory.Path());
	if (dir.InitCheck() != B_OK)
		return;

	BEntry packageEntry;
	BList packages;
	while (dir.GetNextEntry(&packageEntry) == B_OK) {
		Package* package = Package::PackageFromEntry(packageEntry);
		if (package != NULL)
			packages.AddItem(package);
	}
	packages.SortItems(_ComparePackages);

	fPackagesView->AddPackages(packages, new BMessage(PACKAGE_CHECKBOX));
	PostMessage(PACKAGE_CHECKBOX);
}


void
InstallerWindow::_SetStatusMessage(const char *text)
{
	fStatusView->SetText(text);
	fStatusView->InvalidateLayout();
		// In case the status message makes the text view higher than the
		// logo, then we need to resize te whole window to fit it.
}


void
InstallerWindow::_SetCopyEngineCancelSemaphore(sem_id id, bool alreadyLocked)
{
	if (fCopyEngineCancelSemaphore >= 0) {
		if (!alreadyLocked)
			acquire_sem(fCopyEngineCancelSemaphore);
		delete_sem(fCopyEngineCancelSemaphore);
	}
	fCopyEngineCancelSemaphore = id;
}


void
InstallerWindow::_QuitCopyEngine(bool askUser)
{
	if (fCopyEngineCancelSemaphore < 0)
		return;

	// First of all block the copy engine, so that it doesn't continue
	// while the alert is showing, which would be irritating.
	acquire_sem(fCopyEngineCancelSemaphore);

	bool quit = true;
	if (askUser) {
		BAlert* alert = new BAlert("cancel",
			B_TRANSLATE("Are you sure you want to to stop the installation?"),
			B_TRANSLATE_COMMENT("Continue", "In alert after pressing Stop"),
			B_TRANSLATE_COMMENT("Stop", "In alert after pressing Stop"), 0,
			B_WIDTH_AS_USUAL, B_STOP_ALERT);
		alert->SetShortcut(1, B_ESCAPE);
		quit = alert->Go() != 0;
	}

	if (quit) {
		// Make it quit by having it's lock fail...
		_SetCopyEngineCancelSemaphore(-1, true);
	} else
		release_sem(fCopyEngineCancelSemaphore);
}


// #pragma mark -


int
InstallerWindow::_ComparePackages(const void* firstArg, const void* secondArg)
{
	const Group* group1 = *static_cast<const Group* const *>(firstArg);
	const Group* group2 = *static_cast<const Group* const *>(secondArg);
	const Package* package1 = dynamic_cast<const Package*>(group1);
	const Package* package2 = dynamic_cast<const Package*>(group2);
	int sameGroup = strcmp(group1->GroupName(), group2->GroupName());
	if (sameGroup != 0)
		return sameGroup;
	if (package2 == NULL)
		return -1;
	if (package1 == NULL)
		return 1;
	return strcmp(package1->Name(), package2->Name());
}


