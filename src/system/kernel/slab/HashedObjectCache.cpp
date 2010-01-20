/*
 * Copyright 2008, Axel Dörfler. All Rights Reserved.
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 *
 * Distributed under the terms of the MIT License.
 */


#include "HashedObjectCache.h"

#include "slab_private.h"


static inline int
__fls0(size_t value)
{
	if (value == 0)
		return -1;

	int bit;
	for (bit = 0; value != 1; bit++)
		value >>= 1;
	return bit;
}


static slab*
allocate_slab(uint32 flags)
{
	return (slab*)slab_internal_alloc(sizeof(slab), flags);
}


static void
free_slab(slab* slab)
{
	slab_internal_free(slab);
}


// #pragma mark -


HashedObjectCache::HashedObjectCache()
	:
	hash_table(this)
{
}


/*static*/ HashedObjectCache*
HashedObjectCache::Create(const char* name, size_t object_size,
	size_t alignment, size_t maximum, uint32 flags, void* cookie,
	object_cache_constructor constructor, object_cache_destructor destructor,
	object_cache_reclaimer reclaimer)
{
	void* buffer = slab_internal_alloc(sizeof(HashedObjectCache), flags);
	if (buffer == NULL)
		return NULL;

	HashedObjectCache* cache = new(buffer) HashedObjectCache();

	if (cache->Init(name, object_size, alignment, maximum, flags, cookie,
			constructor, destructor, reclaimer) != B_OK) {
		cache->Delete();
		return NULL;
	}

	if ((flags & CACHE_LARGE_SLAB) != 0)
		cache->slab_size = max_c(256 * B_PAGE_SIZE, 128 * object_size);
	else
		cache->slab_size = max_c(16 * B_PAGE_SIZE, 8 * object_size);
	cache->lower_boundary = __fls0(cache->object_size);

	return cache;
}


slab*
HashedObjectCache::CreateSlab(uint32 flags, bool unlockWhileAllocating)
{
	if (!check_cache_quota(this))
		return NULL;

	if (unlockWhileAllocating)
		Unlock();

	slab* slab = allocate_slab(flags);

	if (unlockWhileAllocating)
		Lock();

	if (slab == NULL)
		return NULL;

	void* pages;
	if ((this->*allocate_pages)(&pages, flags, unlockWhileAllocating) == B_OK) {
		if (InitSlab(slab, pages, slab_size))
			return slab;

		(this->*free_pages)(pages);
	}

	free_slab(slab);
	return NULL;
}


void
HashedObjectCache::ReturnSlab(slab* slab)
{
	UninitSlab(slab);
	(this->*free_pages)(slab->pages);
	free_slab(slab);
}


slab*
HashedObjectCache::ObjectSlab(void* object) const
{
	Link* link = hash_table.Lookup(object);
	if (link == NULL) {
		panic("object cache: requested object %p missing from hash table",
			object);
		return NULL;
	}
	return link->parent;
}


status_t
HashedObjectCache::PrepareObject(slab* source, void* object)
{
	Link* link = _AllocateLink(CACHE_DONT_SLEEP);
	if (link == NULL)
		return B_NO_MEMORY;

	link->buffer = object;
	link->parent = source;

	hash_table.Insert(link);
	return B_OK;
}


void
HashedObjectCache::UnprepareObject(slab* source, void* object)
{
	Link* link = hash_table.Lookup(object);
	if (link == NULL) {
		panic("object cache: requested object missing from hash table");
		return;
	}

	if (link->parent != source) {
		panic("object cache: slab mismatch");
		return;
	}

	hash_table.Remove(link);
	_FreeLink(link);
}


/*static*/ inline HashedObjectCache::Link*
HashedObjectCache::_AllocateLink(uint32 flags)
{
	return (HashedObjectCache::Link*)
		slab_internal_alloc(sizeof(HashedObjectCache::Link), flags);
}


/*static*/ inline void
HashedObjectCache::_FreeLink(HashedObjectCache::Link* link)
{
	slab_internal_free(link);
}
