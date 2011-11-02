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
	archive.MakeEmpty();
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
	GUITeamUISettings* settings = new(std::nothrow) GUITeamUISettings(fID);

	if (settings == NULL)
		return NULL;

	if (settings->_SetTo(*this) != B_OK) {
		delete settings;
		return NULL;
	}

	return settings;
}


bool
GUITeamUISettings::SetValue(const char* settingID, const BVariant& value)
{
	fValues.RemoveName(settingID);

	return value.AddToMessage(fValues, settingID) == B_OK;
}


status_t
GUITeamUISettings::Value(const char* settingID, BVariant &value) const
{
	return value.SetFromMessage(fValues, settingID);
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

	fID = other.fID;

	fValues = other.fValues;

	return B_OK;
}


void
GUITeamUISettings::_Unset()
{
	fID.Truncate(0);

	fValues.MakeEmpty();
}
