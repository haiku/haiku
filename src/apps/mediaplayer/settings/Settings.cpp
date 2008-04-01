/*
 * Copyright 2008, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Fredrik Modéen <fredrik@modeen.se>
 */
 
#include "Settings.h"

#include <stdio.h>

#include <String.h>
#include <FindDirectory.h>
#include <File.h>

#include "TPreferences.h"

Settings::Settings(const char *filename)
	: fTPreferences(TPreferences(B_USER_CONFIG_DIRECTORY, filename))
{
}


void
Settings::LoadSettings(mpSettings &settings)
{
	BPath path;
	
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path, true) != B_OK)
		return;
	
	path.Append(SETTINGSFILENAME);
	
	BFile settingsFile(path.Path(), B_READ_ONLY);
	
	if (settingsFile.InitCheck() != B_OK) {
		_SetDefault(settings);
		return;
	}
	
	fTPreferences.Unflatten(&settingsFile);
	memset(&settings, 0, sizeof(settings));
	
	fTPreferences.FindInt8("autostart", (int8 *)&settings.autostart);
	fTPreferences.FindInt8("closeWhenDonePlayingMovie", 
		(int8 *)&settings.closeWhenDonePlayingMovie);
	fTPreferences.FindInt8("closeWhenDonePlayingSound", 
		(int8 *)&settings.closeWhenDonePlayingSound);
	fTPreferences.FindInt8("loopMovie", (int8 *)&settings.loopMovie);
	fTPreferences.FindInt8("loopSound", (int8 *)&settings.loopSound);
	fTPreferences.FindInt8("fullVolume", (int8 *)&settings.fullVolume);
	fTPreferences.FindInt8("halfVolume", (int8 *)&settings.halfVolume);
	fTPreferences.FindInt8("mute", (int8 *)&settings.mute);
			
	fTPreferences.Flatten(&settingsFile);
}


void
Settings::SaveSettings(const mpSettings &settings)
{
	BPath path;
	
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path, true) != B_OK)
		return;
	
	path.Append(SETTINGSFILENAME);
	
	BFile settingsFile(path.Path(), B_READ_WRITE | B_CREATE_FILE);
	
	fTPreferences.Unflatten(&settingsFile);

	fTPreferences.SetInt8("autostart", settings.autostart);
	fTPreferences.SetInt8("closeWhenDonePlayingMovie", 
		settings.closeWhenDonePlayingMovie);
	fTPreferences.SetInt8("closeWhenDonePlayingSound", 
		settings.closeWhenDonePlayingSound);
	fTPreferences.SetInt8("loopMovie", settings.loopMovie);
	fTPreferences.SetInt8("loopSound", settings.loopSound);
	fTPreferences.SetInt8("fullVolume", settings.fullVolume);
	fTPreferences.SetInt8("halfVolume", settings.halfVolume);
	fTPreferences.SetInt8("mute", settings.mute);
	
	settingsFile.SetSize(0);
	settingsFile.Seek(0, SEEK_SET);

	fTPreferences.Flatten(&settingsFile);
}


void 
Settings::_SetDefault(mpSettings &settings)
{
	memset(&settings, 0, sizeof(settings));
	
	settings.autostart = 0;
	settings.closeWhenDonePlayingMovie = 0;
	settings.closeWhenDonePlayingSound = 0;
	settings.loopMovie = 0;
	settings.loopSound = 0;
	settings.fullVolume = 0;
	settings.halfVolume = 0;
	settings.mute = 1;
}


