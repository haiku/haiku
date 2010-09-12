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
		virtual					~SettingsWindow();

		virtual	void			Show();
		virtual	bool			QuitRequested();
		virtual	void			MessageReceived(BMessage* message);

		void					AdoptSettings();
		void					ApplySettings();
		void					Revert();
		bool					IsRevertable() const;

private:
		mpSettings 				fSettings;
		mpSettings 				fLastSettings;
		
		BCheckBox* 				fAutostartCB; 
		BCheckBox*				fCloseWindowMoviesCB; 
		BCheckBox*				fCloseWindowSoundsCB; 
		BCheckBox*				fLoopMoviesCB;
		BCheckBox*				fLoopSoundsCB;

		BCheckBox*				fUseOverlaysCB;
		BCheckBox*				fScaleBilinearCB;
		BCheckBox*				fScaleFullscreenControlsCB;

		BRadioButton*			fFullVolumeBGMoviesRB; 
		BRadioButton*			fHalfVolumeBGMoviesRB; 
		BRadioButton*			fMutedVolumeBGMoviesRB;

		BButton*				fRevertB;
};

#endif
