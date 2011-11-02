/*
 * Copyright 2004-2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include "VMDeviceCache.h"

#include <slab/Slab.h>


status_t
VMDeviceCache::Init(addr_t baseAddress, uint32 allocationFlags)
{
	fBaseAddress = baseAddress;
	return VMCache::Init(CACHE_TYPE_DEVICE, allocationFlags);
}


status_t
VMDeviceCache::Read(off_t offset, const iovec* vecs, size_t count,
	uint32 flags, size_t* _numBytes)
{
	panic("device_store: read called. Invalid!\n");
	return B_ERROR;
}


status_t
VMDeviceCache::Write(off_t offset, const iovec* vecs, size_t count,
	uint32 flags, size_t* _numBytes)
{
	// no place to write, this will cause the page daemon to skip this store
	return B_OK;
}


void
VMDeviceCache::DeleteObject()
{
	object_cache_delete(gDeviceCacheObjectCache, this);
}
