//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------

#include "DialUpView.h"
#include "DialUpAddon.h"

#include "MessageDriverSettingsUtils.h"

#include "GeneralAddon.h"


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

#include <cstring>


#define MSG_CREATE_NEW			'NEWI'
#define MSG_DELETE_CURRENT		'DELI'
#define MSG_SELECT_INTERFACE	'SELI'
#define MSG_CONNECT_BUTTON		'CONI'

#define LABEL_CREATE_NEW			"Create new..."
#define LABEL_DELETE_CURRENT		"Delete current"

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
#define TEXT_OK						"OK"
#define ERROR_TITLE					"Error"
#define ERROR_NO_PPP_STACK			"Error: Could not find the PPP stack!"
#define ERROR_INTERFACE_EXISTS		"Error: An interface with this name already exists!"


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
	fAddons.AddMessenger("Messenger", messenger);
	
	// load integrated add-ons
	fAddons.AddPointer("Tab", new GeneralAddon(&fAddons));
	
	// TODO:
	// load all add-ons
	// register them and check if each one has a kernel module (set fIsValid)
	
	// create pop-up with all interfaces and "New..."/"Delete current" items
	fInterfaceMenu = new BPopUpMenu(LABEL_CREATE_NEW);
	BRect rect = bounds;
	rect.InsetBy(5, 5);
	rect.bottom = rect.top + 20;
	fMenuField = new BMenuField(rect, "Interfaces", "Interface:", fInterfaceMenu);
	fMenuField->SetDivider(StringWidth(fMenuField->Label()) + 10);
	
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
			AddInterface(path.Leaf());
		}
	}
	
	rect.top = rect.bottom + 10;
	rect.bottom = bounds.bottom
		- 20 // height of bottom controls
		- 20; // space for bottom controls
	fTabView = new BTabView(rect, "TabView");
	BRect tabViewRect = fTabView->Bounds();
	tabViewRect.bottom -= fTabView->TabHeight();
	fAddons.AddRect("TabViewRect", tabViewRect);
	
	rect.top = rect.bottom + 15;
	rect.bottom = rect.top + 15;
	rect.right = rect.left + 200;
	fStatusView = new BStringView(rect, "StringView", TEXT_NOT_CONNECTED);
	
	rect.InsetBy(0, -5);
	rect.left = rect.right + 5;
	rect.right = bounds.right - 5;
	fConnectButton = new BButton(rect, "ConnectButton", LABEL_CONNECT,
		new BMessage(MSG_CONNECT_BUTTON));
	
	AddChild(fMenuField);
	AddChild(fTabView);
	AddChild(fStatusView);
	AddChild(fConnectButton);
	
	CreateTabs();
	
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
			fAddons.FindPointer("Tab", index,
				reinterpret_cast<void**>(&addon)) == B_OK;
			index++)
		delete addon;
}


void
DialUpView::AttachedToWindow()
{
	fInterfaceMenu->SetTargetForItems(this);
	fConnectButton->SetTarget(this);
	
	if(fListener.InitCheck() != B_OK) {
		(new BAlert(ERROR_TITLE, ERROR_NO_PPP_STACK, TEXT_OK))->Go();
//		be_app->PostMessage(B_QUIT_REQUESTED);
	}
}


void
DialUpView::MessageReceived(BMessage *message)
{
	switch(message->what) {
		case PPP_REPORT_MESSAGE:
			HandleReportMessage(message);
		break;
		
		case MSG_CREATE_NEW: {
			AddInterface("New interface");
			if(fCurrentItem)
				fCurrentItem->SetMarked(true);
		} break;
		
		case MSG_DELETE_CURRENT: {
			fInterfaceMenu->RemoveItem(fCurrentItem);
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


void
DialUpView::LoadSettings()
{
	BMessage settings;
	
	// TODO: load settings
}


void
DialUpView::IsModified(bool& settings, bool& profile)
{
	settings = profile = false;
	bool addonSettingsChanged, addonProfileChanged;
		// for current addon
	
	DialUpAddon *addon;
	for(int32 index = 0;
			fAddons.FindPointer("Tab", index,
				reinterpret_cast<void**>(&addon)) == B_OK;
			index++) {
		if(!addon || !addon->IsValid())
			continue;
		
		addon->IsModified(addonSettingsChanged, addonProfileChanged);
		if(addonSettingsChanged)
			settings = true;
		if(addonProfileChanged)
			profile = true;
	}
}


bool
DialUpView::SaveSettings(BMessage& settings, BMessage& profile, bool saveModified)
{
	if(!fCurrentItem)
		return false;
	
	// create tabs for all registered and valid "Tab" add-ons
	DialUpAddon *addon;
	TemplateList<DialUpAddon*> addons;
	for(int32 index = 0;
			fAddons.FindPointer("Tab", index,
				reinterpret_cast<void**>(&addon)) == B_OK;
			index++) {
		if(!addon || !addon->IsValid())
			continue;
		
		int32 insertIndex = 0;
		for(; insertIndex < addons.CountItems(); insertIndex++)
			if(addons.ItemAt(insertIndex)->Priority() <= addon->Priority())
				break;
		
		addons.AddItem(addon, insertIndex);
	}
	
	settings.AddInt32("Interface", static_cast<int32>(fWatching));
	if(fCurrentItem)
		settings.AddString("Name", fCurrentItem->Label());
	
	for(int32 index = 0; index < addons.CountItems(); index++)
		addons.ItemAt(index)->SaveSettings(&settings, &profile, saveModified);
	
	return true;
}


bool
DialUpView::SaveSettingsToFile()
{
	bool settingsChanged, profileChanged;
	IsModified(settingsChanged, profileChanged);
	if(!settingsChanged && !profileChanged)
		return true;
	
	BMessage settings, profile;
	if(!SaveSettings(settings, profile, false))
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
	// create tabs for all registered and valid "Tab" add-ons
	DialUpAddon *addon;
	BView *target;
	float width, height;
	TemplateList<DialUpAddon*> addons;
	
	for(int32 index = 0;
			fAddons.FindPointer("Tab", index,
				reinterpret_cast<void**>(&addon)) == B_OK;
			index++) {
		if(!addon || !addon->IsValid() || addon->Position() < 0)
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
	if(fWatching == ID)
		return;
	
	fListener.StopWatchingInterfaces();
	
	if(ID == PPP_UNDEFINED_INTERFACE_ID || fListener.WatchInterface(ID))
		fWatching = ID;
	
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
DialUpView::AddInterface(const char *name)
{
	if(fInterfaceMenu->FindItem(name)) {
		(new BAlert(ERROR_TITLE, ERROR_INTERFACE_EXISTS, TEXT_OK))->Go();
		return;
	}
	
	BMenuItem *item = new BMenuItem(name, new BMessage(MSG_SELECT_INTERFACE));
	item->SetTarget(this);
	fInterfaceMenu->AddItem(item, CountInterfaces());
	if(CountInterfaces() == 1)
		fInterfaceMenu->SetLabelFromMarked(true);
	SelectInterface(CountInterfaces() - 1);
}


void
DialUpView::SelectInterface(int32 index)
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
		fCurrentItem = fInterfaceMenu->ItemAt(index);
		if(!fCurrentItem) {
			SelectInterface(0);
			return;
		}
		
		fCurrentItem->SetMarked(true);
		WatchInterface(fListener.Manager().InterfaceWithName(fCurrentItem->Label()));
	}
	
	LoadSettings();
}


int32
DialUpView::CountInterfaces() const
{
	return fInterfaceMenu->CountItems() - 3;
}
