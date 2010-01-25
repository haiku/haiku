/*
 * Copyright 2008, Axel Dörfler. All Rights Reserved.
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 *
 * Distributed under the terms of the MIT License.
 */


#include "ObjectCache.h"

#include <string.h>

#include <util/AutoLock.h>
#include <vm/vm.h>
#include <vm/VMAddressSpace.h>

#include "slab_private.h"


static const size_t kCacheColorPeriod = 8;


static void
object_cache_return_object_wrapper(object_depot* depot, void* cookie,
	void* object, uint32 flags)
{
	ObjectCache* cache = (ObjectCache*)cookie;

	MutexLocker _(cache->lock);
	cache->ReturnObjectToSlab(cache->ObjectSlab(object), object, flags);
}


// #pragma mark -


ObjectCache::~ObjectCache()
{
}


status_t
ObjectCache::Init(const char* name, size_t objectSize,
	size_t alignment, size_t maximum, uint32 flags, void* cookie,
	object_cache_constructor constructor, object_cache_destructor destructor,
	object_cache_reclaimer reclaimer)
{
	strlcpy(this->name, name, sizeof(this->name));

	mutex_init(&lock, this->name);

	if (objectSize < sizeof(object_link))
		objectSize = sizeof(object_link);

	if (alignment > 0 && (objectSize & (alignment - 1)))
		object_size = objectSize + alignment - (objectSize & (alignment - 1));
	else
		object_size = objectSize;

	TRACE_CACHE(this, "init %lu, %lu -> %lu", objectSize, alignment,
		object_size);

	cache_color_cycle = 0;
	total_objects = 0;
	used_count = 0;
	empty_count = 0;
	pressure = 0;
	min_object_reserve = 0;

	maintenance_pending = false;
	maintenance_in_progress = false;
	maintenance_resize = false;
	maintenance_delete = false;

	usage = 0;
	this->maximum = maximum;

	this->flags = flags;

	resize_request = NULL;
	resize_entry_can_wait = NULL;
	resize_entry_dont_wait = NULL;

	// no gain in using the depot in single cpu setups
	if (smp_get_num_cpus() == 1)
		this->flags |= CACHE_NO_DEPOT;

	if (!(this->flags & CACHE_NO_DEPOT)) {
		status_t status = object_depot_init(&depot, flags, this,
			object_cache_return_object_wrapper);
		if (status < B_OK) {
			mutex_destroy(&lock);
			return status;
		}
	}

	this->cookie = cookie;
	this->constructor = constructor;
	this->destructor = destructor;
	this->reclaimer = reclaimer;

	return B_OK;
}


slab*
ObjectCache::InitSlab(slab* slab, void* pages, size_t byteCount, uint32 flags)
{
	TRACE_CACHE(this, "construct (%p, %p .. %p, %lu)", slab, pages,
		((uint8*)pages) + byteCount, byteCount);

	slab->pages = pages;
	slab->count = slab->size = byteCount / object_size;
	slab->free = NULL;

	size_t spareBytes = byteCount - (slab->size * object_size);

	if ((this->flags & CACHE_ALIGN_ON_SIZE) != 0) {
		slab->offset = cache_color_cycle;

		if (slab->offset > spareBytes)
			cache_color_cycle = slab->offset = 0;
		else
			cache_color_cycle += kCacheColorPeriod;
	} else
		slab->offset = 0;

	TRACE_CACHE(this, "  %lu objects, %lu spare bytes, offset %lu",
		slab->size, spareBytes, slab->offset);

	uint8* data = ((uint8*)pages) + slab->offset;

	CREATE_PARANOIA_CHECK_SET(slab, "slab");

	for (size_t i = 0; i < slab->size; i++) {
		bool failedOnFirst = false;

		status_t status = PrepareObject(slab, data, flags);
		if (status < B_OK)
			failedOnFirst = true;
		else if (constructor)
			status = constructor(cookie, data);

		if (status < B_OK) {
			if (!failedOnFirst)
				UnprepareObject(slab, data, flags);

			data = ((uint8*)pages) + slab->offset;
			for (size_t j = 0; j < i; j++) {
				if (destructor)
					destructor(cookie, data);
				UnprepareObject(slab, data, flags);
				data += object_size;
			}

			DELETE_PARANOIA_CHECK_SET(slab);

			return NULL;
		}

		_push(slab->free, object_to_link(data, object_size));

		ADD_PARANOIA_CHECK(PARANOIA_SUSPICIOUS, slab,
			&object_to_link(data, object_size)->next, sizeof(void*));

		data += object_size;
	}

	usage += slab_size;
	total_objects += slab->size;

	return slab;
}


void
ObjectCache::UninitSlab(slab* slab)
{
	TRACE_CACHE(this, "destruct %p", slab);

	if (slab->count != slab->size)
		panic("cache: destroying a slab which isn't empty.");

	usage -= slab_size;
	total_objects -= slab->size;

	DELETE_PARANOIA_CHECK_SET(slab);

	uint8* data = ((uint8*)slab->pages) + slab->offset;

	for (size_t i = 0; i < slab->size; i++) {
		if (destructor)
			destructor(cookie, data);
		UnprepareObject(slab, data, flags);
		data += object_size;
	}
}


status_t
ObjectCache::PrepareObject(slab* source, void* object, uint32 flags)
{
	return B_OK;
}


void
ObjectCache::UnprepareObject(slab* source, void* object, uint32 flags)
{
}


void
ObjectCache::ReturnObjectToSlab(slab* source, void* object, uint32 flags)
{
	if (source == NULL) {
		panic("object_cache: free'd object has no slab");
		return;
	}

	ParanoiaChecker _(source);

	object_link* link = object_to_link(object, object_size);

	TRACE_CACHE(this, "returning %p (%p) to %p, %lu used (%lu empty slabs).",
		object, link, source, source->size - source->count,
		empty_count);

	_push(source->free, link);
	source->count++;
	used_count--;

	ADD_PARANOIA_CHECK(PARANOIA_SUSPICIOUS, source, &link->next, sizeof(void*));

	if (source->count == source->size) {
		partial.Remove(source);

		if (empty_count < pressure
			&& total_objects - used_count - source->size
				>= min_object_reserve) {
			empty_count++;
			empty.Add(source);
		} else {
			ReturnSlab(source, flags);
		}
	} else if (source->count == 1) {
		full.Remove(source);
		partial.Add(source);
	}
}
