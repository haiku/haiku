/*
 * Copyright 2012, Alexander von Gluck IV, kallisti5@unixzen.com.
 * Copyright 2011, Hamish Morrison, hamish@lavabit.com.
 * Copyright 2008, Zhao Shuai, upczhsh@163.com.
 * Copyright 2008-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2009, Axel Dörfler, axeld@pinc-software.de.
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

#include <FindDirectory.h>
#include <KernelExport.h>
#include <NodeMonitor.h>

#include <arch_config.h>
#include <boot_device.h>
#include <disk_device_manager/KDiskDevice.h>
#include <disk_device_manager/KDiskDeviceManager.h>
#include <disk_device_manager/KDiskSystem.h>
#include <disk_device_manager/KPartitionVisitor.h>
#include <driver_settings.h>
#include <fs/fd.h>
#include <fs/KPath.h>
#include <fs_info.h>
#include <fs_interface.h>
#include <heap.h>
#include <kernel_daemon.h>
#include <slab/Slab.h>
#include <syscalls.h>
#include <system_info.h>
#include <tracing.h>
#include <util/AutoLock.h>
#include <util/DoublyLinkedList.h>
#include <util/OpenHashTable.h>
#include <util/RadixBitmap.h>
#include <vfs.h>
#include <vm/vm.h>
#include <vm/vm_page.h>
#include <vm/vm_priv.h>
#include <vm/VMAddressSpace.h>

#include "IORequest.h"


#if	ENABLE_SWAP_SUPPORT

//#define TRACE_VM_ANONYMOUS_CACHE
#ifdef TRACE_VM_ANONYMOUS_CACHE
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...) do { } while (false)
#endif


// number of free swap blocks the object cache shall minimally have
#define MIN_SWAP_BLOCK_RESERVE	4096

// interval the has resizer is triggered (in 0.1s)
#define SWAP_HASH_RESIZE_INTERVAL	5

#define INITIAL_SWAP_HASH_SIZE		1024

#define SWAP_SLOT_NONE	RADIX_SLOT_NONE

#define SWAP_BLOCK_PAGES 32
#define SWAP_BLOCK_SHIFT 5		/* 1 << SWAP_BLOCK_SHIFT == SWAP_BLOCK_PAGES */
#define SWAP_BLOCK_MASK  (SWAP_BLOCK_PAGES - 1)


static const char* const kDefaultSwapPath = "/var/swap";

struct swap_file : DoublyLinkedListLinkImpl<swap_file> {
	int				fd;
	struct vnode*	vnode;
	void*			cookie;
	swap_addr_t		first_slot;
	swap_addr_t		last_slot;
	radix_bitmap*	bmp;
};

struct swap_hash_key {
	VMAnonymousCache	*cache;
	off_t				page_index;  // page index in the cache
};

// Each swap block contains swap address information for
// SWAP_BLOCK_PAGES continuous pages from the same cache
struct swap_block {
	swap_block*		hash_link;
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
		VMAnonymousCache* cache = key.cache;
		return blockIndex ^ (size_t)(int*)cache;
	}

	size_t Hash(const swap_block* value) const
	{
		return HashKey(value->key);
	}

	bool Compare(const swap_hash_key& key, const swap_block* value) const
	{
		return (key.page_index & ~(off_t)SWAP_BLOCK_MASK)
				== (value->key.page_index & ~(off_t)SWAP_BLOCK_MASK)
			&& key.cache == value->key.cache;
	}

	swap_block*& GetLink(swap_block* value) const
	{
		return value->hash_link;
	}
};

typedef BOpenHashTable<SwapHashTableDefinition> SwapHashTable;
typedef DoublyLinkedList<swap_file> SwapFileList;

static SwapHashTable sSwapHashTable;
static rw_lock sSwapHashLock;

static SwapFileList sSwapFileList;
static mutex sSwapFileListLock;
static swap_file* sSwapFileAlloc = NULL; // allocate from here
static uint32 sSwapFileCount = 0;

static off_t sAvailSwapSpace = 0;
static mutex sAvailSwapSpaceLock;

