//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------

#include "DialUpView.h"
#include "DialUpAddon.h"

#include <cstring>
#include "InterfaceUtils.h"
#include "MessageDriverSettingsUtils.h"
#include "TextRequestDialog.h"

// built-in add-ons
#include "GeneralAddon.h"
#include "IPCPAddon.h"
#include "PPPoEAddon.h"
#include "ProtocolsAddon.h"


#include <PPPInterface.h>
#include <TemplateList.h>

#include <Application.h>

#include <Alert.h>
#include <Button.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Messenger.h>
#include <PopUpMenu.h>
#include <StringView.h>
#include <TabView.h>

#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <Path.h>


#define MSG_CREATE_NEW				'NEWI'
#define MSG_FINISH_CREATE_NEW		'FNEW'
#define MSG_DELETE_CURRENT			'DELI'
#define MSG_SELECT_INTERFACE		'SELI'
#define MSG_CONNECT_BUTTON			'CONI'

#define LABEL_CREATE_NEW			"Create New..."
#define LABEL_DELETE_CURRENT		"Delete Current"

#define LABEL_CONNECT				"Connect"
#define LABEL_DISCONNECT			"Disconnect"

#define TEXT_CONNECTING				"Connecting..."
#define TEXT_CONNECTION_ESTABLISHED	"Connection established."
#define TEXT_NOT_CONNECTED			"Not connected."
#define TEXT_DEVICE_ERROR			"Device error!"
#define TEXT_AUTHENTICATING			"Authenticating..."
#define TEXT_AUTHENTICATION_FAILED	"Authentication failed!"
#define TEXT_CONNECTION_LOST		"Connection lost!"
#define TEXT_CREATION_ERROR			"Error creating interface!"
#define TEXT_NO_INTERFACE_SELECTED	"No interface selected..."
#define TEXT_OK						"OK"
#define ERROR_TITLE					"Error"
#define ERROR_NO_PPP_STACK			"Error: Could not find the PPP stack!"
#define ERROR_INTERFACE_EXISTS		"Error: An interface with this name already " \
										"exists!"
#define ERROR_LOADING_FAILED		"Error: Failed loading interface! The current " \
										"settings will be deleted."


static
status_t
up_down_thread(void *data)
{
	static_cast<DialUpView*>(data)->UpDownThread();
	return B_OK;
}


DialUpView::DialUpView(BRect frame)
	: BView(frame, "DialUpView", B_FOLLOW_NONE, 0),
	fListener(this),
	fUpDownThread(-1),
	fDriverSettings(NULL),
	fCurrentItem(NULL),
	fWatching(PPP_UNDEFINED_INTERFACE_ID),
	fKeepLabel(false)
{
	BRect bounds = Bounds();
		// for caching
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	// add messenger to us so add-ons can contact us
	BMessenger messenger(this);
	fAddons.AddMessenger(DUN_MESSENGER, messenger);
	
	// create pop-up with all interfaces and "New..."/"Delete current" items
	fInterfaceMenu = new BPopUpMenu(LABEL_CREATE_NEW);
	BRect rect = bounds;
	rect.InsetBy(5, 5);
	rect.bottom = rect.top + 20;
	fMenuField = new BMenuField(rect, "Interfaces", "Interface:", fInterfaceMenu);
	fMenuField->SetDivider(StringWidth(fMenuField->Label()) + 10);
	
	rect.top = rect.bottom + 10;
	rect.bottom = bounds.bottom
		- 20 // height of bottom controls
		- 20; // space for bottom controls
	fTabView = new BTabView(rect, "TabView");
	BRect tabViewRect = fTabView->Bounds();
	tabViewRect.bottom -= fTabView->TabHeight();
	fAddons.AddRect(DUN_TAB_VIEW_RECT, tabViewRect);
	
	rect.top = rect.bottom + 15;
	rect.bottom = rect.top + 15;
	rect.right = rect.left + 200;
	fStatusView = new BStringView(rect, "StatusView", TEXT_NOT_CONNECTED);
	
	rect.InsetBy(0, -5);
	rect.left = rect.right + 5;
	rect.right = bounds.right - 5;
	fConnectButton = new BButton(rect, "ConnectButton", LABEL_CONNECT,
		new BMessage(MSG_CONNECT_BUTTON));
	
	AddChild(fMenuField);
	AddChild(fTabView);
	AddChild(fStatusView);
	AddChild(fConnectButton);
	
	// initialize
	LoadInterfaces();
	LoadAddons();
	CreateTabs();
	fCurrentItem = NULL;
		// reset, otherwise SelectInterface will not load the settings
	SelectInterface(0);
}


