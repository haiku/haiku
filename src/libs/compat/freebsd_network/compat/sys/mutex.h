/*
 * Copyright 2009, Colin Günther, coling@gmx.de.
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FBSD_COMPAT_SYS_MUTEX_H_
#define _FBSD_COMPAT_SYS_MUTEX_H_


#include <sys/haiku-module.h>

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

#define MTX_DEF				0x0000
#define MTX_RECURSE			0x0004

#define MTX_NETWORK_LOCK	"network driver"

#define NET_LOCK_GIANT()
#define NET_UNLOCK_GIANT()


extern struct mtx Giant;


void mtx_init(struct mtx*, const char*, const char*, int);
void mtx_destroy(struct mtx*);


static inline void
mtx_lock(struct mtx* mutex)
{
	if (mutex->type == MTX_DEF) {
		mutex_lock(&mutex->u.mutex.lock);
		mutex->u.mutex.owner = find_thread(NULL);
	} else if (mutex->type == MTX_RECURSE)
		recursive_lock_lock(&mutex->u.recursive);
}


static inline void
mtx_unlock(struct mtx* mutex)
{
	if (mutex->type == MTX_DEF) {
		mutex->u.mutex.owner = -1;
		mutex_unlock(&mutex->u.mutex.lock);
	} else if (mutex->type == MTX_RECURSE)
		recursive_lock_unlock(&mutex->u.recursive);
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


#endif	/* _FBSD_COMPAT_SYS_MUTEX_H_ */
