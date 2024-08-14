/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2019-2024, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 */
#include "DataContainer.h"

#include <StackOrHeapArray.h>
#include <util/AutoLock.h>
#include <util/BitUtils.h>
#include <slab/Slab.h>

#include <vfs.h>
#include <vm/VMCache.h>
#include <vm/vm_page.h>
#include "VMAnonymousNoSwapCache.h"
#include "vnode_store.h"

#include "AllocationInfo.h"
#include "DebugSupport.h"
#include "Misc.h"
#include "Volume.h"
#include "cache_support.h"


// Initial size of the DataContainer's small buffer. If it contains data up to
// this size, nothing is allocated, but the small buffer is used instead.
// 16 bytes are for free, since they are shared with the block list.
// (actually even more, since the list has an initial size).
// I ran a test analyzing what sizes the attributes in my system have:
//     size   percentage   bytes used in average
//   <=   0         0.00                   93.45
//   <=   4        25.46                   75.48
//   <=   8        30.54                   73.02
//   <=  16        52.98                   60.37
//   <=  32        80.19                   51.74
//   <=  64        94.38                   70.54
//   <= 126        96.90                  128.23
//
// For average memory usage it is assumed, that attributes larger than 126
// bytes have size 127, that the list has an initial capacity of 10 entries
// (40 bytes), that the block reference consumes 4 bytes and the block header
// 12 bytes. The optimal length is actually 35, with 51.05 bytes per
// attribute, but I conservatively rounded to 32.
static const off_t kMinimumSmallBufferSize = 32;
static const off_t kMaximumSmallBufferSize = (B_PAGE_SIZE / 4);


// We don't use VMVnodeCache because it's for caching pages that exist on disk.
// All we need is an AnonymousCache that tracks when the vnode is referenced.
class VMForVnodeCache final : public VMAnonymousNoSwapCache {
public:
	status_t Init()
	{
		fVnode = NULL;
		return VMAnonymousNoSwapCache::Init(false, 0, 0, 0);
	}

	status_t AcquireUnreferencedStoreRef() override
	{
		return B_NOT_SUPPORTED;
	}

	void AcquireStoreRef() override
	{
		vfs_acquire_vnode(fVnode);
	}

	void ReleaseStoreRef() override
	{
		vfs_put_vnode(fVnode);
	}

protected:
	virtual	void DeleteObject()
	{
		static_assert(sizeof(VMForVnodeCache) <= sizeof(VMVnodeCache), "cache too large");
		object_cache_delete(gVnodeCacheObjectCache, this);
	}

private:
	friend class DataContainer;
	struct vnode* fVnode;
};


DataContainer::DataContainer(Volume *volume)
	: fVolume(volume),
	  fSize(0),
	  fCache(NULL),
	  fSmallBuffer(NULL),
	  fSmallBufferSize(0)
{
}


DataContainer::~DataContainer()
{
	if (fCache != NULL) {
		fCache->Lock();
		fCache->ReleaseRefAndUnlock();
		fCache = NULL;
	}
	if (fSmallBuffer != NULL) {
		free(fSmallBuffer);
		fSmallBuffer = NULL;
	}
}


status_t
DataContainer::InitCheck() const
{
	return (fVolume != NULL ? B_OK : B_ERROR);
}


VMCache*
DataContainer::GetCache(struct vnode* vnode)
{
	// TODO: Because we always get the cache for files on creation vs. on demand,
	// this means files (no matter how small) always use cache mode at present.
	if (!_IsCacheMode())
		_SwitchToCacheMode();
	((VMForVnodeCache*)fCache)->fVnode = vnode;
	return fCache;
}


status_t
DataContainer::Resize(off_t newSize)
{
//	PRINT("DataContainer::Resize(%lld), fSize: %lld\n", newSize, fSize);

	status_t error = B_OK;
	if (_RequiresCacheMode(newSize)) {
		if (newSize < fSize) {
			// shrink
			// resize the VMCache, which will automatically free pages
			AutoLocker<VMCache> _(fCache);
			error = fCache->Resize(newSize, VM_PRIORITY_USER);
			if (error != B_OK)
				return error;
		} else {
			// grow
			if (!_IsCacheMode())
				error = _SwitchToCacheMode();
			if (error != B_OK)
				return error;

			AutoLocker<VMCache> _(fCache);
			fCache->Resize(newSize, VM_PRIORITY_USER);

			// pages will be added as they are written to; so nothing else
			// needs to be done here.
		}
	} else if (fSmallBufferSize < newSize
			|| (fSmallBufferSize - newSize) > (kMaximumSmallBufferSize / 2)) {
		const size_t newBufferSize = max_c(next_power_of_2(newSize),
			kMinimumSmallBufferSize);
		void* newBuffer = realloc(fSmallBuffer, newBufferSize);
		if (newBuffer == NULL)
			return B_NO_MEMORY;

		fSmallBufferSize = newBufferSize;
		fSmallBuffer = (uint8*)newBuffer;
	}

	fSize = newSize;

//	PRINT("DataContainer::Resize() done: %lx, fSize: %lld\n", error, fSize);
	return error;
}


