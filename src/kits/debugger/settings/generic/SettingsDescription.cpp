/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "SettingsDescription.h"

#include "Setting.h"


SettingsDescription::SettingsDescription()
{
}


SettingsDescription::~SettingsDescription()
{
	for (int32 i = 0; Setting* setting = SettingAt(i); i++)
		setting->ReleaseReference();
}


int32
SettingsDescription::CountSettings() const
{
	return fSettings.CountItems();
}


Setting*
SettingsDescription::SettingAt(int32 index) const
{
	return fSettings.ItemAt(index);
}


Setting*
SettingsDescription::SettingByID(const char* id) const
{
	for (int32 i = 0; Setting* setting = fSettings.ItemAt(i); i++) {
		if (strcmp(setting->ID(), id) == 0)
			return setting;
	}

	return NULL;
}


bool
SettingsDescription::AddSetting(Setting* setting)
{
	if (!fSettings.AddItem(setting))
		return false;

	setting->AcquireReference();
	return true;
}
