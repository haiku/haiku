//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------
// PPPoEAddon saves the loaded settings.
// PPPoEView saves the current settings.
//-----------------------------------------------------------------------

#include "PPPoEAddon.h"

#include "InterfaceUtils.h"
#include "MessageDriverSettingsUtils.h"
#include "TextRequestDialog.h"
#include <stl_algobase.h>
	// for max()

#include <Box.h>
#include <Button.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Messenger.h>
#include <PopUpMenu.h>
#include <StringView.h>
#include <Window.h>

#include <PPPManager.h>
#include <PPPoE.h>
	// from PPPoE addon


#define MSG_SELECT_INTERFACE		'SELI'
#define MSG_SELECT_OTHER			'SELO'
#define MSG_FINISH_SELECT_OTHER		'FISO'
#define MSG_SHOW_SERVICE_WINDOW		'SHSW'
#define MSG_CHANGE_SERVICE			'CHGS'
#define MSG_RESET_SERVICE			'RESS'

static const char kFriendlyName[] = "DSL, Cable, etc.";
static const char kTechnicalName[] = "PPPoE";
static const char kKernelModuleName[] = "pppoe";


PPPoEAddon::PPPoEAddon(BMessage *addons)
	: DialUpAddon(addons),
	fSettings(NULL),
	fProfile(NULL),
	fPPPoEView(NULL)
{
	fHeight = 20 // interface name control
		+ 25 // service button
		+ 5; // space between controls
}


PPPoEAddon::~PPPoEAddon()
{
	delete fPPPoEView;
		// this may have been set to NULL from the view's destructor!
}


const char*
PPPoEAddon::FriendlyName() const
{
	return kFriendlyName;
}


const char*
PPPoEAddon::TechnicalName() const
{
	return kTechnicalName;
}


const char*
PPPoEAddon::KernelModuleName() const
{
	return kKernelModuleName;
}


bool
PPPoEAddon::LoadSettings(BMessage *settings, BMessage *profile, bool isNew)
{
	fIsNew = isNew;
	fInterfaceName = fACName = fServiceName = "";
	fSettings = settings;
	fProfile = profile;
	if(!settings || !profile || isNew) {
		if(fPPPoEView)
			fPPPoEView->Reload();
		return true;
	}
	
	BMessage device;
	int32 deviceIndex = 0;
	if(!FindMessageParameter(PPP_DEVICE_KEY, *fSettings, &device, &deviceIndex))
		return false;
			// error: no device
	
	BString name;
	if(device.FindString(MDSU_VALUES, &name) != B_OK || name != kKernelModuleName)
		return false;
			// error: no device
	
	BMessage parameter;
	int32 index = 0;
	if(!FindMessageParameter(PPPoE_INTERFACE_KEY, device, &parameter, &index)
			|| parameter.FindString(MDSU_VALUES, &fInterfaceName) != B_OK)
		return false;
			// error: no interface
	else {
		parameter.AddBool(MDSU_VALID, true);
		device.ReplaceMessage(MDSU_PARAMETERS, index, &parameter);
	}
	
	index = 0;
	if(!FindMessageParameter(PPPoE_AC_NAME_KEY, device, &parameter, &index)
			|| parameter.FindString(MDSU_VALUES, &fACName) != B_OK)
		fACName = "";
	else {
		parameter.AddBool(MDSU_VALID, true);
		device.ReplaceMessage(MDSU_PARAMETERS, index, &parameter);
	}
	
	index = 0;
	if(!FindMessageParameter(PPPoE_SERVICE_NAME_KEY, device, &parameter, &index)
			|| parameter.FindString(MDSU_VALUES, &fServiceName) != B_OK)
		fServiceName = "";
	else {
		parameter.AddBool(MDSU_VALID, true);
		device.ReplaceMessage(MDSU_PARAMETERS, index, &parameter);
	}
	
	device.AddBool(MDSU_VALID, true);
	fSettings->ReplaceMessage(MDSU_PARAMETERS, deviceIndex, &device);
	
	return true;
}


