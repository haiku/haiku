#ifndef SCREEN_SAVER_THREAD_H
#define SCREEN_SAVER_THREAD_H
#include "SupportDefs.h"

class BScreenSaver;
class SSAwindow;
class BView;
class ScreenSaverPrefs;

int32 threadFunc(void *data);

class ScreenSaverThread 
{
public:
	ScreenSaverThread(BScreenSaver *svr, SSAwindow *wnd, BView *vw, ScreenSaverPrefs *p);
	void thread(void);
	void quit(void);
private:
	BScreenSaver *saver;
	SSAwindow *win;
	BView *view;
	ScreenSaverPrefs *pref;

	long frame;
	int snoozeCount;

};

#endif //SCREEN_SAVER_THREAD_H
