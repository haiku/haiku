/*
 * Copyright 2009, Michael Lotz, mmlr@mlotz.ch.
 * Distributed under the terms of the MIT License.
 */
/*
 * Copyright 2008-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2009, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <locks.h>
#include <syscalls.h>
#include <user_thread.h>

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


// #pragma mark - R/W lock


typedef struct rw_lock_waiter {
	rw_lock_waiter *	next;
	thread_id			thread;
	bool				writer;
} rw_lock_waiter;


static status_t
rw_lock_wait(rw_lock *lock, bool writer)
{
	rw_lock_waiter waiter;
	waiter.thread = find_thread(NULL);
	waiter.next = NULL;
	waiter.writer = writer;

	if (lock->waiters != NULL)
		lock->last_waiter->next = &waiter;
	else
		lock->waiters = &waiter;

	lock->last_waiter = &waiter;

	// the rw_lock is locked when entering, release it before blocking
	get_user_thread()->wait_status = 1;
	mutex_unlock(&lock->lock);

	status_t result;
	do {
		result = _kern_block_thread(0, 0);
	} while (result == B_INTERRUPTED);

	// and lock it again before returning
	mutex_lock(&lock->lock);
	return result;
}


static void
rw_lock_unblock(rw_lock *lock)
{
	// this is called locked
	if (lock->holder >= 0)
		return;

	rw_lock_waiter *waiter = lock->waiters;
	if (waiter == NULL)
		return;

	if (waiter->writer) {
		if (lock->reader_count > 0)
			return;

		lock->waiters = waiter->next;
		lock->holder = waiter->thread;
		_kern_unblock_thread(waiter->thread, B_OK);
		return;
	}

	while (waiter != NULL && !waiter->writer) {
		lock->reader_count++;
		lock->waiters = waiter->next;
		_kern_unblock_thread(waiter->thread, B_OK);
		waiter = lock->waiters;
	}
}


status_t
rw_lock_init(rw_lock *lock, const char *name)
{
	lock->name = name;
	lock->waiters = NULL;
	lock->holder = -1;
	lock->reader_count = 0;
	lock->writer_count = 0;
	lock->owner_count = 0;
	return mutex_init(&lock->lock, name);
}


void
rw_lock_destroy(rw_lock *lock)
{
	mutex_lock(&lock->lock);

	rw_lock_waiter *waiter = lock->waiters;
	while (waiter != NULL) {
		_kern_unblock_thread(waiter->thread, B_ERROR);
		waiter = waiter->next;
	}

	mutex_destroy(&lock->lock);
}


status_t
rw_lock_read_lock(rw_lock *lock)
{
	MutexLocker locker(lock->lock);

	if (lock->writer_count == 0) {
		lock->reader_count++;
		return B_OK;
	}

	if (lock->holder == find_thread(NULL)) {
		lock->owner_count++;
		return B_OK;
	}

	return rw_lock_wait(lock, false);
}


status_t
rw_lock_read_unlock(rw_lock *lock)
{
	MutexLocker locker(lock->lock);

	if (lock->holder == find_thread(NULL)) {
		if (--lock->owner_count > 0)
			return B_OK;

		// this originally has been a write lock
		lock->writer_count--;
		lock->holder = -1;

		rw_lock_unblock(lock);
		return B_OK;
	}

	if (lock->reader_count <= 0) {
		debugger("rw_lock not read locked");
		return B_ERROR;
	}

	lock->reader_count--;
	rw_lock_unblock(lock);
	return B_OK;
}


status_t
rw_lock_write_lock(rw_lock *lock)
{
	MutexLocker locker(lock->lock);

	if (lock->reader_count == 0 && lock->writer_count == 0) {
		lock->writer_count++;
		lock->holder = find_thread(NULL);
		lock->owner_count = 1;
		return B_OK;
	}

	if (lock->holder == find_thread(NULL)) {
		lock->owner_count++;
		return B_OK;
	}

	lock->writer_count++;

	status_t result = rw_lock_wait(lock, true);
	if (result != B_OK)
		return result;

	if (lock->holder != find_thread(NULL)) {
		debugger("write locked but holder not set");
		return B_ERROR;
	}

	lock->owner_count = 1;
	return B_OK;
}


status_t
rw_lock_write_unlock(rw_lock *lock)
{
	MutexLocker locker(lock->lock);

	if (lock->holder != find_thread(NULL)) {
		debugger("rw_lock not write locked");
		return B_ERROR;
	}

	if (--lock->owner_count > 0)
		return B_OK;

	lock->writer_count--;
	lock->holder = -1;
	rw_lock_unblock(lock);
	return B_OK;
}


// #pragma mark - recursive lock


int32
recursive_lock_get_recursion(recursive_lock *lock)
{
	if (lock->holder == find_thread(NULL))
		return lock->recursion;

	return -1;
}


status_t
recursive_lock_init(recursive_lock *lock, const char *name)
{
	lock->holder = -1;
	lock->recursion = 0;
	return mutex_init(&lock->lock, name != NULL ? name : "recursive lock");
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
