/*
 * Copyright 2004-2015 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 *	Authors:
 *		Alexander von Gluck, <kallisti5@unixzen.com>
 *		Adrien Destugues, <pulkomandy@pulkomandy.tk>
 */


#include "NetworkSetupWindow.h"

#include <Application.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <Deskbar.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <InterfaceKit.h>
#include <Locale.h>
#include <Roster.h>
#include <StorageKit.h>
#include <SupportKit.h>
#include <TabView.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


const char* kNetworkStatusSignature = "application/x-vnd.Haiku-NetworkStatus";


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT	"NetworkSetupWindow"


NetworkSetupWindow::NetworkSetupWindow()
	:
	BWindow(BRect(100, 100, 300, 300), B_TRANSLATE("Network"), B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS),
	fAddOnCount(0)
{
	// ---- Profiles section
#if 0
	BMenu *profilesPopup = new BPopUpMenu("<none>");
	_BuildProfilesMenu(profilesPopup, kMsgProfileSelected);

	BMenuField *profilesMenuField = new BMenuField("profiles_menu",
		B_TRANSLATE("Profile:"), profilesPopup);

	profilesMenuField->SetFont(be_bold_font);
	profilesMenuField->SetEnabled(false);
#endif

	// ---- Settings section

	fPanel = new BTabView("tabs", B_WIDTH_FROM_LABEL);

	fApplyButton = new BButton("apply", B_TRANSLATE("Apply"),
		new BMessage(kMsgApply));
	SetDefaultButton(fApplyButton);

	fRevertButton = new BButton("revert", B_TRANSLATE("Revert"),
		new BMessage(kMsgRevert));
	// fRevertButton->SetEnabled(false);

	BMessage* message = new BMessage(kMsgToggleReplicant);
	BCheckBox* replicantStatus = new BCheckBox("replicantStatus",
		B_TRANSLATE("Show interfaces status in Deskbar"), message);
	replicantStatus->SetValue(_IsReplicantInstalled());

	// Build the layout
	SetLayout(new BGroupLayout(B_VERTICAL));

	AddChild(BGroupLayoutBuilder(B_VERTICAL, B_USE_SMALL_SPACING)
#if 0
		.AddGroup(B_HORIZONTAL, B_USE_SMALL_SPACING)
			.Add(profilesMenuField)
			.AddGlue()
		.End()
#endif
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.Add(fPanel)
			.Add(new BGroupView("panel"))
		.End()
		.Add(replicantStatus)
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.Add(fRevertButton)
			.AddGlue()
			.Add(fApplyButton)
		.End()
		.SetInsets(B_USE_SMALL_SPACING, B_USE_SMALL_SPACING,
			B_USE_SMALL_SPACING, B_USE_SMALL_SPACING)
	);

	_BuildShowTabView();

	fAddOnView = NULL;

	CenterOnScreen();
}


NetworkSetupWindow::~NetworkSetupWindow()
{
}


bool
NetworkSetupWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


void
NetworkSetupWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgProfileNew:
			break;

		case kMsgProfileSelected:
		{
			const char* path;
			if (message->FindString("path", &path) != B_OK)
				break;

			BPath name(path);
			bool isCurrent = strcmp(name.Leaf(), "current") == 0;

			fApplyButton->SetEnabled(!isCurrent);
			break;
		}

		case kMsgRevert:
		{
			for (int index = 0; index < fAddOnCount; index++) {
				NetworkSetupAddOn* addOn = fNetworkAddOnMap[index];
				addOn->Revert();
			}
			break;
		}

		case kMsgApply:
		{
			for (int index = 0; index < fAddOnCount; index++) {
				NetworkSetupAddOn* addOn = fNetworkAddOnMap[index];
				addOn->Save();
			}
			break;
		}

		case kMsgToggleReplicant:
		{
			_ShowReplicant(message->GetInt32("be:value", B_CONTROL_OFF)
				== B_CONTROL_ON);
			break;
		}

		default:
			inherited::MessageReceived(message);
	}
}


