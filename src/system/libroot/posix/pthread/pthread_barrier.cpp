/*
 * Copyright 2016, Dmytro Shynkevych, dm.shynk@gmail.com
 * Copyright 2023, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 */


#include <pthread.h>
#include "pthread_private.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <syscall_utils.h>
#include <syscalls.h>
#include <user_mutex_defs.h>


#define BARRIER_FLAG_SHARED	0x80000000


static const pthread_barrierattr pthread_barrierattr_default = {
	/* .process_shared = */ false
};


int
pthread_barrier_init(pthread_barrier_t* barrier,
	const pthread_barrierattr_t* _attr, unsigned count)
{
	const pthread_barrierattr* attr = _attr != NULL
		? *_attr : &pthread_barrierattr_default;

	if (barrier == NULL || attr == NULL || count < 1)
		return B_BAD_VALUE;

	barrier->flags = attr->process_shared ? BARRIER_FLAG_SHARED : 0;
	barrier->lock = B_USER_MUTEX_LOCKED;
	barrier->mutex = 0;
	barrier->waiter_count = 0;
	barrier->waiter_max = count;

	return B_OK;
}


static status_t
barrier_lock(__haiku_std_int32* mutex)
{
	const int32 oldValue = atomic_test_and_set((int32*)mutex, B_USER_MUTEX_LOCKED, 0);
	if (oldValue != 0) {
		status_t error;
		do {
			error = _kern_mutex_lock((int32*)mutex, NULL, 0, 0);
		} while (error == B_INTERRUPTED);

		if (error != B_OK)
			return error;
	}
	return B_OK;
}


static void
barrier_unlock(__haiku_std_int32* mutex)
{
	int32 oldValue = atomic_and((int32*)mutex,
		~(int32)B_USER_MUTEX_LOCKED);
	if ((oldValue & B_USER_MUTEX_WAITING) != 0)
		_kern_mutex_unblock((int32*)mutex, 0);
}


int
pthread_barrier_wait(pthread_barrier_t* barrier)
{
	if (barrier == NULL)
		return B_BAD_VALUE;

	if (barrier->waiter_max == 1)
		return PTHREAD_BARRIER_SERIAL_THREAD;

	// waiter_count < 0 means other threads are still exiting.
	// Lock in a loop, if necessary, until this is no longer the case.
	while (atomic_get((int32*)&barrier->waiter_count) < 0) {
		status_t status = barrier_lock(&barrier->mutex);
		if (status != B_OK)
			return status;

		barrier_unlock(&barrier->mutex);
	}

	if (atomic_add((int32*)&barrier->waiter_count, 1) == (barrier->waiter_max - 1)) {
		// We are the last one in. Lock the barrier mutex.
		barrier_lock(&barrier->mutex);

		// Wake everyone else up.
		barrier->waiter_count = (-barrier->waiter_max) + 1;
		atomic_and((int32*)&barrier->lock, ~(int32)B_USER_MUTEX_LOCKED);
		_kern_mutex_unblock((int32*)&barrier->lock, B_USER_MUTEX_UNBLOCK_ALL);

		// Return with the barrier mutex still locked, as waiter_count < 0.
		// The last thread out will take care of unlocking it and resetting state.
		return PTHREAD_BARRIER_SERIAL_THREAD;
	}

	// We aren't the last one in. Wait until we are woken up.
	do {
		_kern_mutex_lock((int32*)&barrier->lock, "barrier wait", 0, 0);
	} while (barrier->waiter_count > 0);

	// Release the barrier, so that any later threads trying to acquire it wake up.
	barrier_unlock(&barrier->lock);

	if (atomic_add((int32*)&barrier->waiter_count, 1) == -1) {
		// We are the last one out. Reset state and unlock.
		barrier->lock = B_USER_MUTEX_LOCKED;
		barrier_unlock(&barrier->mutex);
	}

	return 0;
}


int
pthread_barrier_destroy(pthread_barrier_t* barrier)
{
	// No dynamic resources to free
	return B_OK;
}


int
pthread_barrierattr_init(pthread_barrierattr_t* _attr)
{
	pthread_barrierattr* attr = (pthread_barrierattr*)malloc(
		sizeof(pthread_barrierattr));

	if (attr == NULL)
		return B_NO_MEMORY;

	*attr = pthread_barrierattr_default;
	*_attr = attr;

	return B_OK;
}


int
pthread_barrierattr_destroy(pthread_barrierattr_t* _attr)
{
	pthread_barrierattr* attr = _attr != NULL ? *_attr : NULL;

	if (attr == NULL)
		return B_BAD_VALUE;

	free(attr);

	return B_OK;
}


int
pthread_barrierattr_getpshared(const pthread_barrierattr_t* _attr, int* shared)
{
	pthread_barrierattr* attr;

	if (_attr == NULL || (attr = *_attr) == NULL || shared == NULL)
		return B_BAD_VALUE;

	*shared = attr->process_shared
		? PTHREAD_PROCESS_SHARED : PTHREAD_PROCESS_PRIVATE;

	return B_OK;
}


int
pthread_barrierattr_setpshared(pthread_barrierattr_t* _attr, int shared)
{
	pthread_barrierattr* attr;

	if (_attr == NULL || (attr = *_attr) == NULL
		|| shared < PTHREAD_PROCESS_PRIVATE
		|| shared > PTHREAD_PROCESS_SHARED) {
		return B_BAD_VALUE;
	}

	attr->process_shared = shared == PTHREAD_PROCESS_SHARED;

	return 0;
}
