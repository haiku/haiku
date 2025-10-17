/*
 * Copyright 2008-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2009, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <debug.h>

#if KDEBUG
#define KDEBUG_STATIC static
static status_t _mutex_lock(struct mutex* lock, void* locker);
static void _mutex_unlock(struct mutex* lock);
#else
#define KDEBUG_STATIC
#define mutex_lock		mutex_lock_inline
#define mutex_unlock	mutex_unlock_inline
#define mutex_trylock	mutex_trylock_inline
#define mutex_lock_with_timeout	mutex_lock_with_timeout_inline
#endif

#include <lock.h>

#include <stdlib.h>
#include <string.h>

#include <interrupts.h>
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

#define MUTEX_FLAG_RELEASED		0x2


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
	recursive_lock_init_etc(lock, name, 0);
}


void
recursive_lock_init_etc(recursive_lock *lock, const char *name, uint32 flags)
{
	mutex_init_etc(&lock->lock, name != NULL ? name : "recursive lock", flags);
#if !KDEBUG
	lock->holder = -1;
#endif
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
#if KDEBUG
	if (!gKernelStartup && !are_interrupts_enabled()) {
		panic("recursive_lock_lock: called with interrupts disabled for lock "
			"%p (\"%s\")\n", lock, lock->lock.name);
	}
#endif

	thread_id thread = thread_get_current_thread_id();

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

#if KDEBUG
	if (!gKernelStartup && !are_interrupts_enabled()) {
		panic("recursive_lock_lock: called with interrupts disabled for lock "
			"%p (\"%s\")\n", lock, lock->lock.name);
	}
#endif

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


status_t
recursive_lock_switch_lock(recursive_lock* from, recursive_lock* to)
{
#if KDEBUG
	if (!gKernelStartup && !are_interrupts_enabled()) {
		panic("recursive_lock_switch_lock(): called with interrupts "
			"disabled for locks %p, %p", from, to);
	}
#endif

	if (--from->recursion > 0)
		return recursive_lock_lock(to);

#if !KDEBUG
	from->holder = -1;
#endif

	thread_id thread = thread_get_current_thread_id();

	if (thread == RECURSIVE_LOCK_HOLDER(to)) {
		to->recursion++;
		mutex_unlock(&from->lock);
		return B_OK;
	}

	status_t status = mutex_switch_lock(&from->lock, &to->lock);
	if (status != B_OK) {
		from->recursion++;
#if !KDEBUG
		from->holder = thread;
#endif
		return status;
	}

#if !KDEBUG
	to->holder = thread;
#endif
	to->recursion++;
	return B_OK;
}


status_t
recursive_lock_switch_from_mutex(mutex* from, recursive_lock* to)
{
#if KDEBUG
	if (!gKernelStartup && !are_interrupts_enabled()) {
		panic("recursive_lock_switch_from_mutex(): called with interrupts "
			"disabled for locks %p, %p", from, to);
	}
#endif

	thread_id thread = thread_get_current_thread_id();

	if (thread == RECURSIVE_LOCK_HOLDER(to)) {
		to->recursion++;
		mutex_unlock(from);
		return B_OK;
	}

	status_t status = mutex_switch_lock(from, &to->lock);
	if (status != B_OK)
		return status;

#if !KDEBUG
	to->holder = thread;
#endif
	to->recursion++;
	return B_OK;
}


status_t
recursive_lock_switch_from_read_lock(rw_lock* from, recursive_lock* to)
{
#if KDEBUG
	if (!gKernelStartup && !are_interrupts_enabled()) {
		panic("recursive_lock_switch_from_read_lock(): called with interrupts "
			"disabled for locks %p, %p", from, to);
	}
#endif

	thread_id thread = thread_get_current_thread_id();

	if (thread != RECURSIVE_LOCK_HOLDER(to)) {
		status_t status = mutex_switch_from_read_lock(from, &to->lock);
		if (status != B_OK)
			return status;

#if !KDEBUG
		to->holder = thread;
#endif
	} else {
		rw_lock_read_unlock(from);
	}

	to->recursion++;
	return B_OK;
}


static int
dump_recursive_lock_info(int argc, char** argv)
{
	if (argc < 2) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	recursive_lock* lock = (recursive_lock*)parse_expression(argv[1]);

	if (!IS_KERNEL_ADDRESS(lock)) {
		kprintf("invalid address: %p\n", lock);
		return 0;
	}

	kprintf("recrusive_lock %p:\n", lock);
	kprintf("  mutex:           %p\n", &lock->lock);
	kprintf("  name:            %s\n", lock->lock.name);
	kprintf("  flags:           0x%x\n", lock->lock.flags);
#if KDEBUG
	kprintf("  holder:          %" B_PRId32 "\n", lock->lock.holder);
#else
	kprintf("  holder:          %" B_PRId32 "\n", lock->holder);
#endif
	kprintf("  recursion:       %d\n", lock->recursion);

	kprintf("  waiting threads:");
	mutex_waiter* waiter = lock->lock.waiters;
	while (waiter != NULL) {
		kprintf(" %" B_PRId32, waiter->thread->id);
		waiter = waiter->next;
	}
	kputs("\n");

	return 0;
}


//	#pragma mark -


static status_t
rw_lock_wait(rw_lock* lock, bool writer, InterruptsSpinLocker& locker)
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
	locker.Unlock();

	status_t result = thread_block();

	locker.Lock();
	ASSERT(result != B_OK || waiter.thread == NULL);
	return result;
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
		thread_unblock(waiter->thread, B_OK);
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
		thread_unblock(waiter->thread, B_OK);
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
	B_INITIALIZE_SPINLOCK(&lock->lock);
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
	B_INITIALIZE_SPINLOCK(&lock->lock);
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
	InterruptsSpinLocker locker(lock->lock);

#if KDEBUG
	if ((atomic_get(&lock->count) != 0 || lock->waiters != NULL)
			&& thread_get_current_thread_id() != lock->holder) {
		panic("rw_lock_destroy(): lock is in use and the caller "
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
		thread_unblock(waiter->thread, B_ERROR);
	}

	lock->name = NULL;

	locker.Unlock();

	free(name);
}


#if KDEBUG_RW_LOCK_DEBUG

bool
_rw_lock_is_read_locked(rw_lock* lock)
{
	if (lock->holder == thread_get_current_thread_id())
		return true;

	Thread* thread = thread_get_current_thread();
	for (size_t i = 0; i < B_COUNT_OF(Thread::held_read_locks); i++) {
		if (thread->held_read_locks[i] == lock)
			return true;
	}
	return false;
}


static void
_rw_lock_set_read_locked(rw_lock* lock)
{
	Thread* thread = thread_get_current_thread();
	for (size_t i = 0; i < B_COUNT_OF(Thread::held_read_locks); i++) {
		if (thread->held_read_locks[i] != NULL)
			continue;

		thread->held_read_locks[i] = lock;
		return;
	}

	panic("too many read locks!");
}


static void
_rw_lock_unset_read_locked(rw_lock* lock)
{
	Thread* thread = thread_get_current_thread();
	for (size_t i = 0; i < B_COUNT_OF(Thread::held_read_locks); i++) {
		if (thread->held_read_locks[i] != lock)
			continue;

		thread->held_read_locks[i] = NULL;
		return;
	}

	panic("_rw_lock_unset_read_locked(): lock %p not read-locked by current thread", lock);
}

#endif


status_t
_rw_lock_read_lock(rw_lock* lock)
{
#if KDEBUG
	if (!gKernelStartup && !are_interrupts_enabled()) {
		panic("_rw_lock_read_lock(): called with interrupts disabled for lock %p",
			lock);
	}
#endif
#if KDEBUG_RW_LOCK_DEBUG
	int32 oldCount = atomic_add(&lock->count, 1);
	if (oldCount < RW_LOCK_WRITER_COUNT_BASE) {
		ASSERT_UNLOCKED_RW_LOCK(lock);
		_rw_lock_set_read_locked(lock);
		return B_OK;
	}
#endif

	InterruptsSpinLocker locker(lock->lock);

	// We might be the writer ourselves.
	if (lock->holder == thread_get_current_thread_id()) {
		lock->owner_count++;
		return B_OK;
	}

	// If we hold a read lock already, but some other thread is waiting
	// for a write lock, then trying to read-lock again will deadlock.
	ASSERT_UNLOCKED_RW_LOCK(lock);

	// The writer that originally had the lock when we called atomic_add() might
	// already have gone and another writer could have overtaken us. In this
	// case the original writer set pending_readers, so we know that we don't
	// have to wait.
	if (lock->pending_readers > 0) {
		lock->pending_readers--;

		if (lock->count >= RW_LOCK_WRITER_COUNT_BASE)
			lock->active_readers++;

#if KDEBUG_RW_LOCK_DEBUG
		_rw_lock_set_read_locked(lock);
#endif
		return B_OK;
	}

	ASSERT(lock->count >= RW_LOCK_WRITER_COUNT_BASE);

	// we need to wait
	status_t status = rw_lock_wait(lock, false, locker);

#if KDEBUG_RW_LOCK_DEBUG
	if (status == B_OK)
		_rw_lock_set_read_locked(lock);
#endif

	return status;
}


status_t
_rw_lock_read_lock_with_timeout(rw_lock* lock, uint32 timeoutFlags,
	bigtime_t timeout)
{
#if KDEBUG
	if (!gKernelStartup && !are_interrupts_enabled()) {
		panic("_rw_lock_read_lock_with_timeout(): called with interrupts "
			"disabled for lock %p", lock);
	}
#endif
#if KDEBUG_RW_LOCK_DEBUG
	int32 oldCount = atomic_add(&lock->count, 1);
	if (oldCount < RW_LOCK_WRITER_COUNT_BASE) {
		ASSERT_UNLOCKED_RW_LOCK(lock);
		_rw_lock_set_read_locked(lock);
		return B_OK;
	}
#endif

	InterruptsSpinLocker locker(lock->lock);

	// We might be the writer ourselves.
	if (lock->holder == thread_get_current_thread_id()) {
		lock->owner_count++;
		return B_OK;
	}

	ASSERT_UNLOCKED_RW_LOCK(lock);

	// The writer that originally had the lock when we called atomic_add() might
	// already have gone and another writer could have overtaken us. In this
	// case the original writer set pending_readers, so we know that we don't
	// have to wait.
	if (lock->pending_readers > 0) {
		lock->pending_readers--;

		if (lock->count >= RW_LOCK_WRITER_COUNT_BASE)
			lock->active_readers++;

#if KDEBUG_RW_LOCK_DEBUG
		_rw_lock_set_read_locked(lock);
#endif
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
	locker.Unlock();

	status_t error = thread_block_with_timeout(timeoutFlags, timeout);
	if (error == B_OK || waiter.thread == NULL) {
		// We were unblocked successfully -- potentially our unblocker overtook
		// us after we already failed. In either case, we've got the lock, now.
#if KDEBUG_RW_LOCK_DEBUG
		_rw_lock_set_read_locked(lock);
#endif
		return B_OK;
	}

	locker.Lock();
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
_rw_lock_read_unlock(rw_lock* lock)
{
#if KDEBUG_RW_LOCK_DEBUG
	int32 oldCount = atomic_add(&lock->count, -1);
	if (oldCount < RW_LOCK_WRITER_COUNT_BASE) {
		_rw_lock_unset_read_locked(lock);
		return;
	}
#endif

	InterruptsSpinLocker locker(lock->lock);

	// If we're still holding the write lock or if there are other readers,
	// no-one can be woken up.
	if (lock->holder == thread_get_current_thread_id()) {
		ASSERT((lock->owner_count % RW_LOCK_WRITER_COUNT_BASE) > 0);
		lock->owner_count--;
		return;
	}

#if KDEBUG_RW_LOCK_DEBUG
	_rw_lock_unset_read_locked(lock);
#endif

	if (--lock->active_readers > 0)
		return;

	if (lock->active_readers < 0) {
		panic("rw_lock_read_unlock(): lock %p not read-locked", lock);
		lock->active_readers = 0;
		return;
	}

	rw_lock_unblock(lock);
}


status_t
rw_lock_write_lock(rw_lock* lock)
{
#if KDEBUG
	if (!gKernelStartup && !are_interrupts_enabled()) {
		panic("_rw_lock_write_lock(): called with interrupts disabled for lock %p",
			lock);
	}
#endif

	InterruptsSpinLocker locker(lock->lock);

	// If we're already the lock holder, we just need to increment the owner
	// count.
	thread_id thread = thread_get_current_thread_id();
	if (lock->holder == thread) {
		lock->owner_count += RW_LOCK_WRITER_COUNT_BASE;
		return B_OK;
	}

	ASSERT_UNLOCKED_RW_LOCK(lock);

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

	status_t status = rw_lock_wait(lock, true, locker);
	if (status == B_OK) {
		lock->holder = thread;
		lock->owner_count = RW_LOCK_WRITER_COUNT_BASE;
	}

	return status;
}


void
_rw_lock_write_unlock(rw_lock* lock)
{
	InterruptsSpinLocker locker(lock->lock);

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

#if KDEBUG_RW_LOCK_DEBUG
	if (readerCount != 0)
		_rw_lock_set_read_locked(lock);
#endif

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
	kprintf("  holder:          %" B_PRId32 "\n", lock->holder);
	kprintf("  count:           %#" B_PRIx32 "\n", lock->count);
	kprintf("  active readers   %d\n", lock->active_readers);
	kprintf("  pending readers  %d\n", lock->pending_readers);
	kprintf("  owner count:     %#" B_PRIx32 "\n", lock->owner_count);
	kprintf("  flags:           %#" B_PRIx32 "\n", lock->flags);

#if KDEBUG_RW_LOCK_DEBUG
	kprintf("  reader threads:");
	if (lock->active_readers > 0) {
		ThreadListIterator iterator;
		while (Thread* thread = iterator.Next()) {
			for (size_t i = 0; i < B_COUNT_OF(Thread::held_read_locks); i++) {
				if (thread->held_read_locks[i] == lock) {
					kprintf(" %" B_PRId32, thread->id);
					break;
				}
			}
		}
	}
	kprintf("\n");
#endif

	kprintf("  waiting threads:");
	rw_lock_waiter* waiter = lock->waiters;
	while (waiter != NULL) {
		kprintf(" %" B_PRId32 "/%c", waiter->thread->id, waiter->writer ? 'w' : 'r');
		waiter = waiter->next;
	}
	kputs("\n");

	return 0;
}


// #pragma mark -


void
mutex_init(mutex* lock, const char *name)
{
	mutex_init_etc(lock, name, 0);
}


void
mutex_init_etc(mutex* lock, const char *name, uint32 flags)
{
	lock->name = (flags & MUTEX_FLAG_CLONE_NAME) != 0 ? strdup(name) : name;
	lock->waiters = NULL;
	B_INITIALIZE_SPINLOCK(&lock->lock);
#if KDEBUG
	lock->holder = -1;
#else
	lock->count = 0;
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
	InterruptsSpinLocker locker(lock->lock);

#if KDEBUG
	if (lock->holder != -1 && thread_get_current_thread_id() != lock->holder) {
		panic("mutex_destroy(): the lock (%p) is held by %" B_PRId32 ", not "
			"by the caller @! bt %" B_PRId32, lock, lock->holder, lock->holder);
		if (_mutex_lock(lock, &locker) != B_OK)
			return;
		locker.Lock();
	}
#endif

	while (mutex_waiter* waiter = lock->waiters) {
		// dequeue
		lock->waiters = waiter->next;

		// unblock thread
		Thread* thread = waiter->thread;
		waiter->thread = NULL;
		thread_unblock(thread, B_ERROR);
	}

	lock->name = NULL;
	lock->flags = 0;
#if KDEBUG
	lock->holder = 0;
#else
	lock->count = INT16_MIN;
#endif

	locker.Unlock();

	free(name);
}


static inline status_t
mutex_lock_threads_locked(mutex* lock, InterruptsSpinLocker* locker)
{
#if KDEBUG
	return _mutex_lock(lock, locker);
#else
	if (atomic_add(&lock->count, -1) < 0)
		return _mutex_lock(lock, locker);
	return B_OK;
#endif
}


status_t
mutex_switch_lock(mutex* from, mutex* to)
{
#if KDEBUG
	if (!gKernelStartup && !are_interrupts_enabled()) {
		panic("mutex_switch_lock(): called with interrupts disabled "
			"for locks %p, %p", from, to);
	}
#endif

	InterruptsSpinLocker locker(to->lock);

	mutex_unlock(from);

	return mutex_lock_threads_locked(to, &locker);
}


void
mutex_transfer_lock(mutex* lock, thread_id thread)
{
#if KDEBUG
	if (thread_get_current_thread_id() != lock->holder)
		panic("mutex_transfer_lock(): current thread is not the lock holder!");
	lock->holder = thread;
#endif
}


status_t
mutex_switch_from_read_lock(rw_lock* from, mutex* to)
{
#if KDEBUG
	if (!gKernelStartup && !are_interrupts_enabled()) {
		panic("mutex_switch_from_read_lock(): called with interrupts disabled "
			"for locks %p, %p", from, to);
	}
#endif

	InterruptsSpinLocker locker(to->lock);

	rw_lock_read_unlock(from);

	return mutex_lock_threads_locked(to, &locker);
}


KDEBUG_STATIC status_t
_mutex_lock(mutex* lock, void* _locker)
{
#if KDEBUG
	if (!gKernelStartup && _locker == NULL && !are_interrupts_enabled()) {
		panic("_mutex_lock(): called with interrupts disabled for lock %p",
			lock);
	}
#endif

	// lock only, if !lockLocked
	InterruptsSpinLocker* locker
		= reinterpret_cast<InterruptsSpinLocker*>(_locker);

	InterruptsSpinLocker lockLocker;
	if (locker == NULL) {
		lockLocker.SetTo(lock->lock, false);
		locker = &lockLocker;
	}

	// Might have been released after we decremented the count, but before
	// we acquired the spinlock.
#if KDEBUG
	if (lock->holder < 0) {
		lock->holder = thread_get_current_thread_id();
		return B_OK;
	} else if (lock->holder == thread_get_current_thread_id()) {
		panic("_mutex_lock(): double lock of %p by thread %" B_PRId32, lock,
			lock->holder);
	} else if (lock->holder == 0) {
		panic("_mutex_lock(): using uninitialized lock %p", lock);
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
	locker->Unlock();

	status_t error = thread_block();
#if KDEBUG
	if (error == B_OK) {
		ASSERT(lock->holder == waiter.thread->id);
	} else {
		// This should only happen when the mutex was destroyed.
		ASSERT(waiter.thread == NULL);
	}
#endif
	return error;
}


KDEBUG_STATIC void
_mutex_unlock(mutex* lock)
{
	InterruptsSpinLocker locker(lock->lock);

#if KDEBUG
	if (thread_get_current_thread_id() != lock->holder) {
		panic("_mutex_unlock() failure: thread %" B_PRId32 " is trying to "
			"release mutex %p (current holder %" B_PRId32 ")\n",
			thread_get_current_thread_id(), lock, lock->holder);
		return;
	}
#endif

	mutex_waiter* waiter = lock->waiters;
	if (waiter != NULL) {
		// dequeue the first waiter
		lock->waiters = waiter->next;
		if (lock->waiters != NULL)
			lock->waiters->last = waiter->last;

#if KDEBUG
		// Already set the holder to the unblocked thread. Besides that this
		// actually reflects the current situation, setting it to -1 would
		// cause a race condition, since another locker could think the lock
		// is not held by anyone.
		lock->holder = waiter->thread->id;
#endif

		// unblock thread
		thread_unblock(waiter->thread, B_OK);
	} else {
		// There are no waiters, so mark the lock as released.
#if KDEBUG
		lock->holder = -1;
#else
		lock->flags |= MUTEX_FLAG_RELEASED;
#endif
	}
}


KDEBUG_STATIC status_t
_mutex_lock_with_timeout(mutex* lock, uint32 timeoutFlags, bigtime_t timeout)
{
#if KDEBUG
	if (!gKernelStartup && !are_interrupts_enabled()) {
		panic("_mutex_lock(): called with interrupts disabled for lock %p",
			lock);
	}
#endif

	InterruptsSpinLocker locker(lock->lock);

	// Might have been released after we decremented the count, but before
	// we acquired the spinlock.
#if KDEBUG
	if (lock->holder < 0) {
		lock->holder = thread_get_current_thread_id();
		return B_OK;
	} else if (lock->holder == thread_get_current_thread_id()) {
		panic("_mutex_lock(): double lock of %p by thread %" B_PRId32, lock,
			lock->holder);
	} else if (lock->holder == 0) {
		panic("_mutex_lock(): using uninitialized lock %p", lock);
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
	locker.Unlock();

	status_t error = thread_block_with_timeout(timeoutFlags, timeout);

	if (error == B_OK) {
#if KDEBUG
		ASSERT(lock->holder == waiter.thread->id);
#endif
	} else {
		// If the lock was destroyed, our "thread" entry will be NULL.
		if (waiter.thread == NULL)
			return B_ERROR;

		// TODO: There is still a race condition during mutex destruction,
		// if we resume due to a timeout before our thread is set to NULL.

		locker.Lock();

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
			atomic_add(&lock->count, 1);
#endif
		} else {
			// the structure is not in the list -- even though the timeout
			// occurred, this means we own the lock now
#if KDEBUG
			ASSERT(lock->holder == waiter.thread->id);
#endif
			return B_OK;
		}
	}

	return error;
}


#undef mutex_trylock
status_t
mutex_trylock(mutex* lock)
{
#if KDEBUG
	InterruptsSpinLocker _(lock->lock);

	if (lock->holder < 0) {
		lock->holder = thread_get_current_thread_id();
		return B_OK;
	} else if (lock->holder == 0) {
		panic("_mutex_trylock(): using uninitialized lock %p", lock);
	}
	return B_WOULD_BLOCK;
#else
	return mutex_trylock_inline(lock);
#endif
}


#undef mutex_lock
status_t
mutex_lock(mutex* lock)
{
#if KDEBUG
	return _mutex_lock(lock, NULL);
#else
	return mutex_lock_inline(lock);
#endif
}


#undef mutex_unlock
void
mutex_unlock(mutex* lock)
{
#if KDEBUG
	_mutex_unlock(lock);
#else
	mutex_unlock_inline(lock);
#endif
}


#undef mutex_lock_with_timeout
status_t
mutex_lock_with_timeout(mutex* lock, uint32 timeoutFlags, bigtime_t timeout)
{
#if KDEBUG
	return _mutex_lock_with_timeout(lock, timeoutFlags, timeout);
#else
	return mutex_lock_with_timeout_inline(lock, timeoutFlags, timeout);
#endif
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
	kprintf("  holder:          %" B_PRId32 "\n", lock->holder);
#else
	kprintf("  count:           %" B_PRId32 "\n", lock->count);
#endif

	kprintf("  waiting threads:");
	mutex_waiter* waiter = lock->waiters;
	while (waiter != NULL) {
		kprintf(" %" B_PRId32, waiter->thread->id);
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
	add_debugger_command_etc("recursivelock", &dump_recursive_lock_info,
		"Dump info about a recursive lock",
		"<lock>\n"
		"Prints info about the specified recursive lock.\n"
		"  <lock>  - pointer to the recursive lock to print the info for.\n",
		0);
}
