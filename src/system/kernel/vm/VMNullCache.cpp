/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "VMNullCache.h"


status_t
VMNullCache::Init()
{
	return VMCache::Init(CACHE_TYPE_NULL);
}
