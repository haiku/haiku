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

#include <OS.h>

#include <debug.h>
#include <int.h>
#include <kernel.h>
#include <thread.h>
#include <util/AutoLock.h>


struct cutex_waiter {
	struct thread*	thread;
	cutex_waiter*	next;		// next in queue
	cutex_waiter*	last;		// last in queue (valid for the first in queue)
};


int32
recursive_lock_get_recursion(recursive_lock *lock)
{
	if (lock->holder == thread_get_current_thread_id())
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


status_t
recursive_lock_lock(recursive_lock *lock)
{
	thread_id thread = thread_get_current_thread_id();

	if (!kernel_startup && !are_interrupts_enabled())
		panic("recursive_lock_lock: called with interrupts disabled for lock %p, sem %#lx\n", lock, lock->sem);

	if (thread != lock->holder) {
		status_t status = acquire_sem(lock->sem);
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
	if (thread_get_current_thread_id() != lock->holder)
		panic("recursive_lock %p unlocked by non-holder thread!\n", lock);

	if (--lock->recursion == 0) {
		lock->holder = -1;
		release_sem_etc(lock->sem, 1, 0/*B_DO_NOT_RESCHEDULE*/);
	}
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


status_t
mutex_trylock(mutex *mutex)
{
	thread_id me = thread_get_current_thread_id();
	status_t status;

	if (kernel_startup)
		return B_OK;

	status = acquire_sem_etc(mutex->sem, 1, B_RELATIVE_TIMEOUT, 0);
	if (status < B_OK)
		return status;

	if (me == mutex->holder) {
		panic("mutex_trylock failure: mutex %p (sem = 0x%lx) acquired twice by"
			" thread 0x%lx\n", mutex, mutex->sem, me);
	}

	mutex->holder = me;
	return B_OK;
}


status_t
mutex_lock(mutex *mutex)
{
	thread_id me = thread_get_current_thread_id();
	status_t status;

	if (kernel_startup)
		return B_OK;

	status = acquire_sem(mutex->sem);
	if (status < B_OK)
		return status;

	if (me == mutex->holder) {
		panic("mutex_lock failure: mutex %p (sem = 0x%lx) acquired twice by"
			" thread 0x%lx\n", mutex, mutex->sem, me);
	}

	mutex->holder = me;
	return B_OK;
}


void
mutex_unlock(mutex *mutex)
{
	thread_id me = thread_get_current_thread_id();

	if (kernel_startup)
		return;

	if (me != mutex->holder) {
		panic("mutex_unlock failure: thread 0x%lx is trying to release mutex %p"
			" (current holder 0x%lx)\n", me, mutex, mutex->holder);
	}

	mutex->holder = -1;
	release_sem_etc(mutex->sem, 1, 0/*B_DO_NOT_RESCHEDULE*/);
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
cutex_init(cutex* lock, const char *name)
{
	lock->name = name;
	lock->waiters = NULL;
#ifdef KDEBUG
	lock->holder = -1;
#else
	lock->count = 0;
#endif
	lock->release_count = 0;
}


void
cutex_destroy(cutex* lock)
{
	// no-op
}


void
_cutex_lock(cutex* lock)
{
#ifdef KDEBUG
	if (!kernel_startup && !are_interrupts_enabled()) {
		panic("_cutex_unlock: called with interrupts disabled for lock %p",
			lock);
	}
#endif

	InterruptsSpinLocker _(thread_spinlock);

	// Might have been released after we decremented the count, but before
	// we acquired the spinlock.
#ifdef KDEBUG
	if (lock->release_count >= 0) {
		lock->holder = thread_get_current_thread_id();
#else
	if (lock->release_count > 0) {
#endif
		lock->release_count--;
		return;
	}

	// enqueue in waiter list
	cutex_waiter waiter;
	waiter.thread = thread_get_current_thread();
	waiter.next = NULL;

	if (lock->waiters != NULL) {
		lock->waiters->last->next = &waiter;
	} else
		lock->waiters = &waiter;

	lock->waiters->last = &waiter;

	// block
	thread_prepare_to_block(waiter.thread, 0, THREAD_BLOCK_TYPE_CUTEX, lock);
	thread_block_locked(waiter.thread);

#ifdef KDEBUG
	lock->holder = waiter.thread->id;
#endif
}


void
_cutex_unlock(cutex* lock)
{
	InterruptsSpinLocker _(thread_spinlock);

#ifdef KDEBUG
	if (thread_get_current_thread_id() != lock->holder) {
		panic("_cutex_unlock() failure: thread %ld is trying to release "
			"cutex %p (current holder %ld)\n", thread_get_current_thread_id(),
			lock, lock->holder);
		return;
	}

	lock->holder = -1;
#endif

	cutex_waiter* waiter = lock->waiters;
	if (waiter != NULL) {
		// dequeue the first waiter
		lock->waiters = waiter->next;
		if (lock->waiters != NULL)
			lock->waiters->last = waiter->last;

		// unblock thread
		thread_unblock_locked(waiter->thread, B_OK);
	} else {
		// We acquired the spinlock before the locker that is going to wait.
		// Just increment the release count.
		lock->release_count++;
	}
}


status_t
_cutex_trylock(cutex* lock)
{
#ifdef KDEBUG
	InterruptsSpinLocker _(thread_spinlock);

	if (lock->release_count >= 0) {
		lock->holder = thread_get_current_thread_id();
		lock->release_count--;
		return B_OK;
	}
#endif
	return B_WOULD_BLOCK;
}


static int
dump_cutex_info(int argc, char** argv)
{
	if (argc < 2) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	cutex* lock = (cutex*)strtoul(argv[1], NULL, 0);

	if (!IS_KERNEL_ADDRESS(lock)) {
		kprintf("invalid address: %p\n", lock);
		return 0;
	}

	kprintf("cutex %p:\n", lock);
	kprintf("  name:            %s\n", lock->name);
	kprintf("  release count:   %ld\n", lock->release_count);
#ifdef KDEBUG
	kprintf("  holder:          %ld\n", lock->holder);
#else
	kprintf("  count:           %ld\n", lock->count);
#endif

	kprintf("  waiting threads:");
	cutex_waiter* waiter = lock->waiters;
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
	add_debugger_command_etc("cutex", &dump_cutex_info,
		"Dump info about a cutex",
		"<cutex>\n"
		"Prints info about the specified cutex.\n"
		"  <cutex>  - pointer to the cutex to print the info for.\n", 0);
}
