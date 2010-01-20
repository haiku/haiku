/*
 * Copyright 2008, Axel Dörfler. All Rights Reserved.
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 *
 * Distributed under the terms of the MIT License.
 */


#include "SmallObjectCache.h"

#include "slab_private.h"


/*static*/ SmallObjectCache*
SmallObjectCache::Create(const char* name, size_t object_size,
	size_t alignment, size_t maximum, uint32 flags, void* cookie,
	object_cache_constructor constructor, object_cache_destructor destructor,
	object_cache_reclaimer reclaimer)
{
	void* buffer = slab_internal_alloc(sizeof(SmallObjectCache), flags);
	if (buffer == NULL)
		return NULL;

	SmallObjectCache* cache = new(buffer) SmallObjectCache();

	if (cache->Init(name, object_size, alignment, maximum,
			flags | CACHE_ALIGN_ON_SIZE, cookie, constructor, destructor,
			reclaimer) != B_OK) {
		cache->Delete();
		return NULL;
	}

	if ((flags & CACHE_LARGE_SLAB) != 0)
		cache->slab_size = max_c(16 * B_PAGE_SIZE, 1024 * object_size);
	else
		cache->slab_size = B_PAGE_SIZE;

	cache->objects_per_slab = (cache->slab_size - sizeof(slab))
		/ cache->object_size;

	return cache;
}


slab*
SmallObjectCache::CreateSlab(uint32 flags, bool unlockWhileAllocating)
{
	if (!check_cache_quota(this))
		return NULL;

	void* pages;
	if ((this->*allocate_pages)(&pages, flags, unlockWhileAllocating) != B_OK)
		return NULL;

	return InitSlab(slab_in_pages(pages, slab_size), pages,
		slab_size - sizeof(slab), flags);
}


void
SmallObjectCache::ReturnSlab(slab* slab)
{
	UninitSlab(slab);
	(this->*free_pages)(slab->pages);
}


slab*
SmallObjectCache::ObjectSlab(void* object) const
{
	return slab_in_pages(lower_boundary(object, slab_size), slab_size);
}
