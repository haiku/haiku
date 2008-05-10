/*
 * Copyright 2007, Ryan Leavengood, leavengood@gmail.com.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <pthread.h>
#include "pthread_private.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


static const pthread_condattr pthread_condattr_default = {
	false
};


pthread_cond_t 
_pthread_cond_static_initializer(void)
{
	pthread_cond_t cond;
	if (pthread_cond_init(&cond, NULL) == B_OK)
		return cond;

	return NULL;
}


int
pthread_cond_init(pthread_cond_t *_cond, const pthread_condattr_t *_attr)
{
	pthread_cond *cond;
	const pthread_condattr *attr = NULL;

	if (_cond == NULL)
		return B_BAD_VALUE;

	cond = (pthread_cond *)malloc(sizeof(pthread_cond));
	if (cond == NULL)
		return B_NO_MEMORY;

	if (_attr != NULL)
		attr = *_attr;
	else
		attr = &pthread_condattr_default;

	// TODO: What about the process_shared attribute?
	cond->sem = create_sem(0, "pthread_cond");
	if (cond->sem < B_OK) {
		free(cond);
		return B_WOULD_BLOCK;
			// stupid error code (EAGAIN) but demanded by POSIX
	}

	cond->mutex = NULL;
	cond->waiter_count = 0;
	cond->event_counter = 0;
	memcpy(&cond->attr, attr, sizeof(pthread_condattr));	

	*_cond = cond;
	return B_OK;
}


int
pthread_cond_destroy(pthread_cond_t *_cond)
{
	pthread_cond *cond;

	if (_cond == NULL || (cond = *_cond) == NULL)
		return B_BAD_VALUE;

	delete_sem(cond->sem);
	*_cond = NULL;
	free(cond);

	return B_OK;
}


static status_t
cond_wait(pthread_cond *cond, pthread_mutex_t *_mutex, bigtime_t timeout)
{
	status_t status;
	int32 event;

	if (cond == NULL || *_mutex == NULL)
		return B_BAD_VALUE;

	if ((*_mutex)->owner != find_thread(NULL))
		// POSIX suggests EPERM (= B_NOT_ALLOWED) to be returned
		// if this thread does not own the mutex
		return B_NOT_ALLOWED;

	if (cond->mutex && cond->mutex != _mutex)
		// POSIX suggests EINVAL (= B_BAD_VALUE) to be returned if
		// the same condition variable is used with multiple mutexes
		return B_BAD_VALUE;

	cond->mutex = _mutex;
	cond->waiter_count++;

	event = atomic_get(&cond->event_counter);

	pthread_mutex_unlock(_mutex);

	do {
		status = acquire_sem_etc(cond->sem, 1,
			timeout == B_INFINITE_TIMEOUT ? 0 : B_ABSOLUTE_REAL_TIME_TIMEOUT,
			timeout);
	} while (status == B_OK && atomic_get(&cond->event_counter) == event);

	pthread_mutex_lock(_mutex);

	cond->waiter_count--;
	// If there are no more waiters, we can change mutexes
	if (cond->waiter_count == 0)
		cond->mutex = NULL;

	return status;
}


static status_t
cond_signal(pthread_cond *cond, bool broadcast)
{
	if (cond == NULL)
		return B_BAD_VALUE;

	atomic_add(&cond->event_counter, 1);
	return release_sem_etc(cond->sem, broadcast ? cond->waiter_count : 1, 0);
}


int
pthread_cond_wait(pthread_cond_t *_cond, pthread_mutex_t *_mutex)
{
	if (_cond == NULL || _mutex == NULL)
		return B_BAD_VALUE;

	if (*_cond == NULL)
		pthread_cond_init(_cond, NULL);
	
	return cond_wait(*_cond, _mutex, B_INFINITE_TIMEOUT);
}


int
pthread_cond_timedwait(pthread_cond_t *_cond, pthread_mutex_t *_mutex,
	const struct timespec *tv)
{
	bool invalidTime = false;
	status_t status;

	bigtime_t timeout = 0;
	if (tv && tv->tv_nsec < 1000*1000*1000 && tv->tv_nsec >= 0)
		timeout = tv->tv_sec * 1000000LL + tv->tv_nsec / 1000LL;
	else
		invalidTime = true;

	if (_cond == NULL || _mutex == NULL)
		return B_BAD_VALUE;

	if (*_cond == NULL)
		pthread_cond_init(_cond, NULL);
	
	status = cond_wait(*_cond, _mutex, timeout);
	if (status != B_OK && invalidTime) {
		// POSIX requires EINVAL (= B_BAD_VALUE) to be returned
		// if the timespec structure was invalid
		return B_BAD_VALUE;
	}

	return status;
}


int
pthread_cond_broadcast(pthread_cond_t *_cond)
{
	if (_cond == NULL)
		return B_BAD_VALUE;

	return cond_signal(*_cond, true);
}


int
pthread_cond_signal(pthread_cond_t *_cond)
{
	if (_cond == NULL)
		return B_BAD_VALUE;

	return cond_signal(*_cond, false);
}

