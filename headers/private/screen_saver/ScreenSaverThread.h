#ifndef SCREEN_SAVER_THREAD_H
#define SCREEN_SAVER_THREAD_H
#include <SupportDefs.h>
#include <DirectWindow.h>

class BScreenSaver;
class BView;
class ScreenSaverPrefs;

class ScreenSaverThread 
{
public:
	ScreenSaverThread(BWindow *wnd, BView *vw, ScreenSaverPrefs *p);
	void Thread();
	BScreenSaver *LoadAddOn() ;
	void Quit();

	static int32 ThreadFunc(void *data);
private:
	BScreenSaver *fSaver;
	BWindow *fWin;
	BDirectWindow *fDWin;
	BView *fView;
	ScreenSaverPrefs *fPref;

	long fFrame;
	int fSnoozeCount;
	image_id fAddonImage;
};

#endif //SCREEN_SAVER_THREAD_H
