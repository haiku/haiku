/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef SETTINGS_DESCRIPTION_H
#define SETTINGS_DESCRIPTION_H


#include <ObjectList.h>
#include <Referenceable.h>


class Setting;


class SettingsDescription : public BReferenceable {
public:
								SettingsDescription();
	virtual						~SettingsDescription();

			int32				CountSettings() const;
			Setting*			SettingAt(int32 index) const;
			Setting*			SettingByID(const char* id) const;

			bool				AddSetting(Setting* setting);

private:
			typedef BObjectList<Setting> SettingsList;

private:
			SettingsList		fSettings;
};


#endif	// SETTINGS_DESCRIPTION_H
