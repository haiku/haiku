/*
 * Copyright 2008-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2009, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


/*! Mutex and recursive_lock code */


#include <lock.h>

#include <stdlib.h>
#include <string.h>

#include <OS.h>

#include <debug.h>
#include <int.h>
#include <kernel.h>
#include <listeners.h>
#include <scheduling_analysis.h>
#include <thread.h>
#include <util/AutoLock.h>


struct mutex_waiter {
	Thread*			thread;
	mutex_waiter*	next;		// next in queue
	mutex_waiter*	last;		// last in queue (valid for the first in queue)
};

struct rw_lock_waiter {
	Thread*			thread;
	rw_lock_waiter*	next;		// next in queue
	rw_lock_waiter*	last;		// last in queue (valid for the first in queue)
	bool			writer;
};

#define MUTEX_FLAG_OWNS_NAME	MUTEX_FLAG_CLONE_NAME
#define MUTEX_FLAG_RELEASED		0x2

#define RW_LOCK_FLAG_OWNS_NAME	RW_LOCK_FLAG_CLONE_NAME


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

	if (!gKernelStartup && !are_interrupts_enabled()) {
		panic("recursive_lock_lock: called with interrupts disabled for lock "
			"%p (\"%s\")\n", lock, lock->lock.name);
	}

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
	thread_id thread = thread_get_current_thread_id();

	if (!gKernelStartup && !are_interrupts_enabled())
		panic("recursive_lock_lock: called with interrupts disabled for lock "
			"%p (\"%s\")\n", lock, lock->lock.name);

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
	if (thread_get_current_thread_id() != RECURSIVE_LOCK_HOLDER(lock))
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
	waiter.thread = thread_get_current_thread();
	waiter.next = NULL;
	waiter.writer = writer;

	if (lock->waiters != NULL)
		lock->waiters->last->next = &waiter;
	else
		lock->waiters = &waiter;

	lock->waiters->last = &waiter;

	// block
	thread_prepare_to_block(waiter.thread, 0, THREAD_BLOCK_TYPE_RW_LOCK, lock);
	return thread_block_locked(waiter.thread);
}


static int32
rw_lock_unblock(rw_lock* lock)
{
	// Check whether there are any waiting threads at all and whether anyone
	// has the write lock.
	rw_lock_waiter* waiter = lock->waiters;
	if (waiter == NULL || lock->holder >= 0)
		return 0;

	// writer at head of queue?
	if (waiter->writer) {
		if (lock->active_readers > 0 || lock->pending_readers > 0)
			return 0;

		// dequeue writer
		lock->waiters = waiter->next;
		if (lock->waiters != NULL)
			lock->waiters->last = waiter->last;

		lock->holder = waiter->thread->id;

		// unblock thread
		thread_unblock_locked(waiter->thread, B_OK);
		waiter->thread = NULL;
		return RW_LOCK_WRITER_COUNT_BASE;
	}

	// wake up one or more readers
	uint32 readerCount = 0;
	do {
		// dequeue reader
		lock->waiters = waiter->next;
		if (lock->waiters != NULL)
			lock->waiters->last = waiter->last;

		readerCount++;

		// unblock thread
		thread_unblock_locked(waiter->thread, B_OK);
		waiter->thread = NULL;
	} while ((waiter = lock->waiters) != NULL && !waiter->writer);

	if (lock->count >= RW_LOCK_WRITER_COUNT_BASE)
		lock->active_readers += readerCount;

	return readerCount;
}


void
rw_lock_init(rw_lock* lock, const char* name)
{
	lock->name = name;
	lock->waiters = NULL;
	lock->holder = -1;
	lock->count = 0;
	lock->owner_count = 0;
	lock->active_readers = 0;
	lock->pending_readers = 0;
	lock->flags = 0;

	T_SCHEDULING_ANALYSIS(InitRWLock(lock, name));
	NotifyWaitObjectListeners(&WaitObjectListener::RWLockInitialized, lock);
}


