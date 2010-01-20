/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2008, Axel Dörfler. All Rights Reserved.
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 *
 * Distributed under the terms of the MIT License.
 */


#include <slab/ObjectDepot.h>

#include <algorithm>

#include <int.h>
#include <smp.h>
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
	SpinLocker _(depot->inner_lock);

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
	SpinLocker _(depot->inner_lock);

	if (depot->empty == NULL)
		return false;

	depot->empty_count--;

	if (magazine) {
		_push(depot->full, magazine);
		depot->full_count++;
	}

	magazine = _pop(depot->empty);
	return true;
}


static void
push_empty_magazine(object_depot* depot, DepotMagazine* magazine)
{
	SpinLocker _(depot->inner_lock);

	_push(depot->empty, magazine);
}


static inline depot_cpu_store*
object_depot_cpu(object_depot* depot)
{
	return &depot->stores[smp_get_current_cpu()];
}


// #pragma mark - public API


status_t
object_depot_init(object_depot* depot, uint32 flags, void* cookie,
	void (*return_object)(object_depot* depot, void* cookie, void* object))
{
	depot->full = NULL;
	depot->empty = NULL;
	depot->full_count = depot->empty_count = 0;

	rw_lock_init(&depot->outer_lock, "object depot");
	B_INITIALIZE_SPINLOCK(&depot->inner_lock);

	int cpuCount = smp_get_num_cpus();
	depot->stores = (depot_cpu_store*)slab_internal_alloc(
		sizeof(depot_cpu_store) * cpuCount, flags);
	if (depot->stores == NULL) {
		rw_lock_destroy(&depot->outer_lock);
		return B_NO_MEMORY;
	}

	for (int i = 0; i < cpuCount; i++) {
		depot->stores[i].loaded = NULL;
		depot->stores[i].previous = NULL;
	}

	depot->cookie = cookie;
	depot->return_object = return_object;

	return B_OK;
}


void
object_depot_destroy(object_depot* depot)
{
	object_depot_make_empty(depot);

	slab_internal_free(depot->stores);

	rw_lock_destroy(&depot->outer_lock);
}


void*
object_depot_obtain(object_depot* depot)
{
	ReadLocker readLocker(depot->outer_lock);
	InterruptsLocker interruptsLocker;

	depot_cpu_store* store = object_depot_cpu(depot);

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


int
object_depot_store(object_depot* depot, void* object)
{
	ReadLocker readLocker(depot->outer_lock);
	InterruptsLocker interruptsLocker;

	depot_cpu_store* store = object_depot_cpu(depot);

	// We try to add the object to the loaded magazine if we have one
	// and it's not full, or to the previous one if it is empty. If
	// the magazine depot doesn't provide us with a new empty magazine
	// we return the object directly to the slab.

	while (true) {
		if (store->loaded && store->loaded->Push(object))
			return 1;

		if ((store->previous && store->previous->IsEmpty())
			|| exchange_with_empty(depot, store->previous)) {
			std::swap(store->loaded, store->previous);
		} else {
			// allocate a new empty magazine
			interruptsLocker.Unlock();
			readLocker.Unlock();

			DepotMagazine* magazine = alloc_magazine();
			if (magazine == NULL)
				return 0;

			readLocker.Lock();
			interruptsLocker.Lock();

			push_empty_magazine(depot, magazine);
			store = object_depot_cpu(depot);
		}
	}
}


void
object_depot_make_empty(object_depot* depot)
{
	WriteLocker writeLocker(depot->outer_lock);

	// collect the store magazines

	DepotMagazine* storeMagazines = NULL;

	int cpuCount = smp_get_num_cpus();
	for (int i = 0; i < cpuCount; i++) {
		depot_cpu_store& store = depot->stores[i];

		if (store.loaded) {
			_push(storeMagazines, store.loaded);
			store.loaded = NULL;
		}

		if (store.previous) {
			_push(storeMagazines, store.previous);
			store.previous = NULL;
		}
	}

	// detach the depot's full and empty magazines

	DepotMagazine* fullMagazines = depot->full;
	depot->full = NULL;

	DepotMagazine* emptyMagazines = depot->empty;
	depot->empty = NULL;

	writeLocker.Unlock();

	// free all magazines

	while (storeMagazines != NULL)
		empty_magazine(depot, _pop(storeMagazines));

	while (fullMagazines != NULL)
		empty_magazine(depot, _pop(fullMagazines));

	while (emptyMagazines)
		free_magazine(_pop(emptyMagazines));
}
