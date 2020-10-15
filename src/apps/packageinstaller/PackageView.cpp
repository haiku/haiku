/*
 * Copyright 2007-2014, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Łukasz 'Sil2100' Zemczak <sil2100@vexillium.org>
 *		Stephan Aßmus <superstippi@gmx.de>
 */


#include "InstalledPackageInfo.h"
#include "PackageImageViewer.h"
#include "PackageTextViewer.h"
#include "PackageView.h"

#include <Alert.h>
#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <Directory.h>
#include <FilePanel.h>
#include <FindDirectory.h>
#include <Locale.h>
#include <LayoutBuilder.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <ScrollView.h>
#include <StringForSize.h>
#include <TextView.h>
#include <Volume.h>
#include <VolumeRoster.h>
#include <Window.h>

#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <GroupView.h>

#include <fs_info.h>
#include <stdio.h> // For debugging


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PackageView"

const float kMaxDescHeight = 125.0f;
const uint32 kSeparatorIndex = 3;


// #pragma mark -


PackageView::PackageView(const entry_ref* ref)
	:
	BView("package_view", 0),
	fOpenPanel(new BFilePanel(B_OPEN_PANEL, NULL, NULL, B_DIRECTORY_NODE,
		false)),
	fExpectingOpenPanelResult(false),
	fInfo(ref),
	fInstallProcess(this)
{
	_InitView();

	// Check whether the package has been successfuly parsed
	status_t ret = fInfo.InitCheck();
	if (ret == B_OK)
		_InitProfiles();
}


PackageView::~PackageView()
{
	delete fOpenPanel;
}


