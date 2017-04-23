/*
 * Copyright 2003-2004 Waldemar Kornewald. All rights reserved.
 * Copyright 2017 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

//-----------------------------------------------------------------------
// PPPoEAddon saves the loaded settings.
// PPPoEView saves the current settings.
//-----------------------------------------------------------------------

#include "PPPoEAddon.h"

#include "InterfaceUtils.h"
#include "MessageDriverSettingsUtils.h"
#include "TextRequestDialog.h"

#include <Box.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <StringView.h>

#include <PPPManager.h>
#include <PPPoE.h>
	// from PPPoE addon


// GUI constants
static const uint32 kDefaultButtonWidth = 80;

// message constants
static const uint32 kMsgSelectInterface = 'SELI';
static const uint32 kMsgSelectOther = 'SELO';
static const uint32 kMsgFinishSelectOther = 'FISO';
static const uint32 kMsgShowServiceWindow = 'SHSW';
static const uint32 kMsgChangeService = 'CHGS';
static const uint32 kMsgResetService = 'RESS';

// labels
static const char *kLabelInterfaceName = "Network Interface: ";
static const char *kLabelOptional = "(Optional)";
static const char *kLabelOtherInterface = "Other:";
static const char *kLabelSelectInterface = "Select Interface...";
static const char *kLabelServiceName = "Service: ";

// requests
static const char *kRequestInterfaceName = "Network Interface Name: ";

// add-on descriptions
static const char *kFriendlyName = "Broadband: DSL, Cable, etc.";
static const char *kTechnicalName = "PPPoE";
static const char *kKernelModuleName = "pppoe";


PPPoEAddon::PPPoEAddon(BMessage *addons)
	: DialUpAddon(addons),
	fSettings(NULL),
	fProfile(NULL),
	fPPPoEView(NULL)
{
	fHeight = 20 // interface name control
		+ 20 // service control
		+ 5 + 2; // space between controls and bottom
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
	fInterfaceName = fServiceName = "";
	fSettings = settings;
	fProfile = profile;

	if(fPPPoEView)
		fPPPoEView->Reload();

	if(!settings || !profile || isNew)
		return true;

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
	if(!FindMessageParameter(PPPoE_SERVICE_NAME_KEY, device, &parameter, &index)
			|| parameter.FindString(MDSU_VALUES, &fServiceName) != B_OK)
		fServiceName = "";
	else {
		parameter.AddBool(MDSU_VALID, true);
		device.ReplaceMessage(MDSU_PARAMETERS, index, &parameter);
	}

	device.AddBool(MDSU_VALID, true);
	fSettings->ReplaceMessage(MDSU_PARAMETERS, deviceIndex, &device);

	if(fPPPoEView)
		fPPPoEView->Reload();

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
PPPoEAddon::CreateView()
{
	if(!fPPPoEView) {
		float width;
		if(!Addons()->FindFloat(DUN_DEVICE_VIEW_WIDTH, &width))
			width = 270;
				// default value

		BRect rect(0, 0, width, fHeight);
		fPPPoEView = new PPPoEView(this, rect);
		fPPPoEView->Reload();
	}

	return fPPPoEView;
}


PPPoEView::PPPoEView(PPPoEAddon *addon, BRect frame)
	: BView(frame, "PPPoEView", B_FOLLOW_NONE, 0),
	fAddon(addon)
{
	BRect rect = Bounds();
	rect.InsetBy(5, 0);
	rect.bottom = 20;
	fInterface = new BMenuField(rect, "interface", kLabelInterfaceName,
		new BPopUpMenu(kLabelSelectInterface));
	fInterface->SetDivider(StringWidth(fInterface->Label()) + 5);
	fInterface->Menu()->AddSeparatorItem();
	fOtherInterface = new BMenuItem(kLabelOtherInterface,
		new BMessage(kMsgSelectOther));
	fInterface->Menu()->AddItem(fOtherInterface);
	rect.top = rect.bottom + 5;
	rect.bottom = rect.top + 20;
	rect.right -= 75;
	fServiceName = new BTextControl(rect, "service", kLabelServiceName, NULL, NULL);
	fServiceName->SetDivider(StringWidth(fServiceName->Label()) + 5);
	rect.left = rect.right + 5;
	rect.right += 75;
	rect.bottom = rect.top + 15;
	AddChild(new BStringView(rect, "optional", kLabelOptional));

	AddChild(fInterface);
	AddChild(fServiceName);
}


PPPoEView::~PPPoEView()
{
	Addon()->UnregisterView();
}


void
PPPoEView::Reload()
{
	ReloadInterfaces();
	fServiceName->SetText(Addon()->ServiceName());
}


void
PPPoEView::AttachedToWindow()
{
	SetViewColor(Parent()->ViewColor());
	fInterface->Menu()->SetTargetForItems(this);
	fServiceName->SetTarget(this);
}


void
PPPoEView::MessageReceived(BMessage *message)
{
	switch(message->what) {
		case kMsgSelectInterface: {
			BMenuItem *item = fInterface->Menu()->FindMarked();
			if(item)
				fInterfaceName = item->Label();
		} break;

		case kMsgSelectOther:
			(new TextRequestDialog("InterfaceName", NULL, kRequestInterfaceName,
				fInterfaceName.String()))->Go(new BInvoker(
				new BMessage(kMsgFinishSelectOther), this));
		break;

		case kMsgFinishSelectOther: {
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

			BString label(kLabelOtherInterface);
			label << " " << name;
			fOtherInterface->SetLabel(label.String());
			fOtherInterface->SetMarked(true);
				// XXX: this is needed to tell the owning menu to update its label
		} break;

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
	fOtherInterface->SetLabel(kLabelOtherInterface);

	PPPManager manager;
	char *interfaces = new char[8192];
		// reserve enough space for approximately 512 entries
	int32 count = manager.ControlModule("pppoe", PPPoE_GET_INTERFACES, interfaces,
		8192);

	BMenuItem *item;
	char *name = interfaces;
	int32 insertAt;
	for(int32 index = 0; index < count; index++) {
		item = new BMenuItem(name, new BMessage(kMsgSelectInterface));
		insertAt = FindNextMenuInsertionIndex(menu, name);
		if(insertAt > menu->CountItems() - 2)
			insertAt = menu->CountItems() - 2;

		item->SetTarget(this);
		menu->AddItem(item, insertAt);
		name += strlen(name) + 1;
	}

	// set interface or some default value if nothing was found
	if(Addon()->InterfaceName() && strlen(Addon()->InterfaceName()) > 0)
		fInterfaceName = Addon()->InterfaceName();
	else if(count > 0)
		fInterfaceName = interfaces;
	else
		fInterfaceName = "";

	delete interfaces;

	item = menu->FindItem(fInterfaceName.String());
	if(item && menu->IndexOf(item) <= menu->CountItems() - 2)
		item->SetMarked(true);
	else if(Addon()->InterfaceName()) {
		BString label(kLabelOtherInterface);
		label << " " << fInterfaceName;
		fOtherInterface->SetLabel(label.String());
		fOtherInterface->SetMarked(true);
	}
}
