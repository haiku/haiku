/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H


#include <Locker.h>
#include <Path.h>

#include <ObjectList.h>


class TeamSettings;


class SettingsManager {
public:
								SettingsManager();
								~SettingsManager();

			status_t			Init();

			status_t			LoadTeamSettings(const char* teamName,
									TeamSettings& settings);
			status_t			SaveTeamSettings(const TeamSettings& settings);

private:
			typedef BObjectList<TeamSettings> TeamSettingsList;

private:
			void				_Unset();

			status_t			_LoadSettings();
			status_t			_SaveSettings();

			int32				_TeamSettingsIndex(const char* teamName) const;

private:
			BLocker				fLock;
			BPath				fSettingsPath;
			TeamSettingsList	fRecentTeamSettings;	// oldest is first
};


#endif	// SETTINGS_MANAGER_H