void
rw_lock_init_etc(rw_lock* lock, const char* name, uint32 flags)
{
	lock->name = (flags & RW_LOCK_FLAG_CLONE_NAME) != 0 ? strdup(name) : name;
	lock->waiters = NULL;
	lock->holder = -1;
	lock->count = 0;
	lock->owner_count = 0;
	lock->active_readers = 0;
	lock->pending_readers = 0;
	lock->flags = flags & RW_LOCK_FLAG_CLONE_NAME;

	T_SCHEDULING_ANALYSIS(InitRWLock(lock, name));
	NotifyWaitObjectListeners(&WaitObjectListener::RWLockInitialized, lock);
}


void
rw_lock_destroy(rw_lock* lock)
{
	char* name = (lock->flags & RW_LOCK_FLAG_CLONE_NAME) != 0
		? (char*)lock->name : NULL;

	// unblock all waiters
	InterruptsSpinLocker locker(gSchedulerLock);

#if KDEBUG
	if (lock->waiters != NULL && thread_get_current_thread_id()
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
		thread_unblock_locked(waiter->thread, B_ERROR);
	}

	lock->name = NULL;

	locker.Unlock();

	free(name);
}


#if !KDEBUG_RW_LOCK_DEBUG

status_t
_rw_lock_read_lock(rw_lock* lock)
{
	InterruptsSpinLocker locker(gSchedulerLock);

	// We might be the writer ourselves.
	if (lock->holder == thread_get_current_thread_id()) {
		lock->owner_count++;
		return B_OK;
	}

	// The writer that originally had the lock when we called atomic_add() might
	// already have gone and another writer could have overtaken us. In this
	// case the original writer set pending_readers, so we know that we don't
	// have to wait.
	if (lock->pending_readers > 0) {
		lock->pending_readers--;

		if (lock->count >= RW_LOCK_WRITER_COUNT_BASE)
			lock->active_readers++;

		return B_OK;
	}

	ASSERT(lock->count >= RW_LOCK_WRITER_COUNT_BASE);

	// we need to wait
	return rw_lock_wait(lock, false);
}


status_t
_rw_lock_read_lock_with_timeout(rw_lock* lock, uint32 timeoutFlags,
	bigtime_t timeout)
{
	InterruptsSpinLocker locker(gSchedulerLock);

	// We might be the writer ourselves.
	if (lock->holder == thread_get_current_thread_id()) {
		lock->owner_count++;
		return B_OK;
	}

	// The writer that originally had the lock when we called atomic_add() might
	// already have gone and another writer could have overtaken us. In this
	// case the original writer set pending_readers, so we know that we don't
	// have to wait.
	if (lock->pending_readers > 0) {
		lock->pending_readers--;

		if (lock->count >= RW_LOCK_WRITER_COUNT_BASE)
			lock->active_readers++;

		return B_OK;
	}

	ASSERT(lock->count >= RW_LOCK_WRITER_COUNT_BASE);

	// we need to wait

	// enqueue in waiter list
	rw_lock_waiter waiter;
	waiter.thread = thread_get_current_thread();
	waiter.next = NULL;
	waiter.writer = false;

	if (lock->waiters != NULL)
		lock->waiters->last->next = &waiter;
	else
		lock->waiters = &waiter;

	lock->waiters->last = &waiter;

	// block
	thread_prepare_to_block(waiter.thread, 0, THREAD_BLOCK_TYPE_RW_LOCK, lock);
	status_t error = thread_block_with_timeout_locked(timeoutFlags, timeout);
	if (error == B_OK || waiter.thread == NULL) {
		// We were unblocked successfully -- potentially our unblocker overtook
		// us after we already failed. In either case, we've got the lock, now.
		return B_OK;
	}

	// We failed to get the lock -- dequeue from waiter list.
	rw_lock_waiter* previous = NULL;
	rw_lock_waiter* other = lock->waiters;
	while (other != &waiter) {
		previous = other;
		other = other->next;
	}

	if (previous == NULL) {
		// we are the first in line
		lock->waiters = waiter.next;
		if (lock->waiters != NULL)
			lock->waiters->last = waiter.last;
	} else {
		// one or more other waiters are before us in the queue
		previous->next = waiter.next;
		if (lock->waiters->last == &waiter)
			lock->waiters->last = previous;
	}

	// Decrement the count. ATM this is all we have to do. There's at least
	// one writer ahead of us -- otherwise the last writer would have unblocked
	// us (writers only manipulate the lock data with thread spinlock being
	// held) -- so our leaving doesn't make a difference to the ones behind us
	// in the queue.
	atomic_add(&lock->count, -1);

	return error;
}


