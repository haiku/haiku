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
#include "ScreenSaverSettings.h"
#include "ScreenSaverRunner.h"
#include "ScreenSaverWindow.h"

#include <Application.h>
#include <MessageRunner.h>


const static uint32 kMsgResumeSaver = 'RSSV';


class ScreenBlanker : public BApplication {
	public:
		ScreenBlanker();
		~ScreenBlanker();

		virtual void ReadyToRun();

		virtual bool QuitRequested();
		virtual void MessageReceived(BMessage* message);

	private:
		bool _LoadAddOn();
		void _ShowPasswordWindow();
		void _QueueResumeScreenSaver();
		void _TurnOnScreen();
		void _SetDPMSMode(uint32 mode);
		void _QueueTurnOffScreen();
		void _Shutdown();

		ScreenSaverSettings fSettings;
		ScreenSaverWindow *fWindow;
		BScreenSaver *fSaver;
		ScreenSaverRunner *fRunner;
		PasswordWindow *fPasswordWindow;

		bigtime_t fBlankTime;
		BMessageRunner* fResumeRunner;

		BMessageRunner* fStandByScreenRunner;
		BMessageRunner* fSuspendScreenRunner;
		BMessageRunner* fTurnOffScreenRunner;
};

#endif	// SCREEN_SAVER_APP_H
