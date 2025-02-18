/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2007, Ryan Leavengood, leavengood@gmail.com.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <pthread.h>
#include "pthread_private.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <syscall_utils.h>
#include <time_private.h>

#include <syscalls.h>
#include <user_mutex_defs.h>


#define COND_FLAG_SHARED	0x01
#define COND_FLAG_MONOTONIC	0x02


static const pthread_condattr pthread_condattr_default = {
	false,
	CLOCK_REALTIME
};


int
pthread_cond_init(pthread_cond_t* cond, const pthread_condattr_t* _attr)
{
	const pthread_condattr* attr = _attr != NULL
		? *_attr : &pthread_condattr_default;

	cond->flags = 0;
	if (attr->process_shared)
		cond->flags |= COND_FLAG_SHARED;

	if (attr->clock_id == CLOCK_MONOTONIC)
		cond->flags |= COND_FLAG_MONOTONIC;

	cond->mutex = NULL;
	cond->waiter_count = 0;
	cond->lock = 0;

	return 0;
}


int
pthread_cond_destroy(pthread_cond_t* cond)
{
	return 0;
}


static status_t
cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex, uint32 flags,
	bigtime_t timeout)
{
	if (mutex->owner != find_thread(NULL)) {
		// calling thread isn't mutex owner
		return EPERM;
	}

	if (cond->mutex != NULL && cond->mutex != mutex) {
		// condition variable already used with different mutex
		return EINVAL;
	}

	cond->mutex = mutex;
	cond->waiter_count++;

	// make sure the user mutex we use for blocking is locked
	atomic_test_and_set((int32*)&cond->lock, B_USER_MUTEX_LOCKED, 0);

	// atomically unlock the mutex and start waiting on the user mutex
	mutex->owner = -1;
	mutex->owner_count = 0;

	if ((cond->flags & COND_FLAG_SHARED) != 0)
		flags |= B_USER_MUTEX_SHARED;
	status_t status = _kern_mutex_switch_lock((int32*)&mutex->lock,
		((mutex->flags & MUTEX_FLAG_SHARED) ? B_USER_MUTEX_SHARED : 0),
		(int32*)&cond->lock, "pthread condition", flags, timeout);

	if (status == B_INTERRUPTED) {
		// EINTR is not an allowed return value. We either have to restart
		// waiting -- which we can't atomically -- or return a spurious 0.
		status = 0;
	}

	pthread_mutex_lock(mutex);

	cond->waiter_count--;

	// If there are no more waiters, we can change mutexes.
	if (cond->waiter_count == 0) {
		cond->mutex = NULL;
		cond->lock = 0;
	}

	return status;
}


static inline void
cond_signal(pthread_cond_t* cond, bool broadcast)
{
	if (cond->waiter_count == 0)
		return;

	uint32 flags = 0;
	if (broadcast)
		flags |= B_USER_MUTEX_UNBLOCK_ALL;
	if ((cond->flags & COND_FLAG_SHARED) != 0)
		flags |= B_USER_MUTEX_SHARED;

	// release the condition lock
	if ((atomic_and((int32*)&cond->lock, ~(int32)B_USER_MUTEX_LOCKED) & B_USER_MUTEX_WAITING) != 0)
		_kern_mutex_unblock((int32*)&cond->lock, flags);
}


int
pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* _mutex)
{
	RETURN_AND_TEST_CANCEL(cond_wait(cond, _mutex, 0, B_INFINITE_TIMEOUT));
}


int
pthread_cond_clockwait(pthread_cond_t* cond, pthread_mutex_t* mutex,
	clockid_t clock_id, const struct timespec* abstime)
{
	bigtime_t timeoutMicros;
	if (abstime == NULL || !timespec_to_bigtime(*abstime, timeoutMicros))
		RETURN_AND_TEST_CANCEL(EINVAL);

	uint32 flags = 0;
	switch (clock_id) {
		case CLOCK_REALTIME:
			flags = B_ABSOLUTE_REAL_TIME_TIMEOUT;
			break;
		case CLOCK_MONOTONIC :
			flags = B_ABSOLUTE_TIMEOUT;
			break;
		default:
			return B_BAD_VALUE;
	}

	RETURN_AND_TEST_CANCEL(cond_wait(cond, mutex, flags, timeoutMicros));
}


int
pthread_cond_timedwait(pthread_cond_t* cond, pthread_mutex_t* mutex,
	const struct timespec* abstime)
{
	return pthread_cond_clockwait(cond, mutex,
		(cond->flags & COND_FLAG_MONOTONIC) != 0 ? CLOCK_MONOTONIC : CLOCK_REALTIME,
		abstime);
}


int
pthread_cond_broadcast(pthread_cond_t* cond)
{
	cond_signal(cond, true);
	return 0;
}


int
pthread_cond_signal(pthread_cond_t* cond)
{
	cond_signal(cond, false);
	return 0;
}
