/*
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

/* Mutex and recursive_lock code */

#include "lock.h"

#include "fssh_kernel_export.h"


namespace FSShell {


int32_t
recursive_lock_get_recursion(recursive_lock *lock)
{
	if (lock->holder == fssh_find_thread(NULL))
		return lock->recursion;

	return -1;
}


fssh_status_t
recursive_lock_init(recursive_lock *lock, const char *name)
{
	if (lock == NULL)
		return FSSH_B_BAD_VALUE;

	if (name == NULL)
		name = "recursive lock";

	lock->holder = -1;
	lock->recursion = 0;
	lock->sem = fssh_create_sem(1, name);

	if (lock->sem >= FSSH_B_OK)
		return FSSH_B_OK;

	return lock->sem;
}


void
recursive_lock_destroy(recursive_lock *lock)
{
	if (lock == NULL)
		return;

	fssh_delete_sem(lock->sem);
	lock->sem = -1;
}


fssh_status_t
recursive_lock_lock(recursive_lock *lock)
{
	fssh_thread_id thread = fssh_find_thread(NULL);

	if (thread != lock->holder) {
		fssh_status_t status = fssh_acquire_sem(lock->sem);
		if (status < FSSH_B_OK)
			return status;

		lock->holder = thread;
	}
	lock->recursion++;
	return FSSH_B_OK;
}


void
recursive_lock_unlock(recursive_lock *lock)
{
	if (fssh_find_thread(NULL) != lock->holder)
		fssh_panic("recursive_lock %p unlocked by non-holder thread!\n", lock);

	if (--lock->recursion == 0) {
		lock->holder = -1;
		fssh_release_sem(lock->sem);
	}
}


//	#pragma mark -


fssh_status_t
mutex_init(mutex *m, const char *name)
{
	if (m == NULL)
		return FSSH_EINVAL;

	if (name == NULL)
		name = "mutex_sem";

	m->holder = -1;

	m->sem = fssh_create_sem(1, name);
	if (m->sem >= FSSH_B_OK)
		return FSSH_B_OK;

	return m->sem;
}


void
mutex_destroy(mutex *mutex)
{
	if (mutex == NULL)
		return;

	if (mutex->sem >= 0) {
		fssh_delete_sem(mutex->sem);
		mutex->sem = -1;
	}
	mutex->holder = -1;
}


fssh_status_t
mutex_lock(mutex *mutex)
{
	fssh_thread_id me = fssh_find_thread(NULL);
	fssh_status_t status;

	status = fssh_acquire_sem(mutex->sem);
	if (status < FSSH_B_OK)
		return status;

	if (me == mutex->holder)
		fssh_panic("mutex_lock failure: mutex %p (sem = 0x%x) acquired twice by thread 0x%x\n", mutex, (int)mutex->sem, (int)me);

	mutex->holder = me;
	return FSSH_B_OK;
}


void
mutex_unlock(mutex *mutex)
{
	fssh_thread_id me = fssh_find_thread(NULL);

	if (me != mutex->holder) {
		fssh_panic("mutex_unlock failure: thread 0x%x is trying to release mutex %p (current holder 0x%x)\n",
			(int)me, mutex, (int)mutex->holder);
	}

	mutex->holder = -1;
	fssh_release_sem(mutex->sem);
}


//	#pragma mark -


fssh_status_t
benaphore_init(benaphore *ben, const char *name)
{
	if (ben == NULL || name == NULL)
		return FSSH_B_BAD_VALUE;

	ben->count = 1;
	ben->sem = fssh_create_sem(0, name);
	if (ben->sem >= FSSH_B_OK)
		return FSSH_B_OK;

	return ben->sem;
}


void
benaphore_destroy(benaphore *ben)
{
	fssh_delete_sem(ben->sem);
	ben->sem = -1;
}


//	#pragma mark -


fssh_status_t
rw_lock_init(rw_lock *lock, const char *name)
{
	if (lock == NULL)
		return FSSH_B_BAD_VALUE;

	if (name == NULL)
		name = "r/w lock";

	lock->sem = fssh_create_sem(FSSH_RW_MAX_READERS, name);
	if (lock->sem >= FSSH_B_OK)
		return FSSH_B_OK;

	return lock->sem;
}


void
rw_lock_destroy(rw_lock *lock)
{
	if (lock == NULL)
		return;

	fssh_delete_sem(lock->sem);
}


fssh_status_t
rw_lock_read_lock(rw_lock *lock)
{
	return fssh_acquire_sem(lock->sem);
}


fssh_status_t
rw_lock_read_unlock(rw_lock *lock)
{
	return fssh_release_sem(lock->sem);
}


fssh_status_t
rw_lock_write_lock(rw_lock *lock)
{
	return fssh_acquire_sem_etc(lock->sem, FSSH_RW_MAX_READERS, 0, 0);
}


fssh_status_t
rw_lock_write_unlock(rw_lock *lock)
{
	return fssh_release_sem_etc(lock->sem, FSSH_RW_MAX_READERS, 0);
}


}	// namespace FSShell
