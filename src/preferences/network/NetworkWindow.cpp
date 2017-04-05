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
#include <NetworkDevice.h>
#include <NetworkInterface.h>
#include <NetworkNotifications.h>
#include <NetworkRoster.h>
#include <OutlineListView.h>
#include <Path.h>
#include <PathFinder.h>
#include <PathMonitor.h>
#include <Roster.h>
#include <ScrollView.h>
#include <StringItem.h>
#include <SymLink.h>

#define ENABLE_PROFILES 0
#if ENABLE_PROFILES
#	include <PopUpMenu.h>
#endif

#include "InterfaceListItem.h"
#include "InterfaceView.h"
#include "ServiceListItem.h"


const char* kNetworkStatusSignature = "application/x-vnd.Haiku-NetworkStatus";

static const uint32 kMsgProfileSelected = 'prof';
static const uint32 kMsgProfileManage = 'mngp';
static const uint32 kMsgProfileNew = 'newp';
static const uint32 kMsgRevert = 'rvrt';
static const uint32 kMsgToggleReplicant = 'trep';
static const uint32 kMsgItemSelected = 'ItSl';

BMessenger gNetworkWindow;


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT	"NetworkWindow"


class TitleItem : public BStringItem {
public:
	TitleItem(const char* title)
		:
		BStringItem(title)
	{
	}

	void DrawItem(BView* owner, BRect bounds, bool complete)
	{
		owner->SetFont(be_bold_font);
		BStringItem::DrawItem(owner, bounds, complete);
		owner->SetFont(be_plain_font);
	}

	void Update(BView* owner, const BFont* font)
	{
		BStringItem::Update(owner, be_bold_font);
	}
};


// #pragma mark -


NetworkWindow::NetworkWindow()
	:
	BWindow(BRect(100, 100, 400, 400), B_TRANSLATE("Network"), B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS),
	fServicesItem(NULL),
	fDialUpItem(NULL),
	fVPNItem(NULL),
	fOtherItem(NULL)
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
	scrollView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fAddOnShellView = new BView("add-on shell", 0,
		new BGroupLayout(B_VERTICAL));
	fAddOnShellView->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	fInterfaceView = new InterfaceView();

	// Build the layout
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_WINDOW_SPACING)

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

	gNetworkWindow = this;

	_ScanInterfaces();
	_ScanAddOns();
	_UpdateRevertButton();

	fListView->Select(0);
	_SelectItem(fListView->ItemAt(0));
		// Call this manually, so that CenterOnScreen() below already
		// knows the final window size.

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

		case kMsgSettingsItemUpdated:
			// TODO: update list item
			_UpdateRevertButton();
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

		BNetworkDevice device(interface.Name());
		BNetworkInterfaceType type = B_NETWORK_INTERFACE_TYPE_OTHER;

		if (device.IsWireless())
			type = B_NETWORK_INTERFACE_TYPE_WIFI;
		else if (device.IsEthernet())
			type = B_NETWORK_INTERFACE_TYPE_ETHERNET;

		InterfaceListItem* item = new InterfaceListItem(interface.Name(), type);
		item->SetExpanded(true);

		fInterfaceItemMap.insert(std::pair<BString, InterfaceListItem*>(
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

	// Collect add-on paths by name, so that each name will only be
	// loaded once.
	typedef std::map<BString, BPath> PathMap;
	PathMap addOnMap;

	for (int32 i = 0; i < paths.CountStrings(); i++) {
		BDirectory directory(paths.StringAt(i));
		BEntry entry;
		while (directory.GetNextEntry(&entry) == B_OK) {
			BPath path;
			if (entry.GetPath(&path) != B_OK)
				continue;

			if (addOnMap.find(path.Leaf()) == addOnMap.end())
				addOnMap.insert(std::pair<BString, BPath>(path.Leaf(), path));
		}
	}

	for (PathMap::const_iterator addOnIterator = addOnMap.begin();
			addOnIterator != addOnMap.end(); addOnIterator++) {
		const BPath& path = addOnIterator->second;

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
			printf("No symbol \"instantiate_network_settings_add_on\" found "
				"in %s addon: not a network setup addon!\n", path.Path());
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
				fListView->AddUnder(item->ListItem(), interfaceItem);
			}
			fListView->SortItemsUnder(interfaceItem, true,
				NetworkWindow::_CompareListItems);
		}

		// Generic items
		uint32 cookie = 0;
		while (true) {
			BNetworkSettingsItem* item = addOn->CreateNextItem(cookie);
			if (item == NULL)
				break;

			fSettingsMap[item->ListItem()] = item;
			fListView->AddUnder(item->ListItem(),
				_ListItemFor(item->Type()));
		}

		_SortItemsUnder(fServicesItem);
		_SortItemsUnder(fDialUpItem);
		_SortItemsUnder(fVPNItem);
		_SortItemsUnder(fOtherItem);
	}

	fListView->SortItemsUnder(NULL, true,
		NetworkWindow::_CompareTopLevelListItems);
}


