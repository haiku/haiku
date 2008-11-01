/*
 * Copyright 2003-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <pthread.h>
#include "pthread_private.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#define MUTEX_FLAG_SHARED	0x80000000
#define MUTEX_TYPE_BITS		0x0000000f
#define MUTEX_TYPE(mutex)	((mutex)->flags & MUTEX_TYPE_BITS)


static const pthread_mutexattr pthread_mutexattr_default = {
	PTHREAD_MUTEX_DEFAULT,
	false
};


int
pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *_attr)
{
	const pthread_mutexattr *attr = NULL;

	if (mutex == NULL)
		return B_BAD_VALUE;

	if (_attr != NULL)
		attr = *_attr;
	else
		attr = &pthread_mutexattr_default;

	mutex->sem = create_sem(0, "pthread_mutex");
	if (mutex->sem < B_OK) {
		return B_WOULD_BLOCK;
			// stupid error code (EAGAIN) but demanded by POSIX
	}

	mutex->count = 0;
	mutex->owner = -1;
	mutex->owner_count = 0;
	mutex->flags = attr->type | (attr->process_shared ? MUTEX_FLAG_SHARED : 0);

	return B_OK;
}


int
pthread_mutex_destroy(pthread_mutex_t *mutex)
{
	if (mutex == NULL)
		return B_BAD_VALUE;

	delete_sem(mutex->sem);

	return B_OK;
}


static status_t
mutex_lock(pthread_mutex_t *mutex, bigtime_t timeout)
{
	thread_id thisThread = find_thread(NULL);
	status_t status = B_OK;

	if (mutex == NULL)
		return B_BAD_VALUE;

	// If statically initialized, we need to create the semaphore, now.
	if (mutex->sem == -42) {
		sem_id sem = create_sem(0, "pthread_mutex");
		if (sem < 0)
			return EAGAIN;

		if (atomic_test_and_set((vint32*)&mutex->sem, sem, -42) != -42)
			delete_sem(sem);
	}

	if (MUTEX_TYPE(mutex) == PTHREAD_MUTEX_ERRORCHECK
		&& mutex->owner == thisThread) {
		// we detect this kind of deadlock and return an error
		return EDEADLK;
	}

	if (MUTEX_TYPE(mutex) == PTHREAD_MUTEX_RECURSIVE
		&& mutex->owner == thisThread) {
		// if we already hold the mutex, we don't need to grab it again
		mutex->owner_count++;
		return B_OK;
	}

	if (atomic_add((vint32*)&mutex->count, 1) > 0) {
		// this mutex is already locked by someone else, so we need
		// to wait
		status = acquire_sem_etc(mutex->sem, 1,
			timeout == B_INFINITE_TIMEOUT ? 0 : B_ABSOLUTE_REAL_TIME_TIMEOUT,
			timeout);
	}

	if (status == B_OK) {
		// we have locked the mutex for the first time
		mutex->owner = thisThread;
		mutex->owner_count = 1;
	}

	return status;
}


int
pthread_mutex_lock(pthread_mutex_t *mutex)
{
	return mutex_lock(mutex, B_INFINITE_TIMEOUT);
}


int
pthread_mutex_trylock(pthread_mutex_t *mutex)
{
	status_t status = mutex_lock(mutex, 0);
	return status == B_WOULD_BLOCK ? EBUSY : status;
}


int
pthread_mutex_timedlock(pthread_mutex_t *mutex, const struct timespec *tv)
{
	bool invalidTime = false;
	status_t status;

	bigtime_t timeout = 0;
	if (tv && tv->tv_nsec < 1000*1000*1000 && tv->tv_nsec >= 0)
		timeout = tv->tv_sec * 1000000LL + tv->tv_nsec / 1000LL;
	else
		invalidTime = true;

	status = mutex_lock(mutex, timeout);
	if (status != B_OK && invalidTime) {
		// POSIX requires EINVAL (= B_BAD_VALUE) to be returned
		// if the timespec structure was invalid
		return B_BAD_VALUE;
	}

	return status;
}


int
pthread_mutex_unlock(pthread_mutex_t *mutex)
{
	if (mutex == NULL)
		return B_BAD_VALUE;

	if (mutex->owner != find_thread(NULL))
		return EPERM;

	if (MUTEX_TYPE(mutex) == PTHREAD_MUTEX_RECURSIVE
		&& mutex->owner_count-- > 1) {
		return B_OK;
	}

	mutex->owner = -1;

	if (atomic_add((vint32*)&mutex->count, -1) > 1)
		return release_sem(mutex->sem);

	return B_OK;
}


int
pthread_mutex_getprioceiling(pthread_mutex_t *mutex, int *_prioCeiling)
{
	if (mutex == NULL || _prioCeiling == NULL)
		return B_BAD_VALUE;

	*_prioCeiling = 0;
		// not implemented

	return B_OK;
}


int
pthread_mutex_setprioceiling(pthread_mutex_t *mutex, int prioCeiling,
	int *_oldCeiling)
{
	if (mutex == NULL)
		return B_BAD_VALUE;

	// not implemented
	return B_NOT_ALLOWED;
}
