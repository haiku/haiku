//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------
// GeneralAddon saves the loaded settings.
// GeneralView saves the current settings.
//-----------------------------------------------------------------------

#include "GeneralAddon.h"

#include "MessageDriverSettingsUtils.h"

#include <Box.h>
#include <Button.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Messenger.h>
#include <PopUpMenu.h>
#include <StringView.h>

#include <PPPDefs.h>


#define MSG_SELECT_DEVICE			'SELD'
#define MSG_SELECT_AUTHENTICATOR	'SELA'


GeneralAddon::GeneralAddon(BMessage *addons)
	: DialUpAddon(addons),
	fHasPassword(false),
	fAuthenticatorsCount(0),
	fSettings(NULL),
	fProfile(NULL)
{
	BRect rect;
	addons->FindRect("TabViewRect", &rect);
	fGeneralView = new GeneralView(this, rect);
}


GeneralAddon::~GeneralAddon()
{
}


DialUpAddon*
GeneralAddon::FindDevice(const BString& moduleName) const
{
	DialUpAddon *addon;
	for(int32 index = 0; Addons()->FindPointer("Device", index,
			reinterpret_cast<void**>(addon)); index++)
		if(addon && moduleName == addon->KernelModuleName())
			return addon;
	
	return NULL;
}


bool
GeneralAddon::LoadSettings(BMessage *settings, BMessage *profile, bool isNew)
{
	fIsNew = isNew;
	fHasPassword = false;
	fDeviceName = fUsername = fPassword = "";
	fDeviceAddon = NULL;
	fAuthenticatorsCount = 0;
	fSettings = settings;
	fProfile = profile;
	if(!settings || !profile || isNew)
		return true;
	
	if(!LoadDeviceSettings(settings, profile))
		return false;
	
	if(!LoadAuthenticationSettings(settings, profile))
		return false;
	
	return true;
}


bool
GeneralAddon::LoadDeviceSettings(BMessage *settings, BMessage *profile)
{
	int32 index = 0;
	BMessage device;
	if(!FindMessageParameter(PPP_DEVICE_KEY, *settings, device, &index))
		return false;
			// TODO: tell user that device specification is missing
	
	if(!device.FindString("Values", &fDeviceName))
		return false;
			// TODO: tell user that device specification is missing
	
	device.AddBool("Valid", true);
	settings->ReplaceMessage("Parameters", index, &device);
	
	fDeviceAddon = FindDevice(fDeviceName);
	if(!fDeviceAddon)
		return false;
	
	return fDeviceAddon->LoadSettings(settings, profile, false);
}


bool
GeneralAddon::LoadAuthenticationSettings(BMessage *settings, BMessage *profile)
{
	// we only handle the profile (although settings could contain different data)
	int32 itemIndex = 0;
	BMessage authentication, item;
	
	if(!FindMessageParameter(PPP_AUTHENTICATOR_KEY, *profile, item, &itemIndex))
		return true;
	
	// find authenticators (though we load all authenticators, we only use one)
	BString name;
	for(int32 index = 0; item.FindString("Values", index, &name) == B_OK; index++) {
		BMessage authenticator;
		if(!GetAuthenticator(name, &authenticator))
			return false;
				// fatal error: we do not know how to handle this authenticator
		
		MarkAuthenticatorAsValid(name);
		authentication.AddString("Authenticators", name);
		++fAuthenticatorsCount;
	}
	
	fSettings->AddMessage("Authentication", &authentication);
	
	bool hasUsername = false;
		// a username must be present
	
	// load username and password
	BMessage parameter;
	int32 parameterIndex = 0;
	if(FindMessageParameter("User", item, parameter, &parameterIndex)
			&& parameter.FindString("Values", &fUsername) == B_OK) {
		hasUsername = true;
		parameter.AddBool("Valid", true);
		item.ReplaceMessage("Parameters", parameterIndex, &parameter);
	}
	
	parameterIndex = 0;
	if(FindMessageParameter("Password", item, parameter, &parameterIndex)
			&& parameter.FindString("Values", &fPassword) == B_OK) {
		fHasPassword = true;
		parameter.AddBool("Valid", true);
		item.ReplaceMessage("Parameters", parameterIndex, &parameter);
	}
	
	// tell DUN whether everything is valid
	if(hasUsername)
		item.AddBool("Valid", true);
	
	profile->ReplaceMessage("Parameters", itemIndex, &item);
	
	return true;
}


void
GeneralAddon::IsModified(bool& settings, bool& profile) const
{
	if(!fSettings) {
		settings = profile = false;
		return;
	}
	
	bool deviceSettings, authenticationSettings, deviceProfile, authenticationProfile;
	
	IsDeviceModified(deviceSettings, deviceProfile);
	IsAuthenticationModified(authenticationSettings, authenticationProfile);
	
	settings = (deviceSettings || authenticationSettings);
	profile = (deviceProfile || authenticationProfile);
}


