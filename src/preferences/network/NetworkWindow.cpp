/*
 * Copyright 2004-2015 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 *	Authors:
 *		Adrien Destugues, <pulkomandy@pulkomandy.tk>
 *		Axel Dörfler, <axeld@pinc-software.de>
 *		Alexander von Gluck, <kallisti5@unixzen.com>
 */


#include "NetworkWindow.h"

#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Alert.h>
#include <Application.h>
#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <ControlLook.h>
#include <Deskbar.h>
#include <Directory.h>
#include <LayoutBuilder.h>
#include <NetworkInterface.h>
#include <NetworkNotifications.h>
#include <NetworkRoster.h>
#include <OutlineListView.h>
#include <Path.h>
#include <PathFinder.h>
#include <PathMonitor.h>
#include <Roster.h>
#include <ScrollView.h>
#include <SymLink.h>

#define ENABLE_PROFILES 0
#if ENABLE_PROFILES
#	include <PopUpMenu.h>
#endif

#include "InterfaceListItem.h"
#include "InterfaceView.h"


const char* kNetworkStatusSignature = "application/x-vnd.Haiku-NetworkStatus";

static const uint32 kMsgProfileSelected = 'prof';
static const uint32 kMsgProfileManage = 'mngp';
static const uint32 kMsgProfileNew = 'newp';
static const uint32 kMsgRevert = 'rvrt';
static const uint32 kMsgToggleReplicant = 'trep';
static const uint32 kMsgItemSelected = 'ItSl';


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT	"NetworkWindow"


NetworkWindow::NetworkWindow()
	:
	BWindow(BRect(100, 100, 400, 400), B_TRANSLATE("Network"), B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS)
{
	// Profiles section
#if ENABLE_PROFILES
	BPopUpMenu* profilesPopup = new BPopUpMenu("<none>");
	_BuildProfilesMenu(profilesPopup, kMsgProfileSelected);

	BMenuField* profilesMenuField = new BMenuField("profiles_menu",
		B_TRANSLATE("Profile:"), profilesPopup);

	profilesMenuField->SetFont(be_bold_font);
	profilesMenuField->SetEnabled(false);
#endif

	// Settings section

	fRevertButton = new BButton("revert", B_TRANSLATE("Revert"),
		new BMessage(kMsgRevert));
	// fRevertButton->SetEnabled(false);

	BMessage* message = new BMessage(kMsgToggleReplicant);
	BCheckBox* showReplicantCheckBox = new BCheckBox("showReplicantCheckBox",
		B_TRANSLATE("Show network status in Deskbar"), message);
	showReplicantCheckBox->SetExplicitMaxSize(
		BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	showReplicantCheckBox->SetValue(_IsReplicantInstalled());

	fListView = new BOutlineListView("list", B_SINGLE_SELECTION_LIST,
		B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_FRAME_EVENTS | B_NAVIGABLE);
	fListView->SetSelectionMessage(new BMessage(kMsgItemSelected));

	BScrollView* scrollView = new BScrollView("ScrollView", fListView,
		0, false, true);

	fAddOnShellView = new BView("add-on shell", 0,
		new BGroupLayout(B_VERTICAL));
	fAddOnShellView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fInterfaceView = new InterfaceView();

	// Build the layout
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING)

#if ENABLE_PROFILES
		.AddGroup(B_HORIZONTAL, B_USE_SMALL_SPACING)
			.Add(profilesMenuField)
			.AddGlue()
		.End()
#endif
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.Add(scrollView)
			.Add(fAddOnShellView)
			.End()
		.Add(showReplicantCheckBox)
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.Add(fRevertButton)
			.AddGlue()
		.End();

	_ScanInterfaces();
	_ScanAddOns();

	// Set size of the list view from its contents
	float width;
	float height;
	fListView->GetPreferredSize(&width, &height);
	width += 2 * be_control_look->DefaultItemSpacing();
	fListView->SetExplicitSize(BSize(width, B_SIZE_UNSET));
	fListView->SetExplicitMinSize(BSize(width, std::min(height, 400.f)));

	CenterOnScreen();

	fSettings.StartMonitoring(this);
	start_watching_network(B_WATCH_NETWORK_INTERFACE_CHANGES
		| B_WATCH_NETWORK_LINK_CHANGES | B_WATCH_NETWORK_WLAN_CHANGES, this);
}


