/*
 * Copyright 2009, Michael Lotz, mmlr@mlotz.ch.
 * Distributed under the terms of the MIT License.
 */
#ifndef _LOCKS_H_
#define _LOCKS_H_

#include <OS.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mutex {
	const char*	name;
	int32		lock;
	uint32		flags;
} mutex;

#define MUTEX_FLAG_CLONE_NAME		0x1
#define MUTEX_FLAG_ADAPTIVE			0x2
#define MUTEX_INITIALIZER(name)		{ name, 0, 0 }

#define mutex_init(lock, name)            __mutex_init(lock, name)
#define mutex_init_etc(lock, name, flags) __mutex_init_etc(lock, name, flags)
#define mutex_destroy(lock)               __mutex_destroy(lock)
#define mutex_lock(lock)                  __mutex_lock(lock)
#define mutex_unlock(lock)                __mutex_unlock(lock)

void		__mutex_init(mutex *lock, const char *name);
void		__mutex_init_etc(mutex *lock, const char *name, uint32 flags);
void		__mutex_destroy(mutex *lock);
status_t	__mutex_lock(mutex *lock);
void		__mutex_unlock(mutex *lock);


typedef struct rw_lock {
	mutex					lock;
	struct rw_lock_waiter *	waiters;
	struct rw_lock_waiter *	last_waiter;
	thread_id				holder;
	int32					reader_count;
	int32					writer_count;
	int32					owner_count;
} rw_lock;

#define RW_LOCK_FLAG_CLONE_NAME			MUTEX_FLAG_CLONE_NAME
#define RW_LOCK_INITIALIZER(name)		{ MUTEX_INITIALIZER(name), NULL, \
											NULL, -1, 0, 0, 0 }

#define rw_lock_init(lock, name)   __rw_lock_init(lock, name)
#define rw_lock_init_etc(lock, name, flags) \
      __rw_lock_init_etc(lock, name, flags)
#define rw_lock_destroy(lock)      __rw_lock_destroy(lock)
#define rw_lock_read_lock(lock)    __rw_lock_read_lock(lock)
#define rw_lock_read_unlock(lock)  __rw_lock_read_unlock(lock)
#define rw_lock_write_lock(lock)   __rw_lock_write_lock(lock)
#define rw_lock_write_unlock(lock) __rw_lock_write_unlock(lock)

void		__rw_lock_init(rw_lock *lock, const char *name);
void		__rw_lock_init_etc(rw_lock *lock, const char *name, uint32 flags);
void		__rw_lock_destroy(rw_lock *lock);
status_t	__rw_lock_read_lock(rw_lock *lock);
status_t	__rw_lock_read_unlock(rw_lock *lock);
status_t	__rw_lock_write_lock(rw_lock *lock);
status_t	__rw_lock_write_unlock(rw_lock *lock);


typedef struct recursive_lock {
	mutex		lock;
	thread_id	holder;
	int32		recursion;
} recursive_lock;

#define RECURSIVE_LOCK_FLAG_CLONE_NAME		MUTEX_FLAG_CLONE_NAME
#define RECURSIVE_LOCK_INITIALIZER(name)	{ MUTEX_INITIALIZER(name), -1, 0 }

#define recursive_lock_init(lock, name)    __recursive_lock_init(lock, name)
#define recursive_lock_init_etc(lock, name, flags) \
      __recursive_lock_init_etc(lock, name, flags)
#define recursive_lock_destroy(lock)       __recursive_lock_destroy(lock)
#define recursive_lock_lock(lock)          __recursive_lock_lock(lock)
#define recursive_lock_unlock(lock)        __recursive_lock_unlock(lock)
#define recursive_lock_get_recursion(lock) __recursive_lock_get_recursion(lock)

void		__recursive_lock_init(recursive_lock *lock, const char *name);
void		__recursive_lock_init_etc(recursive_lock *lock, const char *name,
				uint32 flags);
void		__recursive_lock_destroy(recursive_lock *lock);
status_t	__recursive_lock_lock(recursive_lock *lock);
void		__recursive_lock_unlock(recursive_lock *lock);
int32		__recursive_lock_get_recursion(recursive_lock *lock);


#define		INIT_ONCE_UNINITIALIZED	-1
#define		INIT_ONCE_INITIALIZED	-4

status_t	__init_once(int32* control, status_t (*initRoutine)(void*),
				void* data);

#ifdef __cplusplus
} // extern "C"


#include <AutoLocker.h>

class MutexLocking {
public:
	inline bool Lock(struct mutex *lock)
	{
		return mutex_lock(lock) == B_OK;
	}

	inline void Unlock(struct mutex *lock)
	{
		mutex_unlock(lock);
	}
};

typedef AutoLocker<mutex, MutexLocking> MutexLocker;


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

typedef AutoLocker<recursive_lock, RecursiveLockLocking> RecursiveLocker;


class RWLockReadLocking {
public:
	inline bool Lock(struct rw_lock *lock)
	{
		return rw_lock_read_lock(lock) == B_OK;
	}

	inline void Unlock(struct rw_lock *lock)
	{
		rw_lock_read_unlock(lock);
	}
};


class RWLockWriteLocking {
public:
	inline bool Lock(struct rw_lock *lock)
	{
		return rw_lock_write_lock(lock) == B_OK;
	}

	inline void Unlock(struct rw_lock *lock)
	{
		rw_lock_write_unlock(lock);
	}
};


typedef AutoLocker<rw_lock, RWLockReadLocking> ReadLocker;
typedef AutoLocker<rw_lock, RWLockWriteLocking> WriteLocker;

#endif // __cplusplus

#endif // _LOCKS_H_