DialUpView::~DialUpView()
{
	SaveSettingsToFile();
	
	int32 tmp;
	wait_for_thread(fUpDownThread, &tmp);
	
	// free known add-on types (these should free their known add-on types, etc.)
	DialUpAddon *addon;
	for(int32 index = 0;
			fAddons.FindPointer(DUN_DELETE_ON_QUIT, index,
				reinterpret_cast<void**>(&addon)) == B_OK;
			index++)
		delete addon;
}


void
DialUpView::AttachedToWindow()
{
	fInterfaceMenu->SetTargetForItems(this);
	fConnectButton->SetTarget(this);
	
	if(fListener.InitCheck() != B_OK)
		(new BAlert(ERROR_TITLE, ERROR_NO_PPP_STACK, TEXT_OK))->Go(NULL);
}


void
DialUpView::MessageReceived(BMessage *message)
{
	switch(message->what) {
		case PPP_REPORT_MESSAGE:
			HandleReportMessage(message);
		break;
		
		// -------------------------------------------------
		case MSG_CREATE_NEW: {
			(new TextRequestDialog("New Interface", "Interface Name: "))->Go(
				new BInvoker(new BMessage(MSG_FINISH_CREATE_NEW), this));
		} break;
		
		case MSG_FINISH_CREATE_NEW: {
			int32 which;
			message->FindInt32("which", &which);
			const char *name = message->FindString("text");
			if(which == 1 && name && strlen(name) > 0)
				AddInterface(name, true);
			
			if(fCurrentItem)
				fCurrentItem->SetMarked(true);
		} break;
		// -------------------------------------------------
		
		case MSG_DELETE_CURRENT: {
			fInterfaceMenu->RemoveItem(fCurrentItem);
			BDirectory settings, profile;
			GetPPPDirectories(&settings, &profile);
			BEntry entry;
			settings.FindEntry(fCurrentItem->Label(), &entry);
			entry.Remove();
			profile.FindEntry(fCurrentItem->Label(), &entry);
			entry.Remove();
			delete fCurrentItem;
			fCurrentItem = NULL;
			
			if(CountInterfaces() == 0)
				fInterfaceMenu->SetRadioMode(false);
			else
				SelectInterface(0);
		} break;
		
		case MSG_SELECT_INTERFACE: {
			int32 index;
			message->FindInt32("index", &index);
			SelectInterface(index);
		} break;
		
		case MSG_CONNECT_BUTTON: {
			if(!fCurrentItem || fUpDownThread != -1)
				return;
			
			fUpDownThread = spawn_thread(up_down_thread, "up_down_thread",
				B_NORMAL_PRIORITY, this);
			resume_thread(fUpDownThread);
		} break;
		
		default:
			BView::MessageReceived(message);
	}
}


