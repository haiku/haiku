/*
 * Copyright 2007-2010, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Łukasz 'Sil2100' Zemczak <sil2100@vexillium.org>
 */


#include "InstalledPackageInfo.h"
#include "PackageImageViewer.h"
#include "PackageTextViewer.h"
#include "PackageView.h"

#include <Alert.h>
#include <Button.h>
#include <Catalog.h>
#include <Directory.h>
#include <FindDirectory.h>
#include <Locale.h>
#include <MenuItem.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <ScrollView.h>
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


static void
convert_size(uint64 size, char *buffer, uint32 n)
{
	if (size < 1024)
		snprintf(buffer, n, B_TRANSLATE("%llu bytes"), size);
	else if (size < 1024 * 1024)
		snprintf(buffer, n, B_TRANSLATE("%.1f KiB"), size / 1024.0f);
	else if (size < 1024 * 1024 * 1024)
		snprintf(buffer, n, B_TRANSLATE("%.1f MiB"),
			size / (1024.0f * 1024.0f));
	else {
		snprintf(buffer, n, B_TRANSLATE("%.1f GiB"),
			size / (1024.0f * 1024.0f * 1024.0f));
	}
}



// #pragma mark -


PackageView::PackageView(BRect frame, const entry_ref *ref)
	:
	BView(frame, "package_view", B_FOLLOW_NONE, 0),
	fOpenPanel(new BFilePanel(B_OPEN_PANEL, NULL, NULL, B_DIRECTORY_NODE,
		false)),
	fInfo(ref),
	fInstallProcess(this)
{
	_InitView();

	// Check whether the package has been successfuly parsed
	status_t ret = fInfo.InitCheck();
	if (ret == B_OK)
		_InitProfiles();

	ResizeTo(Bounds().Width(), fInstall->Frame().bottom + 4);
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
		BAlert *warning = new BAlert("parsing_failed",
				B_TRANSLATE("The package file is not readable.\nOne of the "
				"possible reasons for this might be that the requested file "
				"is not a valid BeOS .pkg package."), B_TRANSLATE("OK"),
				NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		warning->Go();

		Window()->PostMessage(B_QUIT_REQUESTED);
		return;
	}

	// Set the window title
	BWindow *parent = Window();
	BString title;
	BString name = fInfo.GetName();
	if (name.CountChars() == 0) {
		title = B_TRANSLATE("Package installer");
	}
	else {
		title = B_TRANSLATE("Install ");
		title += name;
	}
	parent->SetTitle(title.String());
	fInstall->SetTarget(this);

	fOpenPanel->SetTarget(BMessenger(this));
	fInstallTypes->SetTargetForItems(this);

	if (fInfo.InitCheck() == B_OK) {
		// If the package is valid, we can set up the default group and all
		// other things. If not, then the application will close just after
		// attaching the view to the window
		_GroupChanged(0);

		fStatusWindow = new PackageStatus
			(B_TRANSLATE("Installation progress"),
			NULL, NULL, this);

		// Show the splash screen, if present
		BMallocIO *image = fInfo.GetSplashScreen();
		if (image) {
			PackageImageViewer *imageViewer = new PackageImageViewer(image);
			imageViewer->Go();
		}

		// Show the disclaimer/info text popup, if present
		BString disclaimer = fInfo.GetDisclaimer();
		if (disclaimer.Length() != 0) {
			PackageTextViewer *text = new PackageTextViewer(
				disclaimer.String());
			int32 selection = text->Go();
			// The user didn't accept our disclaimer, this means we cannot
			// continue.
			if (selection == 0) {
				BWindow *parent = Window();
				if (parent && parent->Lock())
					parent->Quit();
  			}
		}
	}
}


