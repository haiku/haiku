/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _FSSH_KERNEL_LOCK_H
#define _FSSH_KERNEL_LOCK_H


#include <fssh_auto_locker.h>
#include <fssh_errors.h>
#include <fssh_os.h>


typedef struct fssh_mutex {
	fssh_sem_id		sem;
	fssh_thread_id	holder;
} fssh_mutex;

#define FSSH_MUTEX_FLAG_CLONE_NAME	0x1


typedef struct fssh_recursive_lock {
	fssh_sem_id		sem;
	fssh_thread_id	holder;
	int				recursion;
} fssh_recursive_lock;


typedef struct fssh_rw_lock {
	fssh_sem_id		sem;
	fssh_thread_id	holder;
	int32_t			count;
} fssh_rw_lock;

#define FSSH_RW_LOCK_FLAG_CLONE_NAME	0x1

#define FSSH_ASSERT_LOCKED_RECURSIVE(r)
#define FSSH_ASSERT_LOCKED_MUTEX(m)
#define FSSH_ASSERT_WRITE_LOCKED_RW_LOCK(l)
#define FSSH_ASSERT_READ_LOCKED_RW_LOCK(l)

// static initializers
#define FSSH_MUTEX_INITIALIZER(name)			{ name, NULL, 0, 0 }
#define FSSH_RECURSIVE_LOCK_INITIALIZER(name)	{ FSSH_MUTEX_INITIALIZER(name), -1, 0 }
#define FSSH_RW_LOCK_INITIALIZER(name)			{ name, NULL, -1, 0, 0, 0 }

#ifdef __cplusplus
extern "C" {
#endif

extern void	fssh_recursive_lock_init(fssh_recursive_lock *lock, const char *name);
	// name is *not* cloned nor freed in recursive_lock_destroy()
extern void fssh_recursive_lock_init_etc(fssh_recursive_lock *lock, const char *name,
	uint32_t flags);
extern void fssh_recursive_lock_destroy(fssh_recursive_lock *lock);
extern fssh_status_t fssh_recursive_lock_lock(fssh_recursive_lock *lock);
extern fssh_status_t fssh_recursive_lock_trylock(fssh_recursive_lock *lock);
extern void fssh_recursive_lock_unlock(fssh_recursive_lock *lock);
extern int32_t fssh_recursive_lock_get_recursion(fssh_recursive_lock *lock);
extern void fssh_recursive_lock_transfer_lock(fssh_recursive_lock *lock, fssh_thread_id thread);

extern void fssh_rw_lock_init(fssh_rw_lock* lock, const char* name);
	// name is *not* cloned nor freed in rw_lock_destroy()
extern void fssh_rw_lock_init_etc(fssh_rw_lock* lock, const char* name, uint32_t flags);
extern void fssh_rw_lock_destroy(fssh_rw_lock* lock);
extern fssh_status_t fssh_rw_lock_read_lock(fssh_rw_lock* lock);
extern fssh_status_t fssh_rw_lock_read_unlock(fssh_rw_lock* lock);
extern fssh_status_t fssh_rw_lock_write_lock(fssh_rw_lock* lock);
extern fssh_status_t fssh_rw_lock_write_unlock(fssh_rw_lock* lock);

extern void fssh_mutex_init(fssh_mutex* lock, const char* name);
	// name is *not* cloned nor freed in mutex_destroy()
extern void fssh_mutex_init_etc(fssh_mutex* lock, const char* name, uint32_t flags);
extern void fssh_mutex_destroy(fssh_mutex* lock);
extern fssh_status_t fssh_mutex_lock(fssh_mutex* lock);
extern fssh_status_t fssh_mutex_trylock(fssh_mutex* lock);
extern void fssh_mutex_unlock(fssh_mutex* lock);
extern void fssh_mutex_transfer_lock(fssh_mutex* lock, fssh_thread_id thread);

#ifdef __cplusplus
}

namespace FSShell {

// MutexLocking
class MutexLocking {
public:
	inline bool Lock(fssh_mutex *lockable)
	{
		return fssh_mutex_lock(lockable) == FSSH_B_OK;
	}

	inline void Unlock(fssh_mutex *lockable)
	{
		fssh_mutex_unlock(lockable);
	}
};

// MutexLocker
typedef AutoLocker<fssh_mutex, MutexLocking> MutexLocker;

// RecursiveLockLocking
class RecursiveLockLocking {
public:
	inline bool Lock(fssh_recursive_lock *lockable)
	{
		return fssh_recursive_lock_lock(lockable) == FSSH_B_OK;
	}

	inline void Unlock(fssh_recursive_lock *lockable)
	{
		fssh_recursive_lock_unlock(lockable);
	}
};

// RecursiveLocker
typedef AutoLocker<fssh_recursive_lock, RecursiveLockLocking> RecursiveLocker;

class ReadWriteLockReadLocking {
public:
	inline bool Lock(fssh_rw_lock *lockable)
	{
		return fssh_rw_lock_read_lock(lockable) == FSSH_B_OK;
	}

	inline void Unlock(fssh_rw_lock *lockable)
	{
		fssh_rw_lock_read_unlock(lockable);
	}
};

class ReadWriteLockWriteLocking {
public:
	inline bool Lock(fssh_rw_lock *lockable)
	{
		return fssh_rw_lock_write_lock(lockable) == FSSH_B_OK;
	}

	inline void Unlock(fssh_rw_lock *lockable)
	{
		fssh_rw_lock_write_unlock(lockable);
	}
};

typedef AutoLocker<fssh_rw_lock, ReadWriteLockReadLocking> ReadLocker;
typedef AutoLocker<fssh_rw_lock, ReadWriteLockWriteLocking> WriteLocker;

}	// namespace FSShell

using FSShell::AutoLocker;
using FSShell::MutexLocker;
using FSShell::RecursiveLocker;
using FSShell::ReadLocker;
using FSShell::WriteLocker;

#endif	// __cplusplus

#endif	/* _FSSH_KERNEL_LOCK_H */
