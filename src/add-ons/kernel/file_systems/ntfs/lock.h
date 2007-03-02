#ifndef _LOCK_H
#define _LOCK_H

#include <OS.h>

#include <lock.h>

#ifdef __HAIKU__

typedef recursive_lock lock;

#define	LOCK(l)		recursive_lock_lock(&l);
#define	UNLOCK(l)	recursive_lock_unlock(&l);

#else

typedef struct lock lock;
typedef struct mlock mlock;

struct lock {
	sem_id		s;
	long		c;
};

struct mlock {
	sem_id		s;
};

extern  int	new_lock(lock *l, const char *name);
extern  int	free_lock(lock *l);

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


#define	LOCK(l)		if (atomic_add(&l.c, -1) <= 0) acquire_sem(l.s);
#define	UNLOCK(l)	if (atomic_add(&l.c, 1) < 0) release_sem(l.s);

extern  int	new_mlock(mlock *l, long c, const char *name);
extern  int	free_mlock(mlock *l);

#define		LOCKM(l,cnt)	acquire_sem_etc(l.s, cnt, 0, 0)
#define		UNLOCKM(l,cnt)	release_sem_etc(l.s, cnt, 0)

#endif //__HAIKU__

#endif
