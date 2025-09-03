/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2008-2010, Axel Dörfler. All Rights Reserved.
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 *
 * Distributed under the terms of the MIT License.
 */


#include "ObjectDepot.h"

#include <algorithm>

#include <interrupts.h>
#include <slab/Slab.h>
#include <smp.h>
#include <util/AutoLock.h>

#include "slab_debug.h"
#include "slab_private.h"
#include "slab_queue.h"


struct DepotMagazine : public slab_queue_link {
			uint16				current_round;
			uint16				round_count;
			void*				rounds[0];

public:
	inline	bool				IsEmpty() const;
	inline	bool				IsFull() const;

	inline	void*				Pop();
	inline	bool				Push(void* object);

#if PARANOID_KERNEL_FREE
			bool				ContainsObject(void* object) const;
#endif
};


struct depot_cpu_store {
	DepotMagazine*	loaded;
	DepotMagazine*	previous;
};


RANGE_MARKER_FUNCTION_BEGIN(SlabObjectDepot)


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


#if PARANOID_KERNEL_FREE

bool
DepotMagazine::ContainsObject(void* object) const
{
	for (uint16 i = 0; i < current_round; i++) {
		if (rounds[i] == object)
			return true;
	}

	return false;
}

#endif // PARANOID_KERNEL_FREE


// #pragma mark -


static DepotMagazine*
alloc_magazine(object_depot* depot, uint32 flags)
{
	DepotMagazine* magazine = (DepotMagazine*)slab_internal_alloc(
		sizeof(DepotMagazine) + depot->magazine_capacity * sizeof(void*),
		flags);
	if (magazine) {
		magazine->next = NULL;
		magazine->current_round = 0;
		magazine->round_count = depot->magazine_capacity;
	}

	return magazine;
}


static void
free_magazine(DepotMagazine* magazine, uint32 flags)
{
	slab_internal_free(magazine, flags);
}


static void
empty_magazine(object_depot* depot, DepotMagazine* magazine, uint32 flags)
{
	for (uint16 i = 0; i < magazine->current_round; i++)
		depot->return_object(depot, depot->cookie, magazine->rounds[i], flags);
	free_magazine(magazine, flags);
}


static bool
exchange_with_full(object_depot* depot, DepotMagazine*& magazine)
{
	ASSERT(magazine->IsEmpty());

	SpinLocker _(depot->inner_lock);

	if (depot->full.head == NULL)
		return false;

	depot->full_count--;
	depot->empty_count++;

	depot->empty.Push(magazine);
	magazine = (DepotMagazine*)depot->full.Pop();
	return true;
}


static bool
exchange_with_empty(object_depot* depot, DepotMagazine*& magazine,
	DepotMagazine*& freeMagazine)
{
	ASSERT(magazine == NULL || magazine->IsFull());

	SpinLocker _(depot->inner_lock);

	if (depot->empty.head == NULL)
		return false;

	depot->empty_count--;

	if (magazine != NULL) {
		if (depot->full_count < depot->max_count) {
			depot->full.Push(magazine);
			depot->full_count++;
			freeMagazine = NULL;
		} else
			freeMagazine = magazine;
	}

	magazine = (DepotMagazine*)depot->empty.Pop();
	return true;
}


static void
push_empty_magazine(object_depot* depot, DepotMagazine* magazine)
{
	SpinLocker _(depot->inner_lock);

	depot->empty.Push(magazine);
	depot->empty_count++;
}


static inline depot_cpu_store*
object_depot_cpu(object_depot* depot)
{
	return &depot->stores[smp_get_current_cpu()];
}


// #pragma mark - public API


