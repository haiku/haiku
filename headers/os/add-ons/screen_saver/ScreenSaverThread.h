#ifndef SCREEN_SAVER_THREAD_H
#define SCREEN_SAVER_THREAD_H
#include "SupportDefs.h"
#include "DirectWindow.h"

class BScreenSaver;
class BView;
class ScreenSaverPrefs;

int32 threadFunc(void *data);

class ScreenSaverThread 
{
public:
	ScreenSaverThread(BScreenSaver *svr, BDirectWindow *wnd, BView *vw, ScreenSaverPrefs *p);
	void thread(void);
	void quit(void);
private:
	BScreenSaver *saver;
	BDirectWindow *win;
	BView *view;
	ScreenSaverPrefs *pref;

	long frame;
	int snoozeCount;
};

#endif //SCREEN_SAVER_THREAD_H
