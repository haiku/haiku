/*
 * Copyright 2003-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Phipps
 *		Jérôme Duval, jerome.duval@free.fr
 */
#include <FindDirectory.h>
#include <Screen.h>
#include <ScreenSaver.h>
#include <View.h>
#include <stdio.h>
#include "ScreenSaverThread.h"
#include "ScreenSaverPrefs.h"

int32 
ScreenSaverThread::ThreadFunc(void *data) 
{
	ScreenSaverThread* ss = (ScreenSaverThread*)data;
	ss->Thread();
	return B_OK;
}


ScreenSaverThread::ScreenSaverThread(BWindow* window,
									 BView* view,
									 ScreenSaverPrefs* prefs) 
	:	fSaver(NULL),
		fWin(window),
		fView(view),
		fPref(prefs),
		fFrame(0),
		fSnoozeCount(0),
		fAddonImage(-1),

		fQuitting(false)
{
	fDWin = dynamic_cast<BDirectWindow *>(fWin);
}


ScreenSaverThread::~ScreenSaverThread()
{
	_CleanUp();
}


void 
ScreenSaverThread::Quit(thread_id id)
{
	fQuitting = true;
	if (id >= 0) {
		status_t threadRetValue;
		wait_for_thread(id, &threadRetValue);
	}

	if (fSaver)
		fSaver->StopSaver();
}


void 
ScreenSaverThread::Thread() 
{
	if (fWin->Lock()) {
		fView->SetViewColor(0,0,0);
		fView->SetLowColor(0,0,0);
		if (fSaver)
			fSaver->StartSaver(fView, false);
		fWin->Unlock();
	}
	while (!fQuitting) {
		snooze(fSaver->TickSize());
		if (fSnoozeCount) {
			// if we are sleeping, do nothing
			fSnoozeCount--;
			return;
		} else if (fSaver->LoopOnCount() && (fFrame >= fSaver->LoopOnCount())) { // Time to nap
			fFrame = 0;
			fSnoozeCount = fSaver->LoopOffCount();
		} else if (fWin->Lock()) {
			// NOTE: R5 really calls DirectDraw()
			// and then Draw() for the same frame
			if (fDWin)
				fSaver->DirectDraw(fFrame);
			fSaver->Draw(fView, fFrame);
			fWin->Unlock();
			fFrame++;
		}
	}
}


BScreenSaver*
ScreenSaverThread::LoadAddOn() 
{
	if (strcmp("", fPref->ModuleName()) == 0)
		return NULL;

	// This is a new set of preferences. Free up what we did have
	_CleanUp();

	BScreenSaver* (*instantiate)(BMessage*, image_id);

	char temp[B_PATH_NAME_LENGTH]; 
	if (B_OK==find_directory(B_BEOS_ADDONS_DIRECTORY, 0, false, temp, B_PATH_NAME_LENGTH)) { 
		sprintf (temp,"%s/Screen Savers/%s", temp, fPref->ModuleName());
		fAddonImage = load_add_on(temp);
	}
	if (fAddonImage < 0)  {
		//printf ("Unable to open add-on: %s\n",temp);
		sprintf (temp,"%s/Screen Savers/%s", temp, fPref->ModuleName());
		if (B_OK==find_directory(B_COMMON_ADDONS_DIRECTORY, 0, false, temp, B_PATH_NAME_LENGTH)) { 
			sprintf (temp,"%s/Screen Savers/%s", temp, fPref->ModuleName());
			fAddonImage = load_add_on(temp);
		}
	}
	if (fAddonImage < 0)  {
		//printf ("Unable to open add-on: %s\n",temp);
		if (B_OK==find_directory(B_USER_ADDONS_DIRECTORY, 0, false, temp, B_PATH_NAME_LENGTH)) { 
			sprintf (temp,"%s/Screen Savers/%s", temp, fPref->ModuleName());
			fAddonImage = load_add_on(temp);
		}
	}
	if (fAddonImage < 0) {
		printf ("Unable to open add-on: %s\n",temp);
		printf ("add on image = %ld!\n", fAddonImage);
		return NULL;
	} else {
		// Look for the one C function that should exist.
		status_t retVal;
		if (B_OK != (retVal=get_image_symbol(fAddonImage, "instantiate_screen_saver", B_SYMBOL_TYPE_TEXT,(void **) &instantiate))) {
			printf ("Unable to find the instantiator\n");
			printf ("Error = %ld\n",retVal);
			return NULL;
		} else {
			BMessage state;
			fPref->GetState(fPref->ModuleName(), &state);
			fSaver = instantiate(&state, fAddonImage);
		}
		if (B_OK != fSaver->InitCheck()) {
			printf ("InitCheck() Failed!\n");
			unload_add_on(fAddonImage);
			delete fSaver;
			fSaver = NULL;
			return NULL;
		}
	}
	return fSaver;
}


void
ScreenSaverThread::_CleanUp()
{
	delete fSaver;
	fSaver = NULL;
	if (fAddonImage >= 0) {
		unload_add_on(fAddonImage);
		fAddonImage = -1;
	}
}