static object_cache* sSwapBlockCache;


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
	swap_addr_t freeSwapPages = 0;

	kprintf("swap files:\n");

	for (SwapFileList::Iterator it = sSwapFileList.GetIterator();
		swap_file* file = it.Next();) {
		swap_addr_t total = file->last_slot - file->first_slot;
		kprintf("  vnode: %p, pages: total: %lu, free: %lu\n",
			file->vnode, total, file->bmp->free_slots);

		totalSwapPages += total;
		freeSwapPages += file->bmp->free_slots;
	}

	kprintf("\n");
	kprintf("swap space in pages:\n");
	kprintf("total:     %9lu\n", totalSwapPages);
	kprintf("available: %9llu\n", sAvailSwapSpace / B_PAGE_SIZE);
	kprintf("reserved:  %9llu\n",
		totalSwapPages - sAvailSwapSpace / B_PAGE_SIZE);
	kprintf("used:      %9lu\n", totalSwapPages - freeSwapPages);
	kprintf("free:      %9lu\n", freeSwapPages);

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

	// since radix bitmap could not handle more than 32 pages, we return
	// SWAP_SLOT_NONE, this forces Write() adjust allocation amount
	if (count > BITMAP_RADIX) {
		mutex_unlock(&sSwapFileListLock);
		return SWAP_SLOT_NONE;
	}

	swap_addr_t j, addr = SWAP_SLOT_NONE;
	for (j = 0; j < sSwapFileCount; j++) {
		if (sSwapFileAlloc == NULL)
			sSwapFileAlloc = sSwapFileList.First();

		addr = radix_bitmap_alloc(sSwapFileAlloc->bmp, count);
		if (addr != SWAP_SLOT_NONE) {
			addr += sSwapFileAlloc->first_slot;
			break;
		}

		// this swap_file is full, find another
		sSwapFileAlloc = sSwapFileList.GetNext(sSwapFileAlloc);
	}

	if (j == sSwapFileCount) {
		mutex_unlock(&sSwapFileListLock);
		panic("swap_slot_alloc: swap space exhausted!\n");
		return SWAP_SLOT_NONE;
	}

	// if this swap file has used more than 90% percent of its space
	// switch to another
	if (sSwapFileAlloc->bmp->free_slots
		< (sSwapFileAlloc->last_slot - sSwapFileAlloc->first_slot) / 10) {
		sSwapFileAlloc = sSwapFileList.GetNext(sSwapFileAlloc);
	}

	mutex_unlock(&sSwapFileListLock);

	return addr;
}


static swap_file*
find_swap_file(swap_addr_t slotIndex)
{
	for (SwapFileList::Iterator it = sSwapFileList.GetIterator();
		swap_file* swapFile = it.Next();) {
		if (slotIndex >= swapFile->first_slot
			&& slotIndex < swapFile->last_slot) {
			return swapFile;
		}
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
	swap_file* swapFile = find_swap_file(slotIndex);
	slotIndex -= swapFile->first_slot;
	radix_bitmap_dealloc(swapFile->bmp, slotIndex, count);
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
	WriteLocker locker(sSwapHashLock);

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
		generic_size_t bytesTransferred)
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
	int32 numGuardPages, uint32 allocationFlags)
{
	TRACE("%p->VMAnonymousCache::Init(canOvercommit = %s, "
		"numPrecommittedPages = %ld, numGuardPages = %ld)\n", this,
		canOvercommit ? "yes" : "no", numPrecommittedPages, numGuardPages);

	status_t error = VMCache::Init(CACHE_TYPE_RAM, allocationFlags);
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
VMAnonymousCache::Resize(off_t newSize, int priority)
{
	// If the cache size shrinks, drop all swap pages beyond the new size.
	if (fAllocatedSwapSize > 0) {
		page_num_t oldPageCount = (virtual_end + B_PAGE_SIZE - 1) >> PAGE_SHIFT;
		swap_block* swapBlock = NULL;

		for (page_num_t pageIndex = (newSize + B_PAGE_SIZE - 1) >> PAGE_SHIFT;
			pageIndex < oldPageCount && fAllocatedSwapSize > 0; pageIndex++) {

			WriteLocker locker(sSwapHashLock);

			// Get the swap slot index for the page.
			swap_addr_t blockIndex = pageIndex & SWAP_BLOCK_MASK;
			if (swapBlock == NULL || blockIndex == 0) {
				swap_hash_key key = { this, pageIndex };
				swapBlock = sSwapHashTable.Lookup(key);

				if (swapBlock == NULL) {
					pageIndex = ROUNDUP(pageIndex + 1, SWAP_BLOCK_PAGES);
					continue;
				}
			}

			swap_addr_t slotIndex = swapBlock->swap_slots[blockIndex];
			vm_page* page;
			if (slotIndex != SWAP_SLOT_NONE
				&& ((page = LookupPage((off_t)pageIndex * B_PAGE_SIZE)) == NULL
					|| !page->busy)) {
					// TODO: We skip (i.e. leak) swap space of busy pages, since
					// there could be I/O going on (paging in/out). Waiting is
					// not an option as 1. unlocking the cache means that new
					// swap pages could be added in a range we've already
					// cleared (since the cache still has the old size) and 2.
					// we'd risk a deadlock in case we come from the file cache
					// and the FS holds the node's write-lock. We should mark
					// the page invalid and let the one responsible clean up.
					// There's just no such mechanism yet.
				swap_slot_dealloc(slotIndex, 1);
				fAllocatedSwapSize -= B_PAGE_SIZE;

				swapBlock->swap_slots[blockIndex] = SWAP_SLOT_NONE;
				if (--swapBlock->used == 0) {
					// All swap pages have been freed -- we can discard the swap
					// block.
					sSwapHashTable.RemoveUnchecked(swapBlock);
					object_cache_free(sSwapBlockCache, swapBlock,
						CACHE_DONT_WAIT_FOR_MEMORY
							| CACHE_DONT_LOCK_KERNEL_SPACE);
				}
			}
		}
	}

	return VMCache::Resize(newSize, priority);
}


status_t
VMAnonymousCache::Commit(off_t size, int priority)
{
	TRACE("%p->VMAnonymousCache::Commit(%lld)\n", this, size);

	// If we can overcommit, we don't commit here, but in Fault(). We always
	// unreserve memory, if we're asked to shrink our commitment, though.
	if (fCanOvercommit && size > committed_size) {
		if (fHasPrecommitted)
			return B_OK;

		// pre-commit some pages to make a later failure less probable
		fHasPrecommitted = true;
		uint32 precommitted = fPrecommittedPages * B_PAGE_SIZE;
		if (size > precommitted)
			size = precommitted;
	}

	return _Commit(size, priority);
}


bool
VMAnonymousCache::HasPage(off_t offset)
{
	if (_SwapBlockGetAddress(offset >> PAGE_SHIFT) != SWAP_SLOT_NONE)
		return true;

	return false;
}


bool
VMAnonymousCache::DebugHasPage(off_t offset)
{
	page_num_t pageIndex = offset >> PAGE_SHIFT;
	swap_hash_key key = { this, pageIndex };
	swap_block* swap = sSwapHashTable.Lookup(key);
	if (swap == NULL)
		return false;

	return swap->swap_slots[pageIndex & SWAP_BLOCK_MASK] != SWAP_SLOT_NONE;
}


status_t
VMAnonymousCache::Read(off_t offset, const generic_io_vec* vecs, size_t count,
	uint32 flags, generic_size_t* _numBytes)
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

		swap_file* swapFile = find_swap_file(startSlotIndex);

		off_t pos = (off_t)(startSlotIndex - swapFile->first_slot)
			* B_PAGE_SIZE;

		status_t status = vfs_read_pages(swapFile->vnode, swapFile->cookie, pos,
			vecs + i, j - i, flags, _numBytes);
		if (status != B_OK)
			return status;
	}

	return B_OK;
}


