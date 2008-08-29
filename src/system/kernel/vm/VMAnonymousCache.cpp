/*
 * Copyright 2008, Zhao Shuai, upczhsh@163.com.
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

#include "VMAnonymousCache.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <KernelExport.h>
#include <NodeMonitor.h>

#include <arch_config.h>
#include <driver_settings.h>
#include <fs_interface.h>
#include <heap.h>
#include <kernel_daemon.h>
#include <slab/Slab.h>
#include <syscalls.h>
#include <tracing.h>
#include <util/AutoLock.h>
#include <util/DoublyLinkedList.h>
#include <util/OpenHashTable.h>
#include <vfs.h>
#include <vm.h>
#include <vm_page.h>
#include <vm_priv.h>

#include "io_requests.h"


#if	ENABLE_SWAP_SUPPORT

//#define TRACE_STORE
#ifdef TRACE_STORE
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


// number of free swap blocks the object cache shall minimally have
#define MIN_SWAP_BLOCK_RESERVE	4096

// interval the has resizer is triggered (in 0.1s)
#define SWAP_HASH_RESIZE_INTERVAL	5

#define INITIAL_SWAP_HASH_SIZE		1024

#define SWAP_BLOCK_PAGES 32
#define SWAP_BLOCK_SHIFT 5		/* 1 << SWAP_BLOCK_SHIFT == SWAP_BLOCK_PAGES */
#define SWAP_BLOCK_MASK  (SWAP_BLOCK_PAGES - 1)

#define SWAP_SLOT_NONE (~(swap_addr_t)0)

// bitmap allocation macros
#define NUM_BYTES_PER_WORD 4
#define NUM_BITS_PER_WORD  (8 * NUM_BYTES_PER_WORD)
#define MAP_SHIFT  5     // 1 << MAP_SHIFT == NUM_BITS_PER_WORD

#define TESTBIT(map, i) \
	(((map)[(i) >> MAP_SHIFT] & (1 << (i) % NUM_BITS_PER_WORD)))
#define SETBIT(map, i) \
	(((map)[(i) >> MAP_SHIFT] |= (1 << (i) % NUM_BITS_PER_WORD)))
#define CLEARBIT(map, i) \
	(((map)[(i) >> MAP_SHIFT] &= ~(1 << (i) % NUM_BITS_PER_WORD)))

// The stack functionality looks like a good candidate to put into its own
// store. I have not done this because once we have a swap file backing up
// the memory, it would probably not be a good idea to separate this
// anymore.

struct swap_file : DoublyLinkedListLinkImpl<swap_file> {
	struct vnode	*vnode;
	swap_addr_t		first_slot;
	swap_addr_t		last_slot;
	swap_addr_t		used;   // # of slots used
	uint32			*maps;  // bitmap for the slots
	swap_addr_t		hint;   // next free slot
};

struct swap_hash_key {
	VMAnonymousCache	*cache;
	off_t				page_index;  // page index in the cache
};

// Each swap block contains SWAP_BLOCK_PAGES pages
struct swap_block : HashTableLink<swap_block> {
	swap_hash_key	key;
	uint32			used;
	swap_addr_t		swap_slots[SWAP_BLOCK_PAGES];
};

struct SwapHashTableDefinition {
	typedef swap_hash_key KeyType;
	typedef swap_block ValueType;

	SwapHashTableDefinition() {}

	size_t HashKey(const swap_hash_key& key) const
	{
		off_t blockIndex = key.page_index >> SWAP_BLOCK_SHIFT;
		VMAnonymousCache *cache = key.cache;
		return blockIndex ^ (int)(int *)cache;
	}

	size_t Hash(const swap_block *value) const
	{
		return HashKey(value->key);
	}

	bool Compare(const swap_hash_key& key, const swap_block *value) const
	{
		return (key.page_index & ~(off_t)SWAP_BLOCK_MASK)
				== (value->key.page_index & ~(off_t)SWAP_BLOCK_MASK)
			&& key.cache == value->key.cache;
	}

	HashTableLink<swap_block> *GetLink(swap_block *value) const
	{
		return value;
	}
};

typedef OpenHashTable<SwapHashTableDefinition> SwapHashTable;
typedef DoublyLinkedList<swap_file> SwapFileList;

static SwapHashTable sSwapHashTable;
static mutex sSwapHashLock;

static SwapFileList sSwapFileList;
static mutex sSwapFileListLock;
static swap_file *sSwapFileAlloc = NULL; // allocate from here
static uint32 sSwapFileCount = 0;

static off_t sAvailSwapSpace = 0;
static mutex sAvailSwapSpaceLock;

static object_cache *sSwapBlockCache;


#if SWAP_TRACING
namespace SwapTracing {

class SwapTraceEntry : public AbstractTraceEntry {
public:
	SwapTraceEntry(VMAnonymousCache* cache)
		:
		fCache(cache)
	{
	}

protected:
	VMAnonymousCache*	fCache;
};


class ReadPage : public SwapTraceEntry {
public:
	ReadPage(VMAnonymousCache* cache, page_num_t pageIndex,
			swap_addr_t swapSlotIndex)
		:
		SwapTraceEntry(cache),
		fPageIndex(pageIndex),
		fSwapSlotIndex(swapSlotIndex)
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("swap read:  cache %p, page index: %lu <- swap slot: %lu",
			fCache, fPageIndex, fSwapSlotIndex);
	}

private:
	page_num_t		fPageIndex;
	swap_addr_t		fSwapSlotIndex;
};


