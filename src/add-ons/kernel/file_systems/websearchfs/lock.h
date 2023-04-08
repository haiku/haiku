/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _LOCK_H
#define _LOCK_H

#include <BeBuild.h>

#include <OS.h>

typedef struct lock lock;
typedef struct mlock mlock;

struct lock {
	sem_id		s;
	long		c;
};

struct mlock {
	sem_id		s;
};

extern _IMPEXP_KERNEL int	new_lock(lock *l, const char *name);
extern _IMPEXP_KERNEL int	free_lock(lock *l);

static inline status_t LOCK(lock *l)
{
	if (atomic_add(&(l->c), -1) <= 0)
		return acquire_sem(l->s);
	return B_OK;
}

static inline status_t UNLOCK(lock *l)
{
	if (atomic_add(&(l->c), 1) < 0)
		return release_sem(l->s);
	return B_OK;
}


//#define	LOCK(l)		if (atomic_add(&l.c, -1) <= 0) acquire_sem(l.s);
//#define	UNLOCK(l)	if (atomic_add(&l.c, 1) < 0) release_sem(l.s);

extern _IMPEXP_KERNEL int	new_mlock(mlock *l, long c, const char *name);
extern _IMPEXP_KERNEL int	free_mlock(mlock *l);

#define		LOCKM(l,cnt)	acquire_sem_etc(l.s, cnt, 0, 0)
#define		UNLOCKM(l,cnt)	release_sem_etc(l.s, cnt, 0)

#endif
