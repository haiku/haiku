/*
 * Copyright 2010-2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "CachedDataReader.h"

#include <algorithm>

#include <DataIO.h>

#include <util/AutoLock.h>
#include <vm/VMCache.h>
#include <vm/vm_page.h>

#include "DebugSupport.h"


using BPackageKit::BHPKG::BBufferDataReader;


static inline bool
page_physical_number_less(const vm_page* a, const vm_page* b)
{
	return a->physical_page_number < b->physical_page_number;
}


// #pragma mark - PagesDataOutput


struct CachedDataReader::PagesDataOutput : public BDataIO {
	PagesDataOutput(vm_page** pages, size_t pageCount)
		:
		fPages(pages),
		fPageCount(pageCount),
		fInPageOffset(0)
	{
	}

	virtual ssize_t Write(const void* buffer, size_t size)
	{
		size_t bytesRemaining = size;
		while (bytesRemaining > 0) {
			if (fPageCount == 0)
				return B_BAD_VALUE;

			size_t toCopy = std::min(bytesRemaining,
				B_PAGE_SIZE - fInPageOffset);
			status_t error = vm_memcpy_to_physical(
				fPages[0]->physical_page_number * B_PAGE_SIZE + fInPageOffset,
				buffer, toCopy, false);
			if (error != B_OK)
				return error;

			fInPageOffset += toCopy;
			if (fInPageOffset == B_PAGE_SIZE) {
				fInPageOffset = 0;
				fPages++;
				fPageCount--;
			}

			buffer = (const char*)buffer + toCopy;
			bytesRemaining -= toCopy;
		}

		return size;
	}

private:
	vm_page**	fPages;
	size_t		fPageCount;
	size_t		fInPageOffset;
};


// #pragma mark - CachedDataReader


CachedDataReader::CachedDataReader()
	:
	fReader(NULL),
	fCache(NULL),
	fCacheLineLockers()
{
	mutex_init(&fLock, "packagefs cached reader");
}


CachedDataReader::~CachedDataReader()
{
	if (fCache != NULL) {
		fCache->Lock();
		fCache->ReleaseRefAndUnlock();
	}

	mutex_destroy(&fLock);
}


status_t
CachedDataReader::Init(BAbstractBufferedDataReader* reader, off_t size)
{
	fReader = reader;

	status_t error = fCacheLineLockers.Init();
	if (error != B_OK)
		RETURN_ERROR(error);

	error = VMCacheFactory::CreateNullCache(VM_PRIORITY_SYSTEM,
		fCache);
	if (error != B_OK)
		RETURN_ERROR(error);

	AutoLocker<VMCache> locker(fCache);

	error = fCache->Resize(size, VM_PRIORITY_SYSTEM);
	if (error != B_OK)
		RETURN_ERROR(error);

	return B_OK;
}


status_t
CachedDataReader::ReadDataToOutput(off_t offset, size_t size,
	BDataIO* output)
{
	if (offset > fCache->virtual_end
		|| (off_t)size > fCache->virtual_end - offset) {
		return B_BAD_VALUE;
	}

	if (size == 0)
		return B_OK;

	while (size > 0) {
		// the start of the current cache line
		off_t lineOffset = (offset / kCacheLineSize) * kCacheLineSize;

		// intersection of request and cache line
		off_t cacheLineEnd = std::min(lineOffset + (off_t)kCacheLineSize,
			fCache->virtual_end);
		size_t requestLineLength
			= std::min(cacheLineEnd - offset, (off_t)size);

		// transfer the data of the cache line
		status_t error = _ReadCacheLine(lineOffset, cacheLineEnd - lineOffset,
			offset, requestLineLength, output);
		if (error != B_OK)
			return error;

		offset = cacheLineEnd;
		size -= requestLineLength;
	}

	return B_OK;
}


status_t
CachedDataReader::_ReadCacheLine(off_t lineOffset, size_t lineSize,
	off_t requestOffset, size_t requestLength, BDataIO* output)
{
	PRINT("CachedDataReader::_ReadCacheLine(%" B_PRIdOFF ", %zu, %" B_PRIdOFF
		", %zu, %p\n", lineOffset, lineSize, requestOffset, requestLength,
		output);

	CacheLineLocker cacheLineLocker(this, lineOffset);

	// check whether there are pages of the cache line and the mark them used
	page_num_t firstPageOffset = lineOffset / B_PAGE_SIZE;
	page_num_t linePageCount = (lineSize + B_PAGE_SIZE - 1) / B_PAGE_SIZE;
	vm_page* pages[kPagesPerCacheLine] = {};

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
				pages[pageOffset - firstPageOffset] = NULL;
		}

		if (page != NULL) {
			pages[pageOffset++ - firstPageOffset] = page;
			DEBUG_PAGE_ACCESS_START(page);
			vm_page_set_state(page, PAGE_STATE_UNUSED);
			DEBUG_PAGE_ACCESS_END(page);
		}
	}

	cacheLocker.Unlock();

	if (missingPages > 0) {
// TODO: If the missing pages range doesn't intersect with the request, just
// satisfy the request and don't read anything at all.
		// There are pages of the cache line missing. We have to allocate fresh
		// ones.

		// reserve
		vm_page_reservation reservation;
		if (!vm_page_try_reserve_pages(&reservation, missingPages,
				VM_PRIORITY_SYSTEM)) {
			_DiscardPages(pages, firstMissing - firstPageOffset, missingPages);

			// fall back to uncached transfer
			return fReader->ReadDataToOutput(requestOffset, requestLength,
				output);
		}

		// Allocate the missing pages and remove the already existing pages in
		// the range from the cache. We're going to read/write the whole range
		// anyway.
		for (pageOffset = firstMissing; pageOffset <= lastMissing;
				pageOffset++) {
			page_num_t index = pageOffset - firstPageOffset;
			if (pages[index] == NULL) {
				pages[index] = vm_page_allocate_page(&reservation,
					PAGE_STATE_UNUSED);
				DEBUG_PAGE_ACCESS_END(pages[index]);
			} else {
				cacheLocker.Lock();
				fCache->RemovePage(pages[index]);
				cacheLocker.Unlock();
			}
		}

		missingPages = lastMissing - firstMissing + 1;

		// add the pages to the cache
		cacheLocker.Lock();

		for (pageOffset = firstMissing; pageOffset <= lastMissing;
				pageOffset++) {
			page_num_t index = pageOffset - firstPageOffset;
			fCache->InsertPage(pages[index], (off_t)pageOffset * B_PAGE_SIZE);
		}

		cacheLocker.Unlock();

		// read in the missing pages
		status_t error = _ReadIntoPages(pages, firstMissing - firstPageOffset,
			missingPages);
		if (error != B_OK) {
			ERROR("CachedDataReader::_ReadCacheLine(): Failed to read into "
				"cache (offset: %" B_PRIdOFF ", length: %" B_PRIuSIZE "), "
				"trying uncached read (offset: %" B_PRIdOFF ", length: %"
				B_PRIuSIZE ")\n", (off_t)firstMissing * B_PAGE_SIZE,
				(size_t)missingPages * B_PAGE_SIZE, requestOffset,
				requestLength);

			_DiscardPages(pages, firstMissing - firstPageOffset, missingPages);

			// Try again using an uncached transfer
			return fReader->ReadDataToOutput(requestOffset, requestLength,
				output);
		}
	}

	// write data to output
	status_t error = _WritePages(pages, requestOffset - lineOffset,
		requestLength, output);
	_CachePages(pages, 0, linePageCount);
	return error;
}


/*!	Frees all pages in given range of the \a pages array.
	\c NULL entries in the range are OK. All non \c NULL entries must refer
	to pages with \c PAGE_STATE_UNUSED. The pages may belong to \c fCache or
	may not have a cache.
	\c fCache must not be locked.
*/
void
CachedDataReader::_DiscardPages(vm_page** pages, size_t firstPage,
	size_t pageCount)
{
	PRINT("%p->CachedDataReader::_DiscardPages(%" B_PRIuSIZE ", %" B_PRIuSIZE
		")\n", this, firstPage, pageCount);

	AutoLocker<VMCache> cacheLocker(fCache);

	for (size_t i = firstPage; i < firstPage + pageCount; i++) {
		vm_page* page = pages[i];
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


/*!	Marks all pages in the given range of the \a pages array cached.
	There must not be any \c NULL entries in the given array range. All pages
	must belong to \c cache and have state \c PAGE_STATE_UNUSED.
	\c fCache must not be locked.
*/
void
CachedDataReader::_CachePages(vm_page** pages, size_t firstPage,
	size_t pageCount)
{
	PRINT("%p->CachedDataReader::_CachePages(%" B_PRIuSIZE ", %" B_PRIuSIZE
		")\n", this, firstPage, pageCount);

	AutoLocker<VMCache> cacheLocker(fCache);

	for (size_t i = firstPage; i < firstPage + pageCount; i++) {
		vm_page* page = pages[i];
		ASSERT(page != NULL);
		ASSERT_PRINT(page->State() == PAGE_STATE_UNUSED
				&& page->Cache() == fCache,
			"page: %p @! page -m %p", page, page);

		DEBUG_PAGE_ACCESS_START(page);
		vm_page_set_state(page, PAGE_STATE_CACHED);
		DEBUG_PAGE_ACCESS_END(page);
	}
}


/*!	Writes the contents of pages in \c pages to \a output.
	\param pages The pages array.
	\param pagesRelativeOffset The offset relative to \a pages[0] where to
		start writing from.
	\param requestLength The number of bytes to write.
	\param output The output to which the data shall be written.
	\return \c B_OK, if writing went fine, another error code otherwise.
*/
status_t
CachedDataReader::_WritePages(vm_page** pages, size_t pagesRelativeOffset,
	size_t requestLength, BDataIO* output)
{
	PRINT("%p->CachedDataReader::_WritePages(%" B_PRIuSIZE ", %" B_PRIuSIZE
		", %p)\n", this, pagesRelativeOffset, requestLength, output);

	size_t firstPage = pagesRelativeOffset / B_PAGE_SIZE;
	size_t endPage = (pagesRelativeOffset + requestLength + B_PAGE_SIZE - 1)
		/ B_PAGE_SIZE;

	// fallback to copying individual pages
	size_t inPageOffset = pagesRelativeOffset % B_PAGE_SIZE;
	for (size_t i = firstPage; i < endPage; i++) {
		// map the page
		void* handle;
		addr_t address;
		status_t error = vm_get_physical_page(
			pages[i]->physical_page_number * B_PAGE_SIZE, &address,
			&handle);
		if (error != B_OK)
			return error;

		// write the page's data
		size_t toCopy = std::min(B_PAGE_SIZE - inPageOffset, requestLength);
		error = output->WriteExactly((uint8*)(address + inPageOffset), toCopy);

		// unmap the page
		vm_put_physical_page(address, handle);

		if (error != B_OK)
			return error;

		inPageOffset = 0;
		requestLength -= toCopy;
	}

	return B_OK;
}


status_t
CachedDataReader::_ReadIntoPages(vm_page** pages, size_t firstPage,
	size_t pageCount)
{
	PagesDataOutput output(pages + firstPage, pageCount);

	off_t firstPageOffset = (off_t)pages[firstPage]->cache_offset
		* B_PAGE_SIZE;
	generic_size_t requestLength = std::min(
			firstPageOffset + (off_t)pageCount * B_PAGE_SIZE,
			fCache->virtual_end)
		- firstPageOffset;

	return fReader->ReadDataToOutput(firstPageOffset, requestLength, &output);
}


void
CachedDataReader::_LockCacheLine(CacheLineLocker* lineLocker)
{
	MutexLocker locker(fLock);

	CacheLineLocker* otherLineLocker
		= fCacheLineLockers.Lookup(lineLocker->Offset());
	if (otherLineLocker == NULL) {
		fCacheLineLockers.Insert(lineLocker);
		return;
	}

	// queue and wait
	otherLineLocker->Queue().Add(lineLocker);
	lineLocker->Wait(fLock);
}


void
CachedDataReader::_UnlockCacheLine(CacheLineLocker* lineLocker)
{
	MutexLocker locker(fLock);

	fCacheLineLockers.Remove(lineLocker);

	if (CacheLineLocker* nextLineLocker = lineLocker->Queue().RemoveHead()) {
		nextLineLocker->Queue().MoveFrom(&lineLocker->Queue());
		fCacheLineLockers.Insert(nextLineLocker);
		nextLineLocker->WakeUp();
	}
}