class WritePage : public SwapTraceEntry {
public:
	WritePage(VMAnonymousCache* cache, page_num_t pageIndex,
			swap_addr_t swapSlotIndex)
		:
		SwapTraceEntry(cache),
		fPageIndex(pageIndex),
		fSwapSlotIndex(swapSlotIndex)
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("swap write: cache %p, page index: %lu -> swap slot: %lu",
			fCache, fPageIndex, fSwapSlotIndex);
	}

private:
	page_num_t		fPageIndex;
	swap_addr_t		fSwapSlotIndex;
};

}	// namespace SwapTracing

#	define T(x) new(std::nothrow) SwapTracing::x;
#else
#	define T(x) ;
#endif



static int
dump_swap_info(int argc, char** argv)
{
	swap_addr_t totalSwapPages = 0;
	swap_addr_t usedSwapPages = 0;

	kprintf("swap files:\n");

	for (SwapFileList::Iterator it = sSwapFileList.GetIterator();
			swap_file* file = it.Next();) {
		swap_addr_t total = file->last_slot - file->first_slot;
		kprintf("  vnode: %p, pages: total: %lu, used: %lu\n",
			file->vnode, total, file->used);

		totalSwapPages += total;
		usedSwapPages += file->used;
	}

	kprintf("\n");
	kprintf("swap space in pages:\n");
	kprintf("total:     %9lu\n", totalSwapPages);
	kprintf("available: %9llu\n", sAvailSwapSpace / B_PAGE_SIZE);
	kprintf("reserved:  %9llu\n",
		totalSwapPages - sAvailSwapSpace / B_PAGE_SIZE);
	kprintf("used:      %9lu\n", usedSwapPages);
	kprintf("free:      %9lu\n", totalSwapPages - usedSwapPages);

	return 0;
}


static swap_addr_t
swap_slot_alloc(uint32 count)
{
	mutex_lock(&sSwapFileListLock);

	if (sSwapFileList.IsEmpty()) {
		mutex_unlock(&sSwapFileListLock);
		panic("swap_slot_alloc(): no swap file in the system\n");
		return SWAP_SLOT_NONE;
	}

	// compute how many pages are free in all swap files
	uint32 freeSwapPages = 0;
	for (SwapFileList::Iterator it = sSwapFileList.GetIterator();
			swap_file *file = it.Next();)
		freeSwapPages += file->last_slot - file->first_slot - file->used;

	if (freeSwapPages < count) {
		mutex_unlock(&sSwapFileListLock);
		panic("swap_slot_alloc(): swap space exhausted!\n");
		return SWAP_SLOT_NONE;
	}

	swap_addr_t hint = 0;
	swap_addr_t j;
	for (j = 0; j < sSwapFileCount; j++) {
		if (sSwapFileAlloc == NULL)
			sSwapFileAlloc = sSwapFileList.First();

		hint = sSwapFileAlloc->hint;
		swap_addr_t pageCount = sSwapFileAlloc->last_slot
			- sSwapFileAlloc->first_slot;

		swap_addr_t i = 0;
		while (i < count && (hint + count) <= pageCount) {
			if (TESTBIT(sSwapFileAlloc->maps, hint + i)) {
				hint++;
				i = 0;
			} else
				i++;
		}

		if (i == count)
			break;

		// this swap_file is full, find another
		sSwapFileAlloc = sSwapFileList.GetNext(sSwapFileAlloc);
	}

	// no swap file can alloc so many pages, we return SWAP_SLOT_NONE
	// and VMAnonymousCache::Write() will adjust allocation amount
	if (j == sSwapFileCount) {
		mutex_unlock(&sSwapFileListLock);
		return SWAP_SLOT_NONE;
	}

	swap_addr_t slotIndex = sSwapFileAlloc->first_slot + hint;

	for (uint32 i = 0; i < count; i++)
		SETBIT(sSwapFileAlloc->maps, hint + i);
	if (hint == sSwapFileAlloc->hint) {
		sSwapFileAlloc->hint += count;
		swap_addr_t pageCount = sSwapFileAlloc->last_slot
			- sSwapFileAlloc->first_slot;
		while (TESTBIT(sSwapFileAlloc->maps, sSwapFileAlloc->hint)
			&& sSwapFileAlloc->hint < pageCount) {
			sSwapFileAlloc->hint++;
		}
	}

	sSwapFileAlloc->used += count;

	// if this swap file has used more than 90% percent of its slots
	// switch to another
    if (sSwapFileAlloc->used
			> 9 * (sSwapFileAlloc->last_slot - sSwapFileAlloc->first_slot) / 10)
		sSwapFileAlloc = sSwapFileList.GetNext(sSwapFileAlloc);

	mutex_unlock(&sSwapFileListLock);

	return slotIndex;
}


