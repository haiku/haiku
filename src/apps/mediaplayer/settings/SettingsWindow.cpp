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

enum {
	M_AUTOSTART = 0x3000,
	M_CLOSE_WINDOW_MOVIE,
	M_CLOSE_WINDOW_SOUNDS,
	M_LOOP_MOVIE,
	M_LOOP_SOUND,
	M_START_FULL_VOLUME,
	M_START_HALF_VOLUME,
	M_START_MUTE_VOLUME,
	M_SETTINGS_SAVE,
	M_SETTINGS_CANCEL,
};

#define SPACE 10
#define SPACEING 7 
#define BUTTONHEIGHT 20

SettingsWindow::SettingsWindow(BRect frame)
 	: BWindow(frame, "Settings", B_MODAL_WINDOW, B_NOT_CLOSABLE | B_NOT_ZOOMABLE
 	 | B_NOT_RESIZABLE)
{
	fSettingsObj = new Settings();
	fSettingsObj->LoadSettings(fSettings);
	
	frame = Bounds();
	BView* view = new BView(frame,"SettingsView",B_FOLLOW_ALL_SIDES,B_WILL_DRAW);
	view->SetViewColor(216, 216, 216);
	
	BRect btnRect(140.00, frame.bottom - (SPACE + BUTTONHEIGHT), 205.00, 
		frame.bottom-SPACE);
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
	bbox->AddChild(fChkboxAutostart = new BCheckBox(rect, "chkboxAutostart", 
		"Automatically start playing", new BMessage(M_AUTOSTART)));

	rect.OffsetBy(SPACE, rect.Height() + SPACEING);
	bbox->AddChild(fChkBoxCloseWindowMovies = new BCheckBox(rect, "chkBoxCloseWindowMovies", 
		"Close window when done playing movies", new BMessage(M_CLOSE_WINDOW_MOVIE)));
	
	rect.OffsetBy(0, rect.Height() + SPACEING);
	bbox->AddChild(fChkBoxCloseWindowSounds = new BCheckBox(rect, "chkBoxCloseWindowSounds", 
		"Close window when done playing sounds", new BMessage(M_CLOSE_WINDOW_SOUNDS)));
	
	rect.OffsetBy(-SPACE, rect.Height() + SPACEING);
	bbox->AddChild(fChkBoxLoopMovie = new BCheckBox(rect, "chkBoxLoopMovie", "Loop movies by default",
		new BMessage(M_LOOP_MOVIE)));
	
	rect.OffsetBy(0, rect.Height() + SPACEING);
	bbox->AddChild(fChkBoxLoopSounds = new BCheckBox(rect, "chkBoxLoopSounds", "Loop sounds by default",
		new BMessage(M_LOOP_SOUND)));

	rect.OffsetBy(0, rect.Height() + SPACE + SPACEING);
	bbox->AddChild(new BStringView(rect, "stringViewPlayBackg", 
		"Play backgrounds movies at:"));
	
	rect.OffsetBy(SPACE, rect.Height() + SPACEING);
	fRdbtnfullvolume = new BRadioButton(rect, "rdbtnfullvolume", 
		"Full Volume", new BMessage(M_START_FULL_VOLUME));
	bbox->AddChild(fRdbtnfullvolume);
	
	rect.OffsetBy(0, rect.Height() + SPACEING);
	fRdbtnhalfvolume = new BRadioButton(rect, "rdbtnhalfvolume", 
		"Half Volume", new BMessage(M_START_HALF_VOLUME));
	bbox->AddChild(fRdbtnhalfvolume);
	
	rect.OffsetBy(0, rect.Height() + SPACEING);
	fRdbtnmutevolume = new BRadioButton(rect, "rdbtnfullvolume", "Muted",
		new BMessage(M_START_MUTE_VOLUME));
	bbox->AddChild(fRdbtnmutevolume);

	SetSettings();

	view->AddChild(bbox);
	AddChild(view);
}


SettingsWindow::~SettingsWindow()
{
}


void
SettingsWindow::SetSettings()
{
	fChkboxAutostart->SetValue(fSettings.autostart);
	fChkBoxCloseWindowMovies->SetValue(fSettings.closeWhenDonePlayingMovie);
	fChkBoxCloseWindowSounds->SetValue(fSettings.closeWhenDonePlayingSound);
	fChkBoxLoopMovie->SetValue(fSettings.loopMovie);
	fChkBoxLoopSounds->SetValue(fSettings.loopSound);
	fRdbtnfullvolume->SetValue(fSettings.fullVolume);
	fRdbtnhalfvolume->SetValue(fSettings.halfVolume);
	fRdbtnmutevolume->SetValue(fSettings.mute);
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
		case M_START_FULL_VOLUME:
		case M_START_HALF_VOLUME:
		case M_START_MUTE_VOLUME:
			fSettings.autostart = fChkboxAutostart->Value();
			fSettings.closeWhenDonePlayingMovie = fChkBoxCloseWindowMovies->Value();
			fSettings.closeWhenDonePlayingSound = fChkBoxCloseWindowSounds->Value();
			fSettings.loopMovie = fChkBoxLoopMovie->Value();
			fSettings.loopSound = fChkBoxLoopSounds->Value();
			fSettings.fullVolume = fRdbtnfullvolume->Value();
			fSettings.halfVolume = fRdbtnhalfvolume->Value();
			fSettings.mute = fRdbtnmutevolume->Value();
		case B_KEY_DOWN:
			int32 index;
			if(message->FindInt32("key", &index) == B_NO_ERROR)
				if(index == 1)
					PostMessage(B_QUIT_REQUESTED);
		break;
		case M_SETTINGS_SAVE:
			fSettingsObj->SaveSettings(fSettings);
		case M_SETTINGS_CANCEL:
			PostMessage(B_QUIT_REQUESTED);
		break;
		default:
			BWindow::MessageReceived(message);
			break;
	}
}
