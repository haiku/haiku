/*
 * Copyright 1999-2009 Jeremy Friesner
 * Copyright 2009-2014 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jeremy Friesner
 *		John Scipione, jscipione@gmail.com
 */
#ifndef MOUSE_WATCHER_H
#define MOUSE_WATCHER_H


/**** DOCUMENTATION
 *
 * Once started, MouseWatcher will watch the mouse until the mouse buttons
 * are all released, sendin messages to the target BView (TargetView is
 * specified as the target handler in the BMessenger used to send the messages.
 * The BLooper == window of the target view is determined automatically by the
 * BMessenger)
 *
 * If the mouse moves, a MW_MOUSE_MOVED message is sent.
 * If the mouse buttons are changed, but not released, a MW_MOUSE_DOWN
 * message is sent.
 * If the mouse button(s) are released, a MW_MOUSE_UP message is sent.
 *
 * These messages will have three data entries:
 *
 * "where" (B_POINT_TYPE)		- The position of the mouse in TargetView's
 *							  coordinate system.
 * "buttons" (B_INT32_TYPE)	- The mouse buttons.  See BView::GetMouse().
 * "modifiers" (B_INT32_TYPE)	- The modifier keys held down at the time.
 *							  See modifiers().
 *
 * Once it is started, you can't stop it, but that shouldn't matter - the user
 * will most likely releas the buttons soon, and you can interpret the events
 * however you want.
 *
 * StartMouseWatcher returns a valid thread ID, or it returns an error code:
 *	B_NO_MORE_THREADS All thread_id numbers are currently in use.
 *	B_NO_MEMORY Not enough memory to allocate the resources for another
 *	            thread.
 */


#include <SupportDefs.h>
#include <OS.h>


const uint32 MW_MOUSE_DOWN = 'Mw-D';
const uint32 MW_MOUSE_UP = 'Mw-U';
const uint32 MW_MOUSE_MOVED = 'Mw-M';


class BView;

thread_id StartMouseWatcher(BView* target);


#endif	// MOUSE_WATCHER_H
