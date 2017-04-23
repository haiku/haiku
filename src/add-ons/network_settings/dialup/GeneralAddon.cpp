/*
 * Copyright 2003-2004 Waldemar Kornewald. All rights reserved.
 * Copyright 2017 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

//-----------------------------------------------------------------------
// GeneralAddon saves the loaded settings.
// GeneralView saves the current settings.
//-----------------------------------------------------------------------

#include "GeneralAddon.h"

#include "InterfaceUtils.h"
#include "MessageDriverSettingsUtils.h"

#include <Box.h>
#include <Button.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <LayoutBuilder.h>
#include <PopUpMenu.h>
#include <StringView.h>

#include <PPPDefs.h>


// message constants
static const uint32 kMsgSelectDevice = 'SELD';
static const uint32 kMsgSelectAuthenticator = 'SELA';

// labels
static const char *kLabelGeneral = "General";
static const char *kLabelDevice = "Device: ";
static const char *kLabelNoDevicesFound = "No Devices Found!";
static const char *kLabelAuthenticator = "Login: ";
static const char *kLabelNoAuthenticatorsFound = "No Authenticators Found!";
static const char *kLabelName = "Username: ";
static const char *kLabelPassword = "Password: ";
static const char *kLabelSavePassword = "Save Password";
static const char *kLabelNone = "None";

// string constants for information saved in the settings message
static const char *kGeneralTabAuthentication = "Authentication";
static const char *kGeneralTabAuthenticators = "Authenticators";


#define DEFAULT_AUTHENTICATOR		"PAP"
	// this authenticator is selected by default when creating a new interface


GeneralAddon::GeneralAddon(BMessage *addons)
	: DialUpAddon(addons),
	fHasPassword(false),
	fAuthenticatorsCount(0),
	fSettings(NULL),
	fProfile(NULL),
	fGeneralView(NULL)
{
}


GeneralAddon::~GeneralAddon()
{
}


bool
GeneralAddon::NeedsAuthenticationRequest() const
{
	return fGeneralView->AuthenticatorName();
}


DialUpAddon*
GeneralAddon::FindDevice(const BString& moduleName) const
{
	DialUpAddon *addon;
	for(int32 index = 0; Addons()->FindPointer(DUN_DEVICE_ADDON_TYPE, index,
			reinterpret_cast<void**>(&addon)) == B_OK; index++)
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

	if(fGeneralView)
		fGeneralView->Reload();
			// reset all views (empty settings)

	if(!settings || !profile || isNew)
		return true;

	if(!LoadDeviceSettings())
		return false;

	if(!LoadAuthenticationSettings())
		return false;

	if(fGeneralView)
		fGeneralView->Reload();
			// reload new settings

	return true;
}


bool
GeneralAddon::LoadDeviceSettings()
{
	int32 index = 0;
	BMessage device;
	if(!FindMessageParameter(PPP_DEVICE_KEY, *fSettings, &device, &index))
		return false;
			// TODO: tell user that device specification is missing

	if(device.FindString(MDSU_VALUES, &fDeviceName) != B_OK)
		return false;
			// TODO: tell user that device specification is missing

	device.AddBool(MDSU_VALID, true);
	fSettings->ReplaceMessage(MDSU_PARAMETERS, index, &device);

	fDeviceAddon = FindDevice(fDeviceName);
	if(!fDeviceAddon)
		return false;

	return fDeviceAddon->LoadSettings(fSettings, fProfile, false);
}


bool
GeneralAddon::LoadAuthenticationSettings()
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
		authentication.AddString(kGeneralTabAuthenticators, name);
		++fAuthenticatorsCount;
	}

	fSettings->AddMessage(kGeneralTabAuthentication, &authentication);

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
GeneralAddon::HasTemporaryProfile() const
{
	return fGeneralView->HasTemporaryProfile();
}


void
GeneralAddon::IsModified(bool *settings, bool *profile) const
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
GeneralAddon::IsDeviceModified(bool *settings, bool *profile) const
{
	fGeneralView->IsDeviceModified(settings, profile);
}


void
GeneralAddon::IsAuthenticationModified(bool *settings, bool *profile) const
{
	// currently we only support selecting one authenticator
	if(fAuthenticatorsCount == 0)
		*settings = fGeneralView->AuthenticatorName();
	else {
		BMessage authentication;
		if(fSettings->FindMessage(kGeneralTabAuthentication, &authentication) != B_OK) {
			*settings = *profile = false;
			return;
				// error!
		}

		BString authenticator;
		if(authentication.FindString(kGeneralTabAuthenticators,
				&authenticator) != B_OK) {
			*settings = *profile = false;
			return;
				// error!
		}

		*settings = (!fGeneralView->AuthenticatorName()
			|| authenticator != fGeneralView->AuthenticatorName());
	}

	*profile = (*settings || fUsername != fGeneralView->Username()
		|| (fPassword != fGeneralView->Password() && fHasPassword)
		|| fHasPassword != fGeneralView->DoesSavePassword());
}


bool
GeneralAddon::SaveSettings(BMessage *settings, BMessage *profile, bool saveTemporary)
{
	if(!fSettings || !settings || !fGeneralView->DeviceName())
		return false;
			// TODO: tell user that a device is needed (if we fail because of this)

	if(!fGeneralView->DeviceAddon() || !fGeneralView->DeviceAddon()->SaveSettings(
			settings, profile, saveTemporary))
		return false;

	if(fGeneralView->AuthenticatorName()) {
		BMessage authenticator;
		authenticator.AddString(MDSU_NAME, PPP_AUTHENTICATOR_KEY);
		authenticator.AddString(MDSU_VALUES, fGeneralView->AuthenticatorName());
		settings->AddMessage(MDSU_PARAMETERS, &authenticator);

		BMessage username;
		username.AddString(MDSU_NAME, "User");
		username.AddString(MDSU_VALUES, fGeneralView->Username());
		authenticator.AddMessage(MDSU_PARAMETERS, &username);

		if(saveTemporary || fGeneralView->DoesSavePassword()) {
			// save password, too
			BMessage password;
			password.AddString(MDSU_NAME, "Password");
			password.AddString(MDSU_VALUES, fGeneralView->Password());
			authenticator.AddMessage(MDSU_PARAMETERS, &password);
		}

		profile->AddMessage(MDSU_PARAMETERS, &authenticator);
	}

	return true;
}


bool
GeneralAddon::GetPreferredSize(float *width, float *height) const
{
	BRect rect;
	if(Addons()->FindRect(DUN_TAB_VIEW_RECT, &rect) != B_OK)
		rect.Set(0, 0, 200, 300);
			// set default values

	if(width)
		*width = rect.Width();
	if(height)
		*height = rect.Height();

	return true;
}


BView*
GeneralAddon::CreateView()
{
	if (!fGeneralView) {
		fGeneralView = new GeneralView(this);
		fGeneralView->Reload();
	}

	return fGeneralView;
}


bool
GeneralAddon::GetAuthenticator(const BString& moduleName, BMessage *entry) const
{
	if(!entry)
		return false;

	BString name;
	for(int32 index = 0; Addons()->FindMessage(DUN_AUTHENTICATOR_ADDON_TYPE, index,
			entry) == B_OK; index++) {
		entry->FindString("KernelModuleName", &name);
		if (name == moduleName)
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


GeneralView::GeneralView(GeneralAddon *addon)
	: BView(kLabelGeneral, 0),
	fAddon(addon)
{
	fDeviceBox = new BBox("Device");
	Addon()->Addons()->AddFloat(DUN_DEVICE_VIEW_WIDTH,
		fDeviceBox->Bounds().Width() - 10); // FIXME: remove

	fDeviceField = new BMenuField("Device",
		kLabelDevice, new BPopUpMenu(kLabelNoDevicesFound));
	fDeviceField->Menu()->SetRadioMode(true);
	AddDevices();
	fDeviceBox->SetLabel(fDeviceField);

	fAuthenticatorField = new BMenuField("Authenticator",
		kLabelAuthenticator, new BPopUpMenu(kLabelNoAuthenticatorsFound));
	fAuthenticatorField->Menu()->SetRadioMode(true);
	AddAuthenticators();

	fUsername = new BTextControl("username", kLabelName, NULL, NULL);
	fPassword = new BTextControl("password", kLabelPassword, NULL, NULL);
	fPassword->TextView()->HideTyping(true);

	fSavePassword = new BCheckBox("SavePassword", kLabelSavePassword, NULL);

	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_HALF_ITEM_SPACING)
		.SetInsets(B_USE_HALF_ITEM_INSETS)
		.AddGroup(B_HORIZONTAL)
			.Add(fDeviceBox)
			.AddGlue()
		.End()
		.Add(fAuthenticatorField)
		.Add(fUsername)
		.Add(fPassword)
		.Add(fSavePassword)
		.AddGlue()
	.End();
}


GeneralView::~GeneralView()
{
}


void
GeneralView::Reload()
{
	fDeviceAddon = NULL;

	BMenuItem *item = NULL;
	for(int32 index = 0; index < fDeviceField->Menu()->CountItems(); index++) {
		item = fDeviceField->Menu()->ItemAt(index);
		if(item && item->Message() && item->Message()->FindPointer("Addon",
					reinterpret_cast<void**>(&fDeviceAddon)) == B_OK
				&& fDeviceAddon == Addon()->DeviceAddon())
			break;
	}

	if(fDeviceAddon && fDeviceAddon == Addon()->DeviceAddon())
		item->SetMarked(true);
	else if(Addon()->IsNew() && fDeviceField->Menu()->CountItems() > 0) {
		item = fDeviceField->Menu()->ItemAt(0);
		item->SetMarked(true);
		item->Message()->FindPointer("Addon", reinterpret_cast<void**>(&fDeviceAddon));
		fDeviceAddon->LoadSettings(Addon()->Settings(), Addon()->Profile(), true);
	} else {
		fDeviceAddon = NULL;
		item = fDeviceField->Menu()->FindMarked();
		if(item)
			item->SetMarked(false);
	}

	if(Addon()->CountAuthenticators() > 0) {
		BString kernelModule, authenticator;
		BMessage authentication;
		if(Addon()->Settings()->FindMessage(kGeneralTabAuthentication,
				&authentication) == B_OK)
			authentication.FindString(kGeneralTabAuthenticators, &authenticator);
		BMenu *menu = fAuthenticatorField->Menu();
		for(int32 index = 0; index < menu->CountItems(); index++) {
			item = menu->ItemAt(index);
			if(item && item->Message()
					&& item->Message()->FindString("KernelModuleName",
						&kernelModule) == B_OK && kernelModule == authenticator) {
				item->SetMarked(true);
				break;
			}
		}
	} else if(Addon()->IsNew() && fAuthenticatorDefault)
		fAuthenticatorDefault->SetMarked(true);
	else
		fAuthenticatorNone->SetMarked(true);

	fUsername->SetText(Addon()->Username());
	fPassword->SetText(Addon()->Password());
	fSavePassword->SetValue(Addon()->HasPassword());

	ReloadDeviceView();
	UpdateControls();
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


void
GeneralView::IsDeviceModified(bool *settings, bool *profile) const
{
	if(fDeviceAddon != Addon()->DeviceAddon())
		*settings = *profile = true;
	else if(fDeviceAddon)
		fDeviceAddon->IsModified(settings, profile);
	else
		*settings = *profile = false;
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
		case kMsgSelectDevice:
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

		case kMsgSelectAuthenticator:
			UpdateControls();
		break;

		default:
			BView::MessageReceived(message);
	}
}


void
GeneralView::ReloadDeviceView()
{
	// first remove existing device view(s)
	while (fDeviceBox->CountChildren() > 1)
		fDeviceBox->RemoveChild(fDeviceBox->ChildAt(1));

	if (!fDeviceAddon)
		return;

	BView* deviceView = fDeviceAddon->CreateView();
	if (deviceView) {
		BLayoutBuilder::Group<>(fDeviceBox, B_VERTICAL)
			.Add(deviceView)
		.End();
	}
}


void
GeneralView::UpdateControls()
{
	BMenu *menu = fAuthenticatorField->Menu();
	int32 index = menu->IndexOf(menu->FindMarked());
	if (index < 0)
		fAuthenticatorNone->SetMarked(true);

	if (index == 0) {
		fUsername->SetEnabled(false);
		fPassword->SetEnabled(false);
		fSavePassword->SetEnabled(false);
	} else {
		fUsername->SetEnabled(true);
		fPassword->SetEnabled(true);
		fSavePassword->SetEnabled(true);
	}
}


void
GeneralView::AddDevices()
{
	AddAddonsToMenu(Addon()->Addons(), fDeviceField->Menu(), DUN_DEVICE_ADDON_TYPE,
		kMsgSelectDevice);
}


void
GeneralView::AddAuthenticators()
{
	fAuthenticatorDefault = NULL;
	fAuthenticatorNone = new BMenuItem(kLabelNone,
		new BMessage(kMsgSelectAuthenticator));
	fAuthenticatorField->Menu()->AddItem(fAuthenticatorNone);
	fAuthenticatorNone->SetMarked(true);
	fAuthenticatorField->Menu()->AddSeparatorItem();

	BMenuItem *item;
	BMessage addon;
	for(int32 index = 0;
			Addon()->Addons()->FindMessage(DUN_AUTHENTICATOR_ADDON_TYPE, index,
			&addon) == B_OK; index++) {
		BMessage *message = new BMessage(kMsgSelectAuthenticator);
		message->AddString("KernelModuleName", addon.FindString("KernelModuleName"));

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

		int32 insertAt = FindNextMenuInsertionIndex(fAuthenticatorField->Menu(),
			name.String(), 2);
		item = new BMenuItem(name.String(), message);
		if(hasTechnicalName && technicalName == DEFAULT_AUTHENTICATOR)
			fAuthenticatorDefault = item;
		fAuthenticatorField->Menu()->AddItem(item, insertAt);
	}
}
