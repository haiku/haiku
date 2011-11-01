/*
 * Copyright 2011, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#include "GUITeamUISettings.h"

#include <Message.h>


GUITeamUISettings::GUITeamUISettings(const char* settingsID)
	:
	fID(settingsID)
{
}


GUITeamUISettings::GUITeamUISettings(const GUITeamUISettings& other)
{
	fID = other.fID;
}


GUITeamUISettings::~GUITeamUISettings()
{
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
	
	return error;
}


status_t
GUITeamUISettings::WriteTo(BMessage& archive) const
{
	status_t error = archive.AddString("ID", fID);
	
	return error;
}


TeamUISettings*
GUITeamUISettings::Clone() const
{
	GUITeamUISettings* settings = new GUITeamUISettings(fID.String());
	
	return settings;
}


GUITeamUISettings&
GUITeamUISettings::operator=(const GUITeamUISettings& other)
{
	fID = other.fID;
	return *this;
}
