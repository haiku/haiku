/*
 * Copyright 2003-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *              Michael Phipps
 *              Jérôme Duval, jerome.duval@free.fr
 */
#ifndef PASSWORDWINDOW_H
#define PASSWORDWINDOW_H

#include <Button.h>
#include <Constants.h>
#include <RadioButton.h>
#include <String.h>
#include <TextControl.h>
#include <Window.h>
#include "ScreenSaverPrefs.h"

class PasswordWindow : public BWindow
{
	public:
		PasswordWindow(ScreenSaverPrefs &prefs);
		void Setup();
		void Update();
		virtual void MessageReceived(BMessage *message);

	private:
		BRadioButton *fUseNetwork,*fUseCustom;
		BTextControl *fPassword,*fConfirm;
		BButton *fCancel,*fDone;
		BString fThePassword; 
		ScreenSaverPrefs &fPrefs;

};

#endif // PASSWORDWINDOW_H

