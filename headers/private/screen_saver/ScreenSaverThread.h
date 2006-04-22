/*
 * Copyright 2003, Michael Phipps. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef SCREEN_SAVER_THREAD_H
#define SCREEN_SAVER_THREAD_H
#include <SupportDefs.h>
#include <DirectWindow.h>

class BScreenSaver;
class BView;
class ScreenSaverPrefs;

class ScreenSaverThread  {
 public:
								ScreenSaverThread(BWindow* window,
												  BView* view,
												  ScreenSaverPrefs* prefs);
								~ScreenSaverThread();

			void				Thread();
			BScreenSaver*		LoadAddOn();
			void				Quit(thread_id thread);

	static	int32				ThreadFunc(void* data);
 private:
			void				_CleanUp();

			BScreenSaver*		fSaver;
			BWindow*			fWin;
			BDirectWindow*		fDWin;
			BView*				fView;
			ScreenSaverPrefs*	fPref;
		
			long				fFrame;
			int					fSnoozeCount;
			image_id			fAddonImage;

	volatile bool				fQuitting;
};

#endif //SCREEN_SAVER_THREAD_H
