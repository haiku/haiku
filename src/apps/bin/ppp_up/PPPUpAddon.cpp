/*
 * Copyright 2003-2004, Waldemar Kornewald <Waldemar.Kornewald@web.de>
 * Distributed under the terms of the MIT License.
 */

#include "ConnectionView.h"

#include "InterfaceUtils.h"
#include "MessageDriverSettingsUtils.h"
#include "settings_tools.h"

#include <Box.h>
#include <Button.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <StringView.h>

#include <PPPDefs.h>


PPPUpAddon::PPPUpAddon(BMessage *addons, ConnectionView *connectionView)
	: DialUpAddon(addons),
	fAskBeforeConnecting(false),
	fHasPassword(false),
	fAuthenticatorsCount(0),
	fSettings(NULL),
	fProfile(NULL),
	fConnectionView(connectionView)
{
}


PPPUpAddon::~PPPUpAddon()
{
}


DialUpAddon*
PPPUpAddon::FindDevice(const BString& moduleName) const
{
	DialUpAddon *addon;
	for(int32 index = 0; Addons()->FindPointer(DUN_DEVICE_ADDON_TYPE, index,
			reinterpret_cast<void**>(&addon)) == B_OK; index++)
		if(addon && moduleName == addon->KernelModuleName())
			return addon;
	
	return NULL;
}


bool
PPPUpAddon::LoadSettings(BMessage *settings, BMessage *profile, bool isNew)
{
	fIsNew = isNew;
	fAskBeforeConnecting = fHasPassword = false;
	fDeviceName = fUsername = fPassword = "";
	fDeviceAddon = NULL;
	fAuthenticatorsCount = 0;
	fSettings = settings;
	fProfile = profile;
	
	fConnectionView->Reload();
	
	if(!settings || !profile || isNew)
		return true;
	
	BMessage parameter;
	int32 index = 0;
	const char *value;
	if(FindMessageParameter(PPP_ASK_BEFORE_CONNECTING_KEY, *fSettings, &parameter,
			&index) && parameter.FindString(MDSU_VALUES, &value) == B_OK) {
		if(get_boolean_value(value, false))
			fAskBeforeConnecting = true;
		
		parameter.AddBool(MDSU_VALID, true);
		fSettings->ReplaceMessage(MDSU_PARAMETERS, index, &parameter);
	}
	
	if(!LoadDeviceSettings())
		return false;
	
	if(!LoadAuthenticationSettings())
		return false;
	
	fConnectionView->Reload();
		// reload new settings
	
	return true;
}


bool
PPPUpAddon::LoadDeviceSettings()
{
	int32 index = 0;
	BMessage device;
	if(!FindMessageParameter(PPP_DEVICE_KEY, *fSettings, &device, &index))
		return false;
	
	if(device.FindString(MDSU_VALUES, &fDeviceName) != B_OK)
		return false;
	
	device.AddBool(MDSU_VALID, true);
	fSettings->ReplaceMessage(MDSU_PARAMETERS, index, &device);
	
	fDeviceAddon = FindDevice(fDeviceName);
	if(!fDeviceAddon)
		return false;
	
	return fDeviceAddon->LoadSettings(fSettings, fProfile, false);
}


bool
PPPUpAddon::LoadAuthenticationSettings()
{
	// we only handle the profile (although settings could contain different data)
	int32 itemIndex = 0;
	BMessage authentication, item;
	
	if(!FindMessageParameter(PPP_AUTHENTICATOR_KEY, *fProfile, &item, &itemIndex))
		return true;
	
	// find authenticators (though we load all authenticators, we only use one)
	BString name;
	for(int32 index = 0; item.FindString(MDSU_VALUES, index, &name) == B_OK; index++) {
		BMessage authenticator;
		if(!GetAuthenticator(name, &authenticator))
			return false;
				// fatal error: we do not know how to handle this authenticator
		
		MarkAuthenticatorAsValid(name);
		authentication.AddString(PPP_UP_AUTHENTICATORS, name);
		fAuthenticatorName = name;
		++fAuthenticatorsCount;
	}
	
	fSettings->AddMessage(PPP_UP_AUTHENTICATION, &authentication);
	
	bool hasUsername = false;
		// a username must be present
	
	// load username and password
	BMessage parameter;
	int32 parameterIndex = 0;
	if(FindMessageParameter("User", item, &parameter, &parameterIndex)
			&& parameter.FindString(MDSU_VALUES, &fUsername) == B_OK) {
		hasUsername = true;
		parameter.AddBool(MDSU_VALID, true);
		item.ReplaceMessage(MDSU_PARAMETERS, parameterIndex, &parameter);
	}
	
	parameterIndex = 0;
	if(FindMessageParameter("Password", item, &parameter, &parameterIndex)
			&& parameter.FindString(MDSU_VALUES, &fPassword) == B_OK) {
		fHasPassword = true;
		parameter.AddBool(MDSU_VALID, true);
		item.ReplaceMessage(MDSU_PARAMETERS, parameterIndex, &parameter);
	}
	
	// tell DUN whether everything is valid
	if(hasUsername)
		item.AddBool(MDSU_VALID, true);
	
	fProfile->ReplaceMessage(MDSU_PARAMETERS, itemIndex, &item);
	
	return true;
}


