#ifndef SCREEN_SAVER_H
#define SCREEN_SAVER_H

#ifndef _APPLICATION_H
#include <Application.h>
#endif
#include "SSAwindow.h"
#include "ScreenSaverPrefs.h"
#include "ScreenSaverThread.h"

class ScreenSaverApp : public BApplication 
{
public:
	ScreenSaverApp();
	bool LoadAddOn(void);
	void ReadyToRun(void);
	bool QuitRequested(void);
private:
	ScreenSaverPrefs pref;
	image_id addon_image;
	SSAwindow *win;
	BScreenSaver *saver;
	ScreenSaverThread *thrd;

	thread_id threadID;

};

#endif //SCREEN_SAVER_H
