/* Mutex and recursive_lock code */

/* 
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <kernel.h>
#include <OS.h>
#include <lock.h>
#include <debug.h>
#include <arch/cpu.h>
#include <Errors.h>
#include <atomic.h>
#include <thread.h>


int
recursive_lock_get_recursion(recursive_lock *lock)
{
	thread_id thid = thread_get_current_thread_id();

	if (lock->holder == thid)
		return lock->recursion;

	return -1;
}


int
recursive_lock_create(recursive_lock *lock)
{
	if (lock == NULL)
		return EINVAL;

	lock->holder = -1;
	lock->recursion = 0;
	lock->sem = create_sem(1, "recursive_lock_sem");

	return B_NO_ERROR;
}


void
recursive_lock_destroy(recursive_lock *lock)
{
	if (lock == NULL)
		return;

	if (lock->sem > 0)
		delete_sem(lock->sem);
	lock->sem = -1;
}


bool
recursive_lock_lock(recursive_lock *lock)
{
	thread_id thid = thread_get_current_thread_id();
	bool retval = false;
	
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


int
mutex_init(mutex *m, const char *in_name)
{
	const char *name;

	if (m == NULL)
		return EINVAL;

	if (in_name == NULL)
		name = "mutex_sem";
	else
		name = in_name;

	m->holder = -1;

	m->sem = create_sem(1, name);
	if (m->sem < 0)
		return m->sem;

	return 0;
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

	if (me == mutex->holder)
		panic("mutex_lock failure: mutex %p acquired twice by thread 0x%x\n", mutex, me);

	acquire_sem(mutex->sem);
	mutex->holder = me;
}


void
mutex_unlock(mutex *mutex)
{
	thread_id me = thread_get_current_thread_id();

	if (me != mutex->holder)
		panic("mutex_unlock failure: thread 0x%x is trying to release mutex %p (current holder 0x%x)\n",
			me, mutex, mutex->holder);

	mutex->holder = -1;
	release_sem(mutex->sem);
}

