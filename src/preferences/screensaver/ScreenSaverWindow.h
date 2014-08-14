/*
 * Copyright 2003-2013 Haiku, Inc. All rights reserved.
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


#include <DirectWindow.h>

#include "PasswordWindow.h"
#include "ScreenSaverSettings.h"


class BMessage;
class BRect;

class FadeView;
class ModulesView;
class TabView;


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
			TabView*			fTabView;
};


static const int32 kMsgUpdateList = 'UPDL';


#endif	// SCREEN_SAVER_WINDOW_H
