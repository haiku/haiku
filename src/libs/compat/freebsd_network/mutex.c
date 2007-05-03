/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Hugo Santos, hugosantos@gmail.com
 */

#include <KernelExport.h>

#include <compat/sys/mutex.h>


status_t init_mutexes(void);
void uninit_mutexes(void);


// these methods are bit unfriendly, a bit too much panic() around

struct mutex Giant;


void
mtx_init(struct mtx *m, const char *name, const char *type, int opts)
{
	if (opts == MTX_DEF) {
		if (mutex_init(&m->u.mutex, name) < B_OK)
			panic("Panic! Dance like it's 1979, we ran out of semaphores");
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
	return B_ERROR;
}


void
uninit_mutexes()
{
}

