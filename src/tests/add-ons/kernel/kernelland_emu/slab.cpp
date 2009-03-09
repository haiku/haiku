/*
 * Copyright 2002-2008, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Ingo Weinhold, bonefish@cs.tu-berlin.de.
 *		Axel Dörfler, axeld@pinc-software.de.
 */

#include <slab/Slab.h>

#include <stdlib.h>


extern "C" void *
object_cache_alloc(object_cache *cache, uint32 flags)
{
	return malloc((size_t)cache);
}


extern "C" void
object_cache_free(object_cache *cache, void *object)
{
	free(object);
}


extern "C" object_cache *
create_object_cache_etc(const char *name, size_t objectSize,
	size_t alignment, size_t maxByteUsage, uint32 flags, void *cookie,
	object_cache_constructor constructor, object_cache_destructor destructor,
	object_cache_reclaimer reclaimer)
{
	return (object_cache*)objectSize;
}


extern "C" object_cache *
create_object_cache(const char *name, size_t objectSize,
	size_t alignment, void *cookie, object_cache_constructor constructor,
	object_cache_destructor)
{
	return (object_cache*)objectSize;
}


extern "C" void
delete_object_cache(object_cache *cache)
{
}


extern "C" void
object_cache_get_usage(object_cache *cache, size_t *_allocatedMemory)
{
	*_allocatedMemory = 100;
}
