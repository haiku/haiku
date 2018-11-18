/*
	Copyright 1999-2001, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef USERLAND_FS_BEOS_LOCK_H
#define USERLAND_FS_BEOS_LOCK_H

#include <BeBuild.h>

#include <OS.h>

#ifndef _IMPEXP_KERNEL
#define _IMPEXP_KERNEL
#endif

#ifdef __cplusplus
	extern "C" {
#else
	typedef struct beos_lock beos_lock;
	typedef struct beos_mlock beos_mlock;
#endif


struct beos_lock {
	sem_id		s;
	int32		c;
};

struct beos_mlock {
	sem_id		s;
};

extern _IMPEXP_KERNEL int	beos_new_lock(beos_lock *l, const char *name);
extern _IMPEXP_KERNEL int	beos_free_lock(beos_lock *l);

#ifdef LOCK
#undef LOCK
#endif

#define	LOCK(l)		if (atomic_add(&l.c, -1) <= 0) acquire_sem(l.s);
#define	UNLOCK(l)	if (atomic_add(&l.c, 1) < 0) release_sem(l.s);

extern _IMPEXP_KERNEL int	beos_new_mlock(beos_mlock *l, long c, const char *name);
extern _IMPEXP_KERNEL int	beos_free_mlock(beos_mlock *l);

#define		LOCKM(l,cnt)	acquire_sem_etc(l.s, cnt, 0, 0)
#define		UNLOCKM(l,cnt)	release_sem_etc(l.s, cnt, 0)


#ifdef __cplusplus
  } // extern "C"
#endif

#endif
