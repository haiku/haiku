/*
 * Copyright 2016, Dmytro Shynkevych, dm.shynk@gmail.com
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
	barrier->lock = 0;
	barrier->mutex = 0;
	barrier->waiter_count = 0;
	barrier->waiter_max = count;

	return B_OK;
}


int
pthread_barrier_wait(pthread_barrier_t* barrier)
{
	if (barrier == NULL)
		return B_BAD_VALUE;

	// Enter critical region: lock the mutex
	int32 status = atomic_or((int32*)&barrier->mutex, B_USER_MUTEX_LOCKED);

	// If already locked, call the kernel
	if (status & (B_USER_MUTEX_LOCKED | B_USER_MUTEX_WAITING)) {
		do {
			status = _kern_mutex_lock((int32*)&barrier->mutex, NULL, 0, 0);
		} while (status == B_INTERRUPTED);

		if (status != B_OK)
			return status;
	}

	barrier->waiter_count++;

	// If this thread is the last to arrive
	if (barrier->waiter_count == barrier->waiter_max) {
		// Let other threads exit the do...while loop
		barrier->waiter_count = 0;

		// Wake up everyone trying to acquire the barrier lock
		_kern_mutex_unlock((int32*)&barrier->lock, B_USER_MUTEX_UNBLOCK_ALL);

		// Exit critical region: unlock the mutex
		int32 status = atomic_and((int32*)&barrier->mutex,
				~(int32)B_USER_MUTEX_LOCKED);

		if (status & B_USER_MUTEX_WAITING)
			_kern_mutex_unlock((int32*)&barrier->mutex, 0);

		// Inform the calling thread that it arrived last
		return PTHREAD_BARRIER_SERIAL_THREAD;
	}

	do {
		// Wait indefinitely trying to acquire the barrier lock.
		// Other threads may now enter (mutex is unlocked).
		_kern_mutex_switch_lock((int32*)&barrier->mutex,
				(int32*)&barrier->lock, "barrier wait", 0, 0);
	} while (barrier->waiter_count != 0);

	// This thread did not arrive last
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
