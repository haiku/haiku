/*
 * Copyright 2003-2010, Haiku.
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
#include "ScreenSaverSettings.h"

#include <Box.h>
#include <CheckBox.h>
#include <FilePanel.h>
#include <Slider.h>
#include <ListView.h>


class BButton;
class BTabView;
class BTextView;

class ModulesView;
class ScreenCornerSelector;
class ScreenSaverRunner;
class TimeSlider;


class ScreenSaverWindow : public BWindow {
public:
								ScreenSaverWindow();
	virtual						~ScreenSaverWindow();

	virtual	void				MessageReceived(BMessage* message);
	virtual	void				ScreenChanged(BRect frame, color_space space);
	virtual	bool				QuitRequested();

			void				LoadSettings();
			void				SetMinimalSizeLimit(float width, float height);

private:
			void				_SetupFadeTab(BRect frame);
			void				_UpdateTurnOffScreen();
			void				_UpdateStatus();

private:
			float				fMinWidth;
			float				fMinHeight;
			ScreenSaverSettings	fSettings;
			uint32				fTurnOffScreenFlags;

			BView*				fFadeView;
			ModulesView*		fModulesView;
			BTabView*			fTabView;

			BCheckBox*			fEnableCheckBox;
			TimeSlider*			fRunSlider;

			BCheckBox*			fTurnOffCheckBox;
			TimeSlider*			fTurnOffSlider;
			BTextView*			fTurnOffNotSupported;

			BCheckBox*			fPasswordCheckBox;
			TimeSlider*			fPasswordSlider;
			BButton*			fPasswordButton;
			PasswordWindow*		fPasswordWindow;

			ScreenCornerSelector* fFadeNow;
			ScreenCornerSelector* fFadeNever;
};


static const int32 kMsgUpdateList = 'UPDL';


#endif	// SCREEN_SAVER_WINDOW_H
