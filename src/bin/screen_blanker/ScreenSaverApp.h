/*
 * Copyright 2003-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Michael Phipps
 *		Jérôme Duval, jerome.duval@free.fr
 */

#ifndef SCREEN_SAVER_APP_H
#define SCREEN_SAVER_APP_H

#include <Application.h>
#include <MessageRunner.h>
#include "SSAwindow.h"
#include "ScreenSaverPrefs.h"
#include "ScreenSaverThread.h"
#include "PasswordWindow.h"

class ScreenSaverApp : public BApplication 
{
public:
	ScreenSaverApp();
	bool LoadAddOn();
	void ReadyToRun();
	virtual bool QuitRequested();
	virtual void MessageReceived(BMessage *message);
	void ShowPW();
private:
	ScreenSaverPrefs fPref;
	SSAwindow *fWin;
	BScreenSaver *fSaver;
	ScreenSaverThread *fThrd;
	PasswordWindow *fPww;

	thread_id fThreadID;
	bigtime_t fBlankTime;
	BMessageRunner *fRunner;

	void Shutdown();
};

#endif //SCREEN_SAVER_APP_H
