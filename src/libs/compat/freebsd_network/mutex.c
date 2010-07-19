/*
 * Copyright 2009, Colin Günther, coling@gmx.de.
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include "device.h"

#include <compat/sys/mutex.h>


// these methods are bit unfriendly, a bit too much panic() around

struct mtx Giant;
struct mtx ifnet_lock;
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
	mtx_init(&ifnet_lock, "gDevices", NULL, MTX_DEF);
	mtx_init(&gIdStoreLock, "Identity Store", NULL, MTX_DEF);

	return B_OK;
}


void
uninit_mutexes()
{
	mtx_destroy(&Giant);
	mtx_destroy(&ifnet_lock);
	mtx_destroy(&gIdStoreLock);
}