NetworkWindow::~NetworkWindow()
{
	stop_watching_network(this);
	fSettings.StopMonitoring(this);
}


bool
NetworkWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


void
NetworkWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgProfileNew:
			break;

		case kMsgProfileSelected:
		{
			const char* path;
			if (message->FindString("path", &path) != B_OK)
				break;

			// TODO!
			break;
		}

		case kMsgItemSelected:
		{
			BListItem* listItem = fListView->FullListItemAt(
				fListView->FullListCurrentSelection());
			if (listItem == NULL)
				break;

			_SelectItem(listItem);
			break;
		}

		case kMsgRevert:
		{
			SettingsMap::const_iterator iterator = fSettingsMap.begin();
			for (; iterator != fSettingsMap.end(); iterator++)
				iterator->second->Revert();
			break;
		}

		case kMsgToggleReplicant:
		{
			_ShowReplicant(
				message->GetInt32("be:value", B_CONTROL_OFF) == B_CONTROL_ON);
			break;
		}

		case B_PATH_MONITOR:
		{
			fSettings.Update(message);
			break;
		}

		case B_NETWORK_MONITOR:
			_BroadcastConfigurationUpdate(*message);
			break;

		case BNetworkSettings::kMsgInterfaceSettingsUpdated:
		case BNetworkSettings::kMsgNetworkSettingsUpdated:
		case BNetworkSettings::kMsgServiceSettingsUpdated:
			_BroadcastSettingsUpdate(message->what);
			break;

		default:
			inherited::MessageReceived(message);
	}
}


void
NetworkWindow::_BuildProfilesMenu(BMenu* menu, int32 what)
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
NetworkWindow::_ScanInterfaces()
{
	// Try existing devices first
	BNetworkRoster& roster = BNetworkRoster::Default();
	BNetworkInterface interface;
	uint32 cookie = 0;

	while (roster.GetNextInterface(&cookie, interface) == B_OK) {
		if ((interface.Flags() & IFF_LOOPBACK) != 0)
			continue;

		InterfaceListItem* item = new InterfaceListItem(interface.Name());
		item->SetExpanded(true);

		fInterfaceItemMap.insert(std::pair<BString, BListItem*>(
			BString(interface.Name()), item));
		fListView->AddItem(item);
	}

	// TODO: Then consider those from the settings (for example, for USB)
}


