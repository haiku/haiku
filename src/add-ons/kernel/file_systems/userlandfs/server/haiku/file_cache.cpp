/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <fs_cache.h>


void*
file_cache_create(dev_t mountID, ino_t vnodeID, off_t size)
{
	// TODO: Implement!
	return NULL;
}


void
file_cache_delete(void *cacheRef)
{
	// TODO: Implement!
}


void
file_cache_enable(void *cacheRef)
{
	// TODO: Implement!
}


bool
file_cache_is_enabled(void *cacheRef)
{
	// TODO: Implement!
	return false;
}


status_t
file_cache_disable(void *cacheRef)
{
	// TODO: Implement!
	return B_NOT_SUPPORTED;
}


status_t
file_cache_set_size(void *cacheRef, off_t size)
{
	// TODO: Implement!
	return B_NOT_SUPPORTED;
}


status_t
file_cache_sync(void *cache)
{
	// TODO: Implement!
	return B_NOT_SUPPORTED;
}


status_t
file_cache_read(void *cacheRef, void *cookie, off_t offset, void *bufferBase,
	size_t *_size)
{
	// TODO: Implement!
	return B_NOT_SUPPORTED;
}


status_t
file_cache_write(void *cacheRef, void *cookie, off_t offset, const void *buffer,
	size_t *_size)
{
	// TODO: Implement!
	return B_NOT_SUPPORTED;
}