void
PackageView::AttachedToWindow()
{
	status_t ret = fInfo.InitCheck();
	if (ret != B_OK && ret != B_NO_INIT) {
		BAlert* warning = new BAlert("parsing_failed",
				B_TRANSLATE("The package file is not readable.\nOne of the "
				"possible reasons for this might be that the requested file "
				"is not a valid BeOS .pkg package."), B_TRANSLATE("OK"),
				NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		warning->SetFlags(warning->Flags() | B_CLOSE_ON_ESCAPE);
		warning->Go();

		Window()->PostMessage(B_QUIT_REQUESTED);
		return;
	}

	// Set the window title
	BWindow* parent = Window();
	BString title;
	BString name = fInfo.GetName();
	if (name.CountChars() == 0) {
		title = B_TRANSLATE("Package installer");
	} else {
		title = B_TRANSLATE("Install %name%");
		title.ReplaceAll("%name%", name);
	}
	parent->SetTitle(title.String());
	fBeginButton->SetTarget(this);

	fOpenPanel->SetTarget(BMessenger(this));
	fInstallTypes->SetTargetForItems(this);

	if (ret != B_OK)
		return;

	// If the package is valid, we can set up the default group and all
	// other things. If not, then the application will close just after
	// attaching the view to the window
	_InstallTypeChanged(0);

	fStatusWindow = new PackageStatus(B_TRANSLATE("Installation progress"),
		NULL, NULL, this);

	// Show the splash screen, if present
	BMallocIO* image = fInfo.GetSplashScreen();
	if (image != NULL) {
		PackageImageViewer* imageViewer = new PackageImageViewer(image);
		imageViewer->Go();
	}

	// Show the disclaimer/info text popup, if present
	BString disclaimer = fInfo.GetDisclaimer();
	if (disclaimer.Length() != 0) {
		PackageTextViewer* text = new PackageTextViewer(
			disclaimer.String());
		int32 selection = text->Go();
		// The user didn't accept our disclaimer, this means we cannot
		// continue.
		if (selection == 0)
			parent->Quit();
	}
}


void
PackageView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case P_MSG_INSTALL:
		{
			fBeginButton->SetEnabled(false);
			fInstallTypes->SetEnabled(false);
			fDestination->SetEnabled(false);
			fStatusWindow->Show();

			fInstallProcess.Start();
			break;
		}

		case P_MSG_PATH_CHANGED:
		{
			BString path;
			if (message->FindString("path", &path) == B_OK)
				fCurrentPath.SetTo(path.String());
			break;
		}

		case P_MSG_OPEN_PANEL:
			fExpectingOpenPanelResult = true;
			fOpenPanel->Show();
			break;

		case P_MSG_INSTALL_TYPE_CHANGED:
		{
			int32 index;
			if (message->FindInt32("index", &index) == B_OK)
				_InstallTypeChanged(index);
			break;
		}

		case P_MSG_I_FINISHED:
		{
			BAlert* notify = new BAlert("installation_success",
				B_TRANSLATE("The package you requested has been successfully "
					"installed on your system."),
				B_TRANSLATE("OK"));
			notify->SetFlags(notify->Flags() | B_CLOSE_ON_ESCAPE);

			notify->Go();
			fStatusWindow->Hide();
			fBeginButton->SetEnabled(true);
			fInstallTypes->SetEnabled(true);
			fDestination->SetEnabled(true);
			fInstallProcess.Stop();

			BWindow *parent = Window();
			if (parent && parent->Lock())
				parent->Quit();
			break;
		}

		case P_MSG_I_ABORT:
		{
			BAlert* notify = new BAlert("installation_aborted",
				B_TRANSLATE(
					"The installation of the package has been aborted."),
				B_TRANSLATE("OK"));
			notify->SetFlags(notify->Flags() | B_CLOSE_ON_ESCAPE);
			notify->Go();
			fStatusWindow->Hide();
			fBeginButton->SetEnabled(true);
			fInstallTypes->SetEnabled(true);
			fDestination->SetEnabled(true);
			fInstallProcess.Stop();
			break;
		}

		case P_MSG_I_ERROR:
		{
			// TODO: Review this
			BAlert* notify = new BAlert("installation_failed",
				B_TRANSLATE("The requested package failed to install on your "
					"system. This might be a problem with the target package "
					"file. Please consult this issue with the package "
					"distributor."),
				B_TRANSLATE("OK"),
				NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
			fputs(B_TRANSLATE("Error while installing the package\n"),
				stderr);
			notify->SetFlags(notify->Flags() | B_CLOSE_ON_ESCAPE);
			notify->Go();
			fStatusWindow->Hide();
			fBeginButton->SetEnabled(true);
			fInstallTypes->SetEnabled(true);
			fDestination->SetEnabled(true);
			fInstallProcess.Stop();
			break;
		}

		case P_MSG_STOP:
		{
			// This message is sent to us by the PackageStatus window, informing
			// user interruptions.
			// We actually use this message only when a post installation script
			// is running and we want to kill it while it's still running
			fStatusWindow->Hide();
			fBeginButton->SetEnabled(true);
			fInstallTypes->SetEnabled(true);
			fDestination->SetEnabled(true);
			fInstallProcess.Stop();
			break;
		}

		case B_REFS_RECEIVED:
		{
			if (!_ValidateFilePanelMessage(message))
				break;

			entry_ref ref;
			if (message->FindRef("refs", &ref) == B_OK) {
				BPath path(&ref);
				if (path.InitCheck() != B_OK)
					break;

				dev_t device = dev_for_path(path.Path());
				BVolume volume(device);
				if (volume.InitCheck() != B_OK)
					break;

				BMenuItem* item = fDestField->MenuItem();

				BString name = _NamePlusSizeString(path.Path(),
					volume.FreeBytes(), B_TRANSLATE("%name% (%size% free)"));

				item->SetLabel(name.String());
				fCurrentPath.SetTo(path.Path());
			}
			break;
		}

		case B_CANCEL:
		{
			if (!_ValidateFilePanelMessage(message))
				break;

			// file panel aborted, select first suitable item
			for (int32 i = 0; i < fDestination->CountItems(); i++) {
				BMenuItem* item = fDestination->ItemAt(i);
				BMessage* message = item->Message();
				if (message == NULL)
					continue;
				BString path;
				if (message->FindString("path", &path) == B_OK) {
					fCurrentPath.SetTo(path.String());
					item->SetMarked(true);
					break;
				}
			}
			break;
		}

		case B_SIMPLE_DATA:
			if (message->WasDropped()) {
				uint32 type;
				int32 count;
				status_t ret = message->GetInfo("refs", &type, &count);
				// check whether the message means someone dropped a file
				// to our view
				if (ret == B_OK && type == B_REF_TYPE) {
					// if it is, send it along with the refs to the application
					message->what = B_REFS_RECEIVED;
					be_app->PostMessage(message);
				}
			}
			// fall-through
		default:
			BView::MessageReceived(message);
			break;
	}
}


