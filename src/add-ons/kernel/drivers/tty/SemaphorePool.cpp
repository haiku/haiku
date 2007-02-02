/*
 * Copyright 2005-2007, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */

#include <new>

#include <util/AutoLock.h>

#include "SemaphorePool.h"


status_t
Semaphore::ZeroCount()
{
	int32 count;
	status_t error = get_sem_count(fSemaphore, &count);
	if (error != B_OK)
		return error;

	if (count > 0)
		error = acquire_sem_etc(fSemaphore, count, B_RELATIVE_TIMEOUT, 0);
	else if (count < 0)
		error = release_sem_etc(fSemaphore, -count, 0);

	return error;
}


// #pragma mark -

SemaphorePool::SemaphorePool(int32 maxCached)
	: fSemaphores(),
	  fCount(0),
	  fMaxCount(maxCached)
{
	if (fMaxCount < 0)
		fMaxCount = 8;
}


SemaphorePool::~SemaphorePool()
{
	while (Semaphore *sem = fSemaphores.Head()) {
		fSemaphores.Remove(sem);
		delete sem;
	}

	mutex_destroy(&fLock);
}


status_t
SemaphorePool::Init()
{
	return mutex_init(&fLock, "tty semaphore pool");
}


status_t
SemaphorePool::Get(Semaphore *&semaphore)
{
	MutexLocker _(fLock);

	// use a cached one, if available
	if (fCount > 0) {
		fCount--;
		semaphore = fSemaphores.Head();
		fSemaphores.Remove(semaphore);
		return B_OK;
	}

	// create a new one
	Semaphore *sem = new(nothrow) Semaphore(0, "pool semaphore");
	if (!sem)
		return B_NO_MEMORY;

	semaphore = sem;
	return B_OK;
}


void
SemaphorePool::Put(Semaphore *semaphore)
{
	if (!semaphore)
		return;

	MutexLocker _(fLock);

	if (fCount >= fMaxCount
		|| semaphore->InitCheck() != B_OK
		|| semaphore->ZeroCount() != B_OK) {
		delete semaphore;
		return;
	}

	fSemaphores.Add(semaphore);
	fCount++;
}
