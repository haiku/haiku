/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

/* Mutex and recursive_lock code */

#include <lock.h>

#include <stdlib.h>
#include <string.h>

#include <OS.h>

#include <debug.h>
#include <int.h>
#include <kernel.h>
#include <thread.h>
#include <util/AutoLock.h>


struct mutex_waiter {
	struct thread*	thread;
	mutex_waiter*	next;		// next in queue
	mutex_waiter*	last;		// last in queue (valid for the first in queue)
};

#define MUTEX_FLAG_OWNS_NAME	MUTEX_FLAG_CLONE_NAME
#define MUTEX_FLAG_RELEASED		0x2


#ifdef KDEBUG
#	define RECURSIVE_LOCK_HOLDER(lock)	((lock)->lock.holder)
#else
#	define RECURSIVE_LOCK_HOLDER(lock)	((lock)->holder)
#endif


int32
recursive_lock_get_recursion(recursive_lock *lock)
{
	if (RECURSIVE_LOCK_HOLDER(lock) == thread_get_current_thread_id())
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
	thread_id thread = thread_get_current_thread_id();

	if (!kernel_startup && !are_interrupts_enabled())
		panic("recursive_lock_lock: called with interrupts disabled for lock "
			"%p (\"%s\")\n", lock, lock->lock.name);

	if (thread != RECURSIVE_LOCK_HOLDER(lock)) {
		mutex_lock(&lock->lock);
#ifndef KDEBUG
		lock->holder = thread;
#endif
	}

	lock->recursion++;
	return B_OK;
}


void
recursive_lock_unlock(recursive_lock *lock)
{
	if (thread_get_current_thread_id() != RECURSIVE_LOCK_HOLDER(lock))
		panic("recursive_lock %p unlocked by non-holder thread!\n", lock);

	if (--lock->recursion == 0) {
#ifndef KDEBUG
		lock->holder = -1;
#endif
		mutex_unlock(&lock->lock);
	}
}


//	#pragma mark -


status_t
benaphore_init(benaphore *ben, const char *name)
{
	if (ben == NULL || name == NULL)
		return B_BAD_VALUE;

	ben->count = 1;
#ifdef KDEBUG
	ben->sem = create_sem(1, name);
#else
	ben->sem = create_sem(0, name);
#endif
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
	return release_sem_etc(lock->sem, 1, 0/*B_DO_NOT_RESCHEDULE*/);
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


// #pragma mark -


void
mutex_init(mutex* lock, const char *name)
{
	lock->name = name;
	lock->waiters = NULL;
#ifdef KDEBUG
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
#ifdef KDEBUG
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
	InterruptsSpinLocker locker(thread_spinlock);

#ifdef KDEBUG
	if (lock->waiters != NULL && thread_get_current_thread_id()
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
		thread_unblock_locked(waiter->thread, B_ERROR);
	}

	lock->name = NULL;

	locker.Unlock();

	free(name);
}


status_t
_mutex_lock(mutex* lock, bool threadsLocked)
{
#ifdef KDEBUG
	if (!kernel_startup && !threadsLocked && !are_interrupts_enabled()) {
		panic("_mutex_unlock(): called with interrupts disabled for lock %p",
			lock);
	}
#endif

	// lock only, if !threadsLocked
	InterruptsSpinLocker locker(thread_spinlock, false, !threadsLocked);

	// Might have been released after we decremented the count, but before
	// we acquired the spinlock.
#ifdef KDEBUG
	if (lock->holder <= 0) {
		lock->holder = thread_get_current_thread_id();
		return B_OK;
	}
#else
	if ((lock->flags & MUTEX_FLAG_RELEASED) != 0) {
		lock->flags &= ~MUTEX_FLAG_RELEASED;
		return B_OK;
	}
#endif

	// enqueue in waiter list
	mutex_waiter waiter;
	waiter.thread = thread_get_current_thread();
	waiter.next = NULL;

	if (lock->waiters != NULL) {
		lock->waiters->last->next = &waiter;
	} else
		lock->waiters = &waiter;

	lock->waiters->last = &waiter;

	// block
	thread_prepare_to_block(waiter.thread, 0, THREAD_BLOCK_TYPE_MUTEX, lock);
	status_t error = thread_block_locked(waiter.thread);

#ifdef KDEBUG
	if (error == B_OK)
		lock->holder = waiter.thread->id;
#endif

	return error;
}


void
_mutex_unlock(mutex* lock)
{
	InterruptsSpinLocker _(thread_spinlock);

#ifdef KDEBUG
	if (thread_get_current_thread_id() != lock->holder) {
		panic("_mutex_unlock() failure: thread %ld is trying to release "
			"mutex %p (current holder %ld)\n", thread_get_current_thread_id(),
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
		thread_unblock_locked(waiter->thread, B_OK);

#ifdef KDEBUG
		// Already set the holder to the unblocked thread. Besides that this
		// actually reflects the current situation, setting it to -1 would
		// cause a race condition, since another locker could think the lock
		// is not held by anyone.
		lock->holder = waiter->thread->id;
#endif
	} else {
		// We've acquired the spinlock before the locker that is going to wait.
		// Just mark the lock as released.
#ifdef KDEBUG
		lock->holder = -1;
#else
		lock->flags |= MUTEX_FLAG_RELEASED;
#endif
	}
}


status_t
_mutex_trylock(mutex* lock)
{
#ifdef KDEBUG
	InterruptsSpinLocker _(thread_spinlock);

	if (lock->holder <= 0) {
		lock->holder = thread_get_current_thread_id();
		return B_OK;
	}
#endif
	return B_WOULD_BLOCK;
}


static int
dump_mutex_info(int argc, char** argv)
{
	if (argc < 2) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	mutex* lock = (mutex*)strtoul(argv[1], NULL, 0);

	if (!IS_KERNEL_ADDRESS(lock)) {
		kprintf("invalid address: %p\n", lock);
		return 0;
	}

	kprintf("mutex %p:\n", lock);
	kprintf("  name:            %s\n", lock->name);
	kprintf("  flags:           0x%x\n", lock->flags);
#ifdef KDEBUG
	kprintf("  holder:          %ld\n", lock->holder);
#else
	kprintf("  count:           %ld\n", lock->count);
#endif

	kprintf("  waiting threads:");
	mutex_waiter* waiter = lock->waiters;
	while (waiter != NULL) {
		kprintf(" %ld", waiter->thread->id);
		waiter = waiter->next;
	}
	kputs("\n");

	return 0;
}


// #pragma mark -


void
lock_debug_init()
{
	add_debugger_command_etc("mutex", &dump_mutex_info,
		"Dump info about a mutex",
		"<mutex>\n"
		"Prints info about the specified mutex.\n"
		"  <mutex>  - pointer to the mutex to print the info for.\n", 0);
}
