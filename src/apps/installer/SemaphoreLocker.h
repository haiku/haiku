/*
 * Copyright 2008-2009, Stephan Aßmus <superstippi@gmx.de>
 *  All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _SEMAPHORE_LOCKER_H
#define _SEMAPHORE_LOCKER_H


#include "AutoLocker.h"


class SemaphoreLocking {
public:
	inline bool Lock(sem_id* lockable)
	{
		return acquire_sem(*lockable) == B_OK;
	}

	inline void Unlock(sem_id* lockable)
	{
		release_sem(*lockable);
	}
};


class SemaphoreLocker : public AutoLocker<sem_id, SemaphoreLocking> {
public:
	inline SemaphoreLocker(sem_id semaphore, bool alreadyLocked = false,
			bool lockIfNotLocked = true)
		:
		AutoLocker<sem_id, SemaphoreLocking>(),
		fSem(semaphore)
	{
		SetTo(&fSem, alreadyLocked, lockIfNotLocked);
	}

private:
	sem_id	fSem;
};

#endif // _SEMAPHORE_LOCKER_H
