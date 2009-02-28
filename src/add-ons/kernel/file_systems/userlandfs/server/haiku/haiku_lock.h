/*
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef USERLAND_FS_HAIKU_LOCK_H
#define USERLAND_FS_HAIKU_LOCK_H

#include <OS.h>


namespace UserlandFS {
namespace HaikuKernelEmu {

typedef struct recursive_lock {
	sem_id		sem;
	thread_id	holder;
	int			recursion;
} recursive_lock;



typedef struct mutex {
	sem_id		sem;
	thread_id	holder;
} mutex;

#define MUTEX_FLAG_CLONE_NAME	0x1


typedef struct rw_lock {
	sem_id		sem;
} rw_lock;

#define RW_MAX_READERS 1000000
#define RW_LOCK_FLAG_CLONE_NAME	0x1


#define ASSERT_LOCKED_RECURSIVE(r)		do {} while (false)
#define ASSERT_LOCKED_MUTEX(m)			do {} while (false)
#define ASSERT_WRITE_LOCKED_RW_LOCK(m)	do {} while (false)
#define ASSERT_READ_LOCKED_RW_LOCK(l)	do {} while (false)


// static initializers
#define MUTEX_INITIALIZER(name)				{ _init_semaphore(1, name), -1 }
#define RECURSIVE_LOCK_INITIALIZER(name)	{ _init_semaphore(1, name), -1, 0 }
#define RW_LOCK_INITIALIZER(name)			\
	{ _init_semaphore(RW_MAX_READERS, name) }


sem_id _init_semaphore(int32 count, const char* name);
	// implementation private


extern void	recursive_lock_init(recursive_lock *lock, const char *name);
	// name is *not* cloned nor freed in recursive_lock_destroy()
extern void recursive_lock_init_etc(recursive_lock *lock, const char *name,
	uint32 flags);
extern void recursive_lock_destroy(recursive_lock *lock);
extern status_t recursive_lock_lock(recursive_lock *lock);
extern status_t recursive_lock_trylock(recursive_lock *lock);
extern void recursive_lock_unlock(recursive_lock *lock);
extern int32 recursive_lock_get_recursion(recursive_lock *lock);


extern void mutex_init(mutex* lock, const char* name);
	// name is *not* cloned nor freed in mutex_destroy()
extern void mutex_init_etc(mutex* lock, const char* name, uint32 flags);
extern void mutex_destroy(mutex* lock);
//extern status_t mutex_switch_lock(mutex* from, mutex* to);
	// Unlocks "from" and locks "to" such that unlocking and starting to wait
	// for the lock is atomically. I.e. if "from" guards the object "to" belongs
	// to, the operation is safe as long as "from" is held while destroying
	// "to".

status_t mutex_lock(mutex* lock);
//status_t mutex_lock_threads_locked(mutex* lock);
status_t mutex_trylock(mutex* lock);
void mutex_unlock(mutex* lock);
//void mutex_transfer_lock(mutex* lock, thread_id thread);


extern void rw_lock_init(rw_lock* lock, const char* name);
	// name is *not* cloned nor freed in rw_lock_destroy()
extern void rw_lock_init_etc(rw_lock* lock, const char* name, uint32 flags);
extern void rw_lock_destroy(rw_lock* lock);
extern status_t rw_lock_read_lock(rw_lock* lock);
extern status_t rw_lock_read_unlock(rw_lock* lock);
extern status_t rw_lock_write_lock(rw_lock* lock);
extern status_t rw_lock_write_unlock(rw_lock* lock);


/* C++ Auto Locking */

#include "AutoLocker.h"


// MutexLocking
class MutexLocking {
public:
	inline bool Lock(mutex *lockable)
	{
		return mutex_lock(lockable) == B_OK;
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
		return recursive_lock_lock(lockable) == B_OK;
	}

	inline void Unlock(recursive_lock *lockable)
	{
		recursive_lock_unlock(lockable);
	}
};

// RecursiveLocker
typedef AutoLocker<recursive_lock, RecursiveLockLocking> RecursiveLocker;

}	// namespace HaikuKernelEmu
}	// namespace UserlandFS

using UserlandFS::HaikuKernelEmu::MutexLocker;
using UserlandFS::HaikuKernelEmu::RecursiveLocker;

#endif	/* USERLAND_FS_HAIKU_LOCK_H */
