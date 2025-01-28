/*
 * Copyright 2008-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "VMNullCache.h"

#include <slab/Slab.h>


status_t
VMNullCache::Init(uint32 allocationFlags)
{
	return VMCache::Init("VMNullCache", CACHE_TYPE_NULL, allocationFlags);
}


void
VMNullCache::DeleteObject()
{
	object_cache_delete(gNullCacheObjectCache, this);
}

