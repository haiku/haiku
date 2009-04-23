/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <listeners.h>


WaitObjectListenerList gWaitObjectListeners;
spinlock gWaitObjectListenerLock = B_SPINLOCK_INITIALIZER;


WaitObjectListener::~WaitObjectListener()
{
}


/*!	Add the given wait object listener. gWaitObjectListenerLock lock must be
	held.
*/
void
add_wait_object_listener(struct WaitObjectListener* listener)
{
	gWaitObjectListeners.Add(listener);
}


/*!	Remove the given wait object listener. gWaitObjectListenerLock lock must be
	held.
*/
void
remove_wait_object_listener(struct WaitObjectListener* listener)
{
	gWaitObjectListeners.Remove(listener);
}
