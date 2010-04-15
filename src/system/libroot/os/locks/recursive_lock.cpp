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

#include <OS.h>


// #pragma mark - recursive lock


int32
recursive_lock_get_recursion(recursive_lock *lock)
{
	if (lock->holder == find_thread(NULL))
		return lock->recursion;

	return -1;
}


void
recursive_lock_init(recursive_lock *lock, const char *name)
{
	recursive_lock_init_etc(lock, name, 0);
}


void
recursive_lock_init_etc(recursive_lock *lock, const char *name, uint32 flags)
{
	lock->holder = -1;
	lock->recursion = 0;
	mutex_init_etc(&lock->lock, name, flags);
}


void
recursive_lock_destroy(recursive_lock *lock)
{
	if (lock == NULL)
		return;

	mutex_destroy(&lock->lock);
}


status_t
recursive_lock_lock(recursive_lock *lock)
{
	thread_id thread = find_thread(NULL);

	if (thread != lock->holder) {
		mutex_lock(&lock->lock);
		lock->holder = thread;
	}

	lock->recursion++;
	return B_OK;
}


void
recursive_lock_unlock(recursive_lock *lock)
{
	if (find_thread(NULL) != lock->holder) {
		debugger("recursive_lock unlocked by non-holder thread!\n");
		return;
	}

	if (--lock->recursion == 0) {
		lock->holder = -1;
		mutex_unlock(&lock->lock);
	}
}
