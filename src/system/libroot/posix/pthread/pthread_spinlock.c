/*
 * Copyright 2010, Lucian Adrian Grijincu, lucian.grijincu@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include <pthread.h>

#include <arch_cpu_defs.h>

#include "pthread_private.h"


#define UNLOCKED	0
#define LOCKED		1


int
pthread_spin_init(pthread_spinlock_t* lock, int pshared)
{
	// This implementation of spinlocks doesn't differentiate
	// between spin locks used by threads in the same process or
	// between threads of different processes.

	lock->lock = UNLOCKED;
	return 0;
}


int
pthread_spin_destroy(pthread_spinlock_t* lock)
{
	return 0;
}


int
pthread_spin_lock(pthread_spinlock_t* lock)
{
	while (atomic_test_and_set((int32*)&lock->lock, LOCKED, UNLOCKED)
			== LOCKED) {
		SPINLOCK_PAUSE(); // spin
			// TODO: On UP machines we should thread_yield() in the loop.
	}
	return 0;
}


int
pthread_spin_trylock(pthread_spinlock_t* lock)
{
	if (atomic_test_and_set((int32*)&lock->lock, LOCKED, UNLOCKED) == LOCKED)
		return EBUSY;
	return 0;
}


int
pthread_spin_unlock(pthread_spinlock_t* lock)
{
	lock->lock = UNLOCKED;
	return 0;
}
