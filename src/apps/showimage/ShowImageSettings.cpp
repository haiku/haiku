/*
 * Copyright 2003-2011 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer, laplace@haiku-os.org
 */


#include "ShowImageSettings.h"

#include <File.h>
#include <FindDirectory.h>
#include <Path.h>


ShowImageSettings::ShowImageSettings()
	:
	fLock("settings lock"),
	fUpdated(false)
{
	_Load();
}


ShowImageSettings::~ShowImageSettings()
{
	if (Lock()) {
		if (fUpdated)
			_Save();

		Unlock();
	}
}


bool
ShowImageSettings::Lock()
{
	return fLock.Lock();
}


void
ShowImageSettings::Unlock()
{
	fLock.Unlock();
}


bool
ShowImageSettings::GetBool(const char* name, bool defaultValue)
{
	bool value;
	if (fSettings.FindBool(name, &value) == B_OK)
		return value;

	return defaultValue;
}


int32
ShowImageSettings::GetInt32(const char* name, int32 defaultValue)
{
	int32 value;
	if (fSettings.FindInt32(name, &value) == B_OK)
		return value;

	return defaultValue;
}


float
ShowImageSettings::GetFloat(const char* name, float defaultValue)
{
	float value;
	if (fSettings.FindFloat(name, &value) == B_OK)
		return value;

	return defaultValue;
}


BRect
ShowImageSettings::GetRect(const char* name, BRect defaultValue)
{
	BRect value;
	if (fSettings.FindRect(name, &value) == B_OK)
		return value;

	return defaultValue;
}


bigtime_t
ShowImageSettings::GetTime(const char* name, bigtime_t defaultValue)
{
	int64 value;
	if (fSettings.FindInt64(name, &value) == B_OK)
		return value;

	return defaultValue;
}


const char*
ShowImageSettings::GetString(const char* name, const char* defaultValue)
{
	const char* value;
	if (fSettings.FindString(name, &value) == B_OK)
		return value;

	return defaultValue;
}


void
ShowImageSettings::SetBool(const char* name, bool value)
{
	if (fSettings.HasBool(name))
		fSettings.ReplaceBool(name, value);
	else
		fSettings.AddBool(name, value);

	fUpdated = true;
}


void
ShowImageSettings::SetInt32(const char* name, int32 value)
{
	if (fSettings.HasInt32(name))
		fSettings.ReplaceInt32(name, value);
	else
		fSettings.AddInt32(name, value);

	fUpdated = true;
}


void
ShowImageSettings::SetFloat(const char* name, float value)
{
	if (fSettings.HasFloat(name))
		fSettings.ReplaceFloat(name, value);
	else
		fSettings.AddFloat(name, value);

	fUpdated = true;
}


void
ShowImageSettings::SetRect(const char* name, BRect value)
{
	if (fSettings.HasRect(name))
		fSettings.ReplaceRect(name, value);
	else
		fSettings.AddRect(name, value);

	fUpdated = true;
}


void
ShowImageSettings::SetTime(const char* name, bigtime_t value)
{
	if (fSettings.ReplaceInt64(name, value) != B_OK)
		fSettings.AddInt64(name, value);

	fUpdated = true;
}


void
ShowImageSettings::SetString(const char* name, const char* value)
{
	if (fSettings.HasString(name))
		fSettings.ReplaceString(name, value);
	else
		fSettings.AddString(name, value);

	fUpdated = true;
}


bool
ShowImageSettings::_OpenSettingsFile(BFile* file, bool forReading)
{
	BPath path;
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status != B_OK)
		return false;

	path.Append("ShowImage_settings");

	return file->SetTo(path.Path(), forReading
		? B_READ_ONLY : B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE) == B_OK;
}


void
ShowImageSettings::_Load()
{
	BFile file;
	if (_OpenSettingsFile(&file, true))
		fSettings.Unflatten(&file);
}


void
ShowImageSettings::_Save()
{
	BFile file;
	if (_OpenSettingsFile(&file, false))
		fSettings.Flatten(&file);
}