void
NetworkWindow::_ScanAddOns()
{
	BStringList paths;
	BPathFinder::FindPaths(B_FIND_PATH_ADD_ONS_DIRECTORY, "Network Settings",
		paths);

	for (int32 i = 0; i < paths.CountStrings(); i++) {
		BDirectory directory(paths.StringAt(i));
		BEntry entry;
		while (directory.GetNextEntry(&entry) == B_OK) {
			BPath path;
			if (entry.GetPath(&path) != B_OK)
				continue;

			image_id image = load_add_on(path.Path());
			if (image < 0) {
				printf("Failed to load %s addon: %s.\n", path.Path(),
					strerror(image));
				continue;
			}

			BNetworkSettingsAddOn* (*instantiateAddOn)(image_id image,
				BNetworkSettings& settings);

			status_t status = get_image_symbol(image,
				"instantiate_network_settings_add_on",
				B_SYMBOL_TYPE_TEXT, (void**)&instantiateAddOn);
			if (status != B_OK) {
				// No "addon instantiate function" symbol found in this addon
				printf("No symbol \"instantiate_network_settings_add_on\" "
					"found in %s addon: not a network setup addon!\n",
					path.Path());
				unload_add_on(image);
				continue;
			}

			BNetworkSettingsAddOn* addOn = instantiateAddOn(image, fSettings);
			if (addOn == NULL) {
				unload_add_on(image);
				continue;
			}

			fAddOns.AddItem(addOn);

			// Per interface items
			ItemMap::const_iterator iterator = fInterfaceItemMap.begin();
			for (; iterator != fInterfaceItemMap.end(); iterator++) {
				const BString& interface = iterator->first;
				BListItem* interfaceItem = iterator->second;

				uint32 cookie = 0;
				while (true) {
					BNetworkSettingsItem* item = addOn->CreateNextInterfaceItem(
						cookie, interface.String());
					if (item == NULL)
						break;

					fSettingsMap[item->ListItem()] = item;
					// TODO: sort
					fListView->AddUnder(item->ListItem(), interfaceItem);
				}
			}

			// Generic items
			uint32 cookie = 0;
			while (true) {
				BNetworkSettingsItem* item = addOn->CreateNextItem(cookie);
				if (item == NULL)
					break;

				fSettingsMap[item->ListItem()] = item;
				// TODO: sort
				fListView->AddUnder(item->ListItem(),
					_ListItemFor(item->Type()));
			}
		}
	}
}


BNetworkSettingsItem*
NetworkWindow::_SettingsItemFor(BListItem* item)
{
	SettingsMap::const_iterator found = fSettingsMap.find(item);
	if (found != fSettingsMap.end())
		return found->second;

	return NULL;
}


BListItem*
NetworkWindow::_ListItemFor(BNetworkSettingsType type)
{
	switch (type) {
		case B_NETWORK_SETTINGS_TYPE_SERVICE:
			if (fServicesItem == NULL) {
				fServicesItem = new BStringItem(B_TRANSLATE("Services"));
				fServicesItem->SetExpanded(true);
			}

			return fServicesItem;

		case B_NETWORK_SETTINGS_TYPE_DIAL_UP:
			if (fDialUpItem == NULL)
				fDialUpItem = new BStringItem(B_TRANSLATE("Dial Up"));

			return fDialUpItem;

		case B_NETWORK_SETTINGS_TYPE_OTHER:
			if (fOtherItem == NULL)
				fOtherItem = new BStringItem(B_TRANSLATE("Other"));

			return fOtherItem;

		default:
			return NULL;
	}
}


void
NetworkWindow::_SelectItem(BListItem* listItem)
{
	while (fAddOnShellView->CountChildren() > 0)
		fAddOnShellView->ChildAt(0)->RemoveSelf();

	BView* nextView = NULL;

	BNetworkSettingsItem* item = _SettingsItemFor(listItem);
	if (item != NULL) {
		nextView = item->View();
	} else {
		InterfaceListItem* item = dynamic_cast<InterfaceListItem*>(
			listItem);
		if (item != NULL) {
			fInterfaceView->SetTo(item->Name());
			nextView = fInterfaceView;
		}
	}

	if (nextView != NULL)
		fAddOnShellView->AddChild(nextView);
}


void
NetworkWindow::_BroadcastSettingsUpdate(uint32 type)
{
	SettingsMap::const_iterator iterator = fSettingsMap.begin();
	for (; iterator != fSettingsMap.end(); iterator++)
		iterator->second->SettingsUpdated(type);
}


void
NetworkWindow::_BroadcastConfigurationUpdate(const BMessage& message)
{
	SettingsMap::const_iterator iterator = fSettingsMap.begin();
	for (; iterator != fSettingsMap.end(); iterator++)
		iterator->second->ConfigurationUpdated(message);
}


void
NetworkWindow::_ShowReplicant(bool show)
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
NetworkWindow::_IsReplicantInstalled()
{
	BDeskbar deskbar;
	return deskbar.HasItem("NetworkStatus");
}
