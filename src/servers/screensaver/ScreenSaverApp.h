#ifndef SCREEN_SAVER_H
#define SCREEN_SAVER_H

#ifndef _APPLICATION_H
#include <Application.h>
#endif
#include "SSAwindow.h"
#include "ScreenSaverPrefs.h"
#include "ScreenSaverThread.h"
#include "pwWindow.h"

class ScreenSaverApp : public BApplication 
{
public:
	ScreenSaverApp();
	bool LoadAddOn(void);
	void ReadyToRun(void);
	bool QuitRequested(void);
	virtual void MessageReceived(BMessage *message);
	void ShowPW(void);
private:
	ScreenSaverPrefs pref;
	SSAwindow *win;
	BScreenSaver *saver;
	ScreenSaverThread *thrd;
	pwWindow *pww;

	thread_id threadID;
	uint32 blankTime;

};

#endif //SCREEN_SAVER_H
