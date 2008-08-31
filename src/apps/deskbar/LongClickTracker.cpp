/*
 * Copyright 2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexandre Deckner <alex@zappotek.com>
 */

#include "LongClickTracker.h"

#include <View.h>


LongClickTracker::LongClickTracker(BView *view, uint32 messageWhat)
	: fView(view),
	fMessenger(NULL),
	fMessageWhat(messageWhat),
	fThread(B_ERROR),
	fQuit(false)
{
	// use the doubleClickSpeed as a threshold
	get_click_speed(&fLongClickThreshold);
}


LongClickTracker::~LongClickTracker()
{
	if (fThread != B_NO_ERROR)
    	return;

	fQuit = true;

	status_t ret;
	wait_for_thread(fThread, &ret);

	delete fMessenger;
}


// Must be called _after_ the view has been attached to a window
status_t
LongClickTracker::Start()
{
	status_t err;

	if (!fView->Window())
		return B_ERROR;

	fMessenger = new BMessenger(fView, NULL, &err);
	if (err != B_OK)
		return err;

	fThread = spawn_thread(LongClickTracker::_ThreadEntry, "clickTracker",
		 B_NORMAL_PRIORITY,	this);

	err = resume_thread(fThread);

	if (err != B_NO_ERROR) {
		kill_thread(fThread);
		fThread = B_ERROR;
	} else {
		fQuit = false;
	}

	return err;
}


int32
LongClickTracker::_ThreadEntry(void *pointer)
{
	LongClickTracker *that = reinterpret_cast<LongClickTracker*>(pointer);
	that->_Track();

	return B_OK;
}


void
LongClickTracker::_Track()
{
	uint32 buttons;
	BPoint position;

	bool timing = false;
		//when true, we are currently timing the last click duration
	bool ready = true;
		//when true, button has been released, we can start timing on next click

	bigtime_t clickTime = 0;

	BRect bounds;
	fView->LockLooper();
	bounds = fView->Bounds();
	fView->UnlockLooper();

	while (!fQuit) {
		snooze(20000);

		if (fView->Window()) {
			fView->LockLooper();
			fView->GetMouse(&position, &buttons, false);
			fView->UnlockLooper();
		}

		if (timing) {
			if (!bounds.Contains(position)) {
				//mouse exited the view, stop timing
				timing = false;
				ready = false; //not ready yet, the button might be still down
			}

			if (buttons != B_PRIMARY_MOUSE_BUTTON) {
				//button has been released, stop timing, set ready
				timing = false;
				ready = true;

			} else if ((system_time() - clickTime) > fLongClickThreshold){
				BMessage message(fMessageWhat);
      			message.AddPoint("where", position);
				fMessenger->SendMessage(&message);
				timing = false;
				ready = false;
			}
		}

		if (!ready && !timing && (buttons != B_PRIMARY_MOUSE_BUTTON)) {
			//mouse released, ready to time on next click
			ready = true;
		}

		if (ready && !timing && buttons == B_PRIMARY_MOUSE_BUTTON
			&& bounds.Contains(position)) {
			//mouse clicked in view, start timing
			timing = true;
			clickTime = system_time();
		}
	}
}
