/*
 * Copyright 2010-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <string.h>
#include <stdlib.h>

#include <algorithm>

#include <KernelExport.h>
#include <OS.h>

#include <AutoDeleter.h>
#include <util/AutoLock.h>

#include <heap.h>
#include <low_resource_manager.h>
#include <thread.h>
#include <vfs.h>
#include <vm/vm.h>
#include <vm/vm_priv.h>
#include <vm/vm_page.h>
#include <vm/VMCache.h>

#include "IORequest.h"
#include "PageCacheLocker.h"
#include "ModifiedPageQueue.h"


//#define TRACE_VM_PAGE_WRITER
#ifdef TRACE_VM_PAGE_WRITER
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

#define MAX_PAGE_WRITER_IO_PRIORITY				B_URGENT_DISPLAY_PRIORITY
	// maximum I/O priority of the page writer
#define MAX_PAGE_WRITER_IO_PRIORITY_THRESHOLD	10000
	// the maximum I/O priority shall be reached when this many pages need to
	// be written


#if PAGE_WRITER_TRACING

namespace PageWriterTracing {

class WritePage : public AbstractTraceEntry {
	public:
		WritePage(vm_page* page)
			:
			fCache(page->Cache()),
			fPage(page)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("page write: %p, cache: %p", fPage, fCache);
		}

	private:
		VMCache*	fCache;
		vm_page*	fPage;
};

}	// namespace PageWriterTracing

#	define TPW(x)	new(std::nothrow) PageWriterTracing::x

#else
#	define TPW(x)
#endif	// PAGE_WRITER_TRACING



class PageWriteTransfer;
class PageWriteWrapper;


class PageWriterRun {
public:
	status_t Init(uint32 maxPages);

	void PrepareNextRun();
	void AddPage(vm_page* page);
	uint32 Go();

	void PageWritten(PageWriteTransfer* transfer, status_t status,
		bool partialTransfer, size_t bytesTransferred);

private:
	uint32				fMaxPages;
	uint32				fWrapperCount;
	uint32				fTransferCount;
	int32				fPendingTransfers;
	PageWriteWrapper*	fWrappers;
	PageWriteTransfer*	fTransfers;
	ConditionVariable	fAllFinishedCondition;
};


class PageWriteTransfer : public AsyncIOCallback {
public:
	void SetTo(PageWriterRun* run, vm_page* page, int32 maxPages);
	bool AddPage(vm_page* page);

	status_t Schedule(uint32 flags);

	void SetStatus(status_t status, size_t transferred);

	status_t Status() const	{ return fStatus; }
	struct VMCache* Cache() const { return fCache; }
	uint32 PageCount() const { return fPageCount; }

	virtual void IOFinished(status_t status, bool partialTransfer,
		generic_size_t bytesTransferred);

private:
	PageWriterRun*		fRun;
	struct VMCache*		fCache;
	off_t				fOffset;
	uint32				fPageCount;
	int32				fMaxPages;
	status_t			fStatus;
	uint32				fVecCount;
	generic_io_vec		fVecs[32]; // TODO: make dynamic/configurable
};


class PageWriteWrapper {
public:
	PageWriteWrapper();
	~PageWriteWrapper();
	void SetTo(vm_page* page);
	bool Done(status_t result);

private:
	vm_page*			fPage;
	struct VMCache*		fCache;
	bool				fIsActive;
};


PageWriteWrapper::PageWriteWrapper()
	:
	fIsActive(false)
{
}


PageWriteWrapper::~PageWriteWrapper()
{
	if (fIsActive)
		panic("page write wrapper going out of scope but isn't completed");
}


/*!	The page's cache must be locked.
*/
void
PageWriteWrapper::SetTo(vm_page* page)
{
	DEBUG_PAGE_ACCESS_CHECK(page);

	if (page->busy)
		panic("setting page write wrapper to busy page");

	if (fIsActive)
		panic("re-setting page write wrapper that isn't completed");

	fPage = page;
	fCache = page->Cache();
	fIsActive = true;

	fPage->busy = true;
	fPage->busy_io = true;

	// We have a modified page -- however, while we're writing it back,
	// the page might still be mapped. In order not to lose any changes to the
	// page, we mark it clean before actually writing it back; if
	// writing the page fails for some reason, we'll just keep it in the
	// modified page list, but that should happen only rarely.

	// If the page is changed after we cleared the dirty flag, but before we
	// had the chance to write it back, then we'll write it again later -- that
	// will probably not happen that often, though.

	vm_clear_map_flags(fPage, PAGE_MODIFIED);
}


