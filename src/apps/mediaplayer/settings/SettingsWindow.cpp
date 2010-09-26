/*
 * Copyright 2008-2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Fredrik Modéen <fredrik@modeen.se>
 *		Stephan Aßmus <superstippi@gmx.de>
 */
 
#include "SettingsWindow.h"

#include <stdio.h>

#include <Box.h>
#include <Button.h>
#include <CheckBox.h>
#include <GridLayoutBuilder.h>
#include <GroupLayoutBuilder.h>
#include <SpaceLayoutItem.h>
#include <String.h>
#include <StringView.h>
#include <RadioButton.h>
#include <View.h>

enum {
	M_AUTOSTART = 0x3000,
	M_CLOSE_WINDOW_MOVIE,
	M_CLOSE_WINDOW_SOUNDS,
	M_LOOP_MOVIE,
	M_LOOP_SOUND,
	M_USE_OVERLAYS,
	M_SCALE_BILINEAR,
	M_SCALE_CONTROLS,
	M_START_FULL_VOLUME,
	M_START_HALF_VOLUME,
	M_START_MUTE_VOLUME,

	M_SETTINGS_SAVE,
	M_SETTINGS_CANCEL,
	M_SETTINGS_REVERT
};

#define SPACE 10
#define SPACEING 7 
#define BUTTONHEIGHT 20

