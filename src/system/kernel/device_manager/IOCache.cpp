/*
 * Copyright 2010-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "IOCache.h"

#include <algorithm>

#include <condition_variable.h>
#include <heap.h>
#include <low_resource_manager.h>
#include <util/AutoLock.h>
#include <vm/vm.h>
#include <vm/VMAddressSpace.h>
#include <vm/VMCache.h>
#include <vm/VMTranslationMap.h>


//#define TRACE_IO_CACHE 1
#ifdef TRACE_IO_CACHE
#	define TRACE(format...)	dprintf(format)
#else
#	define TRACE(format...)	do {} while (false)
#endif


static inline bool
page_physical_number_less(const vm_page* a, const vm_page* b)
{
	return a->physical_page_number < b->physical_page_number;
}


struct IOCache::Operation : IOOperation {
	ConditionVariable	finishedCondition;
};


IOCache::IOCache(DMAResource* resource, size_t cacheLineSize)
	:
	IOScheduler(resource),
	fDeviceCapacity(0),
	fLineSize(cacheLineSize),
	fPagesPerLine(cacheLineSize / B_PAGE_SIZE),
	fArea(-1),
	fCache(NULL),
	fPages(NULL),
	fVecs(NULL)
{
	ASSERT(resource != NULL);
	TRACE("%p->IOCache::IOCache(%p, %" B_PRIuSIZE ")\n", this, resource,
		cacheLineSize);

	if (cacheLineSize < B_PAGE_SIZE
		|| (cacheLineSize & (cacheLineSize - 1)) != 0) {
		panic("Invalid cache line size (%" B_PRIuSIZE "). Must be a power of 2 "
			"multiple of the page size.", cacheLineSize);
	}

	mutex_init(&fSerializationLock, "I/O cache request serialization");

	fLineSizeShift = 0;
	while (cacheLineSize != 1) {
		fLineSizeShift++;
		cacheLineSize >>= 1;
	}
}


IOCache::~IOCache()
{
	if (fArea >= 0) {
		vm_page_unreserve_pages(&fMappingReservation);
		delete_area(fArea);
	}

	delete[] fPages;
	delete[] fVecs;

	mutex_destroy(&fSerializationLock);
}


status_t
IOCache::Init(const char* name)
{
	TRACE("%p->IOCache::Init(\"%s\")\n", this, name);

	status_t error = IOScheduler::Init(name);
	if (error != B_OK)
		return error;

	// create the area for mapping cache lines
	fArea = vm_create_null_area(B_SYSTEM_TEAM, "I/O cache line", &fAreaBase,
		B_ANY_KERNEL_ADDRESS, fLineSize, 0);
	if (fArea < 0)
		return fArea;

	// reserve pages for mapping a complete cache line
	VMAddressSpace* addressSpace = VMAddressSpace::Kernel();
	VMTranslationMap* translationMap = addressSpace->TranslationMap();
	size_t pagesNeeded = translationMap->MaxPagesNeededToMap((addr_t)fAreaBase,
		(addr_t)fAreaBase + fLineSize - 1);
	vm_page_reserve_pages(&fMappingReservation, pagesNeeded,
		VM_PRIORITY_SYSTEM);

	// get the area's cache
	VMArea* area = VMAreaHash::Lookup(fArea);
	if (area == NULL) {
		panic("IOCache::Init(): Where's our area (id: %" B_PRId32 ")?!", fArea);
		return B_ERROR;
	}
	fCache = area->cache;

	// allocate arrays for pages and io vecs
	fPages = new(std::nothrow) vm_page*[fPagesPerLine];
	fVecs = new(std::nothrow) generic_io_vec[fPagesPerLine];
	if (fPages == NULL || fVecs == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


void
IOCache::SetDeviceCapacity(off_t deviceCapacity)
{
	TRACE("%p->IOCache::SetDeviceCapacity(%" B_PRIdOFF ")\n", this,
		deviceCapacity);

	MutexLocker serializationLocker(fSerializationLock);
	AutoLocker<VMCache> cacheLocker(fCache);

	fDeviceCapacity = deviceCapacity;
}


void
IOCache::MediaChanged()
{
	TRACE("%p->IOCache::MediaChanged()\n", this);

	MutexLocker serializationLocker(fSerializationLock);
	AutoLocker<VMCache> cacheLocker(fCache);

	// new media -- burn all cached data
	while (vm_page* page = fCache->pages.Root()) {
		DEBUG_PAGE_ACCESS_START(page);
		fCache->RemovePage(page);
		vm_page_free(NULL, page);
	}
}


status_t
IOCache::ScheduleRequest(IORequest* request)
{
	TRACE("%p->IOCache::ScheduleRequest(%p)\n", this, request);

	// lock the request's memory
	status_t error;
	IOBuffer* buffer = request->Buffer();
	if (buffer->IsVirtual()) {
		error = buffer->LockMemory(request->TeamID(), request->IsWrite());
		if (error != B_OK) {
			request->SetStatusAndNotify(error);
			return error;
		}
	}

	// we completely serialize all I/O in FIFO order
	MutexLocker serializationLocker(fSerializationLock);
	generic_size_t bytesTransferred = 0;
	error = _DoRequest(request, bytesTransferred);
	serializationLocker.Unlock();

	// unlock memory
	if (buffer->IsVirtual())
		buffer->UnlockMemory(request->TeamID(), request->IsWrite());

	// set status and notify
	if (error == B_OK) {
		request->SetTransferredBytes(bytesTransferred < request->Length(),
			bytesTransferred);
		request->SetStatusAndNotify(B_OK);
	} else
		request->SetStatusAndNotify(error);

	return error;
}


void
IOCache::AbortRequest(IORequest* request, status_t status)
{
	// TODO:...
}


void
IOCache::OperationCompleted(IOOperation* operation, status_t status,
	generic_size_t transferredBytes)
{
	if (status == B_OK) {
		// always fail in case of partial transfers
		((Operation*)operation)->finishedCondition.NotifyAll(false,
			transferredBytes == operation->Length() ? B_OK : B_ERROR);
	} else
		((Operation*)operation)->finishedCondition.NotifyAll(false, status);
}


void
IOCache::Dump() const
{
	kprintf("IOCache at %p\n", this);
	kprintf("  DMA resource:   %p\n", fDMAResource);
}


status_t
IOCache::_DoRequest(IORequest* request, generic_size_t& _bytesTransferred)
{
	off_t offset = request->Offset();
	generic_size_t length = request->Length();

	TRACE("%p->IOCache::ScheduleRequest(%p): offset: %" B_PRIdOFF
		", length: %" B_PRIuSIZE "\n", this, request, offset, length);

	if (offset < 0 || offset > fDeviceCapacity)
		return B_BAD_VALUE;

	// truncate the request to the device capacity
	if (fDeviceCapacity - offset < (off_t)length)
		length = fDeviceCapacity - offset;

	_bytesTransferred = 0;

	while (length > 0) {
		// the start of the current cache line
		off_t lineOffset = (offset >> fLineSizeShift) << fLineSizeShift;

		// intersection of request and cache line
		off_t cacheLineEnd = std::min(lineOffset + (off_t)fLineSize, fDeviceCapacity);
		size_t requestLineLength
			= std::min(cacheLineEnd - offset, (off_t)length);

		// transfer the data of the cache line
		status_t error = _TransferRequestLine(request, lineOffset,
			cacheLineEnd - lineOffset, offset, requestLineLength);
		if (error != B_OK)
			return error;

		offset = cacheLineEnd;
		length -= requestLineLength;
		_bytesTransferred += requestLineLength;
	}

	return B_OK;
}


status_t
IOCache::_TransferRequestLine(IORequest* request, off_t lineOffset,
	size_t lineSize, off_t requestOffset, size_t requestLength)
{
	TRACE("%p->IOCache::_TransferRequestLine(%p, %" B_PRIdOFF
		", %" B_PRIdOFF  ", %" B_PRIuSIZE ")\n", this, request, lineOffset,
		requestOffset, requestLength);

	// check whether there are pages of the cache line and the mark them used
	page_num_t firstPageOffset = lineOffset / B_PAGE_SIZE;
	page_num_t linePageCount = (lineSize + B_PAGE_SIZE - 1) / B_PAGE_SIZE;

	AutoLocker<VMCache> cacheLocker(fCache);

	page_num_t firstMissing = 0;
	page_num_t lastMissing = 0;
	page_num_t missingPages = 0;
	page_num_t pageOffset = firstPageOffset;

	VMCachePagesTree::Iterator it = fCache->pages.GetIterator(pageOffset, true,
		true);
	while (pageOffset < firstPageOffset + linePageCount) {
		vm_page* page = it.Next();
		page_num_t currentPageOffset;
		if (page == NULL
			|| page->cache_offset >= firstPageOffset + linePageCount) {
			page = NULL;
			currentPageOffset = firstPageOffset + linePageCount;
		} else
			currentPageOffset = page->cache_offset;

		if (pageOffset < currentPageOffset) {
			// pages are missing
			if (missingPages == 0)
				firstMissing = pageOffset;
			lastMissing = currentPageOffset - 1;
			missingPages += currentPageOffset - pageOffset;

			for (; pageOffset < currentPageOffset; pageOffset++)
				fPages[pageOffset - firstPageOffset] = NULL;
		}

		if (page != NULL) {
			fPages[pageOffset++ - firstPageOffset] = page;
			DEBUG_PAGE_ACCESS_START(page);
			vm_page_set_state(page, PAGE_STATE_UNUSED);
			DEBUG_PAGE_ACCESS_END(page);
		}
	}

	cacheLocker.Unlock();

	bool isVIP = (request->Flags() & B_VIP_IO_REQUEST) != 0;

	if (missingPages > 0) {
// TODO: If this is a read request and the missing pages range doesn't intersect
// with the request, just satisfy the request and don't read anything at all.
		// There are pages of the cache line missing. We have to allocate fresh
		// ones.

		// reserve
		vm_page_reservation reservation;
		if (!vm_page_try_reserve_pages(&reservation, missingPages,
				VM_PRIORITY_SYSTEM)) {
			_DiscardPages(firstMissing - firstPageOffset, missingPages);

			// fall back to uncached transfer
			return _TransferRequestLineUncached(request, lineOffset,
				requestOffset, requestLength);
		}

		// Allocate the missing pages and remove the already existing pages in
		// the range from the cache. We're going to read/write the whole range
		// anyway and this way we can sort it, possibly improving the physical
		// vecs.
// TODO: When memory is low, we should consider cannibalizing ourselves or
// simply transferring past the cache!
		for (pageOffset = firstMissing; pageOffset <= lastMissing;
				pageOffset++) {
			page_num_t index = pageOffset - firstPageOffset;
			if (fPages[index] == NULL) {
				fPages[index] = vm_page_allocate_page(&reservation,
					PAGE_STATE_UNUSED);
				DEBUG_PAGE_ACCESS_END(fPages[index]);
			} else {
				cacheLocker.Lock();
				fCache->RemovePage(fPages[index]);
				cacheLocker.Unlock();
			}
		}

		missingPages = lastMissing - firstMissing + 1;

		// sort the page array by physical page number
		std::sort(fPages + firstMissing - firstPageOffset,
			fPages + lastMissing - firstPageOffset + 1,
			page_physical_number_less);

		// add the pages to the cache
		cacheLocker.Lock();

		for (pageOffset = firstMissing; pageOffset <= lastMissing;
				pageOffset++) {
			page_num_t index = pageOffset - firstPageOffset;
			fCache->InsertPage(fPages[index], (off_t)pageOffset * B_PAGE_SIZE);
		}

		cacheLocker.Unlock();

		// Read in the missing pages, if this is a read request or a write
		// request that doesn't cover the complete missing range.
		if (request->IsRead()
			|| requestOffset < (off_t)firstMissing * B_PAGE_SIZE
			|| requestOffset + (off_t)requestLength
				> (off_t)(lastMissing + 1) * B_PAGE_SIZE) {
			status_t error = _TransferPages(firstMissing - firstPageOffset,
				missingPages, false, isVIP);
			if (error != B_OK) {
				dprintf("IOCache::_TransferRequestLine(): Failed to read into "
					"cache (offset: %" B_PRIdOFF ", length: %" B_PRIuSIZE "), "
					"trying uncached read (offset: %" B_PRIdOFF ", length: %"
					B_PRIuSIZE ")\n", (off_t)firstMissing * B_PAGE_SIZE,
					(size_t)missingPages * B_PAGE_SIZE, requestOffset,
					requestLength);

				_DiscardPages(firstMissing - firstPageOffset, missingPages);

				// Try again using an uncached transfer
				return _TransferRequestLineUncached(request, lineOffset,
					requestOffset, requestLength);
			}
		}
	}

	if (request->IsRead()) {
		// copy data to request
		status_t error = _CopyPages(request, requestOffset - lineOffset,
			requestOffset, requestLength, true);
		_CachePages(0, linePageCount);
		return error;
	}

	// copy data from request
	status_t error = _CopyPages(request, requestOffset - lineOffset,
		requestOffset, requestLength, false);
	if (error != B_OK) {
		_DiscardPages(0, linePageCount);
		return error;
	}

	// write the pages to disk
	page_num_t firstPage = (requestOffset - lineOffset) / B_PAGE_SIZE;
	page_num_t endPage = (requestOffset + requestLength - lineOffset
		+ B_PAGE_SIZE - 1) / B_PAGE_SIZE;
	error = _TransferPages(firstPage, endPage - firstPage, true, isVIP);

	if (error != B_OK) {
		_DiscardPages(firstPage, endPage - firstPage);
		return error;
	}

	_CachePages(0, linePageCount);
	return error;
}


status_t
IOCache::_TransferRequestLineUncached(IORequest* request, off_t lineOffset,
	off_t requestOffset, size_t requestLength)
{
	TRACE("%p->IOCache::_TransferRequestLineUncached(%p, %" B_PRIdOFF
		", %" B_PRIdOFF  ", %" B_PRIuSIZE ")\n", this, request, lineOffset,
		requestOffset, requestLength);

	// Advance the request to the interesting offset, so the DMAResource can
	// provide us with fitting operations.
	off_t actualRequestOffset
		= request->Offset() + request->Length() - request->RemainingBytes();
	if (actualRequestOffset > requestOffset) {
		dprintf("IOCache::_TransferRequestLineUncached(): Request %p advanced "
			"beyond current cache line (%" B_PRIdOFF " vs. %" B_PRIdOFF ")\n",
			request, actualRequestOffset, requestOffset);
		return B_BAD_VALUE;
	}

	if (actualRequestOffset < requestOffset)
		request->Advance(requestOffset - actualRequestOffset);

	generic_size_t requestRemaining = request->RemainingBytes() - requestLength;

	// Process single operations until the specified part of the request is
	// finished or until an error occurs.
	Operation operation;
	operation.finishedCondition.Init(this, "I/O cache operation finished");

	while (request->RemainingBytes() > requestRemaining
		&& request->Status() > 0) {
		status_t error = fDMAResource->TranslateNext(request, &operation,
			request->RemainingBytes() - requestRemaining);
		if (error != B_OK)
			return error;

		error = _DoOperation(operation);

		request->OperationFinished(&operation, error, false,
			error == B_OK ? operation.OriginalLength() : 0);
		request->SetUnfinished();
			// Keep the request in unfinished state. ScheduleRequest() will set
			// the final status and notify.

		fDMAResource->RecycleBuffer(operation.Buffer());

		if (error != B_OK) {
			TRACE("%p->IOCache::_TransferRequestLineUncached(): operation at "
				"%" B_PRIdOFF " failed: %s\n", this, operation.Offset(),
				strerror(error));
			return error;
		}
	}

	return B_OK;
}


status_t
IOCache::_DoOperation(Operation& operation)
{
	TRACE("%p->IOCache::_DoOperation(%" B_PRIdOFF ", %" B_PRIuSIZE ")\n", this,
		operation.Offset(), operation.Length());

	while (true) {
		ConditionVariableEntry waitEntry;
		operation.finishedCondition.Add(&waitEntry);

		status_t error = fIOCallback(fIOCallbackData, &operation);
		if (error != B_OK) {
			operation.finishedCondition.NotifyAll(false, error);
				// removes the entry from the variable
			return error;
		}

		// wait for the operation to finish
		error = waitEntry.Wait();
		if (error != B_OK)
			return error;

		if (operation.Finish())
			return B_OK;
	}
}


status_t
IOCache::_TransferPages(size_t firstPage, size_t pageCount, bool isWrite,
	bool isVIP)
{
	TRACE("%p->IOCache::_TransferPages(%" B_PRIuSIZE ", %" B_PRIuSIZE
		", write: %d, vip: %d)\n", this, firstPage, pageCount, isWrite, isVIP);

	off_t firstPageOffset = (off_t)fPages[firstPage]->cache_offset
		* B_PAGE_SIZE;
	generic_size_t requestLength = std::min(
			firstPageOffset + (off_t)pageCount * B_PAGE_SIZE, fDeviceCapacity)
		- firstPageOffset;

	// prepare the I/O vecs
	size_t vecCount = 0;
	size_t endPage = firstPage + pageCount;
	phys_addr_t vecsEndAddress = 0;
	for (size_t i = firstPage; i < endPage; i++) {
		phys_addr_t pageAddress
			= (phys_addr_t)fPages[i]->physical_page_number * B_PAGE_SIZE;
		if (vecCount == 0 || pageAddress != vecsEndAddress) {
			fVecs[vecCount].base = pageAddress;
			fVecs[vecCount++].length = B_PAGE_SIZE;
			vecsEndAddress = pageAddress + B_PAGE_SIZE;
		} else {
			// extend the previous vec
			fVecs[vecCount - 1].length += B_PAGE_SIZE;
			vecsEndAddress += B_PAGE_SIZE;
		}
	}

	// create a request for the transfer
	IORequest request;
	status_t error = request.Init(firstPageOffset, fVecs, vecCount,
		requestLength, isWrite,
		B_PHYSICAL_IO_REQUEST | (isVIP ? B_VIP_IO_REQUEST : 0));
	if (error != B_OK)
		return error;

	// Process single operations until the complete request is finished or
	// until an error occurs.
	Operation operation;
	operation.finishedCondition.Init(this, "I/O cache operation finished");

	while (request.RemainingBytes() > 0 && request.Status() > 0) {
		error = fDMAResource->TranslateNext(&request, &operation,
			requestLength);
		if (error != B_OK)
			return error;

		error = _DoOperation(operation);

		request.RemoveOperation(&operation);

		fDMAResource->RecycleBuffer(operation.Buffer());

		if (error != B_OK) {
			TRACE("%p->IOCache::_TransferLine(): operation at %" B_PRIdOFF
				" failed: %s\n", this, operation.Offset(), strerror(error));
			return error;
		}
	}

	return B_OK;
}


/*!	Frees all pages in given range of the \c fPages array.
	\c NULL entries in the range are OK. All non \c NULL entries must refer
	to pages with \c PAGE_STATE_UNUSED. The pages may belong to \c fCache or
	may not have a cache.
	\c fCache must not be locked.
*/
void
IOCache::_DiscardPages(size_t firstPage, size_t pageCount)
{
	TRACE("%p->IOCache::_DiscardPages(%" B_PRIuSIZE ", %" B_PRIuSIZE ")\n",
		this, firstPage, pageCount);

	AutoLocker<VMCache> cacheLocker(fCache);

	for (size_t i = firstPage; i < firstPage + pageCount; i++) {
		vm_page* page = fPages[i];
		if (page == NULL)
			continue;

		DEBUG_PAGE_ACCESS_START(page);

		ASSERT_PRINT(page->State() == PAGE_STATE_UNUSED,
			"page: %p @! page -m %p", page, page);

		if (page->Cache() != NULL)
			fCache->RemovePage(page);

		vm_page_free(NULL, page);
	}
}


