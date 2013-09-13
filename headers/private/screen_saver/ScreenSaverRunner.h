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
#ifndef SCREEN_SAVER_RUNNER_H
#define SCREEN_SAVER_RUNNER_H


#include <SupportDefs.h>
#include <DirectWindow.h>


class BScreenSaver;
class BView;
class ScreenSaverSettings;


class ScreenSaverRunner {
public:
								ScreenSaverRunner(BWindow* window, BView* view,
									bool preview,
									ScreenSaverSettings& settings);
								~ScreenSaverRunner();

			BScreenSaver*		ScreenSaver() const;
			bool				HasStarted() const;

			status_t			Run();
			void				Quit();

			void				Suspend();
			void				Resume();

private:
			void				_LoadAddOn();
			void				_CleanUp();
	static	status_t			_ThreadFunc(void* data);
			status_t			_Run();

			BScreenSaver*		fSaver;
			BWindow*			fWindow;
			BDirectWindow*		fDirectWindow;
			BView*				fView;
			ScreenSaverSettings& fSettings;
			bool				fPreview;
			bool				fHasStarted;

			image_id			fAddonImage;
			thread_id			fThread;
			volatile bool		fQuitting;
};

#endif // SCREEN_SAVER_RUNNER_H