int32
PackageView::ItemExists(PackageItem& item, BPath& path, int32& policy)
{
	int32 choice = P_EXISTS_NONE;

	switch (policy) {
		case P_EXISTS_OVERWRITE:
			choice = P_EXISTS_OVERWRITE;
			break;

		case P_EXISTS_SKIP:
			choice = P_EXISTS_SKIP;
			break;

		case P_EXISTS_ASK:
		case P_EXISTS_NONE:
		{
			const char* formatString;
			switch (item.ItemKind()) {
				case P_KIND_SCRIPT:
					formatString = B_TRANSLATE("The script named \'%s\' "
						"already exists in the given path.\nReplace the script "
						"with the one from this package or skip it?");
					break;
				case P_KIND_FILE:
					formatString = B_TRANSLATE("The file named \'%s\' already "
						"exists in the given path.\nReplace the file with the "
						"one from this package or skip it?");
					break;
				case P_KIND_DIRECTORY:
					formatString = B_TRANSLATE("The directory named \'%s\' "
						"already exists in the given path.\nReplace the "
						"directory with one from this package or skip it?");
					break;
				case P_KIND_SYM_LINK:
					formatString = B_TRANSLATE("The symbolic link named \'%s\' "
						"already exists in the given path.\nReplace the link "
						"with the one from this package or skip it?");
					break;
				default:
					formatString = B_TRANSLATE("The item named \'%s\' already "
						"exists in the given path.\nReplace the item with the "
						"one from this package or skip it?");
					break;
			}
			char buffer[512];
			snprintf(buffer, sizeof(buffer), formatString, path.Leaf());

			BString alertString = buffer;

			BAlert* alert = new BAlert("file_exists", alertString.String(),
				B_TRANSLATE("Replace"),
				B_TRANSLATE("Skip"),
				B_TRANSLATE("Abort"));
			alert->SetShortcut(2, B_ESCAPE);

			choice = alert->Go();
			switch (choice) {
				case 0:
					choice = P_EXISTS_OVERWRITE;
					break;
				case 1:
					choice = P_EXISTS_SKIP;
					break;
				default:
					return P_EXISTS_ABORT;
			}

			if (policy == P_EXISTS_NONE) {
				// TODO: Maybe add 'No, but ask again' type of choice as well?
				alertString = B_TRANSLATE("Do you want to remember this "
					"decision for the rest of this installation?\n");

				BString actionString;
				if (choice == P_EXISTS_OVERWRITE) {
					alertString << B_TRANSLATE(
						"All existing files will be replaced?");
					actionString = B_TRANSLATE("Replace all");
				} else {
					alertString << B_TRANSLATE(
						"All existing files will be skipped?");
					actionString = B_TRANSLATE("Skip all");
				}
				alert = new BAlert("policy_decision", alertString.String(),
					actionString.String(), B_TRANSLATE("Ask again"));

				int32 decision = alert->Go();
				if (decision == 0)
					policy = choice;
				else
					policy = P_EXISTS_ASK;
			}
			break;
		}
	}

	return choice;
}


// #pragma mark -


class DescriptionTextView : public BTextView {
public:
	DescriptionTextView(const char* name, float minHeight)
		:
		BTextView(name)
	{
		SetExplicitMinSize(BSize(B_SIZE_UNSET, minHeight));
	}

	virtual void AttachedToWindow()
	{
		BTextView::AttachedToWindow();
		_UpdateScrollBarVisibility();
	}

	virtual void FrameResized(float width, float height)
	{
		BTextView::FrameResized(width, height);
		_UpdateScrollBarVisibility();
	}

	virtual void Draw(BRect updateRect)
	{
		BTextView::Draw(updateRect);
		_UpdateScrollBarVisibility();
	}

private:
	void _UpdateScrollBarVisibility()
	{
		BScrollBar* verticalBar = ScrollBar(B_VERTICAL);
		if (verticalBar != NULL) {
			float min;
			float max;
			verticalBar->GetRange(&min, &max);
			if (min == max) {
				if (!verticalBar->IsHidden(verticalBar))
					verticalBar->Hide();
			} else {
				if (verticalBar->IsHidden(verticalBar))
					verticalBar->Show();
			}
		}
	}
};