/*!	Marks all pages in the given range of the \c fPages array cached.
	There must not be any \c NULL entries in the given array range. All pages
	must belong to \c cache and have state \c PAGE_STATE_UNUSED.
	\c fCache must not be locked.
*/
void
IOCache::_CachePages(size_t firstPage, size_t pageCount)
{
	TRACE("%p->IOCache::_CachePages(%" B_PRIuSIZE ", %" B_PRIuSIZE ")\n",
		this, firstPage, pageCount);

	AutoLocker<VMCache> cacheLocker(fCache);

	for (size_t i = firstPage; i < firstPage + pageCount; i++) {
		vm_page* page = fPages[i];
		ASSERT(page != NULL);
		ASSERT_PRINT(page->State() == PAGE_STATE_UNUSED
				&& page->Cache() == fCache,
			"page: %p @! page -m %p", page, page);

		DEBUG_PAGE_ACCESS_START(page);
		vm_page_set_state(page, PAGE_STATE_CACHED);
		DEBUG_PAGE_ACCESS_END(page);
	}
}


/*!	Copies the contents of pages in \c fPages to \a request, or vice versa.
	\param request The request.
	\param pagesRelativeOffset The offset relative to \c fPages[0] where to
		start copying.
	\param requestOffset The request offset where to start copying.
	\param requestLength The number of bytes to copy.
	\param toRequest If \c true the copy directory is from \c fPages to
		\a request, otherwise the other way around.
	\return \c B_OK, if copying went fine, another error code otherwise.
*/
status_t
IOCache::_CopyPages(IORequest* request, size_t pagesRelativeOffset,
	off_t requestOffset, size_t requestLength, bool toRequest)
{
	TRACE("%p->IOCache::_CopyPages(%p, %" B_PRIuSIZE ", %" B_PRIdOFF
		", %" B_PRIuSIZE ", %d)\n", this, request, pagesRelativeOffset,
		requestOffset, requestLength, toRequest);

	size_t firstPage = pagesRelativeOffset / B_PAGE_SIZE;
	size_t endPage = (pagesRelativeOffset + requestLength + B_PAGE_SIZE - 1)
		/ B_PAGE_SIZE;

	// map the pages
	status_t error = _MapPages(firstPage, endPage);
// TODO: _MapPages() cannot fail, so the fallback is never needed. Test which
// method is faster (probably the active one)!
#if 0
	if (error != B_OK) {
		// fallback to copying individual pages
		size_t inPageOffset = pagesRelativeOffset % B_PAGE_SIZE;
		for (size_t i = firstPage; i < endPage; i++) {
			// map the page
			void* handle;
			addr_t address;
			error = vm_get_physical_page(
				fPages[i]->physical_page_number * B_PAGE_SIZE, &address,
				&handle);
			if (error != B_OK)
				return error;

			// copy the page's data
			size_t toCopy = std::min(B_PAGE_SIZE - inPageOffset, requestLength);

			if (toRequest) {
				error = request->CopyData((uint8*)(address + inPageOffset),
					requestOffset, toCopy);
			} else {
				error = request->CopyData(requestOffset,
					(uint8*)(address + inPageOffset), toCopy);
			}

			// unmap the page
			vm_put_physical_page(address, handle);

			if (error != B_OK)
				return error;

			inPageOffset = 0;
			requestOffset += toCopy;
			requestLength -= toCopy;
		}

		return B_OK;
	}
#endif	// 0

	// copy
	if (toRequest) {
		error = request->CopyData((uint8*)fAreaBase + pagesRelativeOffset,
			requestOffset, requestLength);
	} else {
		error = request->CopyData(requestOffset,
			(uint8*)fAreaBase + pagesRelativeOffset, requestLength);
	}

	// unmap the pages
	_UnmapPages(firstPage, endPage);

	return error;
}


