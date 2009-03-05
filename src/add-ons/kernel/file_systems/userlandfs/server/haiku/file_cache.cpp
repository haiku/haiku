/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <fs_cache.h>

#include <new>

#include "AutoDeleter.h"

#include "../kernel_emu.h"


struct FileCache {
	dev_t	mountID;
	ino_t	vnodeID;
	bool	enabled;

	FileCache(dev_t mountID, ino_t vnodeID)
		:
		mountID(mountID),
		vnodeID(vnodeID),
		enabled(true)
	{
	}
};


void*
file_cache_create(dev_t mountID, ino_t vnodeID, off_t size)
{
	// create the client-side object
	FileCache* fileCache = new(std::nothrow) FileCache(mountID, vnodeID);
	if (fileCache == NULL)
		return NULL;
	ObjectDeleter<FileCache> cacheDeleter(fileCache);


	// create the kernel-size file cache
	status_t error = UserlandFS::KernelEmu::file_cache_create(mountID, vnodeID,
		size);
	if (error != B_OK)
		return NULL;

	cacheDeleter.Detach();
	return fileCache;
}


void
file_cache_delete(void *cacheRef)
{
	FileCache* fileCache = (FileCache*)cacheRef;

	UserlandFS::KernelEmu::file_cache_delete(fileCache->mountID,
		fileCache->vnodeID);

	delete fileCache;
}


void
file_cache_enable(void *cacheRef)
{
	FileCache* fileCache = (FileCache*)cacheRef;

	if (fileCache->enabled)
		return;

	if (UserlandFS::KernelEmu::file_cache_set_enabled(fileCache->mountID,
			fileCache->vnodeID, true) == B_OK) {
		fileCache->enabled = true;
	}
}


bool
file_cache_is_enabled(void *cacheRef)
{
	FileCache* fileCache = (FileCache*)cacheRef;

	return fileCache->enabled;
}


status_t
file_cache_disable(void *cacheRef)
{
	FileCache* fileCache = (FileCache*)cacheRef;

	if (!fileCache->enabled)
		return B_OK;

	status_t error = UserlandFS::KernelEmu::file_cache_set_enabled(
		fileCache->mountID, fileCache->vnodeID, false);
	if (error == B_OK)
		fileCache->enabled = false;

	return error;
}


status_t
file_cache_set_size(void *cacheRef, off_t size)
{
	FileCache* fileCache = (FileCache*)cacheRef;

	return UserlandFS::KernelEmu::file_cache_set_size(fileCache->mountID,
		fileCache->vnodeID, size);
}


status_t
file_cache_sync(void *cacheRef)
{
	FileCache* fileCache = (FileCache*)cacheRef;

	return UserlandFS::KernelEmu::file_cache_sync(fileCache->mountID,
		fileCache->vnodeID);
}


status_t
file_cache_read(void *cacheRef, void *cookie, off_t offset, void *bufferBase,
	size_t *_size)
{
	FileCache* fileCache = (FileCache*)cacheRef;

	return UserlandFS::KernelEmu::file_cache_read(fileCache->mountID,
		fileCache->vnodeID, cookie, offset, bufferBase, _size);
}


status_t
file_cache_write(void *cacheRef, void *cookie, off_t offset, const void *buffer,
	size_t *_size)
{
	FileCache* fileCache = (FileCache*)cacheRef;

	return UserlandFS::KernelEmu::file_cache_write(fileCache->mountID,
		fileCache->vnodeID, cookie, offset, buffer, _size);
}
