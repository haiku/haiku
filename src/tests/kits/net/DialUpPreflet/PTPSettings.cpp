/*
 * Copyright 2005, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

#include <PTPSettings.h>

#include <PPPDefs.h>
#include <PPPManager.h>

// built-in add-ons
#include "ConnectionOptionsAddon.h"
#include "GeneralAddon.h"
#include "IPCPAddon.h"
#include "PPPoEAddon.h"

#include "MessageDriverSettingsUtils.h"
#include <TemplateList.h>

#include <Directory.h>
#include <File.h>
#include <driver_settings.h>


PTPSettings::PTPSettings()
{
}


PTPSettings::~PTPSettings()
{
	// free known add-on types (these should free their known add-on types, etc.)
	DialUpAddon *addon;
	for(int32 index = 0;
			fAddons.FindPointer(DUN_DELETE_ON_QUIT, index,
				reinterpret_cast<void**>(&addon)) == B_OK;
			index++)
		delete addon;
}



const char*
PTPSettings::SessionPassword() const
{
	return fGeneralAddon->SessionPassword();
}


bool
PTPSettings::LoadSettings(const char *interfaceName, bool isNew)
{
	if(interfaceName && strlen(interfaceName) == 0)
		return false;
	
	fCurrent = interfaceName ? interfaceName : "";
	fSettings.MakeEmpty();
	BMessage *settingsPointer = interfaceName ? &fSettings : NULL;
	
	if(interfaceName && !isNew) {
		BString name("ptpnet/");
		name << fCurrent;
		if(!ReadMessageDriverSettings(name.String(), &fSettings))
			return false;
	}
	
	DialUpAddon *addon;
	for(int32 index = 0; fAddons.FindPointer(DUN_TAB_ADDON_TYPE, index,
			reinterpret_cast<void**>(&addon)) == B_OK; index++) {
		if(!addon)
			continue;
		
		if(!addon->LoadSettings(settingsPointer, isNew))
			return false;
	}
	
	// TODO: check if settings are valid
	
	return true;
}


void
PTPSettings::IsModified(bool *settings)
{
	*settings = false;
	bool addonSettingsChanged;
	
	DialUpAddon *addon;
	for(int32 index = 0; fAddons.FindPointer(DUN_TAB_ADDON_TYPE, index,
			reinterpret_cast<void**>(&addon)) == B_OK; index++) {
		if(!addon)
			continue;
		
		addon->IsModified(&addonSettingsChanged);
		if(addonSettingsChanged)
			*settings = true;
	}
}


bool
PTPSettings::SaveSettings(BMessage *settings)
{
	if(fCurrent.Length() == 0 || !settings)
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
	
	// prepare for next release with support different PTP types
	BMessage parameter;
	parameter.AddString(MDSU_NAME, "PTPType");
	parameter.AddString(MDSU_VALUES, "ppp");
	settings->AddMessage(MDSU_PARAMETERS, &parameter);
	settings->AddString("InterfaceName", fCurrent);
	
	for(int32 index = 0; index < addons.CountItems(); index++)
		if(!addons.ItemAt(index)->SaveSettings(settings))
			return false;
	
	return true;
}


bool
PTPSettings::SaveSettingsToFile()
{
	bool settingsChanged;
	IsModified(&settingsChanged);
	if(!settingsChanged)
		return true;
	
	BMessage settings;
	if(!SaveSettings(&settings))
		return false;
	
	BDirectory settingsDirectory;
	if(!PPPManager::GetSettingsDirectory(&settingsDirectory))
		return false;
	
	BFile file;
	if(settingsChanged) {
		settingsDirectory.CreateFile(fCurrent.String(), &file);
		WriteMessageDriverSettings(file, settings);
	}
	
	return true;
}


void
PTPSettings::LoadAddons()
{
	// Load built-in add-ons:
	// "Connection Options" tab
	ConnectionOptionsAddon *connectionOptionsAddon =
		new ConnectionOptionsAddon(&fAddons);
	fAddons.AddPointer(DUN_TAB_ADDON_TYPE, connectionOptionsAddon);
	fAddons.AddPointer(DUN_DELETE_ON_QUIT, connectionOptionsAddon);
	// "General" tab
	fGeneralAddon = new GeneralAddon(&fAddons);
	fAddons.AddPointer(DUN_TAB_ADDON_TYPE, fGeneralAddon);
	fAddons.AddPointer(DUN_DELETE_ON_QUIT, fGeneralAddon);
	// "IPCP" protocol
	IPCPAddon *ipcpAddon = new IPCPAddon(&fAddons);
	fAddons.AddPointer(DUN_TAB_ADDON_TYPE, ipcpAddon);
	fAddons.AddPointer(DUN_DELETE_ON_QUIT, ipcpAddon);
	// "PPPoE" device
	PPPoEAddon *pppoeAddon = new PPPoEAddon(&fAddons);
	fAddons.AddPointer(DUN_DEVICE_ADDON_TYPE, pppoeAddon);
	fAddons.AddPointer(DUN_DELETE_ON_QUIT, pppoeAddon);
	
	// "PAP" authenticator
	BMessage addon;
	addon.AddString("FriendlyName", "Plain-text Authentication");
	addon.AddString("TechnicalName", "PAP");
	addon.AddString("KernelModuleName", "pap");
	fAddons.AddMessage(DUN_AUTHENTICATOR_ADDON_TYPE, &addon);
	// addon.MakeEmpty(); // for next authenticator
	
	// TODO: load all add-ons from the add-ons folder
}
