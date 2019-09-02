/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2019, Haiku, Inc.
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "DataContainer.h"

#include <AutoDeleter.h>
#include <util/AutoLock.h>

#include <vm/VMCache.h>
#include <vm/vm_page.h>

#include "AllocationInfo.h"
#include "DebugSupport.h"
#include "Misc.h"
#include "Volume.h"

// constructor
DataContainer::DataContainer(Volume *volume)
	: fVolume(volume),
	  fSize(0),
	  fCache(NULL)
{
}

// destructor
DataContainer::~DataContainer()
{
	if (fCache != NULL) {
		fCache->Lock();
		fCache->ReleaseRefAndUnlock();
		fCache = NULL;
	}
}

// InitCheck
status_t
DataContainer::InitCheck() const
{
	return (fVolume != NULL ? B_OK : B_ERROR);
}

// GetCache
VMCache*
DataContainer::GetCache()
{
	if (!_IsCacheMode())
		_SwitchToCacheMode();
	return fCache;
}

// Resize
status_t
DataContainer::Resize(off_t newSize)
{
//	PRINT("DataContainer::Resize(%Ld), fSize: %Ld\n", newSize, fSize);

	status_t error = B_OK;
	if (newSize < fSize) {
		// shrink
		if (_IsCacheMode()) {
			// resize the VMCache, which will automatically free pages
			AutoLocker<VMCache> _(fCache);
			error = fCache->Resize(newSize, VM_PRIORITY_SYSTEM);
			if (error != B_OK)
				return error;
		} else {
			// small buffer mode: just set the new size (done below)
		}
	} else if (newSize > fSize) {
		// grow
		if (_RequiresCacheMode(newSize)) {
			if (!_IsCacheMode())
				error = _SwitchToCacheMode();
			if (error != B_OK)
				return error;

			AutoLocker<VMCache> _(fCache);
			fCache->Resize(newSize, VM_PRIORITY_SYSTEM);

			// pages will be added as they are written to; so nothing else
			// needs to be done here.
		} else {
			// no need to switch to cache mode: just set the new size
			// (done below)
		}
	}

	fSize = newSize;

//	PRINT("DataContainer::Resize() done: %lx, fSize: %Ld\n", error, fSize);
	return error;
}

// ReadAt
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
		// in non-cache mode, we just copy the data directly
		memcpy(buffer, fSmallBuffer + offset, size);
		if (bytesRead != NULL)
			*bytesRead = size;
		return B_OK;
	}

	// cache mode
	error = _DoCacheIO(offset, buffer, size, bytesRead, false);

	return error;
}

// WriteAt
status_t
DataContainer::WriteAt(off_t offset, const void *_buffer, size_t size,
	size_t *bytesWritten)
{
	PRINT("DataContainer::WriteAt(%Ld, %p, %lu, %p), fSize: %Ld\n", offset, _buffer, size, bytesWritten, fSize);

	const uint8 *buffer = (const uint8*)_buffer;
	status_t error = (buffer && offset >= 0 && bytesWritten
		? B_OK : B_BAD_VALUE);
	if (error != B_OK)
		return error;

	// resize the container, if necessary
	if ((offset + size) > fSize)
		error = Resize(offset + size);
	if (error != B_OK)
		return error;

	if (!_IsCacheMode()) {
		// in non-cache mode, we just copy the data directly
		memcpy(fSmallBuffer + offset, buffer, size);
		if (bytesWritten != NULL)
			*bytesWritten = size;
		return B_OK;
	}

	// cache mode
	error = _DoCacheIO(offset, (uint8*)buffer, size, bytesWritten, true);

	PRINT("DataContainer::WriteAt() done: %lx, fSize: %Ld\n", error, fSize);
	return error;
}

// GetAllocationInfo
void
DataContainer::GetAllocationInfo(AllocationInfo &info)
{
	if (_IsCacheMode()) {
		info.AddAreaAllocation(fCache->committed_size);
	} else {
		// ...
	}
}

// _RequiresCacheMode
inline bool
DataContainer::_RequiresCacheMode(size_t size)
{
	// we cannot back out of cache mode after entering it,
	// as there may be other consumers of our VMCache
	return _IsCacheMode() || (size > kSmallDataContainerSize);
}

// _IsCacheMode
inline bool
DataContainer::_IsCacheMode() const
{
	return fCache != NULL;
}

