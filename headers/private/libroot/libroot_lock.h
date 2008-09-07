/*
 * Copyright 2002-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _LIBROOT_LOCK_H
#define _LIBROOT_LOCK_H

#include <OS.h>


// TODO: Copied from the kernel private lock.h/lock.c. We should somehow make
// that code reusable.


namespace BPrivate {


typedef struct benaphore {
	sem_id	sem;
	int32	count;
} benaphore;


static inline status_t
benaphore_init(benaphore *ben, const char *name)
{
	if (ben == NULL || name == NULL)
		return B_BAD_VALUE;

	ben->count = 1;
	ben->sem = create_sem(0, name);
	if (ben->sem >= B_OK)
		return B_OK;

	return ben->sem;
}


static inline status_t
benaphore_lock_etc(benaphore *ben, uint32 flags, bigtime_t timeout)
{
// TODO: This function really shouldn't be used, since timeouts screw the
// benaphore behavior.
	if (atomic_add(&ben->count, -1) <= 0)
		return acquire_sem_etc(ben->sem, 1, flags, timeout);

	return B_OK;
}


static inline status_t
benaphore_lock(benaphore *ben)
{
	if (atomic_add(&ben->count, -1) <= 0) {
		status_t error;
		do {
			error = acquire_sem(ben->sem);
		} while (error == B_INTERRUPTED);

		return error;
	}

	return B_OK;
}


static inline status_t
benaphore_unlock(benaphore *ben)
{
	if (atomic_add(&ben->count, 1) < 0)
		return release_sem(ben->sem);

	return B_OK;
}


}	// namespace BPrivate


using BPrivate::benaphore;
using BPrivate::benaphore_init;
using BPrivate::benaphore_lock_etc;
using BPrivate::benaphore_lock;
using BPrivate::benaphore_unlock;


#endif	// _LIBROOT_LOCK_H