BNetworkSettingsItem*
NetworkWindow::_SettingsItemFor(BListItem* item)
{
	SettingsMap::const_iterator found = fSettingsMap.find(item);
	if (found != fSettingsMap.end())
		return found->second;

	return NULL;
}


void
NetworkWindow::_SortItemsUnder(BListItem* item)
{
	if (item != NULL)
		fListView->SortItemsUnder(item, true, NetworkWindow::_CompareListItems);
}


BListItem*
NetworkWindow::_ListItemFor(BNetworkSettingsType type)
{
	switch (type) {
		case B_NETWORK_SETTINGS_TYPE_SERVICE:
			if (fServicesItem == NULL)
				fServicesItem = _CreateItem(B_TRANSLATE("Services"));
			return fServicesItem;

		case B_NETWORK_SETTINGS_TYPE_DIAL_UP:
			if (fDialUpItem == NULL)
				fDialUpItem = _CreateItem(B_TRANSLATE("Dial Up"));
			return fDialUpItem;

		case B_NETWORK_SETTINGS_TYPE_VPN:
			if (fVPNItem == NULL)
				fVPNItem = _CreateItem(B_TRANSLATE("VPN"));
			return fVPNItem;

		case B_NETWORK_SETTINGS_TYPE_OTHER:
			if (fOtherItem == NULL)
				fOtherItem = _CreateItem(B_TRANSLATE("Other"));
			return fOtherItem;

		default:
			return NULL;
	}
}


BListItem*
NetworkWindow::_CreateItem(const char* label)
{
	BListItem* item = new TitleItem(label);
	item->SetExpanded(true);
	fListView->AddItem(item);
	return item;
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
	for (int32 index = 0; index < fListView->FullListCountItems(); index++) {
		BNetworkSettingsListener* listener
			= dynamic_cast<BNetworkSettingsListener*>(
				fListView->FullListItemAt(index));
		if (listener != NULL)
			listener->SettingsUpdated(type);
	}

	SettingsMap::const_iterator iterator = fSettingsMap.begin();
	for (; iterator != fSettingsMap.end(); iterator++)
		iterator->second->SettingsUpdated(type);

	_UpdateRevertButton();
}


void
NetworkWindow::_BroadcastConfigurationUpdate(const BMessage& message)
{
	for (int32 index = 0; index < fListView->FullListCountItems(); index++) {
		BNetworkConfigurationListener* listener
			= dynamic_cast<BNetworkConfigurationListener*>(
				fListView->FullListItemAt(index));
		if (listener != NULL)
			listener->ConfigurationUpdated(message);
	}

	SettingsMap::const_iterator iterator = fSettingsMap.begin();
	for (; iterator != fSettingsMap.end(); iterator++)
		iterator->second->ConfigurationUpdated(message);

	// TODO: improve invalidated region to the one that matters
	fListView->Invalidate();
	_UpdateRevertButton();
}


void
NetworkWindow::_UpdateRevertButton()
{
	bool enabled = false;
	SettingsMap::const_iterator iterator = fSettingsMap.begin();
	for (; iterator != fSettingsMap.end(); iterator++) {
		if (iterator->second->IsRevertable()) {
			enabled = true;
			break;
		}
	}

	fRevertButton->SetEnabled(enabled);
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


/*static*/ const char*
NetworkWindow::_ItemName(const BListItem* item)
{
	if (const BNetworkInterfaceListItem* listItem = dynamic_cast<
			const BNetworkInterfaceListItem*>(item))
		return listItem->Label();

	if (const ServiceListItem* listItem = dynamic_cast<
			const ServiceListItem*>(item))
		return listItem->Label();

	if (const BStringItem* stringItem = dynamic_cast<const BStringItem*>(item))
		return stringItem->Text();

	return NULL;
}


/*static*/ int
NetworkWindow::_CompareTopLevelListItems(const BListItem* a, const BListItem* b)
{
	if (a == b)
		return 0;

	if (const InterfaceListItem* itemA
			= dynamic_cast<const InterfaceListItem*>(a)) {
		if (const InterfaceListItem* itemB
				= dynamic_cast<const InterfaceListItem*>(b)) {
			return strcasecmp(itemA->Name(), itemB->Name());
		}
		return -1;
	} else if (dynamic_cast<const InterfaceListItem*>(b) != NULL)
		return 1;
/*
	if (a == fDialUpItem)
		return -1;
	if (b == fDialUpItem)
		return 1;

	if (a == fServicesItem)
		return -1;
	if (b == fServicesItem)
		return 1;
*/
	return _CompareListItems(a, b);
}


/*static*/ int
NetworkWindow::_CompareListItems(const BListItem* a, const BListItem* b)
{
	if (a == b)
		return 0;

	const char* nameA = _ItemName(a);
	const char* nameB = _ItemName(b);

	if (nameA != NULL && nameB != NULL)
		return strcasecmp(nameA, nameB);
	if (nameA != NULL)
		return 1;
	if (nameB != NULL)
		return -1;

	return (addr_t)a > (addr_t)b ? 1 : -1;
}
