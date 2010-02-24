/*
 * Copyright 2009-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <slab/Slab.h>

#include <stdlib.h>

#include <new>


struct ObjectCache {
	ObjectCache(const char *name, size_t objectSize,
		size_t alignment, size_t maxByteUsage, uint32 flags, void *cookie,
		object_cache_constructor constructor,
		object_cache_destructor destructor, object_cache_reclaimer reclaimer)
		:
		objectSize(objectSize),
		cookie(cookie),
		objectConstructor(constructor),
		objectDestructor(destructor)
	{
	}

	size_t						objectSize;
	void*						cookie;
	object_cache_constructor	objectConstructor;
	object_cache_destructor		objectDestructor;
};


object_cache *
create_object_cache(const char *name, size_t objectSize,
	size_t alignment, void *cookie, object_cache_constructor constructor,
	object_cache_destructor destructor)
{
	return new(std::nothrow) ObjectCache(name, objectSize, alignment,
		0, 0, cookie, constructor, destructor, NULL);
}


object_cache *
create_object_cache_etc(const char *name, size_t objectSize,
	size_t alignment, size_t maxByteUsage, size_t magazineCapacity,
	size_t maxMagazineCount, uint32 flags, void *cookie,
	object_cache_constructor constructor, object_cache_destructor destructor,
	object_cache_reclaimer reclaimer)
{
	return new(std::nothrow) ObjectCache(name, objectSize, alignment,
		maxByteUsage, flags, cookie, constructor, destructor, reclaimer);
}


void
delete_object_cache(object_cache *cache)
{
	delete cache;
}


status_t
object_cache_set_minimum_reserve(object_cache *cache, size_t objectCount)
{
	return B_OK;
}


void *
object_cache_alloc(object_cache *cache, uint32 flags)
{
	void* object = cache != NULL ? malloc(cache->objectSize) : NULL;
	if (object == NULL)
		return NULL;

	if (cache->objectConstructor != NULL)
		cache->objectConstructor(cache->cookie, object);

	return object;
}


void
object_cache_free(object_cache *cache, void *object, uint32 flags)
{
	if (object != NULL) {
		if (cache != NULL && cache->objectDestructor != NULL)
			cache->objectDestructor(cache->cookie, object);

		free(object);
	}
}


status_t
object_cache_reserve(object_cache *cache, size_t object_count, uint32 flags)
{
	return B_OK;
}


void
object_cache_get_usage(object_cache *cache, size_t *_allocatedMemory)
{
	*_allocatedMemory = 0;
}
