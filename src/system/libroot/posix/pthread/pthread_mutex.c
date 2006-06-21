/*
 * Copyright 2003-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <pthread.h>
#include "pthread_private.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


static const pthread_mutexattr pthread_mutexattr_default = {
	PTHREAD_MUTEX_DEFAULT,
	false
};


pthread_mutex_t
_pthread_recursive_mutex_static_initializer(void)
{
	pthread_mutex_t mutex;
	pthread_mutexattr attr;
	pthread_mutexattr_t attrPointer = &attr;

	attr.type = PTHREAD_MUTEX_RECURSIVE;
	attr.process_shared = false;

	if (pthread_mutex_init(&mutex, &attrPointer) == B_OK)
		return mutex;

	return NULL;
}


pthread_mutex_t 
_pthread_mutex_static_initializer(void)
{
	pthread_mutex_t mutex;
	if (pthread_mutex_init(&mutex, NULL) == B_OK)
		return mutex;

	return NULL;
}


int
pthread_mutex_init(pthread_mutex_t *_mutex, const pthread_mutexattr_t *_attr)
{
	pthread_mutex *mutex;
	const pthread_mutexattr *attr = NULL;

	if (_mutex == NULL)
		return B_BAD_VALUE;

	mutex = (pthread_mutex *)malloc(sizeof(pthread_mutex));
	if (mutex == NULL)
		return B_NO_MEMORY;

	if (_attr != NULL)
		attr = *_attr;
	else
		attr = &pthread_mutexattr_default;

	mutex->sem = create_sem(attr && attr->process_shared ? 0 : 1, "pthread_mutex");
	if (mutex->sem < B_OK) {
		free(mutex);
		return B_WOULD_BLOCK;
			// stupid error code (EAGAIN) but demanded by POSIX
	}

	mutex->count = 0;
	mutex->owner = -1;
	mutex->owner_count = 0;
	memcpy(&mutex->attr, attr, sizeof(pthread_mutexattr));	

	*_mutex = mutex;
	return B_OK;
}


int
pthread_mutex_destroy(pthread_mutex_t *_mutex)
{
	pthread_mutex *mutex;

	if (_mutex == NULL || (mutex = *_mutex) == NULL)
		return B_BAD_VALUE;

	delete_sem(mutex->sem);
	*_mutex = NULL;
	free(mutex);

	return B_OK;
}


static status_t
mutex_unlock(pthread_mutex *mutex)
{
	if (mutex == NULL)
		return B_BAD_VALUE;

	if (mutex->owner != find_thread(NULL)) {
		// this is a bug in the calling application!
		// ToDo: should we handle it in another way?
		fprintf(stderr, "mutex unlocked from foreign thread!\n");
	}

	if (mutex->attr.type == PTHREAD_MUTEX_RECURSIVE) {

		if (mutex->owner_count-- > 1)
			return B_OK;

		mutex->owner = -1;
	}

	if (!mutex->attr.process_shared || atomic_add(&mutex->count, -1) > 1)
		return release_sem(mutex->sem);

	return B_OK;
}


static status_t
mutex_lock(pthread_mutex *mutex, bigtime_t timeout)
{
	thread_id thisThread = find_thread(NULL);
	status_t status = B_OK;

	if (mutex == NULL)
		return B_BAD_VALUE;

	if (mutex->attr.type == PTHREAD_MUTEX_ERRORCHECK && mutex->owner == thisThread) {
		// we detect this kind of deadlock and return an error
		return B_BUSY;
	}

	if (mutex->attr.type == PTHREAD_MUTEX_RECURSIVE && mutex->owner == thisThread) {
		// if we already hold the mutex, we don't need to grab it again
		mutex->owner_count++;
		return B_OK;
	}

	if (!mutex->attr.process_shared || atomic_add(&mutex->count, 1) > 0) {
		// this mutex is already locked by someone else, so we need
		// to wait
		status = acquire_sem_etc(mutex->sem, 1, timeout == B_INFINITE_TIMEOUT ? 0 : B_ABSOLUTE_TIMEOUT, timeout);
	}

	if (status == B_OK) {
		// we have locked the mutex for the first time
		mutex->owner = thisThread;
		mutex->owner_count = 1;
	}

	return status;
}


int
pthread_mutex_lock(pthread_mutex_t *_mutex)
{
	if (_mutex == NULL)
		return B_BAD_VALUE;

	if (*_mutex == NULL)
		pthread_mutex_init(_mutex, NULL);

	return mutex_lock(*_mutex, B_INFINITE_TIMEOUT);
}


int
pthread_mutex_trylock(pthread_mutex_t *_mutex)
{
	if (_mutex == NULL)
		return B_BAD_VALUE;

	return mutex_lock(*_mutex, 0);
}


int 
pthread_mutex_timedlock(pthread_mutex_t *_mutex, const struct timespec *tv)
{
	bool invalidTime = false;
	status_t status;

	bigtime_t timeout = 0;
	if (tv && tv->tv_nsec < 1000*1000*1000 && tv->tv_nsec >= 0)
		timeout = tv->tv_sec * 1000000LL + tv->tv_nsec / 1000LL;
	else
		invalidTime = true;

	if (_mutex == NULL)
		return B_BAD_VALUE;

	status = mutex_lock(*_mutex, timeout);
	if (status != B_OK && invalidTime) {
		// POSIX requires EINVAL (= B_BAD_VALUE) to be returned
		// if the timespec structure was invalid
		return B_BAD_VALUE;
	}

	return status;
}


int
pthread_mutex_unlock(pthread_mutex_t *_mutex)
{
	if (_mutex == NULL)
		return B_BAD_VALUE;

	return mutex_unlock(*_mutex);
}


int 
pthread_mutex_getprioceiling(pthread_mutex_t *_mutex, int *_prioCeiling)
{
	pthread_mutex *mutex;

	if (_mutex == NULL || (mutex = *_mutex) == NULL || _prioCeiling == NULL)
		return B_BAD_VALUE;

	*_prioCeiling = 0;
		// not implemented

	return B_OK;
}


int 
pthread_mutex_setprioceiling(pthread_mutex_t *_mutex, int prioCeiling, int *_oldCeiling)
{
	pthread_mutex *mutex;

	if (_mutex == NULL || (mutex = *_mutex) == NULL)
		return B_BAD_VALUE;

	// not implemented
	return B_NOT_ALLOWED;
}


