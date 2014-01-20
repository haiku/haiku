/*
 * Copyright 1999-2009 Jeremy Friesner
 * Copyright 2009-2014 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jeremy Friesner
 *		John Scipione, jscipione@gmail.com
 */


#include "MouseWatcher.h"

#include <Messenger.h>
#include <InterfaceKit.h>


int32 MouseWatcher(void* data);


thread_id
StartMouseWatcher(BView* target)
{
	thread_id MouseWatcherThread = spawn_thread(MouseWatcher,
		"MouseWatcher", B_NORMAL_PRIORITY, new BMessenger(target));
	if (MouseWatcherThread != B_NO_MORE_THREADS
		&& MouseWatcherThread != B_NO_MEMORY) {
		resume_thread(MouseWatcherThread);
	}

	return MouseWatcherThread;
}


int32
MouseWatcher(void* data)
{
	BMessenger* messenger = (BMessenger*)data;
	BPoint previousPosition;
	uint32 previousbuttons = 0xFFFFFFFF;
	bool isFirstCheck = true;
	BMessage messageToSend;
	messageToSend.AddPoint("where", BPoint(0, 0));
	messageToSend.AddInt32("buttons", 0);
	messageToSend.AddInt32("modifiers", 0);

	while(true) {
		if (!messenger->LockTarget()) {
			// window is dead so exit
			delete messenger;
			return 0;
		}
		BLooper* looper;
		BView* view = (BView*)messenger->Target(&looper);
		BPoint where;
		uint32 buttons;
		view->GetMouse(&where, &buttons, false);
		if (isFirstCheck) {
			previousPosition = where;
			previousbuttons = buttons;
			isFirstCheck = false;
		}
		bool shouldSend = false;
		if (buttons != previousbuttons || buttons == 0
			|| where != previousPosition) {
			if (buttons == 0)
				messageToSend.what = MW_MOUSE_UP;
			else if (buttons != previousbuttons)
				messageToSend.what = MW_MOUSE_DOWN;
			else
				messageToSend.what = MW_MOUSE_MOVED;

			messageToSend.ReplacePoint("where", where);
			messageToSend.ReplaceInt32("buttons", buttons);
			messageToSend.ReplaceInt32("modifiers", modifiers());
			shouldSend = true;
		}

		looper->Unlock();
		if (shouldSend)
			messenger->SendMessage(&messageToSend);

		if (buttons == 0) {
			// mouse button was released
			delete messenger;
			return 0;
		}

		snooze(50000);
	}
}