SettingsWindow::SettingsWindow(BRect frame)
 	: BWindow(frame, "MediaPlayer settings", B_FLOATING_WINDOW_LOOK,
 		B_FLOATING_APP_WINDOW_FEEL,
 		B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE | B_NOT_RESIZABLE
 			| B_AUTO_UPDATE_SIZE_LIMITS)
{
	BBox* settingsBox = new BBox(B_PLAIN_BORDER, NULL);
	BGroupLayout* settingsLayout = new BGroupLayout(B_VERTICAL, 5);
	settingsBox->SetLayout(settingsLayout);
	BBox* buttonBox = new BBox(B_PLAIN_BORDER, NULL);
	BGroupLayout* buttonLayout = new BGroupLayout(B_HORIZONTAL, 5);
	buttonBox->SetLayout(buttonLayout);

	BStringView* playModeLabel = new BStringView("stringViewPlayMode",
		"Play mode");
	BStringView* viewOptionsLabel = new BStringView("stringViewViewOpions", 
		"View options");
	BStringView* bgMoviesModeLabel = new BStringView("stringViewPlayBackg", 
		"Play background clips at");
	BAlignment alignment(B_ALIGN_LEFT, B_ALIGN_VERTICAL_CENTER);
	playModeLabel->SetExplicitAlignment(alignment);
	playModeLabel->SetFont(be_bold_font);
	viewOptionsLabel->SetExplicitAlignment(alignment);
	viewOptionsLabel->SetFont(be_bold_font);
	bgMoviesModeLabel->SetExplicitAlignment(alignment);
	bgMoviesModeLabel->SetFont(be_bold_font);

	fAutostartCB = new BCheckBox("chkboxAutostart", 
		"Automatically start playing", new BMessage(M_AUTOSTART));

	fCloseWindowMoviesCB = new BCheckBox("chkBoxCloseWindowMovies", 
		"Close window when done playing movies",
		new BMessage(M_CLOSE_WINDOW_MOVIE));
	fCloseWindowSoundsCB = new BCheckBox("chkBoxCloseWindowSounds", 
		"Close window when done playing sounds",
		new BMessage(M_CLOSE_WINDOW_SOUNDS));

	fLoopMoviesCB = new BCheckBox("chkBoxLoopMovie",
		"Loop movies by default", new BMessage(M_LOOP_MOVIE));
	fLoopSoundsCB = new BCheckBox("chkBoxLoopSounds",
		"Loop sounds by default", new BMessage(M_LOOP_SOUND));

	fUseOverlaysCB = new BCheckBox("chkBoxUseOverlays",
		"Use hardware video overlays if available",
		new BMessage(M_USE_OVERLAYS));
	fScaleBilinearCB = new BCheckBox("chkBoxScaleBilinear",
		"Scale movies smoothly (non-overlay mode)",
		new BMessage(M_SCALE_BILINEAR));

	fScaleFullscreenControlsCB = new BCheckBox("chkBoxScaleControls",
		"Scale controls in full-screen mode",
		new BMessage(M_SCALE_CONTROLS));

	fFullVolumeBGMoviesRB = new BRadioButton("rdbtnfullvolume",
		"Full volume", new BMessage(M_START_FULL_VOLUME));
	
	fHalfVolumeBGMoviesRB = new BRadioButton("rdbtnhalfvolume", 
		"Low volume", new BMessage(M_START_HALF_VOLUME));
	
	fMutedVolumeBGMoviesRB = new BRadioButton("rdbtnfullvolume",
		"Muted", new BMessage(M_START_MUTE_VOLUME));

	fRevertB = new BButton("revert", "Revert", 
		new BMessage(M_SETTINGS_REVERT));

	BButton* cancelButton = new BButton("cancel", "Cancel", 
		new BMessage(M_SETTINGS_CANCEL));

	BButton* okButton = new BButton("ok", "OK",
		new BMessage(M_SETTINGS_SAVE));
	okButton->MakeDefault(true);


	// Build the layout
	SetLayout(new BGroupLayout(B_HORIZONTAL));

	AddChild(BGroupLayoutBuilder(B_VERTICAL, 0)
		.Add(BGroupLayoutBuilder(settingsLayout)
			.Add(playModeLabel)
			.Add(BGroupLayoutBuilder(B_HORIZONTAL, 0)
				.Add(BSpaceLayoutItem::CreateHorizontalStrut(10))
				.Add(BGroupLayoutBuilder(B_VERTICAL, 0)
					.Add(fAutostartCB)
					.Add(BGridLayoutBuilder(5, 0)
						.Add(BSpaceLayoutItem::CreateHorizontalStrut(10), 0, 0)
						.Add(fCloseWindowMoviesCB, 1, 0)
						.Add(BSpaceLayoutItem::CreateHorizontalStrut(10), 0, 1)
						.Add(fCloseWindowSoundsCB, 1, 1)
					)
					.Add(fLoopMoviesCB)
					.Add(fLoopSoundsCB)
				)
			)
			.Add(BSpaceLayoutItem::CreateVerticalStrut(5))

			.Add(viewOptionsLabel)
			.Add(BGroupLayoutBuilder(B_HORIZONTAL, 0)
				.Add(BSpaceLayoutItem::CreateHorizontalStrut(10))
				.Add(BGroupLayoutBuilder(B_VERTICAL, 0)
					.Add(fUseOverlaysCB)
					.Add(fScaleBilinearCB)
					.Add(fScaleFullscreenControlsCB)
				)
			)
			.Add(BSpaceLayoutItem::CreateVerticalStrut(5))

			.Add(bgMoviesModeLabel)
			.Add(BGroupLayoutBuilder(B_HORIZONTAL, 0)
				.Add(BSpaceLayoutItem::CreateHorizontalStrut(10))
				.Add(BGroupLayoutBuilder(B_VERTICAL, 0)
					.Add(fFullVolumeBGMoviesRB)
					.Add(fHalfVolumeBGMoviesRB)
					.Add(fMutedVolumeBGMoviesRB)
				)
			)
			.Add(BSpaceLayoutItem::CreateVerticalStrut(5))

			.SetInsets(5, 5, 15, 5)
		)
		.Add(BGroupLayoutBuilder(buttonLayout)
			.Add(fRevertB)
			.AddGlue()
			.Add(cancelButton)
			.Add(okButton)
			.SetInsets(5, 5, 5, 5)
		)
	);
}


SettingsWindow::~SettingsWindow()
{
}


void
SettingsWindow::Show()
{
	// The Settings that we want to be able to revert to is the state at which
	// the SettingsWindow was shown. So the current settings are stored in
	// fLastSettings.
	Settings::Default()->LoadSettings(fLastSettings);
	fSettings = fLastSettings;
	AdoptSettings();

	BWindow::Show();
}


bool
SettingsWindow::QuitRequested()
{
	Hide();
	return false;
}


