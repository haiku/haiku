/*
 * Copyright 2008, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Fredrik Modéen <fredrik@modeen.se>
 */

#include "Settings.h"

#include <Autolock.h>


Settings::Settings(const char* filename)
	: BLocker("settings lock"),
	  fSettingsMessage(B_USER_CONFIG_DIRECTORY, filename)
{
}


void
Settings::LoadSettings(mpSettings& settings) const
{
	BAutolock _(const_cast<Settings*>(this));

	settings.autostart = fSettingsMessage.GetValue("autostart", true);
	settings.closeWhenDonePlayingMovie
		= fSettingsMessage.GetValue("closeWhenDonePlayingMovie", false);
	settings.closeWhenDonePlayingSound
		= fSettingsMessage.GetValue("closeWhenDonePlayingSound", false);
	settings.loopMovie = fSettingsMessage.GetValue("loopMovie", false);
	settings.loopSound = fSettingsMessage.GetValue("loopSound", false);

	settings.useOverlays = fSettingsMessage.GetValue("useOverlays", true);
	settings.scaleBilinear = fSettingsMessage.GetValue("scaleBilinear", false);

	settings.backgroundMovieVolumeMode
		= fSettingsMessage.GetValue("bgMovieVolumeMode",
			(uint32)mpSettings::BG_MOVIES_MUTED);
}


void
Settings::SaveSettings(const mpSettings& settings)
{
	BAutolock _(this);

	fSettingsMessage.SetValue("autostart", settings.autostart);
	fSettingsMessage.SetValue("closeWhenDonePlayingMovie", 
		settings.closeWhenDonePlayingMovie);
	fSettingsMessage.SetValue("closeWhenDonePlayingSound", 
		settings.closeWhenDonePlayingSound);
	fSettingsMessage.SetValue("loopMovie", settings.loopMovie);
	fSettingsMessage.SetValue("loopSound", settings.loopSound);

	fSettingsMessage.SetValue("useOverlays", settings.useOverlays);
	fSettingsMessage.SetValue("scaleBilinear", settings.scaleBilinear);

	fSettingsMessage.SetValue("bgMovieVolumeMode",
		settings.backgroundMovieVolumeMode);

	// Save at this point, although saving is also done on destruction,
	// this will make sure the settings are saved even when the player
	// crashes.
	fSettingsMessage.Save();
}


// #pragma mark - static

/*static*/ Settings
Settings::sGlobalInstance;


/*static*/ mpSettings
Settings::CurrentSettings()
{
	mpSettings settings;
	sGlobalInstance.LoadSettings(settings);
	return settings;	
}


/*static*/ Settings*
Settings::Default()
{
	return &sGlobalInstance;
}



