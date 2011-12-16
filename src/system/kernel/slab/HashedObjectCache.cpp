/*
 * Copyright 2008, Axel Dörfler. All Rights Reserved.
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 *
 * Distributed under the terms of the MIT License.
 */


#include "HashedObjectCache.h"

#include "MemoryManager.h"
#include "slab_private.h"


RANGE_MARKER_FUNCTION_BEGIN(SlabHashedObjectCache)


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


static HashedSlab*
allocate_slab(uint32 flags)
{
	return (HashedSlab*)slab_internal_alloc(sizeof(HashedSlab), flags);
}


static void
free_slab(HashedSlab* slab, uint32 flags)
{
	slab_internal_free(slab, flags);
}


// #pragma mark -


HashedObjectCache::HashedObjectCache()
	:
	hash_table(this)
{
}


/*static*/ HashedObjectCache*
HashedObjectCache::Create(const char* name, size_t object_size,
	size_t alignment, size_t maximum, size_t magazineCapacity,
	size_t maxMagazineCount, uint32 flags, void* cookie,
	object_cache_constructor constructor, object_cache_destructor destructor,
	object_cache_reclaimer reclaimer)
{
	void* buffer = slab_internal_alloc(sizeof(HashedObjectCache), flags);
	if (buffer == NULL)
		return NULL;

	HashedObjectCache* cache = new(buffer) HashedObjectCache();

	// init the hash table
	size_t hashSize = cache->hash_table.ResizeNeeded();
	buffer = slab_internal_alloc(hashSize, flags);
	if (buffer == NULL) {
		cache->Delete();
		return NULL;
	}

	cache->hash_table.Resize(buffer, hashSize, true);

	if (cache->Init(name, object_size, alignment, maximum, magazineCapacity,
			maxMagazineCount, flags, cookie, constructor, destructor,
			reclaimer) != B_OK) {
		cache->Delete();
		return NULL;
	}

	if ((flags & CACHE_LARGE_SLAB) != 0)
		cache->slab_size = 128 * object_size;
	else
		cache->slab_size = 8 * object_size;

	cache->slab_size = MemoryManager::AcceptableChunkSize(cache->slab_size);
	cache->lower_boundary = __fls0(cache->slab_size);

	return cache;
}


void
HashedObjectCache::Delete()
{
	this->~HashedObjectCache();
	slab_internal_free(this, 0);
}


slab*
HashedObjectCache::CreateSlab(uint32 flags)
{
	if (!check_cache_quota(this))
		return NULL;

	Unlock();

	HashedSlab* slab = allocate_slab(flags);
	if (slab != NULL) {
		void* pages = NULL;
		if (MemoryManager::Allocate(this, flags, pages) == B_OK
			&& AllocateTrackingInfos(slab, slab_size, flags) == B_OK) {
			Lock();
			if (InitSlab(slab, pages, slab_size, flags)) {
				hash_table.InsertUnchecked(slab);
				_ResizeHashTableIfNeeded(flags);
				return slab;
			}
			Unlock();
			FreeTrackingInfos(slab, flags);
		}

		if (pages != NULL)
			MemoryManager::Free(pages, flags);

		free_slab(slab, flags);
	}

	Lock();
	return NULL;
}


void
HashedObjectCache::ReturnSlab(slab* _slab, uint32 flags)
{
	HashedSlab* slab = static_cast<HashedSlab*>(_slab);

	hash_table.RemoveUnchecked(slab);
	_ResizeHashTableIfNeeded(flags);

	UninitSlab(slab);

	Unlock();
	FreeTrackingInfos(slab, flags);
	MemoryManager::Free(slab->pages, flags);
	free_slab(slab, flags);
	Lock();
}


slab*
HashedObjectCache::ObjectSlab(void* object) const
{
	ASSERT_LOCKED_MUTEX(&lock);

	HashedSlab* slab = hash_table.Lookup(::lower_boundary(object, slab_size));
	if (slab == NULL) {
		panic("hash object cache %p: unknown object %p", this, object);
		return NULL;
	}

	return slab;
}


void
HashedObjectCache::_ResizeHashTableIfNeeded(uint32 flags)
{
	size_t hashSize = hash_table.ResizeNeeded();
	if (hashSize != 0) {
		Unlock();
		void* buffer = slab_internal_alloc(hashSize, flags);
		Lock();

		if (buffer != NULL) {
			if (hash_table.ResizeNeeded() == hashSize) {
				void* oldHash;
				hash_table.Resize(buffer, hashSize, true, &oldHash);
				if (oldHash != NULL) {
					Unlock();
					slab_internal_free(oldHash, flags);
					Lock();
				}
			}
		}
	}
}


RANGE_MARKER_FUNCTION_END(SlabHashedObjectCache)
