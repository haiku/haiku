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

#include <syscalls.h>
#include <user_mutex_defs.h>


#define COND_FLAG_SHARED	0x01


static const pthread_condattr pthread_condattr_default = {
	false
};


int
pthread_cond_init(pthread_cond_t* cond, const pthread_condattr_t* _attr)
{
	const pthread_condattr* attr = _attr != NULL
		? *_attr : &pthread_condattr_default;

	cond->flags = 0;
	if (attr->process_shared)
		cond->flags |= COND_FLAG_SHARED;

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
cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex, bigtime_t timeout)
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
	atomic_or((int32*)&cond->lock, B_USER_MUTEX_LOCKED);

	// atomically unlock the mutex and start waiting on the user mutex
	mutex->owner = -1;
	mutex->owner_count = 0;

	status_t status = _kern_mutex_switch_lock((int32*)&mutex->lock,
		(int32*)&cond->lock, "pthread condition",
		timeout == B_INFINITE_TIMEOUT ? 0 : B_ABSOLUTE_REAL_TIME_TIMEOUT,
		timeout);

	if (status == B_INTERRUPTED) {
		// EINTR is not an allowed return value. We either have to restart
		// waiting -- which we can't atomically -- or return a spurious 0.
		status = 0;
	}

	pthread_mutex_lock(mutex);

	cond->waiter_count--;
	// If there are no more waiters, we can change mutexes.
	if (cond->waiter_count == 0)
		cond->mutex = NULL;

	return status;
}


static inline void
cond_signal(pthread_cond_t* cond, bool broadcast)
{
	if (cond->waiter_count == 0)
		return;

	// release the condition lock
	_kern_mutex_unlock((int32*)&cond->lock,
		broadcast ? B_USER_MUTEX_UNBLOCK_ALL : 0);
}


int
pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* _mutex)
{
	RETURN_AND_TEST_CANCEL(cond_wait(cond, _mutex, B_INFINITE_TIMEOUT));
}


int
pthread_cond_timedwait(pthread_cond_t* cond, pthread_mutex_t* mutex,
	const struct timespec* tv)
{
	if (tv == NULL || tv->tv_nsec < 0 || tv->tv_nsec >= 1000 * 1000 * 1000)
		RETURN_AND_TEST_CANCEL(EINVAL);

	RETURN_AND_TEST_CANCEL(
		cond_wait(cond, mutex, tv->tv_sec * 1000000LL + tv->tv_nsec / 1000LL));
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