status_t
VMAnonymousCache::Write(off_t offset, const generic_io_vec* vecs, size_t count,
	uint32 flags, generic_size_t* _numBytes)
{
	off_t pageIndex = offset >> PAGE_SHIFT;

	AutoLocker<VMCache> locker(this);

	page_num_t totalPages = 0;
	for (uint32 i = 0; i < count; i++) {
		page_num_t pageCount = (vecs[i].length + B_PAGE_SIZE - 1) >> PAGE_SHIFT;
		swap_addr_t slotIndex = _SwapBlockGetAddress(pageIndex + totalPages);
		if (slotIndex != SWAP_SLOT_NONE) {
			swap_slot_dealloc(slotIndex, pageCount);
			_SwapBlockFree(pageIndex + totalPages, pageCount);
			fAllocatedSwapSize -= pageCount * B_PAGE_SIZE;
		}

		totalPages += pageCount;
	}

	off_t totalSize = totalPages * B_PAGE_SIZE;
	if (fAllocatedSwapSize + totalSize > fCommittedSwapSize)
		return B_ERROR;

	fAllocatedSwapSize += totalSize;
	locker.Unlock();

	page_num_t pagesLeft = totalPages;
	totalPages = 0;

	for (uint32 i = 0; i < count; i++) {
		page_num_t pageCount = (vecs[i].length + B_PAGE_SIZE - 1) >> PAGE_SHIFT;

		generic_addr_t vectorBase = vecs[i].base;
		generic_size_t vectorLength = vecs[i].length;
		page_num_t n = pageCount;

		for (page_num_t j = 0; j < pageCount; j += n) {
			swap_addr_t slotIndex;
			// try to allocate n slots, if fail, try to allocate n/2
			while ((slotIndex = swap_slot_alloc(n)) == SWAP_SLOT_NONE && n >= 2)
				n >>= 1;

			if (slotIndex == SWAP_SLOT_NONE)
				panic("VMAnonymousCache::Write(): can't allocate swap space\n");

			T(WritePage(this, pageIndex, slotIndex));
				// TODO: Assumes that only one page is written.

			swap_file* swapFile = find_swap_file(slotIndex);

			off_t pos = (off_t)(slotIndex - swapFile->first_slot) * B_PAGE_SIZE;

			generic_size_t length = (phys_addr_t)n * B_PAGE_SIZE;
			generic_io_vec vector[1];
			vector->base = vectorBase;
			vector->length = length;

			status_t status = vfs_write_pages(swapFile->vnode, swapFile->cookie,
				pos, vector, 1, flags, &length);
			if (status != B_OK) {
				locker.Lock();
				fAllocatedSwapSize -= (off_t)pagesLeft * B_PAGE_SIZE;
				locker.Unlock();

				swap_slot_dealloc(slotIndex, n);
				return status;
			}

			_SwapBlockBuild(pageIndex + totalPages, slotIndex, n);
			pagesLeft -= n;

			if (n != pageCount) {
				vectorBase = vectorBase + n * B_PAGE_SIZE;
				vectorLength -= n * B_PAGE_SIZE;
			}
		}

		totalPages += pageCount;
	}

	ASSERT(pagesLeft == 0);
	return B_OK;
}


