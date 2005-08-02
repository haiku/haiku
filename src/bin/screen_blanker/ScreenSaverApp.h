/*
 * Copyright 2003, Michael Phipps. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef SCREEN_SAVER_APP_H
#define SCREEN_SAVER_APP_H

#include <Application.h>
#include "SSAwindow.h"
#include "ScreenSaverPrefs.h"
#include "ScreenSaverThread.h"
#include "pwWindow.h"

class ScreenSaverApp : public BApplication 
{
public:
	ScreenSaverApp();
	bool LoadAddOn();
	void ReadyToRun();
//	bool QuitRequested();
//	void Quit();
	virtual void MessageReceived(BMessage *message);
	void ShowPW();
private:
	ScreenSaverPrefs fPref;
	SSAwindow *fWin;
	BScreenSaver *fSaver;
	ScreenSaverThread *fThrd;
	PasswordWindow *fPww;

	thread_id fThreadID;
	uint32 fBlankTime;

	void Shutdown();
};

#endif //SCREEN_SAVER_APP_H
