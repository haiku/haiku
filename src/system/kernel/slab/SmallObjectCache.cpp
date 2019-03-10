/*
 * Copyright 2008, Axel Dörfler. All Rights Reserved.
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 *
 * Distributed under the terms of the MIT License.
 */


#include "SmallObjectCache.h"

#include <BytePointer.h>
#include "MemoryManager.h"
#include "slab_private.h"


RANGE_MARKER_FUNCTION_BEGIN(SlabSmallObjectCache)


static inline slab *
slab_in_pages(void *pages, size_t slab_size)
{
	BytePointer<slab> pointer(pages);
	pointer += slab_size - sizeof(slab);
	return &pointer;
}


/*static*/ SmallObjectCache*
SmallObjectCache::Create(const char* name, size_t object_size,
	size_t alignment, size_t maximum, size_t magazineCapacity,
	size_t maxMagazineCount, uint32 flags, void* cookie,
	object_cache_constructor constructor, object_cache_destructor destructor,
	object_cache_reclaimer reclaimer)
{
	void* buffer = slab_internal_alloc(sizeof(SmallObjectCache), flags);
	if (buffer == NULL)
		return NULL;

	SmallObjectCache* cache = new(buffer) SmallObjectCache();

	if (cache->Init(name, object_size, alignment, maximum, magazineCapacity,
			maxMagazineCount, flags, cookie, constructor, destructor,
			reclaimer) != B_OK) {
		cache->Delete();
		return NULL;
	}

	if ((flags & CACHE_LARGE_SLAB) != 0)
		cache->slab_size = 1024 * object_size;
	else
		cache->slab_size = SLAB_CHUNK_SIZE_SMALL;

	cache->slab_size = MemoryManager::AcceptableChunkSize(cache->slab_size);

	return cache;
}


void
SmallObjectCache::Delete()
{
	this->~SmallObjectCache();
	slab_internal_free(this, 0);
}


slab*
SmallObjectCache::CreateSlab(uint32 flags)
{
	if (!check_cache_quota(this))
		return NULL;

	void* pages;

	Unlock();
	status_t error = MemoryManager::Allocate(this, flags, pages);
	Lock();

	if (error != B_OK)
		return NULL;

	slab* newSlab = slab_in_pages(pages, slab_size);
	size_t byteCount = slab_size - sizeof(slab);
	if (AllocateTrackingInfos(newSlab, byteCount, flags) != B_OK) {
		MemoryManager::Free(pages, flags);
		return NULL;
	}
		
	return InitSlab(newSlab, pages, byteCount, flags);
}


void
SmallObjectCache::ReturnSlab(slab* slab, uint32 flags)
{
	UninitSlab(slab);

	Unlock();
	FreeTrackingInfos(slab, flags);
	MemoryManager::Free(slab->pages, flags);
	Lock();
}


slab*
SmallObjectCache::ObjectSlab(void* object) const
{
	return slab_in_pages(lower_boundary(object, slab_size), slab_size);
}


RANGE_MARKER_FUNCTION_END(SlabSmallObjectCache)
