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
//	bool QuitRequested(void);
//	void Quit(void);
	virtual void MessageReceived(BMessage *message);
	void ShowPW(void);
private:
	ScreenSaverPrefs fPref;
	SSAwindow *fWin;
	BScreenSaver *fSaver;
	ScreenSaverThread *fThrd;
	pwWindow *fPww;

	thread_id fThreadID;
	uint32 fBlankTime;

	void shutdown(void);
};

#endif //SCREEN_SAVER_H
