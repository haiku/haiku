#ifndef SCREEN_SAVER_THREAD_H
#include "ScreenSaverThread.h"
#endif
#include <stdio.h>
#include "Screen.h"
#include "ScreenSaver.h"
#include "SSAwindow.h"
#include "ScreenSaverPrefs.h"

int32 threadFunc(void *data) {
	ScreenSaverThread *ss=(ScreenSaverThread *)data;
	ss->thread();
}

ScreenSaverThread::ScreenSaverThread(BScreenSaver *svr, SSAwindow *wnd, BView *vw, ScreenSaverPrefs *p) : 
		saver(svr),win(wnd),view(vw), pref(p), frame(0),snoozeCount(0) {
		if (win) { // Running in full screen mode
			saver->StartSaver(win->view,false);
			win->SetFullScreen(true);
			win->Show(); 
			win->Lock();
			win->view->SetViewColor(0,0,0);
			win->view->SetLowColor(0,0,0);
			win->Unlock();
			win->Sync(); 
		}
}

void ScreenSaverThread::quit(void) {
	saver->StopSaver();
	if (win)
		win->Hide();
}

void ScreenSaverThread::thread() {
	while (1) {
		snooze(saver->TickSize());
		if (snoozeCount) { // If we are sleeping, do nothing
			snoozeCount--;
			return;
		} else if (saver->LoopOnCount() && (frame>=saver->LoopOnCount())) { // Time to nap
			frame=0;
			snoozeCount=saver->LoopOffCount();
		}
		else {
			if (win) {
				saver->DirectDraw(frame);
	   			win->Lock();
			}
	    	saver->Draw(view,frame);
			if (win) {
	    		win->Unlock();
				win->Sync();
			}
			frame++;
		}
	}
}

