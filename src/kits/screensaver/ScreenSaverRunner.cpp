/*
 * Copyright 2003-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Phipps
 *		Jérôme Duval, jerome.duval@free.fr
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "ScreenSaverRunner.h"
#include "ScreenSaverSettings.h"

#include <FindDirectory.h>
#include <Screen.h>
#include <ScreenSaver.h>
#include <View.h>

#include <stdio.h>

ScreenSaverRunner::ScreenSaverRunner(BWindow* window, BView* view,
		bool preview, ScreenSaverSettings& settings)
	:
	fSaver(NULL),
	fWindow(window),
	fView(view),
	fSettings(settings),
	fPreview(preview),
	fAddonImage(-1),
	fThread(-1),
	fQuitting(false)
{
	fDirectWindow = dynamic_cast<BDirectWindow *>(fWindow);
	_LoadAddOn();
}


ScreenSaverRunner::~ScreenSaverRunner()
{
	if (!fQuitting)
		Quit();

	_CleanUp();
}


BScreenSaver*
ScreenSaverRunner::ScreenSaver() const
{
	return fSaver;
}


bool
ScreenSaverRunner::HasStarted() const
{
	return fHasStarted;
}


status_t
ScreenSaverRunner::Run()
{
	fThread = spawn_thread(&_ThreadFunc, "ScreenSaverRenderer", B_LOW_PRIORITY, this);
	Resume();

	return fThread >= B_OK ? B_OK : fThread;
}


void
ScreenSaverRunner::Quit()
{
	fQuitting = true;
	Resume();

	if (fThread >= 0) {
		status_t returnValue;
		wait_for_thread(fThread, &returnValue);
	}
}


void
ScreenSaverRunner::Suspend()
{
	suspend_thread(fThread);
}


void
ScreenSaverRunner::Resume()
{
	resume_thread(fThread);
}


void
ScreenSaverRunner::_LoadAddOn()
{
	// This is a new set of preferences. Free up what we did have
	// TODO: this is currently not meant to be used after creation
	if (fThread >= B_OK) {
		Suspend();
		if (fSaver != NULL)
			fSaver->StopSaver();
	}
	_CleanUp();

	if (!strcmp("", fSettings.ModuleName())) {
		Resume();
		return;
	}

	BScreenSaver* (*instantiate)(BMessage*, image_id);

	// try all directories until the first one succeeds

	directory_which which[] = {
		B_USER_ADDONS_DIRECTORY,
		B_COMMON_ADDONS_DIRECTORY,
		B_SYSTEM_ADDONS_DIRECTORY,
	};
	BPath path;

	for (uint32 i = 0; i < sizeof(which) / sizeof(which[0]); i++) {
		if (find_directory(which[i], &path, false) != B_OK)
			continue;

		path.Append("Screen Savers");
		path.Append(fSettings.ModuleName());

		fAddonImage = load_add_on(path.Path());
		if (fAddonImage >= B_OK)
			break;
	}

	if (fAddonImage < B_OK) {
		printf("Unable to open add-on: %s: %s\n", path.Path(), strerror(fAddonImage));
	} else {
		// Look for the one C function that should exist.
		if (get_image_symbol(fAddonImage, "instantiate_screen_saver",
				B_SYMBOL_TYPE_TEXT, (void **)&instantiate) != B_OK) {
			printf("Unable to find the instantiator\n");
		} else {
			BMessage state;
			fSettings.GetModuleState(fSettings.ModuleName(), &state);
			fSaver = instantiate(&state, fAddonImage);
		}

		if (fSaver->InitCheck() != B_OK) {
			printf("ScreenSaver initialization failed: %s!\n", strerror(fSaver->InitCheck()));
			_CleanUp();
		}
	}

	Resume();
}


void
ScreenSaverRunner::_CleanUp()
{
	delete fSaver;
	fSaver = NULL;

	if (fAddonImage >= 0) {
		unload_add_on(fAddonImage);
		fAddonImage = -1;
	}
}


void
ScreenSaverRunner::_Run() 
{
	static const uint32 kInitialTickRate = 50000;

	if (fWindow->Lock()) {
		fView->SetViewColor(0, 0, 0);
		fView->SetLowColor(0, 0, 0);
		if (fSaver)
			fHasStarted = fSaver->StartSaver(fView, fPreview) == B_OK;
		fWindow->Unlock();
	}
	
	// TODO: This code is getting awfully complicated and should
	// probably be refactored.
	uint32 tickBase = kInitialTickRate;
	int32 snoozeCount = 0;
	int32 frame = 0;
	bigtime_t lastTickTime = 0;
	bigtime_t tick = fSaver ? fSaver->TickSize() : tickBase;

	while (!fQuitting) {
		// break the idle time up into ticks so that we can evaluate
		// the quit condition with greater responsiveness
		// otherwise a screen saver that sets, say, a 30 second tick
		// will result in the screen saver not responding to deactivation
		// for that length of time
		snooze(tickBase);
		if (system_time() - lastTickTime < tick) {
			continue;
		} else {
			// re-evaluate the tick time after each successful wakeup - 
			// screensavers can adjust it on the fly, and we must be
			// prepared to accomodate that
			tick = fSaver ? fSaver->TickSize() : tickBase;
			
			if (tick < tickBase) {
				if (tick < 0)
					tick = 0;
				tickBase = tick;
			} else if (tickBase < kInitialTickRate && tick >= kInitialTickRate) {
				tickBase = kInitialTickRate;
			}
			
			lastTickTime = system_time();
		}
		
		if (snoozeCount) {
			// if we are sleeping, do nothing
			snoozeCount--;
		} else if (fSaver != NULL && fHasStarted) {
			if (fSaver->LoopOnCount() && frame >= fSaver->LoopOnCount()) {
				// Time to nap
				frame = 0;
				snoozeCount = fSaver->LoopOffCount();
			} else if (fWindow->LockWithTimeout(5000LL) == B_OK) {
				if (!fQuitting) {
					// NOTE: R5 really calls DirectDraw()
					// and then Draw() for the same frame
					if (fDirectWindow)
						fSaver->DirectDraw(frame);
					fSaver->Draw(fView, frame);
					fView->Sync();
					frame++;
				}
				fWindow->Unlock();
			}
		} else
			snoozeCount = 1000;
	}

	if (fSaver)
		fSaver->StopSaver();
}


status_t
ScreenSaverRunner::_ThreadFunc(void *data)
{
	ScreenSaverRunner* runner = (ScreenSaverRunner*)data;
	runner->_Run();
	return B_OK;
}