void
PackageView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case P_MSG_INSTALL:
		{
			fInstall->SetEnabled(false);
			fInstallTypes->SetEnabled(false);
			fDestination->SetEnabled(false);
			fStatusWindow->Show();

			fInstallProcess.Start();
			break;
		}
		case P_MSG_PATH_CHANGED:
		{
			BString path;
			if (msg->FindString("path", &path) == B_OK) {
				fCurrentPath.SetTo(path.String());
			}
			break;
		}
		case P_MSG_OPEN_PANEL:
			fOpenPanel->Show();
			break;
		case P_MSG_GROUP_CHANGED:
		{
			int32 index;
			if (msg->FindInt32("index", &index) == B_OK) {
				_GroupChanged(index);
			}
			break;
		}
		case P_MSG_I_FINISHED:
		{
			BAlert *notify = new BAlert("installation_success",
				B_TRANSLATE("The package you requested has been successfully "
					"installed on your system."),
				B_TRANSLATE("OK"));

			notify->Go();
			fStatusWindow->Hide();
			fInstall->SetEnabled(true);
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
			BAlert *notify = new BAlert("installation_aborted",
				B_TRANSLATE(
					"The installation of the package has been aborted."),
				B_TRANSLATE("OK"));
			notify->Go();
			fStatusWindow->Hide();
			fInstall->SetEnabled(true);
			fInstallTypes->SetEnabled(true);
			fDestination->SetEnabled(true);
			fInstallProcess.Stop();
			break;
		}
		case P_MSG_I_ERROR:
		{
			// TODO: Review this
			BAlert *notify = new BAlert("installation_failed",
				B_TRANSLATE("The requested package failed to install on your "
					"system. This might be a problem with the target package "
					"file. Please consult this issue with the package "
					"distributor."),
				B_TRANSLATE("OK"),
				NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
			fprintf(stderr,
				B_TRANSLATE("Error while installing the package\n"));
			notify->Go();
			fStatusWindow->Hide();
			fInstall->SetEnabled(true);
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
			fInstall->SetEnabled(true);
			fInstallTypes->SetEnabled(true);
			fDestination->SetEnabled(true);
			fInstallProcess.Stop();
			break;
		}
		case B_REFS_RECEIVED:
		{
			entry_ref ref;
			if (msg->FindRef("refs", &ref) == B_OK) {
				BPath path(&ref);

				BMenuItem * item = fDestField->MenuItem();
				dev_t device = dev_for_path(path.Path());
				BVolume volume(device);
				if (volume.InitCheck() != B_OK)
					break;

				BString name = path.Path();
				char sizeString[48];

				convert_size(volume.FreeBytes(), sizeString, 48);
				char buffer[512];
				snprintf(buffer, sizeof(buffer), "(%s free)", sizeString);
				name << buffer;

				item->SetLabel(name.String());
				fCurrentPath.SetTo(path.Path());
			}
			break;
		}
		case B_SIMPLE_DATA:
			if (msg->WasDropped()) {
				uint32 type;
				int32 count;
				status_t ret = msg->GetInfo("refs", &type, &count);
				// Check whether the message means someone dropped a file
				// to our view
				if (ret == B_OK && type == B_REF_TYPE) {
					// If it is, send it along with the refs to the application
					msg->what = B_REFS_RECEIVED;
					be_app->PostMessage(msg);
				}
			}
		default:
			BView::MessageReceived(msg);
			break;
	}
}


