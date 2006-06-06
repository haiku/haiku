/*
 * Copyright 2003-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Phipps
 *		Jérôme Duval, jerome.duval@free.fr
 */
#ifndef PASSWORDWINDOW_H
#define PASSWORDWINDOW_H


#include "ScreenSaverPrefs.h"

#include <Window.h>

class BRadioButton;
class BTextControl;

class PasswordWindow : public BWindow {
	public:
		PasswordWindow(ScreenSaverPrefs &prefs);

		virtual void MessageReceived(BMessage *message);

		void Update();

	private:
		void _Setup();

		BRadioButton *fUseNetwork, *fUseCustom;
		BTextControl *fPasswordControl, *fConfirmControl;
		ScreenSaverPrefs &fPrefs;
};

#endif	// PASSWORDWINDOW_H
