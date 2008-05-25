/*
	Driver for USB Human Interface Devices.
	Copyright (C) 2008 Michael Lotz <mmlr@mlotz.ch>
	Distributed under the terms of the MIT license.
*/
#ifndef HAIKU_TARGET_PLATFORM_HAIKU
#ifndef _BEOS_COMPATIBILITY_H_
#define _BEOS_COMPATIBILITY_H_

#include <OS.h>

typedef struct mutex {
	sem_id	sem;
	int32	count;
} mutex;


static inline void
mutex_init(mutex *lock, const char *name)
{
	lock->sem = create_sem(0, name);
	lock->count = 0;
}


static inline void
mutex_destroy(mutex *lock)
{
	delete_sem(lock->sem);
}


static inline status_t
mutex_lock(mutex *lock)
{
	if (atomic_add(&lock->count, -1) < 0)
		return acquire_sem(lock->sem);
	return B_OK;
}


static inline void
mutex_unlock(mutex *lock)
{
	if (atomic_add(&lock->count, 1) < -1)
		release_sem(lock->sem);
}

#endif /* !HAIKU_TARGET_PLATFORM_HAIKU */
#endif /* _BEOS_COMPATIBILITY_H_ */