bool
DialUpView::LoadSettings(bool isNew)
{
	fSettings.MakeEmpty();
	fProfile.MakeEmpty();
	BMessage *settingsPointer = fCurrentItem ? &fSettings : NULL,
		*profilePointer = fCurrentItem ? &fProfile : NULL;
	
	if(fCurrentItem && !isNew) {
		BString name("pppidf/");
		name << fCurrentItem->Label();
		if(!ReadMessageDriverSettings(name.String(), &fSettings))
			return false;
		name = "pppidf/profile/";
		name << fCurrentItem->Label();
		if(!ReadMessageDriverSettings(name.String(), &fProfile))
			profilePointer = settingsPointer;
	}
	
	DialUpAddon *addon;
	for(int32 index = 0; fAddons.FindPointer(DUN_TAB_ADDON_TYPE, index,
			reinterpret_cast<void**>(&addon)) == B_OK; index++) {
		if(!addon)
			continue;
		
		if(!addon->LoadSettings(settingsPointer, profilePointer, isNew))
			return false;
	}
	
	// TODO: check if settings are valid
	
	return true;
}


void
DialUpView::IsModified(bool *settings, bool *profile)
{
	*settings = *profile = false;
	bool addonSettingsChanged, addonProfileChanged;
		// for current addon
	
	DialUpAddon *addon;
	for(int32 index = 0; fAddons.FindPointer(DUN_TAB_ADDON_TYPE, index,
			reinterpret_cast<void**>(&addon)) == B_OK; index++) {
		if(!addon)
			continue;
		
		addon->IsModified(&addonSettingsChanged, &addonProfileChanged);
		if(addonSettingsChanged)
			*settings = true;
		if(addonProfileChanged)
			*profile = true;
	}
}


bool
DialUpView::SaveSettings(BMessage *settings, BMessage *profile, bool saveTemporary)
{
	if(!fCurrentItem || !settings || !profile)
		return false;
	
	DialUpAddon *addon;
	TemplateList<DialUpAddon*> addons;
	for(int32 index = 0;
			fAddons.FindPointer(DUN_TAB_ADDON_TYPE, index,
				reinterpret_cast<void**>(&addon)) == B_OK; index++) {
		if(!addon)
			continue;
		
		int32 insertIndex = 0;
		for(; insertIndex < addons.CountItems(); insertIndex++)
			if(addons.ItemAt(insertIndex)->Priority() <= addon->Priority())
				break;
		
		addons.AddItem(addon, insertIndex);
	}
	
	settings->AddInt32("Interface", static_cast<int32>(fWatching));
	if(fCurrentItem)
		settings->AddString("InterfaceName", fCurrentItem->Label());
	
	for(int32 index = 0; index < addons.CountItems(); index++)
		addons.ItemAt(index)->SaveSettings(settings, profile, saveTemporary);
	
	return true;
}


bool
DialUpView::SaveSettingsToFile()
{
	bool settingsChanged, profileChanged;
	IsModified(&settingsChanged, &profileChanged);
	if(!settingsChanged && !profileChanged)
		return true;
	
	BMessage settings, profile;
	if(!SaveSettings(&settings, &profile, false))
		return false;
	
	BDirectory settingsDirectory;
	BDirectory profileDirectory;
	GetPPPDirectories(&settingsDirectory, &profileDirectory);
	if(settingsDirectory.InitCheck() != B_OK || profileDirectory.InitCheck() != B_OK)
		return false;
	
	BFile file;
	if(settingsChanged) {
		settingsDirectory.CreateFile(fCurrentItem->Label(), &file);
		WriteMessageDriverSettings(file, settings);
	}
	
	if(profileChanged) {
		profileDirectory.CreateFile(fCurrentItem->Label(), &file);
		WriteMessageDriverSettings(file, profile);
	}
	
	return true;
}


void
DialUpView::UpDownThread()
{
	PPPInterface interface;
	if(fWatching == PPP_UNDEFINED_INTERFACE_ID) {
		interface = fListener.Manager().InterfaceWithName(fCurrentItem->Label());
		if(interface.InitCheck() != B_OK)
			interface = fListener.Manager().CreateInterfaceWithName(
				fCurrentItem->Label());
	} else
		interface = fWatching;
	
	if(interface.InitCheck() != B_OK) {
		Window()->Lock();
		fStatusView->SetText(TEXT_CREATION_ERROR);
		Window()->Unlock();
		return;
	}
	
	ppp_interface_info_t info;
	interface.GetInterfaceInfo(&info);
	if(info.info.phase == PPP_DOWN_PHASE)
		interface.Up();
	else
		interface.Down();
	
	fUpDownThread = -1;
}


