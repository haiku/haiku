/*
 * Copyright 2008, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Fredrik Modéen <fredrik@modeen.se>
 */

#include "Settings.h"

#include <Autolock.h>


bool
mpSettings::operator!=(const mpSettings& other) const
{
	return autostart != other.autostart
		|| closeWhenDonePlayingMovie != other.closeWhenDonePlayingMovie
		|| closeWhenDonePlayingSound != other.closeWhenDonePlayingSound
		|| loopMovie != other.loopMovie
		|| loopSound != other.loopSound
		|| useOverlays != other.useOverlays
		|| scaleBilinear != other.scaleBilinear
		|| scaleFullscreenControls != other.scaleFullscreenControls
		|| subtitleSize != other.subtitleSize
		|| subtitlePlacement != other.subtitlePlacement
		|| backgroundMovieVolumeMode != other.backgroundMovieVolumeMode
		|| filePanelFolder != other.filePanelFolder
		|| audioPlayerWindowFrame != other.audioPlayerWindowFrame;
}


Settings::Settings(const char* filename)
	:
	BLocker("settings lock"),
	fSettingsMessage(B_USER_SETTINGS_DIRECTORY, filename)
{
	// The settings are loaded from disk in the SettingsMessage constructor.
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
	settings.scaleBilinear = fSettingsMessage.GetValue("scaleBilinear", true);
	settings.scaleFullscreenControls
		= fSettingsMessage.GetValue("scaleFullscreenControls", true);

	settings.subtitleSize
		= fSettingsMessage.GetValue("subtitleSize",
			(uint32)mpSettings::SUBTITLE_SIZE_MEDIUM);
	settings.subtitlePlacement
		= fSettingsMessage.GetValue("subtitlePlacement",
			(uint32)mpSettings::SUBTITLE_PLACEMENT_BOTTOM_OF_VIDEO);

	settings.backgroundMovieVolumeMode
		= fSettingsMessage.GetValue("bgMovieVolumeMode",
			(uint32)mpSettings::BG_MOVIES_FULL_VOLUME);

	entry_ref defaultFilePanelFolder;
		// an "unset" entry_ref
	settings.filePanelFolder = fSettingsMessage.GetValue(
		"filePanelDirectory", defaultFilePanelFolder);

	settings.audioPlayerWindowFrame = fSettingsMessage.GetValue(
		"audioPlayerWindowFrame", BRect());
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
	fSettingsMessage.SetValue("scaleFullscreenControls",
		settings.scaleFullscreenControls);

	fSettingsMessage.SetValue("subtitleSize", settings.subtitleSize);
	fSettingsMessage.SetValue("subtitlePlacement", settings.subtitlePlacement);

	fSettingsMessage.SetValue("bgMovieVolumeMode",
		settings.backgroundMovieVolumeMode);

	fSettingsMessage.SetValue("filePanelDirectory",
		settings.filePanelFolder);

	fSettingsMessage.SetValue("audioPlayerWindowFrame",
		settings.audioPlayerWindowFrame);

	// Save at this point, although saving is also done on destruction,
	// this will make sure the settings are saved even when the player
	// crashes.
	fSettingsMessage.Save();

	Notify();
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



