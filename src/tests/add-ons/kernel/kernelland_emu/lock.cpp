/*
 * Copyright 2002-2009, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Ingo Weinhold, bonefish@cs.tu-berlin.de.
 *		Axel Dörfler, axeld@pinc-software.de.
 */

#include <lock.h>

#include <stdlib.h>
#include <string.h>

#include <AutoLocker.h>

// libroot
#include <user_thread.h>

// system
#include <syscalls.h>
#include <user_thread_defs.h>

#include "thread.h"


struct mutex_waiter {
	struct thread*	thread;
	mutex_waiter*	next;		// next in queue
	mutex_waiter*	last;		// last in queue (valid for the first in queue)
};

struct rw_lock_waiter {
	struct thread*	thread;
	rw_lock_waiter*	next;		// next in queue
	rw_lock_waiter*	last;		// last in queue (valid for the first in queue)
	bool			writer;
};

#define MUTEX_FLAG_OWNS_NAME	MUTEX_FLAG_CLONE_NAME
#define MUTEX_FLAG_RELEASED		0x2

#define RW_LOCK_FLAG_OWNS_NAME	RW_LOCK_FLAG_CLONE_NAME


/*!	Helper class playing the role of the kernel's thread spinlock. We don't use
	as spinlock as that could be expensive in userland (due to spinlock holder
	potentially being unscheduled), but a benaphore.
*/
struct ThreadSpinlock {
	ThreadSpinlock()
		:
		fCount(1),
		fSemaphore(create_sem(0, "thread spinlock"))
	{
		if (fSemaphore < 0)
			panic("Failed to create thread spinlock semaphore!");
	}

	~ThreadSpinlock()
	{
		if (fSemaphore >= 0)
			delete_sem(fSemaphore);
	}

	bool Lock()
	{
		if (atomic_add(&fCount, -1) > 0)
			return true;

		status_t error;
		do {
			error = acquire_sem(fSemaphore);
		} while (error == B_INTERRUPTED);

		return error == B_OK;
	}

	void Unlock()
	{
		if (atomic_add(&fCount, 1) < 0)
			release_sem(fSemaphore);
	}

private:
	vint32	fCount;
	sem_id	fSemaphore;
};

static ThreadSpinlock sThreadSpinlock;


// #pragma mark -


int32
recursive_lock_get_recursion(recursive_lock *lock)
{
	if (RECURSIVE_LOCK_HOLDER(lock) == find_thread(NULL))
		return lock->recursion;

	return -1;
}


void
recursive_lock_init(recursive_lock *lock, const char *name)
{
	mutex_init(&lock->lock, name != NULL ? name : "recursive lock");
	RECURSIVE_LOCK_HOLDER(lock) = -1;
	lock->recursion = 0;
}


void
recursive_lock_init_etc(recursive_lock *lock, const char *name, uint32 flags)
{
	mutex_init_etc(&lock->lock, name != NULL ? name : "recursive lock", flags);
	RECURSIVE_LOCK_HOLDER(lock) = -1;
	lock->recursion = 0;
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

	if (thread != RECURSIVE_LOCK_HOLDER(lock)) {
		mutex_lock(&lock->lock);
#if !KDEBUG
		lock->holder = thread;
#endif
	}

	lock->recursion++;
	return B_OK;
}


status_t
recursive_lock_trylock(recursive_lock *lock)
{
	thread_id thread = find_thread(NULL);

	if (thread != RECURSIVE_LOCK_HOLDER(lock)) {
		status_t status = mutex_trylock(&lock->lock);
		if (status != B_OK)
			return status;

#if !KDEBUG
		lock->holder = thread;
#endif
	}

	lock->recursion++;
	return B_OK;
}


void
recursive_lock_unlock(recursive_lock *lock)
{
	if (find_thread(NULL) != RECURSIVE_LOCK_HOLDER(lock))
		panic("recursive_lock %p unlocked by non-holder thread!\n", lock);

	if (--lock->recursion == 0) {
#if !KDEBUG
		lock->holder = -1;
#endif
		mutex_unlock(&lock->lock);
	}
}


//	#pragma mark -


static status_t
rw_lock_wait(rw_lock* lock, bool writer)
{
	// enqueue in waiter list
	rw_lock_waiter waiter;
	waiter.thread = get_current_thread();
	waiter.next = NULL;
	waiter.writer = writer;

	if (lock->waiters != NULL)
		lock->waiters->last->next = &waiter;
	else
		lock->waiters = &waiter;

	lock->waiters->last = &waiter;

	// block
	get_user_thread()->wait_status = 1;
	sThreadSpinlock.Unlock();

	status_t error;
	while ((error = _kern_block_thread(0, 0)) == B_INTERRUPTED) {
	}

	sThreadSpinlock.Lock();
	return error;
}


