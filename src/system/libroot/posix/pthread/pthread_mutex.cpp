/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2003-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <pthread.h>
#include "pthread_private.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <syscalls.h>
#include <user_mutex_defs.h>


#define MUTEX_FLAG_SHARED	0x80000000
#define MUTEX_TYPE_BITS		0x0000000f
#define MUTEX_TYPE(mutex)	((mutex)->flags & MUTEX_TYPE_BITS)


static const pthread_mutexattr pthread_mutexattr_default = {
	PTHREAD_MUTEX_DEFAULT,
	false
};


int
pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t* _attr)
{
	const pthread_mutexattr* attr = _attr != NULL
		? *_attr : &pthread_mutexattr_default;

	mutex->lock = 0;
	mutex->owner = -1;
	mutex->owner_count = 0;
	mutex->flags = attr->type | (attr->process_shared ? MUTEX_FLAG_SHARED : 0);

	return 0;
}


int
pthread_mutex_destroy(pthread_mutex_t* mutex)
{
	return 0;
}


static status_t
mutex_lock(pthread_mutex_t* mutex, bigtime_t timeout)
{
	thread_id thisThread = find_thread(NULL);

	if (mutex->owner == thisThread) {
		// recursive locking handling
		if (MUTEX_TYPE(mutex) == PTHREAD_MUTEX_RECURSIVE) {
			if (mutex->owner_count == INT32_MAX)
				return EAGAIN;

			mutex->owner_count++;
			return 0;
		}

		// deadlock check (not for PTHREAD_MUTEX_NORMAL as per the specs)
		if (MUTEX_TYPE(mutex) == PTHREAD_MUTEX_ERRORCHECK
			|| MUTEX_TYPE(mutex) == PTHREAD_MUTEX_DEFAULT) {
			// we detect this kind of deadlock and return an error
			return timeout < 0 ? EBUSY : EDEADLK;
		}
	}

	// set the locked flag
	int32 oldValue = atomic_or((int32*)&mutex->lock, B_USER_MUTEX_LOCKED);

	if ((oldValue & (B_USER_MUTEX_LOCKED | B_USER_MUTEX_WAITING)) != 0) {
		// someone else has the lock or is at least waiting for it
		if (timeout < 0)
			return EBUSY;

		// we have to call the kernel
		status_t error;
		do {
			error = _kern_mutex_lock((int32*)&mutex->lock, NULL,
				timeout == B_INFINITE_TIMEOUT
					? 0 : B_ABSOLUTE_REAL_TIME_TIMEOUT,
				timeout);
		} while (error == B_INTERRUPTED);

		if (error != B_OK)
			return error;
	}

	// we have locked the mutex for the first time
	mutex->owner = thisThread;
	mutex->owner_count = 1;

	return 0;
}


int
pthread_mutex_lock(pthread_mutex_t* mutex)
{
	return mutex_lock(mutex, B_INFINITE_TIMEOUT);
}


int
pthread_mutex_trylock(pthread_mutex_t* mutex)
{
	return mutex_lock(mutex, -1);
}


int
pthread_mutex_timedlock(pthread_mutex_t* mutex, const struct timespec* tv)
{
	// translate the timeout
	bool invalidTime = false;
	bigtime_t timeout = 0;
	if (tv && tv->tv_nsec < 1000 * 1000 * 1000 && tv->tv_nsec >= 0)
		timeout = tv->tv_sec * 1000000LL + tv->tv_nsec / 1000LL;
	else
		invalidTime = true;

	status_t status = mutex_lock(mutex, timeout);
	if (status != B_OK && invalidTime) {
		// The timespec was not valid and the mutex could not be locked
		// immediately.
		return EINVAL;
	}

	return status;
}


int
pthread_mutex_unlock(pthread_mutex_t* mutex)
{
	if (mutex->owner != find_thread(NULL))
		return EPERM;

	if (MUTEX_TYPE(mutex) == PTHREAD_MUTEX_RECURSIVE
		&& --mutex->owner_count > 0) {
		// still locked
		return 0;
	}

	mutex->owner = -1;

	// clear the locked flag
	int32 oldValue = atomic_and((int32*)&mutex->lock,
		~(int32)B_USER_MUTEX_LOCKED);
	if ((oldValue & B_USER_MUTEX_WAITING) != 0)
		_kern_mutex_unlock((int32*)&mutex->lock, 0);

	return 0;
}


int
pthread_mutex_getprioceiling(const pthread_mutex_t* mutex, int* _prioCeiling)
{
	if (mutex == NULL || _prioCeiling == NULL)
		return EINVAL;

	*_prioCeiling = 0;
		// not implemented

	return 0;
}


int
pthread_mutex_setprioceiling(pthread_mutex_t* mutex, int prioCeiling,
	int* _oldCeiling)
{
	if (mutex == NULL)
		return EINVAL;

	// not implemented
	return EPERM;
}
