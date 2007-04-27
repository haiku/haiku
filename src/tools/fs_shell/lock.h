/*
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _FSSH_LOCK_H
#define _FSSH_LOCK_H

#include "fssh_atomic.h"
#include "fssh_auto_locker.h"
#include "fssh_errors.h"
#include "fssh_os.h"


namespace FSShell {


typedef struct recursive_lock {
	fssh_sem_id		sem;
	fssh_thread_id	holder;
	int				recursion;
} recursive_lock;

typedef struct mutex {
	fssh_sem_id		sem;
	fssh_thread_id	holder;
} mutex;

typedef struct benaphore {
	fssh_sem_id	sem;
	int32_t		count;
} benaphore;

// Note: this is currently a trivial r/w lock implementation
//	it will be replaced with something better later - this
//	or a similar API will be made publically available at this point.
typedef struct rw_lock {
	fssh_sem_id		sem;
	int32_t			count;
	benaphore		writeLock;
} rw_lock;

#define FSSH_RW_MAX_READERS 1000000


extern fssh_status_t	recursive_lock_init(recursive_lock *lock,
							const char *name);
extern void				recursive_lock_destroy(recursive_lock *lock);
extern fssh_status_t	recursive_lock_lock(recursive_lock *lock);
extern void				recursive_lock_unlock(recursive_lock *lock);
extern int32_t			recursive_lock_get_recursion(recursive_lock *lock);

extern fssh_status_t	mutex_init(mutex *m, const char *name);
extern void				mutex_destroy(mutex *m);
extern fssh_status_t	mutex_lock(mutex *m);
extern void				mutex_unlock(mutex *m);

extern fssh_status_t	benaphore_init(benaphore *ben,
							const char *name);
extern void				benaphore_destroy(benaphore *ben);

static inline fssh_status_t
benaphore_lock_etc(benaphore *ben, uint32_t flags, fssh_bigtime_t timeout)
{
	if (fssh_atomic_add(&ben->count, -1) <= 0)
		return fssh_acquire_sem_etc(ben->sem, 1, flags, timeout);

	return FSSH_B_OK;
}


static inline fssh_status_t
benaphore_lock(benaphore *ben)
{
	if (fssh_atomic_add(&ben->count, -1) <= 0)
		return fssh_acquire_sem(ben->sem);

	return FSSH_B_OK;
}


static inline fssh_status_t
benaphore_unlock(benaphore *ben)
{
	if (fssh_atomic_add(&ben->count, 1) < 0)
		return fssh_release_sem(ben->sem);

	return FSSH_B_OK;
}

extern fssh_status_t	rw_lock_init(rw_lock *lock, const char *name);
extern void				rw_lock_destroy(rw_lock *lock);
extern fssh_status_t	rw_lock_read_lock(rw_lock *lock);
extern fssh_status_t	rw_lock_read_unlock(rw_lock *lock);
extern fssh_status_t	rw_lock_write_lock(rw_lock *lock);
extern fssh_status_t	rw_lock_write_unlock(rw_lock *lock);


/* C++ Auto Locking */

// MutexLocking
class MutexLocking {
public:
	inline bool Lock(mutex *lockable)
	{
		return mutex_lock(lockable) == FSSH_B_OK;
	}

	inline void Unlock(mutex *lockable)
	{
		mutex_unlock(lockable);
	}
};

// MutexLocker
typedef AutoLocker<mutex, MutexLocking> MutexLocker;

// RecursiveLockLocking
class RecursiveLockLocking {
public:
	inline bool Lock(recursive_lock *lockable)
	{
		return recursive_lock_lock(lockable) == FSSH_B_OK;
	}

	inline void Unlock(recursive_lock *lockable)
	{
		recursive_lock_unlock(lockable);
	}
};

// RecursiveLocker
typedef AutoLocker<recursive_lock, RecursiveLockLocking> RecursiveLocker;

// BenaphoreLocking
class BenaphoreLocking {
public:
	inline bool Lock(benaphore *lockable)
	{
		return benaphore_lock(lockable) == FSSH_B_OK;
	}

	inline void Unlock(benaphore *lockable)
	{
		benaphore_unlock(lockable);
	}
};

// BenaphoreLocker
typedef AutoLocker<benaphore, BenaphoreLocking> BenaphoreLocker;

}	// namespace FSShell

using FSShell::MutexLocker;
using FSShell::RecursiveLocker;
using FSShell::BenaphoreLocker;

#endif	/* _FSSH_LOCK_H */
