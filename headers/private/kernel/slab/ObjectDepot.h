/*
 * Copyright 2010, Axel Dörfler. All Rights Reserved.
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SLAB_OBJECT_DEPOT_H_
#define _SLAB_OBJECT_DEPOT_H_


#include <lock.h>
#include <KernelExport.h>


struct DepotMagazine;

typedef struct object_depot {
	rw_lock					outer_lock;
	spinlock				inner_lock;
	DepotMagazine*			full;
	DepotMagazine*			empty;
	size_t					full_count;
	size_t					empty_count;
	size_t					max_count;
	size_t					magazine_capacity;
	struct depot_cpu_store*	stores;
	void*					cookie;

	void (*return_object)(struct object_depot* depot, void* cookie,
		void* object, uint32 flags);
} object_depot;


#ifdef __cplusplus
extern "C" {
#endif

status_t object_depot_init(object_depot* depot, size_t capacity,
	size_t maxCount, uint32 flags, void* cookie,
	void (*returnObject)(object_depot* depot, void* cookie, void* object,
		uint32 flags));
void object_depot_destroy(object_depot* depot, uint32 flags);

void* object_depot_obtain(object_depot* depot);
void object_depot_store(object_depot* depot, void* object, uint32 flags);

void object_depot_make_empty(object_depot* depot, uint32 flags);

#if PARANOID_KERNEL_FREE
bool object_depot_contains_object(object_depot* depot, void* object);
#endif

#ifdef __cplusplus
}
#endif


#endif	/* _SLAB_OBJECT_DEPOT_H_ */