int32
PackageView::ItemExists(PackageItem &item, BPath &path, int32 &policy)
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
			switch (item.ItemKind()){
				case P_KIND_SCRIPT:
					formatString = B_TRANSLATE("The script named \'%s\' "
						"already exits in the given path.\nReplace the script "
						"with the one from this package or skip it?");
					break;
				case P_KIND_FILE:
					formatString = B_TRANSLATE("The file named \'%s\' already "
						"exits in the given path.\nReplace the file with the "
						"one from this package or skip it?");
					break;
				case P_KIND_DIRECTORY:
					formatString = B_TRANSLATE("The directory named \'%s\' "
						"already exits in the given path.\nReplace the "
						"directory with one from this package or skip it?");
 					break;
				case P_KIND_SYM_LINK:
					formatString = B_TRANSLATE("The symbolic link named \'%s\' "
						"already exists in the give path.\nReplace the link "
						"with the one from this package or skip it?");
					break;
				default:
					formatString = B_TRANSLATE("The item named \'%s\' already "
						"exits in the given path.\nReplace the item with the "
						"one from this package or skip it?");
					break;
			}
			char buffer[512];
			snprintf(buffer, sizeof(buffer), formatString, path.Leaf());

			BString alertString = buffer;

			BAlert *alert = new BAlert("file_exists", alertString.String(),
				B_TRANSLATE("Replace"),
				B_TRANSLATE("Skip"),
				B_TRANSLATE("Abort"));

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
					alertString << B_TRANSLATE("All existing files will be replaced?");
					actionString = B_TRANSLATE("Replace all");
				} else {
					alertString << B_TRANSLATE("All existing files will be skipped?");
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


/*
void
PackageView::_InitView()
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	BTextView *description = new BTextView(BRect(0, 0, 20, 20), "description",
		BRect(4, 4, 16, 16), B_FOLLOW_NONE, B_WILL_DRAW);
	description->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	description->SetText(fInfo.GetDescription());
	description->MakeEditable(false);
	description->MakeSelectable(false);

	fInstallTypes = new BPopUpMenu(B_TRANSLATE("none"));

	BMenuField *installType = new BMenuField("install_type",
		B_TRANSLATE("Installation type:"), fInstallTypes, 0);
	installType->SetAlignment(B_ALIGN_RIGHT);
	installType->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_MIDDLE));

	fInstallDesc = new BTextView(BRect(0, 0, 10, 10), "install_desc",
		BRect(2, 2, 8, 8), B_FOLLOW_NONE, B_WILL_DRAW);
	fInstallDesc->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fInstallDesc->MakeEditable(false);
	fInstallDesc->MakeSelectable(false);
	fInstallDesc->SetText(B_TRANSLATE("No installation type selected"));
	fInstallDesc->TextHeight(0, fInstallDesc->TextLength());

	fInstall = new BButton("install_button", B_TRANSLATE("Install"),
			new BMessage(P_MSG_INSTALL));

	BView *installField = BGroupLayoutBuilder(B_VERTICAL, 5.0f)
		.AddGroup(B_HORIZONTAL)
			.Add(installType)
			.AddGlue()
		.End()
		.Add(fInstallDesc);

	BBox *installBox = new BBox("install_box");
	installBox->AddChild(installField);

	BView *root = BGroupLayoutBuilder(B_VERTICAL, 3.0f)
		.Add(description)
		.Add(installBox)
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(fInstall)
		.End();

	AddChild(root);

	fInstall->MakeDefault(true);
}*/


void
PackageView::_InitView()
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	BRect rect = Bounds();
	BTextView *description = new BTextView(rect, "description",
		rect.InsetByCopy(5, 5), B_FOLLOW_NONE, B_WILL_DRAW);
	description->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	description->SetText(fInfo.GetDescription());
	description->MakeEditable(false);
	description->MakeSelectable(false);

	float length = description->TextHeight(0, description->TextLength()) + 5;
	if (length > kMaxDescHeight) {
		// Set a scroller for the description.
		description->ResizeTo(rect.Width() - B_V_SCROLL_BAR_WIDTH,
			kMaxDescHeight);
		BScrollView *scroller = new BScrollView("desciption_view", description,
			B_FOLLOW_NONE, B_WILL_DRAW | B_FRAME_EVENTS, false, true,
			B_NO_BORDER);

		AddChild(scroller);
		rect = scroller->Frame();
	}
	else {
		description->ResizeTo(rect.Width(), length);
		AddChild(description);
		rect = description->Frame();
	}

	rect.top = rect.bottom + 2;
	rect.bottom += 100;
	BBox *installBox = new BBox(rect.InsetByCopy(2, 2), "install_box");

	fInstallTypes = new BPopUpMenu(B_TRANSLATE("none"));

	BMenuField *installType = new BMenuField(BRect(2, 2, 100, 50),
		"install_type", B_TRANSLATE("Installation type:"),
		fInstallTypes, false);
	installType->SetDivider(installType->StringWidth(installType->Label()) + 8);
	installType->SetAlignment(B_ALIGN_RIGHT);
	installType->ResizeToPreferred();

	installBox->AddChild(installType);

	rect = installBox->Bounds().InsetBySelf(4, 4);
	rect.top = installType->Frame().bottom;
	fInstallDesc = new BTextView(rect, "install_desc",
		BRect(2, 2, rect.Width() - 2, rect.Height() - 2), B_FOLLOW_NONE,
		B_WILL_DRAW);
	fInstallDesc->MakeEditable(false);
	fInstallDesc->MakeSelectable(false);
	fInstallDesc->SetText(B_TRANSLATE("No installation type selected"));
	fInstallDesc->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fInstallDesc->ResizeTo(rect.Width() - B_V_SCROLL_BAR_WIDTH, 60);
	BScrollView *scroller = new BScrollView("desciption_view", fInstallDesc,
		B_FOLLOW_NONE, B_WILL_DRAW | B_FRAME_EVENTS, false, true, B_NO_BORDER);

	installBox->ResizeTo(installBox->Bounds().Width(),
		scroller->Frame().bottom + 10);

	installBox->AddChild(scroller);

	AddChild(installBox);

	fDestination = new BPopUpMenu(B_TRANSLATE("none"));

	rect = installBox->Frame();
	rect.top = rect.bottom + 5;
	rect.bottom += 35;
	fDestField = new BMenuField(rect, "install_to", B_TRANSLATE("Install to:"),
			fDestination, false);
	fDestField->SetDivider(fDestField->StringWidth(fDestField->Label()) + 8);
	fDestField->SetAlignment(B_ALIGN_RIGHT);
	fDestField->ResizeToPreferred();

	AddChild(fDestField);

	fInstall = new BButton(rect, "install_button", B_TRANSLATE("Install"),
			new BMessage(P_MSG_INSTALL));
	fInstall->ResizeToPreferred();
	AddChild(fInstall);
	fInstall->MoveTo(Bounds().Width() - fInstall->Bounds().Width() - 10,
		rect.top + 2);
	fInstall->MakeDefault(true);
}


