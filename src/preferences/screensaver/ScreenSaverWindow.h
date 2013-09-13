/*
 * Copyright 2003-2013 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Jérôme Duval, jerome.duval@free.fr
 *		Michael Phipps
 *		John Scipione, jscipione@gmail.com
 */
#ifndef SCREEN_SAVER_WINDOW_H
#define SCREEN_SAVER_WINDOW_H


#include "PasswordWindow.h"

#include <Box.h>
#include <CheckBox.h>
#include <FilePanel.h>
#include <Slider.h>
#include <ListView.h>

#include "ScreenSaverSettings.h"


class BButton;
class BTabView;

class FadeView;
class ModulesView;


class ScreenSaverWindow : public BWindow {
public:
								ScreenSaverWindow();
	virtual						~ScreenSaverWindow();

	virtual	void				MessageReceived(BMessage* message);
	virtual	void				ScreenChanged(BRect frame, color_space space);
	virtual	bool				QuitRequested();

			void				LoadSettings();

private:
			float				fMinWidth;
			float				fMinHeight;
			ScreenSaverSettings	fSettings;
			PasswordWindow*		fPasswordWindow;

			FadeView*			fFadeView;
			ModulesView*		fModulesView;
			BTabView*			fTabView;
};


static const int32 kMsgUpdateList = 'UPDL';


#endif	// SCREEN_SAVER_WINDOW_H
