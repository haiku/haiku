/*
 * Copyright 2003-2005, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

#include "DialUpView.h"
#include "DialUpAddon.h"

#include <cstring>
#include "InterfaceUtils.h"
#include "MessageDriverSettingsUtils.h"
#include "TextRequestDialog.h"

#include <PPPInterface.h>
#include <settings_tools.h>
#include <TemplateList.h>

#include <Application.h>

#include <Alert.h>
#include <Button.h>
#include <CheckBox.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Messenger.h>
#include <PopUpMenu.h>
#include <StringView.h>
#include <TabView.h>

#include <Directory.h>
#include <Entry.h>
#include <Path.h>


// GUI constants
static const uint32 kInterfaceFieldWidth = 175;

// message constants
static const uint32 kMsgCreateNew = 'NEWI';
static const uint32 kMsgFinishCreateNew = 'FNEW';
static const uint32 kMsgDeleteCurrent = 'DELI';
static const uint32 kMsgSelectInterface = 'SELI';
static const uint32 kMsgConnectButton = 'CONI';
static const uint32 kMsgUpdateDefaultInterface = 'UPDT';

// labels
static const char *kLabelInterface = "Interface: ";
static const char *kLabelInterfaceName = "Interface Name: ";
static const char *kLabelCreateNewInterface = "Create New Interface";
static const char *kLabelCreateNew = "Create New...";
static const char *kLabelDefaultInterface = "Default";
static const char *kLabelDeleteCurrent = "Delete Current";
static const char *kLabelConnect = "Connect";
static const char *kLabelDisconnect = "Disconnect";
static const char *kLabelOK = "OK";

// connection status strings
static const char *kTextConnecting = "Connecting...";
static const char *kTextConnectionEstablished = "Connection established.";
static const char *kTextNotConnected = "Not connected.";
static const char *kTextDeviceUpFailed = "Failed to connect.";
static const char *kTextAuthenticating = "Authenticating...";
static const char *kTextAuthenticationFailed = "Authentication failed!";
static const char *kTextConnectionLost = "Connection lost!";
static const char *kTextCreationError = "Error creating interface!";
static const char *kTextNoInterfacesFound = "Please create a new interface...";
static const char *kTextChooseInterfaceName = "Please choose a new name for this "
											"interface.";

static const char *kErrorTitle = "Error";
static const char *kErrorNoPPPStack = "Error: Could not access the PPP stack!";
static const char *kErrorInterfaceExists = "Error: An interface with this name "
										"already exists!";
static const char *kErrorLoadingFailed = "Error: Failed loading interface! The "
										"current settings will be deleted.";
static const char *kErrorSavingFailed = "Error: Failed saving interface settings!";


DialUpView::DialUpView(BRect frame)
	: BView(frame, "DialUpView", B_FOLLOW_NONE, 0),
	fListener(this),
	fCurrentItem(NULL),
	fKeepLabel(false)
{
	BRect bounds = Bounds();
		// for caching
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	// add messenger to us so add-ons can contact us
	BMessenger messenger(this);
	fSettings.Addons().AddMessenger(DUN_MESSENGER, messenger);
	
	// create pop-up with all interfaces and "New..."/"Delete current" items
	fInterfaceMenu = new BPopUpMenu(kLabelCreateNew);
	BRect rect = bounds;
	rect.InsetBy(5, 5);
	rect.right = kInterfaceFieldWidth;
	rect.bottom = rect.top + 20;
	fMenuField = new BMenuField(rect, "Interfaces", kLabelInterface, fInterfaceMenu);
	fMenuField->SetDivider(StringWidth(fMenuField->Label()) + 5);
	rect.top += 3;
	rect.bottom -= 2;
	rect.left = rect.right + 5;
	rect.right = bounds.right - 5;
	fDefaultInterface = new BCheckBox(rect, "Default", kLabelDefaultInterface,
		new BMessage(kMsgUpdateDefaultInterface));
	rect.left = bounds.left + 5;
	rect.top = rect.bottom + 12;
	rect.bottom = bounds.bottom
		- 20 // height of bottom controls
		- 20; // space for bottom controls
	fTabView = new BTabView(rect, "TabView", B_WIDTH_FROM_LABEL);
	BRect tabViewRect(fTabView->Bounds());
	tabViewRect.bottom -= fTabView->TabHeight();
	fSettings.Addons().AddRect(DUN_TAB_VIEW_RECT, tabViewRect);
	
	BRect tmpRect(rect);
	tmpRect.top += (tmpRect.Height() - 15) / 2;
	tmpRect.bottom = tmpRect.top + 15;
	fStringView = new BStringView(tmpRect, "NoInterfacesFound",
		kTextNoInterfacesFound);
	fStringView->SetAlignment(B_ALIGN_CENTER);
	fStringView->Hide();
	tmpRect.top = tmpRect.bottom + 10;
	tmpRect.bottom = tmpRect.top + 25;
	fCreateNewButton = new BButton(tmpRect, "CreateNewButton",
		kLabelCreateNewInterface, new BMessage(kMsgCreateNew));
	fCreateNewButton->ResizeToPreferred();
	tmpRect.left = (rect.Width() - fCreateNewButton->Bounds().Width()) / 2 + rect.left;
	fCreateNewButton->MoveTo(tmpRect.left, tmpRect.top);
	fCreateNewButton->Hide();
	
	rect.top = rect.bottom + 15;
	rect.bottom = rect.top + 15;
	rect.right = rect.left + 200;
	fStatusView = new BStringView(rect, "StatusView", kTextNotConnected);
	
	rect.InsetBy(0, -5);
	rect.left = rect.right + 5;
	rect.right = bounds.right - 5;
	fConnectButton = new BButton(rect, "ConnectButton", kLabelConnect,
		new BMessage(kMsgConnectButton));
	
	AddChild(fMenuField);
	AddChild(fDefaultInterface);
	AddChild(fTabView);
	AddChild(fStringView);
	AddChild(fCreateNewButton);
	AddChild(fStatusView);
	AddChild(fConnectButton);
	
	// initialize
	fListener.WatchManager();
	LoadInterfaces();
	fSettings.LoadAddons();
	CreateTabs();
	fCurrentItem = NULL;
		// reset, otherwise SelectInterface will not load the settings
	SelectInterface(0);
	UpdateControls();
}


DialUpView::~DialUpView()
{
	fListener.StopWatchingInterface();
	fListener.StopWatchingManager();
	fSettings.SaveSettingsToFile();
}


void
DialUpView::AttachedToWindow()
{
	fInterfaceMenu->SetTargetForItems(this);
	fCreateNewButton->SetTarget(this);
	fConnectButton->SetTarget(this);
	fDefaultInterface->SetTarget(this);
	fDefaultInterface->Hide();
		// TODO: remove this when we have full COD support
	
	if(fListener.InitCheck() != B_OK) {
		(new BAlert(kErrorTitle, kErrorNoPPPStack, kLabelOK,
			NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go(NULL);
		fConnectButton->Hide();
	}
}


void
DialUpView::MessageReceived(BMessage *message)
{
	switch(message->what) {
		case PPP_REPORT_MESSAGE:
			HandleReportMessage(message);
		break;
		
		// -------------------------------------------------
		case kMsgCreateNew: {
			UpdateControls();
			(new TextRequestDialog(kLabelCreateNewInterface, kTextChooseInterfaceName,
					kLabelInterfaceName))->Go(
				new BInvoker(new BMessage(kMsgFinishCreateNew), this));
		} break;
		
		case kMsgFinishCreateNew: {
			int32 which;
			message->FindInt32("which", &which);
			const char *name = message->FindString("text");
			if(which == 1 && name && strlen(name) > 0)
				AddInterface(name, true);
			
			if(fCurrentItem)
				fCurrentItem->SetMarked(true);
			
			UpdateControls();
			
			// a newly created interface is set to default if there is no default one
			if(PPPManager::DefaultInterface() == "") {
				fDefaultInterface->SetValue(true);
				PPPManager::SetDefaultInterface(name);
			}
		} break;
		// -------------------------------------------------
		
		case kMsgDeleteCurrent: {
			if(!fCurrentItem)
				return;
			
			const char *name = fCurrentItem->Message()->FindString("name");
			if(PPPManager::DefaultInterface() == name)
				PPPManager::SetDefaultInterface("");
			fInterfaceMenu->RemoveItem(fCurrentItem);
			BDirectory settings;
			PPPManager::GetSettingsDirectory(&settings);
			BEntry entry;
			settings.FindEntry(name, &entry);
			entry.Remove();
			delete fCurrentItem;
			fCurrentItem = NULL;
			
			BMenuItem *marked = fInterfaceMenu->FindMarked();
			if(marked)
				marked->SetMarked(false);
			
			UpdateControls();
			SelectInterface(0);
				// this stops watching the deleted interface
		} break;
		
		case kMsgSelectInterface: {
			int32 index;
			message->FindInt32("index", &index);
			SelectInterface(index);
		} break;
		
		case kMsgConnectButton: {
			if(!fCurrentItem)
				return;
			
			BringUpOrDown();
		} break;
		
		case kMsgUpdateDefaultInterface:
			UpdateDefaultInterface();
		break;
		
		default:
			BView::MessageReceived(message);
	}
}


void
DialUpView::BringUpOrDown()
{
	fSettings.SaveSettingsToFile();
	BMessage settings;
	fSettings.SaveSettings(&settings);
	
	PPPInterface interface;
	ppp_interface_info_t info;
	
	// if going up: delete interface in order for the settings changes to take effect
	interface = fListener.Manager().InterfaceWithName(
		fCurrentItem->Message()->FindString("name"));
	interface.GetInterfaceInfo(&info);
	if(interface.InitCheck() == B_OK && info.info.state == PPP_INITIAL_STATE
			&& info.info.phase == PPP_DOWN_PHASE)
		fListener.Manager().DeleteInterface(interface.ID());
	
	interface = fListener.Manager().CreateInterfaceWithName(
		fCurrentItem->Message()->FindString("name"));
	
	if(interface.InitCheck() != B_OK) {
		Window()->Lock();
		fStatusView->SetText(kTextCreationError);
		Window()->Unlock();
		return;
	}
	
	interface.GetInterfaceInfo(&info);
	if(info.info.state == PPP_INITIAL_STATE && info.info.phase == PPP_DOWN_PHASE) {
		interface.SetPassword(fSettings.SessionPassword());
		interface.SetAskBeforeConnecting(false);
		interface.Up();
	} else
		interface.Down();
}


void
DialUpView::HandleReportMessage(BMessage *message)
{
	if(!fCurrentItem)
		return;
	
	ppp_interface_id id;
	if(message->FindInt32("interface", reinterpret_cast<int32*>(&id)) != B_OK
			|| (fListener.Interface() != PPP_UNDEFINED_INTERFACE_ID
				&& id != fListener.Interface()))
		return;
	
	int32 type, code;
	message->FindInt32("type", &type);
	message->FindInt32("code", &code);
	
	if(type == PPP_MANAGER_REPORT && code == PPP_REPORT_INTERFACE_CREATED) {
		PPPInterface interface(id);
		if(interface.InitCheck() != B_OK
				|| strcasecmp(interface.Name(),
					fCurrentItem->Message()->FindString("name")))
			return;
		
		WatchInterface(id);
	} else if(type == PPP_CONNECTION_REPORT)
		UpdateStatus(code);
	else if(type == PPP_DESTRUCTION_REPORT)
		WatchInterface(fListener.Manager().InterfaceWithName(
			fCurrentItem->Message()->FindString("name")));
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
			fSettings.Addons().FindPointer(DUN_TAB_ADDON_TYPE, index,
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
		
		target = addon->CreateView(BPoint(0, 0));
		if(!target)
			continue;
		
		fTabView->AddTab(target, NULL);
	}
}


void
DialUpView::UpdateStatus(int32 code)
{
	switch(code) {
		case PPP_REPORT_DEVICE_UP_FAILED:
		case PPP_REPORT_AUTHENTICATION_FAILED:
		case PPP_REPORT_DOWN_SUCCESSFUL:
		case PPP_REPORT_CONNECTION_LOST:
			fConnectButton->SetLabel(kLabelConnect);
		break;
		
		default:
			fConnectButton->SetLabel(kLabelDisconnect);
	}
	
	// maybe the status string must not be changed (codes that set fKeepLabel to false
	// should still be handled)
	if(fKeepLabel && code != PPP_REPORT_GOING_UP && code != PPP_REPORT_UP_SUCCESSFUL)
		return;
	
	if(fListener.InitCheck() != B_OK) {
		fStatusView->SetText(kErrorNoPPPStack);
		return;
	}
	
	// only errors should set fKeepLabel to true
	switch(code) {
		case PPP_REPORT_GOING_UP:
			fKeepLabel = false;
			fStatusView->SetText(kTextConnecting);
		break;
		
		case PPP_REPORT_UP_SUCCESSFUL:
			fKeepLabel = false;
			fStatusView->SetText(kTextConnectionEstablished);
		break;
		
		case PPP_REPORT_DOWN_SUCCESSFUL:
			fStatusView->SetText(kTextNotConnected);
		break;
		
		case PPP_REPORT_DEVICE_UP_FAILED:
			fKeepLabel = true;
			fStatusView->SetText(kTextDeviceUpFailed);
		break;
		
		case PPP_REPORT_AUTHENTICATION_REQUESTED:
			fStatusView->SetText(kTextAuthenticating);
		break;
		
		case PPP_REPORT_AUTHENTICATION_FAILED:
			fKeepLabel = true;
			fStatusView->SetText(kTextAuthenticationFailed);
		break;
		
		case PPP_REPORT_CONNECTION_LOST:
			fKeepLabel = true;
			fStatusView->SetText(kTextConnectionLost);
		break;
	}
}


void
DialUpView::WatchInterface(ppp_interface_id ID)
{
	fListener.WatchInterface(ID);
	
	// update status
	PPPInterface interface(fListener.Interface());
	if(interface.InitCheck() != B_OK)
		UpdateStatus(PPP_REPORT_DOWN_SUCCESSFUL);
}


void
DialUpView::LoadInterfaces()
{
	fInterfaceMenu->AddSeparatorItem();
	fInterfaceMenu->AddItem(new BMenuItem(kLabelCreateNewInterface,
		new BMessage(kMsgCreateNew)));
	fDeleterItem = new BMenuItem(kLabelDeleteCurrent,
		new BMessage(kMsgDeleteCurrent));
	fInterfaceMenu->AddItem(fDeleterItem);
	
	BDirectory settingsDirectory;
	BEntry entry;
	BPath path;
	PPPManager::GetSettingsDirectory(&settingsDirectory);
	while(settingsDirectory.GetNextEntry(&entry) == B_OK) {
		if(entry.IsFile()) {
			entry.GetPath(&path);
			AddInterface(path.Leaf(), true);
		}
	}
}


void
DialUpView::AddInterface(const char *name, bool isNew = false)
{
	if(FindInterface(name)) {
		(new BAlert(kErrorTitle, kErrorInterfaceExists, kLabelOK,
			NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go(NULL);
		return;
	}
	
	BMessage *message = new BMessage(kMsgSelectInterface);
	message->AddString("name", name);
	BString label(name);
	if(PPPManager::DefaultInterface() == label)
		label << " (" << kLabelDefaultInterface << ")";
	BMenuItem *item = new BMenuItem(label.String(), message);
	item->SetTarget(this);
	int32 index = FindNextMenuInsertionIndex(fInterfaceMenu, name);
	if(index > CountInterfaces())
		index = CountInterfaces();
	fInterfaceMenu->AddItem(item, index);
	UpdateControls();
	
	item->SetMarked(true);
	SelectInterface(index, isNew);
}


void
DialUpView::SelectInterface(int32 index, bool isNew = false)
{
	BMenuItem *item = fInterfaceMenu->FindMarked();
	if(fCurrentItem && item == fCurrentItem)
		return;
	
	if(fCurrentItem && !fSettings.SaveSettingsToFile())
		(new BAlert(kErrorTitle, kErrorSavingFailed, kLabelOK,
			NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go(NULL);
	
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
		
		const char *name = fCurrentItem->Message() ?
			fCurrentItem->Message()->FindString("name") : NULL;
		fDefaultInterface->SetValue(name && PPPManager::DefaultInterface() == name);
		fCurrentItem->SetMarked(true);
		fDeleterItem->SetEnabled(true);
		fInterfaceMenu->Superitem()->SetLabel(name);
		WatchInterface(fListener.Manager().InterfaceWithName(name));
	}
	
	UpdateControls();
	
	if(!fCurrentItem)
		fSettings.LoadSettings(NULL, false);
			// tell modules to unload all settings
	else if(!isNew && !fSettings.LoadSettings(
			fCurrentItem->Message()->FindString("name"), false)) {
		(new BAlert(kErrorTitle, kErrorLoadingFailed, kLabelOK,
			NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go(NULL);
		fSettings.LoadSettings(fCurrentItem->Message()->FindString("name"), true);
	} else if(isNew && !fSettings.LoadSettings(
			fCurrentItem->Message()->FindString("name"), true))
		(new BAlert(kErrorTitle, kErrorLoadingFailed, kLabelOK,
			NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go(NULL);
}


int32
DialUpView::CountInterfaces() const
{
	return fInterfaceMenu->CountItems() - 3;
}


BMenuItem*
DialUpView::FindInterface(BString name)
{
	BMenuItem *item;
	for(int32 index = 0; index < CountInterfaces(); index++) {
		item = fInterfaceMenu->ItemAt(index);
		if(item && item->Message() && item->Message()->HasString("name")
				&& name == item->Message()->FindString("name"))
			return item;
	}
	
	return NULL;
}


void
DialUpView::UpdateControls()
{
	if(fTabView->IsHidden() && CountInterfaces() > 0) {
		fInterfaceMenu->SetLabelFromMarked(true);
		fStringView->Hide();
		fCreateNewButton->Hide();
		fTabView->Show();
		fDefaultInterface->Show();
		fConnectButton->SetEnabled(true);
	} else if(!fTabView->IsHidden() && CountInterfaces() == 0) {
		fDeleterItem->SetEnabled(false);
		fInterfaceMenu->SetRadioMode(false);
		fInterfaceMenu->Superitem()->SetLabel(kLabelCreateNew);
		fTabView->Hide();
		fDefaultInterface->Hide();
		fStringView->Show();
		fCreateNewButton->Show();
		fConnectButton->SetEnabled(false);
	}
	
	// move default checkbox next to interface menu (its size might have changed)
	float width = fInterfaceMenu->StringWidth(fMenuField->Label())
		+ fInterfaceMenu->StringWidth(fInterfaceMenu->Superitem()->Label()) + 30;
	if(width > kInterfaceFieldWidth)
		width = kInterfaceFieldWidth;
	fDefaultInterface->MoveTo(fMenuField->Frame().left + width,
		fDefaultInterface->Frame().top);
}


void
DialUpView::UpdateDefaultInterface()
{
	const char *name = fCurrentItem->Message()->FindString("name");
	BMenuItem *defaultItem = FindInterface(PPPManager::DefaultInterface());
	if(fDefaultInterface->Value()) {
		if(!PPPManager::SetDefaultInterface(name)) {
			fDefaultInterface->SetValue(0);
			return;
		}
		if(defaultItem)
			defaultItem->SetLabel(defaultItem->Message()->FindString("name"));
		
		BString label(name);
		label << " (" << kLabelDefaultInterface << ")";
		fCurrentItem->SetLabel(label.String());
	} else {
		PPPManager::SetDefaultInterface("");
		fCurrentItem->SetLabel(name);
	}
}
