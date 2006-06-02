/*
 * Copyright 2005-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef SETTINGS_WINDOW_H
#define SETTINGS_WINDOW_H


#include <Window.h>

#include "Settings.h"

class BStringView;
class BCheckBox;
class BSlider;
class BButton;


class SettingsWindow : public BWindow {
	public:
		SettingsWindow();
		virtual ~SettingsWindow();

		virtual bool QuitRequested();
		virtual void MessageReceived(BMessage* message);

	private:
		void _Update();
		status_t _GetSwapFileLimits(off_t& minSize, off_t& maxSize);

		BCheckBox*		fSwapEnabledCheckBox;
		BSlider*		fSizeSlider;
		BButton*		fRevertButton;
		BStringView*	fWarningStringView;

		Settings		fSettings;
};

#endif	/* SETTINGS_WINDOW_H */
