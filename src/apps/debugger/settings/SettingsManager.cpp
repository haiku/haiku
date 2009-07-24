/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "SettingsManager.h"

#include <new>

#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>

#include <AutoDeleter.h>
#include <AutoLocker.h>

#include "TeamSettings.h"


static const char* const kSettingsDirPath		= "Debugger";
static const char* const kGlobalSettingsName	= "Global";
static const int32 kMaxRecentTeamSettings		= 10;


SettingsManager::SettingsManager()
	:
	fLock("settings manager"),
	fRecentTeamSettings(kMaxRecentTeamSettings, true)
{
}


SettingsManager::~SettingsManager()
{
	_Unset();
}


status_t
SettingsManager::Init()
{
	// check the lock
	status_t error = fLock.InitCheck();
	if (error != B_OK)
		return error;

	// get and create our settings directory
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &fSettingsPath, true) == B_OK
		&& fSettingsPath.Append(kSettingsDirPath) == B_OK
		&& create_directory(fSettingsPath.Path(), 0700) == B_OK
		&& fSettingsPath.Append(kGlobalSettingsName) == B_OK) {
		// load the settings
		_LoadSettings();
	} else {
		// something went wrong -- clear the path
		fSettingsPath.Unset();
	}

	return B_OK;
}


status_t
SettingsManager::LoadTeamSettings(const char* teamName, TeamSettings& settings)
{
	AutoLocker<BLocker> locker(fLock);

	int32 index = _TeamSettingsIndex(teamName);
	if (index < 0)
		return B_ENTRY_NOT_FOUND;

	try {
		settings = *fRecentTeamSettings.ItemAt(index);
		return B_OK;
	} catch (std::bad_alloc) {
		return B_NO_MEMORY;
	}
}


status_t
SettingsManager::SaveTeamSettings(const TeamSettings& _settings)
{
	AutoLocker<BLocker> locker(fLock);

	TeamSettings* settings;
	int32 index = _TeamSettingsIndex(_settings.TeamName());
	if (index >= 0) {
		settings = fRecentTeamSettings.RemoveItemAt(index);
	} else {
		settings = new(std::nothrow) TeamSettings;
		if (settings == NULL)
			return B_NO_MEMORY;

		// enforce recent limit
		while (fRecentTeamSettings.CountItems() >= kMaxRecentTeamSettings)
			delete fRecentTeamSettings.RemoveItemAt(0);

	}
	ObjectDeleter<TeamSettings> settingsDeleter(settings);

	try {
		*settings = _settings;
		if (!fRecentTeamSettings.AddItem(settings))
			return B_NO_MEMORY;
		settingsDeleter.Detach();

		return _SaveSettings();
	} catch (std::bad_alloc) {
		return B_NO_MEMORY;
	}
}


void
SettingsManager::_Unset()
{
	fRecentTeamSettings.MakeEmpty();
}


status_t
SettingsManager::_LoadSettings()
{
	_Unset();

	if (fSettingsPath.Path() == NULL)
		return B_ENTRY_NOT_FOUND;

	// read the settings file
	BFile file;
	status_t error = file.SetTo(fSettingsPath.Path(), B_READ_ONLY);
	if (error != B_OK)
		return error;

	BMessage archive;
	error = archive.Unflatten(&file);
	if (error != B_OK)
		return error;

	// unarchive the recent team settings
	BMessage childArchive;
	for (int32 i = 0; archive.FindMessage("teamSettings", i, &childArchive)
			== B_OK; i++) {
		TeamSettings* settings = new(std::nothrow) TeamSettings;
		if (settings == NULL)
			return B_NO_MEMORY;

		error = settings->SetTo(childArchive);
		if (error != B_OK) {
			delete settings;
			continue;
		}

		if (!fRecentTeamSettings.AddItem(settings)) {
			delete settings;
			return B_NO_MEMORY;
		}
	}

	return B_OK;
}


status_t
SettingsManager::_SaveSettings()
{
	if (fSettingsPath.Path() == NULL)
		return B_ENTRY_NOT_FOUND;

	// archive the recent team settings
	BMessage archive;
	for (int32 i = 0; TeamSettings* settings = fRecentTeamSettings.ItemAt(i);
			i++) {
		BMessage childArchive;
		status_t error = settings->WriteTo(childArchive);
		if (error != B_OK)
			return error;

		error = archive.AddMessage("teamSettings", &childArchive);
		if (error != B_OK)
			return error;
	}

	// open the settings file
	BFile file;
	status_t error = file.SetTo(fSettingsPath.Path(),
		B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (error != B_OK)
		return error;

	return archive.Flatten(&file);
}


int32
SettingsManager::_TeamSettingsIndex(const char* teamName) const
{
	for (int32 i = 0; TeamSettings* settings = fRecentTeamSettings.ItemAt(i);
			i++) {
		if (settings->TeamName() == teamName)
			return i;
	}

	return -1;
}