/*! Moves a previously modified page into a now appropriate queue.
	The page queues must not be locked.
*/
static void
move_page_to_appropriate_queue(vm_page *page)
{
	DEBUG_PAGE_ACCESS_CHECK(page);

	// Note, this logic must be in sync with what the page daemon does.
	int32 state;
	if (page->IsMapped())
		state = PAGE_STATE_ACTIVE;
	else if (page->modified)
		state = PAGE_STATE_MODIFIED;
	else
		state = PAGE_STATE_CACHED;

// TODO: If free + cached pages are low, we might directly want to free the
// page.
	vm_page_set_state(page, state);
}


/*!	The page's cache must be locked.
	The page queues must not be locked.
	\return \c true if the page was written successfully respectively could be
		handled somehow, \c false otherwise.
*/
bool
PageWriteWrapper::Done(status_t result)
{
	if (!fIsActive)
		panic("completing page write wrapper that is not active");

	DEBUG_PAGE_ACCESS_START(fPage);

	fPage->busy = false;
		// Set unbusy and notify later by hand.

	bool success = true;

	if (!fPage->busy_io) {
		// The busy_io flag was cleared. That means the cache tried to remove
		// the page while we were trying to write it. Let the cache handle the rest.
		fCache->FreeRemovedPage(fPage);
	} else if (result == B_OK) {
		// put it into the active/inactive queue
		move_page_to_appropriate_queue(fPage);
		fPage->busy_io = false;
		DEBUG_PAGE_ACCESS_END(fPage);

		fCache->NotifyPageEvents(fPage, PAGE_EVENT_NOT_BUSY);
	} else {
		// Writing the page failed -- mark the page modified and move it to
		// an appropriate queue other than the modified queue, so we don't
		// keep trying to write it over and over again. We keep
		// non-temporary pages in the modified queue, though, so they don't
		// get lost in the inactive queue.
		dprintf("PageWriteWrapper: Failed to write page %p: %s\n", fPage,
			strerror(result));

		fPage->modified = true;
		if (!fCache->temporary)
			vm_page_set_state(fPage, PAGE_STATE_MODIFIED);
		else if (fPage->IsMapped())
			vm_page_set_state(fPage, PAGE_STATE_ACTIVE);
		else
			vm_page_set_state(fPage, PAGE_STATE_INACTIVE);

		fPage->busy_io = false;
		DEBUG_PAGE_ACCESS_END(fPage);

		fCache->NotifyPageEvents(fPage, PAGE_EVENT_NOT_BUSY);
		success = false;
	}

	fIsActive = false;

	return success;
}


/*!	The page's cache must be locked.
*/
void
PageWriteTransfer::SetTo(PageWriterRun* run, vm_page* page, int32 maxPages)
{
	fRun = run;
	fCache = page->Cache();
	fOffset = page->cache_offset;
	fPageCount = 1;
	fMaxPages = maxPages;
	fStatus = B_OK;

	fVecs[0].base = (phys_addr_t)page->physical_page_number << PAGE_SHIFT;
	fVecs[0].length = B_PAGE_SIZE;
	fVecCount = 1;
}


/*!	The page's cache must be locked.
*/
bool
PageWriteTransfer::AddPage(vm_page* page)
{
	if (page->Cache() != fCache
		|| (fMaxPages >= 0 && fPageCount >= (uint32)fMaxPages))
		return false;

	phys_addr_t nextBase = fVecs[fVecCount - 1].base
		+ fVecs[fVecCount - 1].length;

	if ((phys_addr_t)page->physical_page_number << PAGE_SHIFT == nextBase
		&& (off_t)page->cache_offset == fOffset + fPageCount) {
		// append to last iovec
		fVecs[fVecCount - 1].length += B_PAGE_SIZE;
		fPageCount++;
		return true;
	}

	nextBase = fVecs[0].base - B_PAGE_SIZE;
	if ((phys_addr_t)page->physical_page_number << PAGE_SHIFT == nextBase
		&& (off_t)page->cache_offset == fOffset - 1) {
		// prepend to first iovec and adjust offset
		fVecs[0].base = nextBase;
		fVecs[0].length += B_PAGE_SIZE;
		fOffset = page->cache_offset;
		fPageCount++;
		return true;
	}

	if (((off_t)page->cache_offset == fOffset + fPageCount
			|| (off_t)page->cache_offset == fOffset - 1)
		&& fVecCount < sizeof(fVecs) / sizeof(fVecs[0])) {
		// not physically contiguous or not in the right order
		uint32 vectorIndex;
		if ((off_t)page->cache_offset < fOffset) {
			// we are pre-pending another vector, move the other vecs
			for (uint32 i = fVecCount; i > 0; i--)
				fVecs[i] = fVecs[i - 1];

			fOffset = page->cache_offset;
			vectorIndex = 0;
		} else
			vectorIndex = fVecCount;

		fVecs[vectorIndex].base
			= (phys_addr_t)page->physical_page_number << PAGE_SHIFT;
		fVecs[vectorIndex].length = B_PAGE_SIZE;

		fVecCount++;
		fPageCount++;
		return true;
	}

	return false;
}