status_t
VMAnonymousCache::WriteAsync(off_t offset, const generic_io_vec* vecs,
	size_t count, generic_size_t numBytes, uint32 flags,
	AsyncIOCallback* _callback)
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
	WriteCallback* callback = (flags & B_VIP_IO_REQUEST) != 0
		? new(malloc_flags(HEAP_PRIORITY_VIP)) WriteCallback(this, _callback)
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
	off_t pos = (off_t)(slotIndex - swapFile->first_slot) * B_PAGE_SIZE;

	return vfs_asynchronous_write_pages(swapFile->vnode, swapFile->cookie, pos,
		vecs, 1, numBytes, flags, callback);
}


bool
VMAnonymousCache::CanWritePage(off_t offset)
{
	// We can write the page, if we have not used all of our committed swap
	// space or the page already has a swap slot assigned.
	return fAllocatedSwapSize < fCommittedSwapSize
		|| _SwapBlockGetAddress(offset >> PAGE_SHIFT) != SWAP_SLOT_NONE;
}


int32
VMAnonymousCache::MaxPagesPerAsyncWrite() const
{
	return 1;
}


status_t
VMAnonymousCache::Fault(struct VMAddressSpace* aspace, off_t offset)
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
			// never commit more than needed
			if (committed_size / B_PAGE_SIZE > page_count)
				return B_BAD_HANDLER;

			// try to commit additional swap space/memory
			if (swap_space_reserve(B_PAGE_SIZE) == B_PAGE_SIZE) {
				fCommittedSwapSize += B_PAGE_SIZE;
			} else {
				int priority = aspace == VMAddressSpace::Kernel()
					? VM_PRIORITY_SYSTEM : VM_PRIORITY_USER;
				if (vm_try_reserve_memory(B_PAGE_SIZE, priority, 0) != B_OK) {
					dprintf("%p->VMAnonymousCache::Fault(): Failed to reserve "
						"%d bytes of RAM.\n", this, (int)B_PAGE_SIZE);
					return B_NO_MEMORY;
				}
			}

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
		_Commit(actualSize, VM_PRIORITY_USER);

	// Move all not shadowed swap pages from the source to the consumer cache.
	// Also remove all source pages that are shadowed by consumer swap pages.
	_MergeSwapPages(source);

	// Move all not shadowed pages from the source to the consumer cache.
	if (source->page_count < page_count)
		_MergePagesSmallerSource(source);
	else
		_MergePagesSmallerConsumer(source);
}


void
VMAnonymousCache::DeleteObject()
{
	object_cache_delete(gAnonymousCacheObjectCache, this);
}