/*!	Maps a range of pages in \c fPages into fArea.

	If successful, it must be balanced by a call to _UnmapPages().

	\param firstPage The \c fPages relative index of the first page to map.
	\param endPage The \c fPages relative index of the page after the last page
		to map.
	\return \c B_OK, if mapping went fine, another error code otherwise.
*/
status_t
IOCache::_MapPages(size_t firstPage, size_t endPage)
{
	VMTranslationMap* translationMap
		= VMAddressSpace::Kernel()->TranslationMap();

	translationMap->Lock();

	for (size_t i = firstPage; i < endPage; i++) {
		vm_page* page = fPages[i];

		ASSERT_PRINT(page->State() == PAGE_STATE_UNUSED,
			"page: %p @! page -m %p", page, page);

		translationMap->Map((addr_t)fAreaBase + i * B_PAGE_SIZE,
			page->physical_page_number * B_PAGE_SIZE,
			B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, 0, &fMappingReservation);
		// NOTE: We don't increment gMappedPagesCount. Our pages have state
		// PAGE_STATE_UNUSED anyway and we map them only for a short time.
	}

	translationMap->Unlock();

	return B_OK;
}


/*!	Unmaps a range of pages in \c fPages into fArea.

	Must balance a call to _MapPages().

	\param firstPage The \c fPages relative index of the first page to unmap.
	\param endPage The \c fPages relative index of the page after the last page
		to unmap.
*/
void
IOCache::_UnmapPages(size_t firstPage, size_t endPage)
{
	VMTranslationMap* translationMap
		= VMAddressSpace::Kernel()->TranslationMap();

	translationMap->Lock();

	translationMap->Unmap((addr_t)fAreaBase + firstPage * B_PAGE_SIZE,
		(addr_t)fAreaBase + endPage * B_PAGE_SIZE - 1);

	translationMap->Unlock();
}
