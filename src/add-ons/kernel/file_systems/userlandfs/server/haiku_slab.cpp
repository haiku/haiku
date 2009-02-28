/*
 * Copyright 2001-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "haiku_slab.h"

#include <stdlib.h>

#include <new>


namespace UserlandFS {
namespace HaikuKernelEmu {


struct object_cache {
	object_cache(const char *name, size_t objectSize,
		size_t alignment, size_t maxByteUsage, uint32 flags, void *cookie,
		object_cache_constructor constructor,
		object_cache_destructor destructor, object_cache_reclaimer reclaimer)
		:
		objectSize(objectSize),
		objectConstructor(constructor),
		objectDestructor(destructor)
	{
	}

	size_t						objectSize;
	object_cache_constructor	objectConstructor;
	object_cache_destructor		objectDestructor;
};


object_cache *
create_object_cache(const char *name, size_t objectSize,
	size_t alignment, void *cookie, object_cache_constructor constructor,
	object_cache_destructor destructor)
{
	return new(std::nothrow) object_cache(name, objectSize, alignment,
		0, 0, cookie, constructor, destructor, NULL);
}


object_cache *
create_object_cache_etc(const char *name, size_t objectSize,
	size_t alignment, size_t maxByteUsage, uint32 flags, void *cookie,
	object_cache_constructor constructor, object_cache_destructor destructor,
	object_cache_reclaimer reclaimer)
{
	return new(std::nothrow) object_cache(name, objectSize, alignment,
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
	return cache != NULL ? malloc(cache->objectSize) : NULL;
}


void
object_cache_free(object_cache *cache, void *object)
{
	free(object);
}


status_t
object_cache_reserve(object_cache *cache, size_t object_count, uint32 flags)
{
	return B_OK;
}


void object_cache_get_usage(object_cache *cache, size_t *_allocatedMemory)
{
	*_allocatedMemory = 0;
}

}	// namespace HaikuKernelEmu
}	// namespace UserlandFS
