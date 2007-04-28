/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Hugo Santos, hugosantos@gmail.com
 */

#ifndef _SLAB_DEPOT_H_
#define _SLAB_DEPOT_H_

#include <stdint.h>

#include <lock.h>
#include <smp.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct base_depot {
	benaphore lock;
	struct depot_magazine *full, *empty;
	size_t full_count, empty_count;
	struct depot_cpu_store *stores;

	void (*return_object)(struct base_depot *depot, void *object);
} base_depot;


typedef struct depot_cpu_store {
	benaphore lock;
	struct depot_magazine *loaded, *previous;
} depot_cpu_store;


static inline depot_cpu_store *
base_depot_cpu(base_depot *depot)
{
	return &depot->stores[smp_get_current_cpu()];
}


status_t base_depot_init(base_depot *depot,
	void (*return_object)(base_depot *, void *));
void base_depot_destroy(base_depot *depot);
void *base_depot_obtain_from_store(base_depot *depot, depot_cpu_store *store);
int base_depot_return_to_store(base_depot *depot, depot_cpu_store *store,
	void *object);
void base_depot_make_empty(base_depot *depot);

#ifdef __cplusplus
}
#endif

#endif
