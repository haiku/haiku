/* Mutex and recursive_lock code */

/* 
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/


#include <OS.h>
#include <lock.h>
#include <int.h>
#include <debug.h>
#include <thread.h>


int
recursive_lock_get_recursion(recursive_lock *lock)
{
	thread_id thid = thread_get_current_thread_id();

	if (lock->holder == thid)
		return lock->recursion;

	return -1;
}


status_t
recursive_lock_init(recursive_lock *lock, const char *name)
{
	if (lock == NULL)
		return B_BAD_VALUE;

	if (name == NULL)
		name = "recursive lock";

	lock->holder = -1;
	lock->recursion = 0;
	lock->sem = create_sem(1, name);

	if (lock->sem >= B_OK)
		return B_OK;

	return lock->sem;
}


void
recursive_lock_destroy(recursive_lock *lock)
{
	if (lock == NULL)
		return;

	delete_sem(lock->sem);
	lock->sem = -1;
}


bool
recursive_lock_lock(recursive_lock *lock)
{
	thread_id thid = thread_get_current_thread_id();
	bool retval = false;

	if (!kernel_startup && !are_interrupts_enabled())
		panic("recursive_lock_lock: called with interrupts disabled for lock %p, sem %#lx\n", lock, lock->sem);

	if (thid != lock->holder) {
		acquire_sem(lock->sem);
		
		lock->holder = thid;
		retval = true;
	}
	lock->recursion++;
	return retval;
}


bool
recursive_lock_unlock(recursive_lock *lock)
{
	thread_id thid = thread_get_current_thread_id();
	bool retval = false;

	if (thid != lock->holder)
		panic("recursive_lock %p unlocked by non-holder thread!\n", lock);

	if (--lock->recursion == 0) {
		lock->holder = -1;
		release_sem(lock->sem);
		retval = true;
	}
	return retval;
}


//	#pragma mark -


status_t
mutex_init(mutex *m, const char *name)
{
	if (m == NULL)
		return EINVAL;

	if (name == NULL)
		name = "mutex_sem";

	m->holder = -1;

	m->sem = create_sem(1, name);
	if (m->sem >= B_OK)
		return B_OK;

	return m->sem;
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


void
mutex_lock(mutex *mutex)
{
	thread_id me = thread_get_current_thread_id();

	if (!kernel_startup && !are_interrupts_enabled())
		panic("mutex_lock: called with interrupts disabled for mutex %p, sem %#lx\n", mutex, mutex->sem);

	if (me == mutex->holder)
		panic("mutex_lock failure: mutex %p (sem = 0x%lx) acquired twice by thread 0x%lx\n", mutex, mutex->sem, me);

	acquire_sem(mutex->sem);
	mutex->holder = me;
}


void
mutex_unlock(mutex *mutex)
{
	thread_id me = thread_get_current_thread_id();

	if (me != mutex->holder)
		panic("mutex_unlock failure: thread 0x%lx is trying to release mutex %p (current holder 0x%lx)\n",
			me, mutex, mutex->holder);

	mutex->holder = -1;
	release_sem(mutex->sem);
}


//	#pragma mark -


status_t
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


void
benaphore_destroy(benaphore *ben)
{
	delete_sem(ben->sem);
	ben->sem = -1;
}


//	#pragma mark -


status_t
rw_lock_init(rw_lock *lock, const char *name)
{
	if (lock == NULL)
		return B_BAD_VALUE;

	if (name == NULL)
		name = "r/w lock";

	lock->sem = create_sem(RW_MAX_READERS, name);
	if (lock->sem >= B_OK)
		return B_OK;

	return lock->sem;
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