void
_rw_lock_read_unlock(rw_lock* lock, bool schedulerLocked)
{
	InterruptsSpinLocker locker(gSchedulerLock, false, !schedulerLocked);

	// If we're still holding the write lock or if there are other readers,
	// no-one can be woken up.
	if (lock->holder == thread_get_current_thread_id()) {
		ASSERT(lock->owner_count % RW_LOCK_WRITER_COUNT_BASE > 0);
		lock->owner_count--;
		return;
	}

	if (--lock->active_readers > 0)
		return;

 	if (lock->active_readers < 0) {
 		panic("rw_lock_read_unlock(): lock %p not read-locked", lock);
		lock->active_readers = 0;
 		return;
 	}

	rw_lock_unblock(lock);
}

#endif	// !KDEBUG_RW_LOCK_DEBUG


status_t
rw_lock_write_lock(rw_lock* lock)
{
	InterruptsSpinLocker locker(gSchedulerLock);

	// If we're already the lock holder, we just need to increment the owner
	// count.
	thread_id thread = thread_get_current_thread_id();
	if (lock->holder == thread) {
		lock->owner_count += RW_LOCK_WRITER_COUNT_BASE;
		return B_OK;
	}

	// announce our claim
	int32 oldCount = atomic_add(&lock->count, RW_LOCK_WRITER_COUNT_BASE);

	if (oldCount == 0) {
		// No-one else held a read or write lock, so it's ours now.
		lock->holder = thread;
		lock->owner_count = RW_LOCK_WRITER_COUNT_BASE;
		return B_OK;
	}

	// We have to wait. If we're the first writer, note the current reader
	// count.
	if (oldCount < RW_LOCK_WRITER_COUNT_BASE)
		lock->active_readers = oldCount - lock->pending_readers;

	status_t status = rw_lock_wait(lock, true);
	if (status == B_OK) {
		lock->holder = thread;
		lock->owner_count = RW_LOCK_WRITER_COUNT_BASE;
	}

	return status;
}


void
_rw_lock_write_unlock(rw_lock* lock, bool schedulerLocked)
{
	InterruptsSpinLocker locker(gSchedulerLock, false, !schedulerLocked);

	if (thread_get_current_thread_id() != lock->holder) {
		panic("rw_lock_write_unlock(): lock %p not write-locked by this thread",
			lock);
		return;
	}

	ASSERT(lock->owner_count >= RW_LOCK_WRITER_COUNT_BASE);

	lock->owner_count -= RW_LOCK_WRITER_COUNT_BASE;
	if (lock->owner_count >= RW_LOCK_WRITER_COUNT_BASE)
		return;

	// We gave up our last write lock -- clean up and unblock waiters.
	int32 readerCount = lock->owner_count;
	lock->holder = -1;
	lock->owner_count = 0;

	int32 oldCount = atomic_add(&lock->count, -RW_LOCK_WRITER_COUNT_BASE);
	oldCount -= RW_LOCK_WRITER_COUNT_BASE;

	if (oldCount != 0) {
		// If writers are waiting, take over our reader count.
		if (oldCount >= RW_LOCK_WRITER_COUNT_BASE) {
			lock->active_readers = readerCount;
			rw_lock_unblock(lock);
		} else {
			// No waiting writer, but there are one or more readers. We will
			// unblock all waiting readers -- that's the easy part -- and must
			// also make sure that all readers that haven't entered the critical
			// section yet, won't start to wait. Otherwise a writer overtaking
			// such a reader will correctly start to wait, but the reader,
			// seeing the writer count > 0, would also start to wait. We set
			// pending_readers to the number of readers that are still expected
			// to enter the critical section.
			lock->pending_readers = oldCount - readerCount
				- rw_lock_unblock(lock);
		}
	}
}


