/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_LOCK_H
#define _KERNEL_LOCK_H

#include <OS.h>
#include <debug.h>


struct mutex_waiter;

typedef struct mutex {
	const char*				name;
	struct mutex_waiter*	waiters;
#ifdef KDEBUG
	thread_id				holder;
#else
	int32					count;
#endif
	uint8					flags;
} mutex;

#define MUTEX_FLAG_CLONE_NAME	0x1


typedef struct recursive_lock {
	mutex		lock;
#ifndef KDEBUG
	thread_id	holder;
#endif
	int			recursion;
} recursive_lock;


typedef struct benaphore {
	sem_id	sem;
	int32	count;
} benaphore;

// Note: this is currently a trivial r/w lock implementation
//	it will be replaced with something better later - this
//	or a similar API will be made publically available at this point.
typedef struct rw_lock {
	sem_id		sem;
	int32		count;
	benaphore	writeLock;
} rw_lock;

#define RW_MAX_READERS 1000000

#if 0 && KDEBUG // XXX disable this for now, it causes problems when including thread.h here
#	include <thread.h>
#define ASSERT_LOCKED_RECURSIVE(r) { ASSERT(thread_get_current_thread_id() == (r)->holder); }
#define ASSERT_LOCKED_MUTEX(m) { ASSERT(thread_get_current_thread_id() == (m)->holder); }
#else
#define ASSERT_LOCKED_RECURSIVE(r)
#define ASSERT_LOCKED_MUTEX(m)
#endif


#ifdef __cplusplus
extern "C" {
#endif

extern void	recursive_lock_init(recursive_lock *lock, const char *name);
	// name is *not* cloned nor freed in recursive_lock_destroy()
extern void recursive_lock_init_etc(recursive_lock *lock, const char *name,
	uint32 flags);
extern void recursive_lock_destroy(recursive_lock *lock);
extern status_t recursive_lock_lock(recursive_lock *lock);
extern void recursive_lock_unlock(recursive_lock *lock);
extern int32 recursive_lock_get_recursion(recursive_lock *lock);

extern status_t benaphore_init(benaphore *ben, const char *name);
extern void benaphore_destroy(benaphore *ben);


static inline status_t
benaphore_lock(benaphore *ben)
{
#ifdef KDEBUG
	return acquire_sem(ben->sem);
#else
	if (atomic_add(&ben->count, -1) <= 0)
		return acquire_sem(ben->sem);

	return B_OK;
#endif
}


static inline status_t
benaphore_unlock(benaphore *ben)
{
#ifdef KDEBUG
	return release_sem(ben->sem);
#else
	if (atomic_add(&ben->count, 1) < 0)
		return release_sem(ben->sem);

	return B_OK;
#endif
}

extern status_t rw_lock_init(rw_lock *lock, const char *name);
extern void rw_lock_destroy(rw_lock *lock);
extern status_t rw_lock_read_lock(rw_lock *lock);
extern status_t rw_lock_read_unlock(rw_lock *lock);
extern status_t rw_lock_write_lock(rw_lock *lock);
extern status_t rw_lock_write_unlock(rw_lock *lock);

extern void mutex_init(mutex* lock, const char *name);
	// name is *not* cloned nor freed in mutex_destroy()
extern void mutex_init_etc(mutex* lock, const char *name, uint32 flags);
extern void mutex_destroy(mutex* lock);

// implementation private:
extern status_t _mutex_lock(mutex* lock, bool threadsLocked);
extern void _mutex_unlock(mutex* lock);
extern status_t _mutex_trylock(mutex* lock);


static inline status_t
mutex_lock(mutex* lock)
{
#ifdef KDEBUG
	return _mutex_lock(lock, false);
#else
	if (atomic_add(&lock->count, -1) < 0)
		return _mutex_lock(lock, false);
	return B_OK;
#endif
}


static inline status_t
mutex_lock_threads_locked(mutex* lock)
{
#ifdef KDEBUG
	return _mutex_lock(lock, true);
#else
	if (atomic_add(&lock->count, -1) < 0)
		return _mutex_lock(lock, true);
	return B_OK;
#endif
}


static inline status_t
mutex_trylock(mutex* lock)
{
#ifdef KDEBUG
	return _mutex_trylock(lock);
#else
	if (atomic_test_and_set(&lock->count, -1, 0) != 0)
		return B_WOULD_BLOCK;
	return B_OK;
#endif
}


static inline void
mutex_unlock(mutex* lock)
{
#ifdef KDEBUG
	_mutex_unlock(lock);
#else
	if (atomic_add(&lock->count, 1) < -1)
		_mutex_unlock(lock);
#endif
}


extern void lock_debug_init();

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_LOCK_H */
