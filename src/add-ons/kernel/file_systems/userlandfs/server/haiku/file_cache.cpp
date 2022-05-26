/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <fs_cache.h>

#include <new>

#include "AutoDeleter.h"
#include "Debug.h"

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
	PRINT(("file_cache_create(%" B_PRIdDEV ", %" B_PRIdINO ", %"
		B_PRIdOFF ")\n", mountID, vnodeID, size));

	// create the client-side object
	FileCache* fileCache = new(std::nothrow) FileCache(mountID, vnodeID);
	if (fileCache == NULL)
		return NULL;
	ObjectDeleter<FileCache> cacheDeleter(fileCache);

	// create the kernel-size file cache
	status_t error = UserlandFS::KernelEmu::file_cache_create(mountID, vnodeID,
		size);
	if (error != B_OK) {
		REPORT_ERROR(error);
		return NULL;
	}

	cacheDeleter.Detach();

	PRINT(("file_cache_create() -> %p\n", fileCache));
	return fileCache;
}


void
file_cache_delete(void *cacheRef)
{
	if (cacheRef == NULL)
		return;

	PRINT(("file_cache_delete(%p)\n", cacheRef));

	FileCache* fileCache = (FileCache*)cacheRef;

	UserlandFS::KernelEmu::file_cache_delete(fileCache->mountID,
		fileCache->vnodeID);

	delete fileCache;
}


void
file_cache_enable(void *cacheRef)
{
	PRINT(("file_cache_enable(%p)\n", cacheRef));

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
	PRINT(("file_cache_disable(%p)\n", cacheRef));

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
	if (cacheRef == NULL)
		return B_BAD_VALUE;

	PRINT(("file_cache_set_size(%p)\n", cacheRef));

	FileCache* fileCache = (FileCache*)cacheRef;

	return UserlandFS::KernelEmu::file_cache_set_size(fileCache->mountID,
		fileCache->vnodeID, size);
}


status_t
file_cache_sync(void *cacheRef)
{
	if (cacheRef == NULL)
		return B_BAD_VALUE;

	PRINT(("file_cache_sync(%p)\n", cacheRef));

	FileCache* fileCache = (FileCache*)cacheRef;

	return UserlandFS::KernelEmu::file_cache_sync(fileCache->mountID,
		fileCache->vnodeID);
}


status_t
file_cache_read(void *cacheRef, void *cookie, off_t offset, void *bufferBase,
	size_t *_size)
{
	PRINT(("file_cache_read(%p, %p, %" B_PRIdOFF ", %p, %lu)\n",
		cacheRef, cookie, offset, bufferBase, *_size));

	FileCache* fileCache = (FileCache*)cacheRef;

	return UserlandFS::KernelEmu::file_cache_read(fileCache->mountID,
		fileCache->vnodeID, cookie, offset, bufferBase, _size);
}


status_t
file_cache_write(void *cacheRef, void *cookie, off_t offset, const void *buffer,
	size_t *_size)
{
	PRINT(("file_cache_write(%p, %p, %" B_PRIdOFF ", %p, %lu)\n",
		cacheRef, cookie, offset, buffer, *_size));

	FileCache* fileCache = (FileCache*)cacheRef;

	return UserlandFS::KernelEmu::file_cache_write(fileCache->mountID,
		fileCache->vnodeID, cookie, offset, buffer, _size);
}
