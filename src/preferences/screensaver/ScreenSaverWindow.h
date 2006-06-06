/*
 * Copyright 2003-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Phipps
 *		Jérôme Duval, jerome.duval@free.fr
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef SCREEN_SAVER_WINDOW_H
#define SCREEN_SAVER_WINDOW_H


#include "PasswordWindow.h"
#include "ScreenSaverPrefs.h"

#include <Box.h>
#include <CheckBox.h>
#include <FilePanel.h>
#include <Slider.h>
#include <ListView.h>
#include <StringView.h>

class BButton;

class ModulesView;
class MouseAreaView;
class ScreenSaverRunner;
class TimeSlider;

class ScreenSaverWindow : public BWindow {
	public:
		ScreenSaverWindow();
		virtual ~ScreenSaverWindow();

		virtual void MessageReceived(BMessage *message);
		virtual void ScreenChanged(BRect frame, color_space colorSpace);
		virtual bool QuitRequested();

		void LoadSettings();

	private:
		void _SetupFadeTab(BRect frame);
		void _UpdateTurnOffScreen();
		void _UpdateStatus();

		ScreenSaverPrefs fPrefs;
		uint32			fTurnOffScreenFlags;

		BView*			fFadeView;
		ModulesView*	fModulesView;
		BTabView*		fTabView;

		BCheckBox*		fEnableCheckBox;
		TimeSlider*		fRunSlider;

		BCheckBox*		fTurnOffCheckBox;
		TimeSlider*		fTurnOffSlider;

		BCheckBox*		fPasswordCheckBox;
		TimeSlider*		fPasswordSlider;
		BButton*		fPasswordButton;
		PasswordWindow*	fPasswordWindow;

		BStringView *fFadeNowString;
		BStringView *fFadeNowString2;
		BStringView *fDontFadeString;
		BStringView *fDontFadeString2;

		MouseAreaView*	fFadeNow;
		MouseAreaView*	fFadeNever;
};

static const int32 kMsgUpdateList = 'UPDL';

#endif	// SCREEN_SAVER_WINDOW_H