bool
PPPUpAddon::HasTemporaryProfile() const
{
	return fConnectionView->HasTemporaryProfile();
}


void
PPPUpAddon::IsModified(bool *settings, bool *profile) const
{
	if(!fSettings) {
		*settings = *profile = false;
		return;
	}
	
	bool deviceSettings, authenticationSettings, deviceProfile, authenticationProfile;
	
	IsDeviceModified(&deviceSettings, &deviceProfile);
	IsAuthenticationModified(&authenticationSettings, &authenticationProfile);
	
	*settings = (deviceSettings || authenticationSettings);
	*profile = (deviceProfile || authenticationProfile);
}


void
PPPUpAddon::IsDeviceModified(bool *settings, bool *profile) const
{
	fConnectionView->IsDeviceModified(settings, profile);
}


void
PPPUpAddon::IsAuthenticationModified(bool *settings, bool *profile) const
{
	// currently we only support selecting one authenticator
	*profile = (*settings || fUsername != fConnectionView->Username()
		|| (fPassword != fConnectionView->Password() && fHasPassword)
		|| fHasPassword != fConnectionView->DoesSavePassword());
}


bool
PPPUpAddon::SaveSettings(BMessage *settings, BMessage *profile, bool saveTemporary)
{
	if(!fSettings || !settings)
		return false;
			// TODO: tell user that a device is needed (if we fail because of this)
	
	if(!DeviceAddon()->SaveSettings(settings, profile, saveTemporary))
		return false;
	
	if(CountAuthenticators() > 0) {
		BMessage authenticator;
		authenticator.AddString(MDSU_NAME, PPP_AUTHENTICATOR_KEY);
		authenticator.AddString(MDSU_VALUES, AuthenticatorName());
		settings->AddMessage(MDSU_PARAMETERS, &authenticator);
		
		BMessage username;
		username.AddString(MDSU_NAME, "User");
		username.AddString(MDSU_VALUES, fConnectionView->Username());
		authenticator.AddMessage(MDSU_PARAMETERS, &username);
		
		if(saveTemporary || fConnectionView->DoesSavePassword()) {
			// save password, too
			BMessage password;
			password.AddString(MDSU_NAME, "Password");
			password.AddString(MDSU_VALUES, fConnectionView->Password());
			authenticator.AddMessage(MDSU_PARAMETERS, &password);
		}
		
		profile->AddMessage(MDSU_PARAMETERS, &authenticator);
	}
	
	return true;
}


bool
PPPUpAddon::GetPreferredSize(float *width, float *height) const
{
	// this method is not used, just set values
	if(width)
		*width = 200;
	if(height)
		*height = 300;
	
	return true;
}


BView*
PPPUpAddon::CreateView(BPoint leftTop)
{
	return NULL;
		// ConnectionView is responsible for our UI
}


bool
PPPUpAddon::GetAuthenticator(const BString& moduleName, BMessage *entry) const
{
	if(!entry)
		return false;
	
	BString name;
	for(int32 index = 0; Addons()->FindMessage(DUN_AUTHENTICATOR_ADDON_TYPE, index,
			entry) == B_OK; index++) {
		entry->FindString("KernelModuleName", &name);
		if(name == moduleName)
			return true;
	}
	
	return false;
}


bool
PPPUpAddon::MarkAuthenticatorAsValid(const BString& moduleName)
{
	BMessage authenticator;
	int32 index = 0;
	BString name;
	
	for(; FindMessageParameter(PPP_AUTHENTICATOR_KEY, *fSettings, &authenticator,
			&index); index++) {
		authenticator.FindString("KernelModuleName", &name);
		if(name == moduleName) {
			authenticator.AddBool(MDSU_VALID, true);
			fSettings->ReplaceMessage(MDSU_PARAMETERS, index, &authenticator);
			return true;
		}
	}
	
	return false;
}
