/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "pthread_private.h"

#include <syscalls.h>


static inline void
test_asynchronous_cancel(int32 flags)
{
	static const int32 kFlags = THREAD_CANCELED | THREAD_CANCEL_ENABLED
		| THREAD_CANCEL_ASYNCHRONOUS;

	if ((~flags & kFlags) == 0)
		pthread_exit(PTHREAD_CANCELED);
}


/*!	Signal handler like function invoked when this thread has been canceled.
	Has the simple signal handler signature, since it is invoked just like a
	signal handler.
*/
static void
asynchronous_cancel_thread(int)
{
	pthread_t thread = pthread_self();

	// Exit when asynchronous cancellation is enabled, otherwise we don't have
	// to do anything -- the syscall interrupting side effect is all we need.
	if ((atomic_get(&thread->flags) & THREAD_CANCEL_ASYNCHRONOUS) != 0)
		pthread_exit(PTHREAD_CANCELED);
}


// #pragma mark - public API


int
pthread_cancel(pthread_t thread)
{
	// set the canceled flag
	int32 oldFlags = atomic_or(&thread->flags, THREAD_CANCELED);

	// If the flag was already set, we're done.
	if ((oldFlags & THREAD_CANCELED) != 0)
		return 0;

	// If cancellation is enabled, notify the thread. This will call the
	// asynchronous_cancel_thread() handler.
	if ((oldFlags & THREAD_CANCEL_ENABLED) != 0)
		return _kern_cancel_thread(thread->id, &asynchronous_cancel_thread);

	return 0;
}


int
pthread_setcancelstate(int state, int *_oldState)
{
	pthread_thread* thread = pthread_self();
	if (thread == NULL)
		return EINVAL;

	// set the new flags
	int32 oldFlags;
	if (state == PTHREAD_CANCEL_ENABLE) {
		oldFlags = atomic_or(&thread->flags, THREAD_CANCEL_ENABLED);
		test_asynchronous_cancel(oldFlags | THREAD_CANCEL_ENABLED);
	} else if (state == PTHREAD_CANCEL_DISABLE) {
		oldFlags = atomic_and(&thread->flags, ~(int32)THREAD_CANCEL_ENABLED);
		test_asynchronous_cancel(oldFlags);
	} else
		return EINVAL;

	// return the old state
	if (_oldState != NULL) {
		*_oldState = (oldFlags & THREAD_CANCEL_ENABLED) != 0
			? PTHREAD_CANCEL_ENABLE : PTHREAD_CANCEL_DISABLE;
	}

	return 0;
}


int
pthread_setcanceltype(int type, int *_oldType)
{
	pthread_thread* thread = pthread_self();
	if (thread == NULL)
		return EINVAL;

	// set the new type
	int32 oldFlags;
	if (type == PTHREAD_CANCEL_DEFERRED) {
		oldFlags = atomic_and(&thread->flags,
			~(int32)THREAD_CANCEL_ASYNCHRONOUS);
		test_asynchronous_cancel(oldFlags);
	} else if (type == PTHREAD_CANCEL_ASYNCHRONOUS) {
		oldFlags = atomic_or(&thread->flags, THREAD_CANCEL_ASYNCHRONOUS);
		test_asynchronous_cancel(oldFlags | THREAD_CANCEL_ASYNCHRONOUS);
	} else
		return EINVAL;

	// return the old type
	if (_oldType != NULL) {
		*_oldType = (oldFlags & THREAD_CANCEL_ASYNCHRONOUS) != 0
			? PTHREAD_CANCEL_ASYNCHRONOUS : PTHREAD_CANCEL_DEFERRED;
	}

	return 0;
}


void
pthread_testcancel(void)
{
	pthread_thread* thread = pthread_self();
	if (thread == NULL)
		return;

	static const int32 kFlags = THREAD_CANCELED | THREAD_CANCEL_ENABLED;

	if ((~atomic_get(&thread->flags) & kFlags) == 0)
		pthread_exit(NULL);
}
