/*
 * Copyright 2009, Michael Lotz, mmlr@mlotz.ch.
 * Copyright 2008-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2009, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <locks.h>

#include <stdlib.h>
#include <string.h>

#include <syscalls.h>
#include <user_mutex_defs.h>


#define MAX_UNSUCCESSFUL_SPINS	100


extern int32 __gCPUCount;


// #pragma mark - mutex


void
__mutex_init(mutex *lock, const char *name)
{
	lock->name = name;
	lock->lock = 0;
	lock->flags = 0;
}


void
__mutex_init_etc(mutex *lock, const char *name, uint32 flags)
{
	lock->name = (flags & MUTEX_FLAG_CLONE_NAME) != 0 ? strdup(name) : name;
	lock->lock = 0;
	lock->flags = flags;

	if (__gCPUCount < 2)
		lock->flags &= ~uint32(MUTEX_FLAG_ADAPTIVE);
}


void
__mutex_destroy(mutex *lock)
{
	if ((lock->flags & MUTEX_FLAG_CLONE_NAME) != 0)
		free(const_cast<char*>(lock->name));
}


status_t
__mutex_lock(mutex *lock)
{
	uint32 count = 0;
	const uint32 kMaxCount
		= (lock->flags & MUTEX_FLAG_ADAPTIVE) != 0 ? MAX_UNSUCCESSFUL_SPINS : 1;

	int32 oldValue;
	do {
		// set the locked flag
		oldValue = atomic_or(&lock->lock, B_USER_MUTEX_LOCKED);

		if ((oldValue & (B_USER_MUTEX_LOCKED | B_USER_MUTEX_WAITING)) == 0
				|| (oldValue & B_USER_MUTEX_DISABLED) != 0) {
			// No one has the lock or is waiting for it, or the mutex has been
			// disabled.
			return B_OK;
		}
	} while (count++ < kMaxCount && (oldValue & B_USER_MUTEX_WAITING) != 0);

	// we have to call the kernel
	status_t error;
	do {
		error = _kern_mutex_lock(&lock->lock, lock->name, 0, 0);
	} while (error == B_INTERRUPTED);

	return error;
}


void
__mutex_unlock(mutex *lock)
{
	// clear the locked flag
	int32 oldValue = atomic_and(&lock->lock, ~(int32)B_USER_MUTEX_LOCKED);
	if ((oldValue & B_USER_MUTEX_WAITING) != 0
			&& (oldValue & B_USER_MUTEX_DISABLED) == 0) {
		_kern_mutex_unlock(&lock->lock, 0);
	}
}
