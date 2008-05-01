/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Hugo Santos, hugosantos@gmail.com
 */

#include "device.h"

#include <compat/sys/mutex.h>


// these methods are bit unfriendly, a bit too much panic() around

struct mtx Giant;


void
mtx_init(struct mtx *m, const char *name, const char *type, int opts)
{
	if (opts == MTX_DEF) {
		mutex_init_etc(&m->u.mutex, name, MUTEX_FLAG_CLONE_NAME);
	} else if (opts == MTX_RECURSE) {
		if (recursive_lock_init(&m->u.recursive, name) < B_OK)
			panic("Hell just froze as someone was trying to init a recursive mutex.");
	} else
		panic("Uh-oh, someone is pressing the wrong buttons");

	m->type = opts;
}


void
mtx_destroy(struct mtx *m)
{
	if (m->type == MTX_DEF) {
		mutex_destroy(&m->u.mutex);
	} else if (m->type == MTX_RECURSE) {
		recursive_lock_destroy(&m->u.recursive);
	} else
		panic("Uh-oh, someone is pressing the wrong buttons");
}


status_t
init_mutexes()
{
	mtx_init(&Giant, "Banana Giant", NULL, MTX_DEF);

	return B_OK;
}


void
uninit_mutexes()
{
	mtx_destroy(&Giant);
}

