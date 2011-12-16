/*
 * Copyright 2008-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <semaphore.h>

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>

#include <OS.h>

#include <AutoDeleter.h>
#include <errno_private.h>
#include <posix/realtime_sem_defs.h>
#include <syscall_utils.h>
#include <syscalls.h>


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
	status_t error = _kern_realtime_sem_close(semaphore->id, &deleteSem);
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
	RETURN_AND_SET_ERRNO(_kern_realtime_sem_open(NULL, shared, 0, value,
		semaphore, NULL));
}


int
sem_destroy(sem_t* semaphore)
{
	RETURN_AND_SET_ERRNO(_kern_realtime_sem_close(semaphore->id, NULL));
}


int
sem_post(sem_t* semaphore)
{
	RETURN_AND_SET_ERRNO(_kern_realtime_sem_post(semaphore->id));
}


int
sem_timedwait(sem_t* semaphore, const struct timespec* timeout)
{
	bigtime_t timeoutMicros = ((bigtime_t)timeout->tv_sec) * 1000000
		+ timeout->tv_nsec / 1000;

	RETURN_AND_SET_ERRNO_TEST_CANCEL(
		_kern_realtime_sem_wait(semaphore->id, timeoutMicros));
}


int
sem_trywait(sem_t* semaphore)
{
	RETURN_AND_SET_ERRNO(_kern_realtime_sem_wait(semaphore->id, 0));
}


int
sem_wait(sem_t* semaphore)
{
	RETURN_AND_SET_ERRNO_TEST_CANCEL(
		_kern_realtime_sem_wait(semaphore->id, B_INFINITE_TIMEOUT));
}


int
sem_getvalue(sem_t* semaphore, int* value)
{
	RETURN_AND_SET_ERRNO(_kern_realtime_sem_get_value(semaphore->id, value));
}