void
DialUpView::GetPPPDirectories(BDirectory *settingsDirectory,
	BDirectory *profileDirectory) const
{
	if(settingsDirectory) {
		BDirectory settings(PPP_INTERFACE_SETTINGS_PATH);
		if(settings.InitCheck() != B_OK) {
			create_directory(PPP_INTERFACE_SETTINGS_PATH, 0750);
			settings.SetTo(PPP_INTERFACE_SETTINGS_PATH);
		}
		
		*settingsDirectory = settings;
	}
	
	if(profileDirectory) {
		BDirectory profile(PPP_INTERFACE_SETTINGS_PATH "/profile");
		if(profile.InitCheck() != B_OK) {
			create_directory(PPP_INTERFACE_SETTINGS_PATH "/profile", 0750);
			profile.SetTo(PPP_INTERFACE_SETTINGS_PATH "/profile");
		}
		
		*profileDirectory = profile;
	}
}


void
DialUpView::HandleReportMessage(BMessage *message)
{
	thread_id sender;
	message->FindInt32("sender", &sender);
	
	send_data(sender, B_OK, NULL, 0);
	
	if(!fCurrentItem)
		return;
	
	ppp_interface_id id;
	if(message->FindInt32("interface", reinterpret_cast<int32*>(&id)) != B_OK
			|| (fWatching != PPP_UNDEFINED_INTERFACE_ID && id != fWatching))
		return;
	
	int32 type, code;
	message->FindInt32("type", &type);
	message->FindInt32("code", &code);
	
	if(type == PPP_MANAGER_REPORT && code == PPP_REPORT_INTERFACE_CREATED) {
		PPPInterface interface(id);
		if(interface.InitCheck() != B_OK)
			return;
		
		ppp_interface_info_t info;
		interface.GetInterfaceInfo(&info);
		if(strcasecmp(info.info.name, fCurrentItem->Label()))
			return;
		
		WatchInterface(id);
	} else if(type == PPP_CONNECTION_REPORT) {
		if(fWatching == PPP_UNDEFINED_INTERFACE_ID)
			return;
		
		UpdateStatus(code);
	} else if(type == PPP_DESTRUCTION_REPORT) {
		if(fWatching == PPP_UNDEFINED_INTERFACE_ID)
			return;
		
		WatchInterface(fListener.Manager().InterfaceWithName(fCurrentItem->Label()));
	}
}


void
DialUpView::CreateTabs()
{
	// create tabs for all registered and valid tab add-ons
	DialUpAddon *addon;
	BView *target;
	float width, height;
	TemplateList<DialUpAddon*> addons;
	
	for(int32 index = 0;
			fAddons.FindPointer(DUN_TAB_ADDON_TYPE, index,
				reinterpret_cast<void**>(&addon)) == B_OK;
			index++) {
		if(!addon || addon->Position() < 0)
			continue;
		
		int32 insertIndex = 0;
		for(; insertIndex < addons.CountItems(); insertIndex++)
			if(addons.ItemAt(insertIndex)->Position() > addon->Position())
				break;
		
		addons.AddItem(addon, insertIndex);
	}
	
	for(int32 index = 0; index < addons.CountItems(); index++) {
		addon = addons.ItemAt(index);
		
		if(!addon->GetPreferredSize(&width, &height))
			continue;
		
		target = addon->CreateView(BPoint(0,0));
		if(!target)
			continue;
		
		fTabView->AddTab(target, NULL);
	}
}


