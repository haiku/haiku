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


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT	"NetworkSetupWindow"


// --------------------------------------------------------------
NetworkSetupWindow::NetworkSetupWindow(const char *title)
	:
	BWindow(BRect(100, 100, 300, 300), title, B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS),
	fAddonCount(0)
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

	fAddonView = NULL;

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

		case kMsgProfileSelected: {
			BPath name;
			const char *path;
			bool is_default;
			bool is_current;

			if (message->FindString("path", &path) != B_OK)
				break;

			name.SetTo(path);

			is_default = (strcmp(name.Leaf(), "default") == 0);
			is_current = (strcmp(name.Leaf(), "current") == 0);

			fApplyButton->SetEnabled(!is_current);
			break;
		}

		case kMsgRevert: {
			for (int addonIndex = 0; addonIndex < fAddonCount; addonIndex++) {
				NetworkSetupAddOn* addon
					= fNetworkAddOnMap[addonIndex];
				addon->Revert();
			}
			break;
		}

		case kMsgApply: {
			for (int addonIndex = 0; addonIndex < fAddonCount; addonIndex++) {
				NetworkSetupAddOn* addon
					= fNetworkAddOnMap[addonIndex];
				addon->Save();
			}
			break;
		}

		case kMsgToggleReplicant: {
			_ShowReplicant(message->FindInt32("be:value"));
		}

		default:
			inherited::MessageReceived(message);
	}
}


void
NetworkSetupWindow::_BuildProfilesMenu(BMenu* menu, int32 msg_what)
{
	BMenuItem*	item;
	char current_profile[256] = { 0 };

	menu->SetRadioMode(true);

	BDirectory dir("/boot/system/settings/network/profiles");

	if (dir.InitCheck() == B_OK) {
		BEntry entry;
		BMessage* msg;

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

				symlink.ReadLink(current_profile, sizeof(current_profile));
				continue;
			};

			if (!entry.IsDirectory())
				continue;

			msg = new BMessage(msg_what);
			msg->AddString("path", name.Path());

			item = new BMenuItem(name.Leaf(), msg);
			menu->AddItem(item);
		}
	}

	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(B_TRANSLATE("New" B_UTF8_ELLIPSIS),
		new BMessage(kMsgProfileNew)));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Manage" B_UTF8_ELLIPSIS),
		new BMessage(kMsgProfileManage)));

	if (strlen(current_profile)) {
		item = menu->FindItem(current_profile);
		if (item) {
			BString label;
			label << item->Label();
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
	BPath addon_path;
	BDirectory dir;
	BEntry entry;

	char* search_paths = getenv("ADDON_PATH");
	if (!search_paths)
		return;

	search_paths = strdup(search_paths);
	char* next_path_token;
	char* search_path = strtok_r(search_paths, ":", &next_path_token);

	while (search_path) {
		if (strncmp(search_path, "%A/", 3) == 0) {
			app_info ai;
			be_app->GetAppInfo(&ai);
			entry.SetTo(&ai.ref);
			entry.GetPath(&path);
			path.GetParent(&path);
			path.Append(search_path + 3);
		} else {
			path.SetTo(search_path);
			path.Append("Network Setup");
		}

		search_path = strtok_r(NULL, ":", &next_path_token);

		dir.SetTo(path.Path());
		if (dir.InitCheck() != B_OK)
			continue;

		dir.Rewind();
		while (dir.GetNextEntry(&entry) >= 0) {
			if (entry.IsDirectory())
				continue;

			entry.GetPath(&addon_path);
			image_id addon_id = load_add_on(addon_path.Path());
			if (addon_id < 0) {
				printf("Failed to load %s addon: %s.\n", addon_path.Path(),
					strerror(addon_id));
				continue;
			}

			network_setup_addon_instantiate get_nth_addon;
			status_t status = get_image_symbol(addon_id, "get_nth_addon",
				B_SYMBOL_TYPE_TEXT, (void **) &get_nth_addon);

			int tabCount = 0;

			if (status != B_OK) {
				//  No "addon instantiate function" symbol found in this addon
				printf("No symbol \"get_nth_addon\" found in %s addon: not a "
					"network setup addon!\n", addon_path.Path());
				unload_add_on(addon_id);
				continue;
			}

			while ((fNetworkAddOnMap[fAddonCount]
					= get_nth_addon(addon_id, tabCount)) != NULL) {
				printf("Adding Tab: %d\n", fAddonCount);
				BView* addon_view
					= fNetworkAddOnMap[fAddonCount]->CreateView();

				// FIXME rework this: we don't want to use a tab view here,
				// instead add-ons should populate the "interfaces" list with
				// interfaces, services, etc.
				fPanel->AddTab(addon_view);
				fAddonCount++;
					// Number of tab addons total
				tabCount++;
					// Tabs for *this* addon
			}
		}
	}

	free(search_paths);
}


void
NetworkSetupWindow::_ShowReplicant(bool show)
{
	if (show) {
		char* argv[] = {const_cast<char *>("--deskbar"), NULL};

		status_t ret = be_roster->Launch(
			"application/x-vnd.Haiku-NetworkStatus", 1, argv);

		if (ret != B_OK) {
			BString errorMessage;
			errorMessage.SetToFormat(
				B_TRANSLATE("Installing NetworkStatus in Deskbar failed: %s"),
				strerror(ret));
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