void
GeneralAddon::IsDeviceModified(bool& settings, bool& profile) const
{
	if(fDeviceAddon)
		settings = (!fGeneralView->DeviceName() || fGeneralView->IsDeviceModified());
	else
		settings = fGeneralView->DeviceName();
	
	profile = false;
}


void
GeneralAddon::IsAuthenticationModified(bool& settings, bool& profile) const
{
	// currently we only support selecting one authenticator
	if(fAuthenticatorsCount == 0)
		settings = fGeneralView->AuthenticatorName();
	else {
		BMessage authentication;
		if(fSettings->FindMessage("Authentication", &authentication) != B_OK) {
			settings = profile = false;
			return;
				// error!
		}
		
		BString authenticator;
		if(authentication.FindString("Authenticators", &authenticator) != B_OK) {
			settings = profile = false;
			return;
				// error!
		}
		
		settings = (!fGeneralView->AuthenticatorName()
			|| authenticator != fGeneralView->AuthenticatorName());
	}
	
	profile = (fUsername != fGeneralView->Username()
		|| (fPassword != fGeneralView->Password() && fHasPassword)
		|| fHasPassword != fGeneralView->DoesSavePassword());
}


bool
GeneralAddon::SaveSettings(BMessage *settings, BMessage *profile, bool saveTemporary)
{
	if(!fSettings || !settings || !fGeneralView->DeviceName())
		return false;
			// TODO: tell user that a device is needed (if we fail because of this)
	
	if(fGeneralView->AuthenticatorName()) {
		BMessage authenticator;
		authenticator.AddString("Name", PPP_AUTHENTICATOR_KEY);
		authenticator.AddString("Values", fGeneralView->AuthenticatorName());
		
		BMessage username;
		username.AddString("Name", "User");
		username.AddString("Values", fGeneralView->Username());
		authenticator.AddMessage("Parameters", &username);
		
		if(saveTemporary || fGeneralView->DoesSavePassword()) {
			// save password, too
			BMessage password;
			password.AddString("Name", "Password");
			password.AddString("Values", fGeneralView->Password());
			authenticator.AddMessage("Parameters", &password);
		}
	}
	
	DialUpAddon *addon = FindDevice(fGeneralView->DeviceName());
	if(!addon)
		return false;
	
	return addon->SaveSettings(settings, profile, saveTemporary);
}


bool
GeneralAddon::GetPreferredSize(float *width, float *height) const
{
	BRect rect(0, 0, 200, 300);
		// set default values
	Addons()->FindRect("TabViewRect", &rect);
	
	if(width)
		*width = rect.Width();
	if(height)
		*height = rect.Height();
	
	return true;
}


BView*
GeneralAddon::CreateView(BPoint leftTop)
{
	BRect rect;
	if(Addons()->FindRect("TabViewRect", &rect) != B_OK);
		rect.Set(0, 0, 200, 300);
			// set default values
	
	fGeneralView->MoveTo(leftTop);
	fGeneralView->Reload();
	return fGeneralView;
}


bool
GeneralAddon::GetAuthenticator(const BString& moduleName, BMessage *entry) const
{
	if(!entry)
		return false;
	
	BString name;
	for(int32 index = 0; Addons()->FindMessage("Authenticator", index, entry) == B_OK;
			index++) {
		entry->FindString("KernelModuleName", &name);
		if(name == moduleName)
			return true;
	}
	
	return false;
}


bool
GeneralAddon::MarkAuthenticatorAsValid(const BString& moduleName)
{
	BMessage authenticator;
	int32 index = 0;
	BString name;
	
	for(; FindMessageParameter(PPP_AUTHENTICATOR_KEY, *fSettings, authenticator,
			&index); index++) {
		authenticator.FindString("KernelModuleName", &name);
		if(name == moduleName) {
			authenticator.AddBool("Valid", true);
			fSettings->ReplaceMessage("Parameters", index, &authenticator);
			return true;
		}
	}
	
	return false;
}


