/*
 * Copyright 2009, Colin Günther, coling@gmx.de.
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FBSD_COMPAT_SYS_MUTEX_H_
#define _FBSD_COMPAT_SYS_MUTEX_H_


#include <sys/haiku-module.h>
#include <kernel/int.h>

#include <sys/queue.h>
#include <sys/_mutex.h>
#include <sys/pcpu.h>
#include <machine/atomic.h>
#include <machine/cpufunc.h>


#define MA_OWNED		0x1
#define MA_NOTOWNED		0x2
#define MA_RECURSED		0x4
#define MA_NOTRECURSED	0x8

#define mtx_assert(mtx, what)

#define	MTX_DEF			0x00000000
#define MTX_SPIN		0x00000001
#define MTX_RECURSE		0x00000004
#define MTX_QUIET		0x00040000
#define MTX_DUPOK		0x00400000


#define MTX_NETWORK_LOCK	"network driver"


/* on FreeBSD these are different functions */
#define	mtx_lock_spin(x) 	mtx_lock(x)
#define	mtx_unlock_spin(x)	mtx_unlock(x)


extern struct mtx Giant;


void mtx_init(struct mtx*, const char*, const char*, int);
void mtx_sysinit(void *arg);
void mtx_destroy(struct mtx*);


static inline void
mtx_lock(struct mtx* mutex)
{
	if (mutex->type == MTX_DEF) {
		mutex_lock(&mutex->u.mutex.lock);
		mutex->u.mutex.owner = find_thread(NULL);
	} else if (mutex->type == MTX_RECURSE) {
		recursive_lock_lock(&mutex->u.recursive);
	} else if (mutex->type == MTX_SPIN) {
		cpu_status status = disable_interrupts();
		acquire_spinlock(&mutex->u.spinlock.lock);
		mutex->u.spinlock.state = status;
	}
}


static inline int
mtx_trylock(struct mtx* mutex)
{
	if (mutex->type == MTX_DEF) {
		if (mutex_trylock(&mutex->u.mutex.lock) != B_OK)
			return 0;
		mutex->u.mutex.owner = find_thread(NULL);
		return 1;
	} else if (mutex->type == MTX_RECURSE) {
		if (recursive_lock_trylock(&mutex->u.recursive) != B_OK)
			return 0;
		return 1;
	} else if (mutex->type == MTX_SPIN) {
		return 0;
	}
	return 0;
}


static inline void
mtx_unlock(struct mtx* mutex)
{
	if (mutex->type == MTX_DEF) {
		mutex->u.mutex.owner = -1;
		mutex_unlock(&mutex->u.mutex.lock);
	} else if (mutex->type == MTX_RECURSE) {
		recursive_lock_unlock(&mutex->u.recursive);
	} else if (mutex->type == MTX_SPIN) {
		cpu_status status = mutex->u.spinlock.state;
		release_spinlock(&mutex->u.spinlock.lock);
		restore_interrupts(status);
	}
}


static inline int
mtx_initialized(struct mtx* mutex)
{
	/* TODO */
	return 1;
}


static inline int
mtx_owned(struct mtx* mutex)
{
	if (mutex->type == MTX_DEF)
		return mutex->u.mutex.owner == find_thread(NULL);
	if (mutex->type == MTX_RECURSE) {
#if KDEBUG
		return mutex->u.recursive.lock.holder == find_thread(NULL);
#else
		return mutex->u.recursive.holder == find_thread(NULL);
#endif
	}

	return 0;
}


struct mtx_args {
	void		*ma_mtx;
	const char 	*ma_desc;
	int		 ma_opts;
};

#define	MTX_SYSINIT(name, mtx, desc, opts) \
	static struct mtx_args name##_args = {	\
		(mtx), \
		(desc), \
		(opts), \
	}; \
	SYSINIT(name##_mtx, SI_SUB_LOCK, SI_ORDER_MIDDLE, \
		mtx_sysinit, &name##_args); \
	SYSUNINIT(name##_mtx, SI_SUB_LOCK, SI_ORDER_MIDDLE, \
	    (system_init_func_t)mtx_destroy, (void*)mtx)


#endif	/* _FBSD_COMPAT_SYS_MUTEX_H_ */