void
PPPoEAddon::IsModified(bool *settings, bool *profile) const
{
	*profile = false;
	
	if(!fSettings) {
		*settings = false;
		return;
	}
	
	*settings = (fInterfaceName != fPPPoEView->InterfaceName()
		|| fACName != fPPPoEView->ACName()
		|| fServiceName != fPPPoEView->ServiceName());
}


bool
PPPoEAddon::SaveSettings(BMessage *settings, BMessage *profile, bool saveTemporary)
{
	if(!fSettings || !settings || !fPPPoEView->InterfaceName()
			|| strlen(fPPPoEView->InterfaceName()) == 0)
		return false;
			// TODO: tell user that an interface is needed (if we fail because of this)
	
	BMessage device, interface;
	device.AddString(MDSU_NAME, PPP_DEVICE_KEY);
	device.AddString(MDSU_VALUES, kKernelModuleName);
	
	interface.AddString(MDSU_NAME, PPPoE_INTERFACE_KEY);
	interface.AddString(MDSU_VALUES, fPPPoEView->InterfaceName());
	device.AddMessage(MDSU_PARAMETERS, &interface);
	
	if(fPPPoEView->ACName() && strlen(fPPPoEView->ACName()) > 0) {
		// save access concentrator, too
		BMessage ac;
		ac.AddString(MDSU_NAME, PPPoE_AC_NAME_KEY);
		ac.AddString(MDSU_VALUES, fPPPoEView->ACName());
		device.AddMessage(MDSU_PARAMETERS, &ac);
	}
	
	if(fPPPoEView->ServiceName() && strlen(fPPPoEView->ServiceName()) > 0) {
		// save service name, too
		BMessage service;
		service.AddString(MDSU_NAME, PPPoE_SERVICE_NAME_KEY);
		service.AddString(MDSU_VALUES, fPPPoEView->ServiceName());
		device.AddMessage(MDSU_PARAMETERS, &service);
	}
	
	settings->AddMessage(MDSU_PARAMETERS, &device);
	
	return true;
}


bool
PPPoEAddon::GetPreferredSize(float *width, float *height) const
{
	float viewWidth;
	if(Addons()->FindFloat(DUN_DEVICE_VIEW_WIDTH, &viewWidth) != B_OK)
		viewWidth = 270;
			// default value
	
	if(width)
		*width = viewWidth;
	if(height)
		*height = fHeight;
	
	return true;
}


BView*
PPPoEAddon::CreateView(BPoint leftTop)
{
	if(!fPPPoEView) {
		float width;
		if(!Addons()->FindFloat(DUN_DEVICE_VIEW_WIDTH, &width))
			width = 270;
				// default value
		
		BRect rect(0, 0, width, fHeight);
		fPPPoEView = new PPPoEView(this, rect);
	}
	
	fPPPoEView->MoveTo(leftTop);
	fPPPoEView->Reload();
	return fPPPoEView;
}


