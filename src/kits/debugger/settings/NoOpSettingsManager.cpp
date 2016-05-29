/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "NoOpSettingsManager.h"


NoOpSettingsManager::NoOpSettingsManager()
	:
	SettingsManager()
{
}


NoOpSettingsManager::~NoOpSettingsManager()
{
}


status_t
NoOpSettingsManager::LoadTeamSettings(const char* teamName,
	TeamSettings& settings)
{
	return B_OK;
}


status_t
NoOpSettingsManager::SaveTeamSettings(const TeamSettings& settings)
{
	return B_OK;
}