static swap_file *
find_swap_file(swap_addr_t slotIndex)
{
	for (SwapFileList::Iterator it = sSwapFileList.GetIterator();
			swap_file *swapFile = it.Next();) {
		if (slotIndex >= swapFile->first_slot
				&& slotIndex < swapFile->last_slot)
			return swapFile;
	}

	panic("find_swap_file(): can't find swap file for slot %ld\n", slotIndex);
	return NULL;
}


static void
swap_slot_dealloc(swap_addr_t slotIndex, uint32 count)
{
	if (slotIndex == SWAP_SLOT_NONE)
		return;

	mutex_lock(&sSwapFileListLock);
	swap_file *swapFile = find_swap_file(slotIndex);

	slotIndex -= swapFile->first_slot;

	for (uint32 i = 0; i < count; i++)
		CLEARBIT(swapFile->maps, slotIndex + i);

	if (swapFile->hint > slotIndex)
		swapFile->hint = slotIndex;

	swapFile->used -= count;
	mutex_unlock(&sSwapFileListLock);
}


static off_t
swap_space_reserve(off_t amount)
{
	mutex_lock(&sAvailSwapSpaceLock);
	if (sAvailSwapSpace >= amount)
		sAvailSwapSpace -= amount;
	else {
		amount = sAvailSwapSpace;
		sAvailSwapSpace = 0;
	}
	mutex_unlock(&sAvailSwapSpaceLock);

	return amount;
}


static void
swap_space_unreserve(off_t amount)
{
	mutex_lock(&sAvailSwapSpaceLock);
	sAvailSwapSpace += amount;
	mutex_unlock(&sAvailSwapSpaceLock);
}


static void
swap_hash_resizer(void*, int)
{
	MutexLocker locker(sSwapHashLock);

	size_t size;
	void* allocation;

	do {
		size = sSwapHashTable.ResizeNeeded();
		if (size == 0)
			return;

		locker.Unlock();

		allocation = malloc(size);
		if (allocation == NULL)
			return;

		locker.Lock();

	} while (!sSwapHashTable.Resize(allocation, size));
}


// #pragma mark -


class VMAnonymousCache::WriteCallback : public StackableAsyncIOCallback {
public:
	WriteCallback(VMAnonymousCache* cache, AsyncIOCallback* callback)
		:
		StackableAsyncIOCallback(callback),
		fCache(cache)
	{
	}

	void SetTo(page_num_t pageIndex, swap_addr_t slotIndex, bool newSlot)
	{
		fPageIndex = pageIndex;
		fSlotIndex = slotIndex;
		fNewSlot = newSlot;
	}

	virtual void IOFinished(status_t status, bool partialTransfer,
		size_t bytesTransferred)
	{
		if (fNewSlot) {
			if (status == B_OK) {
				fCache->_SwapBlockBuild(fPageIndex, fSlotIndex, 1);
			} else {
				AutoLocker<VMCache> locker(fCache);
				fCache->fAllocatedSwapSize -= B_PAGE_SIZE;
				locker.Unlock();

				swap_slot_dealloc(fSlotIndex, 1);
			}
		}

		fNextCallback->IOFinished(status, partialTransfer, bytesTransferred);

		delete this;
	}

	void operator delete(void* address, size_t size)
	{
		io_request_free(address);
	}

private:
	VMAnonymousCache*	fCache;
	page_num_t			fPageIndex;
	swap_addr_t			fSlotIndex;
	bool				fNewSlot;
};


// #pragma mark -


VMAnonymousCache::~VMAnonymousCache()
{
	// free allocated swap space and swap block
	for (off_t offset = virtual_base, toFree = fAllocatedSwapSize;
			offset < virtual_end && toFree > 0; offset += B_PAGE_SIZE) {
		swap_addr_t slotIndex = _SwapBlockGetAddress(offset >> PAGE_SHIFT);
		if (slotIndex == SWAP_SLOT_NONE)
			continue;

		swap_slot_dealloc(slotIndex, 1);
		_SwapBlockFree(offset >> PAGE_SHIFT, 1);
		toFree -= B_PAGE_SIZE;
	}

	swap_space_unreserve(fCommittedSwapSize);
	if (committed_size > fCommittedSwapSize)
		vm_unreserve_memory(committed_size - fCommittedSwapSize);
}


status_t
VMAnonymousCache::Init(bool canOvercommit, int32 numPrecommittedPages,
	int32 numGuardPages)
{
	TRACE(("VMAnonymousCache::Init(canOvercommit = %s, numGuardPages = %ld) "
		"at %p\n", canOvercommit ? "yes" : "no", numGuardPages, store));

	status_t error = VMCache::Init(CACHE_TYPE_RAM);
	if (error != B_OK)
		return error;

	fCanOvercommit = canOvercommit;
	fHasPrecommitted = false;
	fPrecommittedPages = min_c(numPrecommittedPages, 255);
	fGuardedSize = numGuardPages * B_PAGE_SIZE;
	fCommittedSwapSize = 0;
	fAllocatedSwapSize = 0;

	return B_OK;
}