status_t
PageWriteTransfer::Schedule(uint32 flags)
{
	off_t writeOffset = (off_t)fOffset << PAGE_SHIFT;
	generic_size_t writeLength = (phys_size_t)fPageCount << PAGE_SHIFT;

	if (fRun != NULL) {
		return fCache->WriteAsync(writeOffset, fVecs, fVecCount, writeLength,
			flags | B_PHYSICAL_IO_REQUEST, this);
	}

	status_t status = fCache->Write(writeOffset, fVecs, fVecCount,
		flags | B_PHYSICAL_IO_REQUEST, &writeLength);

	SetStatus(status, writeLength);
	return fStatus;
}


void
PageWriteTransfer::SetStatus(status_t status, size_t transferred)
{
	// only succeed if all pages up to the last one have been written fully
	// and the last page has at least been written partially
	if (status == B_OK && transferred <= (fPageCount - 1) * B_PAGE_SIZE)
		status = B_ERROR;

	fStatus = status;
}


void
PageWriteTransfer::IOFinished(status_t status, bool partialTransfer,
	generic_size_t bytesTransferred)
{
	SetStatus(status, bytesTransferred);
	fRun->PageWritten(this, fStatus, partialTransfer, bytesTransferred);
}


status_t
PageWriterRun::Init(uint32 maxPages)
{
	fMaxPages = maxPages;
	fWrapperCount = 0;
	fTransferCount = 0;
	fPendingTransfers = 0;

	fWrappers = new(std::nothrow) PageWriteWrapper[maxPages];
	fTransfers = new(std::nothrow) PageWriteTransfer[maxPages];
	if (fWrappers == NULL || fTransfers == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


void
PageWriterRun::PrepareNextRun()
{
	fWrapperCount = 0;
	fTransferCount = 0;
	fPendingTransfers = 0;
}


/*!	The page's cache must be locked.
*/
void
PageWriterRun::AddPage(vm_page* page)
{
	fWrappers[fWrapperCount++].SetTo(page);

	if (fTransferCount == 0 || !fTransfers[fTransferCount - 1].AddPage(page)) {
		fTransfers[fTransferCount++].SetTo(this, page,
			page->Cache()->MaxPagesPerAsyncWrite());
	}
}


/*!	Writes all pages previously added.
	\return The number of pages that could not be written or otherwise handled.
*/
uint32
PageWriterRun::Go()
{
	atomic_set(&fPendingTransfers, fTransferCount);

	fAllFinishedCondition.Init(this, "page writer wait for I/O");
	ConditionVariableEntry waitEntry;
	fAllFinishedCondition.Add(&waitEntry);

	// schedule writes
	for (uint32 i = 0; i < fTransferCount; i++)
		fTransfers[i].Schedule(B_VIP_IO_REQUEST);

	// wait until all pages have been written
	waitEntry.Wait();

	// mark pages depending on whether they could be written or not

	uint32 failedPages = 0;
	uint32 wrapperIndex = 0;
	for (uint32 i = 0; i < fTransferCount; i++) {
		PageWriteTransfer& transfer = fTransfers[i];
		transfer.Cache()->Lock();

		for (uint32 j = 0; j < transfer.PageCount(); j++) {
			if (!fWrappers[wrapperIndex++].Done(transfer.Status()))
				failedPages++;
		}

		transfer.Cache()->Unlock();
	}

	ASSERT(wrapperIndex == fWrapperCount);

	for (uint32 i = 0; i < fTransferCount; i++) {
		PageWriteTransfer& transfer = fTransfers[i];
		struct VMCache* cache = transfer.Cache();

		// We've acquired a references for each page
		for (uint32 j = 0; j < transfer.PageCount(); j++) {
			// We release the cache references after all pages were made
			// unbusy again - otherwise releasing a vnode could deadlock.
			cache->ReleaseStoreRef();
			cache->ReleaseRef();
		}
	}

	return failedPages;
}


void
PageWriterRun::PageWritten(PageWriteTransfer* transfer, status_t status,
	bool partialTransfer, size_t bytesTransferred)
{
	if (atomic_add(&fPendingTransfers, -1) == 1)
		fAllFinishedCondition.NotifyAll();
}


static vm_page*
next_modified_page(VMPageQueue& queue, page_num_t& maxPagesToSee)
{
	InterruptsSpinLocker locker(queue.GetLock());

	while (maxPagesToSee > 0) {
		vm_page* page = queue.Head();
		if (page == NULL)
			return NULL;

		queue.Requeue(page, true);

		maxPagesToSee--;

		if (!page->busy)
			return page;
	}

	return NULL;
}


/*!	The page writer continuously takes some pages from the modified
	queue, writes them back, and moves them back to the active queue.
	It runs in its own thread, and is only there to keep the number
	of modified pages low, so that more pages can be reused with
	fewer costs.
*/
status_t
ModifiedPageQueue::_PageWriter()
{
	ModifiedPageQueue& queue = *this;

	const uint32 kNumPages = 256;
#ifdef TRACE_VM_PAGE_WRITER
	uint32 writtenPages = 0;
	bigtime_t lastWrittenTime = 0;
	bigtime_t pageCollectionTime = 0;
	bigtime_t pageWritingTime = 0;
#endif

	PageWriterRun run;
	if (run.Init(kNumPages) != B_OK) {
		panic("page writer: Failed to init PageWriterRun!");
		return B_ERROR;
	}

	page_num_t pagesSinceLastSuccessfulWrite = 0;

	while (true) {
// TODO: Maybe wait shorter when memory is low!
		if (queue.Count() < kNumPages) {
			fPageWriterCondition.Wait(3000000, true);
				// all 3 seconds when no one triggers us
		}

		page_num_t modifiedPages = queue.Count();
		if (modifiedPages == 0)
			continue;

		if (modifiedPages <= pagesSinceLastSuccessfulWrite) {
			// We ran through the whole queue without being able to write a
			// single page. Take a break.
			snooze(500000);
			pagesSinceLastSuccessfulWrite = 0;
		}

#if ENABLE_SWAP_SUPPORT
		const bool activePaging = vm_page_should_do_active_paging();
#endif

		// depending on how urgent it becomes to get pages to disk, we adjust
		// our I/O priority
		uint32 lowPagesState = low_resource_state(B_KERNEL_RESOURCE_PAGES);
		int32 ioPriority = B_IDLE_PRIORITY;
		if (lowPagesState >= B_LOW_RESOURCE_CRITICAL
			|| modifiedPages > MAX_PAGE_WRITER_IO_PRIORITY_THRESHOLD) {
			ioPriority = MAX_PAGE_WRITER_IO_PRIORITY;
		} else {
			ioPriority = (uint64)MAX_PAGE_WRITER_IO_PRIORITY * modifiedPages
				/ MAX_PAGE_WRITER_IO_PRIORITY_THRESHOLD;
		}

		thread_set_io_priority(ioPriority);

		uint32 numPages = 0;
		run.PrepareNextRun();

		// TODO: make this laptop friendly, too (ie. only start doing
		// something if someone else did something or there is really
		// enough to do).

		// collect pages to be written
#ifdef TRACE_VM_PAGE_WRITER
		pageCollectionTime -= system_time();
#endif

		page_num_t maxPagesToSee = modifiedPages;

		while (numPages < kNumPages && maxPagesToSee > 0) {
			vm_page *page = next_modified_page(queue, maxPagesToSee);
			if (page == NULL)
				break;

			PageCacheLocker cacheLocker(page, false);
			if (!cacheLocker.IsLocked())
				continue;

			VMCache *cache = page->Cache();

			// If the page is busy or its state has changed while we were
			// locking the cache, just ignore it.
			if (page->busy || page->State() != PAGE_STATE_MODIFIED)
				continue;

			DEBUG_PAGE_ACCESS_START(page);

			// Don't write back wired (locked) pages.
			if (page->WiredCount() > 0) {
				vm_page_set_state(page, PAGE_STATE_ACTIVE);
				DEBUG_PAGE_ACCESS_END(page);
				continue;
			}

			// Write back temporary pages only when we're actively paging.
			if (cache->temporary
#if ENABLE_SWAP_SUPPORT
				&& (!activePaging
					|| !cache->CanWritePage(
							(off_t)page->cache_offset << PAGE_SHIFT))
#endif
				) {
				// We can't/don't want to do anything with this page, so move it
				// to one of the other queues.
				if (page->mappings.IsEmpty())
					vm_page_set_state(page, PAGE_STATE_INACTIVE);
				else
					vm_page_set_state(page, PAGE_STATE_ACTIVE);

				DEBUG_PAGE_ACCESS_END(page);
				continue;
			}

			// We need our own reference to the store, as it might currently be
			// destroyed.
			if (cache->AcquireUnreferencedStoreRef() != B_OK) {
				DEBUG_PAGE_ACCESS_END(page);
				cacheLocker.Unlock();
				thread_yield();
				continue;
			}

			run.AddPage(page);
				// TODO: We're possibly adding pages of different caches and
				// thus maybe of different underlying file systems here. This
				// is a potential problem for loop file systems/devices, since
				// we could mark a page busy that would need to be accessed
				// when writing back another page, thus causing a deadlock.

			DEBUG_PAGE_ACCESS_END(page);

			//dprintf("write page %p, cache %p (%ld)\n", page, page->cache, page->cache->ref_count);
			TPW(WritePage(page));

			cache->AcquireRefLocked();
			numPages++;

			// Write adjacent pages at the same time, if they're also modified.
			if (cache->temporary)
				continue;
			while (page->cache_next != NULL && numPages < kNumPages) {
				page = page->cache_next;
				if (page->busy || page->State() != PAGE_STATE_MODIFIED)
					break;
				if (page->WiredCount() > 0)
					break;

				DEBUG_PAGE_ACCESS_START(page);
				queue.RequeueUnlocked(page, true);
				run.AddPage(page);
				DEBUG_PAGE_ACCESS_END(page);

				cache->AcquireStoreRef();
				cache->AcquireRefLocked();
				numPages++;
				if (maxPagesToSee > 0)
					maxPagesToSee--;
			}
		}

#ifdef TRACE_VM_PAGE_WRITER
		pageCollectionTime += system_time();
#endif
		if (numPages == 0)
			continue;

		// write pages to disk and do all the cleanup
#ifdef TRACE_VM_PAGE_WRITER
		pageWritingTime -= system_time();
#endif
		uint32 failedPages = run.Go();
#ifdef TRACE_VM_PAGE_WRITER
		pageWritingTime += system_time();

		// debug output only...
		writtenPages += numPages;
		if (writtenPages >= 1024) {
			bigtime_t now = system_time();
			TRACE(("page writer: wrote 1024 pages (total: %" B_PRIu64 " ms, "
				"collect: %" B_PRIu64 " ms, write: %" B_PRIu64 " ms)\n",
				(now - lastWrittenTime) / 1000,
				pageCollectionTime / 1000, pageWritingTime / 1000));
			lastWrittenTime = now;

			writtenPages -= 1024;
			pageCollectionTime = 0;
			pageWritingTime = 0;
		}
#endif

		if (failedPages == numPages)
			pagesSinceLastSuccessfulWrite += modifiedPages - maxPagesToSee;
		else
			pagesSinceLastSuccessfulWrite = 0;
	}

	return B_OK;
}


/*static*/ status_t
ModifiedPageQueue::_WriterThreadEntry(void* _this)
{
	return ((ModifiedPageQueue*)_this)->_PageWriter();
}



//	#pragma mark - private kernel API


/*!	Writes a range of modified pages of a cache to disk.
	You need to hold the VMCache lock when calling this function.
	Note that the cache lock is released in this function.
	\param cache The cache.
	\param firstPage Offset (in page size units) of the first page in the range.
	\param endPage End offset (in page size units) of the page range. The page
		at this offset is not included.
*/
status_t
vm_page_write_modified_page_range(struct VMCache* cache, uint32 firstPage,
	uint32 endPage)
{
	static const int32 kMaxPages = 256;
	int32 maxPages = cache->MaxPagesPerWrite();
	if (maxPages < 0 || maxPages > kMaxPages)
		maxPages = kMaxPages;

	const uint32 allocationFlags = HEAP_DONT_WAIT_FOR_MEMORY
		| HEAP_DONT_LOCK_KERNEL_SPACE;

	PageWriteWrapper stackWrappersPool[2];
	PageWriteWrapper* stackWrappers[1];
	PageWriteWrapper* wrapperPool
		= new(malloc_flags(allocationFlags)) PageWriteWrapper[maxPages + 1];
	PageWriteWrapper** wrappers
		= new(malloc_flags(allocationFlags)) PageWriteWrapper*[maxPages];
	if (wrapperPool == NULL || wrappers == NULL) {
		// don't fail, just limit our capabilities
		delete[] wrapperPool;
		delete[] wrappers;
		wrapperPool = stackWrappersPool;
		wrappers = stackWrappers;
		maxPages = 1;
	}

	int32 nextWrapper = 0;
	int32 usedWrappers = 0;

	PageWriteTransfer transfer;
	bool transferEmpty = true;

	VMCachePagesTree::Iterator it
		= cache->pages.GetIterator(firstPage, true, true);

	while (true) {
		vm_page* page = it.Next();
		if (page == NULL || page->cache_offset >= endPage) {
			if (transferEmpty)
				break;

			page = NULL;
		}

		if (page != NULL) {
			if (page->busy
				|| (page->State() != PAGE_STATE_MODIFIED
					&& !vm_test_map_modification(page))) {
				page = NULL;
			}
		}

		PageWriteWrapper* wrapper = NULL;
		if (page != NULL) {
			wrapper = &wrapperPool[nextWrapper++];
			if (nextWrapper > maxPages)
				nextWrapper = 0;

			DEBUG_PAGE_ACCESS_START(page);

			wrapper->SetTo(page);

			if (transferEmpty || transfer.AddPage(page)) {
				if (transferEmpty) {
					transfer.SetTo(NULL, page, maxPages);
					transferEmpty = false;
				}

				DEBUG_PAGE_ACCESS_END(page);

				wrappers[usedWrappers++] = wrapper;
				continue;
			}

			DEBUG_PAGE_ACCESS_END(page);
		}

		if (transferEmpty)
			continue;

		cache->Unlock();
		status_t status = transfer.Schedule(0);
		cache->Lock();

		for (int32 i = 0; i < usedWrappers; i++)
			wrappers[i]->Done(status);

		usedWrappers = 0;

		if (page != NULL) {
			transfer.SetTo(NULL, page, maxPages);
			wrappers[usedWrappers++] = wrapper;
		} else
			transferEmpty = true;
	}

	if (wrapperPool != stackWrappersPool) {
		delete[] wrapperPool;
		delete[] wrappers;
	}

	return B_OK;
}


/*!	You need to hold the VMCache lock when calling this function.
	Note that the cache lock is released in this function.
*/
status_t
vm_page_write_modified_pages(VMCache *cache)
{
	return vm_page_write_modified_page_range(cache, 0,
		(cache->virtual_end + B_PAGE_SIZE - 1) >> PAGE_SHIFT);
}


/*!	Schedules the page writer to write back the specified \a page.
	Note, however, that it might not do this immediately, and it can well
	take several seconds until the page is actually written out.
*/
void
vm_page_schedule_write_page(vm_page *page)
{
	ASSERT(page->State() == PAGE_STATE_MODIFIED);

	VMPageQueue* queue = NULL;
	vm_page_requeue(page, false, &queue);

	((ModifiedPageQueue*)queue)->NotifyWriter();
}


/*!	Cache must be locked.
*/
void
vm_page_schedule_write_page_range(struct VMCache *cache, uint32 firstPage,
	uint32 endPage)
{
	VMPageQueue* queue = NULL;

	uint32 modified = 0;
	for (VMCachePagesTree::Iterator it
				= cache->pages.GetIterator(firstPage, true, true);
			vm_page *page = it.Next();) {
		if (page->cache_offset >= endPage)
			break;

		if (!page->busy && page->State() == PAGE_STATE_MODIFIED) {
			DEBUG_PAGE_ACCESS_START(page);
			vm_page_requeue(page, false, &queue);
			modified++;
			DEBUG_PAGE_ACCESS_END(page);
		}
	}

	if (modified > 0)
		((ModifiedPageQueue*)queue)->NotifyWriter();
}


status_t
ModifiedPageQueue::StartWriter()
{
	fPageWriterCondition.Init("page writer");

	fWriterThread = spawn_kernel_thread(&_WriterThreadEntry, "page writer",
		B_NORMAL_PRIORITY + 1, this);
	if (fWriterThread < 0)
		return fWriterThread;

	return resume_thread(fWriterThread);
}
