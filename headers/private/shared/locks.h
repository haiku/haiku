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
#define MUTEX_INITIALIZER(name)		{ name, 0, 0 }

void		mutex_init(mutex *lock, const char *name);
void		mutex_init_etc(mutex *lock, const char *name, uint32 flags);
void		mutex_destroy(mutex *lock);
status_t	mutex_lock(mutex *lock);
void		mutex_unlock(mutex *lock);


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

void		rw_lock_init(rw_lock *lock, const char *name);
void		rw_lock_init_etc(rw_lock *lock, const char *name, uint32 flags);
void		rw_lock_destroy(rw_lock *lock);
status_t	rw_lock_read_lock(rw_lock *lock);
status_t	rw_lock_read_unlock(rw_lock *lock);
status_t	rw_lock_write_lock(rw_lock *lock);
status_t	rw_lock_write_unlock(rw_lock *lock);


typedef struct recursive_lock {
	mutex		lock;
	thread_id	holder;
	int32		recursion;
} recursive_lock;

#define RECURSIVE_LOCK_FLAG_CLONE_NAME		MUTEX_FLAG_CLONE_NAME
#define RECURSIVE_LOCK_INITIALIZER(name)	{ MUTEX_INITIALIZER(name), -1, 0 }

void		recursive_lock_init(recursive_lock *lock, const char *name);
void		recursive_lock_init_etc(recursive_lock *lock, const char *name,
				uint32 flags);
void		recursive_lock_destroy(recursive_lock *lock);
status_t	recursive_lock_lock(recursive_lock *lock);
void		recursive_lock_unlock(recursive_lock *lock);
int32		recursive_lock_get_recursion(recursive_lock *lock);


#define		INIT_ONCE_UNINITIALIZED	-1
#define		INIT_ONCE_INITIALIZED	-4

status_t	__init_once(vint32* control, status_t (*initRoutine)(void*),
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


typedef AutoLocker<mutex, MutexLocking> MutexLocker;
typedef AutoLocker<rw_lock, RWLockReadLocking> ReadLocker;
typedef AutoLocker<rw_lock, RWLockWriteLocking> WriteLocker;

#endif // __cplusplus

#endif // _LOCKS_H_
