/*
 * Copyright 2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#include "NetworkTargetHostInterfaceInfo.h"

#include <AutoDeleter.h>

#include "NetworkTargetHostInterface.h"
#include "SettingsDescription.h"
#include "Setting.h"


static const char* kHostnameSetting = "hostname";
static const char* kPortSetting = "port";


NetworkTargetHostInterfaceInfo::NetworkTargetHostInterfaceInfo()
	:
	TargetHostInterfaceInfo("Network"),
	fDescription(NULL)
{
}


NetworkTargetHostInterfaceInfo::~NetworkTargetHostInterfaceInfo()
{
	delete fDescription;
}


status_t
NetworkTargetHostInterfaceInfo::Init()
{
	fDescription = new(std::nothrow) SettingsDescription;
	if (fDescription == NULL)
		return B_NO_MEMORY;

	Setting* setting = new(std::nothrow) StringSettingImpl(kHostnameSetting,
		"Hostname", "");
	if (setting == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<Setting> settingDeleter(setting);
	if (!fDescription->AddSetting(setting))
		return B_NO_MEMORY;

	settingDeleter.Detach();
	setting = new(std::nothrow) BoundedSettingImpl(kPortSetting, "Port",
		(uint16)0, (uint16)65535, (uint16)8305);
	if (setting == NULL)
		return B_NO_MEMORY;
	if (!fDescription->AddSetting(setting)) {
		delete setting;
		return B_NO_MEMORY;
	}

	return B_OK;
}


bool
NetworkTargetHostInterfaceInfo::IsLocal() const
{
	return false;
}


bool
NetworkTargetHostInterfaceInfo::IsConfigured(Settings* settings) const
{
	return true;
}


SettingsDescription*
NetworkTargetHostInterfaceInfo::GetSettingsDescription() const
{
	return fDescription;
}


status_t
NetworkTargetHostInterfaceInfo::CreateInterface(Settings* settings,
	TargetHostInterface*& _interface) const
{
	NetworkTargetHostInterface* interface
		= new(std::nothrow) NetworkTargetHostInterface;
	if (interface == NULL)
		return B_NO_MEMORY;

	status_t error = interface->Init(settings);
	if (error != B_OK) {
		delete interface;
		return error;
	}

	_interface = interface;
	return B_OK;
}