void
NetworkSetupWindow::_BuildProfilesMenu(BMenu* menu, int32 what)
{
	char currentProfile[256] = { 0 };

	menu->SetRadioMode(true);

	BDirectory dir("/boot/system/settings/network/profiles");
	if (dir.InitCheck() == B_OK) {
		BEntry entry;

		dir.Rewind();
		while (dir.GetNextEntry(&entry) >= 0) {
			BPath name;
			entry.GetPath(&name);

			if (entry.IsSymLink() &&
				strcmp("current", name.Leaf()) == 0) {
				BSymLink symlink(&entry);

				if (symlink.IsAbsolute())
					// oh oh, sorry, wrong symlink...
					continue;

				symlink.ReadLink(currentProfile, sizeof(currentProfile));
				continue;
			};

			if (!entry.IsDirectory())
				continue;

			BMessage* message = new BMessage(what);
			message->AddString("path", name.Path());

			BMenuItem* item = new BMenuItem(name.Leaf(), message);
			menu->AddItem(item);
		}
	}

	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(B_TRANSLATE("New" B_UTF8_ELLIPSIS),
		new BMessage(kMsgProfileNew)));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Manage" B_UTF8_ELLIPSIS),
		new BMessage(kMsgProfileManage)));

	if (currentProfile[0] != '\0') {
		BMenuItem* item = menu->FindItem(currentProfile);
		if (item != NULL) {
			// TODO: translate
			BString label(item->Label());
			label << " (current)";
			item->SetLabel(label.String());
			item->SetMarked(true);
		}
	}
}


void
NetworkSetupWindow::_BuildShowTabView()
{
	BPath path;
	BPath addOnPath;
	BDirectory dir;
	BEntry entry;

	char* searchPaths = getenv("ADDON_PATH");
	if (!searchPaths)
		return;

	searchPaths = strdup(searchPaths);
	char* nextPathToken;
	char* searchPath = strtok_r(searchPaths, ":", &nextPathToken);

	while (searchPath) {
		if (strncmp(searchPath, "%A/", 3) == 0) {
			app_info ai;
			be_app->GetAppInfo(&ai);
			entry.SetTo(&ai.ref);
			entry.GetPath(&path);
			path.GetParent(&path);
			path.Append(searchPath + 3);
		} else {
			path.SetTo(searchPath);
			path.Append("Network Setup");
		}

		searchPath = strtok_r(NULL, ":", &nextPathToken);

		dir.SetTo(path.Path());
		if (dir.InitCheck() != B_OK)
			continue;

		dir.Rewind();
		while (dir.GetNextEntry(&entry) >= 0) {
			if (entry.IsDirectory())
				continue;

			entry.GetPath(&addOnPath);
			image_id image = load_add_on(addOnPath.Path());
			if (image < 0) {
				printf("Failed to load %s addon: %s.\n", addOnPath.Path(),
					strerror(image));
				continue;
			}

			network_setup_addon_instantiate get_nth_addon;
			status_t status = get_image_symbol(image, "get_nth_addon",
				B_SYMBOL_TYPE_TEXT, (void **) &get_nth_addon);

			int tabCount = 0;

			if (status != B_OK) {
				//  No "addon instantiate function" symbol found in this addon
				printf("No symbol \"get_nth_addon\" found in %s addon: not a "
					"network setup addon!\n", addOnPath.Path());
				unload_add_on(image);
				continue;
			}

			while ((fNetworkAddOnMap[fAddOnCount]
					= get_nth_addon(image, tabCount)) != NULL) {
				printf("Adding Tab: %d\n", fAddOnCount);
				BView* view = fNetworkAddOnMap[fAddOnCount]->CreateView();

				// FIXME rework this: we don't want to use a tab view here,
				// instead add-ons should populate the "interfaces" list with
				// interfaces, services, etc.
				fPanel->AddTab(view);
				fAddOnCount++;
					// Number of tab addons total
				tabCount++;
					// Tabs for *this* addon
			}
		}
	}

	free(searchPaths);
}


void
NetworkSetupWindow::_ShowReplicant(bool show)
{
	if (show) {
		const char* argv[] = {"--deskbar", NULL};

		status_t status = be_roster->Launch(kNetworkStatusSignature, 1, argv);
		if (status != B_OK) {
			BString errorMessage;
			errorMessage.SetToFormat(
				B_TRANSLATE("Installing NetworkStatus in Deskbar failed: %s"),
				strerror(status));
			BAlert* alert = new BAlert(B_TRANSLATE("launch error"),
				errorMessage, B_TRANSLATE("Ok"));
			alert->Go(NULL);
		}
	} else {
		BDeskbar deskbar;
		deskbar.RemoveItem("NetworkStatus");
	}
}


bool
NetworkSetupWindow::_IsReplicantInstalled()
{
	BDeskbar deskbar;
	return deskbar.HasItem("NetworkStatus");
}