status_t
VMAnonymousCache::Commit(off_t size)
{
	// if we can overcommit, we don't commit here, but in anonymous_fault()
	if (fCanOvercommit) {
		if (fHasPrecommitted)
			return B_OK;

		// pre-commit some pages to make a later failure less probable
		fHasPrecommitted = true;
		uint32 precommitted = fPrecommittedPages * B_PAGE_SIZE;
		if (size > precommitted)
			size = precommitted;
	}

	return _Commit(size);
}


bool
VMAnonymousCache::HasPage(off_t offset)
{
	if (_SwapBlockGetAddress(offset >> PAGE_SHIFT) != SWAP_SLOT_NONE)
		return true;

	return false;
}


status_t
VMAnonymousCache::Read(off_t offset, const iovec *vecs, size_t count,
	size_t *_numBytes)
{
	off_t pageIndex = offset >> PAGE_SHIFT;

	for (uint32 i = 0, j = 0; i < count; i = j) {
		swap_addr_t startSlotIndex = _SwapBlockGetAddress(pageIndex + i);
		for (j = i + 1; j < count; j++) {
			swap_addr_t slotIndex = _SwapBlockGetAddress(pageIndex + j);
			if (slotIndex != startSlotIndex + j - i)
				break;
		}

		T(ReadPage(this, pageIndex, startSlotIndex));
			// TODO: Assumes that only one page is read.

		swap_file *swapFile = find_swap_file(startSlotIndex);

		off_t pos = (startSlotIndex - swapFile->first_slot) * B_PAGE_SIZE;

		status_t status = vfs_read_pages(swapFile->vnode, NULL, pos, vecs + i,
				j - i, 0, _numBytes);
		if (status != B_OK)
			return status;
	}

	return B_OK;
}


status_t
VMAnonymousCache::Write(off_t offset, const iovec *vecs, size_t count,
	uint32 flags, size_t *_numBytes)
{
	off_t pageIndex = offset >> PAGE_SHIFT;

	AutoLocker<VMCache> locker(this);

	for (uint32 i = 0; i < count; i++) {
		swap_addr_t slotIndex = _SwapBlockGetAddress(pageIndex + i);
		if (slotIndex != SWAP_SLOT_NONE) {
			swap_slot_dealloc(slotIndex, 1);
			_SwapBlockFree(pageIndex + i, 1);
			fAllocatedSwapSize -= B_PAGE_SIZE;
		}
	}

	if (fAllocatedSwapSize + count * B_PAGE_SIZE > fCommittedSwapSize)
		return B_ERROR;

	fAllocatedSwapSize += count * B_PAGE_SIZE;
	locker.Unlock();

	uint32 n = count;
	for (uint32 i = 0; i < count; i += n) {
		swap_addr_t slotIndex;
		// try to allocate n slots, if fail, try to allocate n/2
		while ((slotIndex = swap_slot_alloc(n)) == SWAP_SLOT_NONE && n >= 2)
			n >>= 1;
		if (slotIndex == SWAP_SLOT_NONE)
			panic("VMAnonymousCache::Write(): can't allocate swap space\n");

		T(WritePage(this, pageIndex, slotIndex));
			// TODO: Assumes that only one page is written.

		swap_file *swapFile = find_swap_file(slotIndex);

		off_t pos = (slotIndex - swapFile->first_slot) * B_PAGE_SIZE;

		status_t status = vfs_write_pages(swapFile->vnode, NULL, pos, vecs + i,
				n, flags, _numBytes);
		if (status != B_OK) {
			locker.Lock();
			fAllocatedSwapSize -= n * B_PAGE_SIZE;
			locker.Unlock();

			swap_slot_dealloc(slotIndex, n);
			return status;
		}

		_SwapBlockBuild(pageIndex + i, slotIndex, n);
	}

	return B_OK;
}


status_t
VMAnonymousCache::WriteAsync(off_t offset, const iovec* vecs, size_t count,
	size_t numBytes, uint32 flags, AsyncIOCallback* _callback)
{
	// TODO: Currently this method is only used for single pages. Either make
	// more flexible use of it or change the interface!
	// This implementation relies on the current usage!
	ASSERT(count == 1);
	ASSERT(numBytes <= B_PAGE_SIZE);

	page_num_t pageIndex = offset >> PAGE_SHIFT;
	swap_addr_t slotIndex = _SwapBlockGetAddress(pageIndex);
	bool newSlot = slotIndex == SWAP_SLOT_NONE;

	// If the page doesn't have any swap space yet, allocate it.
	if (newSlot) {
		AutoLocker<VMCache> locker(this);
		if (fAllocatedSwapSize + B_PAGE_SIZE > fCommittedSwapSize) {
			_callback->IOFinished(B_ERROR, true, 0);
			return B_ERROR;
		}

		fAllocatedSwapSize += B_PAGE_SIZE;

		slotIndex = swap_slot_alloc(1);
	}

	// create our callback
	WriteCallback* callback = (flags & B_VIP_IO_REQUEST != 0)
 		? new(vip_io_alloc) WriteCallback(this, _callback)
		: new(std::nothrow) WriteCallback(this, _callback);
	if (callback == NULL) {
		if (newSlot) {
			AutoLocker<VMCache> locker(this);
			fAllocatedSwapSize -= B_PAGE_SIZE;
			locker.Unlock();

			swap_slot_dealloc(slotIndex, 1);
		}
		_callback->IOFinished(B_NO_MEMORY, true, 0);
		return B_NO_MEMORY;
	}
// TODO: If the page already had swap space assigned, we don't need an own
// callback.

	callback->SetTo(pageIndex, slotIndex, newSlot);

	T(WritePage(this, pageIndex, slotIndex));

	// write the page asynchrounously
	swap_file* swapFile = find_swap_file(slotIndex);
	off_t pos = (slotIndex - swapFile->first_slot) * B_PAGE_SIZE;

	return vfs_asynchronous_write_pages(swapFile->vnode, NULL, pos, vecs, 1,
		numBytes, flags, callback);
}


