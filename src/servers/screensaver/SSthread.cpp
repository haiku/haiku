#include "ScreenSaverApp.h"
#include <ScreenSaver.h>
#include <View.h>
#include <stdio.h>
#include <unistd.h>

extern SSstates nextAction; 
extern int32 frame;
extern BView *view;
extern BScreenSaver *saver;
extern sem_id ssSem;
extern SSAwindow *win;

int32 screenSaverThread(void *none)
{
	bool done=false;
	while (!done)
		switch (nextAction)
			{
			case DRAW:
				saver->Draw(view,frame++);
				break;
			case DIRECTDRAW:
				saver->DirectDraw(frame);
				win->Lock();
				saver->Draw(view,frame++);
				win->Unlock();
				break;
			case STOP:
				frame=0;
				acquire_sem(ssSem);
				break;
			case EXIT:
				done=true;
				delete saver;
				break;
			}
	return 0;
}

void callDirectConnected (direct_buffer_info *info)
{
	saver->DirectConnected(info);
}

void callSaverDelete(void)
{
	delete saver;
}

