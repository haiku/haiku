//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
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


GeneralAddon::GeneralAddon(BMessage *addons)
	: DialUpAddon(addons),
	fHasDevice(false),
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


bool
GeneralAddon::LoadSettings(BMessage *settings, BMessage *profile)
{
	fHasDevice = fHasPassword = false;
	fDevice = fUsername = fPassword = "";
	fAuthenticatorsCount = 0;
	fSettings = settings;
	fProfile = profile;
	if(!settings)
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
	fHasDevice = false;
	fDevice = "";
	
	return true;
}


bool
GeneralAddon::LoadAuthenticationSettings(BMessage *settings, BMessage *profile)
{
	fHasPassword = false;
	fUsername = fPassword = "";
	fAuthenticatorsCount = 0;
	
	// we only handle the profile (although settings could contain different data)
	BMessage authentication, *item = FindMessageParameter(PPP_AUTHENTICATOR_KEY,
		*profile);
	
	if(!item)
		return true;
	
	// find authenticators (though we load all authenticators, we only use one)
	BString name;
	for(int32 index = 0; item->FindString("values", index, &name) == B_OK; index++) {
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
	BMessage *parameter = FindMessageParameter("username", *item);
	if(parameter && parameter->FindString("values", &fUsername) == B_OK) {
		hasUsername = true;
		parameter->AddBool("Valid", true);
	}
	
	parameter = FindMessageParameter("password", *item);
	if(parameter && parameter->FindString("values", &fPassword) == B_OK) {
		fHasPassword = true;
		parameter->AddBool("Valid", true);
	}
	
	// tell DUN whether everything is valid
	if(hasUsername)
		item->AddBool("Valid", true);
	
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
	if(fHasDevice)
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
	if(!fSettings || !settings)
		return false;
	
	// TODO: save settings
	
	return true;
}


bool
GeneralAddon::GetPreferredSize(float *width, float *height) const
{
	if(!fSettings)
		return false;
	
	BRect rect(0,0,200,300);
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
	if(!fSettings)
		return NULL;
	
	BRect rect(0,0,200,300);
		// set default values
	Addons()->FindRect("TabViewRect", &rect);
	
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
	BMessage *authenticator;
	int32 index = 0;
	BString name;
	
	for(; (authenticator = FindMessageParameter(PPP_AUTHENTICATOR_KEY, *fSettings,
			&index)); index++) {
		authenticator->FindString("KernelModuleName", &name);
		if(name == moduleName) {
			authenticator->AddBool("Valid", true);
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
		+ 2 * 20 // size of controls
		+ 2 * 5; // space beween controls and bottom of box
	fAuthenticationBox = new BBox(rect, "Authentication");
	
	BString deviceLabel("Device: ");
	deviceLabel << "PPPoE";
		// TODO: get real device name
	BStringView *deviceLabelView = new BStringView(BRect(0,0,250,25), "Device",
		deviceLabel.String());
	fDeviceBox->SetLabel(deviceLabelView);
	
	fAuthenticator = new BMenuField(BRect(5,0,250,25), "Authenticator",
		"Authenticator:", new BPopUpMenu("No Authenticators Found!"));
	fAuthenticator->SetDivider(fAuthenticator->StringWidth(
		fAuthenticator->Label()) + 10);
	fAuthenticationBox->SetLabel(fAuthenticator);
	
	rect = fAuthenticationBox->Bounds();
	rect.InsetBy(10, 0);
	rect.top = 25;
	rect.bottom = rect.top + 20;
	fUsername = new BTextControl(rect, "username", "Name: ", NULL, NULL);
	rect.top = rect.bottom + 5;
	rect.bottom = rect.top + 20;
	fPassword = new BTextControl(rect, "password", "Password: ", NULL, NULL);
	fPassword->TextView()->HideTyping(true);
	
	fAuthenticationBox->AddChild(fUsername);
	fAuthenticationBox->AddChild(fPassword);
	
	AddChild(fDeviceBox);
	AddChild(fAuthenticationBox);
}


GeneralView::~GeneralView()
{
}


void
GeneralView::Reload()
{
	
}


void
GeneralView::AttachedToWindow()
{
	SetViewColor(Parent()->ViewColor());
}


void
GeneralView::MessageReceived(BMessage *message)
{
	switch(message->what) {
		
		default:
			BView::MessageReceived(message);
	}
}