void
PackageView::_InitProfiles()
{
	// Set all profiles
	int i = 0, num = fInfo.GetProfileCount();
	pkg_profile *prof;
	BMenuItem *item = 0;
	char sizeString[48];
	BString name = "";
	BMessage *message;

	if (num > 0) { // Add the default item
		prof = fInfo.GetProfile(0);
		convert_size(prof->space_needed, sizeString, 48);
		char buffer[512];
		snprintf(buffer, sizeof(buffer), "(%s)", sizeString);
		name << prof->name << buffer;

		message = new BMessage(P_MSG_GROUP_CHANGED);
		message->AddInt32("index", 0);
		item = new BMenuItem(name.String(), message);
		fInstallTypes->AddItem(item);
		item->SetMarked(true);
		fCurrentType = 0;
	}

	for (i = 1; i < num; i++) {
		prof = fInfo.GetProfile(i);

		if (prof) {
			convert_size(prof->space_needed, sizeString, 48);
			name = prof->name;
			char buffer[512];
			snprintf(buffer, sizeof(buffer), "(%s)", sizeString);
			name << buffer;

			message = new BMessage(P_MSG_GROUP_CHANGED);
			message->AddInt32("index", i);
			item = new BMenuItem(name.String(), message);
			fInstallTypes->AddItem(item);
		}
		else
			fInstallTypes->AddSeparatorItem();
	}
}


status_t
PackageView::_GroupChanged(int32 index)
{
	if (index < 0)
		return B_ERROR;

	BMenuItem *iter;
	int32 i, num = fDestination->CountItems();

	// Clear the choice list
	for (i = 0;i < num;i++) {
		iter = fDestination->RemoveItem((int32)0);
		delete iter;
	}

	fCurrentType = index;
	pkg_profile *prof = fInfo.GetProfile(index);
	BString test;
	
	if (prof) {
		fInstallDesc->SetText(prof->description.String());
		BMenuItem *item = 0;
		BPath path;
		BMessage *temp;
		BVolume volume;
		char buffer[512];
		const char *tmp = B_TRANSLATE("(%s free)");

		if (prof->path_type == P_INSTALL_PATH) {
			dev_t device;
			BString name;
			char sizeString[48];

			if (find_directory(B_BEOS_APPS_DIRECTORY, &path) == B_OK) {
				device = dev_for_path(path.Path());
				if (volume.SetTo(device) == B_OK && !volume.IsReadOnly()) {
					temp = new BMessage(P_MSG_PATH_CHANGED);
					temp->AddString("path", BString(path.Path()));

					convert_size(volume.FreeBytes(), sizeString, 48);
					name = path.Path();
					snprintf(buffer, sizeof(buffer), tmp, sizeString);
					name << buffer;
					item = new BMenuItem(name.String(), temp);
					item->SetTarget(this);
					fDestination->AddItem(item);
				}
			}
			if (find_directory(B_APPS_DIRECTORY, &path) == B_OK) {
				device = dev_for_path(path.Path());
				if (volume.SetTo(device) == B_OK && !volume.IsReadOnly()) {
					temp = new BMessage(P_MSG_PATH_CHANGED);
					temp->AddString("path", BString(path.Path()));

					convert_size(volume.FreeBytes(), sizeString, 48);
					char buffer[512];
					snprintf(buffer, sizeof(buffer), tmp, sizeString);
					name = path.Path();
					name << buffer;
					item = new BMenuItem(name.String(), temp);
					item->SetTarget(this);
					fDestination->AddItem(item);
				}
			}

			if (item != NULL) {
				item->SetMarked(true);
				fCurrentPath.SetTo(path.Path());
			}
			fDestination->AddSeparatorItem();

			item = new BMenuItem(B_TRANSLATE("Other" B_UTF8_ELLIPSIS),
				new BMessage(P_MSG_OPEN_PANEL));
			item->SetTarget(this);
			fDestination->AddItem(item);

			fDestField->SetEnabled(true);
		} else if (prof->path_type == P_USER_PATH) {
			BString name;
			bool defaultPathSet = false;
			char sizeString[48], volumeName[B_FILE_NAME_LENGTH];
			BVolumeRoster roster;
			BDirectory mountPoint;

			while (roster.GetNextVolume(&volume) != B_BAD_VALUE) {
				if (volume.IsReadOnly() || !volume.IsPersistent()
					|| volume.GetRootDirectory(&mountPoint) != B_OK) {
					continue;
				}

				if (path.SetTo(&mountPoint, NULL) != B_OK)
					continue;

				temp = new BMessage(P_MSG_PATH_CHANGED);
				temp->AddString("path", BString(path.Path()));

				convert_size(volume.FreeBytes(), sizeString, 48);
				volume.GetName(volumeName);
				name = volumeName;
				snprintf(buffer, sizeof(buffer), tmp, sizeString);
				name << buffer;
				item = new BMenuItem(name.String(), temp);
				item->SetTarget(this);
				fDestination->AddItem(item);

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
	}

	return B_OK;
}

