/*
 * Copyright 2008, Axel Dörfler. All Rights Reserved.
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 *
 * Distributed under the terms of the MIT License.
 */


#include <slab/ObjectDepot.h>

#include <algorithm>

#include <util/AutoLock.h>

#include "slab_private.h"


static const int kMagazineCapacity = 32;
	// TODO: Should be dynamically tuned per cache.


struct DepotMagazine {
			DepotMagazine*		next;
			uint16				current_round;
			uint16				round_count;
			void*				rounds[0];

public:
	inline	bool				IsEmpty() const;
	inline	bool				IsFull() const;

	inline	void*				Pop();
	inline	bool				Push(void* object);
};


struct depot_cpu_store {
	recursive_lock	lock;
	DepotMagazine*	loaded;
	DepotMagazine*	previous;
};


bool
DepotMagazine::IsEmpty() const
{
	return current_round == 0;
}


bool
DepotMagazine::IsFull() const
{
	return current_round == round_count;
}


void*
DepotMagazine::Pop()
{
	return rounds[--current_round];
}


bool
DepotMagazine::Push(void* object)
{
	if (IsFull())
		return false;

	rounds[current_round++] = object;
	return true;
}


static DepotMagazine*
alloc_magazine()
{
	DepotMagazine* magazine = (DepotMagazine*)slab_internal_alloc(
		sizeof(DepotMagazine) + kMagazineCapacity * sizeof(void*), 0);
	if (magazine) {
		magazine->next = NULL;
		magazine->current_round = 0;
		magazine->round_count = kMagazineCapacity;
	}

	return magazine;
}


static void
free_magazine(DepotMagazine* magazine)
{
	slab_internal_free(magazine);
}


static void
empty_magazine(object_depot* depot, DepotMagazine* magazine)
{
	for (uint16 i = 0; i < magazine->current_round; i++)
		depot->return_object(depot, depot->cookie, magazine->rounds[i]);
	free_magazine(magazine);
}


static bool
exchange_with_full(object_depot* depot, DepotMagazine*& magazine)
{
	RecursiveLocker _(depot->lock);

	if (depot->full == NULL)
		return false;

	depot->full_count--;
	depot->empty_count++;

	_push(depot->empty, magazine);
	magazine = _pop(depot->full);
	return true;
}


static bool
exchange_with_empty(object_depot* depot, DepotMagazine*& magazine)
{
	RecursiveLocker _(depot->lock);

	if (depot->empty == NULL) {
		depot->empty = alloc_magazine();
		if (depot->empty == NULL)
			return false;
	} else {
		depot->empty_count--;
	}

	if (magazine) {
		_push(depot->full, magazine);
		depot->full_count++;
	}

	magazine = _pop(depot->empty);
	return true;
}


static inline depot_cpu_store*
object_depot_cpu(object_depot* depot)
{
	return &depot->stores[smp_get_current_cpu()];
}


// #pragma mark -


status_t
object_depot_init(object_depot* depot, uint32 flags, void* cookie,
	void (*return_object)(object_depot* depot, void* cookie, void* object))
{
	depot->full = NULL;
	depot->empty = NULL;
	depot->full_count = depot->empty_count = 0;

	recursive_lock_init(&depot->lock, "depot");

	int cpuCount = smp_get_num_cpus();
	depot->stores = (depot_cpu_store*)slab_internal_alloc(
		sizeof(depot_cpu_store) * cpuCount, flags);
	if (depot->stores == NULL) {
		recursive_lock_destroy(&depot->lock);
		return B_NO_MEMORY;
	}

	for (int i = 0; i < cpuCount; i++) {
		recursive_lock_init(&depot->stores[i].lock, "cpu store");
		depot->stores[i].loaded = depot->stores[i].previous = NULL;
	}

	depot->cookie = cookie;
	depot->return_object = return_object;

	return B_OK;
}


void
object_depot_destroy(object_depot* depot)
{
	object_depot_make_empty(depot);

	int cpuCount = smp_get_num_cpus();
	for (int i = 0; i < cpuCount; i++)
		recursive_lock_destroy(&depot->stores[i].lock);

	slab_internal_free(depot->stores);

	recursive_lock_destroy(&depot->lock);
}


static void*
object_depot_obtain_from_store(object_depot* depot, depot_cpu_store* store)
{
	RecursiveLocker _(store->lock);

	// To better understand both the Alloc() and Free() logic refer to
	// Bonwick's ``Magazines and Vmem'' [in 2001 USENIX proceedings]

	// In a nutshell, we try to get an object from the loaded magazine
	// if it's not empty, or from the previous magazine if it's full
	// and finally from the Slab if the magazine depot has no full magazines.

	if (store->loaded == NULL)
		return NULL;

	while (true) {
		if (!store->loaded->IsEmpty())
			return store->loaded->Pop();

		if (store->previous
			&& (store->previous->IsFull()
				|| exchange_with_full(depot, store->previous))) {
			std::swap(store->previous, store->loaded);
		} else
			return NULL;
	}
}


static int
object_depot_return_to_store(object_depot* depot, depot_cpu_store* store,
	void* object)
{
	RecursiveLocker _(store->lock);

	// We try to add the object to the loaded magazine if we have one
	// and it's not full, or to the previous one if it is empty. If
	// the magazine depot doesn't provide us with a new empty magazine
	// we return the object directly to the slab.

	while (true) {
		if (store->loaded && store->loaded->Push(object))
			return 1;

		if ((store->previous && store->previous->IsEmpty())
			|| exchange_with_empty(depot, store->previous))
			std::swap(store->loaded, store->previous);
		else
			return 0;
	}
}


void*
object_depot_obtain(object_depot* depot)
{
	return object_depot_obtain_from_store(depot, object_depot_cpu(depot));
}


int
object_depot_store(object_depot* depot, void* object)
{
	return object_depot_return_to_store(depot, object_depot_cpu(depot),
		object);
}


void
object_depot_make_empty(object_depot* depot)
{
	int cpuCount = smp_get_num_cpus();
	for (int i = 0; i < cpuCount; i++) {
		depot_cpu_store* store = &depot->stores[i];

		RecursiveLocker _(store->lock);

		if (depot->stores[i].loaded)
			empty_magazine(depot, depot->stores[i].loaded);
		if (depot->stores[i].previous)
			empty_magazine(depot, depot->stores[i].previous);
		depot->stores[i].loaded = depot->stores[i].previous = NULL;
	}

	RecursiveLocker _(depot->lock);

	while (depot->full)
		empty_magazine(depot, _pop(depot->full));

	while (depot->empty)
		empty_magazine(depot, _pop(depot->empty));
}
