/*
 * Copyright 2009, Michael Lotz, mmlr@mlotz.ch.
 * Copyright 2008-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2009, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <locks.h>

#include <OS.h>


// #pragma mark - mutex


status_t
mutex_init(mutex *lock, const char *name)
{
	if (lock == NULL || name == NULL)
		return B_BAD_VALUE;

	lock->benaphore = 0;
	lock->semaphore = create_sem(0, name);
	if (lock->semaphore < 0)
		return lock->semaphore;

	return B_OK;
}


void
mutex_destroy(mutex *lock)
{
	delete_sem(lock->semaphore);
}


status_t
mutex_lock(mutex *lock)
{
	if (atomic_add(&lock->benaphore, 1) == 0)
		return B_OK;

	status_t result;
	do {
		result = acquire_sem(lock->semaphore);
	} while (result == B_INTERRUPTED);

	return result;
}


void
mutex_unlock(mutex *lock)
{
	if (atomic_add(&lock->benaphore, -1) != 1)
		release_sem(lock->semaphore);
}


// #pragma mark - lazy mutex


enum {
	STATE_UNINITIALIZED	= -1,
	STATE_INITIALIZING	= -2,
	STATE_SPIN_LOCKED	= -3,
	STATE_SPIN_UNLOCKED	= -4
};


static inline bool
lazy_mutex_ensure_init(lazy_mutex *lock)
{
	int32 value = atomic_test_and_set((vint32*)&lock->semaphore,
		STATE_INITIALIZING, STATE_UNINITIALIZED);

	if (value >= 0)
		return true;

	if (value == STATE_UNINITIALIZED) {
		// we're the first -- perform the initialization
		sem_id semaphore = create_sem(0, lock->name);
		if (semaphore < 0)
			semaphore = STATE_SPIN_UNLOCKED;
		atomic_set((vint32*)&lock->semaphore, semaphore);
		return semaphore >= 0;
	}

	if (value == STATE_INITIALIZING) {
		// someone else is initializing -- spin until that is done
		while (atomic_get((vint32*)&lock->semaphore) == STATE_INITIALIZING) {
		}
	}

	return lock->semaphore >= 0;
}


status_t
lazy_mutex_init(lazy_mutex *lock, const char *name)
{
	if (lock == NULL || name == NULL)
		return B_BAD_VALUE;

	lock->benaphore = 0;
	lock->semaphore = STATE_UNINITIALIZED;
	lock->name = name;

	return B_OK;
}


void
lazy_mutex_destroy(lazy_mutex *lock)
{
	if (lock->semaphore >= 0)
		delete_sem(lock->semaphore);
}


status_t
lazy_mutex_lock(lazy_mutex *lock)
{
	if (atomic_add(&lock->benaphore, 1) == 0)
		return B_OK;

	if (lazy_mutex_ensure_init(lock)) {
		// acquire the semaphore
		status_t result;
		do {
			result = acquire_sem(lock->semaphore);
		} while (result == B_INTERRUPTED);

		return result;
	} else {
		// the semaphore creation failed -- so we use it like a spinlock instead
		while (atomic_test_and_set((vint32*)&lock->semaphore,
				STATE_SPIN_LOCKED, STATE_SPIN_UNLOCKED)
					!= STATE_SPIN_UNLOCKED) {
		}
		return B_OK;
	}
}


void
lazy_mutex_unlock(lazy_mutex *lock)
{
	if (atomic_add(&lock->benaphore, -1) == 1)
		return;

	if (lazy_mutex_ensure_init(lock)) {
		// release the semaphore
		release_sem(lock->semaphore);
	} else {
		// the semaphore creation failed -- so we use it like a spinlock instead
		atomic_set((vint32*)&lock->semaphore, STATE_SPIN_UNLOCKED);
	}
}
