/* 
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _KERNEL_LOCK_H
#define _KERNEL_LOCK_H

#include <kernel.h>

typedef struct recursive_lock {
	thread_id holder;
	int recursion;
	sem_id sem;
} recursive_lock;

int recursive_lock_create(recursive_lock *lock);
void recursive_lock_destroy(recursive_lock *lock);
bool recursive_lock_lock(recursive_lock *lock);
bool recursive_lock_unlock(recursive_lock *lock);
int recursive_lock_get_recursion(recursive_lock *lock);

typedef struct mutex {
	int count;
	sem_id sem;
} mutex;

int mutex_init(mutex *m, const char *name);
void mutex_destroy(mutex *m);
void mutex_lock(mutex *m);
void mutex_unlock(mutex *m);

// for read/write locks
#define MAX_READERS 100000

struct benaphore {
	sem_id	sem;
	int32	count;
};

typedef struct benaphore benaphore;

// it may make sense to add a status field to the rw_lock to
// be able to check if the semaphore could be locked

struct rw_lock {
	sem_id		sem;
	int32		count;
	benaphore	writeLock;
};

typedef struct rw_lock rw_lock;

#define INIT_BENAPHORE(lock,name) \
	{ \
		(lock).count = 1; \
		(lock).sem = create_sem(0, name); \
	}

#define CHECK_BENAPHORE(lock) \
	((lock).sem)

#define UNINIT_BENAPHORE(lock) \
	delete_sem((lock).sem);

#define ACQUIRE_BENAPHORE(lock) \
	(atomic_add(&((lock).count), -1) <= 0 ? \
		acquire_sem_etc((lock).sem, 1, B_CAN_INTERRUPT, 0) \
		: 0)

#define RELEASE_BENAPHORE(lock) \
	{ \
		if (atomic_add(&((lock).count), 1) < 0) \
			release_sem_etc((lock).sem, 1, B_CAN_INTERRUPT); \
	}

/* read/write lock */
#define INIT_RW_LOCK(lock,name) \
	{ \
		(lock).sem = create_sem(0, name); \
		(lock).count = MAX_READERS; \
		INIT_BENAPHORE((lock).writeLock, "r/w write lock"); \
	}

#define CHECK_RW_LOCK(lock) \
	((lock).sem)

#define UNINIT_RW_LOCK(lock) \
	delete_sem((lock).sem); \
	UNINIT_BENAPHORE((lock).writeLock)

#define ACQUIRE_READ_LOCK(lock) \
	{ \
		if (atomic_add(&(lock).count, -1) <= 0) \
			acquire_sem_etc((lock).sem, 1, B_CAN_INTERRUPT, 0); \
	}

#define RELEASE_READ_LOCK(lock) \
	{ \
		if (atomic_add(&(lock).count, 1) < 0) \
			release_sem_etc((lock).sem, 1, B_CAN_INTERRUPT); \
	}

#define ACQUIRE_WRITE_LOCK(lock) \
	{ \
		int32 readers; \
		ACQUIRE_BENAPHORE((lock).writeLock); \
		readers = atomic_add(&(lock).count, -MAX_READERS); \
		if (readers < MAX_READERS) \
			acquire_sem_etc((lock).sem,readers <= 0 ? 1 : MAX_READERS - readers, \
			                B_CAN_INTERRUPT,0); \
		RELEASE_BENAPHORE((lock).writeLock); \
	}

#define RELEASE_WRITE_LOCK(lock) \
	{ \
		int32 readers = atomic_add(&(lock).count,MAX_READERS); \
		if (readers < 0) \
			release_sem_etc((lock).sem,readers <= -MAX_READERS ? 1 : -readers,B_CAN_INTERRUPT); \
	}


#endif

