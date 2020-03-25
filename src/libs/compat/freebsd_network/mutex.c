/*
 * Copyright 2009, Colin Günther, coling@gmx.de.
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include "device.h"

#include <compat/sys/mutex.h>


struct mtx Giant;
struct rw_lock ifnet_rwlock;
struct mtx gIdStoreLock;


void
mtx_init(struct mtx *mutex, const char *name, const char *type,
	int options)
{
	if ((options & MTX_RECURSE) != 0) {
		recursive_lock_init_etc(&mutex->u.recursive, name,
			MUTEX_FLAG_CLONE_NAME);
		mutex->type = MTX_RECURSE;
	} else if ((options & MTX_SPIN) != 0) {
		B_INITIALIZE_SPINLOCK(&mutex->u.spinlock.lock);
		mutex->type = MTX_SPIN;
	} else {
		mutex_init_etc(&mutex->u.mutex.lock, name, MUTEX_FLAG_CLONE_NAME);
		mutex->u.mutex.owner = -1;
		mutex->type = MTX_DEF;
	}
}


void
mtx_sysinit(void *arg)
{
	struct mtx_args *margs = arg;

	mtx_init((struct mtx *)margs->ma_mtx, margs->ma_desc, NULL,
	    margs->ma_opts);
}


void
mtx_destroy(struct mtx *mutex)
{
	if ((mutex->type & MTX_RECURSE) != 0) {
		recursive_lock_destroy(&mutex->u.recursive);
	} else if ((mutex->type & MTX_SPIN) != 0) {
		KASSERT(!B_SPINLOCK_IS_LOCKED(&mutex->u.spinlock.lock), ("spin mutex is locked"));
	} else {
		mutex_destroy(&mutex->u.mutex.lock);
	}
}


status_t
init_mutexes()
{
	mtx_init(&Giant, "Banana Giant", NULL, MTX_DEF);
	rw_lock_init(&ifnet_rwlock, "gDevices");
	mtx_init(&gIdStoreLock, "Identity Store", NULL, MTX_DEF);

	return B_OK;
}


void
uninit_mutexes()
{
	mtx_destroy(&Giant);
	rw_lock_destroy(&ifnet_rwlock);
	mtx_destroy(&gIdStoreLock);
}