GeneralView::GeneralView(GeneralAddon *addon, BRect frame)
	: BView(frame, "General", B_FOLLOW_NONE, 0),
	fAddon(addon)
{
	BRect rect = Bounds();
	rect.InsetBy(5, 5);
	rect.bottom = 100;
	fDeviceBox = new BBox(rect, "Device");
	rect.top = rect.bottom + 10;
	rect.bottom = rect.top
		+ 25 // space for topmost control
		+ 3 * 20 // size of controls
		+ 3 * 5; // space beween controls and bottom of box
	fAuthenticationBox = new BBox(rect, "Authentication");
	
	fDeviceField = new BMenuField(BRect(5, 0, 250, 25), "Device",
		"Device:", new BPopUpMenu("No Devices Found!"));
	fDeviceField->SetDivider(fDeviceField->StringWidth(fDeviceField->Label()) + 10);
	fDeviceField->Menu()->SetRadioMode(true);
	AddDevices();
	fDeviceBox->SetLabel(fDeviceField);
	
	fAuthenticatorField = new BMenuField(BRect(5, 0, 250, 25), "Authenticator",
		"Authenticator:", new BPopUpMenu("No Authenticators Found!"));
	fAuthenticatorField->SetDivider(fAuthenticatorField->StringWidth(
		fAuthenticatorField->Label()) + 10);
	fAuthenticatorField->Menu()->SetRadioMode(true);
	AddAuthenticators();
	fAuthenticationBox->SetLabel(fAuthenticatorField);
	
	rect = fAuthenticationBox->Bounds();
	rect.InsetBy(10, 0);
	rect.top = 25;
	rect.bottom = rect.top + 20;
	fUsername = new BTextControl(rect, "username", "Name: ", NULL, NULL);
	rect.top = rect.bottom + 5;
	rect.bottom = rect.top + 20;
	fPassword = new BTextControl(rect, "password", "Password: ", NULL, NULL);
	fPassword->TextView()->HideTyping(true);
	rect.top = rect.bottom + 5;
	rect.bottom = rect.top + 20;
	fSavePassword = new BCheckBox(rect, "SavePassword", "Save Password", NULL);
	
	fAuthenticationBox->AddChild(fUsername);
	fAuthenticationBox->AddChild(fPassword);
	fAuthenticationBox->AddChild(fSavePassword);
	
	AddChild(fDeviceBox);
	AddChild(fAuthenticationBox);
}


GeneralView::~GeneralView()
{
	// TODO: delete device add-on
}


void
GeneralView::Reload()
{
	// TODO: reload settings:
	// - select device and authenticator
	// - add device view (and move authenticator view to fit)
	// - set username, password, and checkbox
	
	fDeviceAddon = NULL;
	
	BMenuItem *item;
	for(int32 index = 0; index < fDeviceField->Menu()->CountItems(); index++) {
		item = fDeviceField->Menu()->ItemAt(index);
		if(item && item->Message() && item->Message()->FindPointer("Addon",
					reinterpret_cast<void**>(&fDeviceAddon)) == B_OK
				&& fDeviceAddon == Addon()->DeviceAddon())
			break;
	}
	
	if(fDeviceAddon == Addon()->DeviceAddon()) {
		item->SetMarked(true);
		ReloadDeviceView();
	} else {
		fDeviceAddon = NULL;
		item = fDeviceField->Menu()->FindMarked();
		if(item)
			item->SetMarked(false);
	}
	
	if(Addon()->CountAuthenticators() > 0) {
		BString kernelModule, authenticator;
		BMessage authentication;
		if(Addon()->Settings()->FindMessage("Authentication", &authentication) == B_OK)
			authentication.FindString("Authenticators", &authenticator);
		for(int32 index = 0; index < fDeviceField->Menu()->CountItems(); index++) {
			item = fDeviceField->Menu()->ItemAt(index);
			if(item && item->Message()
					&& item->Message()->FindString("KernelModuleName",
						&kernelModule) == B_OK && kernelModule == authenticator) {
				item->SetMarked(true);
				break;
			}
		}
	} else
		fAuthenticatorNone->SetMarked(true);
	
	fUsername->SetText(Addon()->Username());
	fPassword->SetText(Addon()->Password());
	fSavePassword->SetValue(Addon()->HasPassword());
}


const char*
GeneralView::DeviceName() const
{
	if(fDeviceAddon)
		return fDeviceAddon->KernelModuleName();
	
	return NULL;
}


const char*
GeneralView::AuthenticatorName() const
{
	BMenuItem *marked = fAuthenticatorField->Menu()->FindMarked();
	if(marked && marked != fAuthenticatorNone)
		return marked->Message()->FindString("KernelModuleName");
	
	return NULL;
}


bool
GeneralView::IsDeviceModified() const
{
	if(fDeviceAddon != Addon()->DeviceAddon())
		return true;
	else if(fDeviceAddon) {
		bool settings, profile;
		fDeviceAddon->IsModified(settings, profile);
		return settings;
	}
	
	return false;
}


void
GeneralView::AttachedToWindow()
{
	SetViewColor(Parent()->ViewColor());
	fDeviceField->Menu()->SetTargetForItems(this);
	fAuthenticatorField->Menu()->SetTargetForItems(this);
	fUsername->SetTarget(this);
	fPassword->SetTarget(this);
}