static void
rw_lock_unblock(rw_lock* lock)
{
	// Check whether there any waiting threads at all and whether anyone
	// has the write lock.
	rw_lock_waiter* waiter = lock->waiters;
	if (waiter == NULL || lock->holder > 0)
		return;

	// writer at head of queue?
	if (waiter->writer) {
		if (lock->reader_count == 0) {
			// dequeue writer
			lock->waiters = waiter->next;
			if (lock->waiters != NULL)
				lock->waiters->last = waiter->last;

			lock->holder = get_thread_id(waiter->thread);

			// unblock thread
			_kern_unblock_thread(get_thread_id(waiter->thread), B_OK);
		}
		return;
	}

	// wake up one or more readers
	while ((waiter = lock->waiters) != NULL && !waiter->writer) {
		// dequeue reader
		lock->waiters = waiter->next;
		if (lock->waiters != NULL)
			lock->waiters->last = waiter->last;

		lock->reader_count++;

		// unblock thread
		_kern_unblock_thread(get_thread_id(waiter->thread), B_OK);
	}
}


void
rw_lock_init(rw_lock* lock, const char* name)
{
	lock->name = name;
	lock->waiters = NULL;
	lock->holder = -1;
	lock->reader_count = 0;
	lock->writer_count = 0;
	lock->owner_count = 0;
	lock->flags = 0;
}


void
rw_lock_init_etc(rw_lock* lock, const char* name, uint32 flags)
{
	lock->name = (flags & RW_LOCK_FLAG_CLONE_NAME) != 0 ? strdup(name) : name;
	lock->waiters = NULL;
	lock->holder = -1;
	lock->reader_count = 0;
	lock->writer_count = 0;
	lock->owner_count = 0;
	lock->flags = flags & RW_LOCK_FLAG_CLONE_NAME;
}


void
rw_lock_destroy(rw_lock* lock)
{
	char* name = (lock->flags & RW_LOCK_FLAG_CLONE_NAME) != 0
		? (char*)lock->name : NULL;

	// unblock all waiters
	AutoLocker<ThreadSpinlock> locker(sThreadSpinlock);

#if KDEBUG
	if (lock->waiters != NULL && find_thread(NULL)
		!= lock->holder) {
		panic("rw_lock_destroy(): there are blocking threads, but the caller "
			"doesn't hold the write lock (%p)", lock);

		locker.Unlock();
		if (rw_lock_write_lock(lock) != B_OK)
			return;
		locker.Lock();
	}
#endif

	while (rw_lock_waiter* waiter = lock->waiters) {
		// dequeue
		lock->waiters = waiter->next;

		// unblock thread
		_kern_unblock_thread(get_thread_id(waiter->thread), B_ERROR);
	}

	lock->name = NULL;

	locker.Unlock();

	free(name);
}


status_t
rw_lock_read_lock(rw_lock* lock)
{
#if KDEBUG_RW_LOCK_DEBUG
	return rw_lock_write_lock(lock);
#else
	AutoLocker<ThreadSpinlock> locker(sThreadSpinlock);

	if (lock->writer_count == 0) {
		lock->reader_count++;
		return B_OK;
	}
	if (lock->holder == find_thread(NULL)) {
		lock->owner_count++;
		return B_OK;
	}

	return rw_lock_wait(lock, false);
#endif
}


status_t
rw_lock_read_unlock(rw_lock* lock)
{
#if KDEBUG_RW_LOCK_DEBUG
	return rw_lock_write_unlock(lock);
#else
	AutoLocker<ThreadSpinlock> locker(sThreadSpinlock);

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
		panic("rw_lock_read_unlock(): lock %p not read-locked", lock);
		return B_BAD_VALUE;
	}

	lock->reader_count--;

	rw_lock_unblock(lock);
	return B_OK;
#endif
}


status_t
rw_lock_write_lock(rw_lock* lock)
{
	AutoLocker<ThreadSpinlock> locker(sThreadSpinlock);

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

	status_t status = rw_lock_wait(lock, true);
	if (status == B_OK) {
		lock->holder = find_thread(NULL);
		lock->owner_count = 1;
	}
	return status;
}


status_t
rw_lock_write_unlock(rw_lock* lock)
{
	AutoLocker<ThreadSpinlock> locker(sThreadSpinlock);

	if (find_thread(NULL) != lock->holder) {
		panic("rw_lock_write_unlock(): lock %p not write-locked by this thread",
			lock);
		return B_BAD_VALUE;
	}
	if (--lock->owner_count > 0)
		return B_OK;

	lock->writer_count--;
	lock->holder = -1;

	rw_lock_unblock(lock);

	return B_OK;
}


// #pragma mark -


void
mutex_init(mutex* lock, const char *name)
{
	lock->name = name;
	lock->waiters = NULL;
#if KDEBUG
	lock->holder = -1;
#else
	lock->count = 0;
#endif
	lock->flags = 0;
}