status_t
VMAnonymousCache::Fault(struct vm_address_space *aspace, off_t offset)
{
	if (fCanOvercommit && LookupPage(offset) == NULL && !HasPage(offset)) {
		if (fGuardedSize > 0) {
			uint32 guardOffset;

#ifdef STACK_GROWS_DOWNWARDS
			guardOffset = 0;
#elif defined(STACK_GROWS_UPWARDS)
			guardOffset = virtual_size - fGuardedSize;
#else
#	error Stack direction has not been defined in arch_config.h
#endif

			// report stack fault, guard page hit!
			if (offset >= guardOffset && offset < guardOffset + fGuardedSize) {
				TRACE(("stack overflow!\n"));
				return B_BAD_ADDRESS;
			}
		}

		if (fPrecommittedPages == 0) {
			// try to commit additional swap space/memory
			if (swap_space_reserve(B_PAGE_SIZE) == B_PAGE_SIZE)
				fCommittedSwapSize += B_PAGE_SIZE;
			else if (vm_try_reserve_memory(B_PAGE_SIZE, 0) != B_OK)
				return B_NO_MEMORY;

			committed_size += B_PAGE_SIZE;
		} else
			fPrecommittedPages--;
	}

	// This will cause vm_soft_fault() to handle the fault
	return B_BAD_HANDLER;
}