PPPoEView::PPPoEView(PPPoEAddon *addon, BRect frame)
	: BView(frame, "PPPoEView", B_FOLLOW_NONE, 0),
	fAddon(addon)
{
	BRect rect = Bounds();
	rect.InsetBy(5, 0);
	rect.bottom = 20;
	fInterface = new BMenuField(rect, "interface", "Interface: ",
		new BPopUpMenu("Select Interface..."));
	fInterface->SetDivider(StringWidth(fInterface->Label()) + 5);
	fInterface->Menu()->AddSeparatorItem();
	fOtherInterface = new BMenuItem("Other:", new BMessage(MSG_SELECT_OTHER));
	fInterface->Menu()->AddItem(fOtherInterface);
	
	rect.top = rect.bottom + 5;
	rect.bottom = rect.top + 45;
	fServiceButton = new BButton(rect, "ServiceButton", "Service",
		new BMessage(MSG_SHOW_SERVICE_WINDOW));
	fServiceButton->ResizeToPreferred();
	
	// create service window
	float boxHeight = 20 // space at top
		+ 2 * 20 // size of controls
		+ 2 * 5; // space between controls and bottom
	float windowHeight = boxHeight
		+ 35 // buttons
		+ 2 * 5; // space between controls and bottom
	rect.Set(350, 250, 650, 250 + windowHeight);
	fServiceWindow = new BWindow(rect, "Service", B_MODAL_WINDOW,
		B_NOT_RESIZABLE | B_NOT_CLOSABLE);
	rect = fServiceWindow->Bounds();
	BView *serviceView = new BView(rect, "serviceView", B_FOLLOW_NONE, 0);
	serviceView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	rect.InsetBy(5, 5);
	rect.bottom = rect.top + boxHeight;
	BBox *serviceBox = new BBox(rect, "Service");
	serviceBox->SetLabel("Service");
	rect = serviceBox->Bounds();
	rect.InsetBy(10, 0);
	rect.top = 20;
	rect.bottom = rect.top + 20;
	fACName = new BTextControl(rect, "ac", "AC: ", NULL, NULL);
	rect.top = rect.bottom + 5;
	rect.bottom = rect.top + 20;
	fServiceName = new BTextControl(rect, "service", "Service: ", NULL, NULL);
	
	// set divider of text controls
	float acNameWidth = StringWidth(fACName->Label()) + 5;
	float serviceNameWidth = StringWidth(fServiceName->Label()) + 5;
	float width = max(acNameWidth, serviceNameWidth);
	fACName->SetDivider(width);
	fServiceName->SetDivider(width);
	
	// add buttons to service window
	rect = serviceBox->Frame();
	rect.top = rect.bottom + 10;
	rect.bottom = rect.top + 25;
	float buttonWidth = (rect.Width() - 10) / 2;
	rect.right = rect.left + buttonWidth;
	fCancelButton = new BButton(rect, "CancelButton", "Cancel",
		new BMessage(MSG_RESET_SERVICE));
	rect.left = rect.right + 10;
	rect.right = rect.left + buttonWidth;
	fOKButton = new BButton(rect, "OKButton", "OK", new BMessage(MSG_CHANGE_SERVICE));
	
	serviceBox->AddChild(fACName);
	serviceBox->AddChild(fServiceName);
	serviceView->AddChild(serviceBox);
	serviceView->AddChild(fCancelButton);
	serviceView->AddChild(fOKButton);
	fServiceWindow->AddChild(serviceView);
	fServiceWindow->SetDefaultButton(fOKButton);
	fServiceWindow->Run();
		// this must be called in order for Reload() to work properly
	
	AddChild(fInterface);
	AddChild(fServiceButton);
}


PPPoEView::~PPPoEView()
{
	Addon()->UnregisterView();
}


void
PPPoEView::Reload()
{
	// update interface settings
	ReloadInterfaces();
	
	// update service settings (in service window => must be locked)
	fServiceWindow->Lock();
	fACName->SetText(Addon()->ACName());
	fPreviousACName = Addon()->ACName();
	fServiceName->MakeFocus(true);
	fServiceName->SetText(Addon()->ServiceName());
	fPreviousServiceName = Addon()->ServiceName();
	fServiceWindow->Unlock();
}


void
PPPoEView::AttachedToWindow()
{
	SetViewColor(Parent()->ViewColor());
	fInterface->Menu()->SetTargetForItems(this);
	fServiceButton->SetTarget(this);
	fACName->SetTarget(this);
	fServiceName->SetTarget(this);
	fCancelButton->SetTarget(this);
	fOKButton->SetTarget(this);
}


