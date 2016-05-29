/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H


#include <SupportDefs.h>


class TeamSettings;


class SettingsManager {
public:
	virtual						~SettingsManager();

	virtual	status_t			LoadTeamSettings(const char* teamName,
									TeamSettings& settings) = 0;
	virtual	status_t			SaveTeamSettings(const TeamSettings& settings)
									= 0;
};


#endif	// SETTINGS_MANAGER_H
