/*
 * Copyright 2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2005-2008, Jérôme DUVAL.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "InstallerWindow.h"

#include <stdio.h>
#include <string.h>

#include <Alert.h>
#include <Application.h>
#include <Autolock.h>
#include <Box.h>
#include <ClassInfo.h>
#include <Directory.h>
#include <GridLayoutBuilder.h>
#include <GroupLayoutBuilder.h>
#include <LayoutUtils.h>
#include <MenuBar.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Roster.h>
#include <Screen.h>
#include <SpaceLayoutItem.h>
#include <String.h>
#include <TranslationUtils.h>
#include <TranslatorFormats.h>

#include "tracker_private.h"

#include "DialogPane.h"
#include "PartitionMenuItem.h"


#define DRIVESETUP_SIG "application/x-vnd.Haiku-DriveSetup"

const uint32 BEGIN_MESSAGE = 'iBGN';
const uint32 SHOW_BOTTOM_MESSAGE = 'iSBT';
const uint32 SETUP_MESSAGE = 'iSEP';
const uint32 START_SCAN = 'iSSC';
const uint32 PACKAGE_CHECKBOX = 'iPCB';

class LogoView : public BView {
public:
								LogoView(const BRect& frame);
								LogoView();
	virtual						~LogoView();

	virtual	void				Draw(BRect update);

	virtual	void				GetPreferredSize(float* _width, float* _height);

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
	if (fLogo == NULL)
		return;

	BRect bounds(Bounds());
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
	fLogo = BTranslationUtils::GetBitmap(B_PNG_FORMAT, "haikulogo.png");
}


// #pragma mark -


class SeparatorView : public BView {
public:
								SeparatorView(enum orientation orientation);
	virtual						~SeparatorView();

	virtual	BSize				MinSize();
	virtual	BSize				PreferredSize();
	virtual	BSize				MaxSize();
private:
			enum orientation	fOrientation;
};


SeparatorView::SeparatorView(enum orientation orientation)
	:
	BView("separator", 0),
	fOrientation(orientation)
{
	SetViewColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
		B_DARKEN_2_TINT));
}


SeparatorView::~SeparatorView()
{
}


BSize
SeparatorView::MinSize()
{
	return BLayoutUtils::ComposeSize(ExplicitMinSize(), BSize(0, 0));
}


BSize
SeparatorView::MaxSize()
{
	BSize size(0, 0);
	if (fOrientation == B_VERTICAL)
		size.height = B_SIZE_UNLIMITED;
	else
		size.width = B_SIZE_UNLIMITED;

	return BLayoutUtils::ComposeSize(ExplicitMaxSize(), size);
}


BSize
SeparatorView::PreferredSize()
{
	BSize size(0, 0);
	if (fOrientation == B_VERTICAL)
		size.height = 10;
	else
		size.width = 10;

	return BLayoutUtils::ComposeSize(ExplicitPreferredSize(), size);
}


// #pragma mark -


InstallerWindow::InstallerWindow()
	: BWindow(BRect(-2000, -2000, -1800, -1800), "Installer", B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS),
	fNeedsToCenterOnScreen(true),

	fDriveSetupLaunched(false),
	fInstallStatus(kReadyForInstall),

	fPackagesLayoutItem(NULL),
	fSizeViewLayoutItem(NULL),

	fLastSrcItem(NULL),
	fLastTargetItem(NULL)
{
	fCopyEngine = new CopyEngine(this);

	LogoView* logoView = new LogoView();

	fStatusView = new BTextView("statusView", be_plain_font, NULL, B_WILL_DRAW);
	fStatusView->SetInsets(10, 10, 10, 10);
	fStatusView->MakeEditable(false);
	fStatusView->MakeSelectable(false);

	BSize logoSize = logoView->MinSize();
	fStatusView->SetExplicitMinSize(BSize(logoSize.width * 0.66, B_SIZE_UNSET));

	fBeginButton = new BButton("begin_button", "Begin",
		new BMessage(BEGIN_MESSAGE));
	fBeginButton->MakeDefault(true);
	fBeginButton->SetEnabled(false);

	fSetupButton = new BButton("setup_button",
		"Setup partitions" B_UTF8_ELLIPSIS, new BMessage(SETUP_MESSAGE));

	fPackagesView = new PackagesView("packages_view");
	BScrollView* packagesScrollView = new BScrollView("packagesScroll",
		fPackagesView, B_WILL_DRAW, false, true);

	fDrawButton = new PaneSwitch("options_button");
	fDrawButton->SetLabels("Hide Optional Packages", "Show Optional Packages");
	fDrawButton->SetMessage(new BMessage(SHOW_BOTTOM_MESSAGE));
	fDrawButton->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));
	fDrawButton->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_TOP));

	fDestMenu = new BPopUpMenu("scanning" B_UTF8_ELLIPSIS, true, false);
	fSrcMenu = new BPopUpMenu("scanning" B_UTF8_ELLIPSIS, true, false);

	fSrcMenuField = new BMenuField("srcMenuField", "Install from: ", fSrcMenu,
		NULL);
	fSrcMenuField->SetAlignment(B_ALIGN_RIGHT);

	fDestMenuField = new BMenuField("destMenuField", "Onto: ", fDestMenu,
		NULL);
	fDestMenuField->SetAlignment(B_ALIGN_RIGHT);

	const char* requiredDiskSpaceString
		= "Additional disk space required: 0.0 KB";
	fSizeView = new BStringView("size_view", requiredDiskSpaceString);
	fSizeView->SetAlignment(B_ALIGN_RIGHT);
	fSizeView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));
	fSizeView->SetExplicitAlignment(
		BAlignment(B_ALIGN_RIGHT, B_ALIGN_MIDDLE));

	SetLayout(new BGroupLayout(B_HORIZONTAL));
	AddChild(BGroupLayoutBuilder(B_VERTICAL)
		.Add(BGroupLayoutBuilder(B_HORIZONTAL)
			.Add(logoView)
			.Add(fStatusView)
		)
		.Add(new SeparatorView(B_HORIZONTAL))
		.Add(BGroupLayoutBuilder(B_VERTICAL, 10)
			.Add(BGridLayoutBuilder(0, 10)
				.Add(fSrcMenuField->CreateLabelLayoutItem(), 0, 0)
				.Add(fSrcMenuField->CreateMenuBarLayoutItem(), 1, 0)
				.Add(fDestMenuField->CreateLabelLayoutItem(), 0, 1)
				.Add(fDestMenuField->CreateMenuBarLayoutItem(), 1, 1)

				.Add(BSpaceLayoutItem::CreateVerticalStrut(5), 0, 2, 2)

				.Add(fDrawButton, 0, 3, 2)
				.Add(packagesScrollView, 0, 4, 2)
				.Add(fSizeView, 0, 5, 2)
			)

			.AddStrut(5)

			.Add(BGroupLayoutBuilder(B_HORIZONTAL)
				.Add(fSetupButton)
				.AddGlue()
				.Add(fBeginButton)
			)
			.SetInsets(10, 10, 10, 10)
		)
	);

	// Make the optional packages invisible on start
	BLayout* layout = packagesScrollView->Parent()->GetLayout();
	int32 index = layout->IndexOfView(packagesScrollView);
	fPackagesLayoutItem = layout->ItemAt(index);

	layout = fSizeView->Parent()->GetLayout();
	index = layout->IndexOfView(fSizeView);
	fSizeViewLayoutItem = layout->ItemAt(index);

	fPackagesLayoutItem->SetVisible(false);
	fSizeViewLayoutItem->SetVisible(false);

	// finish creating window
	if (!be_roster->IsRunning(kDeskbarSignature))
		SetFlags(Flags() | B_NOT_MINIMIZABLE);

	Show();

	fDriveSetupLaunched = be_roster->IsRunning(DRIVESETUP_SIG);
 
	if (Lock()) {
		fSetupButton->SetEnabled(!fDriveSetupLaunched);
		Unlock();
	}

	be_roster->StartWatching(this);

	PostMessage(START_SCAN);
}


InstallerWindow::~InstallerWindow()
{
	be_roster->StopWatching(this);
}


void
InstallerWindow::FrameResized(float width, float height)
{
	BWindow::FrameResized(width, height);

	if (fNeedsToCenterOnScreen) {
		// We have created ourselves off-screen, since the size adoption
		// because of the layout management may happen after Show(). We
		// assume that the first frame event is because of this adoption and
		// move ourselves to the screen center...
		fNeedsToCenterOnScreen = false;
		BRect frame = BScreen(this).Frame();
		MoveTo(frame.left + (frame.Width() - Frame().Width()) / 2,
			frame.top + (frame.Height() - Frame().Height()) / 2);
	}
}


void
InstallerWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case RESET_INSTALL:
			fInstallStatus = kReadyForInstall;
			fBeginButton->SetEnabled(true);
			_DisableInterface(false);
			fBeginButton->SetLabel("Begin");
			break;
		case START_SCAN:
			_ScanPartitions();
			break;
		case BEGIN_MESSAGE:
			switch (fInstallStatus) {
				case kReadyForInstall:
				{
					BList *list = new BList();
					int32 size = 0;
					fPackagesView->GetPackagesToInstall(list, &size);
					fCopyEngine->SetPackagesList(list);
					fCopyEngine->SetSpaceRequired(size);
					fInstallStatus = kInstalling;
					BMessenger(fCopyEngine).SendMessage(ENGINE_START);
					fBeginButton->SetLabel("Stop");
					_DisableInterface(true);
					break;
				}
				case kInstalling:
					if (fCopyEngine->Cancel()) {
						fInstallStatus = kCancelled;
						_SetStatusMessage("Installation cancelled.");
					}
					break;
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
		case SRC_PARTITION:
			if (fLastSrcItem == fSrcMenu->FindMarked())
				break;
			fLastSrcItem = fSrcMenu->FindMarked();
			_PublishPackages();
			_UpdateMenus();
			break;
		case TARGET_PARTITION:
			if (fLastTargetItem == fDestMenu->FindMarked())
				break;
			fLastTargetItem = fDestMenu->FindMarked();
			_UpdateMenus();
			break;
		case SETUP_MESSAGE:
			_LaunchDriveSetup();
			break;
		case PACKAGE_CHECKBOX: {
			char buffer[15];
			fPackagesView->GetTotalSizeAsString(buffer);
			char string[255];
			sprintf(string, "Additional disk space required: %s", buffer);
			fSizeView->SetText(string);
			break;
		}
		case STATUS_MESSAGE: {
			if (fInstallStatus != kInstalling)
				break;
			const char *status;
			if (msg->FindString("status", &status) == B_OK) {
				fLastStatus = fStatusView->Text();
				_SetStatusMessage(status);
			} else
				_SetStatusMessage(fLastStatus.String());
			break;
		}
		case INSTALL_FINISHED:
			fBeginButton->SetLabel("Quit");
			_SetStatusMessage("Installation completed.");
			fInstallStatus = kFinished;
			_DisableInterface(false);
			break;
		case B_SOME_APP_LAUNCHED:
		case B_SOME_APP_QUIT:
		{
			const char *signature;
			if (msg->FindString("be:signature", &signature) == B_OK
				&& strcasecmp(signature, DRIVESETUP_SIG) == 0) {
				fDriveSetupLaunched = msg->what == B_SOME_APP_LAUNCHED;
				fBeginButton->SetEnabled(!fDriveSetupLaunched);
				_DisableInterface(fDriveSetupLaunched);
				if (fDriveSetupLaunched)
					_SetStatusMessage("Running DriveSetup" B_UTF8_ELLIPSIS
						"\nClose DriveSetup to continue with the\n"
						"installation.");
				else
					_ScanPartitions();
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
			"Please close the DriveSetup window before closing the\n"
			"Installer window.", "OK"))->Go();
		return false;
	}
	be_app->PostMessage(B_QUIT_REQUESTED);
	fCopyEngine->PostMessage(B_QUIT_REQUESTED);
	return true;
}


// #pragma mark -


void
InstallerWindow::_ShowOptionalPackages()
{
	if (fPackagesLayoutItem && fSizeViewLayoutItem) {
		fPackagesLayoutItem->SetVisible(fDrawButton->Value());
		fSizeViewLayoutItem->SetVisible(fDrawButton->Value());
	}
}


void
InstallerWindow::_LaunchDriveSetup()
{
	if (be_roster->Launch(DRIVESETUP_SIG) != B_OK) {
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
			BAlert* alert = new BAlert("error", "DriveSetup, the application "
				"to configure disk partitions, could not be launched.",
				"Ok");
			alert->Go();
		}
	}
}


void
InstallerWindow::_DisableInterface(bool disable)
{
	fSetupButton->SetEnabled(!disable);
	fSrcMenuField->SetEnabled(!disable);
	fDestMenuField->SetEnabled(!disable);
}


void
InstallerWindow::_ScanPartitions()
{
	_SetStatusMessage("Scanning for disks" B_UTF8_ELLIPSIS);

	BMenuItem *item;
	while ((item = fSrcMenu->RemoveItem((int32)0)))
		delete item;
	while ((item = fDestMenu->RemoveItem((int32)0)))
		delete item;

	fCopyEngine->ScanDisksPartitions(fSrcMenu, fDestMenu);

	if (fSrcMenu->ItemAt(0)) {
		_PublishPackages();
	}
	_UpdateMenus();
	_SetStatusMessage("Choose the disk you want to install onto from the "
		"pop-up menu. Then click \"Begin\".");
}


void
InstallerWindow::_UpdateMenus()
{
	PartitionMenuItem *item1 = (PartitionMenuItem *)fSrcMenu->FindMarked();
	BString label;
	if (item1) {
		label = item1->MenuLabel();
	} else {
		if (fSrcMenu->CountItems() == 0)
			label = "<none>";
		else
			label = ((PartitionMenuItem *)fSrcMenu->ItemAt(0))->MenuLabel();
	}
	fSrcMenuField->TruncateString(&label, B_TRUNCATE_END, 260);
	fSrcMenuField->MenuItem()->SetLabel(label.String());

	PartitionMenuItem *item2 = (PartitionMenuItem *)fDestMenu->FindMarked();
	if (item2) {
		label = item2->MenuLabel();
	} else {
		if (fDestMenu->CountItems() == 0)
			label = "<none>";
		else
			label = "Please Choose Target";
	}
	fDestMenuField->TruncateString(&label, B_TRUNCATE_END, 260);
	fDestMenuField->MenuItem()->SetLabel(label.String());
	char message[255];
	sprintf(message, "Press the Begin button to install from '%s' onto '%s'",
		item1 ? item1->Name() : "null", item2 ? item2->Name() : "null");
	_SetStatusMessage(message);
	if (item1 && item2)
		fBeginButton->SetEnabled(true);
}


void
InstallerWindow::_PublishPackages()
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
		if (partition->GetMountPoint(&directory) != B_OK)
			return;
	} else if (roster.GetDeviceWithID(item->ID(), &device) == B_OK) {
		if (device.GetMountPoint(&directory) != B_OK)
			return;
	} else
		return; // shouldn't happen
#else
	BPath directory = "/BeOS 5 PE Max Edition V3.1 beta";
#endif

	directory.Append(PACKAGES_DIRECTORY);
	BDirectory dir(directory.Path());
	if (dir.InitCheck() != B_OK)
		return;

	BEntry packageEntry;
	BList packages;
	while (dir.GetNextEntry(&packageEntry) == B_OK) {
		Package *package = Package::PackageFromEntry(packageEntry);
		if (package) {
			packages.AddItem(package);
		}
	}
	packages.SortItems(_ComparePackages);

	fPackagesView->AddPackages(packages, new BMessage(PACKAGE_CHECKBOX));
	PostMessage(PACKAGE_CHECKBOX);
}


void
InstallerWindow::_SetStatusMessage(const char *text)
{
	fStatusView->SetText(text);
}


// #pragma mark -


int
InstallerWindow::_ComparePackages(const void *firstArg, const void *secondArg)
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