void
mutex_init_etc(mutex* lock, const char *name, uint32 flags)
{
	lock->name = (flags & MUTEX_FLAG_CLONE_NAME) != 0 ? strdup(name) : name;
	lock->waiters = NULL;
#if KDEBUG
	lock->holder = -1;
#else
	lock->count = 0;
#endif
	lock->flags = flags & MUTEX_FLAG_CLONE_NAME;
}


void
mutex_destroy(mutex* lock)
{
	char* name = (lock->flags & MUTEX_FLAG_CLONE_NAME) != 0
		? (char*)lock->name : NULL;

	// unblock all waiters
	AutoLocker<ThreadSpinlock> locker(sThreadSpinlock);

#if KDEBUG
	if (lock->waiters != NULL && find_thread(NULL)
		!= lock->holder) {
		panic("mutex_destroy(): there are blocking threads, but caller doesn't "
			"hold the lock (%p)", lock);
		if (_mutex_lock(lock, true) != B_OK)
			return;
	}
#endif

	while (mutex_waiter* waiter = lock->waiters) {
		// dequeue
		lock->waiters = waiter->next;

		// unblock thread
		_kern_unblock_thread(get_thread_id(waiter->thread), B_ERROR);
	}

	lock->name = NULL;

	locker.Unlock();

	free(name);
}


status_t
mutex_switch_lock(mutex* from, mutex* to)
{
	AutoLocker<ThreadSpinlock> locker(sThreadSpinlock);

#if !KDEBUG
	if (atomic_add(&from->count, 1) < -1)
#endif
		_mutex_unlock(from, true);

	return mutex_lock_threads_locked(to);
}


status_t
_mutex_lock(mutex* lock, bool threadsLocked)
{
	// lock only, if !threadsLocked
	AutoLocker<ThreadSpinlock> locker(sThreadSpinlock, false, !threadsLocked);

	// Might have been released after we decremented the count, but before
	// we acquired the spinlock.
#if KDEBUG
	if (lock->holder < 0) {
		lock->holder = find_thread(NULL);
		return B_OK;
	} else if (lock->holder == find_thread(NULL)) {
		panic("_mutex_lock(): double lock of %p by thread %ld", lock,
			lock->holder);
	} else if (lock->holder == 0)
		panic("_mutex_lock(): using unitialized lock %p", lock);
#else
	if ((lock->flags & MUTEX_FLAG_RELEASED) != 0) {
		lock->flags &= ~MUTEX_FLAG_RELEASED;
		return B_OK;
	}
#endif

	// enqueue in waiter list
	mutex_waiter waiter;
	waiter.thread = get_current_thread();
	waiter.next = NULL;

	if (lock->waiters != NULL) {
		lock->waiters->last->next = &waiter;
	} else
		lock->waiters = &waiter;

	lock->waiters->last = &waiter;

	// block
	get_user_thread()->wait_status = 1;
	locker.Unlock();

	status_t error;
	while ((error = _kern_block_thread(0, 0)) == B_INTERRUPTED) {
	}

	locker.Lock();

#if KDEBUG
	if (error == B_OK)
		lock->holder = get_thread_id(waiter.thread);
#endif

	return error;
}


void
_mutex_unlock(mutex* lock, bool threadsLocked)
{
	// lock only, if !threadsLocked
	AutoLocker<ThreadSpinlock> locker(sThreadSpinlock, false, !threadsLocked);

#if KDEBUG
	if (find_thread(NULL) != lock->holder) {
		panic("_mutex_unlock() failure: thread %ld is trying to release "
			"mutex %p (current holder %ld)\n", find_thread(NULL),
			lock, lock->holder);
		return;
	}
#endif

	mutex_waiter* waiter = lock->waiters;
	if (waiter != NULL) {
		// dequeue the first waiter
		lock->waiters = waiter->next;
		if (lock->waiters != NULL)
			lock->waiters->last = waiter->last;

		// unblock thread
		_kern_unblock_thread(get_thread_id(waiter->thread), B_OK);

#if KDEBUG
		// Already set the holder to the unblocked thread. Besides that this
		// actually reflects the current situation, setting it to -1 would
		// cause a race condition, since another locker could think the lock
		// is not held by anyone.
		lock->holder = get_thread_id(waiter->thread);
#endif
	} else {
		// We've acquired the spinlock before the locker that is going to wait.
		// Just mark the lock as released.
#if KDEBUG
		lock->holder = -1;
#else
		lock->flags |= MUTEX_FLAG_RELEASED;
#endif
	}
}


status_t
_mutex_trylock(mutex* lock)
{
#if KDEBUG
	AutoLocker<ThreadSpinlock> _(sThreadSpinlock);

	if (lock->holder <= 0) {
		lock->holder = find_thread(NULL);
		return B_OK;
	}
#endif
	return B_WOULD_BLOCK;
}
