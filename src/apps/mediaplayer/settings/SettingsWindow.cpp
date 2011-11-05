/*
 * Copyright 2008-2011, Haiku, Inc. All rights reserved.
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
#include <Catalog.h>
#include <CheckBox.h>
#include <GridLayoutBuilder.h>
#include <GroupLayoutBuilder.h>
#include <Locale.h>
#include <OptionPopUp.h>
#include <SpaceLayoutItem.h>
#include <String.h>
#include <StringView.h>
#include <RadioButton.h>
#include <View.h>


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "MediaPlayer-SettingsWindow"


enum {
	M_SETTINGS_CHANGED = 0x3000,

	M_SETTINGS_SAVE,
	M_SETTINGS_CANCEL,
	M_SETTINGS_REVERT
};


#define SPACE 10
#define SPACEING 7
#define BUTTONHEIGHT 20


SettingsWindow::SettingsWindow(BRect frame)
 	:
 	BWindow(frame, B_TRANSLATE("MediaPlayer settings"), B_FLOATING_WINDOW_LOOK,
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
		B_TRANSLATE("Play mode"));
	BStringView* viewOptionsLabel = new BStringView("stringViewViewOpions",
		B_TRANSLATE("View options"));
	BStringView* bgMoviesModeLabel = new BStringView("stringViewPlayBackg",
		B_TRANSLATE("Volume of background clips"));
	BAlignment alignment(B_ALIGN_LEFT, B_ALIGN_VERTICAL_CENTER);
	playModeLabel->SetExplicitAlignment(alignment);
	playModeLabel->SetFont(be_bold_font);
	viewOptionsLabel->SetExplicitAlignment(alignment);
	viewOptionsLabel->SetFont(be_bold_font);
	bgMoviesModeLabel->SetExplicitAlignment(alignment);
	bgMoviesModeLabel->SetFont(be_bold_font);

	fAutostartCB = new BCheckBox("chkboxAutostart",
		B_TRANSLATE("Automatically start playing"),
		new BMessage(M_SETTINGS_CHANGED));

	fCloseWindowMoviesCB = new BCheckBox("chkBoxCloseWindowMovies",
		B_TRANSLATE("Close window after playing video"),
		new BMessage(M_SETTINGS_CHANGED));
	fCloseWindowSoundsCB = new BCheckBox("chkBoxCloseWindowSounds",
		B_TRANSLATE("Close window after playing audio"),
		new BMessage(M_SETTINGS_CHANGED));

	fLoopMoviesCB = new BCheckBox("chkBoxLoopMovie",
		B_TRANSLATE("Loop video"),
		new BMessage(M_SETTINGS_CHANGED));
	fLoopSoundsCB = new BCheckBox("chkBoxLoopSounds",
		B_TRANSLATE("Loop audio"),
		new BMessage(M_SETTINGS_CHANGED));

	fUseOverlaysCB = new BCheckBox("chkBoxUseOverlays",
		B_TRANSLATE("Use hardware video overlays if available"),
		new BMessage(M_SETTINGS_CHANGED));
	fScaleBilinearCB = new BCheckBox("chkBoxScaleBilinear",
		B_TRANSLATE("Scale movies smoothly (non-overlay mode)"),
		new BMessage(M_SETTINGS_CHANGED));

	fScaleFullscreenControlsCB = new BCheckBox("chkBoxScaleControls",
		B_TRANSLATE("Scale controls in full screen mode"),
		new BMessage(M_SETTINGS_CHANGED));

	fSubtitleSizeOP = new BOptionPopUp("subtitleSize",
		B_TRANSLATE("Subtitle size:"), new BMessage(M_SETTINGS_CHANGED));
	fSubtitleSizeOP->AddOption(
		B_TRANSLATE("Small"), mpSettings::SUBTITLE_SIZE_SMALL);
	fSubtitleSizeOP->AddOption(
		B_TRANSLATE("Medium"), mpSettings::SUBTITLE_SIZE_MEDIUM);
	fSubtitleSizeOP->AddOption(
		B_TRANSLATE("Large"), mpSettings::SUBTITLE_SIZE_LARGE);

	fSubtitlePlacementOP = new BOptionPopUp("subtitlePlacement",
		B_TRANSLATE("Subtitle placement:"), new BMessage(M_SETTINGS_CHANGED));
	fSubtitlePlacementOP->AddOption(B_TRANSLATE("Bottom of video"),
		mpSettings::SUBTITLE_PLACEMENT_BOTTOM_OF_VIDEO);
	fSubtitlePlacementOP->AddOption(B_TRANSLATE("Bottom of screen"),
		mpSettings::SUBTITLE_PLACEMENT_BOTTOM_OF_SCREEN);

	fFullVolumeBGMoviesRB = new BRadioButton("rdbtnfullvolume",
		B_TRANSLATE("Full volume"), new BMessage(M_SETTINGS_CHANGED));

	fHalfVolumeBGMoviesRB = new BRadioButton("rdbtnhalfvolume",
		B_TRANSLATE("Low volume"), new BMessage(M_SETTINGS_CHANGED));

	fMutedVolumeBGMoviesRB = new BRadioButton("rdbtnfullvolume",
		B_TRANSLATE("Muted"), new BMessage(M_SETTINGS_CHANGED));

	fRevertB = new BButton("revert", B_TRANSLATE("Revert"),
		new BMessage(M_SETTINGS_REVERT));

	BButton* cancelButton = new BButton("cancel", B_TRANSLATE("Cancel"),
		new BMessage(M_SETTINGS_CANCEL));

	BButton* okButton = new BButton("ok", B_TRANSLATE("OK"),
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
					.Add(fSubtitleSizeOP)
					.Add(fSubtitlePlacementOP)
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
	Settings::Default()->Get(fLastSettings);
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
		case M_SETTINGS_CHANGED:
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

	fSubtitleSizeOP->SetValue(fSettings.subtitleSize);
	fSubtitlePlacementOP->SetValue(fSettings.subtitlePlacement);

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

	fSettings.subtitleSize = fSubtitleSizeOP->Value();
	fSettings.subtitlePlacement = fSubtitlePlacementOP->Value();

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

	Settings::Default()->Update(fSettings);

	fRevertB->SetEnabled(IsRevertable());
}


void
SettingsWindow::Revert()
{
	fSettings = fLastSettings;
	AdoptSettings();
	Settings::Default()->Update(fSettings);
}


bool
SettingsWindow::IsRevertable() const
{
	return fSettings != fLastSettings;
}

