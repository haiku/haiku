/*
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

/* Mutex and recursive_lock code */

#include "haiku_lock.h"

#include <OS.h>

#include "kernel_emu.h"

using UserlandFS::KernelEmu::panic;

namespace UserlandFS {
namespace HaikuKernelEmu {


sem_id
_init_semaphore(int32 count, const char* name)
{
	sem_id sem = create_sem(count, name);
	if (sem < 0)
		panic("_init_semaphore(): Failed to create semaphore!\n");
	return sem;
}


void
recursive_lock_init(recursive_lock *lock, const char *name)
{
	recursive_lock_init_etc(lock, name, 0);
}


void
recursive_lock_init_etc(recursive_lock *lock, const char *name, uint32 flags)
{
	if (lock == NULL)
		panic("recursive_lock_init_etc(): NULL lock\n");

	if (name == NULL)
		name = "recursive lock";

	lock->holder = -1;
	lock->recursion = 0;
	lock->sem = _init_semaphore(1, name);
}


void
recursive_lock_destroy(recursive_lock *lock)
{
	if (lock == NULL)
		return;

	delete_sem(lock->sem);
	lock->sem = -1;
}


status_t
recursive_lock_lock(recursive_lock *lock)
{
	thread_id thread = find_thread(NULL);

	if (thread != lock->holder) {
		status_t status = acquire_sem(lock->sem);
		if (status < B_OK)
			return status;

		lock->holder = thread;
	}
	lock->recursion++;
	return B_OK;
}


status_t
recursive_lock_trylock(recursive_lock *lock)
{
	thread_id thread = find_thread(NULL);

	if (thread != lock->holder) {
		status_t status = acquire_sem_etc(lock->sem, 1, B_RELATIVE_TIMEOUT, 0);
		if (status < B_OK)
			return status;

		lock->holder = thread;
	}
	lock->recursion++;
	return B_OK;
}


void
recursive_lock_unlock(recursive_lock *lock)
{
	if (find_thread(NULL) != lock->holder)
		panic("recursive_lock %p unlocked by non-holder thread!\n", lock);

	if (--lock->recursion == 0) {
		lock->holder = -1;
		release_sem(lock->sem);
	}
}


int32
recursive_lock_get_recursion(recursive_lock *lock)
{
	if (lock->holder == find_thread(NULL))
		return lock->recursion;

	return -1;
}


//	#pragma mark -


void
mutex_init(mutex *lock, const char *name)
{
	mutex_init_etc(lock, name, 0);
}


void
mutex_init_etc(mutex* lock, const char* name, uint32 flags)
{
	if (lock == NULL)
		panic("mutex_init_etc(): NULL lock\n");

	if (name == NULL)
		name = "mutex_sem";

	lock->holder = -1;

	lock->sem = _init_semaphore(1, name);
}


void
mutex_destroy(mutex *mutex)
{
	if (mutex == NULL)
		return;

	if (mutex->sem >= 0) {
		delete_sem(mutex->sem);
		mutex->sem = -1;
	}
	mutex->holder = -1;
}


status_t
mutex_lock(mutex *mutex)
{
	thread_id me = find_thread(NULL);
	status_t status;

	status = acquire_sem(mutex->sem);
	if (status < B_OK)
		return status;

	if (me == mutex->holder)
		panic("mutex_lock failure: mutex %p (sem = 0x%lx) acquired twice by thread 0x%lx\n", mutex, mutex->sem, me);

	mutex->holder = me;
	return B_OK;
}


status_t
mutex_trylock(mutex *mutex)
{
	thread_id me = find_thread(NULL);
	status_t status;

	status = acquire_sem_etc(mutex->sem, 1, B_RELATIVE_TIMEOUT, 0);
	if (status < B_OK)
		return status;

	if (me == mutex->holder)
		panic("mutex_lock failure: mutex %p (sem = 0x%lx) acquired twice by thread 0x%lx\n", mutex, mutex->sem, me);

	mutex->holder = me;
	return B_OK;
}


void
mutex_unlock(mutex *mutex)
{
	thread_id me = find_thread(NULL);

	if (me != mutex->holder) {
		panic("mutex_unlock failure: thread 0x%lx is trying to release mutex %p (current holder 0x%lx)\n",
			me, mutex, mutex->holder);
	}

	mutex->holder = -1;
	release_sem(mutex->sem);
}


//	#pragma mark -


void
rw_lock_init(rw_lock *lock, const char *name)
{
	rw_lock_init_etc(lock, name, 0);
}


void
rw_lock_init_etc(rw_lock* lock, const char* name, uint32 flags)
{
	if (lock == NULL)
		panic("rw_lock_init_etc(): NULL lock\n");

	if (name == NULL)
		name = "r/w lock";

	lock->sem = _init_semaphore(RW_MAX_READERS, name);
}


void
rw_lock_destroy(rw_lock *lock)
{
	if (lock == NULL)
		return;

	delete_sem(lock->sem);
}


status_t
rw_lock_read_lock(rw_lock *lock)
{
	return acquire_sem(lock->sem);
}


status_t
rw_lock_read_unlock(rw_lock *lock)
{
	return release_sem(lock->sem);
}


status_t
rw_lock_write_lock(rw_lock *lock)
{
	return acquire_sem_etc(lock->sem, RW_MAX_READERS, 0, 0);
}


status_t
rw_lock_write_unlock(rw_lock *lock)
{
	return release_sem_etc(lock->sem, RW_MAX_READERS, 0);
}

}	// namespace HaikuKernelEmu
}	// namespace UserlandFS
