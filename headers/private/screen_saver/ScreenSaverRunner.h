/*
 * Copyright 2003-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Phipps
 *		Jérôme Duval, jerome.duval@free.fr
 *		Axel Dörfler, axeld@pinc-software.de
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
			bool preview, ScreenSaverSettings& settings);
		~ScreenSaverRunner();

		BScreenSaver* ScreenSaver() const;
		bool HasStarted() const;

		status_t Run();
		void Quit();

		void Suspend();
		void Resume();

	private:
		void _LoadAddOn();
		void _CleanUp();
		static status_t _ThreadFunc(void* data);
		void _Run();

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
