/*
 * Copyright 2011, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "GuiTeamUiSettings.h"

#include <Message.h>


GuiTeamUiSettings::GuiTeamUiSettings()
{
}


GuiTeamUiSettings::GuiTeamUiSettings(const char* settingsID)
	:
	fID(settingsID)
{
}


GuiTeamUiSettings::GuiTeamUiSettings(const GuiTeamUiSettings& other)
{
	if (_SetTo(other) != B_OK)
		throw std::bad_alloc();
}


GuiTeamUiSettings::~GuiTeamUiSettings()
{
	_Unset();
}


team_ui_settings_type
GuiTeamUiSettings::Type() const
{
	return TEAM_UI_SETTINGS_TYPE_GUI;
}


const char*
GuiTeamUiSettings::ID() const
{
	return fID.String();
}


status_t
GuiTeamUiSettings::SetTo(const BMessage& archive)
{
	status_t error = archive.FindString("ID", &fID);
	if (error != B_OK)
		return error;

	error = archive.FindMessage("values", &fValues);

	return error;
}


status_t
GuiTeamUiSettings::WriteTo(BMessage& archive) const
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


TeamUiSettings*
GuiTeamUiSettings::Clone() const
{
	GuiTeamUiSettings* settings = new(std::nothrow) GuiTeamUiSettings(fID);

	if (settings == NULL)
		return NULL;

	if (settings->_SetTo(*this) != B_OK) {
		delete settings;
		return NULL;
	}

	return settings;
}


bool
GuiTeamUiSettings::AddSettings(const char* settingID, const BMessage& data)
{
	fValues.RemoveName(settingID);

	return fValues.AddMessage(settingID, &data) == B_OK;
}


status_t
GuiTeamUiSettings::Settings(const char* settingID, BMessage &data) const
{
	return fValues.FindMessage(settingID, &data);
}


const BMessage&
GuiTeamUiSettings::Values() const
{
	return fValues;
}


GuiTeamUiSettings&
GuiTeamUiSettings::operator=(const GuiTeamUiSettings& other)
{
	if (_SetTo(other) != B_OK)
		throw std::bad_alloc();

	return *this;
}


status_t
GuiTeamUiSettings::_SetTo(const GuiTeamUiSettings& other)
{
	_Unset();

	fID = other.fID;

	fValues = other.fValues;

	return B_OK;
}


void
GuiTeamUiSettings::_Unset()
{
	fID.Truncate(0);

	fValues.MakeEmpty();
}