void
PPPoEView::MessageReceived(BMessage *message)
{
	switch(message->what) {
		case MSG_SELECT_INTERFACE: {
			BMenuItem *item = fInterface->Menu()->FindMarked();
			if(item)
				fInterfaceName = item->Label();
		} break;
		
		case MSG_SELECT_OTHER:
			(new TextRequestDialog("InterfaceName", "Interface Name: ",
				fInterfaceName.String()))->Go(new BInvoker(
				new BMessage(MSG_FINISH_SELECT_OTHER), this));
		break;
		
		case MSG_FINISH_SELECT_OTHER: {
			int32 which;
			message->FindInt32("which", &which);
			
			const char *name = message->FindString("text");
			BMenu *menu = fInterface->Menu();
			BMenuItem *item;
			if(which != 1 || !name || strlen(name) == 0) {
				item = menu->FindItem(fInterfaceName.String());
				if(item && menu->IndexOf(item) <= menu->CountItems() - 2)
					item->SetMarked(true);
				else
					fOtherInterface->SetMarked(true);
				
				return;
			}
			
			fInterfaceName = name;
			
			item = menu->FindItem(fInterfaceName.String());
			if(item && menu->IndexOf(item) <= menu->CountItems() - 2) {
				item->SetMarked(true);
				return;
			}
			
			BString label("Other: ");
			label << name;
			fOtherInterface->SetLabel(label.String());
			fOtherInterface->SetMarked(true);
				// XXX: this is needed to tell the owning menu to update its label
		} break;
		
		case MSG_SHOW_SERVICE_WINDOW:
			fServiceWindow->MoveTo(center_on_screen(fServiceWindow->Bounds(),
				Window()));
			fServiceWindow->Show();
		break;
		
		case MSG_CHANGE_SERVICE:
			fServiceWindow->Hide();
			fServiceWindow->Lock();
			fPreviousACName = fACName->Text();
			fPreviousServiceName = fServiceName->Text();
			fServiceWindow->Unlock();
		break;
		
		case MSG_RESET_SERVICE:
			fServiceWindow->Hide();
			fServiceWindow->Lock();
			fACName->SetText(fPreviousACName.String());
			fServiceName->SetText(fPreviousServiceName.String());
			fServiceWindow->Unlock();
		break;
		
		default:
			BView::MessageReceived(message);
	}
}


void
PPPoEView::ReloadInterfaces()
{
	// delete all items and request a new bunch from the pppoe kernel module
	BMenu *menu = fInterface->Menu();
	while(menu->CountItems() > 2)
		delete menu->RemoveItem((int32) 0);
	fOtherInterface->SetLabel("Other:");
	
	PPPManager manager;
	char *interfaces = new char[8192];
		// reserve enough space for approximately 512 entries
	int32 count = manager.ControlModule("pppoe", PPPoE_GET_INTERFACES, interfaces,
		8192);
	
	BMenuItem *item;
	char *name = interfaces;
	int32 insertAt;
	for(int32 index = 0; index < count; index++) {
		item = new BMenuItem(name, new BMessage(MSG_SELECT_INTERFACE));
		insertAt = FindNextMenuInsertionIndex(menu, name);
		if(insertAt > menu->CountItems() - 2)
			insertAt = menu->CountItems() - 2;
		
		item->SetTarget(this);
		menu->AddItem(item, insertAt);
		name += strlen(name) + 1;
	}
	
	delete interfaces;
	
	if(!Addon()->InterfaceName())
		fInterfaceName = "";
	else
		fInterfaceName = Addon()->InterfaceName();
	
	item = menu->FindItem(fInterfaceName.String());
	if(item && menu->IndexOf(item) <= menu->CountItems() - 2)
		item->SetMarked(true);
	else if(Addon()->InterfaceName()) {
		BString label("Other: ");
		label << fInterfaceName;
		fOtherInterface->SetLabel(label.String());
		fOtherInterface->SetMarked(true);
	}
}