status_t
DataContainer::ReadAt(off_t offset, void *_buffer, size_t size,
	size_t *bytesRead)
{
	uint8 *buffer = (uint8*)_buffer;
	status_t error = (buffer && offset >= 0 &&
		bytesRead ? B_OK : B_BAD_VALUE);
	if (error != B_OK)
		return error;

	// read not more than we have to offer
	offset = min(offset, fSize);
	size = min(size, size_t(fSize - offset));

	if (!_IsCacheMode()) {
		// in non-cache mode, use the "small buffer"
		if (IS_USER_ADDRESS(buffer)) {
			error = user_memcpy(buffer, fSmallBuffer + offset, size);
			if (error != B_OK)
				size = 0;
		} else {
			memcpy(buffer, fSmallBuffer + offset, size);
		}

		if (bytesRead != NULL)
			*bytesRead = size;
		return error;
	}

	// cache mode
	error = _DoCacheIO(offset, buffer, size, bytesRead, false);

	return error;
}


status_t
DataContainer::WriteAt(off_t offset, const void *_buffer, size_t size,
	size_t *bytesWritten)
{
	PRINT("DataContainer::WriteAt(%lld, %p, %lu, %p), fSize: %lld\n", offset, _buffer, size, bytesWritten, fSize);

	const uint8 *buffer = (const uint8*)_buffer;
	status_t error = (buffer && offset >= 0 && bytesWritten
		? B_OK : B_BAD_VALUE);
	if (error != B_OK)
		return error;

	// resize the container, if necessary
	if ((offset + (off_t)size) > fSize)
		error = Resize(offset + size);
	if (error != B_OK)
		return error;

	if (!_IsCacheMode()) {
		// in non-cache mode, use the "small buffer"
		if (IS_USER_ADDRESS(buffer)) {
			error = user_memcpy(fSmallBuffer + offset, buffer, size);
			if (error != B_OK)
				size = 0;
		} else {
			memcpy(fSmallBuffer + offset, buffer, size);
		}

		if (bytesWritten != NULL)
			*bytesWritten = size;
		return error;
	}

	// cache mode
	error = _DoCacheIO(offset, (uint8*)buffer, size, bytesWritten, true);

	PRINT("DataContainer::WriteAt() done: %lx, fSize: %lld\n", error, fSize);
	return error;
}


void
DataContainer::GetAllocationInfo(AllocationInfo &info)
{
	if (_IsCacheMode()) {
		info.AddAreaAllocation(fCache->committed_size);
	} else {
		// ...
	}
}


inline bool
DataContainer::_RequiresCacheMode(size_t size)
{
	// we cannot back out of cache mode after entering it,
	// as there may be other consumers of our VMCache
	return _IsCacheMode() || (size > kMaximumSmallBufferSize);
}


inline bool
DataContainer::_IsCacheMode() const
{
	return fCache != NULL;
}


inline int32
DataContainer::_CountBlocks() const
{
	if (_IsCacheMode())
		return fCache->page_count;
	else if (fSize == 0)	// small buffer mode, empty buffer
		return 0;
	return 1;	// small buffer mode, non-empty buffer
}


status_t
DataContainer::_SwitchToCacheMode()
{
	VMForVnodeCache* cache = new(gVnodeCacheObjectCache, 0) VMForVnodeCache;
	if (cache == NULL)
		return B_NO_MEMORY;

	status_t error = cache->Init();
	if (error != B_OK)
		return error;

	fCache = cache;
	fCache->temporary = 1;
	fCache->unmergeable = 1;
	fCache->virtual_end = fSize;

	error = fCache->Commit(fSize, VM_PRIORITY_USER);
	if (error != B_OK)
		return error;

	if (fSize != 0)
		error = _DoCacheIO(0, fSmallBuffer, fSize, NULL, true);

	free(fSmallBuffer);
	fSmallBuffer = NULL;
	fSmallBufferSize = 0;

	return error;
}


status_t
DataContainer::_DoCacheIO(const off_t offset, uint8* buffer, ssize_t length,
	size_t* bytesProcessed, bool isWrite)
{
	const size_t originalLength = length;
	const bool user = IS_USER_ADDRESS(buffer);

	const off_t rounded_offset = ROUNDDOWN(offset, B_PAGE_SIZE);
	const size_t rounded_len = ROUNDUP((length) + (offset - rounded_offset),
		B_PAGE_SIZE);
	BStackOrHeapArray<vm_page*, 16> pages(rounded_len / B_PAGE_SIZE);
	if (!pages.IsValid())
		return B_NO_MEMORY;

	cache_get_pages(fCache, rounded_offset, rounded_len, isWrite, pages);

	status_t error = B_OK;
	size_t index = 0;

	while (length > 0) {
		vm_page* page = pages[index];
		phys_addr_t at = (page != NULL)
			? (page->physical_page_number * B_PAGE_SIZE) : 0;
		ssize_t bytes = B_PAGE_SIZE;
		if (index == 0) {
			const uint32 pageoffset = (offset % B_PAGE_SIZE);
			at += pageoffset;
			bytes -= pageoffset;
		}
		bytes = min(length, bytes);

		if (isWrite) {
			page->modified = true;
			error = vm_memcpy_to_physical(at, buffer, bytes, user);
		} else {
			if (page != NULL) {
				error = vm_memcpy_from_physical(buffer, at, bytes, user);
			} else {
				if (user) {
					error = user_memset(buffer, 0, bytes);
				} else {
					memset(buffer, 0, bytes);
				}
			}
		}
		if (error != B_OK)
			break;

		buffer += bytes;
		length -= bytes;
		index++;
	}

	cache_put_pages(fCache, rounded_offset, rounded_len, pages, error == B_OK);

	if (bytesProcessed != NULL)
		*bytesProcessed = length > 0 ? originalLength - length : originalLength;

	return error;
}
