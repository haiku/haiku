/*
 * Copyright 2003-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Michael Phipps
 *		Jérôme Duval, jerome.duval@free.fr
 */
#ifndef SCREEN_SAVER_APP_H
#define SCREEN_SAVER_APP_H


#include "PasswordWindow.h"
#include "ScreenSaverPrefs.h"
#include "ScreenSaverThread.h"
#include "ScreenSaverWindow.h"

#include <Application.h>
#include <MessageRunner.h>


class ScreenSaverApp : public BApplication {
	public:
		ScreenSaverApp();

		virtual void ReadyToRun();

		virtual bool QuitRequested();
		virtual void MessageReceived(BMessage *message);

	private:
		bool _LoadAddOn();
		void _ShowPasswordWindow();
		void _ResumeScreenSaver();
		void _Shutdown();

		ScreenSaverPrefs fPrefs;
		ScreenSaverWindow *fWindow;
		BScreenSaver *fSaver;
		ScreenSaverThread *fThread;
		PasswordWindow *fPasswordWindow;

		thread_id fThreadID;
		bigtime_t fBlankTime;
		BMessageRunner *fRunner;
};

#endif	// SCREEN_SAVER_APP_H
