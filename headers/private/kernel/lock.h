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


struct rw_lock_waiter;

typedef struct rw_lock {
	const char*				name;
	struct rw_lock_waiter*	waiters;
	thread_id				holder;
	int32					reader_count;
	int32					writer_count;
	int32					owner_count;
	uint32					flags;
} rw_lock;

#define RW_LOCK_FLAG_CLONE_NAME	0x1


#if KDEBUG
#	define ASSERT_LOCKED_RECURSIVE(r) \
		{ ASSERT(find_thread(NULL) == (r)->lock.holder); }
#	define ASSERT_LOCKED_MUTEX(m) { ASSERT(find_thread(NULL) == (m)->holder); }
#else
#	define ASSERT_LOCKED_RECURSIVE(r)	do {} while (false)
#	define ASSERT_LOCKED_MUTEX(m)		do {} while (false)
#endif


// static initializers
#ifdef KDEBUG
#	define MUTEX_INITIALIZER(name)			{ name, NULL, -1, 0 }
#	define RECURSIVE_LOCK_INITIALIZER(name)	{ MUTEX_INITIALIZER(name), 0 }
#else
#	define MUTEX_INITIALIZER(name)			{ name, NULL, 0, 0 }
#	define RECURSIVE_LOCK_INITIALIZER(name)	{ MUTEX_INITIALIZER(name), -1, 0 }
#endif

#define RW_LOCK_INITIALIZER(name)			{ name, NULL, -1, 0, 0, 0 }


#ifdef __cplusplus
extern "C" {
#endif

extern void	recursive_lock_init(recursive_lock *lock, const char *name);
	// name is *not* cloned nor freed in recursive_lock_destroy()
extern void recursive_lock_init_etc(recursive_lock *lock, const char *name,
	uint32 flags);
extern void recursive_lock_destroy(recursive_lock *lock);
extern status_t recursive_lock_lock(recursive_lock *lock);
extern status_t recursive_lock_trylock(recursive_lock *lock);
extern void recursive_lock_unlock(recursive_lock *lock);
extern int32 recursive_lock_get_recursion(recursive_lock *lock);

extern void rw_lock_init(rw_lock* lock, const char* name);
	// name is *not* cloned nor freed in rw_lock_destroy()
extern void rw_lock_init_etc(rw_lock* lock, const char* name, uint32 flags);
extern void rw_lock_destroy(rw_lock* lock);
extern status_t rw_lock_read_lock(rw_lock* lock);
extern status_t rw_lock_read_unlock(rw_lock* lock);
extern status_t rw_lock_write_lock(rw_lock* lock);
extern status_t rw_lock_write_unlock(rw_lock* lock);

extern void mutex_init(mutex* lock, const char* name);
	// name is *not* cloned nor freed in mutex_destroy()
extern void mutex_init_etc(mutex* lock, const char* name, uint32 flags);
extern void mutex_destroy(mutex* lock);
extern status_t mutex_switch_lock(mutex* from, mutex* to);
	// Unlocks "from" and locks "to" such that unlocking and starting to wait
	// for the lock is atomically. I.e. if "from" guards the object "to" belongs
	// to, the operation is safe as long as "from" is held while destroying
	// "to".

// implementation private:
extern status_t _mutex_lock(mutex* lock, bool threadsLocked);
extern void _mutex_unlock(mutex* lock, bool threadsLocked);
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
#if !defined(KDEBUG)
	if (atomic_add(&lock->count, 1) < -1)
#endif
		_mutex_unlock(lock, false);
}


static inline void
mutex_transfer_lock(mutex* lock, thread_id thread)
{
#ifdef KDEBUG
	lock->holder = thread;
#endif
}


extern void lock_debug_init();

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_LOCK_H */