void
DialUpView::UpdateStatus(int32 code)
{
	switch(code) {
		case PPP_REPORT_UP_ABORTED:
		case PPP_REPORT_DOWN_SUCCESSFUL:
		case PPP_REPORT_CONNECTION_LOST: {
			fConnectButton->SetLabel(LABEL_CONNECT);
		} break;
		
		default:
			fConnectButton->SetLabel(LABEL_DISCONNECT);
	}
	
	// maybe the information string must stay
	if(fKeepLabel && code != PPP_REPORT_GOING_UP && code != PPP_REPORT_UP_SUCCESSFUL)
		return;
	
	switch(code) {
		case PPP_REPORT_GOING_UP:
			fKeepLabel = false;
			fStatusView->SetText(TEXT_CONNECTING);
		break;
		
		case PPP_REPORT_UP_SUCCESSFUL:
			fKeepLabel = false;
			fStatusView->SetText(TEXT_CONNECTION_ESTABLISHED);
		break;
		
		case PPP_REPORT_UP_ABORTED:
		case PPP_REPORT_DOWN_SUCCESSFUL:
			fStatusView->SetText(TEXT_NOT_CONNECTED);
		break;
		
		case PPP_REPORT_LOCAL_AUTHENTICATION_REQUESTED:
		case PPP_REPORT_PEER_AUTHENTICATION_REQUESTED:
			fStatusView->SetText(TEXT_AUTHENTICATING);
		break;
		
		case PPP_REPORT_LOCAL_AUTHENTICATION_FAILED:
		case PPP_REPORT_PEER_AUTHENTICATION_FAILED:
			fKeepLabel = true;
			fStatusView->SetText(TEXT_AUTHENTICATION_FAILED);
		break;
		
		case PPP_REPORT_CONNECTION_LOST:
			fKeepLabel = true;
			fStatusView->SetText(TEXT_CONNECTION_LOST);
		break;
	}
}


void
DialUpView::WatchInterface(ppp_interface_id ID)
{
	// This method can be used to update the interface's connection status.
	
	if(fWatching != ID) {
		fListener.StopWatchingInterfaces();
		
		if(ID == PPP_UNDEFINED_INTERFACE_ID || fListener.WatchInterface(ID))
			fWatching = ID;
	}
	
	// update status
	PPPInterface interface(fWatching);
	if(interface.InitCheck() != B_OK) {
		UpdateStatus(PPP_REPORT_DOWN_SUCCESSFUL);
		return;
	}
	
	ppp_interface_info_t info;
	interface.GetInterfaceInfo(&info);
	
	// transform phase into status
	switch(info.info.phase) {
		case PPP_DOWN_PHASE:
			UpdateStatus(PPP_REPORT_DOWN_SUCCESSFUL);
		break;
		
		case PPP_TERMINATION_PHASE:
		break;
		
		case PPP_ESTABLISHED_PHASE:
			UpdateStatus(PPP_REPORT_UP_SUCCESSFUL);
		break;
		
		default:
			UpdateStatus(PPP_REPORT_GOING_UP);
	}
}


void
DialUpView::LoadInterfaces()
{
	fInterfaceMenu->AddSeparatorItem();
	fInterfaceMenu->AddItem(new BMenuItem(LABEL_CREATE_NEW,
		new BMessage(MSG_CREATE_NEW)));
	fInterfaceMenu->AddItem(new BMenuItem(LABEL_DELETE_CURRENT,
		new BMessage(MSG_DELETE_CURRENT)));
	
	BDirectory settingsDirectory;
	BEntry entry;
	BPath path;
	GetPPPDirectories(&settingsDirectory, NULL);
	while(settingsDirectory.GetNextEntry(&entry) == B_OK) {
		if(entry.IsFile()) {
			entry.GetPath(&path);
			AddInterface(path.Leaf(), true);
		}
	}
}


