/*
 * Copyright 2009, Colin Günther, coling@gmx.de.
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include "device.h"

#include <compat/sys/mutex.h>


// these methods are bit unfriendly, a bit too much panic() around

struct mtx Giant;
struct rw_lock ifnet_rwlock;
struct mtx gIdStoreLock;


void
mtx_init(struct mtx *mutex, const char *name, const char *type,
	int options)
{
	if (options == MTX_DEF) {
		mutex_init_etc(&mutex->u.mutex.lock, name, MUTEX_FLAG_CLONE_NAME);
		mutex->u.mutex.owner = -1;
	} else if (options == MTX_RECURSE) {
		recursive_lock_init_etc(&mutex->u.recursive, name,
			MUTEX_FLAG_CLONE_NAME);
	} else
		panic("fbsd: unsupported mutex type");

	mutex->type = options;
}


void
mtx_destroy(struct mtx *mutex)
{
	if (mutex->type == MTX_DEF)
		mutex_destroy(&mutex->u.mutex.lock);
	else if (mutex->type == MTX_RECURSE)
		recursive_lock_destroy(&mutex->u.recursive);
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
