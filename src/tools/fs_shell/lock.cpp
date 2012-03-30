/*
 * Copyright 2002-2012, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

/* Mutex and recursive_lock code */

#include "fssh_lock.h"

#include "fssh_kernel_export.h"


#define FSSH_RW_MAX_READERS 100000


extern "C" int32_t
fssh_recursive_lock_get_recursion(fssh_recursive_lock *lock)
{
	if (lock->holder == fssh_find_thread(NULL))
		return lock->recursion;

	return -1;
}


extern "C" void
fssh_recursive_lock_init(fssh_recursive_lock *lock, const char *name)
{
	if (lock == NULL)
		return;

	if (name == NULL)
		name = "recursive lock";

	lock->holder = -1;
	lock->recursion = 0;
	lock->sem = fssh_create_sem(1, name);
	if (lock->sem < FSSH_B_OK)
		fssh_panic("could not create recursive lock");
}


extern "C" void
fssh_recursive_lock_destroy(fssh_recursive_lock *lock)
{
	if (lock == NULL)
		return;

	fssh_delete_sem(lock->sem);
	lock->sem = -1;
}


extern "C" fssh_status_t
fssh_recursive_lock_lock(fssh_recursive_lock *lock)
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


extern "C" fssh_status_t
fssh_recursive_lock_trylock(fssh_recursive_lock *lock)
{
	fssh_thread_id thread = fssh_find_thread(NULL);

	if (thread != lock->holder) {
		fssh_status_t status = fssh_acquire_sem_etc(lock->sem, 1,
			FSSH_B_RELATIVE_TIMEOUT, 0);
		if (status < FSSH_B_OK)
			return status;

		lock->holder = thread;
	}
	lock->recursion++;
	return FSSH_B_OK;
}


extern "C" void
fssh_recursive_lock_unlock(fssh_recursive_lock *lock)
{
	if (fssh_find_thread(NULL) != lock->holder)
		fssh_panic("recursive_lock %p unlocked by non-holder thread!\n", lock);

	if (--lock->recursion == 0) {
		lock->holder = -1;
		fssh_release_sem(lock->sem);
	}
}


extern "C" void
fssh_recursive_lock_transfer_lock(fssh_recursive_lock *lock,
	fssh_thread_id thread)
{
	if (lock->recursion != 1)
		fssh_panic("invalid recursion level for lock transfer!");

	lock->holder = thread;
}


//	#pragma mark -


extern "C" void
fssh_mutex_init(fssh_mutex *m, const char *name)
{
	if (m == NULL)
		return;

	if (name == NULL)
		name = "mutex_sem";

	m->holder = -1;

	m->sem = fssh_create_sem(1, name);
	if (m->sem < FSSH_B_OK)
		fssh_panic("could not create mutex");
}


extern "C" void
fssh_mutex_init_etc(fssh_mutex *m, const char *name, uint32_t flags)
{
	fssh_mutex_init(m, name);
}


extern "C" void
fssh_mutex_destroy(fssh_mutex *mutex)
{
	if (mutex == NULL)
		return;

	if (mutex->sem >= 0) {
		fssh_delete_sem(mutex->sem);
		mutex->sem = -1;
	}
	mutex->holder = -1;
}


extern "C" fssh_status_t
fssh_mutex_lock(fssh_mutex *mutex)
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


extern "C" void
fssh_mutex_unlock(fssh_mutex *mutex)
{
	fssh_thread_id me = fssh_find_thread(NULL);

	if (me != mutex->holder) {
		fssh_panic("mutex_unlock failure: thread 0x%x is trying to release mutex %p (current holder 0x%x)\n",
			(int)me, mutex, (int)mutex->holder);
	}

	mutex->holder = -1;
	fssh_release_sem(mutex->sem);
}


extern "C" void
fssh_mutex_transfer_lock(fssh_mutex *mutex, fssh_thread_id thread)
{
	mutex->holder = thread;
}


//	#pragma mark -


extern "C" void
fssh_rw_lock_init(fssh_rw_lock *lock, const char *name)
{
	if (lock == NULL)
		return;

	if (name == NULL)
		name = "r/w lock";

	lock->count = 0;
	lock->holder = -1;

	lock->sem = fssh_create_sem(FSSH_RW_MAX_READERS, name);
	if (lock->sem < FSSH_B_OK)
		fssh_panic("could not create r/w lock");
}


extern "C" void
fssh_rw_lock_init_etc(fssh_rw_lock *lock, const char *name, uint32_t flags)
{
	fssh_rw_lock_init(lock, name);
}


extern "C" void
fssh_rw_lock_destroy(fssh_rw_lock *lock)
{
	if (lock == NULL)
		return;

	fssh_delete_sem(lock->sem);
}


extern "C" fssh_status_t
fssh_rw_lock_read_lock(fssh_rw_lock *lock)
{
	if (lock->holder == fssh_find_thread(NULL)) {
		lock->count++;
		return FSSH_B_OK;
	}

	return fssh_acquire_sem(lock->sem);
}


extern "C" fssh_status_t
fssh_rw_lock_read_unlock(fssh_rw_lock *lock)
{
	if (lock->holder == fssh_find_thread(NULL) && --lock->count > 0)
		return FSSH_B_OK;

	return fssh_release_sem(lock->sem);
}


extern "C" fssh_status_t
fssh_rw_lock_write_lock(fssh_rw_lock *lock)
{
	if (lock->holder == fssh_find_thread(NULL)) {
		lock->count++;
		return FSSH_B_OK;
	}

	fssh_status_t status = fssh_acquire_sem_etc(lock->sem, FSSH_RW_MAX_READERS,
		0, 0);
	if (status == FSSH_B_OK) {
		lock->holder = fssh_find_thread(NULL);
		lock->count = 1;
	}
	return status;
}


extern "C" fssh_status_t
fssh_rw_lock_write_unlock(fssh_rw_lock *lock)
{
	if (--lock->count > 0)
		return FSSH_B_OK;

	lock->holder = -1;

	return fssh_release_sem_etc(lock->sem, FSSH_RW_MAX_READERS, 0);
}