void
VMAnonymousCache::Merge(VMCache* _source)
{
	VMAnonymousCache* source = dynamic_cast<VMAnonymousCache*>(_source);
	if (source == NULL) {
		panic("VMAnonymousCache::MergeStore(): merge with incompatible cache "
			"%p requested", _source);
		return;
	}

	// take over the source' committed size
	fCommittedSwapSize += source->fCommittedSwapSize;
	source->fCommittedSwapSize = 0;
	committed_size += source->committed_size;
	source->committed_size = 0;

	off_t actualSize = virtual_end - virtual_base;
	if (committed_size > actualSize)
		_Commit(actualSize);

	// Move all not shadowed pages from the source to the consumer cache.

	for (VMCachePagesTree::Iterator it = source->pages.GetIterator();
			vm_page* page = it.Next();) {
		// Note: Removing the current node while iterating through a
		// IteratableSplayTree is safe.
		vm_page* consumerPage = LookupPage(
			(off_t)page->cache_offset << PAGE_SHIFT);
		swap_addr_t consumerSwapSlot = _SwapBlockGetAddress(page->cache_offset);
		if (consumerPage != NULL && consumerPage->state == PAGE_STATE_BUSY
			&& consumerPage->type == PAGE_TYPE_DUMMY
			&& consumerSwapSlot == SWAP_SLOT_NONE) {
			// the page is currently busy taking a read fault - IOW,
			// vm_soft_fault() has mapped our page so we can just
			// move it up
			//dprintf("%ld: merged busy page %p, cache %p, offset %ld\n", find_thread(NULL), page, cacheRef->cache, page->cache_offset);
			RemovePage(consumerPage);
			consumerPage->state = PAGE_STATE_INACTIVE;
			((vm_dummy_page*)consumerPage)->busy_condition.Unpublish();
			consumerPage = NULL;
		}

		if (consumerPage == NULL && consumerSwapSlot == SWAP_SLOT_NONE) {
			// the page is not yet in the consumer cache - move it upwards
			source->RemovePage(page);
			InsertPage(page, (off_t)page->cache_offset << PAGE_SHIFT);

			// If the moved-up page has a swap page associated, we mark it, so
			// that the swap page is moved upwards, too. We would lose if the
			// page was modified and written to swap, and is now not marked
			// modified.
			if (source->_SwapBlockGetAddress(page->cache_offset)
					!= SWAP_SLOT_NONE) {
				page->merge_swap = true;
			}
#ifdef DEBUG_PAGE_CACHE_TRANSITIONS
		} else {
			page->debug_flags = 0;
			if (consumerPage->state == PAGE_STATE_BUSY)
				page->debug_flags |= 0x1;
			if (consumerPage->type == PAGE_TYPE_DUMMY)
				page->debug_flags |= 0x2;
			page->collided_page = consumerPage;
			consumerPage->collided_page = page;
#endif	// DEBUG_PAGE_CACHE_TRANSITIONS
		}
	}

	// Move all not shadowed swap pages from the source to the consumer cache.

	for (off_t offset = source->virtual_base
				& ~(off_t)(B_PAGE_SIZE * SWAP_BLOCK_PAGES - 1);
			offset < source->virtual_end;
			offset += B_PAGE_SIZE * SWAP_BLOCK_PAGES) {

		MutexLocker locker(sSwapHashLock);

		page_num_t swapBlockPageIndex = offset >> PAGE_SHIFT;
		swap_hash_key key = { source, swapBlockPageIndex };
		swap_block* sourceSwapBlock = sSwapHashTable.Lookup(key);

		if (sourceSwapBlock == NULL)
			continue;

		// remove the source swap block -- we will either take over the swap
		// space (and the block) or free it
		sSwapHashTable.RemoveUnchecked(sourceSwapBlock);

		key.cache = this;
		swap_block* swapBlock = sSwapHashTable.Lookup(key);

		locker.Unlock();

		for (uint32 i = 0; i < SWAP_BLOCK_PAGES; i++) {
			off_t pageIndex = swapBlockPageIndex + i;
			swap_addr_t sourceSlotIndex = sourceSwapBlock->swap_slots[i];

			if (sourceSlotIndex == SWAP_SLOT_NONE)
				// this page is not swapped out
				continue;

			vm_page* page = LookupPage(pageIndex << PAGE_SHIFT);

			bool keepSwapPage = true;
			if (page != NULL && !page->merge_swap) {
				// The consumer already has a page at this index and it wasn't
				// one taken over from the source. So we can simply free the
				// swap space.
				keepSwapPage = false;
			} else {
				if (page != NULL) {
					// The page was taken over from the source cache. Clear the
					// indicator flag. We'll take over the swap page too.
					page->merge_swap = false;
				} else if (swapBlock != NULL
						&& swapBlock->swap_slots[i] != SWAP_SLOT_NONE) {
					// There's no page in the consumer cache, but a swap page.
					// Free the source swap page.
					keepSwapPage = false;
				}
			}

			if (!keepSwapPage) {
				swap_slot_dealloc(sourceSlotIndex, 1);
				sourceSwapBlock->swap_slots[i] = SWAP_SLOT_NONE;
				sourceSwapBlock->used--;
			}

			// We've either freed the source swap page or are going to move it
			// to the consumer. At any rate, the source cache doesn't own it
			// anymore.
			source->fAllocatedSwapSize -= B_PAGE_SIZE;
		}

		// All source swap pages that have not been freed yet are taken over by
		// by the consumer.
		fAllocatedSwapSize += B_PAGE_SIZE * sourceSwapBlock->used;

		if (sourceSwapBlock->used == 0) {
			// All swap pages have been freed -- we can discard the source swap
			// block.
			object_cache_free(sSwapBlockCache, sourceSwapBlock);
		} else if (swapBlock == NULL) {
			// We need to take over some of the source's swap pages and there's
			// no swap block in the consumer cache. Just take over the source
			// swap block.
			sourceSwapBlock->key.cache = this;
			locker.Lock();
			sSwapHashTable.InsertUnchecked(sourceSwapBlock);
			locker.Unlock();
		} else {
			// We need to take over some of the source's swap pages and there's
			// already swap block in the consumer cache. Copy the respective
			// swap addresses and discard the source swap block.
			for (uint32 i = 0; i < SWAP_BLOCK_PAGES; i++) {
				if (sourceSwapBlock->swap_slots[i] != SWAP_SLOT_NONE)
					swapBlock->swap_slots[i] = sourceSwapBlock->swap_slots[i];
			}

			object_cache_free(sSwapBlockCache, sourceSwapBlock);
		}
	}
}


void
VMAnonymousCache::_SwapBlockBuild(off_t startPageIndex,
	swap_addr_t startSlotIndex, uint32 count)
{
	mutex_lock(&sSwapHashLock);

	uint32 left = count;
	for (uint32 i = 0, j = 0; i < count; i += j) {
		off_t pageIndex = startPageIndex + i;
		swap_addr_t slotIndex = startSlotIndex + i;

		swap_hash_key key = { this, pageIndex };

		swap_block *swap = sSwapHashTable.Lookup(key);
		while (swap == NULL) {
			swap = (swap_block *)object_cache_alloc(sSwapBlockCache,
				CACHE_DONT_SLEEP);
			if (swap == NULL) {
				// Wait a short time until memory is available again.
				mutex_unlock(&sSwapHashLock);
				snooze(10000);
				mutex_lock(&sSwapHashLock);
				swap = sSwapHashTable.Lookup(key);
				continue;
			}

			swap->key.cache = this;
			swap->key.page_index = pageIndex & ~(off_t)SWAP_BLOCK_MASK;
			swap->used = 0;
			for (uint32 i = 0; i < SWAP_BLOCK_PAGES; i++)
				swap->swap_slots[i] = SWAP_SLOT_NONE;

			sSwapHashTable.InsertUnchecked(swap);
		}

		swap_addr_t blockIndex = pageIndex & SWAP_BLOCK_MASK;
		for (j = 0; blockIndex < SWAP_BLOCK_PAGES && left > 0; j++) {
			swap->swap_slots[blockIndex++] = slotIndex + j;
			left--;
		}

		swap->used += j;
	}

	mutex_unlock(&sSwapHashLock);
}


