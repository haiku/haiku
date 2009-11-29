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
	int32	benaphore;
	sem_id	semaphore;
} mutex;

status_t	mutex_init(mutex *lock, const char *name);
void		mutex_destroy(mutex *lock);
status_t	mutex_lock(mutex *lock);
void		mutex_unlock(mutex *lock);


typedef struct lazy_mutex {
	int32		benaphore;
	sem_id		semaphore;
	const char*	name;
} lazy_mutex;

status_t	lazy_mutex_init(lazy_mutex *lock, const char *name);
				// name will not be cloned and must rename valid
void		lazy_mutex_destroy(mutex *lock);
status_t	lazy_mutex_lock(lazy_mutex *lock);
void		lazy_mutex_unlock(lazy_mutex *lock);


typedef struct rw_lock {
	const char *			name;
	mutex					lock;
	struct rw_lock_waiter *	waiters;
	struct rw_lock_waiter *	last_waiter;
	thread_id				holder;
	int32					reader_count;
	int32					writer_count;
	int32					owner_count;
} rw_lock;

status_t	rw_lock_init(rw_lock *lock, const char *name);
void		rw_lock_destroy(rw_lock *lock);
status_t	rw_lock_read_lock(rw_lock *lock);
status_t	rw_lock_read_unlock(rw_lock *lock);
status_t	rw_lock_write_lock(rw_lock *lock);
status_t	rw_lock_write_unlock(rw_lock *lock);


typedef struct recursive_lock {
	mutex		lock;
	thread_id	holder;
	int			recursion;
} recursive_lock;

status_t	recursive_lock_init(recursive_lock *lock, const char *name);
void		recursive_lock_destroy(recursive_lock *lock);
status_t	recursive_lock_lock(recursive_lock *lock);
void		recursive_lock_unlock(recursive_lock *lock);
int32		recursive_lock_get_recursion(recursive_lock *lock);


typedef struct lazy_recursive_lock {
	lazy_mutex	lock;
	thread_id	holder;
	int			recursion;
} lazy_recursive_lock;

status_t	lazy_recursive_lock_init(lazy_recursive_lock *lock,
				const char *name);
				// name will not be cloned and must rename valid
void		lazy_recursive_lock_destroy(lazy_recursive_lock *lock);
status_t	lazy_recursive_lock_lock(lazy_recursive_lock *lock);
void		lazy_recursive_lock_unlock(lazy_recursive_lock *lock);
int32		lazy_recursive_lock_get_recursion(lazy_recursive_lock *lock);

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
