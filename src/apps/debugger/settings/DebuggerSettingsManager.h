/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef DEBUGGER_SETTINGS_MANAGER_H
#define DEBUGGER_SETTINGS_MANAGER_H

#include "SettingsManager.h"

#include <Locker.h>
#include <Path.h>

#include <ObjectList.h>


class TeamUiSettingsFactory;


class DebuggerSettingsManager : public SettingsManager {
public:
								DebuggerSettingsManager();
								~DebuggerSettingsManager();

			status_t			Init(TeamUiSettingsFactory* factory);

	virtual	status_t			LoadTeamSettings(const char* teamName,
									TeamSettings& settings);
	virtual	status_t			SaveTeamSettings(const TeamSettings& settings);

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
			TeamUiSettingsFactory* fUiSettingsFactory;
};


#endif	// DEBUGGER_SETTINGS_MANAGER_H