void
VMAnonymousCache::_SwapBlockBuild(off_t startPageIndex,
	swap_addr_t startSlotIndex, uint32 count)
{
	WriteLocker locker(sSwapHashLock);

	uint32 left = count;
	for (uint32 i = 0, j = 0; i < count; i += j) {
		off_t pageIndex = startPageIndex + i;
		swap_addr_t slotIndex = startSlotIndex + i;

		swap_hash_key key = { this, pageIndex };

		swap_block* swap = sSwapHashTable.Lookup(key);
		while (swap == NULL) {
			swap = (swap_block*)object_cache_alloc(sSwapBlockCache,
				CACHE_DONT_WAIT_FOR_MEMORY | CACHE_DONT_LOCK_KERNEL_SPACE);
			if (swap == NULL) {
				// Wait a short time until memory is available again.
				locker.Unlock();
				snooze(10000);
				locker.Lock();
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
}


void
VMAnonymousCache::_SwapBlockFree(off_t startPageIndex, uint32 count)
{
	WriteLocker locker(sSwapHashLock);

	uint32 left = count;
	for (uint32 i = 0, j = 0; i < count; i += j) {
		off_t pageIndex = startPageIndex + i;
		swap_hash_key key = { this, pageIndex };
		swap_block* swap = sSwapHashTable.Lookup(key);

		ASSERT(swap != NULL);

		swap_addr_t blockIndex = pageIndex & SWAP_BLOCK_MASK;
		for (j = 0; blockIndex < SWAP_BLOCK_PAGES && left > 0; j++) {
			swap->swap_slots[blockIndex++] = SWAP_SLOT_NONE;
			left--;
		}

		swap->used -= j;
		if (swap->used == 0) {
			sSwapHashTable.RemoveUnchecked(swap);
			object_cache_free(sSwapBlockCache, swap,
				CACHE_DONT_WAIT_FOR_MEMORY | CACHE_DONT_LOCK_KERNEL_SPACE);
		}
	}
}


swap_addr_t
VMAnonymousCache::_SwapBlockGetAddress(off_t pageIndex)
{
	ReadLocker locker(sSwapHashLock);

	swap_hash_key key = { this, pageIndex };
	swap_block* swap = sSwapHashTable.Lookup(key);
	swap_addr_t slotIndex = SWAP_SLOT_NONE;

	if (swap != NULL) {
		swap_addr_t blockIndex = pageIndex & SWAP_BLOCK_MASK;
		slotIndex = swap->swap_slots[blockIndex];
	}

	return slotIndex;
}


status_t
VMAnonymousCache::_Commit(off_t size, int priority)
{
	TRACE("%p->VMAnonymousCache::_Commit(%lld), already committed: %lld "
		"(%lld swap)\n", this, size, committed_size, fCommittedSwapSize);

	// Basic strategy: reserve swap space first, only when running out of swap
	// space, reserve real memory.

	off_t committedMemory = committed_size - fCommittedSwapSize;

	// Regardless of whether we're asked to grow or shrink the commitment,
	// we always try to reserve as much as possible of the final commitment
	// in the swap space.
	if (size > fCommittedSwapSize) {
		fCommittedSwapSize += swap_space_reserve(size - fCommittedSwapSize);
		committed_size = fCommittedSwapSize + committedMemory;
		if (size > fCommittedSwapSize) {
			TRACE("%p->VMAnonymousCache::_Commit(%lld), reserved only %lld "
				"swap\n", this, size, fCommittedSwapSize);
		}
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
	if (vm_try_reserve_memory(toReserve, priority, 1000000) != B_OK) {
		dprintf("%p->VMAnonymousCache::_Commit(%lld): Failed to reserve %lld "
			"bytes of RAM\n", this, size, toReserve);
		return B_NO_MEMORY;
	}

	committed_size = size;
	return B_OK;
}


void
VMAnonymousCache::_MergePagesSmallerSource(VMAnonymousCache* source)
{
	// The source cache has less pages than the consumer (this cache), so we
	// iterate through the source's pages and move the ones that are not
	// shadowed up to the consumer.

	for (VMCachePagesTree::Iterator it = source->pages.GetIterator();
			vm_page* page = it.Next();) {
		// Note: Removing the current node while iterating through a
		// IteratableSplayTree is safe.
		vm_page* consumerPage = LookupPage(
			(off_t)page->cache_offset << PAGE_SHIFT);
		if (consumerPage == NULL) {
			// the page is not yet in the consumer cache - move it upwards
			ASSERT_PRINT(!page->busy, "page: %p", page);
			MovePage(page);
		}
	}
}


void
VMAnonymousCache::_MergePagesSmallerConsumer(VMAnonymousCache* source)
{
	// The consumer (this cache) has less pages than the source, so we move the
	// consumer's pages to the source (freeing shadowed ones) and finally just
	// all pages of the source back to the consumer.

	for (VMCachePagesTree::Iterator it = pages.GetIterator();
		vm_page* page = it.Next();) {
		// If a source page is in the way, remove and free it.
		vm_page* sourcePage = source->LookupPage(
			(off_t)page->cache_offset << PAGE_SHIFT);
		if (sourcePage != NULL) {
			DEBUG_PAGE_ACCESS_START(sourcePage);
			ASSERT_PRINT(!sourcePage->busy, "page: %p", sourcePage);
			source->RemovePage(sourcePage);
			vm_page_free(source, sourcePage);
		}

		// Note: Removing the current node while iterating through a
		// IteratableSplayTree is safe.
		source->MovePage(page);
	}

	MoveAllPages(source);
}


void
VMAnonymousCache::_MergeSwapPages(VMAnonymousCache* source)
{
	// If neither source nor consumer have swap pages, we don't have to do
	// anything.
	if (source->fAllocatedSwapSize == 0 && fAllocatedSwapSize == 0)
		return;

	for (off_t offset = source->virtual_base
		& ~(off_t)(B_PAGE_SIZE * SWAP_BLOCK_PAGES - 1);
		offset < source->virtual_end;
		offset += B_PAGE_SIZE * SWAP_BLOCK_PAGES) {

		WriteLocker locker(sSwapHashLock);

		page_num_t swapBlockPageIndex = offset >> PAGE_SHIFT;
		swap_hash_key key = { source, swapBlockPageIndex };
		swap_block* sourceSwapBlock = sSwapHashTable.Lookup(key);

		// remove the source swap block -- we will either take over the swap
		// space (and the block) or free it
		if (sourceSwapBlock != NULL)
			sSwapHashTable.RemoveUnchecked(sourceSwapBlock);

		key.cache = this;
		swap_block* swapBlock = sSwapHashTable.Lookup(key);

		locker.Unlock();

		// remove all source pages that are shadowed by consumer swap pages
		if (swapBlock != NULL) {
			for (uint32 i = 0; i < SWAP_BLOCK_PAGES; i++) {
				if (swapBlock->swap_slots[i] != SWAP_SLOT_NONE) {
					vm_page* page = source->LookupPage(
						(off_t)(swapBlockPageIndex + i) << PAGE_SHIFT);
					if (page != NULL) {
						DEBUG_PAGE_ACCESS_START(page);
						ASSERT_PRINT(!page->busy, "page: %p", page);
						source->RemovePage(page);
						vm_page_free(source, page);
					}
				}
			}
		}

		if (sourceSwapBlock == NULL)
			continue;

		for (uint32 i = 0; i < SWAP_BLOCK_PAGES; i++) {
			off_t pageIndex = swapBlockPageIndex + i;
			swap_addr_t sourceSlotIndex = sourceSwapBlock->swap_slots[i];

			if (sourceSlotIndex == SWAP_SLOT_NONE)
				continue;

			if ((swapBlock != NULL
					&& swapBlock->swap_slots[i] != SWAP_SLOT_NONE)
				|| LookupPage((off_t)pageIndex << PAGE_SHIFT) != NULL) {
				// The consumer already has a page or a swapped out page
				// at this index. So we can free the source swap space.
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
		// the consumer.
		fAllocatedSwapSize += B_PAGE_SIZE * (off_t)sourceSwapBlock->used;

		if (sourceSwapBlock->used == 0) {
			// All swap pages have been freed -- we can discard the source swap
			// block.
			object_cache_free(sSwapBlockCache, sourceSwapBlock,
				CACHE_DONT_WAIT_FOR_MEMORY | CACHE_DONT_LOCK_KERNEL_SPACE);
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
			// already a swap block in the consumer cache. Copy the respective
			// swap addresses and discard the source swap block.
			for (uint32 i = 0; i < SWAP_BLOCK_PAGES; i++) {
				if (sourceSwapBlock->swap_slots[i] != SWAP_SLOT_NONE)
					swapBlock->swap_slots[i] = sourceSwapBlock->swap_slots[i];
			}

			object_cache_free(sSwapBlockCache, sourceSwapBlock,
				CACHE_DONT_WAIT_FOR_MEMORY | CACHE_DONT_LOCK_KERNEL_SPACE);
		}
	}
}


// #pragma mark -


// TODO: This can be removed if we get BFS uuid's
struct VolumeInfo {
	char name[B_FILE_NAME_LENGTH];
	char device[B_FILE_NAME_LENGTH];
	char filesystem[B_OS_NAME_LENGTH];
	off_t capacity;
};


class PartitionScorer : public KPartitionVisitor {
public:
	PartitionScorer(VolumeInfo& volumeInfo)
		:
		fBestPartition(NULL),
		fBestScore(-1),
		fVolumeInfo(volumeInfo)
	{
	}

	virtual bool VisitPre(KPartition* partition)
	{
		if (!partition->ContainsFileSystem())
			return false;

		KPath path;
		partition->GetPath(&path);

		int score = 0;
		if (strcmp(fVolumeInfo.name, partition->ContentName()) == 0)
			score += 4;
		if (strcmp(fVolumeInfo.device, path.Path()) == 0)
			score += 3;
		if (fVolumeInfo.capacity == partition->Size())
			score += 2;
		if (strcmp(fVolumeInfo.filesystem,
			partition->DiskSystem()->ShortName()) == 0) {
			score += 1;
		}
		if (score >= 4 && score > fBestScore) {
			fBestPartition = partition;
			fBestScore = score;
		}

		return false;
	}

	KPartition* fBestPartition;

private:
	int32		fBestScore;
	VolumeInfo	fVolumeInfo;
};


status_t
get_mount_point(KPartition* partition, KPath* mountPoint)
{
	if (!mountPoint || !partition->ContainsFileSystem())
		return B_BAD_VALUE;

	const char* volumeName = partition->ContentName();
	if (!volumeName || strlen(volumeName) == 0)
		volumeName = partition->Name();
	if (!volumeName || strlen(volumeName) == 0)
		volumeName = "unnamed volume";

	char basePath[B_PATH_NAME_LENGTH];
	int32 len = snprintf(basePath, sizeof(basePath), "/%s", volumeName);
	for (int32 i = 1; i < len; i++)
		if (basePath[i] == '/')
		basePath[i] = '-';
	char* path = mountPoint->LockBuffer();
	int32 pathLen = mountPoint->BufferSize();
	strncpy(path, basePath, pathLen);

	struct stat dummy;
	for (int i = 1; ; i++) {
		if (stat(path, &dummy) != 0)
			break;
		snprintf(path, pathLen, "%s%d", basePath, i);
	}

	mountPoint->UnlockBuffer();
	return B_OK;
}


status_t
swap_file_add(const char* path)
{
	// open the file
	int fd = open(path, O_RDWR | O_NOCACHE, S_IRUSR | S_IWUSR);
	if (fd < 0)
		return errno;

	// fstat() it and check whether we can use it
	struct stat st;
	if (fstat(fd, &st) < 0) {
		close(fd);
		return errno;
	}

	if (!(S_ISREG(st.st_mode) || S_ISCHR(st.st_mode) || S_ISBLK(st.st_mode))) {
		close(fd);
		return B_BAD_VALUE;
	}

	if (st.st_size < B_PAGE_SIZE) {
		close(fd);
		return B_BAD_VALUE;
	}

	// get file descriptor, vnode, and cookie
	file_descriptor* descriptor = get_fd(get_current_io_context(true), fd);
	put_fd(descriptor);

	vnode* node = fd_vnode(descriptor);
	if (node == NULL) {
		close(fd);
		return B_BAD_VALUE;
	}

	// do the allocations and prepare the swap_file structure
	swap_file* swap = (swap_file*)malloc(sizeof(swap_file));
	if (swap == NULL) {
		close(fd);
		return B_NO_MEMORY;
	}

	swap->fd = fd;
	swap->vnode = node;
	swap->cookie = descriptor->cookie;

	uint32 pageCount = st.st_size >> PAGE_SHIFT;
	swap->bmp = radix_bitmap_create(pageCount);
	if (swap->bmp == NULL) {
		free(swap);
		close(fd);
		return B_NO_MEMORY;
	}

	// set slot index and add this file to swap file list
	mutex_lock(&sSwapFileListLock);
	// TODO: Also check whether the swap file is already registered!
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
	sAvailSwapSpace += (off_t)pageCount * B_PAGE_SIZE;
	mutex_unlock(&sAvailSwapSpaceLock);

	return B_OK;
}


status_t
swap_file_delete(const char* path)
{
	vnode* node = NULL;
	status_t status = vfs_get_vnode_from_path(path, true, &node);
	if (status != B_OK)
		return status;

	MutexLocker locker(sSwapFileListLock);

	swap_file* swapFile = NULL;
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
	if (swapFile->bmp->free_slots < swapFile->last_slot - swapFile->first_slot)
		return B_ERROR;

	sSwapFileList.Remove(swapFile);
	sSwapFileCount--;
	locker.Unlock();

	mutex_lock(&sAvailSwapSpaceLock);
	sAvailSwapSpace -= (off_t)(swapFile->last_slot - swapFile->first_slot)
		* PAGE_SIZE;
	mutex_unlock(&sAvailSwapSpaceLock);

	close(swapFile->fd);
	radix_bitmap_destroy(swapFile->bmp);
	free(swapFile);

	return B_OK;
}


void
swap_init(void)
{
	// create swap block cache
	sSwapBlockCache = create_object_cache("swapblock", sizeof(swap_block),
		sizeof(void*), NULL, NULL, NULL);
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
	rw_lock_init(&sSwapHashLock, "swaphash");

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
	// Never try to create a swap file on a read-only device - when booting
	// from CD, the write overlay is used.
	if (gReadOnlyBootDevice)
		return;

	bool swapEnabled = true;
	bool swapAutomatic = true;
	off_t swapSize = 0;

	dev_t swapDeviceID = -1;
	VolumeInfo selectedVolume = {};

	void* settings = load_driver_settings("virtual_memory");

	if (settings != NULL) {
		// We pass a lot of information on the swap device, this is mostly to
		// ensure that we are dealing with the same device that was configured.

		// TODO: Some kind of BFS uuid would be great here :)
		const char* enabled = get_driver_parameter(settings, "vm", NULL, NULL);

		if (enabled != NULL) {
			swapEnabled = get_driver_boolean_parameter(settings, "vm",
				false, false);

			const char* size = get_driver_parameter(settings, "swap_size",
				NULL, NULL);
			const char* volume = get_driver_parameter(settings,
				"swap_volume_name", NULL, NULL);
			const char* device = get_driver_parameter(settings,
				"swap_volume_device", NULL, NULL);
			const char* filesystem = get_driver_parameter(settings,
				"swap_volume_filesystem", NULL, NULL);
			const char* capacity = get_driver_parameter(settings,
				"swap_volume_capacity", NULL, NULL);

			if (size != NULL && device != NULL && volume != NULL
				&& filesystem != NULL && capacity != NULL) {
				// User specified a size / volume
				swapAutomatic = false;
				swapSize = atoll(size);
				strncpy(selectedVolume.name, volume,
					sizeof(selectedVolume.name));
				strncpy(selectedVolume.device, device,
					sizeof(selectedVolume.device));
				strncpy(selectedVolume.filesystem, filesystem,
					sizeof(selectedVolume.filesystem));
				selectedVolume.capacity = atoll(capacity);
			} else if (size != NULL) {
				// Older file format, no location information (assume /var/swap)
				swapAutomatic = false;
				swapSize = atoll(size);
				swapDeviceID = gBootDevice;
			}
		}
		unload_driver_settings(settings);
	}

	if (swapAutomatic) {
		swapEnabled = true;
		swapSize = (off_t)vm_page_num_pages() * B_PAGE_SIZE * 2;
	}

	if (!swapEnabled || swapSize < B_PAGE_SIZE)
		return;

	if (!swapAutomatic && swapDeviceID < 0) {
		// If user-specified swap, and no swap device has been chosen yet...
		KDiskDeviceManager::CreateDefault();
		KDiskDeviceManager* manager = KDiskDeviceManager::Default();
		PartitionScorer visitor(selectedVolume);

		KDiskDevice* device;
		int32 cookie = 0;
		while ((device = manager->NextDevice(&cookie)) != NULL) {
			if (device->IsReadOnlyMedia() || device->IsWriteOnce()
				|| device->IsRemovable()) {
				continue;
			}
			device->VisitEachDescendant(&visitor);
		}

		if (!visitor.fBestPartition) {
			dprintf("%s: Can't find configured swap partition '%s'\n",
				__func__, selectedVolume.name);
		} else {
			if (visitor.fBestPartition->IsMounted())
				swapDeviceID = visitor.fBestPartition->VolumeID();
			else {
				KPath devPath, mountPoint;
				visitor.fBestPartition->GetPath(&devPath);
				get_mount_point(visitor.fBestPartition, &mountPoint);
				const char* mountPath = mountPoint.Path();
				mkdir(mountPath, S_IRWXU | S_IRWXG | S_IRWXO);
				swapDeviceID = _kern_mount(mountPath, devPath.Path(),
					NULL, 0, NULL, 0);
				if (swapDeviceID < 0) {
					dprintf("%s: Can't mount configured swap partition '%s'\n",
						__func__, selectedVolume.name);
				}
			}
		}
	}

	if (swapDeviceID < 0)
		swapDeviceID = gBootDevice;

	// We now have a swapDeviceID which is used for the swap file

	KPath path;
	struct fs_info info;
	_kern_read_fs_info(swapDeviceID, &info);
	if (swapDeviceID == gBootDevice)
		path = kDefaultSwapPath;
	else {
		vfs_entry_ref_to_path(info.dev, info.root,
			".", path.LockBuffer(), path.BufferSize());
		path.UnlockBuffer();
		path.Append("swap");
	}

	const char* swapPath = path.Path();

	// Swap size limits prevent oversized swap files
	off_t existingSwapSize = 0;
	struct stat existingSwapStat;
	if (stat(swapPath, &existingSwapStat) == 0)
		existingSwapSize = existingSwapStat.st_size;

	off_t freeSpace = info.free_blocks * info.block_size + existingSwapSize;
	off_t maxSwap = freeSpace;
	if (swapAutomatic) {
		// Adjust automatic swap to a maximum of 25% of the free space
		maxSwap = (off_t)(0.25 * freeSpace);
	} else {
		// If user specified, leave 10% of the disk free
		maxSwap = freeSpace - (off_t)(0.10 * freeSpace);
		dprintf("%s: Warning: User specified swap file consumes over 90%% of "
			"the available free space, limiting to 90%%\n", __func__);
	}

	if (swapSize > maxSwap)
		swapSize = maxSwap;

	// Create swap file
	int fd = open(swapPath, O_RDWR | O_CREAT | O_NOCACHE, S_IRUSR | S_IWUSR);
	if (fd < 0) {
		dprintf("%s: Can't open/create %s: %s\n", __func__,
			swapPath, strerror(errno));
		return;
	}

	struct stat stat;
	stat.st_size = swapSize;
	status_t error = _kern_write_stat(fd, NULL, false, &stat,
		sizeof(struct stat), B_STAT_SIZE | B_STAT_SIZE_INSECURE);
	if (error != B_OK) {
		dprintf("%s: Failed to resize %s to %lld bytes: %s\n", __func__,
			swapPath, swapSize, strerror(error));
	}

	close(fd);

	error = swap_file_add(swapPath);
	if (error != B_OK) {
		dprintf("%s: Failed to add swap file %s: %s\n", __func__, swapPath,
			strerror(error));
	}
}


//! Used by page daemon to free swap space.
bool
swap_free_page_swap_space(vm_page* page)
{
	VMAnonymousCache* cache = dynamic_cast<VMAnonymousCache*>(page->Cache());
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
		swap_file* swapFile = it.Next();) {
		totalSwapSlots += swapFile->last_slot - swapFile->first_slot;
	}

	mutex_unlock(&sSwapFileListLock);

	return totalSwapSlots;
}


#endif	// ENABLE_SWAP_SUPPORT


void
swap_get_info(struct system_memory_info* info)
{
#if ENABLE_SWAP_SUPPORT
	info->max_swap_space = (uint64)swap_total_swap_pages() * B_PAGE_SIZE;
	info->free_swap_space = (uint64)swap_available_pages() * B_PAGE_SIZE;
#else
	info->max_swap_space = 0;
	info->free_swap_space = 0;
#endif
}