void
VMAnonymousCache::_SwapBlockFree(off_t startPageIndex, uint32 count)
{
	mutex_lock(&sSwapHashLock);

	uint32 left = count;
	for (uint32 i = 0, j = 0; i < count; i += j) {
		off_t pageIndex = startPageIndex + i;
		swap_hash_key key = { this, pageIndex };
		swap_block *swap = sSwapHashTable.Lookup(key);

		ASSERT(swap != NULL);

		swap_addr_t blockIndex = pageIndex & SWAP_BLOCK_MASK;
		for (j = 0; blockIndex < SWAP_BLOCK_PAGES && left > 0; j++) {
			swap->swap_slots[blockIndex++] = SWAP_SLOT_NONE;
			left--;
		}

		swap->used -= j;
		if (swap->used == 0) {
			sSwapHashTable.RemoveUnchecked(swap);
			object_cache_free(sSwapBlockCache, swap);
		}
	}

	mutex_unlock(&sSwapHashLock);
}


swap_addr_t
VMAnonymousCache::_SwapBlockGetAddress(off_t pageIndex)
{
	mutex_lock(&sSwapHashLock);

	swap_hash_key key = { this, pageIndex };
	swap_block *swap = sSwapHashTable.Lookup(key);
	swap_addr_t slotIndex = SWAP_SLOT_NONE;

	if (swap != NULL) {
		swap_addr_t blockIndex = pageIndex & SWAP_BLOCK_MASK;
		slotIndex = swap->swap_slots[blockIndex];
	}

	mutex_unlock(&sSwapHashLock);

	return slotIndex;
}


status_t
VMAnonymousCache::_Commit(off_t size)
{
	// Basic strategy: reserve swap space first, only when running out of swap
	// space, reserve real memory.

	off_t committedMemory = committed_size - fCommittedSwapSize;

	// Regardless of whether we're asked to grow or shrink the commitment,
	// we always try to reserve as much as possible of the final commitment
	// in the swap space.
	if (size > fCommittedSwapSize) {
		fCommittedSwapSize += swap_space_reserve(size - fCommittedSwapSize);
		committed_size = fCommittedSwapSize + committedMemory;
	}

	if (committed_size == size)
		return B_OK;

	if (committed_size > size) {
		// The commitment shrinks -- unreserve real memory first.
		off_t toUnreserve = committed_size - size;
		if (committedMemory > 0) {
			off_t unreserved = min_c(toUnreserve, committedMemory);
			vm_unreserve_memory(unreserved);
			committedMemory -= unreserved;
			committed_size -= unreserved;
			toUnreserve -= unreserved;
		}

		// Unreserve swap space.
		if (toUnreserve > 0) {
			swap_space_unreserve(toUnreserve);
			fCommittedSwapSize -= toUnreserve;
			committed_size -= toUnreserve;
		}

		return B_OK;
	}

	// The commitment grows -- we have already tried to reserve swap space at
	// the start of the method, so we try to reserve real memory, now.

	off_t toReserve = size - committed_size;
	if (vm_try_reserve_memory(toReserve, 1000000) != B_OK)
		return B_NO_MEMORY;

	committed_size = size;
	return B_OK;
}


// #pragma mark -


status_t
swap_file_add(char *path)
{
	vnode *node = NULL;
	status_t status = vfs_get_vnode_from_path(path, true, &node);
	if (status != B_OK)
		return status;

	swap_file *swap = (swap_file *)malloc(sizeof(swap_file));
	if (swap == NULL) {
		vfs_put_vnode(node);
		return B_NO_MEMORY;
	}

	swap->vnode = node;

	struct stat st;
	status = vfs_stat_vnode(node, &st);
	if (status != B_OK) {
		free(swap);
		vfs_put_vnode(node);
		return status;
    }

	if (!(S_ISREG(st.st_mode) || S_ISCHR(st.st_mode) || S_ISBLK(st.st_mode))) {
		free(swap);
		vfs_put_vnode(node);
		return B_ERROR;
	}

	int32 pageCount = st.st_size >> PAGE_SHIFT;
	swap->used = 0;

	swap->maps = (uint32 *)malloc((pageCount + 7) / 8);
	if (swap->maps == NULL) {
		free(swap);
		vfs_put_vnode(node);
		return B_NO_MEMORY;
	}
	memset(swap->maps, 0, (pageCount + 7) / 8);
	swap->hint = 0;

	// set slot index and add this file to swap file list
	mutex_lock(&sSwapFileListLock);
	if (sSwapFileList.IsEmpty()) {
		swap->first_slot = 0;
		swap->last_slot = pageCount;
	} else {
		// leave one page gap between two swap files
		swap->first_slot = sSwapFileList.Last()->last_slot + 1;
		swap->last_slot = swap->first_slot + pageCount;
	}
	sSwapFileList.Add(swap);
	sSwapFileCount++;
	mutex_unlock(&sSwapFileListLock);

	mutex_lock(&sAvailSwapSpaceLock);
	sAvailSwapSpace += pageCount * B_PAGE_SIZE;
	mutex_unlock(&sAvailSwapSpaceLock);

	return B_OK;
}


