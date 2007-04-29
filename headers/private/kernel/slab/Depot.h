/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Hugo Santos, hugosantos@gmail.com
 */

#ifndef _SLAB_DEPOT_H_
#define _SLAB_DEPOT_H_

#include <lock.h>
#include <KernelExport.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct object_depot {
	benaphore lock;
	struct depot_magazine *full, *empty;
	size_t full_count, empty_count;
	struct depot_cpu_store *stores;

	void (*return_object)(struct object_depot *depot, void *object);
} object_depot;


status_t object_depot_init(object_depot *depot, uint32 flags,
	void (*return_object)(object_depot *, void *));
void object_depot_destroy(object_depot *depot);

void *object_depot_obtain(object_depot *depot);
int object_depot_store(object_depot *depot, void *object);

void object_depot_make_empty(object_depot *depot);

#ifdef __cplusplus
}
#endif

#endif