void
DialUpView::LoadAddons()
{
	// Load integrated add-ons:
	// "General" tab
	GeneralAddon *generalAddon = new GeneralAddon(&fAddons);
	fAddons.AddPointer(DUN_TAB_ADDON_TYPE, generalAddon);
	fAddons.AddPointer(DUN_DELETE_ON_QUIT, generalAddon);
	// "IPCP" protocol
	IPCPAddon *ipcpAddon = new IPCPAddon(&fAddons);
	fAddons.AddPointer(DUN_PROTOCOL_ADDON_TYPE, ipcpAddon);
	fAddons.AddPointer(DUN_DELETE_ON_QUIT, ipcpAddon);
	// "PPPoE" device
	PPPoEAddon *pppoeAddon = new PPPoEAddon(&fAddons);
	fAddons.AddPointer(DUN_DEVICE_ADDON_TYPE, pppoeAddon);
	fAddons.AddPointer(DUN_DELETE_ON_QUIT, pppoeAddon);
	// "Protocols" tab
	ProtocolsAddon *protocolsAddon = new ProtocolsAddon(&fAddons);
	fAddons.AddPointer(DUN_TAB_ADDON_TYPE, protocolsAddon);
	fAddons.AddPointer(DUN_DELETE_ON_QUIT, protocolsAddon);
	// "PAP" authenticator
	BMessage addon;
	addon.AddString("KernelModuleName", "pap");
	addon.AddString("FriendlyName", "Plain-text Authentication");
	addon.AddString("TechnicalName", "PAP");
	fAddons.AddMessage(DUN_AUTHENTICATOR_ADDON_TYPE, &addon);
	// addon.MakeEmpty(); // for next authenticator
	
	// TODO:
	// load all add-ons from the add-ons folder
}


void
DialUpView::AddInterface(const char *name, bool isNew = false)
{
	if(fInterfaceMenu->FindItem(name)) {
		(new BAlert(ERROR_TITLE, ERROR_INTERFACE_EXISTS, TEXT_OK))->Go(NULL);
		return;
	}
	
	BMenuItem *item = new BMenuItem(name, new BMessage(MSG_SELECT_INTERFACE));
	item->SetTarget(this);
	int32 index = FindNextMenuInsertionIndex(fInterfaceMenu, name);
	if(index > CountInterfaces())
		index = CountInterfaces();
	fInterfaceMenu->AddItem(item, index);
	if(CountInterfaces() == 1)
		fInterfaceMenu->SetLabelFromMarked(true);
	item->SetMarked(true);
	SelectInterface(index, isNew);
}


void
DialUpView::SelectInterface(int32 index, bool isNew = false)
{
	BMenuItem *item = fInterfaceMenu->FindMarked();
	if(fCurrentItem && item == fCurrentItem)
		return;
	
	SaveSettingsToFile();
	
	if(index >= CountInterfaces() || index < 0) {
		if(CountInterfaces() > 0)
			SelectInterface(0);
		else {
			fCurrentItem = NULL;
			WatchInterface(PPP_UNDEFINED_INTERFACE_ID);
		}
	} else {
		if(!fCurrentItem) {
			fTabView->Show();
			fConnectButton->SetEnabled(true);
		}
		
		fCurrentItem = fInterfaceMenu->ItemAt(index);
		if(!fCurrentItem) {
			SelectInterface(0);
			return;
		}
		
		fCurrentItem->SetMarked(true);
		WatchInterface(fListener.Manager().InterfaceWithName(fCurrentItem->Label()));
	}
	
	if(!fCurrentItem) {
		LoadSettings(false);
			// tell modules to unload all settings
		
		fTabView->Hide();
		fConnectButton->SetEnabled(false);
	} else if(!isNew && !LoadSettings(false)) {
		(new BAlert(ERROR_TITLE, ERROR_LOADING_FAILED, TEXT_OK))->Go(NULL);
		LoadSettings(true);
	} else if(isNew && !LoadSettings(true))
		(new BAlert(ERROR_TITLE, ERROR_LOADING_FAILED, TEXT_OK))->Go(NULL);
}


int32
DialUpView::CountInterfaces() const
{
	return fInterfaceMenu->CountItems() - 3;
}
