#ifndef _LOCK_H
#define _LOCK_H

#ifdef __cplusplus
extern "C" {
//#	define lock fsh_lock
#endif

typedef struct lock lock;
typedef struct mlock mlock;

struct lock {
    sem_id      s;
    long        c;
};

struct mlock {
    sem_id      s;
};

extern int  new_lock(lock *l, const char *name);
extern int  free_lock(lock *l);

#define LOCK(l)     if (atomic_add(&l.c, -1) <= 0) acquire_sem(l.s);
#define UNLOCK(l)   if (atomic_add(&l.c, 1) < 0) release_sem(l.s);

extern int  new_mlock(mlock *l, long c, const char *name);
extern int  free_mlock(mlock *l);

#define     LOCKM(l,cnt)    acquire_sem_etc(l.s, cnt, 0, 0.0)
#define     UNLOCKM(l,cnt)  release_sem_etc(l.s, cnt, 0)

#ifdef __cplusplus
}
#endif

#endif	/* _LOCK_H */