status_t
object_depot_init(object_depot* depot, size_t capacity, size_t maxCount,
	uint32 flags, void* cookie, void (*return_object)(object_depot* depot,
		void* cookie, void* object, uint32 flags))
{
	depot->full.Init();
	depot->empty.Init();
	depot->full_count = depot->empty_count = 0;
	depot->max_count = maxCount;
	depot->magazine_capacity = capacity;

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
object_depot_destroy(object_depot* depot, uint32 flags)
{
	object_depot_make_empty(depot, flags);

	slab_internal_free(depot->stores, flags);

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


void
object_depot_store(object_depot* depot, void* object, uint32 flags)
{
	ReadLocker readLocker(depot->outer_lock);
	InterruptsLocker interruptsLocker;

	depot_cpu_store* store = object_depot_cpu(depot);

	// We try to add the object to the loaded magazine if we have one
	// and it's not full, or to the previous one if it is empty. If
	// the magazine depot doesn't provide us with a new empty magazine
	// we return the object directly to the slab.

	while (true) {
		if (store->loaded != NULL && store->loaded->Push(object))
			return;

		DepotMagazine* freeMagazine = NULL;
		if ((store->previous != NULL && store->previous->IsEmpty())
			|| exchange_with_empty(depot, store->previous, freeMagazine)) {
			std::swap(store->loaded, store->previous);

			if (freeMagazine != NULL) {
				// Free the magazine that didn't have space in the list
				interruptsLocker.Unlock();
				readLocker.Unlock();

				empty_magazine(depot, freeMagazine, flags);

				readLocker.Lock();
				interruptsLocker.Lock();

				store = object_depot_cpu(depot);
			}
		} else {
			// allocate a new empty magazine
			interruptsLocker.Unlock();
			readLocker.Unlock();

			DepotMagazine* magazine = alloc_magazine(depot, flags);
			if (magazine == NULL) {
				depot->return_object(depot, depot->cookie, object, flags);
				return;
			}

			readLocker.Lock();
			interruptsLocker.Lock();

			push_empty_magazine(depot, magazine);
			store = object_depot_cpu(depot);
		}
	}
}


void
object_depot_make_empty(object_depot* depot, uint32 flags)
{
	WriteLocker writeLocker(depot->outer_lock);

	// collect the store magazines

	slab_queue storeMagazines;
	storeMagazines.Init();

	int cpuCount = smp_get_num_cpus();
	for (int i = 0; i < cpuCount; i++) {
		depot_cpu_store& store = depot->stores[i];

		if (store.loaded != NULL) {
			storeMagazines.Push(store.loaded);
			store.loaded = NULL;
		}

		if (store.previous != NULL) {
			storeMagazines.Push(store.previous);
			store.previous = NULL;
		}
	}

	// detach the depot's full and empty magazines

	slab_queue fullMagazines = depot->full;
	depot->full.head = depot->full.tail = NULL;

	slab_queue emptyMagazines = depot->empty;
	depot->empty.head = depot->empty.tail = NULL;

	writeLocker.Unlock();

	// free all magazines

	while (storeMagazines.head != NULL)
		empty_magazine(depot, (DepotMagazine*)storeMagazines.Pop(), flags);

	while (fullMagazines.head != NULL)
		empty_magazine(depot, (DepotMagazine*)fullMagazines.Pop(), flags);

	while (emptyMagazines.head != NULL)
		free_magazine((DepotMagazine*)emptyMagazines.Pop(), flags);
}


#if PARANOID_KERNEL_FREE

bool
object_depot_contains_object(object_depot* depot, void* object)
{
	WriteLocker writeLocker(depot->outer_lock);

	int cpuCount = smp_get_num_cpus();
	for (int i = 0; i < cpuCount; i++) {
		depot_cpu_store& store = depot->stores[i];

		if (store.loaded != NULL && !store.loaded->IsEmpty()) {
			if (store.loaded->ContainsObject(object))
				return true;
		}

		if (store.previous != NULL && !store.previous->IsEmpty()) {
			if (store.previous->ContainsObject(object))
				return true;
		}
	}

	for (DepotMagazine* magazine = (DepotMagazine*)depot->full.head; magazine != NULL;
			magazine = (DepotMagazine*)magazine->next) {
		if (magazine->ContainsObject(object))
			return true;
	}

	return false;
}

#endif // PARANOID_KERNEL_FREE


// #pragma mark - private kernel API


void
dump_object_depot(object_depot* depot)
{
	kprintf("  full:     %p, count %lu\n", depot->full.head, depot->full_count);
	kprintf("  empty:    %p, count %lu\n", depot->empty.head, depot->empty_count);
	kprintf("  max full: %lu\n", depot->max_count);
	kprintf("  capacity: %lu\n", depot->magazine_capacity);
	kprintf("  stores:\n");

	int cpuCount = smp_get_num_cpus();

	for (int i = 0; i < cpuCount; i++) {
		kprintf("  [%d] loaded:   %p\n", i, depot->stores[i].loaded);
		kprintf("      previous: %p\n", depot->stores[i].previous);
	}
}


int
dump_object_depot(int argCount, char** args)
{
	if (argCount != 2)
		kprintf("usage: %s [address]\n", args[0]);
	else
		dump_object_depot((object_depot*)parse_expression(args[1]));

	return 0;
}


int
dump_depot_magazine(int argCount, char** args)
{
	if (argCount != 2) {
		kprintf("usage: %s [address]\n", args[0]);
		return 0;
	}

	DepotMagazine* magazine = (DepotMagazine*)parse_expression(args[1]);

	kprintf("next:          %p\n", magazine->next);
	kprintf("current_round: %u\n", magazine->current_round);
	kprintf("round_count:   %u\n", magazine->round_count);

	for (uint16 i = 0; i < magazine->current_round; i++)
		kprintf("  [%i] %p\n", i, magazine->rounds[i]);

	return 0;
}


RANGE_MARKER_FUNCTION_END(SlabObjectDepot)
