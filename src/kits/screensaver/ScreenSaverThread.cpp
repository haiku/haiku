#ifndef SCREEN_SAVER_THREAD_H
#include "ScreenSaverThread.h"
#endif
#include "Screen.h"
#include "ScreenSaver.h"
#include "ScreenSaverPrefs.h"
#include "FindDirectory.h"
#include <stdio.h>

int32 
threadFunc(void *data) 
{
	ScreenSaverThread *ss=(ScreenSaverThread *)data;
	ss->thread();
	return B_OK;
}


ScreenSaverThread::ScreenSaverThread(BWindow *wnd, BView *vw, ScreenSaverPrefs *p) : 
		saver(NULL),win(wnd),view(vw), pref(p), frame(0),snoozeCount(0),addon_image(0) 
{
	dwin=reinterpret_cast<BDirectWindow *>(wnd);
}


void 
ScreenSaverThread::quit(void) 
{
	saver->StopSaver();
	if (win)
		win->Hide();
}


void 
ScreenSaverThread::thread() 
{
	win->Lock();
	view->SetViewColor(0,0,0);
	view->SetLowColor(0,0,0);
	saver->StartSaver(view,false);
	win->Unlock();
	while (1) {
		snooze(saver->TickSize());
		if (snoozeCount) { // If we are sleeping, do nothing
			snoozeCount--;
			return;
		} else if (saver->LoopOnCount() && (frame>=saver->LoopOnCount())) { // Time to nap
			frame=0;
			snoozeCount=saver->LoopOffCount();
		} else {
	   		win->Lock();
			if (dwin) 
				saver->DirectDraw(frame);
	    	saver->Draw(view,frame);
	    	win->Unlock();
			frame++;
		}
	}
}


BScreenSaver *
ScreenSaverThread::LoadAddOn() 
{
	BScreenSaver *(*instantiate)(BMessage *, image_id );
	if (addon_image) { // This is a new set of preferences. Free up what we did have 
		unload_add_on(addon_image);
	}
	char temp[B_PATH_NAME_LENGTH]; 
	if (B_OK==find_directory(B_BEOS_ADDONS_DIRECTORY,NULL,false,temp,B_PATH_NAME_LENGTH)) { 
		sprintf (temp,"%s/Screen Savers/%s",temp,pref->ModuleName());
		addon_image = load_add_on(temp);
	}
	if (addon_image<0)  {
		//printf ("Unable to open add-on: %s\n",temp);
		sprintf (temp,"%s/Screen Savers/%s",temp,pref->ModuleName());
		if (B_OK==find_directory(B_COMMON_ADDONS_DIRECTORY,NULL,false,temp,B_PATH_NAME_LENGTH)) { 
			sprintf (temp,"%s/Screen Savers/%s",temp,pref->ModuleName());
			addon_image = load_add_on(temp);
		}
	}
	if (addon_image<0)  {
		//printf ("Unable to open add-on: %s\n",temp);
		if (B_OK==find_directory(B_USER_ADDONS_DIRECTORY,NULL,false,temp,B_PATH_NAME_LENGTH)) { 
			sprintf (temp,"%s/Screen Savers/%s",temp,pref->ModuleName());
			addon_image = load_add_on(temp);
		}
	}
	if (addon_image<0) {
		printf ("Unable to open add-on: %s\n",temp);
		printf ("add on image = %ld!\n",addon_image);
		return NULL;
	} else {
		// Look for the one C function that should exist.
		status_t retVal;
		if (B_OK != (retVal=get_image_symbol(addon_image, "instantiate_screen_saver", B_SYMBOL_TYPE_TEXT,(void **) &instantiate))) {
			printf ("Unable to find the instantiator\n");
			printf ("Error = %ld\n",retVal);
			return NULL;
		} else
			saver=instantiate(pref->GetState(),addon_image);
		if (B_OK!=saver->InitCheck()) {
			printf ("InitCheck() Failed!\n");
			unload_add_on(addon_image);
			delete saver;
			saver=NULL;
			return NULL;
		}
	}
	return saver;
}

