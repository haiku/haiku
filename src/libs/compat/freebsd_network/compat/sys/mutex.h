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
	if (mutex->type == MTX_DEF)
		mutex_lock(&mutex->u.mutex);
	else if (mutex->type == MTX_RECURSE)
		recursive_lock_lock(&mutex->u.recursive);
}


static inline void
mtx_unlock(struct mtx* mutex)
{
	if (mutex->type == MTX_DEF)
		mutex_unlock(&mutex->u.mutex);
	else if (mutex->type == MTX_RECURSE)
		recursive_lock_unlock(&mutex->u.recursive);
}


static inline int
mtx_initialized(struct mtx* mutex)
{
	/* XXX */
	return 1;
}


static inline int
mtx_owned(struct mtx* mutex)
{
	if (mutex->type == MTX_DEF)
#if KDEBUG
		return mutex->u.mutex.holder == thread_get_current_thread_id();
#else
		return 0;
			// found no way how to determine the holder of the mutex
			// so we setting it to 0 because a starving thread is easier
			// to detect than a race condition; Colin Günther
#endif
	else if (mutex->type == MTX_RECURSE)
#if KDEBUG
		return mutex->u.recursive.lock.holder == thread_get_current_thread_id();
#else
		return mutex->u.recursive.holder == thread_get_current_thread_id();
#endif
	else
		return 0;
}

#endif	/* _FBSD_COMPAT_SYS_MUTEX_H_ */
