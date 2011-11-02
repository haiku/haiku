/*
 * Copyright 2011, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "GUITeamUISettings.h"

#include <Message.h>


GUITeamUISettings::GUITeamUISettings()
{
}


GUITeamUISettings::GUITeamUISettings(const char* settingsID)
	:
	fID(settingsID)
{
}


GUITeamUISettings::GUITeamUISettings(const GUITeamUISettings& other)
{
	if (_SetTo(other) != B_OK)
		throw std::bad_alloc();
}


GUITeamUISettings::~GUITeamUISettings()
{
	_Unset();
}


team_ui_settings_type
GUITeamUISettings::Type() const
{
	return TEAM_UI_SETTINGS_TYPE_GUI;
}


const char*
GUITeamUISettings::ID() const
{
	return fID.String();
}


status_t
GUITeamUISettings::SetTo(const BMessage& archive)
{
	status_t error = archive.FindString("ID", &fID);
	if (error != B_OK)
		return error;

	error = archive.FindMessage("values", &fValues);

	return error;
}


status_t
GUITeamUISettings::WriteTo(BMessage& archive) const
{
	status_t error = archive.AddString("ID", fID);
	if (error != B_OK)
		return error;

	error = archive.AddInt32("type", Type());
	if (error != B_OK)
		return error;

	error = archive.AddMessage("values", &fValues);

	return error;
}


TeamUISettings*
GUITeamUISettings::Clone() const
{
	GUITeamUISettings* settings = new(std::nothrow) GUITeamUISettings();

	if (settings == NULL)
		return NULL;

	if (settings->_SetTo(*this) != B_OK) {
		delete settings;
		return NULL;
	}

	return settings;
}


status_t
GUITeamUISettings::AddSetting(Setting* setting)
{
	if (!fSettings.AddItem(setting))
		return B_NO_MEMORY;

	setting->AcquireReference();

	return B_OK;
}


bool
GUITeamUISettings::SetValue(const Setting* setting, const BVariant& value)
{
	const char* fieldName = setting->ID();
	fValues.RemoveName(fieldName);

	return value.AddToMessage(fValues, fieldName) == B_OK;
}


BVariant
GUITeamUISettings::Value(const Setting* setting) const
{
	BVariant value;

	return value.SetFromMessage(fValues, setting->ID()) == B_OK ?
		value : setting->DefaultValue();
}


BVariant
GUITeamUISettings::Value(const char* settingID) const
{
	BVariant value;

	status_t result = value.SetFromMessage(fValues, settingID);

	if (result != B_OK) {
		for (int32 i = 0; i < fSettings.CountItems(); i++) {
			Setting* setting = fSettings.ItemAt(i);
			if (strcmp(setting->ID(), settingID) == 0) {
				value = setting->DefaultValue();
				break;
			}
		}
	}

	return value;
}


GUITeamUISettings&
GUITeamUISettings::operator=(const GUITeamUISettings& other)
{
	if (_SetTo(other) != B_OK)
		throw std::bad_alloc();

	return *this;
}


status_t
GUITeamUISettings::_SetTo(const GUITeamUISettings& other)
{
	_Unset();

	for (int32 i = 0; i < other.fSettings.CountItems(); i++) {
		Setting* setting = other.fSettings.ItemAt(i);
		if (!fSettings.AddItem(setting))
			return B_NO_MEMORY;

		setting->AcquireReference();
	}

	fValues = other.fValues;

	return B_OK;
}


void
GUITeamUISettings::_Unset()
{
	fID.Truncate(0);

	for (int32 i = 0; i < fSettings.CountItems(); i++)
		fSettings.ItemAt(i)->ReleaseReference();

	fSettings.MakeEmpty();
	fValues.MakeEmpty();
}
