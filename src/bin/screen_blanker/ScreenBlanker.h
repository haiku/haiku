/*
 * Copyright 2003-2014 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval, jerome.duval@free.fr
 * 		Michael Phipps
 *		John Scipione, jscipione@gmail.com
 *		Puck Meerburg, puck@puckipedia.nl
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

	virtual	void				ReadyToRun();

	virtual	bool				QuitRequested();
	virtual	void				MessageReceived(BMessage* message);

			bool				IsPasswordWindowShown() const;

private:
			bool				_LoadAddOn();
			void				_ShowPasswordWindow();
			void				_QueueResumeScreenSaver();
			void				_TurnOnScreen();
			void				_SetDPMSMode(uint32 mode);
			void				_QueueTurnOffScreen();
			void				_Shutdown();

			ScreenSaverSettings	fSettings;
			ScreenSaverWindow*	fWindow;
			ScreenSaverRunner*	fSaverRunner;
			PasswordWindow*		fPasswordWindow;

			bigtime_t			fBlankTime;
			bool				fTestSaver;
			BMessageRunner*		fResumeRunner;

			BMessageRunner*		fStandByScreenRunner;
			BMessageRunner*		fSuspendScreenRunner;
			BMessageRunner*		fTurnOffScreenRunner;
};

#endif	// SCREEN_SAVER_APP_H