status_t
swap_file_delete(char *path)
{
	vnode *node = NULL;
	status_t status = vfs_get_vnode_from_path(path, true, &node);
	if (status != B_OK)
		return status;

	MutexLocker locker(sSwapFileListLock);

	swap_file *swapFile = NULL;
	for (SwapFileList::Iterator it = sSwapFileList.GetIterator();
			(swapFile = it.Next()) != NULL;) {
		if (swapFile->vnode == node)
			break;
	}

	vfs_put_vnode(node);

	if (swapFile == NULL)
		return B_ERROR;

	// if this file is currently used, we can't delete
	// TODO: mark this swap file deleting, and remove it after releasing
	// all the swap space
	if (swapFile->used > 0)
		return B_ERROR;

	sSwapFileList.Remove(swapFile);
	sSwapFileCount--;
	locker.Unlock();

	mutex_lock(&sAvailSwapSpaceLock);
	sAvailSwapSpace -= (swapFile->last_slot - swapFile->first_slot) * PAGE_SIZE;
	mutex_unlock(&sAvailSwapSpaceLock);

	vfs_put_vnode(swapFile->vnode);
	free(swapFile->maps);
	free(swapFile);

	return B_OK;
}


void
swap_init(void)
{
	// create swap block cache
	sSwapBlockCache = create_object_cache("swapblock",
			sizeof(swap_block), sizeof(void*), NULL, NULL, NULL);
	if (sSwapBlockCache == NULL)
		panic("swap_init(): can't create object cache for swap blocks\n");

	status_t error = object_cache_set_minimum_reserve(sSwapBlockCache,
		MIN_SWAP_BLOCK_RESERVE);
	if (error != B_OK) {
		panic("swap_init(): object_cache_set_minimum_reserve() failed: %s",
			strerror(error));
	}

	// init swap hash table
	sSwapHashTable.Init(INITIAL_SWAP_HASH_SIZE);
	mutex_init(&sSwapHashLock, "swaphash");

	error = register_resource_resizer(swap_hash_resizer, NULL,
		SWAP_HASH_RESIZE_INTERVAL);
	if (error != B_OK) {
		panic("swap_init(): Failed to register swap hash resizer: %s",
			strerror(error));
	}

	// init swap file list
	mutex_init(&sSwapFileListLock, "swaplist");
	sSwapFileAlloc = NULL;
	sSwapFileCount = 0;

	// init available swap space
	mutex_init(&sAvailSwapSpaceLock, "avail swap space");
	sAvailSwapSpace = 0;

	add_debugger_command_etc("swap", &dump_swap_info,
		"Print infos about the swap usage",
		"\n"
		"Print infos about the swap usage.\n", 0);
}


void
swap_init_post_modules()
{
	off_t size = 0;

	void *settings = load_driver_settings("virtual_memory");
	if (settings != NULL) {
		if (!get_driver_boolean_parameter(settings, "vm", false, false))
			return;

		const char *string = get_driver_parameter(settings, "swap_size", NULL,
			NULL);
		size = string ? atoll(string) : 0;

		unload_driver_settings(settings);
	} else
		size = vm_page_num_pages() * B_PAGE_SIZE * 2;

	if (size < B_PAGE_SIZE)
		return;

	int fd = open("/var/swap", O_RDWR | O_CREAT | O_NOCACHE, S_IRUSR | S_IWUSR);
	if (fd < 0) {
		dprintf("Can't open/create /var/swap: %s\n", strerror(errno));
		return;
	}

	struct stat stat;
	stat.st_size = size;
	status_t error = _kern_write_stat(fd, NULL, false, &stat,
		sizeof(struct stat), B_STAT_SIZE | B_STAT_SIZE_INSECURE);
	if (error != B_OK) {
		dprintf("Failed to resize /var/swap to %lld bytes: %s\n", size,
			strerror(error));
	}

	close(fd);
		// TODO: if we don't keep the file open, O_NOCACHE is going to be
		// removed - we must not do this while we're using the swap file

	swap_file_add("/var/swap");
}


// used by page daemon to free swap space
bool
swap_free_page_swap_space(vm_page *page)
{
	VMAnonymousCache *cache = dynamic_cast<VMAnonymousCache *>(page->cache);
	if (cache == NULL)
		return false;

	swap_addr_t slotIndex = cache->_SwapBlockGetAddress(page->cache_offset);
	if (slotIndex == SWAP_SLOT_NONE)
		return false;

	swap_slot_dealloc(slotIndex, 1);
	cache->fAllocatedSwapSize -= B_PAGE_SIZE;
	cache->_SwapBlockFree(page->cache_offset, 1);

  	return true;
}


uint32
swap_available_pages()
{
	mutex_lock(&sAvailSwapSpaceLock);
	uint32 avail = sAvailSwapSpace >> PAGE_SHIFT;
	mutex_unlock(&sAvailSwapSpaceLock);

	return avail;
}


uint32
swap_total_swap_pages()
{
	mutex_lock(&sSwapFileListLock);

	uint32 totalSwapSlots = 0;
	for (SwapFileList::Iterator it = sSwapFileList.GetIterator();
			swap_file *swapFile = it.Next();)
		totalSwapSlots += swapFile->last_slot - swapFile->first_slot;

	mutex_unlock(&sSwapFileListLock);

	return totalSwapSlots;
}

#endif	// ENABLE_SWAP_SUPPORT
