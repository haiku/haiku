/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef LOCK_H
#define LOCK_H


#include <OS.h>


typedef struct lock {
	sem_id	sem;
	vint32	count;
} lock;


static inline status_t
init_lock(struct lock *lock, const char *name)
{
	lock->sem = create_sem(0, name);
	lock->count = 0;

	return lock->sem < B_OK ? lock->sem : B_OK;
}


static inline void
uninit_lock(struct lock *lock)
{
	delete_sem(lock->sem);
}


static inline status_t
acquire_lock(struct lock *lock)
{
	if (atomic_add(&lock->count, 1) > 0)
		return acquire_sem(lock->sem);

	return B_OK;
}


static inline status_t
release_lock(struct lock *lock)
{
	if (atomic_add(&lock->count, -1) > 1)
		return release_sem(lock->sem);

	return B_OK;
}


class Autolock {
	public:
		Autolock(struct lock &lock)
			:
			fLock(&lock)
		{
			fStatus = acquire_lock(fLock);
		}

		~Autolock()
		{
			if (fStatus == B_OK)
				release_lock(fLock);
		}

		bool
		IsLocked() const
		{
			return fStatus == B_OK;
		}

	private:
		status_t	fStatus;
		struct lock	*fLock;
};


#endif	/* LOCK_H */