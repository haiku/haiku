/*
 * Copyright 2008-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "VMNullCache.h"


status_t
VMNullCache::Init(uint32 allocationFlags)
{
	return VMCache::Init(CACHE_TYPE_NULL, allocationFlags);
}
