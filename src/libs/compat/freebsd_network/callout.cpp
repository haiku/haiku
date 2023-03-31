/*
 * Copyright 2010, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2018-2023, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 */


#include <lock.h>
#include <thread.h>

extern "C" {
#	include "device.h"
#	include <sys/callout.h>
#	include <sys/mutex.h>
}

#include <util/AutoLock.h>


//#define TRACE_CALLOUT
#ifdef TRACE_CALLOUT
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...) ;
#endif


static struct list sTimers;
static mutex sLock;
static sem_id sWaitSem;
static callout* sCurrentCallout;
static thread_id sThread;
static bigtime_t sTimeout;


static void
invoke_callout(callout *c, struct mtx *c_mtx)
{
	if (c_mtx != NULL) {
		mtx_lock(c_mtx);

		if (c->c_due < 0) {
			mtx_unlock(c_mtx);
			return;
		}
		c->c_due = -1;
	}

	c->c_func(c->c_arg);

	if (c_mtx != NULL && (c->c_flags & CALLOUT_RETURNUNLOCKED) == 0)
		mtx_unlock(c_mtx);
}


static status_t
callout_thread(void* /*data*/)
{
	status_t status = B_NO_INIT;

	do {
		bigtime_t timeout = B_INFINITE_TIMEOUT;

		if (status == B_TIMED_OUT || status == B_OK) {
			// scan timers for new timeout and/or execute a timer
			if ((status = mutex_lock(&sLock)) != B_OK)
				continue;

			struct callout* c = NULL;
			while (true) {
				c = (callout*)list_get_next_item(&sTimers, c);
				if (c == NULL)
					break;

				if (c->c_due > system_time()) {
					// calculate new timeout
					if (timeout > c->c_due)
						timeout = c->c_due;
					continue;
				}

				// execute timer
				list_remove_item(&sTimers, c);
				struct mtx *c_mtx = c->c_mtx;
				c->c_due = 0;
				sCurrentCallout = c;

				mutex_unlock(&sLock);

				invoke_callout(c, c_mtx);

				if ((status = mutex_lock(&sLock)) != B_OK)
					break;

				sCurrentCallout = NULL;
				c = NULL;
					// restart scanning as we unlocked the list
			}

			sTimeout = timeout;
			mutex_unlock(&sLock);
		}

		status = acquire_sem_etc(sWaitSem, 1, B_ABSOLUTE_TIMEOUT, timeout);
			// the wait sem normally can't be acquired, so we
			// have to look at the status value the call returns:
			//
			// B_OK - a new timer has been added or canceled
			// B_TIMED_OUT - look for timers to be executed
			// B_BAD_SEM_ID - we are asked to quit
	} while (status != B_BAD_SEM_ID);

	return B_OK;
}


// #pragma mark - private API


status_t
init_callout(void)
{
	list_init(&sTimers);
	sTimeout = B_INFINITE_TIMEOUT;

	status_t status = B_OK;
	mutex_init(&sLock, "fbsd callout");

	sWaitSem = create_sem(0, "fbsd callout wait");
	if (sWaitSem < 0) {
		status = sWaitSem;
		goto err1;
	}

	sThread = spawn_kernel_thread(callout_thread, "fbsd callout",
		B_DISPLAY_PRIORITY, NULL);
	if (sThread < 0) {
		status = sThread;
		goto err2;
	}

	return resume_thread(sThread);

err2:
	delete_sem(sWaitSem);
err1:
	mutex_destroy(&sLock);
	return status;
}


void
uninit_callout(void)
{
	delete_sem(sWaitSem);

	wait_for_thread(sThread, NULL);

	mutex_lock(&sLock);
	mutex_destroy(&sLock);
}


// #pragma mark - public API


void
callout_init(struct callout *callout, int mpsafe)
{
	if (mpsafe)
		callout_init_mtx(callout, NULL, 0);
	else
		callout_init_mtx(callout, &Giant, 0);
}


void
callout_init_mtx(struct callout *c, struct mtx *mtx, int flags)
{
	c->c_due = 0;

	c->c_arg = NULL;
	c->c_func = NULL;
	c->c_mtx = mtx;
	c->c_flags = flags;
}


int
callout_reset(struct callout *c, int _ticks, void (*func)(void *), void *arg)
{
	int cancelled = callout_stop(c);

	MutexLocker locker(sLock);

	c->c_func = func;
	c->c_arg = arg;

	TRACE("callout_reset %p, func %p, arg %p\n", c, c->c_func, c->c_arg);

	if (_ticks >= 0) {
		// reschedule or add this timer
		if (c->c_due <= 0)
			list_add_item(&sTimers, c);

		c->c_due = system_time() + TICKS_2_USEC(_ticks);

		// notify timer about the change if necessary
		if (sTimeout > c->c_due)
			release_sem(sWaitSem);
	}

	return (cancelled == -1) ? 0 : 1;
}


int
callout_schedule(struct callout *callout, int _ticks)
{
	return callout_reset(callout, _ticks, callout->c_func, callout->c_arg);
}


int
_callout_stop_safe(struct callout *c, int safe)
{
	if (c == NULL)
		return -1;

	TRACE("_callout_stop_safe %p, func %p, arg %p\n", c, c->c_func, c->c_arg);

	MutexLocker locker(sLock);

	int ret = -1;
	if (callout_active(c)) {
		ret = 0;
		if (!safe && c->c_mtx != NULL && c->c_due == 0) {
			mtx_assert(c->c_mtx, MA_OWNED);

			// The callout is active, but c_due == 0 and we hold the locks: this
			// means the callout thread has dequeued it and is waiting for c_mtx.
			// Clear c_due to signal the callout thread.
			c->c_due = -1;
			ret = 1;
		}
		if (safe) {
			locker.Unlock();
			while (callout_active(c))
				snooze(100);
			locker.Lock();
		}
	}

	if (c->c_due <= 0)
		return ret;

	// this timer is scheduled, cancel it
	list_remove_item(&sTimers, c);
	c->c_due = 0;
	return (ret == -1) ? 1 : ret;
}


int
callout_pending(struct callout *c)
{
	return c->c_due > 0;
}


int
callout_active(struct callout *c)
{
	return c == sCurrentCallout;
}
