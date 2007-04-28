/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Hugo Santos, hugosantos@gmail.com
 */

#include <Slab.h>

#include <string.h>

#include <KernelExport.h>
#include <util/AutoLock.h>

#include <algorithm> // swap
#include <new>

// TODO all of the small allocations we perform here will fallback
//      to the internal allocator which in the future will use this
//      same code. We'll have to resolve all of the dependencies
//      then, for now, it is still not required.

//#define TRACE_SLAB

#ifdef TRACE_SLAB
#define TRACE_CACHE(cache, format, args...) \
	dprintf("Cache[%p, %s] " format "\n", cache, cache->name , ##args)
#else
#define TRACE_CACHE(cache, format, bananas...) do { } while (0)
#endif


// TODO this value should be dynamically tuned per cache.
static const int kMagazineCapacity = 32;

static const size_t kCacheColorPeriod = 8;

typedef struct cache_object_link {
	struct cache_object_link *next;
} cache_object_link;


static depot_magazine *alloc_magazine();
static void free_magazine(depot_magazine *magazine);

template<typename Type> static Type *
_pop(Type* &head)
{
	Type *oldHead = head;
	head = head->next;
	return oldHead;
}


template<typename Type> static inline void
_push(Type* &head, Type *object)
{
	object->next = head;
	head = object;
}


static inline void *
link_to_object(cache_object_link *link, size_t objectSize)
{
	return ((uint8_t *)link) - (objectSize - sizeof(cache_object_link));
}


static inline cache_object_link *
object_to_link(void *object, size_t objectSize)
{
	return (cache_object_link *)(((uint8_t *)object)
		+ (objectSize - sizeof(cache_object_link)));
}


status_t
slab_area_backend_allocate(base_cache *cache, area_id *id, void **pages,
	size_t byteCount, uint32_t flags)
{
	if (flags & CACHE_ALIGN_TO_TOTAL && byteCount > B_PAGE_SIZE)
		return NULL;

	TRACE_CACHE(cache, "allocate pages (%lu, 0x0%lx)", byteCount, flags);

	area_id areaId = create_area(cache->name, pages,
		B_ANY_KERNEL_ADDRESS, byteCount, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
	if (areaId < 0)
		return areaId;

	TRACE_CACHE(cache, "  ... = { %ld, %p }", areaId, *pages);

	*id = areaId;
	return B_OK;
}

void
slab_area_backend_free(base_cache *cache, area_id area)
{
	TRACE_CACHE(cache, "delete pages %ld", area);
	delete_area(area);
}


void
base_cache_init(base_cache *cache, const char *name, size_t objectSize,
	size_t alignment, base_cache_constructor constructor,
	base_cache_destructor destructor, void *cookie)
{
	strlcpy(cache->name, name, sizeof(cache->name));

	if (objectSize < sizeof(void *) && alignment < sizeof(void *))
		objectSize = sizeof(void *);

	if (alignment > 0 && (objectSize & (alignment - 1)))
		cache->object_size = objectSize + alignment
			- (objectSize & (alignment - 1));
	else
		cache->object_size = objectSize;

	TRACE_CACHE(cache, "init %lu, %lu -> %lu", objectSize, alignment,
		cache->object_size);

	cache->cache_color_cycle = 0;

	list_init_etc(&cache->empty, offsetof(cache_slab, link));
	list_init_etc(&cache->partial, offsetof(cache_slab, link));
	list_init_etc(&cache->full, offsetof(cache_slab, link));

	cache->empty_count = 0;

	// pressure is increased whenever we need a slab and don't have one
	cache->pressure = 0;

	cache->constructor = constructor;
	cache->destructor = destructor;
	cache->cookie = cookie;
}


void
base_cache_destroy(base_cache *cache,
	void (*return_slab)(base_cache *, cache_slab *))
{
	if (!list_is_empty(&cache->full))
		panic("cache destroy: still has full slabs");

	if (!list_is_empty(&cache->partial))
		panic("cache destroy: still has partial slabs");

	while (!list_is_empty(&cache->empty)) {
		cache_slab *slab = (cache_slab *)list_remove_head_item(&cache->empty);
		return_slab(cache, slab);
	}

	cache->empty_count = 0;
}


void
base_cache_low_memory(base_cache *cache, int32 level,
	void (*return_slab)(base_cache *, cache_slab *))
{
	size_t minimumAllowed;

	// only thing we can do right now is free up empty slabs

	switch (level) {
	case B_LOW_MEMORY_NOTE:
		minimumAllowed = cache->pressure / 2 + 1;
		break;

	case B_LOW_MEMORY_WARNING:
		cache->pressure /= 2;
		minimumAllowed = 0;
		break;

	default:
		cache->pressure = 0;
		minimumAllowed = 0;
		break;
	}

	if (cache->empty_count <= minimumAllowed)
		return;

	TRACE_CACHE(cache, "cache: memory pressure, will release down to %lu.",
		minimumAllowed);

	while (cache->empty_count > minimumAllowed) {
		cache_slab *slab = (cache_slab *)list_remove_head_item(&cache->empty);
		return_slab(cache, slab);
		cache->empty_count--;
	}
}


void *
base_cache_allocate_object(base_cache *cache)
{
	cache_slab *slab;

	if (list_is_empty(&cache->partial)) {
		if (list_is_empty(&cache->empty)) {
			cache->pressure++;
			return NULL;
		}

		cache->empty_count--;
		slab = (cache_slab *)list_remove_head_item(&cache->empty);
		list_add_item(&cache->partial, slab);
	} else
		slab = (cache_slab *)list_get_first_item(&cache->partial);

	cache_object_link *link = _pop(slab->free);
	slab->count--;

	TRACE_CACHE(cache, "allocate %p from %p, %lu remaining.", link, slab,
		slab->count);

	if (slab->count == 0) {
		// move the partial slab to the full list
		list_remove_item(&cache->partial, slab);
		list_add_item(&cache->full, slab);
	}

	return link_to_object(link, cache->object_size);
}


void *
base_cache_allocate_object_with_new_slab(base_cache *cache,
	cache_slab *newSlab)
{
	list_add_item(&cache->partial, newSlab);
	return base_cache_allocate_object(cache);
}


int
base_cache_return_object(base_cache *cache, cache_slab *slab,
	void *object)
{
	// We return true if the slab is completely unused.
	cache_object_link *link = object_to_link(object, cache->object_size);

	TRACE_CACHE(cache, "returning %p to %p, %lu used (%lu empty slabs).",
		link, slab, slab->size - slab->count, cache->empty_count);

	_push(slab->free, link);
	slab->count++;
	if (slab->count == slab->size) {
		list_remove_item(&cache->partial, slab);

		if (cache->empty_count >= cache->pressure)
			return 1;

		cache->empty_count++;
		list_add_item(&cache->empty, slab);
	} else if (slab->count == 1) {
		list_remove_item(&cache->full, slab);
		list_add_item(&cache->partial, slab);
	}

	return 0;
}


cache_slab *
base_cache_construct_slab(base_cache *cache, cache_slab *slab, void *pages,
	size_t byteCount, void *parent,
	base_cache_owner_prepare prepare, base_cache_owner_unprepare unprepare)
{
	TRACE_CACHE(cache, "construct (%p, %p, %lu)", slab, pages, byteCount);

	slab->pages = pages;
	slab->count = slab->size = byteCount / cache->object_size;
	slab->free = NULL;

	size_t spareBytes = byteCount - (slab->size * cache->object_size);
	slab->offset = cache->cache_color_cycle;

	if (slab->offset > spareBytes)
		cache->cache_color_cycle = slab->offset = 0;
	else
		cache->cache_color_cycle += kCacheColorPeriod;

	TRACE_CACHE(cache, "  %lu objects, %lu spare bytes, offset %lu",
		slab->size, spareBytes, slab->offset);

	uint8_t *data = ((uint8_t *)pages) + slab->offset;

	for (size_t i = 0; i < slab->size; i++) {
		if (prepare || cache->constructor) {
			bool failedOnFirst = false;
			status_t status = B_OK;

			if (prepare)
				status = prepare(parent, slab, data);
			if (status < B_OK)
				failedOnFirst = true;
			else if (cache->constructor)
				status = cache->constructor(cache->cookie, data);

			if (status < B_OK) {
				if (!failedOnFirst && unprepare)
					unprepare(parent, slab, data);

				data = ((uint8_t *)pages) + slab->offset;
				for (size_t j = 0; j < i; j++) {
					if (cache->destructor)
						cache->destructor(cache->cookie, data);
					if (unprepare)
						unprepare(parent, slab, data);
					data += cache->object_size;
				}

				return NULL;
			}
		}

		_push(slab->free, object_to_link(data, cache->object_size));
		data += cache->object_size;
	}

	return slab;
}


void
base_cache_destruct_slab(base_cache *cache, cache_slab *slab, void *parent,
	base_cache_owner_unprepare unprepare)
{
	TRACE_CACHE(cache, "destruct %p", slab);

	if (slab->count != slab->size)
		panic("cache: destroying a slab which isn't empty.");

	uint8_t *data = ((uint8_t *)slab->pages) + slab->offset;

	for (size_t i = 0; i < slab->size; i++) {
		if (cache->destructor)
			cache->destructor(cache->cookie, data);
		if (unprepare)
			unprepare(parent, slab, data);
		data += cache->object_size;
	}
}


static inline bool
is_magazine_empty(depot_magazine *magazine)
{
	return magazine->current_round == 0;
}


static inline bool
is_magazine_full(depot_magazine *magazine)
{
	return magazine->current_round == magazine->round_count;
}


static inline void *
pop_magazine(depot_magazine *magazine)
{
	return magazine->rounds[--magazine->current_round];
}


static inline bool
push_magazine(depot_magazine *magazine, void *object)
{
	if (is_magazine_full(magazine))
		return false;
	magazine->rounds[magazine->current_round++] = object;
	return true;
}


static bool
exchange_with_full(base_depot *depot, depot_magazine* &magazine)
{
	BenaphoreLocker _(depot->lock);

	if (depot->full == NULL)
		return false;

	depot->full_count--;
	depot->empty_count++;

	_push(depot->empty, magazine);
	magazine = _pop(depot->full);
	return true;
}


static bool
exchange_with_empty(base_depot *depot, depot_magazine* &magazine)
{
	BenaphoreLocker _(depot->lock);

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


static depot_magazine *
alloc_magazine()
{
	depot_magazine *magazine = (depot_magazine *)malloc(sizeof(depot_magazine)
		+ kMagazineCapacity * sizeof(void *));
	if (magazine) {
		magazine->next = NULL;
		magazine->current_round = 0;
		magazine->round_count = kMagazineCapacity;
	}

	return magazine;
}


static void
free_magazine(depot_magazine *magazine)
{
	free(magazine);
}


static void
empty_magazine(base_depot *depot, depot_magazine *magazine)
{
	for (uint16_t i = 0; i < magazine->current_round; i++)
		depot->return_object(depot, magazine->rounds[i]);
	free_magazine(magazine);
}


status_t
base_depot_init(base_depot *depot,
	void (*return_object)(base_depot *depot, void *object))
{
	depot->full = NULL;
	depot->empty = NULL;
	depot->full_count = depot->empty_count = 0;

	status_t status = benaphore_init(&depot->lock, "depot");
	if (status < B_OK)
		return status;

	depot->stores = new (std::nothrow) depot_cpu_store[smp_get_num_cpus()];
	if (depot->stores == NULL) {
		benaphore_destroy(&depot->lock);
		return B_NO_MEMORY;
	}

	for (int i = 0; i < smp_get_num_cpus(); i++) {
		benaphore_init(&depot->stores[i].lock, "cpu store");
		depot->stores[i].loaded = depot->stores[i].previous = NULL;
	}

	depot->return_object = return_object;

	return B_OK;
}


void
base_depot_destroy(base_depot *depot)
{
	base_depot_make_empty(depot);

	for (int i = 0; i < smp_get_num_cpus(); i++) {
		benaphore_destroy(&depot->stores[i].lock);
	}

	delete [] depot->stores;

	benaphore_destroy(&depot->lock);
}


void *
base_depot_obtain_from_store(base_depot *depot, depot_cpu_store *store)
{
	BenaphoreLocker _(store->lock);

	// To better understand both the Alloc() and Free() logic refer to
	// Bonwick's ``Magazines and Vmem'' [in 2001 USENIX proceedings]

	// In a nutshell, we try to get an object from the loaded magazine
	// if it's not empty, or from the previous magazine if it's full
	// and finally from the Slab if the magazine depot has no full magazines.

	if (store->loaded == NULL)
		return NULL;

	while (true) {
		if (!is_magazine_empty(store->loaded))
			return pop_magazine(store->loaded);

		if (store->previous && (is_magazine_full(store->previous)
			|| exchange_with_full(depot, store->previous)))
			std::swap(store->previous, store->loaded);
		else
			return NULL;
	}
}


int
base_depot_return_to_store(base_depot *depot, depot_cpu_store *store,
	void *object)
{
	BenaphoreLocker _(store->lock);

	// We try to add the object to the loaded magazine if we have one
	// and it's not full, or to the previous one if it is empty. If
	// the magazine depot doesn't provide us with a new empty magazine
	// we return the object directly to the slab.

	while (true) {
		if (store->loaded && push_magazine(store->loaded, object))
			return 1;

		if ((store->previous && is_magazine_empty(store->previous))
			|| exchange_with_empty(depot, store->previous))
			std::swap(store->loaded, store->previous);
		else
			return 0;
	}
}


void
base_depot_make_empty(base_depot *depot)
{
	// TODO locking

	for (int i = 0; i < smp_get_num_cpus(); i++) {
		if (depot->stores[i].loaded)
			empty_magazine(depot, depot->stores[i].loaded);
		if (depot->stores[i].previous)
			empty_magazine(depot, depot->stores[i].previous);
		depot->stores[i].loaded = depot->stores[i].previous = NULL;
	}

	while (depot->full)
		empty_magazine(depot, _pop(depot->full));

	while (depot->empty)
		empty_magazine(depot, _pop(depot->empty));
}