static int
dump_rw_lock_info(int argc, char** argv)
{
	if (argc < 2) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	rw_lock* lock = (rw_lock*)parse_expression(argv[1]);

	if (!IS_KERNEL_ADDRESS(lock)) {
		kprintf("invalid address: %p\n", lock);
		return 0;
	}

	kprintf("rw lock %p:\n", lock);
	kprintf("  name:            %s\n", lock->name);
	kprintf("  holder:          %ld\n", lock->holder);
	kprintf("  count:           %#lx\n", lock->count);
	kprintf("  active readers   %d\n", lock->active_readers);
	kprintf("  pending readers  %d\n", lock->pending_readers);
	kprintf("  owner count:     %#lx\n", lock->owner_count);
	kprintf("  flags:           %#lx\n", lock->flags);

	kprintf("  waiting threads:");
	rw_lock_waiter* waiter = lock->waiters;
	while (waiter != NULL) {
		kprintf(" %ld/%c", waiter->thread->id, waiter->writer ? 'w' : 'r');
		waiter = waiter->next;
	}
	kputs("\n");

	return 0;
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
	lock->ignore_unlock_count = 0;
#endif
	lock->flags = 0;

	T_SCHEDULING_ANALYSIS(InitMutex(lock, name));
	NotifyWaitObjectListeners(&WaitObjectListener::MutexInitialized, lock);
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
	lock->ignore_unlock_count = 0;
#endif
	lock->flags = flags & MUTEX_FLAG_CLONE_NAME;

	T_SCHEDULING_ANALYSIS(InitMutex(lock, name));
	NotifyWaitObjectListeners(&WaitObjectListener::MutexInitialized, lock);
}