// _CountBlocks
inline int32
DataContainer::_CountBlocks() const
{
	if (_IsCacheMode())
		return fCache->page_count;
	else if (fSize == 0)	// small buffer mode, empty buffer
		return 0;
	return 1;	// small buffer mode, non-empty buffer
}

// _SwitchToCacheMode
status_t
DataContainer::_SwitchToCacheMode()
{
	status_t error = VMCacheFactory::CreateAnonymousCache(fCache, false, 0,
		0, false, VM_PRIORITY_SYSTEM);
	if (error != B_OK)
		return error;

	fCache->temporary = 1;
	fCache->virtual_end = fSize;

	error = fCache->Commit(fSize, VM_PRIORITY_SYSTEM);
	if (error != B_OK)
		return error;

	if (fSize != 0)
		error = _DoCacheIO(0, fSmallBuffer, fSize, NULL, true);

	return error;
}

// _DoCacheIO
status_t
DataContainer::_DoCacheIO(const off_t offset, uint8* buffer, ssize_t length,
	size_t* bytesProcessed, bool isWrite)
{
	const size_t originalLength = length;
	const bool user = IS_USER_ADDRESS(buffer);

	const off_t rounded_offset = ROUNDDOWN(offset, B_PAGE_SIZE);
	const size_t rounded_len = ROUNDUP((length) + (offset - rounded_offset),
		B_PAGE_SIZE);
	vm_page** pages = new(std::nothrow) vm_page*[rounded_len / B_PAGE_SIZE];
	if (pages == NULL)
		return B_NO_MEMORY;
	ArrayDeleter<vm_page*> pagesDeleter(pages);

	_GetPages(rounded_offset, rounded_len, isWrite, pages);

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

	_PutPages(rounded_offset, rounded_len, pages, error == B_OK);

	if (bytesProcessed != NULL)
		*bytesProcessed = length > 0 ? originalLength - length : originalLength;

	return error;
}

// _GetPages
void
DataContainer::_GetPages(off_t offset, off_t length, bool isWrite,
	vm_page** pages)
{
	// TODO: This method is duplicated in the ram_disk. Perhaps it
	// should be put into a common location?

	// get the pages, we already have
	AutoLocker<VMCache> locker(fCache);

	size_t pageCount = length / B_PAGE_SIZE;
	size_t index = 0;
	size_t missingPages = 0;

	while (length > 0) {
		vm_page* page = fCache->LookupPage(offset);
		if (page != NULL) {
			if (page->busy) {
				fCache->WaitForPageEvents(page, PAGE_EVENT_NOT_BUSY, true);
				continue;
			}

			DEBUG_PAGE_ACCESS_START(page);
			page->busy = true;
		} else
			missingPages++;

		pages[index++] = page;
		offset += B_PAGE_SIZE;
		length -= B_PAGE_SIZE;
	}

	locker.Unlock();

	// For a write we need to reserve the missing pages.
	if (isWrite && missingPages > 0) {
		vm_page_reservation reservation;
		vm_page_reserve_pages(&reservation, missingPages,
			VM_PRIORITY_SYSTEM);

		for (size_t i = 0; i < pageCount; i++) {
			if (pages[i] != NULL)
				continue;

			pages[i] = vm_page_allocate_page(&reservation,
				PAGE_STATE_WIRED | VM_PAGE_ALLOC_BUSY);

			if (--missingPages == 0)
				break;
		}

		vm_page_unreserve_pages(&reservation);
	}
}

void
DataContainer::_PutPages(off_t offset, off_t length, vm_page** pages,
	bool success)
{
	// TODO: This method is duplicated in the ram_disk. Perhaps it
	// should be put into a common location?

	AutoLocker<VMCache> locker(fCache);

	// Mark all pages unbusy. On error free the newly allocated pages.
	size_t index = 0;

	while (length > 0) {
		vm_page* page = pages[index++];
		if (page != NULL) {
			if (page->CacheRef() == NULL) {
				if (success) {
					fCache->InsertPage(page, offset);
					fCache->MarkPageUnbusy(page);
					DEBUG_PAGE_ACCESS_END(page);
				} else
					vm_page_free(NULL, page);
			} else {
				fCache->MarkPageUnbusy(page);
				DEBUG_PAGE_ACCESS_END(page);
			}
		}

		offset += B_PAGE_SIZE;
		length -= B_PAGE_SIZE;
	}
}