void
PackageView::_InitView()
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	float fontHeight = be_plain_font->Size();
	rgb_color textColor = ui_color(B_PANEL_TEXT_COLOR);

	BTextView* packageDescriptionView = new DescriptionTextView(
		"package description", fontHeight * 13);
	packageDescriptionView->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	packageDescriptionView->SetText(fInfo.GetDescription());
	packageDescriptionView->MakeEditable(false);
	packageDescriptionView->MakeSelectable(false);
	packageDescriptionView->SetFontAndColor(be_plain_font, B_FONT_ALL,
		&textColor);

	BScrollView* descriptionScrollView = new BScrollView(
		"package description scroll view", packageDescriptionView,
		0, false, true, B_NO_BORDER);

	// Install type menu field
	fInstallTypes = new BPopUpMenu(B_TRANSLATE("none"));
	BMenuField* installType = new BMenuField("install_type",
		B_TRANSLATE("Installation type:"), fInstallTypes);

	// Install type description text view
	fInstallTypeDescriptionView = new DescriptionTextView(
		"install type description", fontHeight * 4);
	fInstallTypeDescriptionView->MakeEditable(false);
	fInstallTypeDescriptionView->MakeSelectable(false);
	fInstallTypeDescriptionView->SetInsets(8, 0, 0, 0);
		// Left inset needs to match BMenuField text offset
	fInstallTypeDescriptionView->SetText(
		B_TRANSLATE("No installation type selected"));
	fInstallTypeDescriptionView->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	BFont font(be_plain_font);
	font.SetSize(ceilf(font.Size() * 0.85));
	fInstallTypeDescriptionView->SetFontAndColor(&font, B_FONT_ALL,
		&textColor);

	BScrollView* installTypeScrollView = new BScrollView(
		"install type description scroll view", fInstallTypeDescriptionView,
		 0, false, true, B_NO_BORDER);

	// Destination menu field
	fDestination = new BPopUpMenu(B_TRANSLATE("none"));
	fDestField = new BMenuField("install_to", B_TRANSLATE("Install to:"),
		fDestination);

	fBeginButton = new BButton("begin_button", B_TRANSLATE("Begin"),
		new BMessage(P_MSG_INSTALL));

	BLayoutItem* typeLabelItem = installType->CreateLabelLayoutItem();
	BLayoutItem* typeMenuItem = installType->CreateMenuBarLayoutItem();

	BLayoutItem* destFieldLabelItem = fDestField->CreateLabelLayoutItem();
	BLayoutItem* destFieldMenuItem = fDestField->CreateMenuBarLayoutItem();

	float forcedMinWidth = be_plain_font->StringWidth("XXX") * 5;
	destFieldMenuItem->SetExplicitMinSize(BSize(forcedMinWidth, B_SIZE_UNSET));
	typeMenuItem->SetExplicitMinSize(BSize(forcedMinWidth, B_SIZE_UNSET));

	BAlignment labelAlignment(B_ALIGN_RIGHT, B_ALIGN_VERTICAL_UNSET);
	typeLabelItem->SetExplicitAlignment(labelAlignment);
	destFieldLabelItem->SetExplicitAlignment(labelAlignment);

	// Build the layout
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.Add(descriptionScrollView)
		.AddGrid(B_USE_SMALL_SPACING, B_USE_DEFAULT_SPACING)
			.Add(typeLabelItem, 0, 0)
			.Add(typeMenuItem, 1, 0)
			.Add(installTypeScrollView, 1, 1)
			.Add(destFieldLabelItem, 0, 2)
			.Add(destFieldMenuItem, 1, 2)
		.End()
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(fBeginButton)
		.End()
		.SetInsets(B_USE_DEFAULT_SPACING)
	;

	fBeginButton->MakeDefault(true);
}


void
PackageView::_InitProfiles()
{
	int count = fInfo.GetProfileCount();

	if (count > 0) {
		// Add the default item
		pkg_profile* profile = fInfo.GetProfile(0);
		BMenuItem* item = _AddInstallTypeMenuItem(profile->name,
			profile->space_needed, 0);
		item->SetMarked(true);
		fCurrentType = 0;
	}

	for (int i = 1; i < count; i++) {
		pkg_profile* profile = fInfo.GetProfile(i);

		if (profile != NULL)
			_AddInstallTypeMenuItem(profile->name, profile->space_needed, i);
		else
			fInstallTypes->AddSeparatorItem();
	}
}


