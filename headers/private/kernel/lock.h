/*
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

typedef struct recursive_lock {
	sem_id		sem;
	thread_id	holder;
	int			recursion;
} recursive_lock;

typedef struct mutex {
	sem_id		sem;
	thread_id	holder;
} mutex;

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

struct cutex_waiter;

typedef struct cutex {
	const char*				name;
	struct cutex_waiter*	waiters;
#ifdef KDEBUG
	thread_id				holder;
#else
	int32					count;
#endif
	uint8					flags;
} cutex;

#define CUTEX_FLAG_CLONE_NAME	0x1


#if 0 && KDEBUG // XXX disable this for now, it causes problems when including thread.h here
#	include <thread.h>
#define ASSERT_LOCKED_RECURSIVE(r) { ASSERT(thread_get_current_thread_id() == (r)->holder); }
#define ASSERT_LOCKED_MUTEX(m) { ASSERT(thread_get_current_thread_id() == (m)->holder); }
#define ASSERT_LOCKED_CUTEX(m) { ASSERT(thread_get_current_thread_id() == (m)->holder); }
#else
#define ASSERT_LOCKED_RECURSIVE(r)
#define ASSERT_LOCKED_MUTEX(m)
#define ASSERT_LOCKED_CUTEX(m)
#endif


#ifdef __cplusplus
extern "C" {
#endif

extern status_t	recursive_lock_init(recursive_lock *lock, const char *name);
extern void recursive_lock_destroy(recursive_lock *lock);
extern status_t recursive_lock_lock(recursive_lock *lock);
extern void recursive_lock_unlock(recursive_lock *lock);
extern int32 recursive_lock_get_recursion(recursive_lock *lock);

extern status_t	mutex_init(mutex *m, const char *name);
extern void mutex_destroy(mutex *m);
extern status_t mutex_trylock(mutex *mutex);
extern status_t mutex_lock(mutex *m);
extern void mutex_unlock(mutex *m);

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

extern void cutex_init(cutex* lock, const char *name);
	// name is *not* cloned nor freed in cutex_destroy()
extern void cutex_init_etc(cutex* lock, const char *name, uint32 flags);
extern void cutex_destroy(cutex* lock);

// implementation private:
extern status_t _cutex_lock(cutex* lock);
extern void _cutex_unlock(cutex* lock);
extern status_t _cutex_trylock(cutex* lock);


static inline status_t
cutex_lock(cutex* lock)
{
#ifdef KDEBUG
	return _cutex_lock(lock);
#else
	if (atomic_add(&lock->count, -1) < 0)
		return _cutex_lock(lock);
	return B_OK;
#endif
}


static inline status_t
cutex_trylock(cutex* lock)
{
#ifdef KDEBUG
	return _cutex_trylock(lock);
#else
	if (atomic_test_and_set(&lock->count, -1, 0) != 0)
		return B_WOULD_BLOCK;
	return B_OK;
#endif
}


static inline void
cutex_unlock(cutex* lock)
{
#ifdef KDEBUG
	_cutex_unlock(lock);
#else
	if (atomic_add(&lock->count, 1) < -1)
		_cutex_unlock(lock);
#endif
}


extern void lock_debug_init();

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_LOCK_H */
