/*
 * Copyright 2008, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Fredrik Modéen <fredrik@modeen.se>
 */
 
#include "SettingsWindow.h"

#include <stdio.h>

#include <Box.h>
#include <CheckBox.h>
#include <StringView.h>
#include <RadioButton.h>
#include <View.h>
#include <Button.h>
#include <String.h>

#ifdef __HAIKU__
#	include <GridLayoutBuilder.h>
#	include <GroupLayout.h>
#	include <GroupLayoutBuilder.h>
#	include <SpaceLayoutItem.h>
#endif

enum {
	M_AUTOSTART = 0x3000,
	M_CLOSE_WINDOW_MOVIE,
	M_CLOSE_WINDOW_SOUNDS,
	M_LOOP_MOVIE,
	M_LOOP_SOUND,
	M_USE_OVERLAYS,
	M_SCALE_BILINEAR,
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
 	: BWindow(frame, "MediaPlayer Settings", B_FLOATING_WINDOW_LOOK,
 		B_FLOATING_APP_WINDOW_FEEL,
 		B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE | B_NOT_RESIZABLE
#ifdef __HAIKU__
 			| B_AUTO_UPDATE_SIZE_LIMITS)
#else
 			)
#endif
{
#ifdef __HAIKU__

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

	BButton* okButton = new BButton("ok", "Ok",
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

#else

	frame = Bounds();
	BView* view = new BView(frame,"SettingsView",B_FOLLOW_ALL_SIDES,B_WILL_DRAW);
	view->SetViewColor(216, 216, 216);
	
	BRect btnRect(80.00, frame.bottom - (SPACE + BUTTONHEIGHT), 145.00, 
		frame.bottom-SPACE);

	fRevertB = new BButton(btnRect, "revert", "Revert", 
		new BMessage(M_SETTINGS_REVERT));
	view->AddChild(fRevertB);

	btnRect.OffsetBy(btnRect.Width() + SPACE, 0);
	BButton* btn = new BButton(btnRect, "btnCancel", "Cancel", 
		new BMessage(M_SETTINGS_CANCEL));
	view->AddChild(btn);
	
	btnRect.OffsetBy(btnRect.Width() + SPACE, 0);
	btn = new BButton(btnRect, "btnOK", "OK", new BMessage(M_SETTINGS_SAVE));
	view->AddChild(btn);
	
	BRect rectBox(frame.left + SPACE, frame.top + SPACE, frame.right - SPACE, 
		btnRect.top- SPACE);
	BBox* bbox = new BBox(rectBox, "box1", B_FOLLOW_ALL_SIDES,B_WILL_DRAW | B_NAVIGABLE,
		B_FANCY_BORDER);
	bbox->SetLabel("MediaPlayer Settings");
	
	BFont font;
	font_height fh1;
	font.GetHeight(&fh1);

	BString str("Play Mode:");
	BRect rect(rectBox.left, rectBox.top + SPACE, rectBox.right - (12*2), 
		rectBox.top + fh1.leading + fh1.ascent + 10);
	bbox->AddChild(new BStringView(rect, "stringViewPlayMode", str.String()));
	
	rect.OffsetBy(0, rect.Height());
	bbox->AddChild(fAutostartCB = new BCheckBox(rect, "chkboxAutostart", 
		"Automatically start playing", new BMessage(M_AUTOSTART)));

	rect.OffsetBy(SPACE, rect.Height() + SPACEING);
	bbox->AddChild(fCloseWindowMoviesCB = new BCheckBox(rect, "chkBoxCloseWindowMovies", 
		"Close window when done playing movies", new BMessage(M_CLOSE_WINDOW_MOVIE)));
	
	rect.OffsetBy(0, rect.Height() + SPACEING);
	bbox->AddChild(fCloseWindowSoundsCB = new BCheckBox(rect, "chkBoxCloseWindowSounds", 
		"Close window when done playing sounds", new BMessage(M_CLOSE_WINDOW_SOUNDS)));
	
	rect.OffsetBy(-SPACE, rect.Height() + SPACEING);
	bbox->AddChild(fLoopMoviesCB = new BCheckBox(rect, "chkBoxLoopMovie", "Loop movies by default",
		new BMessage(M_LOOP_MOVIE)));
	
	rect.OffsetBy(0, rect.Height() + SPACEING);
	bbox->AddChild(fLoopSoundsCB = new BCheckBox(rect, "chkBoxLoopSounds", "Loop sounds by default",
		new BMessage(M_LOOP_SOUND)));

	rect.OffsetBy(0, rect.Height() + SPACEING);
	bbox->AddChild(fUseOverlaysCB = new BCheckBox(rect, "chkBoxUseOverlays", "Use hardware video overlays if available",
		new BMessage(M_USE_OVERLAYS)));

	rect.OffsetBy(0, rect.Height() + SPACEING);
	bbox->AddChild(fScaleBilinearCB = new BCheckBox(rect, "chkBoxScaleBilinear", "Scale movies smoothly (non-overlay mode)",
		new BMessage(M_SCALE_BILINEAR)));

	rect.OffsetBy(0, rect.Height() + SPACE + SPACEING);
	bbox->AddChild(new BStringView(rect, "stringViewPlayBackg", 
		"Play backgrounds clips at:"));
	
	rect.OffsetBy(SPACE, rect.Height() + SPACEING);
	fFullVolumeBGMoviesRB = new BRadioButton(rect, "rdbtnfullvolume", 
		"Full Volume", new BMessage(M_START_FULL_VOLUME));
	bbox->AddChild(fFullVolumeBGMoviesRB);
	
	rect.OffsetBy(0, rect.Height() + SPACEING);
	fHalfVolumeBGMoviesRB = new BRadioButton(rect, "rdbtnhalfvolume", 
		"Low Volume", new BMessage(M_START_HALF_VOLUME));
	bbox->AddChild(fHalfVolumeBGMoviesRB);
	
	rect.OffsetBy(0, rect.Height() + SPACEING);
	fMutedVolumeBGMoviesRB = new BRadioButton(rect, "rdbtnfullvolume", "Muted",
		new BMessage(M_START_MUTE_VOLUME));
	bbox->AddChild(fMutedVolumeBGMoviesRB);

	view->AddChild(bbox);
	AddChild(view);
#endif

	// disable currently unsupported features
	fLoopMoviesCB->SetEnabled(false);
	fLoopSoundsCB->SetEnabled(false);
}


SettingsWindow::~SettingsWindow()
{
}


void
SettingsWindow::Show()
{
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

