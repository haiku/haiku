#ifndef SCREEN_SAVER_THREAD_H
#define SCREEN_SAVER_THREAD_H
#include <SupportDefs.h>
#include <DirectWindow.h>

class BScreenSaver;
class BView;
class ScreenSaverPrefs;

int32 threadFunc(void *data);

class ScreenSaverThread 
{
public:
	ScreenSaverThread(BWindow *wnd, BView *vw, ScreenSaverPrefs *p);
	void thread(void);
	BScreenSaver *LoadAddOn(void) ;
	void quit(void);
private:
	BScreenSaver *saver;
	BWindow *win;
	BDirectWindow *dwin;
	BView *view;
	ScreenSaverPrefs *pref;

	long frame;
	int snoozeCount;
	image_id addon_image;
};

#endif //SCREEN_SAVER_THREAD_H
