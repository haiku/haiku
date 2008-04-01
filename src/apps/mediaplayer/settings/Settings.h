/*
 * Copyright 2008, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Fredrik Modéen <fredrik@modeen.se>
 */
 
#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include <Message.h>
#include <Path.h>

#include "TPreferences.h"

struct mpSettings {
	int8 
	autostart, closeWhenDonePlayingMovie, closeWhenDonePlayingSound, 
	loopMovie, loopSound, fullVolume, halfVolume, mute;
};

#define SETTINGSFILENAME "MediaPlayerSettings"

class Settings {
	public:
		Settings(const char *filename = SETTINGSFILENAME);
		
		void LoadSettings(mpSettings &settings);
		void SaveSettings(const mpSettings &settings);
	
	private:
		void _SetDefault(mpSettings &settings);
				
		TPreferences 	fTPreferences;
};

#endif  // __SETTINGS_H__
