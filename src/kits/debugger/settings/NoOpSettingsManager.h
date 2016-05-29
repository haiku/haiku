/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef NOOP_SETTINGS_MANAGER_H
#define NOOP_SETTINGS_MANAGER_H

#include "SettingsManager.h"


class NoOpSettingsManager : public SettingsManager {
public:
								NoOpSettingsManager();
	virtual						~NoOpSettingsManager();

	virtual	status_t			LoadTeamSettings(const char* teamName,
									TeamSettings& settings);
	virtual	status_t			SaveTeamSettings(const TeamSettings& settings);
};


#endif	// NOOP_SETTINGS_MANAGER_H