void
mutex_destroy(mutex* lock)
{
	char* name = (lock->flags & MUTEX_FLAG_CLONE_NAME) != 0
		? (char*)lock->name : NULL;

	// unblock all waiters
	InterruptsSpinLocker locker(gSchedulerLock);

#if KDEBUG
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
mutex_switch_lock(mutex* from, mutex* to)
{
	InterruptsSpinLocker locker(gSchedulerLock);

#if !KDEBUG
	if (atomic_add(&from->count, 1) < -1)
#endif
		_mutex_unlock(from, true);

	return mutex_lock_threads_locked(to);
}


status_t
mutex_switch_from_read_lock(rw_lock* from, mutex* to)
{
	InterruptsSpinLocker locker(gSchedulerLock);

#if KDEBUG_RW_LOCK_DEBUG
	_rw_lock_write_unlock(from, true);
#else
	int32 oldCount = atomic_add(&from->count, -1);
	if (oldCount >= RW_LOCK_WRITER_COUNT_BASE)
		_rw_lock_read_unlock(from, true);
#endif

	return mutex_lock_threads_locked(to);
}


status_t
_mutex_lock(mutex* lock, bool schedulerLocked)
{
#if KDEBUG
	if (!gKernelStartup && !schedulerLocked && !are_interrupts_enabled()) {
		panic("_mutex_lock(): called with interrupts disabled for lock %p",
			lock);
	}
#endif

	// lock only, if !threadsLocked
	InterruptsSpinLocker locker(gSchedulerLock, false, !schedulerLocked);

	// Might have been released after we decremented the count, but before
	// we acquired the spinlock.
#if KDEBUG
	if (lock->holder < 0) {
		lock->holder = thread_get_current_thread_id();
		return B_OK;
	} else if (lock->holder == thread_get_current_thread_id()) {
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

#if KDEBUG
	if (error == B_OK)
		lock->holder = waiter.thread->id;
#endif

	return error;
}


void
_mutex_unlock(mutex* lock, bool schedulerLocked)
{
	// lock only, if !threadsLocked
	InterruptsSpinLocker locker(gSchedulerLock, false, !schedulerLocked);

#if KDEBUG
	if (thread_get_current_thread_id() != lock->holder) {
		panic("_mutex_unlock() failure: thread %ld is trying to release "
			"mutex %p (current holder %ld)\n", thread_get_current_thread_id(),
			lock, lock->holder);
		return;
	}
#else
	if (lock->ignore_unlock_count > 0) {
		lock->ignore_unlock_count--;
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

#if KDEBUG
		// Already set the holder to the unblocked thread. Besides that this
		// actually reflects the current situation, setting it to -1 would
		// cause a race condition, since another locker could think the lock
		// is not held by anyone.
		lock->holder = waiter->thread->id;
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
	InterruptsSpinLocker _(gSchedulerLock);

	if (lock->holder <= 0) {
		lock->holder = thread_get_current_thread_id();
		return B_OK;
	}
#endif
	return B_WOULD_BLOCK;
}


status_t
_mutex_lock_with_timeout(mutex* lock, uint32 timeoutFlags, bigtime_t timeout)
{
#if KDEBUG
	if (!gKernelStartup && !are_interrupts_enabled()) {
		panic("_mutex_lock(): called with interrupts disabled for lock %p",
			lock);
	}
#endif

	InterruptsSpinLocker locker(gSchedulerLock);

	// Might have been released after we decremented the count, but before
	// we acquired the spinlock.
#if KDEBUG
	if (lock->holder < 0) {
		lock->holder = thread_get_current_thread_id();
		return B_OK;
	} else if (lock->holder == thread_get_current_thread_id()) {
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
	waiter.thread = thread_get_current_thread();
	waiter.next = NULL;

	if (lock->waiters != NULL) {
		lock->waiters->last->next = &waiter;
	} else
		lock->waiters = &waiter;

	lock->waiters->last = &waiter;

	// block
	thread_prepare_to_block(waiter.thread, 0, THREAD_BLOCK_TYPE_MUTEX, lock);
	status_t error = thread_block_with_timeout_locked(timeoutFlags, timeout);

	if (error == B_OK) {
#if KDEBUG
		lock->holder = waiter.thread->id;
#endif
	} else {
		// If the timeout occurred, we must remove our waiter structure from
		// the queue.
		mutex_waiter* previousWaiter = NULL;
		mutex_waiter* otherWaiter = lock->waiters;
		while (otherWaiter != NULL && otherWaiter != &waiter) {
			previousWaiter = otherWaiter;
			otherWaiter = otherWaiter->next;
		}
		if (otherWaiter == &waiter) {
			// the structure is still in the list -- dequeue
			if (&waiter == lock->waiters) {
				if (waiter.next != NULL)
					waiter.next->last = waiter.last;
				lock->waiters = waiter.next;
			} else {
				if (waiter.next == NULL)
					lock->waiters->last = previousWaiter;
				previousWaiter->next = waiter.next;
			}

#if !KDEBUG
			// we need to fix the lock count
			if (atomic_add(&lock->count, 1) == -1) {
				// This means we were the only thread waiting for the lock and
				// the lock owner has already called atomic_add() in
				// mutex_unlock(). That is we probably would get the lock very
				// soon (if the lock holder has a low priority, that might
				// actually take rather long, though), but the timeout already
				// occurred, so we don't try to wait. Just increment the ignore
				// unlock count.
				lock->ignore_unlock_count++;
			}
#endif
		}
	}

	return error;
}


static int
dump_mutex_info(int argc, char** argv)
{
	if (argc < 2) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	mutex* lock = (mutex*)parse_expression(argv[1]);

	if (!IS_KERNEL_ADDRESS(lock)) {
		kprintf("invalid address: %p\n", lock);
		return 0;
	}

	kprintf("mutex %p:\n", lock);
	kprintf("  name:            %s\n", lock->name);
	kprintf("  flags:           0x%x\n", lock->flags);
#if KDEBUG
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
	add_debugger_command_etc("rwlock", &dump_rw_lock_info,
		"Dump info about an rw lock",
		"<lock>\n"
		"Prints info about the specified rw lock.\n"
		"  <lock>  - pointer to the rw lock to print the info for.\n", 0);
}