void
GeneralView::MessageReceived(BMessage *message)
{
	switch(message->what) {
		case MSG_SELECT_DEVICE:
			if(message->FindPointer("Addon", reinterpret_cast<void**>(&fDeviceAddon))
					!= B_OK)
				fDeviceAddon = NULL;
			else {
				if(fDeviceAddon != Addon()->DeviceAddon())
					fDeviceAddon->LoadSettings(Addon()->Settings(), Addon()->Profile(),
						Addon()->IsNew());
				
				ReloadDeviceView();
			}
		break;
		
		case MSG_SELECT_AUTHENTICATOR:
		break;
		
		default:
			BView::MessageReceived(message);
	}
}


void
GeneralView::ReloadDeviceView()
{
	// first remove existing device view(s)
	while(fDeviceBox->CountChildren() > 0)
		fDeviceBox->RemoveChild(fDeviceBox->ChildAt(0));
	
	BRect rect(fDeviceBox->Bounds());
	rect.top = 25;
	float width, height;
	if(!fDeviceAddon->GetPreferredSize(&width, &height)) {
		width = 50;
		height = 50;
	}
	
	if(width > rect.Width())
		width = rect.Width();
	if(height < 10)
		height = 10;
	
	rect.InsetBy((rect.Width() - width) / 2, 0);
	rect.bottom = rect.top + height;
	float deltaY = rect.Height() - fDeviceBox->Bounds().Height();
	fDeviceBox->ResizeBy(0, deltaY);
	fAuthenticationBox->MoveBy(0, deltaY);
	
	BView *deviceView = fDeviceAddon->CreateView(rect.LeftTop());
	if(deviceView)
		fDeviceBox->AddChild(deviceView);
}


void
GeneralView::AddDevices()
{
	AddAddonsToMenu(fDeviceField->Menu(), "Device", MSG_SELECT_DEVICE);
}


void
GeneralView::AddAuthenticators()
{
	fAuthenticatorNone = new BMenuItem("None", new BMessage(MSG_SELECT_AUTHENTICATOR));
	fAuthenticatorField->Menu()->AddItem(fAuthenticatorNone);
	fAuthenticatorNone->SetMarked(true);
	fAuthenticatorField->Menu()->AddSeparatorItem();
	
	BMessage addon;
	for(int32 index = 0;
			Addon()->Addons()->FindMessage("Authenticator", index, &addon) == B_OK;
			index++) {
		BMessage *message = new BMessage(MSG_SELECT_AUTHENTICATOR);
		message->AddString("Authenticator", addon.FindString("KernelModuleName"));
		
		BString name, technicalName, friendlyName;
		bool hasTechnicalName
			= (addon.FindString("TechnicalName", &technicalName) == B_OK);
		bool hasFriendlyName
			= (addon.FindString("FriendlyName", &friendlyName) == B_OK);
		if(hasTechnicalName) {
			name << technicalName;
			if(hasFriendlyName)
				name << " (";
		}
		if(hasFriendlyName) {
			name << friendlyName;
			if(hasTechnicalName)
				name << ")";
		}
		
		int32 insertAt = FindNextAddonInsertionIndex(fAuthenticatorField->Menu(),
			name, 2);
		fAuthenticatorField->Menu()->AddItem(new BMenuItem(name.String(), message),
			insertAt);
	}
}


void
GeneralView::AddAddonsToMenu(BMenu *menu, const char *type, uint32 what)
{
	DialUpAddon *addon;
	for(int32 index = 0; Addon()->Addons()->FindPointer(type, index,
			reinterpret_cast<void**>(&addon)) == B_OK; index++) {
		if(!addon || (!addon->FriendlyName() && !addon->TechnicalName()))
			continue;
		
		BMessage *message = new BMessage(what);
		message->AddPointer("Addon", addon);
		
		BString name;
		if(addon->TechnicalName()) {
			name << addon->TechnicalName();
			if(addon->FriendlyName())
				name << " (";
		}
		if(addon->FriendlyName()) {
			name << addon->FriendlyName();
			if(addon->TechnicalName())
				name << ")";
		}
		
		int32 insertAt = FindNextAddonInsertionIndex(menu, name);
		menu->AddItem(new BMenuItem(name.String(), message), insertAt);
	}
}


int32
GeneralView::FindNextAddonInsertionIndex(BMenu *menu, const BString& name,
	int32 index = 0)
{
	BMenuItem *item;
	for(; index < menu->CountItems(); index++) {
		item = menu->ItemAt(index);
		if(item && name.ICompare(item->Label()) <= 0)
			return index;
	}
	
	return index;
}