void
SettingsWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case M_AUTOSTART:
		case M_CLOSE_WINDOW_MOVIE:
		case M_CLOSE_WINDOW_SOUNDS:
		case M_LOOP_MOVIE:
		case M_LOOP_SOUND:
		case M_USE_OVERLAYS:
		case M_SCALE_BILINEAR:
		case M_SCALE_CONTROLS:
		case M_START_FULL_VOLUME:
		case M_START_HALF_VOLUME:
		case M_START_MUTE_VOLUME:
			ApplySettings();
			break;

		case B_KEY_DOWN:
			int32 index;
			if (message->FindInt32("key", &index) == B_OK && index == 1)
				PostMessage(B_QUIT_REQUESTED);
			break;

		case M_SETTINGS_REVERT:
			Revert();
			break;

		case M_SETTINGS_CANCEL:
			Revert();
			// fall through
		case M_SETTINGS_SAVE:
			PostMessage(B_QUIT_REQUESTED);
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
SettingsWindow::AdoptSettings()
{
	fAutostartCB->SetValue(fSettings.autostart);
	fCloseWindowMoviesCB->SetValue(fSettings.closeWhenDonePlayingMovie);
	fCloseWindowSoundsCB->SetValue(fSettings.closeWhenDonePlayingSound);
	fLoopMoviesCB->SetValue(fSettings.loopMovie);
	fLoopSoundsCB->SetValue(fSettings.loopSound);

	fUseOverlaysCB->SetValue(fSettings.useOverlays);
	fScaleBilinearCB->SetValue(fSettings.scaleBilinear);
	fScaleFullscreenControlsCB->SetValue(fSettings.scaleFullscreenControls);

	fFullVolumeBGMoviesRB->SetValue(fSettings.backgroundMovieVolumeMode
		== mpSettings::BG_MOVIES_FULL_VOLUME);
	fHalfVolumeBGMoviesRB->SetValue(fSettings.backgroundMovieVolumeMode
		== mpSettings::BG_MOVIES_HALF_VLUME);
	fMutedVolumeBGMoviesRB->SetValue(fSettings.backgroundMovieVolumeMode
		== mpSettings::BG_MOVIES_MUTED);

	fRevertB->SetEnabled(IsRevertable());
}


void
SettingsWindow::ApplySettings()
{
	fSettings.autostart = fAutostartCB->Value() == B_CONTROL_ON;
	fSettings.closeWhenDonePlayingMovie
		= fCloseWindowMoviesCB->Value() == B_CONTROL_ON;
	fSettings.closeWhenDonePlayingSound
		= fCloseWindowSoundsCB->Value() == B_CONTROL_ON;
	fSettings.loopMovie = fLoopMoviesCB->Value() == B_CONTROL_ON;
	fSettings.loopSound = fLoopSoundsCB->Value() == B_CONTROL_ON;

	fSettings.useOverlays = fUseOverlaysCB->Value() == B_CONTROL_ON;
	fSettings.scaleBilinear = fScaleBilinearCB->Value() == B_CONTROL_ON;
	fSettings.scaleFullscreenControls
		= fScaleFullscreenControlsCB->Value() == B_CONTROL_ON;

	if (fFullVolumeBGMoviesRB->Value() == B_CONTROL_ON) {
		fSettings.backgroundMovieVolumeMode
			= mpSettings::BG_MOVIES_FULL_VOLUME;
	} else if (fHalfVolumeBGMoviesRB->Value() == B_CONTROL_ON) {
		fSettings.backgroundMovieVolumeMode
			= mpSettings::BG_MOVIES_HALF_VLUME;
	} else if (fMutedVolumeBGMoviesRB->Value() == B_CONTROL_ON) {
		fSettings.backgroundMovieVolumeMode
			= mpSettings::BG_MOVIES_MUTED;
	}

	Settings::Default()->SaveSettings(fSettings);

	fRevertB->SetEnabled(IsRevertable());
}


void
SettingsWindow::Revert()
{
	fSettings = fLastSettings;
	AdoptSettings();
	Settings::Default()->SaveSettings(fSettings);
}


bool
SettingsWindow::IsRevertable() const
{
	return fSettings != fLastSettings;
}