status_t
PackageView::_InstallTypeChanged(int32 index)
{
	if (index < 0)
		return B_ERROR;

	// Clear the choice list
	for (int32 i = fDestination->CountItems() - 1; i >= 0; i--) {
		BMenuItem* item = fDestination->RemoveItem(i);
		delete item;
	}

	fCurrentType = index;
	pkg_profile* profile = fInfo.GetProfile(index);

	if (profile == NULL)
		return B_ERROR;

	BString typeDescription = profile->description;
	if (typeDescription.IsEmpty())
		typeDescription = profile->name;

	fInstallTypeDescriptionView->SetText(typeDescription.String());

	BPath path;
	BVolume volume;

	if (profile->path_type == P_INSTALL_PATH) {
		BMenuItem* item = NULL;
		if (find_directory(B_SYSTEM_NONPACKAGED_DIRECTORY, &path) == B_OK) {
			dev_t device = dev_for_path(path.Path());
			if (volume.SetTo(device) == B_OK && !volume.IsReadOnly()
				&& path.Append("apps") == B_OK) {
				item = _AddDestinationMenuItem(path.Path(), volume.FreeBytes(),
					path.Path());
			}
		}

		if (item != NULL) {
			item->SetMarked(true);
			fCurrentPath.SetTo(path.Path());
			fDestination->AddSeparatorItem();
		}

		_AddMenuItem(B_TRANSLATE("Other" B_UTF8_ELLIPSIS),
			new BMessage(P_MSG_OPEN_PANEL), fDestination);

		fDestField->SetEnabled(true);
	} else if (profile->path_type == P_USER_PATH) {
		bool defaultPathSet = false;
		BVolumeRoster roster;

		while (roster.GetNextVolume(&volume) != B_BAD_VALUE) {
			BDirectory mountPoint;
			if (volume.IsReadOnly() || !volume.IsPersistent()
				|| volume.GetRootDirectory(&mountPoint) != B_OK) {
				continue;
			}

			if (path.SetTo(&mountPoint, NULL) != B_OK)
				continue;

			char volumeName[B_FILE_NAME_LENGTH];
			volume.GetName(volumeName);

			BMenuItem* item = _AddDestinationMenuItem(volumeName,
				volume.FreeBytes(), path.Path());

			// The first volume becomes the default element
			if (!defaultPathSet) {
				item->SetMarked(true);
				fCurrentPath.SetTo(path.Path());
				defaultPathSet = true;
			}
		}

		fDestField->SetEnabled(true);
	} else
		fDestField->SetEnabled(false);

	return B_OK;
}


BString
PackageView::_NamePlusSizeString(BString baseName, size_t size,
	const char* format) const
{
	char sizeString[48];
	string_for_size(size, sizeString, sizeof(sizeString));

	BString name(format);
	name.ReplaceAll("%name%", baseName);
	name.ReplaceAll("%size%", sizeString);

	return name;
}


BMenuItem*
PackageView::_AddInstallTypeMenuItem(BString baseName, size_t size,
	int32 index) const
{
	BString name = _NamePlusSizeString(baseName, size,
		B_TRANSLATE("%name% (%size%)"));

	BMessage* message = new BMessage(P_MSG_INSTALL_TYPE_CHANGED);
	message->AddInt32("index", index);

	return _AddMenuItem(name, message, fInstallTypes);
}


BMenuItem*
PackageView::_AddDestinationMenuItem(BString baseName, size_t size,
	const char* path) const
{
	BString name = _NamePlusSizeString(baseName, size,
		B_TRANSLATE("%name% (%size% free)"));

	BMessage* message = new BMessage(P_MSG_PATH_CHANGED);
	message->AddString("path", path);

	return _AddMenuItem(name, message, fDestination);
}


BMenuItem*
PackageView::_AddMenuItem(const char* name, BMessage* message,
	BMenu* menu) const
{
	BMenuItem* item = new BMenuItem(name, message);
	item->SetTarget(this);
	menu->AddItem(item);
	return item;
}


bool
PackageView::_ValidateFilePanelMessage(BMessage* message)
{
	if (!fExpectingOpenPanelResult)
		return false;

	fExpectingOpenPanelResult = false;
	return true;
}
