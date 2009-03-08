/*
 * Copyright 2002-2009, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Ingo Weinhold, bonefish@cs.tu-berlin.de.
 *		Axel Dörfler, axeld@pinc-software.de.
 */

#include <lock.h>


#define RW_MAX_READERS 10000


int32
recursive_lock_get_recursion(recursive_lock *lock)
{
	thread_id thread = find_thread(NULL);

#if !KDEBUG
	if (lock->holder == thread)
		return lock->recursion;
#else
	if (lock->lock.holder == thread)
		return lock->recursion;
#endif

	return -1;
}


void
recursive_lock_init_etc(recursive_lock *lock, const char *name, uint32 flags)
{
	if (lock == NULL)
		return;

	if (name == NULL)
		name = "recursive lock";

#if !KDEBUG
	lock->holder = -1;
#else
	lock->lock.holder = -1;
#endif
	lock->recursion = 0;
	lock->lock.waiters = (mutex_waiter*)create_sem(1, name);

	if ((sem_id)lock->lock.waiters < B_OK)
		panic("recursive lock creation failed: %s\n", name);
}


void
recursive_lock_init(recursive_lock *lock, const char *name)
{
	recursive_lock_init_etc(lock, name, 0);
}


void
recursive_lock_destroy(recursive_lock *lock)
{
	if (lock == NULL)
		return;

	delete_sem((sem_id)lock->lock.waiters);
	lock->lock.waiters = (mutex_waiter*)-1;
}


status_t
recursive_lock_lock(recursive_lock *lock)
{
	thread_id thread = find_thread(NULL);

#if !KDEBUG
	if (thread != lock->holder) {
		status_t status;
		do {
			status = acquire_sem((sem_id)lock->lock.waiters);
		} while (status == B_INTERRUPTED);
		if (status < B_OK)
			return status;

		lock->holder = thread;
	}
#else
	if (thread != lock->lock.holder) {
		status_t status;
		do {
			status = acquire_sem((sem_id)lock->lock.waiters);
		} while (status == B_INTERRUPTED);
		if (status < B_OK)
			return status;

		lock->lock.holder = thread;
	}
#endif
	lock->recursion++;
	return B_OK;
}


void
recursive_lock_unlock(recursive_lock *lock)
{
#if !KDEBUG
	if (find_thread(NULL) != lock->holder)
		panic("recursive_lock %p unlocked by non-holder thread!\n", lock);
#else
	if (find_thread(NULL) != lock->lock.holder)
		panic("recursive_lock %p unlocked by non-holder thread!\n", lock);
#endif

	if (--lock->recursion == 0) {
#if !KDEBUG
		lock->holder = -1;
#else
		lock->lock.holder = -1;
#endif
		release_sem_etc((sem_id)lock->lock.waiters, 1, 0);
	}
}


//	#pragma mark -


void
mutex_init(mutex *m, const char *name)
{
	if (m == NULL)
		return;

	if (name == NULL)
		name = "mutex_sem";

	// We need to store the semaphore in "waiters", as it is no sem anymore
	// Also, kernel mutex creation cannot fail anymore, but we could...
	m->waiters = (struct mutex_waiter *)create_sem(1, name);
	if ((sem_id)m->waiters < B_OK)
		debugger("semaphore creation failed");
}


void
mutex_init_etc(mutex *m, const char *name, uint32 flags)
{
	if (m == NULL)
		return;

	if (name == NULL)
		name = "mutex_sem";

	m->waiters = (struct mutex_waiter *)create_sem(1, name);
	if ((sem_id)m->waiters < B_OK)
		debugger("semaphore creation failed");
}


void
mutex_destroy(mutex *mutex)
{
	if (mutex == NULL)
		return;

	if ((sem_id)mutex->waiters >= 0) {
		delete_sem((sem_id)mutex->waiters);
		mutex->waiters = (struct mutex_waiter *)-1;
	}
}


status_t
_mutex_trylock(mutex *mutex)
{
	status_t status;
	do {
		status = acquire_sem_etc((sem_id)mutex->waiters, 1, B_RELATIVE_TIMEOUT,
			0);
	} while (status == B_INTERRUPTED);
	return status;
}


status_t
_mutex_lock(mutex *mutex, bool threadsLocked)
{
	if (mutex->waiters == NULL) {
		// MUTEX_INITIALIZER has been used; this is not thread-safe!
		mutex_init(mutex, mutex->name);
	}

	status_t status;
	do {
		status = acquire_sem((sem_id)mutex->waiters);
	} while (status == B_INTERRUPTED);

#if KDEBUG
	if (status == B_OK)
		mutex->holder = find_thread(NULL);
#endif
	return status;
}


void
_mutex_unlock(mutex *mutex, bool threadsLocked)
{
#if KDEBUG
	mutex->holder = -1;
#endif
	release_sem((sem_id)mutex->waiters);
}


//	#pragma mark -


void
rw_lock_init_etc(rw_lock *lock, const char *name, uint32 flags)
{
	if (lock == NULL)
		return;

	if (name == NULL)
		name = "r/w lock";

	lock->waiters = (rw_lock_waiter*)create_sem(RW_MAX_READERS, name);
	if ((sem_id)lock->waiters < B_OK)
		panic("r/w lock \"%s\" creation failed.", name);
}


void
rw_lock_init(rw_lock *lock, const char *name)
{
	rw_lock_init_etc(lock, name, 0);
}


void
rw_lock_destroy(rw_lock *lock)
{
	if (lock == NULL)
		return;

	delete_sem((sem_id)lock->waiters);
}


status_t
rw_lock_read_lock(rw_lock *lock)
{
	status_t status;
	do {
		status = acquire_sem((sem_id)lock->waiters);
	} while (status == B_INTERRUPTED);
	return status;
}


status_t
rw_lock_read_unlock(rw_lock *lock)
{
	return release_sem((sem_id)lock->waiters);
}


status_t
rw_lock_write_lock(rw_lock *lock)
{
	status_t status;
	do {
		status = acquire_sem_etc((sem_id)lock->waiters, RW_MAX_READERS, 0, 0);
	} while (status == B_INTERRUPTED);
	return status;
}


status_t
rw_lock_write_unlock(rw_lock *lock)
{
	return release_sem_etc((sem_id)lock->waiters, RW_MAX_READERS, 0);
}
