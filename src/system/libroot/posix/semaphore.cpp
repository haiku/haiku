/*
 * Copyright 2015, Hamish Morrison, hamishm53@gmail.com.
 * Copyright 2008-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <semaphore.h>

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>
#include <pthread.h>

#include <OS.h>

#include <AutoDeleter.h>
#include <errno_private.h>
#include <posix/realtime_sem_defs.h>
#include <syscall_utils.h>
#include <syscalls.h>
#include <user_mutex_defs.h>


#define SEM_TYPE_NAMED		1
#define SEM_TYPE_UNNAMED	2


static int32
atomic_add_if_greater(int32* value, int32 amount, int32 testValue)
{
	int32 current = atomic_get(value);
	while (current > testValue) {
		int32 old = atomic_test_and_set(value, current + amount, current);
		if (old == current)
			return old;
		current = old;
	}
	return current;
}


sem_t*
sem_open(const char* name, int openFlags,...)
{
	if (name == NULL) {
		__set_errno(B_BAD_VALUE);
		return SEM_FAILED;
	}

	// get the mode and semaphore count parameters, if O_CREAT is specified
	mode_t mode = 0;
	unsigned semCount = 0;

	if ((openFlags & O_CREAT) != 0) {
		va_list args;
		va_start(args, openFlags);
		mode = va_arg(args, mode_t);
		semCount = va_arg(args, unsigned);
		va_end(args);
	} else {
		// clear O_EXCL, if O_CREAT is not given
		openFlags &= ~O_EXCL;
	}

	// Allocate a sem_t structure -- we don't know, whether this is the first
	// call of this process to open the semaphore. If it is, we will keep the
	// structure, otherwise we will delete it later.
	sem_t* sem = (sem_t*)malloc(sizeof(sem_t));
	if (sem == NULL) {
		__set_errno(B_NO_MEMORY);
		return SEM_FAILED;
	}

	sem->type = SEM_TYPE_NAMED;
	MemoryDeleter semDeleter(sem);

	// ask the kernel to open the semaphore
	sem_t* usedSem;
	status_t error = _kern_realtime_sem_open(name, openFlags, mode, semCount,
		sem, &usedSem);
	if (error != B_OK) {
		__set_errno(error);
		return SEM_FAILED;
	}

	if (usedSem == sem)
		semDeleter.Detach();

	return usedSem;
}


int
sem_close(sem_t* semaphore)
{
	sem_t* deleteSem = NULL;
	status_t error = _kern_realtime_sem_close(semaphore->u.named_sem_id,
		&deleteSem);
	if (error == B_OK)
		free(deleteSem);

	RETURN_AND_SET_ERRNO(error);
}


int
sem_unlink(const char* name)
{
	RETURN_AND_SET_ERRNO(_kern_realtime_sem_unlink(name));
}


int
sem_init(sem_t* semaphore, int shared, unsigned value)
{
	semaphore->type = SEM_TYPE_UNNAMED;
	semaphore->u.unnamed_sem = value;
	return 0;
}


int
sem_destroy(sem_t* semaphore)
{
	if (semaphore->type != SEM_TYPE_UNNAMED)
		RETURN_AND_SET_ERRNO(EINVAL);

	return 0;
}


static int
unnamed_sem_post(sem_t* semaphore) {
	int32* sem = (int32*)&semaphore->u.unnamed_sem;
	int32 oldValue = atomic_add_if_greater(sem, 1, -1);
	if (oldValue > -1)
		return 0;

	return _kern_mutex_sem_release(sem);
}


static int
unnamed_sem_trywait(sem_t* semaphore) {
	int32* sem = (int32*)&semaphore->u.unnamed_sem;
	int32 oldValue = atomic_add_if_greater(sem, -1, 0);
	if (oldValue > 0)
		return 0;

	return EAGAIN;
}


static int
unnamed_sem_timedwait(sem_t* semaphore, const struct timespec* timeout) {
	int32* sem = (int32*)&semaphore->u.unnamed_sem;

	bigtime_t timeoutMicros = B_INFINITE_TIMEOUT;
	if (timeout != NULL) {
		timeoutMicros = ((bigtime_t)timeout->tv_sec) * 1000000
			+ timeout->tv_nsec / 1000;
	}

	int result = unnamed_sem_trywait(semaphore);
	if (result == 0)
		return 0;

	return _kern_mutex_sem_acquire(sem, NULL,
		timeoutMicros == B_INFINITE_TIMEOUT ? 0 : B_ABSOLUTE_REAL_TIME_TIMEOUT,
		timeoutMicros);
}


int
sem_post(sem_t* semaphore)
{
	status_t error;
	if (semaphore->type == SEM_TYPE_NAMED)
		error = _kern_realtime_sem_post(semaphore->u.named_sem_id);
	else
		error = unnamed_sem_post(semaphore);

	RETURN_AND_SET_ERRNO(error);
}


static int
named_sem_timedwait(sem_t* semaphore, const struct timespec* timeout)
{
	if (timeout != NULL
		&& (timeout->tv_nsec < 0 || timeout->tv_nsec >= 1000000000)) {
		status_t err = _kern_realtime_sem_wait(semaphore->u.named_sem_id, 0);
		if (err == B_WOULD_BLOCK)
			err = EINVAL;
		// do nothing, return err as it is.
		return err;
	}

	bigtime_t timeoutMicros = B_INFINITE_TIMEOUT;
	if (timeout != NULL) {
		timeoutMicros = ((bigtime_t)timeout->tv_sec) * 1000000
			+ timeout->tv_nsec / 1000;
	}
	status_t err = _kern_realtime_sem_wait(semaphore->u.named_sem_id,
		timeoutMicros);
	if (err == B_WOULD_BLOCK)
		err = ETIMEDOUT;

	return err;
}


int
sem_trywait(sem_t* semaphore)
{
	status_t error;
	if (semaphore->type == SEM_TYPE_NAMED)
		error = _kern_realtime_sem_wait(semaphore->u.named_sem_id, 0);
	else
		error = unnamed_sem_trywait(semaphore);

	RETURN_AND_SET_ERRNO(error);
}


int
sem_wait(sem_t* semaphore)
{
	status_t error;
	if (semaphore->type == SEM_TYPE_NAMED)
		error = named_sem_timedwait(semaphore, NULL);
	else
		error = unnamed_sem_timedwait(semaphore, NULL);

	RETURN_AND_SET_ERRNO_TEST_CANCEL(error);
}


int
sem_timedwait(sem_t* semaphore, const struct timespec* timeout)
{
	status_t error;
	if (semaphore->type == SEM_TYPE_NAMED)
		error = named_sem_timedwait(semaphore, timeout);
	else
		error = unnamed_sem_timedwait(semaphore, timeout);

	RETURN_AND_SET_ERRNO_TEST_CANCEL(error);
}


int
sem_getvalue(sem_t* semaphore, int* value)
{
	if (semaphore->type == SEM_TYPE_NAMED) {
		RETURN_AND_SET_ERRNO(_kern_realtime_sem_get_value(
			semaphore->u.named_sem_id, value));
	} else {
		*value = semaphore->u.unnamed_sem < 0 ? 0 : semaphore->u.unnamed_sem;
		return 0;
	}
}
