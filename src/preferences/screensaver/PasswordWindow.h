/*
 * Copyright 2003-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Phipps
 *		Jérôme Duval, jerome.duval@free.fr
 */
#ifndef PASSWORD_WINDOW_H
#define PASSWORD_WINDOW_H


#include "ScreenSaverSettings.h"

#include <Window.h>

class BRadioButton;
class BTextControl;

class PasswordWindow : public BWindow {
	public:
		PasswordWindow(ScreenSaverSettings &settings);

		virtual void MessageReceived(BMessage *message);

		void Update();

	private:
		void _Setup();

		BRadioButton *fUseNetwork, *fUseCustom;
		BTextControl *fPasswordControl, *fConfirmControl;
		ScreenSaverSettings &fSettings;
};

#endif	// PASSWORD_WINDOW_H
