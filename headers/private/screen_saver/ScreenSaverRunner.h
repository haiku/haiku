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
#ifndef SCREEN_SAVER_RUNNER_H
#define SCREEN_SAVER_RUNNER_H


#include <ScreenSaver.h>
#include <View.h>
#include <Window.h>

#include "ScreenSaverSettings.h"


class ScreenSaverRunner {
public:
								ScreenSaverRunner(BWindow* window,
									BView* view,
									ScreenSaverSettings& settings);
								~ScreenSaverRunner();

			BScreenSaver*		ScreenSaver() const { return fSaver; };

			status_t			Run();
			void				Quit();

			status_t			Suspend();
			status_t			Resume();

private:
			void				_LoadAddOn();
			void				_CleanUp();
	static	status_t			_ThreadFunc(void* data);
			status_t			_Run();

			BWindow*			fWindow;
			BView*				fView;
			bool				fIsDirectDraw;
			ScreenSaverSettings& fSettings;

			BScreenSaver*		fSaver;

			image_id			fAddonImage;
			thread_id			fThread;
			volatile bool		fQuitting;
};

#endif // SCREEN_SAVER_RUNNER_H
