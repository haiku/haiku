/*
 * Copyright 2008, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Fredrik Modéen <fredrik@modeen.se>
 */
 
#ifndef _SETTINGS_WINDOW_H
#define _SETTINGS_WINDOW_H

#include <Window.h>
#include <CheckBox.h>
#include <RadioButton.h>

#include "Settings.h"

class SettingsWindow : public BWindow {
	public:
							SettingsWindow(BRect frame);
		virtual				~SettingsWindow();
		void				SetSettings();
		virtual	bool		QuitRequested();
		virtual	void		MessageReceived(BMessage* message);
	private:
		Settings*			fSettingsObj;
		mpSettings 			fSettings;
		
		BCheckBox* 			fChkboxAutostart; 
		BCheckBox*			fChkBoxCloseWindowMovies; 
		BCheckBox*			fChkBoxCloseWindowSounds; 
		BCheckBox*			fChkBoxLoopMovie;
		BCheckBox*			fChkBoxLoopSounds;
		BRadioButton*		fRdbtnfullvolume; 
		BRadioButton*		fRdbtnhalfvolume; 
		BRadioButton*		fRdbtnmutevolume;
};

#endif
