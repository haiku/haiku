#ifndef LOCK_H
#define LOCK_H
/* lock.h - benaphore locking, "faster semaphores", read/write locks
**		sorry for all those macros; I wish C would support C++-style
**		inline functions.
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
**
** This file may be used under the terms of the OpenBeOS License.
*/


#include <SupportDefs.h>

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

#ifndef _KERNEL_MODE
#define INIT_BENAPHORE(lock,name) \
	{ \
		(lock).count = 1; \
		(lock).sem = create_sem(0, name); \
	}

#else

#define INIT_BENAPHORE(lock,name) \
        { \
                (lock).count = 1; \
                (lock).sem = create_sem(0, name); \
                set_sem_owner((lock).sem, B_SYSTEM_TEAM); \
        }
#endif


#define CHECK_BENAPHORE(lock) \
	((lock).sem)

#define UNINIT_BENAPHORE(lock) \
	delete_sem((lock).sem);

#define ACQUIRE_BENAPHORE(lock) \
	(atomic_add(&((lock).count), -1) <= 0 ? \
		acquire_sem_etc((lock).sem, 1, B_CAN_INTERRUPT, 0) \
		: B_OK)

#define RELEASE_BENAPHORE(lock) \
	{ \
		if (atomic_add(&((lock).count), 1) < 0) \
			release_sem((lock).sem); \
	}

/* read/write lock */
#ifndef _KERNEL_MODE
#define INIT_RW_LOCK(lock,name) \
	{ \
		(lock).sem = create_sem(0, name); \
		(lock).count = MAX_READERS; \
		INIT_BENAPHORE((lock).writeLock, "r/w write lock"); \
	}

#else

#define INIT_RW_LOCK(lock,name) \
        { \
                (lock).sem = create_sem(0, name); \
                set_sem_owner((lock).sem, B_SYSTEM_TEAM); \
                (lock).count = MAX_READERS; \
                INIT_BENAPHORE((lock).writeLock, "r/w write lock"); \
        }

#endif
#define CHECK_RW_LOCK(lock) \
	((lock).sem)

#define UNINIT_RW_LOCK(lock) \
	delete_sem((lock).sem); \
	UNINIT_BENAPHORE((lock).writeLock)

#define ACQUIRE_READ_LOCK(lock) \
	{ \
		if (atomic_add(&(lock).count, -1) <= 0) \
			acquire_sem((lock).sem); \
	}

#define RELEASE_READ_LOCK(lock) \
	{ \
		if (atomic_add(&(lock).count, 1) < 0) \
			release_sem((lock).sem); \
	}

#define ACQUIRE_WRITE_LOCK(lock) \
	{ \
		int32 readers; \
		ACQUIRE_BENAPHORE((lock).writeLock); \
		readers = atomic_add(&(lock).count, -MAX_READERS); \
		if (readers < MAX_READERS) \
			acquire_sem_etc((lock).sem,readers <= 0 ? 1 : MAX_READERS - readers,0,0); \
		RELEASE_BENAPHORE((lock).writeLock); \
	}

#define RELEASE_WRITE_LOCK(lock) \
	{ \
		int32 readers = atomic_add(&(lock).count,MAX_READERS); \
		if (readers < 0) \
			release_sem_etc((lock).sem,readers <= -MAX_READERS ? 1 : -readers,B_CAN_INTERRUPT); \
	}

#endif	/* LOCK_H */
