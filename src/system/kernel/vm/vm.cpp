/*
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <vm/vm.h>

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#include <algorithm>

#include <OS.h>
#include <KernelExport.h>

#include <AutoDeleterDrivers.h>

#include <symbol_versioning.h>

#include <arch/cpu.h>
#include <arch/vm.h>
#include <arch/user_memory.h>
#include <boot/elf.h>
#include <boot/stage2.h>
#include <condition_variable.h>
#include <console.h>
#include <debug.h>
#include <file_cache.h>
#include <fs/fd.h>
#include <heap.h>
#include <kernel.h>
#include <interrupts.h>
#include <lock.h>
#include <low_resource_manager.h>
#include <slab/Slab.h>
#include <smp.h>
#include <system_info.h>
#include <thread.h>
#include <team.h>
#include <tracing.h>
#include <util/AutoLock.h>
#include <util/BitUtils.h>
#include <util/ThreadAutoLock.h>
#include <vm/vm_page.h>
#include <vm/vm_priv.h>
#include <vm/VMAddressSpace.h>
#include <vm/VMArea.h>
#include <vm/VMCache.h>

#include "VMAddressSpaceLocking.h"
#include "VMAnonymousCache.h"
#include "VMAnonymousNoSwapCache.h"
#include "IORequest.h"


//#define TRACE_VM
//#define TRACE_FAULTS
#ifdef TRACE_VM
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif
#ifdef TRACE_FAULTS
#	define FTRACE(x) dprintf x
#else
#	define FTRACE(x) ;
#endif


namespace {

class AreaCacheLocking {
public:
	inline bool Lock(VMCache* lockable)
	{
		return false;
	}

	inline void Unlock(VMCache* lockable)
	{
		vm_area_put_locked_cache(lockable);
	}
};

class AreaCacheLocker : public AutoLocker<VMCache, AreaCacheLocking> {
public:
	inline AreaCacheLocker(VMCache* cache = NULL)
		: AutoLocker<VMCache, AreaCacheLocking>(cache, true)
	{
	}

	inline AreaCacheLocker(VMArea* area)
		: AutoLocker<VMCache, AreaCacheLocking>()
	{
		SetTo(area);
	}

	inline void SetTo(VMCache* cache, bool alreadyLocked)
	{
		AutoLocker<VMCache, AreaCacheLocking>::SetTo(cache, alreadyLocked);
	}

	inline void SetTo(VMArea* area)
	{
		return AutoLocker<VMCache, AreaCacheLocking>::SetTo(
			area != NULL ? vm_area_get_locked_cache(area) : NULL, true, true);
	}
};


class VMCacheChainLocker {
public:
	VMCacheChainLocker()
		:
		fTopCache(NULL),
		fBottomCache(NULL)
	{
	}

	VMCacheChainLocker(VMCache* topCache)
		:
		fTopCache(topCache),
		fBottomCache(topCache)
	{
	}

	~VMCacheChainLocker()
	{
		Unlock();
	}

	void SetTo(VMCache* topCache)
	{
		fTopCache = topCache;
		fBottomCache = topCache;

		if (topCache != NULL)
			topCache->SetUserData(NULL);
	}

	VMCache* LockSourceCache()
	{
		if (fBottomCache == NULL || fBottomCache->source == NULL)
			return NULL;

		VMCache* previousCache = fBottomCache;

		fBottomCache = fBottomCache->source;
		fBottomCache->Lock();
		fBottomCache->AcquireRefLocked();
		fBottomCache->SetUserData(previousCache);

		return fBottomCache;
	}

	void LockAllSourceCaches()
	{
		while (LockSourceCache() != NULL) {
		}
	}

	void Unlock(VMCache* exceptCache = NULL)
	{
		if (fTopCache == NULL)
			return;

		// Unlock caches in source -> consumer direction. This is important to
		// avoid double-locking and a reversal of locking order in case a cache
		// is eligable for merging.
		VMCache* cache = fBottomCache;
		while (cache != NULL) {
			VMCache* nextCache = (VMCache*)cache->UserData();
			if (cache != exceptCache)
				cache->ReleaseRefAndUnlock(cache != fTopCache);

			if (cache == fTopCache)
				break;

			cache = nextCache;
		}

		fTopCache = NULL;
		fBottomCache = NULL;
	}

	void UnlockKeepRefs(bool keepTopCacheLocked)
	{
		if (fTopCache == NULL)
			return;

		VMCache* nextCache = fBottomCache;
		VMCache* cache = NULL;

		while (keepTopCacheLocked
				? nextCache != fTopCache : cache != fTopCache) {
			cache = nextCache;
			nextCache = (VMCache*)cache->UserData();
			cache->Unlock(cache != fTopCache);
		}
	}

	void RelockCaches(bool topCacheLocked)
	{
		if (fTopCache == NULL)
			return;

		VMCache* nextCache = fTopCache;
		VMCache* cache = NULL;
		if (topCacheLocked) {
			cache = nextCache;
			nextCache = cache->source;
		}

		while (cache != fBottomCache && nextCache != NULL) {
			VMCache* consumer = cache;
			cache = nextCache;
			nextCache = cache->source;
			cache->Lock();
			cache->SetUserData(consumer);
		}
	}

private:
	VMCache*	fTopCache;
	VMCache*	fBottomCache;
};

} // namespace


// The memory reserve an allocation of the certain priority must not touch.
static const size_t kMemoryReserveForPriority[] = {
	VM_MEMORY_RESERVE_USER,		// user
	VM_MEMORY_RESERVE_SYSTEM,	// system
	0							// VIP
};


static ObjectCache** sPageMappingsObjectCaches;
static uint32 sPageMappingsMask;

static rw_lock sAreaCacheLock = RW_LOCK_INITIALIZER("area->cache");

static rw_spinlock sAvailableMemoryLock = B_RW_SPINLOCK_INITIALIZER;
static off_t sAvailableMemory;
static off_t sNeededMemory;

static uint32 sPageFaults;
static VMPhysicalPageMapper* sPhysicalPageMapper;


// function declarations
static void delete_area(VMAddressSpace* addressSpace, VMArea* area,
	bool deletingAddressSpace, bool alreadyRemoved = false);
static status_t vm_soft_fault(VMAddressSpace* addressSpace, addr_t address,
	bool isWrite, bool isExecute, bool isUser, vm_page** wirePage);
static status_t map_backing_store(VMAddressSpace* addressSpace,
	VMCache* cache, off_t offset, const char* areaName, addr_t size, int wiring,
	int protection, int protectionMax, int mapping, uint32 flags,
	const virtual_address_restrictions* addressRestrictions, bool kernel,
	VMArea** _area, void** _virtualAddress);
static void fix_protection(uint32* protection);


//	#pragma mark -


#if VM_PAGE_FAULT_TRACING

namespace VMPageFaultTracing {

class PageFaultStart : public AbstractTraceEntry {
public:
	PageFaultStart(addr_t address, bool write, bool user, addr_t pc)
		:
		fAddress(address),
		fPC(pc),
		fWrite(write),
		fUser(user)
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("page fault %#lx %s %s, pc: %#lx", fAddress,
			fWrite ? "write" : "read", fUser ? "user" : "kernel", fPC);
	}

private:
	addr_t	fAddress;
	addr_t	fPC;
	bool	fWrite;
	bool	fUser;
};


// page fault errors
enum {
	PAGE_FAULT_ERROR_NO_AREA		= 0,
	PAGE_FAULT_ERROR_KERNEL_ONLY,
	PAGE_FAULT_ERROR_WRITE_PROTECTED,
	PAGE_FAULT_ERROR_READ_PROTECTED,
	PAGE_FAULT_ERROR_EXECUTE_PROTECTED,
	PAGE_FAULT_ERROR_KERNEL_BAD_USER_MEMORY,
	PAGE_FAULT_ERROR_NO_ADDRESS_SPACE
};


class PageFaultError : public AbstractTraceEntry {
public:
	PageFaultError(area_id area, status_t error)
		:
		fArea(area),
		fError(error)
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		switch (fError) {
			case PAGE_FAULT_ERROR_NO_AREA:
				out.Print("page fault error: no area");
				break;
			case PAGE_FAULT_ERROR_KERNEL_ONLY:
				out.Print("page fault error: area: %ld, kernel only", fArea);
				break;
			case PAGE_FAULT_ERROR_WRITE_PROTECTED:
				out.Print("page fault error: area: %ld, write protected",
					fArea);
				break;
			case PAGE_FAULT_ERROR_READ_PROTECTED:
				out.Print("page fault error: area: %ld, read protected", fArea);
				break;
			case PAGE_FAULT_ERROR_EXECUTE_PROTECTED:
				out.Print("page fault error: area: %ld, execute protected",
					fArea);
				break;
			case PAGE_FAULT_ERROR_KERNEL_BAD_USER_MEMORY:
				out.Print("page fault error: kernel touching bad user memory");
				break;
			case PAGE_FAULT_ERROR_NO_ADDRESS_SPACE:
				out.Print("page fault error: no address space");
				break;
			default:
				out.Print("page fault error: area: %ld, error: %s", fArea,
					strerror(fError));
				break;
		}
	}

private:
	area_id		fArea;
	status_t	fError;
};


class PageFaultDone : public AbstractTraceEntry {
public:
	PageFaultDone(area_id area, VMCache* topCache, VMCache* cache,
			vm_page* page)
		:
		fArea(area),
		fTopCache(topCache),
		fCache(cache),
		fPage(page)
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("page fault done: area: %ld, top cache: %p, cache: %p, "
			"page: %p", fArea, fTopCache, fCache, fPage);
	}

private:
	area_id		fArea;
	VMCache*	fTopCache;
	VMCache*	fCache;
	vm_page*	fPage;
};

}	// namespace VMPageFaultTracing

#	define TPF(x) new(std::nothrow) VMPageFaultTracing::x;
#else
#	define TPF(x) ;
#endif	// VM_PAGE_FAULT_TRACING


//	#pragma mark - page mappings allocation


static void
create_page_mappings_object_caches()
{
	// We want an even power of 2 smaller than the number of CPUs.
	const int32 numCPUs = smp_get_num_cpus();
	int32 count = next_power_of_2(numCPUs);
	if (count > numCPUs)
		count >>= 1;
	sPageMappingsMask = count - 1;

	sPageMappingsObjectCaches = new object_cache*[count];
	if (sPageMappingsObjectCaches == NULL)
		panic("failed to allocate page mappings object_cache array");

	for (int32 i = 0; i < count; i++) {
		char name[32];
		snprintf(name, sizeof(name), "page mappings %" B_PRId32, i);

		object_cache* cache = create_object_cache_etc(name,
			sizeof(vm_page_mapping), 0, 0, 64, 128, CACHE_LARGE_SLAB, NULL, NULL,
			NULL, NULL);
		if (cache == NULL)
			panic("failed to create page mappings object_cache");

		object_cache_set_minimum_reserve(cache, 1024);
		sPageMappingsObjectCaches[i] = cache;
	}
}


static object_cache*
page_mapping_object_cache_for(page_num_t page)
{
	return sPageMappingsObjectCaches[page & sPageMappingsMask];
}


static vm_page_mapping*
allocate_page_mapping(page_num_t page, uint32 flags = 0)
{
	return (vm_page_mapping*)object_cache_alloc(page_mapping_object_cache_for(page),
		flags);
}


void
vm_free_page_mapping(page_num_t page, vm_page_mapping* mapping, uint32 flags)
{
	object_cache_free(page_mapping_object_cache_for(page), mapping, flags);
}


//	#pragma mark -


/*!	The page's cache must be locked.
*/
static inline void
increment_page_wired_count(vm_page* page)
{
	if (!page->IsMapped())
		atomic_add(&gMappedPagesCount, 1);
	page->IncrementWiredCount();
}


/*!	The page's cache must be locked.
*/
static inline void
decrement_page_wired_count(vm_page* page)
{
	page->DecrementWiredCount();
	if (!page->IsMapped())
		atomic_add(&gMappedPagesCount, -1);
}


static inline addr_t
virtual_page_address(VMArea* area, vm_page* page)
{
	return area->Base()
		+ ((page->cache_offset << PAGE_SHIFT) - area->cache_offset);
}


static inline bool
is_page_in_area(VMArea* area, vm_page* page)
{
	off_t pageCacheOffsetBytes = (off_t)(page->cache_offset << PAGE_SHIFT);
	return pageCacheOffsetBytes >= area->cache_offset
		&& pageCacheOffsetBytes < area->cache_offset + (off_t)area->Size();
}


//! You need to have the address space locked when calling this function
static VMArea*
lookup_area(VMAddressSpace* addressSpace, area_id id)
{
	VMAreas::ReadLock();

	VMArea* area = VMAreas::LookupLocked(id);
	if (area != NULL && area->address_space != addressSpace)
		area = NULL;

	VMAreas::ReadUnlock();

	return area;
}


static inline size_t
area_page_protections_size(size_t areaSize)
{
	// In the page protections we store only the three user protections,
	// so we use 4 bits per page.
	return (areaSize / B_PAGE_SIZE + 1) / 2;
}


static status_t
allocate_area_page_protections(VMArea* area)
{
	size_t bytes = area_page_protections_size(area->Size());
	area->page_protections = (uint8*)malloc_etc(bytes,
		area->address_space == VMAddressSpace::Kernel()
			? HEAP_DONT_LOCK_KERNEL_SPACE : 0);
	if (area->page_protections == NULL)
		return B_NO_MEMORY;

	// init the page protections for all pages to that of the area
	uint32 areaProtection = area->protection
		& (B_READ_AREA | B_WRITE_AREA | B_EXECUTE_AREA);
	memset(area->page_protections, areaProtection | (areaProtection << 4), bytes);

	// clear protections from the area
	area->protection &= ~(B_READ_AREA | B_WRITE_AREA | B_EXECUTE_AREA
		| B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA | B_KERNEL_EXECUTE_AREA);
	return B_OK;
}


static inline uint8*
realloc_area_page_protections(uint8* pageProtections, size_t areaSize,
	uint32 allocationFlags)
{
	size_t bytes = area_page_protections_size(areaSize);
	return (uint8*)realloc_etc(pageProtections, bytes, allocationFlags);
}


static inline void
set_area_page_protection(VMArea* area, addr_t pageAddress, uint32 protection)
{
	protection &= B_READ_AREA | B_WRITE_AREA | B_EXECUTE_AREA;
	addr_t pageIndex = (pageAddress - area->Base()) / B_PAGE_SIZE;
	uint8& entry = area->page_protections[pageIndex / 2];
	if (pageIndex % 2 == 0)
		entry = (entry & 0xf0) | protection;
	else
		entry = (entry & 0x0f) | (protection << 4);
}


static inline uint32
get_area_page_protection(VMArea* area, addr_t pageAddress)
{
	if (area->page_protections == NULL)
		return area->protection;

	uint32 pageIndex = (pageAddress - area->Base()) / B_PAGE_SIZE;
	uint32 protection = area->page_protections[pageIndex / 2];
	if (pageIndex % 2 == 0)
		protection &= 0x0f;
	else
		protection >>= 4;

	uint32 kernelProtection = 0;
	if ((protection & B_READ_AREA) != 0)
		kernelProtection |= B_KERNEL_READ_AREA;
	if ((protection & B_WRITE_AREA) != 0)
		kernelProtection |= B_KERNEL_WRITE_AREA;

	// If this is a kernel area we return only the kernel flags.
	if (area->address_space == VMAddressSpace::Kernel())
		return kernelProtection;

	return protection | kernelProtection;
}


/*! Computes the committed size an area's cache ought to have,
	based on the area's page_protections and any pages already present.
*/
static inline uint32
compute_area_page_commitment(VMArea* area)
{
	if (area->page_protections == NULL) {
		if ((area->protection & (B_WRITE_AREA | B_KERNEL_WRITE_AREA)) != 0)
			return area->Size();
		return area->cache->page_count * B_PAGE_SIZE;
	}

	const size_t bytes = area_page_protections_size(area->Size());
	const bool oddPageCount = ((area->Size() / B_PAGE_SIZE) % 2) != 0;
	size_t pages = 0;
	for (size_t i = 0; i < bytes; i++) {
		const uint8 protection = area->page_protections[i];
		const off_t pageOffset = area->cache_offset + (i * 2 * B_PAGE_SIZE);
		if (area->cache->LookupPage(pageOffset) != NULL)
			pages++;
		else
			pages += ((protection & (B_WRITE_AREA << 0)) != 0) ? 1 : 0;

		if (i == (bytes - 1) && oddPageCount)
			break;

		if (area->cache->LookupPage(pageOffset + B_PAGE_SIZE) != NULL)
			pages++;
		else
			pages += ((protection & (B_WRITE_AREA << 4)) != 0) ? 1 : 0;
	}
	return pages;
}


/*!	The caller must have reserved enough pages the translation map
	implementation might need to map this page.
	The page's cache must be locked.
*/
static status_t
map_page(VMArea* area, vm_page* page, addr_t address, uint32 protection,
	vm_page_reservation* reservation)
{
	VMTranslationMap* map = area->address_space->TranslationMap();

	bool wasMapped = page->IsMapped();

	if (area->wiring == B_NO_LOCK) {
		DEBUG_PAGE_ACCESS_CHECK(page);

		bool isKernelSpace = area->address_space == VMAddressSpace::Kernel();
		vm_page_mapping* mapping = allocate_page_mapping(page->physical_page_number,
			CACHE_DONT_WAIT_FOR_MEMORY
				| (isKernelSpace ? CACHE_DONT_LOCK_KERNEL_SPACE : 0));
		if (mapping == NULL)
			return B_NO_MEMORY;

		mapping->page = page;
		mapping->area = area;

		map->Lock();

		map->Map(address, page->physical_page_number * B_PAGE_SIZE, protection,
			area->MemoryType(), reservation);

		// insert mapping into lists
		if (!page->IsMapped())
			atomic_add(&gMappedPagesCount, 1);

		page->mappings.Add(mapping);
		area->mappings.Add(mapping);

		map->Unlock();
	} else {
		DEBUG_PAGE_ACCESS_CHECK(page);

		map->Lock();
		map->Map(address, page->physical_page_number * B_PAGE_SIZE, protection,
			area->MemoryType(), reservation);
		map->Unlock();

		increment_page_wired_count(page);
	}

	if (!wasMapped) {
		// The page is mapped now, so we must not remain in the cached queue.
		// It also makes sense to move it from the inactive to the active, since
		// otherwise the page daemon wouldn't come to keep track of it (in idle
		// mode) -- if the page isn't touched, it will be deactivated after a
		// full iteration through the queue at the latest.
		if (page->State() == PAGE_STATE_CACHED
				|| page->State() == PAGE_STATE_INACTIVE) {
			vm_page_set_state(page, PAGE_STATE_ACTIVE);
		}
	}

	return B_OK;
}


/*!	If \a preserveModified is \c true, the caller must hold the lock of the
	page's cache.
*/
static inline bool
unmap_page(VMArea* area, addr_t virtualAddress)
{
	return area->address_space->TranslationMap()->UnmapPage(area,
		virtualAddress, true);
}


/*!	If \a preserveModified is \c true, the caller must hold the lock of all
	mapped pages' caches.
*/
static inline void
unmap_pages(VMArea* area, addr_t base, size_t size)
{
	area->address_space->TranslationMap()->UnmapPages(area, base, size, true);
}


static inline bool
intersect_area(VMArea* area, addr_t& address, addr_t& size, addr_t& offset)
{
	if (address < area->Base()) {
		offset = area->Base() - address;
		if (offset >= size)
			return false;

		address = area->Base();
		size -= offset;
		offset = 0;
		if (size > area->Size())
			size = area->Size();

		return true;
	}

	offset = address - area->Base();
	if (offset >= area->Size())
		return false;

	if (size >= area->Size() - offset)
		size = area->Size() - offset;

	return true;
}


/*!	Cuts a piece out of an area. If the given cut range covers the complete
	area, it is deleted. If it covers the beginning or the end, the area is
	resized accordingly. If the range covers some part in the middle of the
	area, it is split in two; in this case the second area is returned via
	\a _secondArea (the variable is left untouched in the other cases).
	The address space must be write locked.
	The caller must ensure that no part of the given range is wired.
*/
static status_t
cut_area(VMAddressSpace* addressSpace, VMArea* area, addr_t address,
	addr_t size, VMArea** _secondArea, bool kernel)
{
	addr_t offset;
	if (!intersect_area(area, address, size, offset))
		return B_OK;

	// Is the area fully covered?
	if (address == area->Base() && size == area->Size()) {
		delete_area(addressSpace, area, false);
		return B_OK;
	}

	int priority;
	uint32 allocationFlags;
	if (addressSpace == VMAddressSpace::Kernel()) {
		priority = VM_PRIORITY_SYSTEM;
		allocationFlags = HEAP_DONT_WAIT_FOR_MEMORY
			| HEAP_DONT_LOCK_KERNEL_SPACE;
	} else {
		priority = VM_PRIORITY_USER;
		allocationFlags = 0;
	}

	int resizePriority = priority;
	const bool overcommitting = (area->protection & B_OVERCOMMITTING_AREA) != 0,
		writable = (area->protection & (B_WRITE_AREA | B_KERNEL_WRITE_AREA)) != 0;
	if ((area->page_protections != NULL || !writable) && !overcommitting) {
		// We'll adjust commitments directly, rather than letting VMCache do it.
		resizePriority = -1;
	}

	VMCache* cache = vm_area_get_locked_cache(area);
	VMCacheChainLocker cacheChainLocker(cache);
	cacheChainLocker.LockAllSourceCaches();

	// If no one else uses the area's cache and it's an anonymous cache, we can
	// resize or split it, too.
	bool onlyCacheUser = cache->areas.First() == area && cache->areas.GetNext(area) == NULL
		&& cache->consumers.IsEmpty() && cache->type == CACHE_TYPE_RAM;

	const addr_t oldSize = area->Size();

	// Cut the end only?
	if (offset > 0 && size == (area->Size() - offset)) {
		status_t error = addressSpace->ResizeArea(area, offset,
			allocationFlags);
		if (error != B_OK)
			return error;

		if (area->page_protections != NULL) {
			uint8* newProtections = realloc_area_page_protections(
				area->page_protections, area->Size(), allocationFlags);

			if (newProtections == NULL) {
				addressSpace->ResizeArea(area, oldSize, allocationFlags);
				return B_NO_MEMORY;
			}

			area->page_protections = newProtections;
		}

		// unmap pages
		unmap_pages(area, address, size);

		if (onlyCacheUser) {
			// Since VMCache::Resize() can temporarily drop the lock, we must
			// unlock all lower caches to prevent locking order inversion.
			cacheChainLocker.Unlock(cache);
			status_t status = cache->Resize(cache->virtual_base + offset, resizePriority);
			ASSERT_ALWAYS(status == B_OK);
		}

		if (resizePriority == -1) {
			const size_t newCommitmentPages = compute_area_page_commitment(area);
			cache->Commit(newCommitmentPages * B_PAGE_SIZE, priority);
		}

		if (onlyCacheUser)
			cache->ReleaseRefAndUnlock();
		return B_OK;
	}

	// Cut the beginning only?
	if (area->Base() == address) {
		uint8* newProtections = NULL;
		if (area->page_protections != NULL) {
			// Allocate all memory before shifting, as the shift might lose some bits.
			newProtections = realloc_area_page_protections(NULL, area->Size(),
				allocationFlags);

			if (newProtections == NULL)
				return B_NO_MEMORY;
		}

		// resize the area
		status_t error = addressSpace->ShrinkAreaHead(area, area->Size() - size,
			allocationFlags);
		if (error != B_OK) {
			free_etc(newProtections, allocationFlags);
			return error;
		}

		if (area->page_protections != NULL) {
			size_t oldBytes = area_page_protections_size(oldSize);
			ssize_t pagesShifted = (oldSize - area->Size()) / B_PAGE_SIZE;
			bitmap_shift<uint8>(area->page_protections, oldBytes * 8, -(pagesShifted * 4));

			size_t bytes = area_page_protections_size(area->Size());
			memcpy(newProtections, area->page_protections, bytes);
			free_etc(area->page_protections, allocationFlags);
			area->page_protections = newProtections;
		}

		// unmap pages
		unmap_pages(area, address, size);

		if (onlyCacheUser) {
			// Since VMCache::Rebase() can temporarily drop the lock, we must
			// unlock all lower caches to prevent locking order inversion.
			cacheChainLocker.Unlock(cache);
			status_t status = cache->Rebase(cache->virtual_base + size, resizePriority);
			ASSERT_ALWAYS(status == B_OK);
		}

		area->cache_offset += size;
		if (resizePriority == -1) {
			const size_t newCommitmentPages = compute_area_page_commitment(area);
			cache->Commit(newCommitmentPages * B_PAGE_SIZE, priority);
		}

		if (onlyCacheUser)
			cache->ReleaseRefAndUnlock();

		return B_OK;
	}

	// The tough part -- cut a piece out of the middle of the area.
	// We do that by shrinking the area to the begin section and creating a
	// new area for the end section.
	const addr_t firstNewSize = offset;
	const addr_t secondBase = address + size;
	const addr_t secondSize = area->Size() - offset - size;
	const off_t secondCacheOffset = area->cache_offset + (secondBase - area->Base());

	// unmap pages
	unmap_pages(area, address, area->Size() - firstNewSize);

	// resize the area
	status_t error = addressSpace->ResizeArea(area, firstNewSize,
		allocationFlags);
	if (error != B_OK)
		return error;

	uint8* areaNewProtections = NULL;
	uint8* secondAreaNewProtections = NULL;

	// Try to allocate the new memory before making some hard to reverse
	// changes.
	if (area->page_protections != NULL) {
		areaNewProtections = realloc_area_page_protections(NULL, area->Size(),
			allocationFlags);
		secondAreaNewProtections = realloc_area_page_protections(NULL, secondSize,
			allocationFlags);

		if (areaNewProtections == NULL || secondAreaNewProtections == NULL) {
			addressSpace->ResizeArea(area, oldSize, allocationFlags);
			free_etc(areaNewProtections, allocationFlags);
			free_etc(secondAreaNewProtections, allocationFlags);
			return B_NO_MEMORY;
		}
	}

	virtual_address_restrictions addressRestrictions = {};
	addressRestrictions.address = (void*)secondBase;
	addressRestrictions.address_specification = B_EXACT_ADDRESS;
	VMArea* secondArea;
	AutoLocker<VMCache> secondCacheLocker;

	if (onlyCacheUser) {
		// Create a new cache for the second area.
		VMCache* secondCache;
		error = VMCacheFactory::CreateAnonymousCache(secondCache,
			overcommitting, 0, 0,
			dynamic_cast<VMAnonymousNoSwapCache*>(cache) == NULL, priority);
		if (error != B_OK) {
			addressSpace->ResizeArea(area, oldSize, allocationFlags);
			free_etc(areaNewProtections, allocationFlags);
			free_etc(secondAreaNewProtections, allocationFlags);
			return error;
		}

		secondCache->Lock();
		secondCacheLocker.SetTo(secondCache, true);
		secondCache->temporary = cache->temporary;
		secondCache->virtual_base = secondCacheOffset;

		size_t commitmentStolen = 0;
		if (!overcommitting && resizePriority != -1) {
			// Steal some of the original cache's commitment.
			const size_t steal = PAGE_ALIGN(secondSize);
			if (cache->committed_size > (off_t)steal) {
				cache->committed_size -= steal;
				secondCache->committed_size += steal;
				commitmentStolen = steal;
			}
		}
		error = secondCache->Resize(secondCache->virtual_base + secondSize, resizePriority);

		if (error == B_OK) {
			if (cache->source != NULL)
				cache->source->AddConsumer(secondCache);

			// Transfer the concerned pages from the first cache.
			error = secondCache->Adopt(cache, secondCache->virtual_base, secondSize,
				secondCache->virtual_base);
		}

		if (error == B_OK) {
			// We no longer need the lower cache locks (and they can't be held
			// during the later Resize() anyway, since it could unlock temporarily.)
			cacheChainLocker.Unlock(cache);
			cacheChainLocker.SetTo(cache);

			// Map the second area.
			error = map_backing_store(addressSpace, secondCache,
				secondCacheOffset, area->name, secondSize,
				area->wiring, area->protection, area->protection_max,
				REGION_NO_PRIVATE_MAP, CREATE_AREA_DONT_COMMIT_MEMORY,
				&addressRestrictions, kernel, &secondArea, NULL);
		}

		if (error != B_OK) {
			secondCache->committed_size -= commitmentStolen;
			cache->committed_size += commitmentStolen;

			// Move the pages back.
			status_t readoptStatus = cache->Adopt(secondCache,
				secondCache->virtual_base, secondSize, secondCache->virtual_base);
			if (readoptStatus != B_OK) {
				// Some (swap) pages have not been moved back and will be lost
				// once the second cache is deleted.
				panic("failed to restore cache range: %s",
					strerror(readoptStatus));

				// TODO: Handle out of memory cases by freeing memory and
				// retrying.
			}

			secondCache->ReleaseRefLocked();
			addressSpace->ResizeArea(area, oldSize, allocationFlags);
			free_etc(areaNewProtections, allocationFlags);
			free_etc(secondAreaNewProtections, allocationFlags);
			return error;
		}

		error = cache->Resize(cache->virtual_base + firstNewSize, resizePriority);
		ASSERT_ALWAYS(error == B_OK);
	} else {
		// Reuse the existing cache.
		error = map_backing_store(addressSpace, cache, secondCacheOffset,
			area->name, secondSize, area->wiring, area->protection,
			area->protection_max, REGION_NO_PRIVATE_MAP, 0,
			&addressRestrictions, kernel, &secondArea, NULL);
		if (error != B_OK) {
			addressSpace->ResizeArea(area, oldSize, allocationFlags);
			free_etc(areaNewProtections, allocationFlags);
			free_etc(secondAreaNewProtections, allocationFlags);
			return error;
		}

		// We need a cache reference for the new area.
		cache->AcquireRefLocked();
	}

	if (area->page_protections != NULL) {
		// Copy the protection bits of the first area.
		const size_t areaBytes = area_page_protections_size(area->Size());
		memcpy(areaNewProtections, area->page_protections, areaBytes);
		uint8* areaOldProtections = area->page_protections;
		area->page_protections = areaNewProtections;

		// Shift the protection bits of the second area to the start of
		// the old array.
		const size_t oldBytes = area_page_protections_size(oldSize);
		addr_t secondAreaOffset = secondBase - area->Base();
		ssize_t secondAreaPagesShifted = secondAreaOffset / B_PAGE_SIZE;
		bitmap_shift<uint8>(areaOldProtections, oldBytes * 8, -(secondAreaPagesShifted * 4));

		// Copy the protection bits of the second area.
		const size_t secondAreaBytes = area_page_protections_size(secondSize);
		memcpy(secondAreaNewProtections, areaOldProtections, secondAreaBytes);
		secondArea->page_protections = secondAreaNewProtections;

		// We don't need this anymore.
		free_etc(areaOldProtections, allocationFlags);
	}

	if (resizePriority == -1) {
		// Adjust commitments.
		const off_t areaCommit = compute_area_page_commitment(area) * B_PAGE_SIZE;
		if (areaCommit < area->cache->committed_size) {
			secondArea->cache->committed_size += area->cache->committed_size - areaCommit;
			area->cache->committed_size = areaCommit;
		}
		area->cache->Commit(areaCommit, priority);

		const off_t secondCommit = compute_area_page_commitment(secondArea) * B_PAGE_SIZE;
		secondArea->cache->Commit(secondCommit, priority);
	}

	if (_secondArea != NULL)
		*_secondArea = secondArea;

	return B_OK;
}


/*!	Deletes or cuts all areas in the given address range.
	The address space must be write-locked.
	The caller must ensure that no part of the given range is wired.
*/
static status_t
unmap_address_range(VMAddressSpace* addressSpace, addr_t address, addr_t size,
	bool kernel)
{
	size = PAGE_ALIGN(size);

	// Check, whether the caller is allowed to modify the concerned areas.
	if (!kernel) {
		for (VMAddressSpace::AreaRangeIterator it
				= addressSpace->GetAreaRangeIterator(address, size);
			VMArea* area = it.Next();) {

			if ((area->protection & B_KERNEL_AREA) != 0) {
				dprintf("unmap_address_range: team %" B_PRId32 " tried to "
					"unmap range of kernel area %" B_PRId32 " (%s)\n",
					team_get_current_team_id(), area->id, area->name);
				return B_NOT_ALLOWED;
			}
		}
	}

	for (VMAddressSpace::AreaRangeIterator it
			= addressSpace->GetAreaRangeIterator(address, size);
		VMArea* area = it.Next();) {

		status_t error = cut_area(addressSpace, area, address, size, NULL,
			kernel);
		if (error != B_OK)
			return error;
			// Failing after already messing with areas is ugly, but we
			// can't do anything about it.
	}

	return B_OK;
}


static status_t
discard_area_range(VMArea* area, addr_t address, addr_t size)
{
	addr_t offset;
	if (!intersect_area(area, address, size, offset))
		return B_OK;

	// If someone else uses the area's cache or it's not an anonymous cache, we
	// can't discard.
	VMCache* cache = vm_area_get_locked_cache(area);
	if (cache->areas.First() != area || VMArea::CacheList::GetNext(area) != NULL
		|| !cache->consumers.IsEmpty() || cache->type != CACHE_TYPE_RAM) {
		return B_OK;
	}

	VMCacheChainLocker cacheChainLocker(cache);
	cacheChainLocker.LockAllSourceCaches();

	unmap_pages(area, address, size);

	ssize_t commitmentChange = 0;
	if (cache->temporary && !cache->CanOvercommit() && area->page_protections != NULL) {
		// See if the commitment can be shrunken after the pages are discarded.
		const off_t areaCacheBase = area->Base() - area->cache_offset;
		const off_t endAddress = address + size;
		for (off_t pageAddress = address; pageAddress < endAddress; pageAddress += B_PAGE_SIZE) {
			if (cache->LookupPage(pageAddress - areaCacheBase) == NULL)
				continue;

			const bool isWritable
				= (get_area_page_protection(area, pageAddress) & B_WRITE_AREA) != 0;
			if (!isWritable)
				commitmentChange -= B_PAGE_SIZE;
		}
	}

	// Since VMCache::Discard() can temporarily drop the lock, we must
	// unlock all lower caches to prevent locking order inversion.
	cacheChainLocker.Unlock(cache);
	cache->Discard(area->cache_offset + offset, size);

	if (commitmentChange != 0)
		cache->Commit(cache->committed_size + commitmentChange, VM_PRIORITY_USER);

	cache->ReleaseRefAndUnlock();
	return B_OK;
}


static status_t
discard_address_range(VMAddressSpace* addressSpace, addr_t address, addr_t size,
	bool kernel)
{
	for (VMAddressSpace::AreaRangeIterator it
		= addressSpace->GetAreaRangeIterator(address, size);
			VMArea* area = it.Next();) {
		status_t error = discard_area_range(area, address, size);
		if (error != B_OK)
			return error;
	}

	return B_OK;
}


/*! You need to hold the lock of the cache and the write lock of the address
	space when calling this function.
	Note, that in case of error your cache will be temporarily unlocked.
	If \a addressSpec is \c B_EXACT_ADDRESS and the
	\c CREATE_AREA_UNMAP_ADDRESS_RANGE flag is specified, the caller must ensure
	that no part of the specified address range (base \c *_virtualAddress, size
	\a size) is wired. The cache will also be temporarily unlocked.
*/
static status_t
map_backing_store(VMAddressSpace* addressSpace, VMCache* cache, off_t offset,
	const char* areaName, addr_t size, int wiring, int protection,
	int protectionMax, int mapping,
	uint32 flags, const virtual_address_restrictions* addressRestrictions,
	bool kernel, VMArea** _area, void** _virtualAddress)
{
	TRACE(("map_backing_store: aspace %p, cache %p, virtual %p, offset 0x%"
		B_PRIx64 ", size %" B_PRIuADDR ", addressSpec %" B_PRIu32 ", wiring %d"
		", protection %d, protectionMax %d, area %p, areaName '%s'\n",
		addressSpace, cache, addressRestrictions->address, offset, size,
		addressRestrictions->address_specification, wiring, protection,
		protectionMax, _area, areaName));
	cache->AssertLocked();

	if (size == 0) {
#if KDEBUG
		panic("map_backing_store(): called with size=0 for area '%s'!",
			areaName);
#endif
		return B_BAD_VALUE;
	}
	if (offset < 0)
		return B_BAD_VALUE;

	uint32 allocationFlags = HEAP_DONT_WAIT_FOR_MEMORY
		| HEAP_DONT_LOCK_KERNEL_SPACE;
	int priority;
	if (addressSpace != VMAddressSpace::Kernel()) {
		priority = VM_PRIORITY_USER;
	} else if ((flags & CREATE_AREA_PRIORITY_VIP) != 0) {
		priority = VM_PRIORITY_VIP;
		allocationFlags |= HEAP_PRIORITY_VIP;
	} else
		priority = VM_PRIORITY_SYSTEM;

	VMArea* area = addressSpace->CreateArea(areaName, wiring, protection,
		allocationFlags);
	if (area == NULL)
		return B_NO_MEMORY;

	if (mapping != REGION_PRIVATE_MAP)
		area->protection_max = protectionMax & B_USER_PROTECTION;

	status_t status;

	// if this is a private map, we need to create a new cache
	// to handle the private copies of pages as they are written to
	VMCache* sourceCache = cache;
	if (mapping == REGION_PRIVATE_MAP) {
		VMCache* newCache;

		// create an anonymous cache
		status = VMCacheFactory::CreateAnonymousCache(newCache,
			(protection & B_STACK_AREA) != 0
				|| (protection & B_OVERCOMMITTING_AREA) != 0, 0,
			cache->GuardSize() / B_PAGE_SIZE, true, VM_PRIORITY_USER);
		if (status != B_OK)
			goto err1;

		newCache->Lock();
		newCache->temporary = 1;
		newCache->virtual_base = offset;
		newCache->virtual_end = offset + size;

		cache->AddConsumer(newCache);

		cache = newCache;
	}

	if ((flags & CREATE_AREA_DONT_COMMIT_MEMORY) == 0) {
		status = cache->SetMinimalCommitment(size, priority);
		if (status != B_OK)
			goto err2;
	}

	// check to see if this address space has entered DELETE state
	if (addressSpace->IsBeingDeleted()) {
		// okay, someone is trying to delete this address space now, so we can't
		// insert the area, so back out
		status = B_BAD_TEAM_ID;
		goto err2;
	}

	if (addressRestrictions->address_specification == B_EXACT_ADDRESS
			&& (flags & CREATE_AREA_UNMAP_ADDRESS_RANGE) != 0) {
		// temporarily unlock the current cache since it might be mapped to
		// some existing area, and unmap_address_range also needs to lock that
		// cache to delete the area.
		cache->Unlock();
		status = unmap_address_range(addressSpace,
			(addr_t)addressRestrictions->address, size, kernel);
		cache->Lock();
		if (status != B_OK)
			goto err2;
	}

	status = addressSpace->InsertArea(area, size, addressRestrictions,
		allocationFlags, _virtualAddress);
	if (status == B_NO_MEMORY
			&& addressRestrictions->address_specification == B_ANY_KERNEL_ADDRESS) {
		// Due to how many locks are held, we cannot wait here for space to be
		// freed up, but we can at least notify the low_resource handler.
		low_resource(B_KERNEL_RESOURCE_ADDRESS_SPACE, size, B_RELATIVE_TIMEOUT, 0);
	}
	if (status != B_OK)
		goto err2;

	// attach the cache to the area
	area->cache = cache;
	area->cache_offset = offset;

	// point the cache back to the area
	cache->InsertAreaLocked(area);
	if (mapping == REGION_PRIVATE_MAP)
		cache->Unlock();

	// insert the area in the global areas map
	status = VMAreas::Insert(area);
	if (status != B_OK)
		goto err3;

	// grab a ref to the address space (the area holds this)
	addressSpace->Get();

//	ktrace_printf("map_backing_store: cache: %p (source: %p), \"%s\" -> %p",
//		cache, sourceCache, areaName, area);

	*_area = area;
	return B_OK;

err3:
	cache->Lock();
	cache->RemoveArea(area);
	area->cache = NULL;
err2:
	if (mapping == REGION_PRIVATE_MAP) {
		// We created this cache, so we must delete it again. Note, that we
		// need to temporarily unlock the source cache or we'll otherwise
		// deadlock, since VMCache::_RemoveConsumer() will try to lock it, too.
		sourceCache->Unlock();
		cache->ReleaseRefAndUnlock();
		sourceCache->Lock();
	}
err1:
	addressSpace->DeleteArea(area, allocationFlags);
	return status;
}


/*!	Equivalent to wait_if_area_range_is_wired(area, area->Base(), area->Size(),
	  locker1, locker2).
*/
template<typename LockerType1, typename LockerType2>
static inline bool
wait_if_area_is_wired(VMArea* area, LockerType1* locker1, LockerType2* locker2)
{
	area->cache->AssertLocked();

	VMAreaUnwiredWaiter waiter;
	if (!area->AddWaiterIfWired(&waiter))
		return false;

	// unlock everything and wait
	if (locker1 != NULL)
		locker1->Unlock();
	if (locker2 != NULL)
		locker2->Unlock();

	waiter.waitEntry.Wait();

	return true;
}


/*!	Checks whether the given area has any wired ranges intersecting with the
	specified range and waits, if so.

	When it has to wait, the function calls \c Unlock() on both \a locker1
	and \a locker2, if given.
	The area's top cache must be locked and must be unlocked as a side effect
	of calling \c Unlock() on either \a locker1 or \a locker2.

	If the function does not have to wait it does not modify or unlock any
	object.

	\param area The area to be checked.
	\param base The base address of the range to check.
	\param size The size of the address range to check.
	\param locker1 An object to be unlocked when before starting to wait (may
		be \c NULL).
	\param locker2 An object to be unlocked when before starting to wait (may
		be \c NULL).
	\return \c true, if the function had to wait, \c false otherwise.
*/
template<typename LockerType1, typename LockerType2>
static inline bool
wait_if_area_range_is_wired(VMArea* area, addr_t base, size_t size,
	LockerType1* locker1, LockerType2* locker2)
{
	area->cache->AssertLocked();

	VMAreaUnwiredWaiter waiter;
	if (!area->AddWaiterIfWired(&waiter, base, size))
		return false;

	// unlock everything and wait
	if (locker1 != NULL)
		locker1->Unlock();
	if (locker2 != NULL)
		locker2->Unlock();

	waiter.waitEntry.Wait();

	return true;
}


/*!	Checks whether the given address space has any wired ranges intersecting
	with the specified range and waits, if so.

	Similar to wait_if_area_range_is_wired(), with the following differences:
	- All areas intersecting with the range are checked (respectively all until
	  one is found that contains a wired range intersecting with the given
	  range).
	- The given address space must at least be read-locked and must be unlocked
	  when \c Unlock() is called on \a locker.
	- None of the areas' caches are allowed to be locked.
*/
template<typename LockerType>
static inline bool
wait_if_address_range_is_wired(VMAddressSpace* addressSpace, addr_t base,
	size_t size, LockerType* locker)
{
	VMAddressSpace::AreaRangeIterator it = addressSpace->GetAreaRangeIterator(base, size);
	while (VMArea* area = it.Next()) {
		AreaCacheLocker cacheLocker(area);
		if (wait_if_area_range_is_wired(area, base, size, locker, &cacheLocker))
			return true;
	}

	return false;
}


/*!	Prepares an area to be used for vm_set_kernel_area_debug_protection().
	It must be called in a situation where the kernel address space may be
	locked.
*/
status_t
vm_prepare_kernel_area_debug_protection(area_id id, void** cookie)
{
	AddressSpaceReadLocker locker;
	VMArea* area;
	status_t status = locker.SetFromArea(id, area);
	if (status != B_OK)
		return status;

	if (area->page_protections == NULL) {
		status = allocate_area_page_protections(area);
		if (status != B_OK)
			return status;
	}

	*cookie = (void*)area;
	return B_OK;
}


/*!	This is a debug helper function that can only be used with very specific
	use cases.
	Sets protection for the given address range to the protection specified.
	If \a protection is 0 then the involved pages will be marked non-present
	in the translation map to cause a fault on access. The pages aren't
	actually unmapped however so that they can be marked present again with
	additional calls to this function. For this to work the area must be
	fully locked in memory so that the pages aren't otherwise touched.
	This function does not lock the kernel address space and needs to be
	supplied with a \a cookie retrieved from a successful call to
	vm_prepare_kernel_area_debug_protection().
*/
status_t
vm_set_kernel_area_debug_protection(void* cookie, void* _address, size_t size,
	uint32 protection)
{
	// check address range
	addr_t address = (addr_t)_address;
	size = PAGE_ALIGN(size);

	if ((address % B_PAGE_SIZE) != 0
		|| (addr_t)address + size < (addr_t)address
		|| !IS_KERNEL_ADDRESS(address)
		|| !IS_KERNEL_ADDRESS((addr_t)address + size)) {
		return B_BAD_VALUE;
	}

	// Translate the kernel protection to user protection as we only store that.
	if ((protection & B_KERNEL_READ_AREA) != 0)
		protection |= B_READ_AREA;
	if ((protection & B_KERNEL_WRITE_AREA) != 0)
		protection |= B_WRITE_AREA;

	VMAddressSpace* addressSpace = VMAddressSpace::GetKernel();
	VMTranslationMap* map = addressSpace->TranslationMap();
	VMArea* area = (VMArea*)cookie;

	addr_t offset = address - area->Base();
	if (area->Size() - offset < size) {
		panic("protect range not fully within supplied area");
		return B_BAD_VALUE;
	}

	if (area->page_protections == NULL) {
		panic("area has no page protections");
		return B_BAD_VALUE;
	}

	// Invalidate the mapping entries so any access to them will fault or
	// restore the mapping entries unchanged so that lookup will success again.
	map->Lock();
	map->DebugMarkRangePresent(address, address + size, protection != 0);
	map->Unlock();

	// And set the proper page protections so that the fault case will actually
	// fail and not simply try to map a new page.
	for (addr_t pageAddress = address; pageAddress < address + size;
			pageAddress += B_PAGE_SIZE) {
		set_area_page_protection(area, pageAddress, protection);
	}

	return B_OK;
}


status_t
vm_block_address_range(const char* name, void* address, addr_t size)
{
	AddressSpaceWriteLocker locker;
	status_t status = locker.SetTo(VMAddressSpace::KernelID());
	if (status != B_OK)
		return status;

	VMAddressSpace* addressSpace = locker.AddressSpace();

	VMCache* cache;
	status = VMCacheFactory::CreateNullCache(VM_PRIORITY_SYSTEM, cache);
	if (status != B_OK)
		return status;

	cache->temporary = 1;
	cache->virtual_end = size;
	cache->Lock();

	VMArea* area;
	virtual_address_restrictions addressRestrictions = {};
	addressRestrictions.address = address;
	addressRestrictions.address_specification = B_EXACT_ADDRESS;
	status = map_backing_store(addressSpace, cache, 0, name, size,
		B_NO_LOCK, 0, REGION_NO_PRIVATE_MAP, 0, CREATE_AREA_DONT_COMMIT_MEMORY,
		&addressRestrictions, true, &area, NULL);
	if (status != B_OK) {
		cache->ReleaseRefAndUnlock();
		return status;
	}

	cache->Unlock();
	area->cache_type = CACHE_TYPE_NULL;
	return area->id;
}


status_t
vm_unreserve_address_range(team_id team, void* address, addr_t size)
{
	AddressSpaceWriteLocker locker(team);
	if (!locker.IsLocked())
		return B_BAD_TEAM_ID;

	VMAddressSpace* addressSpace = locker.AddressSpace();
	return addressSpace->UnreserveAddressRange((addr_t)address, size,
		addressSpace == VMAddressSpace::Kernel()
			? HEAP_DONT_WAIT_FOR_MEMORY | HEAP_DONT_LOCK_KERNEL_SPACE : 0);
}


status_t
vm_reserve_address_range(team_id team, void** _address, uint32 addressSpec,
	addr_t size, uint32 flags)
{
	if (size == 0)
		return B_BAD_VALUE;

	AddressSpaceWriteLocker locker(team);
	if (!locker.IsLocked())
		return B_BAD_TEAM_ID;

	virtual_address_restrictions addressRestrictions = {};
	addressRestrictions.address = *_address;
	addressRestrictions.address_specification = addressSpec;
	VMAddressSpace* addressSpace = locker.AddressSpace();
	return addressSpace->ReserveAddressRange(size, &addressRestrictions, flags,
		addressSpace == VMAddressSpace::Kernel()
			? HEAP_DONT_WAIT_FOR_MEMORY | HEAP_DONT_LOCK_KERNEL_SPACE : 0,
		_address);
}


area_id
vm_create_anonymous_area(team_id team, const char *name, addr_t size,
	uint32 wiring, uint32 protection, uint32 flags, addr_t guardSize,
	const virtual_address_restrictions* virtualAddressRestrictions,
	const physical_address_restrictions* physicalAddressRestrictions,
	bool kernel, void** _address)
{
	VMArea* area;
	VMCache* cache;
	vm_page* page = NULL;
	bool isStack = (protection & B_STACK_AREA) != 0;
	page_num_t guardPages;
	bool canOvercommit = false;
	uint32 pageAllocFlags = (flags & CREATE_AREA_DONT_CLEAR) == 0
		? VM_PAGE_ALLOC_CLEAR : 0;

	TRACE(("create_anonymous_area [%" B_PRId32 "] %s: size 0x%" B_PRIxADDR "\n",
		team, name, size));

	size = PAGE_ALIGN(size);
	guardSize = PAGE_ALIGN(guardSize);
	guardPages = guardSize / B_PAGE_SIZE;

	if (size == 0 || size < guardSize)
		return B_BAD_VALUE;
	if (!arch_vm_supports_protection(protection))
		return B_NOT_SUPPORTED;

	if (team == B_CURRENT_TEAM)
		team = VMAddressSpace::CurrentID();
	if (team < 0)
		return B_BAD_TEAM_ID;

	if (isStack || (protection & B_OVERCOMMITTING_AREA) != 0)
		canOvercommit = true;

#ifdef DEBUG_KERNEL_STACKS
	if ((protection & B_KERNEL_STACK_AREA) != 0)
		isStack = true;
#endif

	// check parameters
	switch (virtualAddressRestrictions->address_specification) {
		case B_ANY_ADDRESS:
		case B_EXACT_ADDRESS:
		case B_BASE_ADDRESS:
		case B_ANY_KERNEL_ADDRESS:
		case B_ANY_KERNEL_BLOCK_ADDRESS:
		case B_RANDOMIZED_ANY_ADDRESS:
		case B_RANDOMIZED_BASE_ADDRESS:
			break;

		default:
			return B_BAD_VALUE;
	}

	// If low or high physical address restrictions are given, we force
	// B_CONTIGUOUS wiring, since only then we'll use
	// vm_page_allocate_page_run() which deals with those restrictions.
	if (physicalAddressRestrictions->low_address != 0
		|| physicalAddressRestrictions->high_address != 0) {
		wiring = B_CONTIGUOUS;
	}

	physical_address_restrictions stackPhysicalRestrictions;
	bool doReserveMemory = false;
	addr_t reservedMemory = 0;
	switch (wiring) {
		case B_NO_LOCK:
			break;
		case B_FULL_LOCK:
		case B_LAZY_LOCK:
		case B_CONTIGUOUS:
			doReserveMemory = true;
			break;
		case B_LOMEM:
			stackPhysicalRestrictions = *physicalAddressRestrictions;
			stackPhysicalRestrictions.high_address = 16 * 1024 * 1024;
			physicalAddressRestrictions = &stackPhysicalRestrictions;
			wiring = B_CONTIGUOUS;
			doReserveMemory = true;
			break;
		case B_32_BIT_FULL_LOCK:
			if (B_HAIKU_PHYSICAL_BITS <= 32
				|| (uint64)vm_page_max_address() < (uint64)1 << 32) {
				wiring = B_FULL_LOCK;
				doReserveMemory = true;
				break;
			}
			// TODO: We don't really support this mode efficiently. Just fall
			// through for now ...
		case B_32_BIT_CONTIGUOUS:
#if B_HAIKU_PHYSICAL_BITS > 32
				if (vm_page_max_address() >= (phys_addr_t)1 << 32) {
					stackPhysicalRestrictions = *physicalAddressRestrictions;
					stackPhysicalRestrictions.high_address
						= (phys_addr_t)1 << 32;
					physicalAddressRestrictions = &stackPhysicalRestrictions;
				}
#endif
			wiring = B_CONTIGUOUS;
			doReserveMemory = true;
			break;
		case B_ALREADY_WIRED:
			ASSERT(gKernelStartup);
			// The used memory will already be accounted for.
			reservedMemory = size;
			break;
		default:
			return B_BAD_VALUE;
	}

	// Optimization: For a single-page contiguous allocation without low/high
	// memory restriction B_FULL_LOCK wiring suffices.
	if (wiring == B_CONTIGUOUS && size == B_PAGE_SIZE
		&& physicalAddressRestrictions->low_address == 0
		&& physicalAddressRestrictions->high_address == 0) {
		wiring = B_FULL_LOCK;
	}

	// For full lock or contiguous areas we're also going to map the pages and
	// thus need to reserve pages for the mapping backend upfront.
	addr_t reservedMapPages = 0;
	if (wiring == B_FULL_LOCK || wiring == B_CONTIGUOUS) {
		AddressSpaceWriteLocker locker;
		status_t status = locker.SetTo(team);
		if (status != B_OK)
			return status;

		VMTranslationMap* map = locker.AddressSpace()->TranslationMap();
		reservedMapPages = map->MaxPagesNeededToMap(0, size - 1);
	}

	int priority;
	if (team != VMAddressSpace::KernelID())
		priority = VM_PRIORITY_USER;
	else if ((flags & CREATE_AREA_PRIORITY_VIP) != 0)
		priority = VM_PRIORITY_VIP;
	else
		priority = VM_PRIORITY_SYSTEM;

	// Reserve memory before acquiring the address space lock. This reduces the
	// chances of failure, since while holding the write lock to the address
	// space (if it is the kernel address space that is), the low memory handler
	// won't be able to free anything for us.
	if (doReserveMemory) {
		bigtime_t timeout = (flags & CREATE_AREA_DONT_WAIT) != 0 ? 0 : 1000000;
		if (vm_try_reserve_memory(size, priority, timeout) != B_OK)
			return B_NO_MEMORY;
		reservedMemory = size;
		// TODO: We don't reserve the memory for the pages for the page
		// directories/tables. We actually need to do since we currently don't
		// reclaim them (and probably can't reclaim all of them anyway). Thus
		// there are actually less physical pages than there should be, which
		// can get the VM into trouble in low memory situations.
	}

	AddressSpaceWriteLocker locker;
	VMAddressSpace* addressSpace;
	status_t status;

	// For full lock areas reserve the pages before locking the address
	// space. E.g. block caches can't release their memory while we hold the
	// address space lock.
	page_num_t reservedPages = reservedMapPages;
	if (wiring == B_FULL_LOCK)
		reservedPages += size / B_PAGE_SIZE;

	vm_page_reservation reservation;
	if (reservedPages > 0) {
		if ((flags & CREATE_AREA_DONT_WAIT) != 0) {
			if (!vm_page_try_reserve_pages(&reservation, reservedPages,
					priority)) {
				reservedPages = 0;
				status = B_WOULD_BLOCK;
				goto err0;
			}
		} else
			vm_page_reserve_pages(&reservation, reservedPages, priority);
	}

	if (wiring == B_CONTIGUOUS) {
		// we try to allocate the page run here upfront as this may easily
		// fail for obvious reasons
		page = vm_page_allocate_page_run(PAGE_STATE_WIRED | pageAllocFlags,
			size / B_PAGE_SIZE, physicalAddressRestrictions, priority);
		if (page == NULL) {
			status = B_NO_MEMORY;
			goto err0;
		}
	}

	// Lock the address space and, if B_EXACT_ADDRESS and
	// CREATE_AREA_UNMAP_ADDRESS_RANGE were specified, ensure the address range
	// is not wired.
	do {
		status = locker.SetTo(team);
		if (status != B_OK)
			goto err1;

		addressSpace = locker.AddressSpace();
	} while (virtualAddressRestrictions->address_specification
			== B_EXACT_ADDRESS
		&& (flags & CREATE_AREA_UNMAP_ADDRESS_RANGE) != 0
		&& wait_if_address_range_is_wired(addressSpace,
			(addr_t)virtualAddressRestrictions->address, size, &locker));

	// create an anonymous cache
	// if it's a stack, make sure that two pages are available at least
	status = VMCacheFactory::CreateAnonymousCache(cache, canOvercommit,
		isStack ? (min_c(2, size / B_PAGE_SIZE - guardPages)) : 0, guardPages,
		wiring == B_NO_LOCK, priority);
	if (status != B_OK)
		goto err1;

	cache->temporary = 1;
	cache->virtual_end = size;
	cache->committed_size = reservedMemory;
		// TODO: This should be done via a method.
	reservedMemory = 0;

	cache->Lock();

	status = map_backing_store(addressSpace, cache, 0, name, size, wiring,
		protection, 0, REGION_NO_PRIVATE_MAP, flags,
		virtualAddressRestrictions, kernel, &area, _address);

	if (status != B_OK) {
		cache->ReleaseRefAndUnlock();
		goto err1;
	}

	locker.DegradeToReadLock();

	switch (wiring) {
		case B_NO_LOCK:
		case B_LAZY_LOCK:
			// do nothing - the pages are mapped in as needed
			break;

		case B_FULL_LOCK:
		{
			// Allocate and map all pages for this area

			off_t offset = 0;
			for (addr_t address = area->Base();
					address < area->Base() + (area->Size() - 1);
					address += B_PAGE_SIZE, offset += B_PAGE_SIZE) {
#ifdef DEBUG_KERNEL_STACKS
#	ifdef STACK_GROWS_DOWNWARDS
				if (isStack && address < area->Base()
						+ KERNEL_STACK_GUARD_PAGES * B_PAGE_SIZE)
#	else
				if (isStack && address >= area->Base() + area->Size()
						- KERNEL_STACK_GUARD_PAGES * B_PAGE_SIZE)
#	endif
					continue;
#endif
				vm_page* page = vm_page_allocate_page(&reservation,
					PAGE_STATE_WIRED | pageAllocFlags);
				cache->InsertPage(page, offset);
				map_page(area, page, address, protection, &reservation);

				DEBUG_PAGE_ACCESS_END(page);
			}

			break;
		}

		case B_ALREADY_WIRED:
		{
			// The pages should already be mapped. This is only really useful
			// during boot time. Find the appropriate vm_page objects and stick
			// them in the cache object.
			VMTranslationMap* map = addressSpace->TranslationMap();
			off_t offset = 0;

			if (!gKernelStartup)
				panic("ALREADY_WIRED flag used outside kernel startup\n");

			map->Lock();

			for (addr_t virtualAddress = area->Base();
					virtualAddress < area->Base() + (area->Size() - 1);
					virtualAddress += B_PAGE_SIZE, offset += B_PAGE_SIZE) {
				phys_addr_t physicalAddress;
				uint32 flags;
				status = map->Query(virtualAddress, &physicalAddress, &flags);
				if (status < B_OK) {
					panic("looking up mapping failed for va 0x%lx\n",
						virtualAddress);
				}
				page = vm_lookup_page(physicalAddress / B_PAGE_SIZE);
				if (page == NULL) {
					panic("looking up page failed for pa %#" B_PRIxPHYSADDR
						"\n", physicalAddress);
				}

				DEBUG_PAGE_ACCESS_START(page);

				cache->InsertPage(page, offset);
				increment_page_wired_count(page);
				vm_page_set_state(page, PAGE_STATE_WIRED);
				page->busy = false;

				DEBUG_PAGE_ACCESS_END(page);
			}

			map->Unlock();
			break;
		}

		case B_CONTIGUOUS:
		{
			// We have already allocated our continuous pages run, so we can now
			// just map them in the address space
			VMTranslationMap* map = addressSpace->TranslationMap();
			phys_addr_t physicalAddress
				= (phys_addr_t)page->physical_page_number * B_PAGE_SIZE;
			addr_t virtualAddress = area->Base();
			off_t offset = 0;

			map->Lock();

			for (virtualAddress = area->Base(); virtualAddress < area->Base()
					+ (area->Size() - 1); virtualAddress += B_PAGE_SIZE,
					offset += B_PAGE_SIZE, physicalAddress += B_PAGE_SIZE) {
				page = vm_lookup_page(physicalAddress / B_PAGE_SIZE);
				if (page == NULL)
					panic("couldn't lookup physical page just allocated\n");

				status = map->Map(virtualAddress, physicalAddress, protection,
					area->MemoryType(), &reservation);
				if (status < B_OK)
					panic("couldn't map physical page in page run\n");

				cache->InsertPage(page, offset);
				increment_page_wired_count(page);

				DEBUG_PAGE_ACCESS_END(page);
			}

			map->Unlock();
			break;
		}

		default:
			break;
	}

	cache->Unlock();

	if (reservedPages > 0)
		vm_page_unreserve_pages(&reservation);

	TRACE(("vm_create_anonymous_area: done\n"));

	area->cache_type = CACHE_TYPE_RAM;
	return area->id;

err1:
	if (wiring == B_CONTIGUOUS) {
		// we had reserved the area space upfront...
		phys_addr_t pageNumber = page->physical_page_number;
		int32 i;
		for (i = size / B_PAGE_SIZE; i-- > 0; pageNumber++) {
			page = vm_lookup_page(pageNumber);
			if (page == NULL)
				panic("couldn't lookup physical page just allocated\n");

			vm_page_free(NULL, page);
		}
	}

err0:
	if (reservedPages > 0)
		vm_page_unreserve_pages(&reservation);
	if (reservedMemory > 0)
		vm_unreserve_memory(reservedMemory);

	return status;
}


area_id
vm_map_physical_memory(team_id team, const char* name, void** _address,
	uint32 addressSpec, addr_t size, uint32 protection,
	phys_addr_t physicalAddress, bool alreadyWired)
{
	VMArea* area;
	VMCache* cache;
	addr_t mapOffset;

	TRACE(("vm_map_physical_memory(aspace = %" B_PRId32 ", \"%s\", virtual = %p"
		", spec = %" B_PRIu32 ", size = %" B_PRIxADDR ", protection = %"
		B_PRIu32 ", phys = %#" B_PRIxPHYSADDR ")\n", team, name, *_address,
		addressSpec, size, protection, physicalAddress));

	if (!arch_vm_supports_protection(protection))
		return B_NOT_SUPPORTED;

	AddressSpaceWriteLocker locker(team);
	if (!locker.IsLocked())
		return B_BAD_TEAM_ID;

	// if the physical address is somewhat inside a page,
	// move the actual area down to align on a page boundary
	mapOffset = physicalAddress % B_PAGE_SIZE;
	size += mapOffset;
	physicalAddress -= mapOffset;

	size = PAGE_ALIGN(size);

	// create a device cache
	status_t status = VMCacheFactory::CreateDeviceCache(cache, physicalAddress);
	if (status != B_OK)
		return status;

	cache->virtual_end = size;

	cache->Lock();

	virtual_address_restrictions addressRestrictions = {};
	addressRestrictions.address = *_address;
	addressRestrictions.address_specification = addressSpec & ~B_MEMORY_TYPE_MASK;
	status = map_backing_store(locker.AddressSpace(), cache, 0, name, size,
		B_FULL_LOCK, protection, 0, REGION_NO_PRIVATE_MAP, CREATE_AREA_DONT_COMMIT_MEMORY,
		&addressRestrictions, true, &area, _address);

	if (status < B_OK)
		cache->ReleaseRefLocked();

	cache->Unlock();

	if (status == B_OK) {
		// Set requested memory type -- default to uncached, but allow
		// that to be overridden by ranges that may already exist.
		uint32 memoryType = addressSpec & B_MEMORY_TYPE_MASK;
		const bool weak = (memoryType == 0);
		if (weak)
			memoryType = B_UNCACHED_MEMORY;

		status = arch_vm_set_memory_type(area, physicalAddress, memoryType,
			weak ? &memoryType : NULL);

		area->SetMemoryType(memoryType);

		if (status != B_OK)
			delete_area(locker.AddressSpace(), area, false);
	}

	if (status != B_OK)
		return status;

	VMTranslationMap* map = locker.AddressSpace()->TranslationMap();

	if (alreadyWired) {
		// The area is already mapped, but possibly not with the right
		// memory type.
		map->Lock();
		map->ProtectArea(area, area->protection);
		map->Unlock();
	} else {
		// Map the area completely.

		// reserve pages needed for the mapping
		size_t reservePages = map->MaxPagesNeededToMap(area->Base(),
			area->Base() + (size - 1));
		vm_page_reservation reservation;
		vm_page_reserve_pages(&reservation, reservePages,
			team == VMAddressSpace::KernelID()
				? VM_PRIORITY_SYSTEM : VM_PRIORITY_USER);

		map->Lock();

		for (addr_t offset = 0; offset < size; offset += B_PAGE_SIZE) {
			map->Map(area->Base() + offset, physicalAddress + offset,
				protection, area->MemoryType(), &reservation);
		}

		map->Unlock();

		vm_page_unreserve_pages(&reservation);
	}

	// modify the pointer returned to be offset back into the new area
	// the same way the physical address in was offset
	*_address = (void*)((addr_t)*_address + mapOffset);

	area->cache_type = CACHE_TYPE_DEVICE;
	return area->id;
}


/*!	Don't use!
	TODO: This function was introduced to map physical page vecs to
	contiguous virtual memory in IOBuffer::GetNextVirtualVec(). It does
	use a device cache and does not track vm_page::wired_count!
*/
area_id
vm_map_physical_memory_vecs(team_id team, const char* name, void** _address,
	uint32 addressSpec, addr_t* _size, uint32 protection,
	struct generic_io_vec* vecs, uint32 vecCount)
{
	TRACE(("vm_map_physical_memory_vecs(team = %" B_PRId32 ", \"%s\", virtual "
		"= %p, spec = %" B_PRIu32 ", _size = %p, protection = %" B_PRIu32 ", "
		"vecs = %p, vecCount = %" B_PRIu32 ")\n", team, name, *_address,
		addressSpec, _size, protection, vecs, vecCount));

	if (!arch_vm_supports_protection(protection)
		|| (addressSpec & B_MEMORY_TYPE_MASK) != 0) {
		return B_NOT_SUPPORTED;
	}

	AddressSpaceWriteLocker locker(team);
	if (!locker.IsLocked())
		return B_BAD_TEAM_ID;

	if (vecCount == 0)
		return B_BAD_VALUE;

	addr_t size = 0;
	for (uint32 i = 0; i < vecCount; i++) {
		if (vecs[i].base % B_PAGE_SIZE != 0
			|| vecs[i].length % B_PAGE_SIZE != 0) {
			return B_BAD_VALUE;
		}

		size += vecs[i].length;
	}

	// create a device cache
	VMCache* cache;
	status_t result = VMCacheFactory::CreateDeviceCache(cache, vecs[0].base);
	if (result != B_OK)
		return result;

	cache->virtual_end = size;

	cache->Lock();

	VMArea* area;
	virtual_address_restrictions addressRestrictions = {};
	addressRestrictions.address = *_address;
	addressRestrictions.address_specification = addressSpec & ~B_MEMORY_TYPE_MASK;
	result = map_backing_store(locker.AddressSpace(), cache, 0, name, size,
		B_FULL_LOCK, protection, 0, REGION_NO_PRIVATE_MAP, CREATE_AREA_DONT_COMMIT_MEMORY,
		&addressRestrictions, true, &area, _address);

	if (result != B_OK)
		cache->ReleaseRefLocked();

	cache->Unlock();

	if (result != B_OK)
		return result;

	VMTranslationMap* map = locker.AddressSpace()->TranslationMap();
	size_t reservePages = map->MaxPagesNeededToMap(area->Base(),
		area->Base() + (size - 1));

	vm_page_reservation reservation;
	vm_page_reserve_pages(&reservation, reservePages,
			team == VMAddressSpace::KernelID()
				? VM_PRIORITY_SYSTEM : VM_PRIORITY_USER);
	map->Lock();

	uint32 vecIndex = 0;
	size_t vecOffset = 0;
	for (addr_t offset = 0; offset < size; offset += B_PAGE_SIZE) {
		while (vecOffset >= vecs[vecIndex].length && vecIndex < vecCount) {
			vecOffset = 0;
			vecIndex++;
		}

		if (vecIndex >= vecCount)
			break;

		map->Map(area->Base() + offset, vecs[vecIndex].base + vecOffset,
			protection, area->MemoryType(), &reservation);

		vecOffset += B_PAGE_SIZE;
	}

	map->Unlock();
	vm_page_unreserve_pages(&reservation);

	if (_size != NULL)
		*_size = size;

	area->cache_type = CACHE_TYPE_DEVICE;
	return area->id;
}


area_id
vm_create_null_area(team_id team, const char* name, void** address,
	uint32 addressSpec, addr_t size, uint32 flags)
{
	size = PAGE_ALIGN(size);

	// Lock the address space and, if B_EXACT_ADDRESS and
	// CREATE_AREA_UNMAP_ADDRESS_RANGE were specified, ensure the address range
	// is not wired.
	AddressSpaceWriteLocker locker;
	do {
		if (locker.SetTo(team) != B_OK)
			return B_BAD_TEAM_ID;
	} while (addressSpec == B_EXACT_ADDRESS
		&& (flags & CREATE_AREA_UNMAP_ADDRESS_RANGE) != 0
		&& wait_if_address_range_is_wired(locker.AddressSpace(),
			(addr_t)*address, size, &locker));

	// create a null cache
	int priority = (flags & CREATE_AREA_PRIORITY_VIP) != 0
		? VM_PRIORITY_VIP : VM_PRIORITY_SYSTEM;
	VMCache* cache;
	status_t status = VMCacheFactory::CreateNullCache(priority, cache);
	if (status != B_OK)
		return status;

	cache->temporary = 1;
	cache->virtual_end = size;

	cache->Lock();

	VMArea* area;
	virtual_address_restrictions addressRestrictions = {};
	addressRestrictions.address = *address;
	addressRestrictions.address_specification = addressSpec;
	status = map_backing_store(locker.AddressSpace(), cache, 0, name, size,
		B_LAZY_LOCK, B_KERNEL_READ_AREA, B_KERNEL_READ_AREA,
		REGION_NO_PRIVATE_MAP, flags | CREATE_AREA_DONT_COMMIT_MEMORY,
		&addressRestrictions, true, &area, address);

	if (status < B_OK) {
		cache->ReleaseRefAndUnlock();
		return status;
	}

	cache->Unlock();

	area->cache_type = CACHE_TYPE_NULL;
	return area->id;
}


/*!	Creates the vnode cache for the specified \a vnode.
	The vnode has to be marked busy when calling this function.
*/
status_t
vm_create_vnode_cache(struct vnode* vnode, struct VMCache** cache)
{
	return VMCacheFactory::CreateVnodeCache(*cache, vnode);
}


/*!	\a cache must be locked. The area's address space must be read-locked.
*/
static void
pre_map_area_pages(VMArea* area, VMCache* cache,
	vm_page_reservation* reservation, int32 maxCount)
{
	addr_t baseAddress = area->Base();
	addr_t cacheOffset = area->cache_offset;
	page_num_t firstPage = cacheOffset / B_PAGE_SIZE;
	page_num_t endPage = firstPage + area->Size() / B_PAGE_SIZE;

	VMCachePagesTree::Iterator it = cache->pages.GetIterator(firstPage, true, true);
	vm_page* page;
	while ((page = it.Next()) != NULL && maxCount > 0) {
		if (page->cache_offset >= endPage)
			break;

		// skip busy and inactive pages
		if (page->busy || (page->usage_count == 0 && !page->accessed))
			continue;

		DEBUG_PAGE_ACCESS_START(page);
		map_page(area, page,
			baseAddress + (page->cache_offset * B_PAGE_SIZE - cacheOffset),
			B_READ_AREA | B_KERNEL_READ_AREA, reservation);
		maxCount--;
		DEBUG_PAGE_ACCESS_END(page);
	}
}


/*!	Will map the file specified by \a fd to an area in memory.
	The file will be mirrored beginning at the specified \a offset. The
	\a offset and \a size arguments have to be page aligned.
*/
static area_id
_vm_map_file(team_id team, const char* name, void** _address,
	uint32 addressSpec, size_t size, uint32 protection, uint32 mapping,
	bool unmapAddressRange, int fd, off_t offset, bool kernel)
{
	// TODO: for binary files, we want to make sure that they get the
	//	copy of a file at a given time, ie. later changes should not
	//	make it into the mapped copy -- this will need quite some changes
	//	to be done in a nice way
	TRACE(("_vm_map_file(fd = %d, offset = %" B_PRIdOFF ", size = %lu, mapping "
		"%" B_PRIu32 ")\n", fd, offset, size, mapping));

	if ((offset % B_PAGE_SIZE) != 0)
		return B_BAD_VALUE;
	size = PAGE_ALIGN(size);

	if (mapping == REGION_NO_PRIVATE_MAP)
		protection |= B_SHARED_AREA;
	if (addressSpec != B_EXACT_ADDRESS)
		unmapAddressRange = false;

	uint32 mappingFlags = 0;
	if (unmapAddressRange)
		mappingFlags |= CREATE_AREA_UNMAP_ADDRESS_RANGE;
	if (mapping == REGION_PRIVATE_MAP) {
		// For privately mapped read-only regions, skip committing memory.
		// (If protections are changed later on, memory will be committed then.)
		if ((protection & (B_WRITE_AREA | B_KERNEL_WRITE_AREA)) == 0)
			mappingFlags |= CREATE_AREA_DONT_COMMIT_MEMORY;
	}

	if (fd < 0) {
		virtual_address_restrictions virtualRestrictions = {};
		virtualRestrictions.address = *_address;
		virtualRestrictions.address_specification = addressSpec;
		physical_address_restrictions physicalRestrictions = {};
		return vm_create_anonymous_area(team, name, size, B_NO_LOCK, protection,
			mappingFlags, 0, &virtualRestrictions, &physicalRestrictions, kernel,
			_address);
	}

	// get the open flags of the FD
	file_descriptor* descriptor = get_fd(get_current_io_context(kernel), fd);
	if (descriptor == NULL)
		return EBADF;
	int32 openMode = descriptor->open_mode;
	put_fd(descriptor);

	// The FD must open for reading at any rate. For shared mapping with write
	// access, additionally the FD must be open for writing.
	if ((openMode & O_ACCMODE) == O_WRONLY
		|| (mapping == REGION_NO_PRIVATE_MAP
			&& (protection & (B_WRITE_AREA | B_KERNEL_WRITE_AREA)) != 0
			&& (openMode & O_ACCMODE) == O_RDONLY)) {
		return EACCES;
	}

	uint32 protectionMax = 0;
	if (mapping == REGION_NO_PRIVATE_MAP) {
		if ((openMode & O_ACCMODE) == O_RDWR)
			protectionMax = protection | B_USER_PROTECTION;
		else
			protectionMax = protection | (B_USER_PROTECTION & ~B_WRITE_AREA);
	}

	// get the vnode for the object, this also grabs a ref to it
	struct vnode* vnode = NULL;
	status_t status = vfs_get_vnode_from_fd(fd, kernel, &vnode);
	if (status < B_OK)
		return status;
	VnodePutter vnodePutter(vnode);

	// If we're going to pre-map pages, we need to reserve the pages needed by
	// the mapping backend upfront.
	page_num_t reservedPreMapPages = 0;
	vm_page_reservation reservation;
	if ((protection & B_READ_AREA) != 0) {
		AddressSpaceWriteLocker locker;
		status = locker.SetTo(team);
		if (status != B_OK)
			return status;

		VMTranslationMap* map = locker.AddressSpace()->TranslationMap();
		reservedPreMapPages = map->MaxPagesNeededToMap(0, size - 1);

		locker.Unlock();

		vm_page_reserve_pages(&reservation, reservedPreMapPages,
			team == VMAddressSpace::KernelID()
				? VM_PRIORITY_SYSTEM : VM_PRIORITY_USER);
	}

	struct PageUnreserver {
		PageUnreserver(vm_page_reservation* reservation)
			:
			fReservation(reservation)
		{
		}

		~PageUnreserver()
		{
			if (fReservation != NULL)
				vm_page_unreserve_pages(fReservation);
		}

		vm_page_reservation* fReservation;
	} pageUnreserver(reservedPreMapPages > 0 ? &reservation : NULL);

	// Lock the address space and, if the specified address range shall be
	// unmapped, ensure it is not wired.
	AddressSpaceWriteLocker locker;
	do {
		if (locker.SetTo(team) != B_OK)
			return B_BAD_TEAM_ID;
	} while (unmapAddressRange
		&& wait_if_address_range_is_wired(locker.AddressSpace(),
			(addr_t)*_address, size, &locker));

	// TODO: this only works for file systems that use the file cache
	VMCache* cache;
	status = vfs_get_vnode_cache(vnode, &cache, false);
	if (status < B_OK)
		return status;

	cache->Lock();

	if (mapping != REGION_PRIVATE_MAP && (cache->virtual_base > offset
			|| PAGE_ALIGN(cache->virtual_end) < (off_t)(offset + size))) {
		cache->ReleaseRefAndUnlock();
		return B_BAD_VALUE;
	}

	VMArea* area;
	virtual_address_restrictions addressRestrictions = {};
	addressRestrictions.address = *_address;
	addressRestrictions.address_specification = addressSpec;
	status = map_backing_store(locker.AddressSpace(), cache, offset, name, size,
		0, protection, protectionMax, mapping, mappingFlags,
		&addressRestrictions, kernel, &area, _address);

	if (status != B_OK || mapping == REGION_PRIVATE_MAP) {
		// map_backing_store() cannot know we no longer need the ref
		cache->ReleaseRefLocked();
	}

	if (status == B_OK && (protection & B_READ_AREA) != 0 && cache->page_count > 0) {
		// Pre-map up to 1 MB for every time the cache has been faulted "in full".
		pre_map_area_pages(area, cache, &reservation,
			(cache->FaultCount() / cache->page_count)
				* ((1 * 1024 * 1024) / B_PAGE_SIZE));
	}

	cache->Unlock();

	if (status == B_OK) {
		// TODO: this probably deserves a smarter solution, e.g. probably
		// trigger prefetch somewhere else.

		// Prefetch at most 10MB starting from "offset", but only if the cache
		// doesn't already contain more pages than the prefetch size.
		const size_t prefetch = min_c(size, 10LL * 1024 * 1024);
		if (cache->page_count < (prefetch / B_PAGE_SIZE))
			cache_prefetch_vnode(vnode, offset, prefetch);
	}

	if (status != B_OK)
		return status;

	area->cache_type = CACHE_TYPE_VNODE;
	return area->id;
}


area_id
vm_map_file(team_id aid, const char* name, void** address, uint32 addressSpec,
	addr_t size, uint32 protection, uint32 mapping, bool unmapAddressRange,
	int fd, off_t offset)
{
	if (!arch_vm_supports_protection(protection))
		return B_NOT_SUPPORTED;

	return _vm_map_file(aid, name, address, addressSpec, size, protection,
		mapping, unmapAddressRange, fd, offset, true);
}


VMCache*
vm_area_get_locked_cache(VMArea* area)
{
	rw_lock_read_lock(&sAreaCacheLock);

	while (true) {
		VMCache* cache = area->cache;

		if (!cache->SwitchFromReadLock(&sAreaCacheLock)) {
			// cache has been deleted
			rw_lock_read_lock(&sAreaCacheLock);
			continue;
		}

		rw_lock_read_lock(&sAreaCacheLock);

		if (cache == area->cache) {
			cache->AcquireRefLocked();
			rw_lock_read_unlock(&sAreaCacheLock);
			return cache;
		}

		// the cache changed in the meantime
		cache->Unlock();
	}
}


void
vm_area_put_locked_cache(VMCache* cache)
{
	cache->ReleaseRefAndUnlock();
}


area_id
vm_clone_area(team_id team, const char* name, void** address,
	uint32 addressSpec, uint32 protection, uint32 mapping, area_id sourceID,
	bool kernel)
{
	// Check whether the source area exists and is cloneable. If so, mark it
	// B_SHARED_AREA, so that we don't get problems with copy-on-write.
	{
		AddressSpaceWriteLocker locker;
		VMArea* sourceArea;
		status_t status = locker.SetFromArea(sourceID, sourceArea);
		if (status != B_OK)
			return status;

		if (!kernel && (sourceArea->protection & B_KERNEL_AREA) != 0)
			return B_NOT_ALLOWED;

		sourceArea->protection |= B_SHARED_AREA;
		protection |= B_SHARED_AREA;
	}

	// Now lock both address spaces and actually do the cloning.

	MultiAddressSpaceLocker locker;
	VMAddressSpace* sourceAddressSpace;
	status_t status = locker.AddArea(sourceID, false, &sourceAddressSpace);
	if (status != B_OK)
		return status;

	VMAddressSpace* targetAddressSpace;
	status = locker.AddTeam(team, true, &targetAddressSpace);
	if (status != B_OK)
		return status;

	status = locker.Lock();
	if (status != B_OK)
		return status;

	VMArea* sourceArea = lookup_area(sourceAddressSpace, sourceID);
	if (sourceArea == NULL)
		return B_BAD_VALUE;

	if (!kernel && (sourceArea->protection & B_KERNEL_AREA) != 0)
		return B_NOT_ALLOWED;

	AreaCacheLocker cacheLocker(sourceArea);
	VMCache* cache = cacheLocker.Get();

	int protectionMax = sourceArea->protection_max;
	if (!kernel && sourceAddressSpace != targetAddressSpace) {
		if ((sourceArea->protection & B_CLONEABLE_AREA) == 0) {
#if KDEBUG
			Team* team = thread_get_current_thread()->team;
			dprintf("team \"%s\" (%" B_PRId32 ") attempted to clone area \"%s\" (%"
				B_PRId32 ")!\n", team->Name(), team->id, sourceArea->name, sourceID);
#endif
			return B_NOT_ALLOWED;
		}

		if (protectionMax == 0)
			protectionMax = B_USER_PROTECTION;
		if ((sourceArea->protection & (B_WRITE_AREA | B_KERNEL_WRITE_AREA)) == 0)
			protectionMax &= ~B_WRITE_AREA;
		if (((protection & B_USER_PROTECTION) & ~protectionMax) != 0) {
#if KDEBUG
			Team* team = thread_get_current_thread()->team;
			dprintf("team \"%s\" (%" B_PRId32 ") attempted to clone area \"%s\" (%"
				B_PRId32 ") with extra permissions (0x%x)!\n", team->Name(), team->id,
				sourceArea->name, sourceID, protection);
#endif
			return B_NOT_ALLOWED;
		}
	}
	if (sourceArea->cache_type == CACHE_TYPE_NULL)
		return B_NOT_ALLOWED;

	uint32 mappingFlags = 0;
	if (mapping != REGION_PRIVATE_MAP)
		mappingFlags |= CREATE_AREA_DONT_COMMIT_MEMORY;

	virtual_address_restrictions addressRestrictions = {};
	VMArea* newArea;
	addressRestrictions.address = *address;
	addressRestrictions.address_specification = addressSpec;
	status = map_backing_store(targetAddressSpace, cache,
		sourceArea->cache_offset, name, sourceArea->Size(),
		sourceArea->wiring, protection, protectionMax,
		mapping, mappingFlags, &addressRestrictions,
		kernel, &newArea, address);
	if (status < B_OK)
		return status;

	if (mapping != REGION_PRIVATE_MAP) {
		// If the mapping is REGION_PRIVATE_MAP, map_backing_store() needed to
		// create a new cache, and has therefore already acquired a reference
		// to the source cache - but otherwise it has no idea that we need one.
		cache->AcquireRefLocked();
	}

	if (newArea->wiring == B_FULL_LOCK) {
		// we need to map in everything at this point
		if (sourceArea->cache_type == CACHE_TYPE_DEVICE) {
			// we don't have actual pages to map but a physical area
			VMTranslationMap* map
				= sourceArea->address_space->TranslationMap();
			map->Lock();

			phys_addr_t physicalAddress;
			uint32 oldProtection;
			map->Query(sourceArea->Base(), &physicalAddress, &oldProtection);

			map->Unlock();

			map = targetAddressSpace->TranslationMap();
			size_t reservePages = map->MaxPagesNeededToMap(newArea->Base(),
				newArea->Base() + (newArea->Size() - 1));

			vm_page_reservation reservation;
			vm_page_reserve_pages(&reservation, reservePages,
				targetAddressSpace == VMAddressSpace::Kernel()
					? VM_PRIORITY_SYSTEM : VM_PRIORITY_USER);
			map->Lock();

			for (addr_t offset = 0; offset < newArea->Size();
					offset += B_PAGE_SIZE) {
				map->Map(newArea->Base() + offset, physicalAddress + offset,
					protection, newArea->MemoryType(), &reservation);
			}

			map->Unlock();
			vm_page_unreserve_pages(&reservation);
		} else {
			VMTranslationMap* map = targetAddressSpace->TranslationMap();
			size_t reservePages = map->MaxPagesNeededToMap(
				newArea->Base(), newArea->Base() + (newArea->Size() - 1));
			vm_page_reservation reservation;
			vm_page_reserve_pages(&reservation, reservePages,
				targetAddressSpace == VMAddressSpace::Kernel()
					? VM_PRIORITY_SYSTEM : VM_PRIORITY_USER);

			// map in all pages from source
			for (VMCachePagesTree::Iterator it = cache->pages.GetIterator();
					vm_page* page  = it.Next();) {
				if (!page->busy) {
					DEBUG_PAGE_ACCESS_START(page);
					map_page(newArea, page,
						newArea->Base() + ((page->cache_offset << PAGE_SHIFT)
							- newArea->cache_offset),
						protection, &reservation);
					DEBUG_PAGE_ACCESS_END(page);
				}
			}
			// TODO: B_FULL_LOCK means that all pages are locked. We are not
			// ensuring that!

			vm_page_unreserve_pages(&reservation);
		}
	}

	newArea->cache_type = sourceArea->cache_type;
	return newArea->id;
}


/*!	Deletes the specified area of the given address space.

	The address space must be write-locked.
	The caller must ensure that the area does not have any wired ranges.

	\param addressSpace The address space containing the area.
	\param area The area to be deleted.
	\param deletingAddressSpace \c true, if the address space is in the process
		of being deleted.
	\param alreadyRemoved \c true, if the area was already removed from the global
		areas map (and thus had its ID deallocated.)
*/
static void
delete_area(VMAddressSpace* addressSpace, VMArea* area,
	bool deletingAddressSpace, bool alreadyRemoved)
{
	ASSERT(!area->IsWired());

	if (area->id >= 0 && !alreadyRemoved)
		VMAreas::Remove(area);

	// At this point the area is removed from the global hash table, but
	// still exists in the area list.

	// Unmap the virtual address space the area occupied.
	{
		// We need to lock the complete cache chain.
		VMCache* topCache = vm_area_get_locked_cache(area);
		VMCacheChainLocker cacheChainLocker(topCache);
		cacheChainLocker.LockAllSourceCaches();

		// If the area's top cache is a temporary cache and the area is the only
		// one referencing it (besides us currently holding a second reference),
		// the unmapping code doesn't need to care about preserving the accessed
		// and dirty flags of the top cache page mappings.
		bool ignoreTopCachePageFlags
			= topCache->temporary && topCache->RefCount() == 2;

		area->address_space->TranslationMap()->UnmapArea(area,
			deletingAddressSpace, ignoreTopCachePageFlags);
	}

	if (!area->cache->temporary)
		area->cache->WriteModified();

	uint32 allocationFlags = addressSpace == VMAddressSpace::Kernel()
		? HEAP_DONT_WAIT_FOR_MEMORY | HEAP_DONT_LOCK_KERNEL_SPACE : 0;

	arch_vm_unset_memory_type(area);
	addressSpace->RemoveArea(area, allocationFlags);
	addressSpace->Put();

	area->cache->RemoveArea(area);
	area->cache->ReleaseRef();

	addressSpace->DeleteArea(area, allocationFlags);
}


status_t
vm_delete_area(team_id team, area_id id, bool kernel)
{
	TRACE(("vm_delete_area(team = 0x%" B_PRIx32 ", area = 0x%" B_PRIx32 ")\n",
		team, id));

	// lock the address space and make sure the area isn't wired
	AddressSpaceWriteLocker locker;
	VMArea* area;
	AreaCacheLocker cacheLocker;

	do {
		status_t status = locker.SetFromArea(team, id, area);
		if (status != B_OK)
			return status;

		cacheLocker.SetTo(area);
	} while (wait_if_area_is_wired(area, &locker, &cacheLocker));

	cacheLocker.Unlock();

	if (!kernel && (area->protection & B_KERNEL_AREA) != 0)
		return B_NOT_ALLOWED;

	delete_area(locker.AddressSpace(), area, false);
	return B_OK;
}


/*!	Creates a new cache on top of given cache, moves all areas from
	the old cache to the new one, and changes the protection of all affected
	areas' pages to read-only. If requested, wired pages are moved up to the
	new cache and copies are added to the old cache in their place.
	Preconditions:
	- The given cache must be locked.
	- All of the cache's areas' address spaces must be read locked.
	- Either the cache must not have any wired ranges or a page reservation for
	  all wired pages must be provided, so they can be copied.

	\param lowerCache The cache on top of which a new cache shall be created.
	\param wiredPagesReservation If \c NULL there must not be any wired pages
		in \a lowerCache. Otherwise as many pages must be reserved as the cache
		has wired page. The wired pages are copied in this case.
*/
static status_t
vm_copy_on_write_area(VMCache* lowerCache,
	vm_page_reservation* wiredPagesReservation)
{
	TRACE(("vm_copy_on_write_area(cache = %p)\n", lowerCache));

	// We need to separate the cache from its areas. The cache goes one level
	// deeper and we create a new cache inbetween.

	// create an anonymous cache
	VMCache* upperCache;
	status_t status = VMCacheFactory::CreateAnonymousCache(upperCache,
		lowerCache->CanOvercommit(), 0,
		lowerCache->GuardSize() / B_PAGE_SIZE,
		dynamic_cast<VMAnonymousNoSwapCache*>(lowerCache) == NULL,
		VM_PRIORITY_USER);
	if (status != B_OK)
		return status;

	upperCache->Lock();

	upperCache->temporary = 1;
	upperCache->virtual_base = lowerCache->virtual_base;
	upperCache->virtual_end = lowerCache->virtual_end;

	// Shrink the lower cache's commitment (if possible) and steal the remainder;
	// and increase the upper cache's commitment to the lower cache's old commitment.
	const off_t lowerOldCommitment = lowerCache->committed_size,
		lowerNewCommitment = (lowerCache->page_count * B_PAGE_SIZE);
	if (lowerNewCommitment < lowerOldCommitment) {
		lowerCache->committed_size = lowerNewCommitment;
		upperCache->committed_size = lowerOldCommitment - lowerNewCommitment;
	}
	status = upperCache->Commit(lowerOldCommitment, VM_PRIORITY_USER);
	if (status != B_OK) {
		lowerCache->committed_size += upperCache->committed_size;
		upperCache->committed_size = 0;
		upperCache->ReleaseRefAndUnlock();
		return status;
	}

	// transfer the lower cache areas to the upper cache
	rw_lock_write_lock(&sAreaCacheLock);
	upperCache->TransferAreas(lowerCache);
	rw_lock_write_unlock(&sAreaCacheLock);

	lowerCache->AddConsumer(upperCache);

	// We now need to remap all pages from all of the cache's areas read-only,
	// so that a copy will be created on next write access. If there are wired
	// pages, we keep their protection, move them to the upper cache and create
	// copies for the lower cache.
	if (wiredPagesReservation != NULL) {
		// We need to handle wired pages -- iterate through the cache's pages.
		for (VMCachePagesTree::Iterator it = lowerCache->pages.GetIterator();
				vm_page* page = it.Next();) {
			if (page->WiredCount() > 0) {
				// allocate a new page and copy the wired one
				vm_page* copiedPage = vm_page_allocate_page(
					wiredPagesReservation, PAGE_STATE_ACTIVE);

				vm_memcpy_physical_page(
					copiedPage->physical_page_number * B_PAGE_SIZE,
					page->physical_page_number * B_PAGE_SIZE);

				// move the wired page to the upper cache (note: removing is OK
				// with the SplayTree iterator) and insert the copy
				upperCache->MovePage(page);
				lowerCache->InsertPage(copiedPage,
					page->cache_offset * B_PAGE_SIZE);

				DEBUG_PAGE_ACCESS_END(copiedPage);
			} else {
				// Change the protection of this page in all areas.
				for (VMArea* tempArea = upperCache->areas.First(); tempArea != NULL;
						tempArea = upperCache->areas.GetNext(tempArea)) {
					if (!is_page_in_area(tempArea, page))
						continue;

					// The area must be readable in the same way it was
					// previously writable.
					addr_t address = virtual_page_address(tempArea, page);
					uint32 protection = 0;
					uint32 pageProtection = get_area_page_protection(tempArea, address);
					if ((pageProtection & B_KERNEL_READ_AREA) != 0)
						protection |= B_KERNEL_READ_AREA;
					if ((pageProtection & B_READ_AREA) != 0)
						protection |= B_READ_AREA;

					VMTranslationMap* map
						= tempArea->address_space->TranslationMap();
					map->Lock();
					map->ProtectPage(tempArea, address, protection);
					map->Unlock();
				}
			}
		}
	} else {
		ASSERT(lowerCache->WiredPagesCount() == 0);

		// just change the protection of all areas
		for (VMArea* tempArea = upperCache->areas.First(); tempArea != NULL;
				tempArea = upperCache->areas.GetNext(tempArea)) {
			if (tempArea->page_protections != NULL) {
				// Change the protection of all pages in this area.
				VMTranslationMap* map = tempArea->address_space->TranslationMap();
				map->Lock();
				for (VMCachePagesTree::Iterator it = lowerCache->pages.GetIterator();
					vm_page* page = it.Next();) {
					if (!is_page_in_area(tempArea, page))
						continue;

					// The area must be readable in the same way it was
					// previously writable.
					addr_t address = virtual_page_address(tempArea, page);
					uint32 protection = 0;
					uint32 pageProtection = get_area_page_protection(tempArea, address);
					if ((pageProtection & B_KERNEL_READ_AREA) != 0)
						protection |= B_KERNEL_READ_AREA;
					if ((pageProtection & B_READ_AREA) != 0)
						protection |= B_READ_AREA;

					map->ProtectPage(tempArea, address, protection);
				}
				map->Unlock();
				continue;
			}
			// The area must be readable in the same way it was previously
			// writable.
			uint32 protection = 0;
			if ((tempArea->protection & B_KERNEL_READ_AREA) != 0)
				protection |= B_KERNEL_READ_AREA;
			if ((tempArea->protection & B_READ_AREA) != 0)
				protection |= B_READ_AREA;

			VMTranslationMap* map = tempArea->address_space->TranslationMap();
			map->Lock();
			map->ProtectArea(tempArea, protection);
			map->Unlock();
		}
	}

	vm_area_put_locked_cache(upperCache);

	return B_OK;
}


area_id
vm_copy_area(team_id team, const char* name, void** _address,
	uint32 addressSpec, area_id sourceID)
{
	// Do the locking: target address space, all address spaces associated with
	// the source cache, and the cache itself.
	MultiAddressSpaceLocker locker;
	VMAddressSpace* targetAddressSpace;
	VMCache* cache;
	VMArea* source;
	AreaCacheLocker cacheLocker;
	status_t status;
	bool sharedArea;

	page_num_t wiredPages = 0;
	vm_page_reservation wiredPagesReservation;

	bool restart;
	do {
		restart = false;

		locker.Unset();
		status = locker.AddTeam(team, true, &targetAddressSpace);
		if (status == B_OK) {
			status = locker.AddAreaCacheAndLock(sourceID, false, false, source,
				&cache);
		}
		if (status != B_OK)
			return status;

		cacheLocker.SetTo(cache, true);	// already locked

		sharedArea = (source->protection & B_SHARED_AREA) != 0;

		page_num_t oldWiredPages = wiredPages;
		wiredPages = 0;

		// If the source area isn't shared, count the number of wired pages in
		// the cache and reserve as many pages.
		if (!sharedArea) {
			wiredPages = cache->WiredPagesCount();

			if (wiredPages > oldWiredPages) {
				cacheLocker.Unlock();
				locker.Unlock();

				if (oldWiredPages > 0)
					vm_page_unreserve_pages(&wiredPagesReservation);

				vm_page_reserve_pages(&wiredPagesReservation, wiredPages,
					VM_PRIORITY_USER);

				restart = true;
			}
		} else if (oldWiredPages > 0)
			vm_page_unreserve_pages(&wiredPagesReservation);
	} while (restart);

	// unreserve pages later
	CObjectDeleter<vm_page_reservation, void, vm_page_unreserve_pages>
		pagesUnreserver(wiredPages > 0 ? &wiredPagesReservation : NULL);

	bool writableCopy
		= (source->protection & (B_KERNEL_WRITE_AREA | B_WRITE_AREA)) != 0;
	uint8* targetPageProtections = NULL;

	if (source->page_protections != NULL) {
		const size_t bytes = area_page_protections_size(source->Size());
		targetPageProtections = (uint8*)malloc_etc(bytes,
			(source->address_space == VMAddressSpace::Kernel()
					|| targetAddressSpace == VMAddressSpace::Kernel())
				? HEAP_DONT_LOCK_KERNEL_SPACE : 0);
		if (targetPageProtections == NULL)
			return B_NO_MEMORY;

		memcpy(targetPageProtections, source->page_protections, bytes);

		for (size_t i = 0; i < bytes; i++) {
			if ((targetPageProtections[i]
					& (B_WRITE_AREA | (B_WRITE_AREA << 4))) != 0) {
				writableCopy = true;
				break;
			}
		}
	}

	if (addressSpec == B_CLONE_ADDRESS) {
		addressSpec = B_EXACT_ADDRESS;
		*_address = (void*)source->Base();
	}

	// First, create a cache on top of the source area, respectively use the
	// existing one, if this is a shared area.

	VMArea* target;
	virtual_address_restrictions addressRestrictions = {};
	addressRestrictions.address = *_address;
	addressRestrictions.address_specification = addressSpec;
	status = map_backing_store(targetAddressSpace, cache, source->cache_offset,
		name, source->Size(), source->wiring, source->protection,
		source->protection_max,
		sharedArea ? REGION_NO_PRIVATE_MAP : REGION_PRIVATE_MAP,
		writableCopy ? 0 : CREATE_AREA_DONT_COMMIT_MEMORY,
		&addressRestrictions, true, &target, _address);
	if (status < B_OK) {
		free_etc(targetPageProtections, HEAP_DONT_LOCK_KERNEL_SPACE);
		return status;
	}

	if (targetPageProtections != NULL) {
		target->page_protections = targetPageProtections;

		if (!sharedArea) {
			// Shrink the commitment (this should never fail).
			AreaCacheLocker locker(target);
			const size_t newPageCommitment = compute_area_page_commitment(target);
			target->cache->Commit(newPageCommitment * B_PAGE_SIZE, VM_PRIORITY_USER);
		}
	}

	if (sharedArea) {
		// The new area uses the old area's cache, but map_backing_store()
		// hasn't acquired a ref. So we have to do that now.
		cache->AcquireRefLocked();
	}

	// If the source area is writable, we need to move it one layer up as well.
	if (!sharedArea && writableCopy) {
		status_t status = vm_copy_on_write_area(cache,
			wiredPages > 0 ? &wiredPagesReservation : NULL);
		if (status != B_OK) {
			cacheLocker.Unlock();
			delete_area(targetAddressSpace, target, false);
			return status;
		}
	}

	// we return the ID of the newly created area
	return target->id;
}


status_t
vm_set_area_protection(team_id team, area_id areaID, uint32 newProtection,
	bool kernel)
{
	fix_protection(&newProtection);

	TRACE(("vm_set_area_protection(team = %#" B_PRIx32 ", area = %#" B_PRIx32
		", protection = %#" B_PRIx32 ")\n", team, areaID, newProtection));

	if (!arch_vm_supports_protection(newProtection))
		return B_NOT_SUPPORTED;

	bool becomesWritable
		= (newProtection & (B_WRITE_AREA | B_KERNEL_WRITE_AREA)) != 0;

	// lock address spaces and cache
	MultiAddressSpaceLocker locker;
	VMCache* cache;
	VMArea* area;
	status_t status;
	AreaCacheLocker cacheLocker;
	bool isWritable;

	bool restart;
	do {
		restart = false;

		locker.Unset();
		status = locker.AddAreaCacheAndLock(areaID, true, false, area, &cache);
		if (status != B_OK)
			return status;

		cacheLocker.SetTo(cache, true);	// already locked

		// enforce restrictions
		if (!kernel && (area->address_space == VMAddressSpace::Kernel()
				|| (area->protection & B_KERNEL_AREA) != 0)) {
#if KDEBUG
			dprintf("vm_set_area_protection: team %" B_PRId32 " tried to "
				"set protection %#" B_PRIx32 " on kernel area %" B_PRId32
				" (%s)\n", team, newProtection, areaID, area->name);
#endif
			return B_NOT_ALLOWED;
		}
		if (!kernel && area->protection_max != 0
			&& (newProtection & area->protection_max)
				!= (newProtection & B_USER_PROTECTION)) {
#if KDEBUG
			dprintf("vm_set_area_protection: team %" B_PRId32 " tried to "
				"set protection %#" B_PRIx32 " (max %#" B_PRIx32 ") on area "
				"%" B_PRId32 " (%s)\n", team, newProtection,
				area->protection_max, areaID, area->name);
#endif
			return B_NOT_ALLOWED;
		}
		if (team != VMAddressSpace::KernelID()
			&& area->address_space->ID() != team) {
			// unless you're the kernel, you're only allowed to set
			// the protection of your own areas
			return B_NOT_ALLOWED;
		}

		if (area->protection == newProtection)
			return B_OK;

		isWritable
			= (area->protection & (B_WRITE_AREA | B_KERNEL_WRITE_AREA)) != 0;

		// Make sure the area (respectively, if we're going to call
		// vm_copy_on_write_area(), all areas of the cache) doesn't have any
		// wired ranges.
		if (!isWritable && becomesWritable && !cache->consumers.IsEmpty()) {
			for (VMArea* otherArea = cache->areas.First(); otherArea != NULL;
					otherArea = cache->areas.GetNext(otherArea)) {
				if (wait_if_area_is_wired(otherArea, &locker, &cacheLocker)) {
					restart = true;
					break;
				}
			}
		} else {
			if (wait_if_area_is_wired(area, &locker, &cacheLocker))
				restart = true;
		}
	} while (restart);

	if (area->page_protections != NULL) {
		// Get rid of the per-page protections.
		free_etc(area->page_protections,
			area->address_space == VMAddressSpace::Kernel() ? HEAP_DONT_LOCK_KERNEL_SPACE : 0);
		area->page_protections = NULL;

		// Assume the existing protections don't match the new ones.
		isWritable = !becomesWritable;
	}

	bool changePageProtection = true;
	bool changeTopCachePagesOnly = false;

	if (isWritable && !becomesWritable) {
		// writable -> !writable

		if (cache->source != NULL && cache->temporary) {
			if (cache->CountWritableAreas(area) == 0) {
				// Since this cache now lives from the pages in its source cache,
				// we can change the cache's commitment to take only those pages
				// into account that really are in this cache.

				status = cache->Commit(cache->page_count * B_PAGE_SIZE,
					team == VMAddressSpace::KernelID()
						? VM_PRIORITY_SYSTEM : VM_PRIORITY_USER);

				// TODO: we may be able to join with our source cache, if
				// count == 0
			}
		}

		// If only the writability changes, we can just remap the pages of the
		// top cache, since the pages of lower caches are mapped read-only
		// anyway. That's advantageous only, if the number of pages in the cache
		// is significantly smaller than the number of pages in the area,
		// though.
		if (newProtection
				== (area->protection & ~(B_WRITE_AREA | B_KERNEL_WRITE_AREA))
			&& cache->page_count * 2 < area->Size() / B_PAGE_SIZE) {
			changeTopCachePagesOnly = true;
		}
	} else if (!isWritable && becomesWritable) {
		// !writable -> writable

		if (!cache->consumers.IsEmpty()) {
			// There are consumers -- we have to insert a new cache. Fortunately
			// vm_copy_on_write_area() does everything that's needed.
			changePageProtection = false;
			status = vm_copy_on_write_area(cache, NULL);
		} else {
			// No consumers, so we don't need to insert a new one.
			if (cache->temporary) {
				// the cache's commitment must contain all possible pages
				status = cache->Commit(cache->virtual_end - cache->virtual_base,
					team == VMAddressSpace::KernelID()
						? VM_PRIORITY_SYSTEM : VM_PRIORITY_USER);
			}

			if (status == B_OK && cache->source != NULL) {
				// There's a source cache, hence we can't just change all pages'
				// protection or we might allow writing into pages belonging to
				// a lower cache.
				changeTopCachePagesOnly = true;
			}
		}
	} else {
		// we don't have anything special to do in all other cases
	}

	if (status == B_OK) {
		// remap existing pages in this cache
		if (changePageProtection) {
			VMTranslationMap* map = area->address_space->TranslationMap();
			map->Lock();

			if (changeTopCachePagesOnly) {
				page_num_t firstPageOffset = area->cache_offset / B_PAGE_SIZE;
				page_num_t lastPageOffset
					= firstPageOffset + area->Size() / B_PAGE_SIZE;
				for (VMCachePagesTree::Iterator it = cache->pages.GetIterator();
						vm_page* page = it.Next();) {
					if (page->cache_offset >= firstPageOffset
						&& page->cache_offset <= lastPageOffset) {
						addr_t address = virtual_page_address(area, page);
						map->ProtectPage(area, address, newProtection);
					}
				}
			} else
				map->ProtectArea(area, newProtection);

			map->Unlock();
		}

		area->protection = newProtection;
	}

	return status;
}


status_t
vm_get_page_mapping(team_id team, addr_t vaddr, phys_addr_t* paddr)
{
	VMAddressSpace* addressSpace = VMAddressSpace::Get(team);
	if (addressSpace == NULL)
		return B_BAD_TEAM_ID;

	VMTranslationMap* map = addressSpace->TranslationMap();

	map->Lock();
	uint32 dummyFlags;
	status_t status = map->Query(vaddr, paddr, &dummyFlags);
	map->Unlock();

	addressSpace->Put();
	return status;
}


/*!	The page's cache must be locked.
*/
bool
vm_test_map_modification(vm_page* page)
{
	if (page->modified)
		return true;

	vm_page_mappings::Iterator iterator = page->mappings.GetIterator();
	vm_page_mapping* mapping;
	while ((mapping = iterator.Next()) != NULL) {
		VMArea* area = mapping->area;
		VMTranslationMap* map = area->address_space->TranslationMap();

		phys_addr_t physicalAddress;
		uint32 flags;
		map->Lock();
		map->Query(virtual_page_address(area, page), &physicalAddress, &flags);
		map->Unlock();

		if ((flags & PAGE_MODIFIED) != 0)
			return true;
	}

	return false;
}


/*!	The page's cache must be locked.
*/
void
vm_clear_map_flags(vm_page* page, uint32 flags)
{
	if ((flags & PAGE_ACCESSED) != 0)
		page->accessed = false;
	if ((flags & PAGE_MODIFIED) != 0)
		page->modified = false;

	vm_page_mappings::Iterator iterator = page->mappings.GetIterator();
	vm_page_mapping* mapping;
	while ((mapping = iterator.Next()) != NULL) {
		VMArea* area = mapping->area;
		VMTranslationMap* map = area->address_space->TranslationMap();

		map->Lock();
		map->ClearFlags(virtual_page_address(area, page), flags);
		map->Unlock();
	}
}


/*!	Removes all mappings from a page.
	After you've called this function, the page is unmapped from memory and
	the page's \c accessed and \c modified flags have been updated according
	to the state of the mappings.
	The page's cache must be locked.
*/
void
vm_remove_all_page_mappings(vm_page* page)
{
	while (vm_page_mapping* mapping = page->mappings.Head()) {
		VMArea* area = mapping->area;
		VMTranslationMap* map = area->address_space->TranslationMap();
		addr_t address = virtual_page_address(area, page);
		map->UnmapPage(area, address, false);
	}
}


int32
vm_clear_page_mapping_accessed_flags(struct vm_page *page)
{
	int32 count = 0;

	vm_page_mappings::Iterator iterator = page->mappings.GetIterator();
	vm_page_mapping* mapping;
	while ((mapping = iterator.Next()) != NULL) {
		VMArea* area = mapping->area;
		VMTranslationMap* map = area->address_space->TranslationMap();

		bool modified;
		if (map->ClearAccessedAndModified(area,
				virtual_page_address(area, page), false, modified)) {
			count++;
		}

		page->modified |= modified;
	}


	if (page->accessed) {
		count++;
		page->accessed = false;
	}

	return count;
}


/*!	Removes all mappings of a page and/or clears the accessed bits of the
	mappings.
	The function iterates through the page mappings and removes them until
	encountering one that has been accessed. From then on it will continue to
	iterate, but only clear the accessed flag of the mapping. The page's
	\c modified bit will be updated accordingly, the \c accessed bit will be
	cleared.
	\return The number of mapping accessed bits encountered, including the
		\c accessed bit of the page itself. If \c 0 is returned, all mappings
		of the page have been removed.
*/
int32
vm_remove_all_page_mappings_if_unaccessed(struct vm_page *page)
{
	ASSERT(page->WiredCount() == 0);

	if (page->accessed)
		return vm_clear_page_mapping_accessed_flags(page);

	while (vm_page_mapping* mapping = page->mappings.Head()) {
		VMArea* area = mapping->area;
		VMTranslationMap* map = area->address_space->TranslationMap();
		addr_t address = virtual_page_address(area, page);
		bool modified = false;
		if (map->ClearAccessedAndModified(area, address, true, modified)) {
			page->accessed = true;
			page->modified |= modified;
			return vm_clear_page_mapping_accessed_flags(page);
		}
		page->modified |= modified;
	}

	return 0;
}


/*!	Deletes all areas and reserved regions in the given address space.

	The caller must ensure that none of the areas has any wired ranges.

	\param addressSpace The address space.
	\param deletingAddressSpace \c true, if the address space is in the process
		of being deleted.
*/
void
vm_delete_areas(struct VMAddressSpace* addressSpace, bool deletingAddressSpace)
{
	TRACE(("vm_delete_areas: called on address space 0x%" B_PRIx32 "\n",
		addressSpace->ID()));

	addressSpace->WriteLock();

	// remove all reserved areas in this address space
	addressSpace->UnreserveAllAddressRanges(0);

	// remove all areas from the areas map at once (to avoid lock contention)
	VMAreas::WriteLock();
	{
		VMAddressSpace::AreaIterator it = addressSpace->GetAreaIterator();
		while (VMArea* area = it.Next())
			VMAreas::Remove(area);
	}
	VMAreas::WriteUnlock();

	// delete all the areas in this address space
	while (VMArea* area = addressSpace->FirstArea()) {
		ASSERT(!area->IsWired());
		delete_area(addressSpace, area, deletingAddressSpace, true);
	}

	addressSpace->WriteUnlock();
}


static area_id
vm_area_for(addr_t address, bool kernel)
{
	team_id team;
	if (IS_USER_ADDRESS(address)) {
		// we try the user team address space, if any
		team = VMAddressSpace::CurrentID();
		if (team < 0)
			return team;
	} else
		team = VMAddressSpace::KernelID();

	AddressSpaceReadLocker locker(team);
	if (!locker.IsLocked())
		return B_BAD_TEAM_ID;

	VMArea* area = locker.AddressSpace()->LookupArea(address);
	if (area != NULL) {
		if (!kernel && team == VMAddressSpace::KernelID()
				&& (area->protection & (B_READ_AREA | B_WRITE_AREA | B_CLONEABLE_AREA)) == 0)
			return B_ERROR;

		return area->id;
	}

	return B_ERROR;
}


/*!	Frees physical pages that were used during the boot process.
	\a end is inclusive.
*/
static void
unmap_and_free_physical_pages(VMTranslationMap* map, addr_t start, addr_t end)
{
	// free all physical pages in the specified range

	vm_page_reservation reservation = {};
	for (addr_t current = start; current < end; current += B_PAGE_SIZE) {
		phys_addr_t physicalAddress;
		uint32 flags;

		if (map->Query(current, &physicalAddress, &flags) == B_OK
				&& (flags & PAGE_PRESENT) != 0) {
			vm_page* page = vm_lookup_page(physicalAddress / B_PAGE_SIZE);
			if (page != NULL && page->State() != PAGE_STATE_FREE
					&& page->State() != PAGE_STATE_CLEAR
					&& page->State() != PAGE_STATE_UNUSED) {
				DEBUG_PAGE_ACCESS_START(page);
				vm_page_free_etc(NULL, page, &reservation);
			}
		}
	}

	// unmap the memory
	map->Unmap(start, end);

	// unreserve the memory
	vm_unreserve_memory(reservation.count * B_PAGE_SIZE);
	vm_page_unreserve_pages(&reservation);
}


void
vm_free_unused_boot_loader_range(addr_t start, addr_t size)
{
	VMTranslationMap* map = VMAddressSpace::Kernel()->TranslationMap();
	addr_t end = start + (size - 1);
	addr_t lastEnd = start;

	TRACE(("vm_free_unused_boot_loader_range(): asked to free %p - %p\n",
		(void*)start, (void*)end));

	// The areas are sorted in virtual address space order, so
	// we just have to find the holes between them that fall
	// into the area we should dispose

	map->Lock();

	for (VMAddressSpace::AreaIterator it
				= VMAddressSpace::Kernel()->GetAreaIterator();
			VMArea* area = it.Next();) {
		addr_t areaStart = area->Base();
		addr_t areaEnd = areaStart + (area->Size() - 1);

		if (areaEnd < start)
			continue;

		if (areaStart > end) {
			// we are done, the area is already beyond of what we have to free
			break;
		}

		if (areaStart > lastEnd) {
			// this is something we can free
			TRACE(("free boot range: get rid of %p - %p\n", (void*)lastEnd,
				(void*)areaStart));
			unmap_and_free_physical_pages(map, lastEnd, areaStart - 1);
		}

		if (areaEnd >= end) {
			lastEnd = areaEnd;
				// no +1 to prevent potential overflow
			break;
		}

		lastEnd = areaEnd + 1;
	}

	if (lastEnd < end) {
		// we can also get rid of some space at the end of the area
		TRACE(("free boot range: also remove %p - %p\n", (void*)lastEnd,
			(void*)end));
		unmap_and_free_physical_pages(map, lastEnd, end);
	}

	map->Unlock();
}


static void
create_preloaded_image_areas(struct preloaded_image* _image)
{
	preloaded_elf_image* image = static_cast<preloaded_elf_image*>(_image);
	char name[B_OS_NAME_LENGTH];
	void* address;
	int32 length;

	// use file name to create a good area name
	char* fileName = strrchr(image->name, '/');
	if (fileName == NULL)
		fileName = image->name;
	else
		fileName++;

	length = strlen(fileName);
	// make sure there is enough space for the suffix
	if (length > 25)
		length = 25;

	memcpy(name, fileName, length);
	strcpy(name + length, "_text");
	address = (void*)ROUNDDOWN(image->text_region.start, B_PAGE_SIZE);
	image->text_region.id = create_area(name, &address, B_EXACT_ADDRESS,
		PAGE_ALIGN(image->text_region.size), B_ALREADY_WIRED,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
		// this will later be remapped read-only/executable by the
		// ELF initialization code

	strcpy(name + length, "_data");
	address = (void*)ROUNDDOWN(image->data_region.start, B_PAGE_SIZE);
	image->data_region.id = create_area(name, &address, B_EXACT_ADDRESS,
		PAGE_ALIGN(image->data_region.size), B_ALREADY_WIRED,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
}


/*!	Frees all previously kernel arguments areas from the kernel_args structure.
	Any boot loader resources contained in that arguments must not be accessed
	anymore past this point.
*/
void
vm_free_kernel_args(kernel_args* args)
{
	TRACE(("vm_free_kernel_args()\n"));

	for (uint32 i = 0; i < args->num_kernel_args_ranges; i++) {
		area_id area = area_for((void*)(addr_t)args->kernel_args_range[i].start);
		if (area >= B_OK)
			delete_area(area);
	}
}


static void
allocate_kernel_args(kernel_args* args)
{
	TRACE(("allocate_kernel_args()\n"));

	for (uint32 i = 0; i < args->num_kernel_args_ranges; i++) {
		const addr_range& range = args->kernel_args_range[i];
		void* address = (void*)(addr_t)range.start;

		create_area("_kernel args_", &address, B_EXACT_ADDRESS,
			range.size, B_ALREADY_WIRED, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	}
}


static void
unreserve_boot_loader_ranges(kernel_args* args)
{
	TRACE(("unreserve_boot_loader_ranges()\n"));

	for (uint32 i = 0; i < args->num_virtual_allocated_ranges; i++) {
		const addr_range& range = args->virtual_allocated_range[i];
		vm_unreserve_address_range(VMAddressSpace::KernelID(),
			(void*)(addr_t)range.start, range.size);
	}
}


static void
reserve_boot_loader_ranges(kernel_args* args)
{
	TRACE(("reserve_boot_loader_ranges()\n"));

	for (uint32 i = 0; i < args->num_virtual_allocated_ranges; i++) {
		const addr_range& range = args->virtual_allocated_range[i];
		void* address = (void*)(addr_t)range.start;

		// If the address is no kernel address, we just skip it. The
		// architecture specific code has to deal with it.
		if (!IS_KERNEL_ADDRESS(address)) {
			dprintf("reserve_boot_loader_ranges(): Skipping range: %p, %"
				B_PRIu64 "\n", address, range.size);
			continue;
		}

		status_t status = vm_reserve_address_range(VMAddressSpace::KernelID(),
			&address, B_EXACT_ADDRESS, range.size, 0);
		if (status < B_OK)
			panic("could not reserve boot loader ranges\n");
	}
}


static addr_t
allocate_early_virtual(kernel_args* args, size_t size, addr_t alignment)
{
	size = PAGE_ALIGN(size);
	if (alignment <= B_PAGE_SIZE) {
		// All allocations are naturally page-aligned.
		alignment = 0;
	} else {
		ASSERT((alignment % B_PAGE_SIZE) == 0);
	}

	// Find a slot in the virtual allocation ranges.
	for (uint32 i = 1; i < args->num_virtual_allocated_ranges; i++) {
		// Check if the space between this one and the previous is big enough.
		const addr_range& range = args->virtual_allocated_range[i];
		addr_range& previousRange = args->virtual_allocated_range[i - 1];
		const addr_t previousRangeEnd = previousRange.start + previousRange.size;

		addr_t base = alignment > 0
			? ROUNDUP(previousRangeEnd, alignment) : previousRangeEnd;

		if (base >= KERNEL_BASE && base < range.start && (range.start - base) >= size) {
			previousRange.size += base + size - previousRangeEnd;
			return base;
		}
	}

	// We didn't find one between allocation ranges. This is OK.
	// See if there's a gap after the last one.
	addr_range& lastRange
		= args->virtual_allocated_range[args->num_virtual_allocated_ranges - 1];
	const addr_t lastRangeEnd = lastRange.start + lastRange.size;
	addr_t base = alignment > 0
		? ROUNDUP(lastRangeEnd, alignment) : lastRangeEnd;
	if ((KERNEL_TOP - base) >= size) {
		lastRange.size += base + size - lastRangeEnd;
		return base;
	}

	// See if there's a gap before the first one.
	addr_range& firstRange = args->virtual_allocated_range[0];
	if (firstRange.start > KERNEL_BASE && (firstRange.start - KERNEL_BASE) >= size) {
		base = firstRange.start - size;
		if (alignment > 0)
			base = ROUNDDOWN(base, alignment);

		if (base >= KERNEL_BASE) {
			firstRange.size += firstRange.start - base;
			firstRange.start = base;
			return base;
		}
	}

	return 0;
}


static bool
is_page_in_physical_memory_range(kernel_args* args, phys_addr_t address)
{
	// TODO: horrible brute-force method of determining if the page can be
	// allocated
	for (uint32 i = 0; i < args->num_physical_memory_ranges; i++) {
		const addr_range& range = args->physical_memory_range[i];
		if (address >= range.start && address < (range.start + range.size))
			return true;
	}
	return false;
}


page_num_t
vm_allocate_early_physical_page(kernel_args* args, phys_addr_t maxAddress)
{
	if (args->num_physical_allocated_ranges == 0) {
		panic("early physical page allocations no longer possible!");
		return 0;
	}
	if (maxAddress == 0)
		maxAddress = __HAIKU_PHYS_ADDR_MAX;

#if defined(B_HAIKU_PHYSICAL_64_BIT)
	// Check if the last physical range is above the 32-bit maximum.
	const addr_range& lastMemoryRange =
		args->physical_memory_range[args->num_physical_memory_ranges - 1];
	const uint64 post32bitAddr = 0x100000000LL;
	if ((lastMemoryRange.start + lastMemoryRange.size) > post32bitAddr
			&& args->num_physical_allocated_ranges < MAX_PHYSICAL_ALLOCATED_RANGE) {
		// To avoid consuming physical memory in the 32-bit range (which drivers may need),
		// ensure the last allocated range at least ends past the 32-bit boundary.
		const addr_range& lastAllocatedRange =
			args->physical_allocated_range[args->num_physical_allocated_ranges - 1];
		const phys_addr_t lastAllocatedPage = lastAllocatedRange.start + lastAllocatedRange.size;
		if (lastAllocatedPage < post32bitAddr) {
			// Create ranges until we have one at least starting at the first point past 4GB.
			// (Some of the logic here is similar to the new-range code at the end of the method.)
			for (uint32 i = 0; i < args->num_physical_memory_ranges; i++) {
				addr_range& memoryRange = args->physical_memory_range[i];
				if ((memoryRange.start + memoryRange.size) < lastAllocatedPage)
					continue;
				if (memoryRange.size < (B_PAGE_SIZE * 128))
					continue;

				uint64 rangeStart = memoryRange.start;
				if ((memoryRange.start + memoryRange.size) <= post32bitAddr) {
					if (memoryRange.start < lastAllocatedPage)
						continue;

					// Range has no pages allocated and ends before the 32-bit boundary.
				} else {
					// Range ends past the 32-bit boundary. It could have some pages allocated,
					// but if we're here, we know that nothing is allocated above the boundary,
					// so we want to create a new range with it regardless.
					if (rangeStart < post32bitAddr)
						rangeStart = post32bitAddr;
				}

				addr_range& allocatedRange =
					args->physical_allocated_range[args->num_physical_allocated_ranges++];
				allocatedRange.start = rangeStart;
				allocatedRange.size = 0;

				if (rangeStart >= post32bitAddr)
					break;
				if (args->num_physical_allocated_ranges == MAX_PHYSICAL_ALLOCATED_RANGE)
					break;
			}
		}
	}
#endif

	// Try expanding the existing physical ranges upwards.
	for (int32 i = args->num_physical_allocated_ranges - 1; i >= 0; i--) {
		addr_range& range = args->physical_allocated_range[i];
		phys_addr_t nextPage = range.start + range.size;

		// check constraints
		if (nextPage > maxAddress)
			continue;

		// make sure the page does not collide with the next allocated range
		if ((i + 1) < (int32)args->num_physical_allocated_ranges) {
			addr_range& nextRange = args->physical_allocated_range[i + 1];
			if (nextRange.size != 0 && nextPage >= nextRange.start)
				continue;
		}
		// see if the next page fits in the memory block
		if (is_page_in_physical_memory_range(args, nextPage)) {
			// we got one!
			range.size += B_PAGE_SIZE;
			return nextPage / B_PAGE_SIZE;
		}
	}

	// Expanding upwards didn't work, try going downwards.
	for (uint32 i = 0; i < args->num_physical_allocated_ranges; i++) {
		addr_range& range = args->physical_allocated_range[i];
		phys_addr_t nextPage = range.start - B_PAGE_SIZE;

		// check constraints
		if (nextPage > maxAddress)
			continue;

		// make sure the page does not collide with the previous allocated range
		if (i > 0) {
			addr_range& previousRange = args->physical_allocated_range[i - 1];
			if (previousRange.size != 0 && nextPage < (previousRange.start + previousRange.size))
				continue;
		}
		// see if the next physical page fits in the memory block
		if (is_page_in_physical_memory_range(args, nextPage)) {
			// we got one!
			range.start -= B_PAGE_SIZE;
			range.size += B_PAGE_SIZE;
			return nextPage / B_PAGE_SIZE;
		}
	}

	// Try starting a new range.
	if (args->num_physical_allocated_ranges < MAX_PHYSICAL_ALLOCATED_RANGE) {
		const addr_range& lastAllocatedRange =
			args->physical_allocated_range[args->num_physical_allocated_ranges - 1];
		const phys_addr_t lastAllocatedPage = lastAllocatedRange.start + lastAllocatedRange.size;

		phys_addr_t nextPage = 0;
		for (uint32 i = 0; i < args->num_physical_memory_ranges; i++) {
			const addr_range& range = args->physical_memory_range[i];
			// Ignore everything before the last-allocated page, as well as small ranges.
			if (range.start < lastAllocatedPage || range.size < (B_PAGE_SIZE * 128))
				continue;
			if (range.start > maxAddress)
				break;

			nextPage = range.start;
			break;
		}

		if (nextPage != 0) {
			// we got one!
			addr_range& range =
				args->physical_allocated_range[args->num_physical_allocated_ranges++];
			range.start = nextPage;
			range.size = B_PAGE_SIZE;
			return nextPage / B_PAGE_SIZE;
		}
	}

	return 0;
		// could not allocate a block
}


/*!	This one uses the kernel_args' physical and virtual memory ranges to
	allocate some pages before the VM is completely up.
*/
addr_t
vm_allocate_early(kernel_args* args, size_t virtualSize, size_t physicalSize,
	uint32 attributes, addr_t alignment)
{
	if (physicalSize > virtualSize)
		physicalSize = virtualSize;

	// find the vaddr to allocate at
	addr_t virtualBase = allocate_early_virtual(args, virtualSize, alignment);
	//dprintf("vm_allocate_early: vaddr 0x%lx\n", virtualBase);
	if (virtualBase == 0) {
		panic("vm_allocate_early: could not allocate virtual address\n");
		return 0;
	}

	// map the pages
	for (uint32 i = 0; i < HOWMANY(physicalSize, B_PAGE_SIZE); i++) {
		page_num_t physicalAddress = vm_allocate_early_physical_page(args);
		if (physicalAddress == 0)
			panic("error allocating early page!\n");

		//dprintf("vm_allocate_early: paddr 0x%lx\n", physicalAddress);

		status_t status = arch_vm_translation_map_early_map(args,
			virtualBase + i * B_PAGE_SIZE,
			physicalAddress * B_PAGE_SIZE, attributes);
		if (status != B_OK)
			panic("error mapping early page!");
	}

	return virtualBase;
}


/*!	The main entrance point to initialize the VM. */
status_t
vm_init(kernel_args* args)
{
	struct preloaded_image* image;
	void* address;
	status_t err = 0;
	uint32 i;

	TRACE(("vm_init: entry\n"));
	err = arch_vm_translation_map_init(args, &sPhysicalPageMapper);
	err = arch_vm_init(args);

	// initialize some globals
	vm_page_init_num_pages(args);

	slab_init(args);

#if USE_DEBUG_HEAP_FOR_MALLOC || USE_GUARDED_HEAP_FOR_MALLOC
	off_t heapSize = INITIAL_HEAP_SIZE;
	// try to accomodate low memory systems
	while (heapSize > sAvailableMemory / 8)
		heapSize /= 2;
	if (heapSize < 1024 * 1024)
		panic("vm_init: go buy some RAM please.");

	// map in the new heap and initialize it
	addr_t heapBase = vm_allocate_early(args, heapSize, heapSize,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, 0);
	TRACE(("heap at 0x%lx\n", heapBase));
	heap_init(heapBase, heapSize);
#endif

	// initialize the free page list and physical page mapper
	vm_page_init(args);

	// initialize the cache allocators
	vm_cache_init(args);

	{
		status_t error = VMAreas::Init();
		if (error != B_OK)
			panic("vm_init: error initializing areas map\n");
	}

	VMAddressSpace::Init();
	reserve_boot_loader_ranges(args);

#if USE_DEBUG_HEAP_FOR_MALLOC || USE_GUARDED_HEAP_FOR_MALLOC
	heap_init_post_area();
#endif

	// Do any further initialization that the architecture dependant layers may
	// need now
	arch_vm_translation_map_init_post_area(args);
	arch_vm_init_post_area(args);
	vm_page_init_post_area(args);
	slab_init_post_area();

	// allocate areas to represent stuff that already exists

#if USE_DEBUG_HEAP_FOR_MALLOC || USE_GUARDED_HEAP_FOR_MALLOC
	address = (void*)ROUNDDOWN(heapBase, B_PAGE_SIZE);
	create_area("kernel heap", &address, B_EXACT_ADDRESS, heapSize,
		B_ALREADY_WIRED, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
#endif

	allocate_kernel_args(args);

	create_preloaded_image_areas(args->kernel_image);

	// allocate areas for preloaded images
	for (image = args->preloaded_images; image != NULL; image = image->next)
		create_preloaded_image_areas(image);

	// allocate kernel stacks
	for (i = 0; i < args->num_cpus; i++) {
		char name[64];

		sprintf(name, "idle thread %" B_PRIu32 " kstack", i + 1);
		address = (void*)args->cpu_kstack[i].start;
		create_area(name, &address, B_EXACT_ADDRESS, args->cpu_kstack[i].size,
			B_ALREADY_WIRED, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	}

	void* lastPage = (void*)ROUNDDOWN(~(addr_t)0, B_PAGE_SIZE);
	vm_block_address_range("overflow protection", lastPage, B_PAGE_SIZE);

#if PARANOID_KERNEL_MALLOC
	addr_t blockAddress = 0xcccccccc;
	if (blockAddress < KERNEL_BASE)
		blockAddress |= KERNEL_BASE;
	vm_block_address_range("uninitialized heap memory",
		(void *)ROUNDDOWN(blockAddress, B_PAGE_SIZE), B_PAGE_SIZE * 64);

#if B_HAIKU_64_BIT
	blockAddress = 0xcccccccccccccccc;
	if (blockAddress < KERNEL_BASE)
		blockAddress |= KERNEL_BASE;
	vm_block_address_range("uninitialized heap memory",
		(void *)ROUNDDOWN(blockAddress, B_PAGE_SIZE), B_PAGE_SIZE * 64);
#endif
#endif

#if PARANOID_KERNEL_FREE
	blockAddress = 0xdeadbeef;
	if (blockAddress < KERNEL_BASE)
		blockAddress |= KERNEL_BASE;
	vm_block_address_range("freed heap memory",
		(void *)ROUNDDOWN(blockAddress, B_PAGE_SIZE), B_PAGE_SIZE * 64);

#if B_HAIKU_64_BIT
	blockAddress = 0xdeadbeefdeadbeef;
	if (blockAddress < KERNEL_BASE)
		blockAddress |= KERNEL_BASE;
	vm_block_address_range("freed heap memory",
		(void *)ROUNDDOWN(blockAddress, B_PAGE_SIZE), B_PAGE_SIZE * 64);
#endif
#endif

	create_page_mappings_object_caches();

	vm_debug_init();

	TRACE(("vm_init: exit\n"));

	vm_cache_init_post_heap();

	return err;
}


status_t
vm_init_post_sem(kernel_args* args)
{
	// This frees all unused boot loader resources and makes its space available
	// again
	arch_vm_init_end(args);
	unreserve_boot_loader_ranges(args);

	// fill in all of the semaphores that were not allocated before
	// since we're still single threaded and only the kernel address space
	// exists, it isn't that hard to find all of the ones we need to create

	arch_vm_translation_map_init_post_sem(args);

	slab_init_post_sem();

#if USE_DEBUG_HEAP_FOR_MALLOC || USE_GUARDED_HEAP_FOR_MALLOC
	heap_init_post_sem();
#endif

	return B_OK;
}


status_t
vm_init_post_thread(kernel_args* args)
{
	vm_page_init_post_thread(args);
	slab_init_post_thread();
	return heap_init_post_thread();
}


status_t
vm_init_post_modules(kernel_args* args)
{
	return arch_vm_init_post_modules(args);
}


void
permit_page_faults(void)
{
	Thread* thread = thread_get_current_thread();
	if (thread != NULL)
		atomic_add(&thread->page_faults_allowed, 1);
}


void
forbid_page_faults(void)
{
	Thread* thread = thread_get_current_thread();
	if (thread != NULL)
		atomic_add(&thread->page_faults_allowed, -1);
}


status_t
vm_page_fault(addr_t address, addr_t faultAddress, bool isWrite, bool isExecute,
	bool isUser, addr_t* newIP)
{
	FTRACE(("vm_page_fault: page fault at 0x%lx, ip 0x%lx\n", address,
		faultAddress));

	TPF(PageFaultStart(address, isWrite, isUser, faultAddress));

	addr_t pageAddress = ROUNDDOWN(address, B_PAGE_SIZE);
	VMAddressSpace* addressSpace = NULL;

	status_t status = B_OK;
	*newIP = 0;
	atomic_add((int32*)&sPageFaults, 1);

	if (IS_KERNEL_ADDRESS(pageAddress)) {
		addressSpace = VMAddressSpace::GetKernel();
	} else if (IS_USER_ADDRESS(pageAddress)) {
		addressSpace = VMAddressSpace::GetCurrent();
		if (addressSpace == NULL) {
			if (!isUser) {
				dprintf("vm_page_fault: kernel thread accessing invalid user "
					"memory!\n");
				status = B_BAD_ADDRESS;
				TPF(PageFaultError(-1,
					VMPageFaultTracing
						::PAGE_FAULT_ERROR_KERNEL_BAD_USER_MEMORY));
			} else {
				// XXX weird state.
				panic("vm_page_fault: non kernel thread accessing user memory "
					"that doesn't exist!\n");
				status = B_BAD_ADDRESS;
			}
		}
	} else {
		// the hit was probably in the 64k DMZ between kernel and user space
		// this keeps a user space thread from passing a buffer that crosses
		// into kernel space
		status = B_BAD_ADDRESS;
		TPF(PageFaultError(-1,
			VMPageFaultTracing::PAGE_FAULT_ERROR_NO_ADDRESS_SPACE));
	}

	if (status == B_OK) {
		status = vm_soft_fault(addressSpace, pageAddress, isWrite, isExecute,
			isUser, NULL);
	}

	if (status < B_OK) {
		if (!isUser) {
			dprintf("vm_page_fault: vm_soft_fault returned error '%s' on fault at "
				"0x%lx, ip 0x%lx, write %d, kernel, exec %d, thread 0x%" B_PRIx32 "\n",
				strerror(status), address, faultAddress, isWrite, isExecute,
				thread_get_current_thread_id());

			Thread* thread = thread_get_current_thread();
			if (thread != NULL && thread->fault_handler != 0) {
				// this will cause the arch dependant page fault handler to
				// modify the IP on the interrupt frame or whatever to return
				// to this address
				*newIP = reinterpret_cast<uintptr_t>(thread->fault_handler);
			} else {
				// unhandled page fault in the kernel
				panic("vm_page_fault: unhandled page fault in kernel space at "
					"0x%lx, ip 0x%lx\n", address, faultAddress);
			}
		} else {
			Thread* thread = thread_get_current_thread();

#ifdef TRACE_FAULTS
			VMArea* area = NULL;
			if (addressSpace != NULL) {
				addressSpace->ReadLock();
				area = addressSpace->LookupArea(faultAddress);
			}

			dprintf("vm_page_fault: thread \"%s\" (%" B_PRId32 ") in team "
				"\"%s\" (%" B_PRId32 ") tried to %s address %#lx, ip %#lx "
				"(\"%s\" +%#lx)\n", thread->name, thread->id,
				thread->team->Name(), thread->team->id,
				isWrite ? "write" : (isExecute ? "execute" : "read"), address,
				faultAddress, area ? area->name : "???", faultAddress - (area ?
					area->Base() : 0x0));

			if (addressSpace != NULL)
				addressSpace->ReadUnlock();
#endif

			// If the thread has a signal handler for SIGSEGV, we simply
			// send it the signal. Otherwise we notify the user debugger
			// first.
			struct sigaction action;
			if ((sigaction(SIGSEGV, NULL, &action) == 0
					&& action.sa_handler != SIG_DFL
					&& action.sa_handler != SIG_IGN)
				|| user_debug_exception_occurred(B_SEGMENT_VIOLATION,
					SIGSEGV)) {
				Signal signal(SIGSEGV,
					status == B_PERMISSION_DENIED
						? SEGV_ACCERR : SEGV_MAPERR,
					EFAULT, thread->team->id);
				signal.SetAddress((void*)address);
				send_signal_to_thread(thread, signal, 0);
			}
		}
	}

	if (addressSpace != NULL)
		addressSpace->Put();

	return B_HANDLED_INTERRUPT;
}


struct PageFaultContext {
	AddressSpaceReadLocker	addressSpaceLocker;
	VMCacheChainLocker		cacheChainLocker;

	VMTranslationMap*		map;
	VMCache*				topCache;
	off_t					cacheOffset;
	vm_page_reservation		reservation;
	bool					isWrite;

	// return values
	vm_page*				page;
	bool					restart;
	bool					pageAllocated;


	PageFaultContext(VMAddressSpace* addressSpace, bool isWrite)
		:
		addressSpaceLocker(addressSpace, true),
		map(addressSpace->TranslationMap()),
		isWrite(isWrite)
	{
	}

	~PageFaultContext()
	{
		UnlockAll();
		vm_page_unreserve_pages(&reservation);
	}

	void Prepare(VMCache* topCache, off_t cacheOffset)
	{
		this->topCache = topCache;
		this->cacheOffset = cacheOffset;
		page = NULL;
		restart = false;
		pageAllocated = false;

		cacheChainLocker.SetTo(topCache);
	}

	void UnlockAll(VMCache* exceptCache = NULL)
	{
		topCache = NULL;
		addressSpaceLocker.Unlock();
		cacheChainLocker.Unlock(exceptCache);
	}
};


/*!	Gets the page that should be mapped into the area.
	Returns an error code other than \c B_OK, if the page couldn't be found or
	paged in. The locking state of the address space and the caches is undefined
	in that case.
	Returns \c B_OK with \c context.restart set to \c true, if the functions
	had to unlock the address space and all caches and is supposed to be called
	again.
	Returns \c B_OK with \c context.restart set to \c false, if the page was
	found. It is returned in \c context.page. The address space will still be
	locked as well as all caches starting from the top cache to at least the
	cache the page lives in.
*/
static status_t
fault_get_page(PageFaultContext& context)
{
	VMCache* cache = context.topCache;
	vm_page* page = NULL;

	while (cache != NULL) {
		// We already hold the lock of the cache at this point.

		page = cache->LookupPage(context.cacheOffset);
		if (page != NULL && page->busy) {
			// page must be busy -- wait for it to become unbusy
			context.UnlockAll(cache);
			cache->ReleaseRefLocked();
			cache->WaitForPageEvents(page, PAGE_EVENT_NOT_BUSY, false);

			// restart the whole process
			context.restart = true;
			return B_OK;
		}

		if (page != NULL)
			break;

		// The current cache does not contain the page we're looking for.

		// see if the backing store has it
		if (cache->StoreHasPage(context.cacheOffset)) {
			// insert a fresh page and mark it busy -- we're going to read it in
			page = vm_page_allocate_page(&context.reservation,
				PAGE_STATE_ACTIVE | VM_PAGE_ALLOC_BUSY);
			cache->InsertPage(page, context.cacheOffset);

			// We need to unlock all caches and the address space while reading
			// the page in. Keep a reference to the cache around.
			cache->AcquireRefLocked();
			context.UnlockAll();

			// read the page in
			generic_io_vec vec;
			vec.base = (phys_addr_t)page->physical_page_number * B_PAGE_SIZE;
			generic_size_t bytesRead = vec.length = B_PAGE_SIZE;

			status_t status = cache->Read(context.cacheOffset, &vec, 1,
				B_PHYSICAL_IO_REQUEST, &bytesRead);

			cache->Lock();

			if (status < B_OK) {
				// on error remove and free the page
				dprintf("reading page from cache %p returned: %s!\n",
					cache, strerror(status));

				cache->NotifyPageEvents(page, PAGE_EVENT_NOT_BUSY);
				cache->RemovePage(page);
				vm_page_free_etc(cache, page, &context.reservation);

				cache->ReleaseRefAndUnlock();
				return status;
			}

			// mark the page unbusy again
			cache->MarkPageUnbusy(page);

			DEBUG_PAGE_ACCESS_END(page);

			// Since we needed to unlock everything temporarily, the area
			// situation might have changed. So we need to restart the whole
			// process.
			cache->ReleaseRefAndUnlock();
			context.restart = true;
			return B_OK;
		}

		cache = context.cacheChainLocker.LockSourceCache();
	}

	if (page == NULL) {
		// There was no adequate page. Insert a clean one into the topmost cache.
		cache = context.topCache;

		// We don't need the other caches anymore.
		context.cacheChainLocker.Unlock(context.topCache);
		context.cacheChainLocker.SetTo(context.topCache);

		// allocate a clean page
		page = vm_page_allocate_page(&context.reservation,
			PAGE_STATE_ACTIVE | VM_PAGE_ALLOC_CLEAR);
		FTRACE(("vm_soft_fault: just allocated page 0x%" B_PRIxPHYSADDR "\n",
			page->physical_page_number));

		// insert the new page into our cache
		cache->InsertPage(page, context.cacheOffset);
		context.pageAllocated = true;
	} else if (page->Cache() != context.topCache && context.isWrite) {
		// We have a page that has the data we want, but in the wrong cache
		// object so we need to copy it and stick it into the top cache.
		vm_page* sourcePage = page;

		// TODO: If memory is low, it might be a good idea to steal the page
		// from our source cache -- if possible, that is.
		FTRACE(("get new page, copy it, and put it into the topmost cache\n"));
		page = vm_page_allocate_page(&context.reservation, PAGE_STATE_ACTIVE);

		// To not needlessly kill concurrency we unlock all caches but the top
		// one while copying the page. Lacking another mechanism to ensure that
		// the source page doesn't disappear, we mark it busy.
		sourcePage->busy = true;
		context.cacheChainLocker.UnlockKeepRefs(true);

		// copy the page
		vm_memcpy_physical_page(page->physical_page_number * B_PAGE_SIZE,
			sourcePage->physical_page_number * B_PAGE_SIZE);

		context.cacheChainLocker.RelockCaches(true);
		sourcePage->Cache()->MarkPageUnbusy(sourcePage);
		sourcePage->Cache()->IncrementCopiedPagesCount();

		// insert the new page into our cache
		context.topCache->InsertPage(page, context.cacheOffset);
		context.pageAllocated = true;
	} else
		DEBUG_PAGE_ACCESS_START(page);

	context.page = page;
	return B_OK;
}


/*!	Makes sure the address in the given address space is mapped.

	\param addressSpace The address space.
	\param originalAddress The address. Doesn't need to be page aligned.
	\param isWrite If \c true the address shall be write-accessible.
	\param isUser If \c true the access is requested by a userland team.
	\param wirePage On success, if non \c NULL, the wired count of the page
		mapped at the given address is incremented and the page is returned
		via this parameter.
	\return \c B_OK on success, another error code otherwise.
*/
static status_t
vm_soft_fault(VMAddressSpace* addressSpace, addr_t originalAddress,
	bool isWrite, bool isExecute, bool isUser, vm_page** wirePage)
{
	FTRACE(("vm_soft_fault: thid 0x%" B_PRIx32 " address 0x%" B_PRIxADDR ", "
		"isWrite %d, isUser %d\n", thread_get_current_thread_id(),
		originalAddress, isWrite, isUser));

	PageFaultContext context(addressSpace, isWrite);

	addr_t address = ROUNDDOWN(originalAddress, B_PAGE_SIZE);
	status_t status = B_OK;

	addressSpace->IncrementFaultCount();

	// We may need up to 2 pages plus pages needed for mapping them -- reserving
	// the pages upfront makes sure we don't have any cache locked, so that the
	// page daemon/thief can do their job without problems.
	size_t reservePages = 2 + context.map->MaxPagesNeededToMap(originalAddress,
		originalAddress);
	context.addressSpaceLocker.Unlock();
	vm_page_reserve_pages(&context.reservation, reservePages,
		addressSpace == VMAddressSpace::Kernel()
			? VM_PRIORITY_SYSTEM : VM_PRIORITY_USER);

#ifdef TRACE_FAULTS
	const bool logFaults = true;
#else
	const bool logFaults = !isUser;
#endif
	while (true) {
		context.addressSpaceLocker.Lock();

		// get the area the fault was in
		VMArea* area = addressSpace->LookupArea(address);
		if (area == NULL) {
			if (logFaults) {
				dprintf("vm_soft_fault: va 0x%lx not covered by area in address "
					"space\n", originalAddress);
			}
			TPF(PageFaultError(-1,
				VMPageFaultTracing::PAGE_FAULT_ERROR_NO_AREA));
			status = B_BAD_ADDRESS;
			break;
		}

		// check permissions
		uint32 protection = get_area_page_protection(area, address);
		if (isUser && (protection & B_USER_PROTECTION) == 0
				&& (area->protection & B_KERNEL_AREA) != 0) {
			if (logFaults) {
				dprintf("user access on kernel area 0x%" B_PRIx32 " at %p\n",
					area->id, (void*)originalAddress);
			}
			TPF(PageFaultError(area->id,
				VMPageFaultTracing::PAGE_FAULT_ERROR_KERNEL_ONLY));
			status = B_PERMISSION_DENIED;
			break;
		}
		if (isWrite && (protection
				& (B_WRITE_AREA | (isUser ? 0 : B_KERNEL_WRITE_AREA))) == 0) {
			if (logFaults) {
				dprintf("write access attempted on write-protected area 0x%"
					B_PRIx32 " at %p\n", area->id, (void*)originalAddress);
			}
			TPF(PageFaultError(area->id,
				VMPageFaultTracing::PAGE_FAULT_ERROR_WRITE_PROTECTED));
			status = B_PERMISSION_DENIED;
			break;
		} else if (isExecute && (protection
				& (B_EXECUTE_AREA | (isUser ? 0 : B_KERNEL_EXECUTE_AREA))) == 0) {
			if (logFaults) {
				dprintf("instruction fetch attempted on execute-protected area 0x%"
					B_PRIx32 " at %p\n", area->id, (void*)originalAddress);
			}
			TPF(PageFaultError(area->id,
				VMPageFaultTracing::PAGE_FAULT_ERROR_EXECUTE_PROTECTED));
			status = B_PERMISSION_DENIED;
			break;
		} else if (!isWrite && !isExecute && (protection
				& (B_READ_AREA | (isUser ? 0 : B_KERNEL_READ_AREA))) == 0) {
			if (logFaults) {
				dprintf("read access attempted on read-protected area 0x%" B_PRIx32
					" at %p\n", area->id, (void*)originalAddress);
			}
			TPF(PageFaultError(area->id,
				VMPageFaultTracing::PAGE_FAULT_ERROR_READ_PROTECTED));
			status = B_PERMISSION_DENIED;
			break;
		}

		// We have the area, it was a valid access, so let's try to resolve the
		// page fault now.
		// At first, the top most cache from the area is investigated.

		context.Prepare(vm_area_get_locked_cache(area),
			address - area->Base() + area->cache_offset);

		// See if this cache has a fault handler -- this will do all the work
		// for us.
		{
			// Note, since the page fault is resolved with interrupts enabled,
			// the fault handler could be called more than once for the same
			// reason -- the store must take this into account.
			status = context.topCache->Fault(addressSpace, context.cacheOffset);
			if (status != B_BAD_HANDLER)
				break;
		}

		// The top most cache has no fault handler, so let's see if the cache or
		// its sources already have the page we're searching for (we're going
		// from top to bottom).
		status = fault_get_page(context);
		if (status != B_OK) {
			TPF(PageFaultError(area->id, status));
			break;
		}

		if (context.restart)
			continue;

		// All went fine, all there is left to do is to map the page into the
		// address space.
		TPF(PageFaultDone(area->id, context.topCache, context.page->Cache(),
			context.page));

		// If the page doesn't reside in the area's cache, we need to make sure
		// it's mapped in read-only, so that we cannot overwrite someone else's
		// data (copy-on-write)
		uint32 newProtection = protection;
		if (context.page->Cache() != context.topCache && !isWrite)
			newProtection &= ~(B_WRITE_AREA | B_KERNEL_WRITE_AREA);

		bool unmapPage = false;
		bool mapPage = true;

		// check whether there's already a page mapped at the address
		context.map->Lock();

		phys_addr_t physicalAddress;
		uint32 flags;
		vm_page* mappedPage = NULL;
		if (context.map->Query(address, &physicalAddress, &flags) == B_OK
			&& (flags & PAGE_PRESENT) != 0
			&& (mappedPage = vm_lookup_page(physicalAddress / B_PAGE_SIZE))
				!= NULL) {
			// Yep there's already a page. If it's ours, we can simply adjust
			// its protection. Otherwise we have to unmap it.
			if (mappedPage == context.page) {
				context.map->ProtectPage(area, address, newProtection);
					// Note: We assume that ProtectPage() is atomic (i.e.
					// the page isn't temporarily unmapped), otherwise we'd have
					// to make sure it isn't wired.
				mapPage = false;
			} else
				unmapPage = true;
		}

		context.map->Unlock();

		if (unmapPage) {
			// If the page is wired, we can't unmap it. Wait until it is unwired
			// again and restart. Note that the page cannot be wired for
			// writing, since it it isn't in the topmost cache. So we can safely
			// ignore ranges wired for writing (our own and other concurrent
			// wiring attempts in progress) and in fact have to do that to avoid
			// a deadlock.
			VMAreaUnwiredWaiter waiter;
			if (area->AddWaiterIfWired(&waiter, address, B_PAGE_SIZE,
					VMArea::IGNORE_WRITE_WIRED_RANGES)) {
				// unlock everything and wait
				if (context.pageAllocated) {
					// ... but since we allocated a page and inserted it into
					// the top cache, remove and free it first. Otherwise we'd
					// have a page from a lower cache mapped while an upper
					// cache has a page that would shadow it.
					context.topCache->RemovePage(context.page);
					vm_page_free_etc(context.topCache, context.page,
						&context.reservation);
				} else
					DEBUG_PAGE_ACCESS_END(context.page);

				context.UnlockAll();
				waiter.waitEntry.Wait();
				continue;
			}

			// Note: The mapped page is a page of a lower cache. We are
			// guaranteed to have that cached locked, our new page is a copy of
			// that page, and the page is not busy. The logic for that guarantee
			// is as follows: Since the page is mapped, it must live in the top
			// cache (ruled out above) or any of its lower caches, and there is
			// (was before the new page was inserted) no other page in any
			// cache between the top cache and the page's cache (otherwise that
			// would be mapped instead). That in turn means that our algorithm
			// must have found it and therefore it cannot be busy either.
			DEBUG_PAGE_ACCESS_START(mappedPage);
			unmap_page(area, address);
			DEBUG_PAGE_ACCESS_END(mappedPage);
		}

		if (mapPage) {
			if (map_page(area, context.page, address, newProtection,
					&context.reservation) != B_OK) {
				// Mapping can only fail, when the page mapping object couldn't
				// be allocated. Save for the missing mapping everything is
				// fine, though. If this was a regular page fault, we'll simply
				// leave and probably fault again. To make sure we'll have more
				// luck then, we ensure that the minimum object reserve is
				// available.
				DEBUG_PAGE_ACCESS_END(context.page);

				context.UnlockAll();

				if (object_cache_reserve(page_mapping_object_cache_for(
							context.page->physical_page_number), 1, 0)
						!= B_OK) {
					// Apparently the situation is serious. (We ought to have
					// blocked waiting for pages or failed to reserve memory
					// before now.)
					panic("vm_soft_fault: failed to allocate mapping object for page %p",
						context.page);
					status = B_NO_MEMORY;
				} else if (wirePage != NULL) {
					// The caller expects us to wire the page. Since
					// object_cache_reserve() succeeded, we should now be able
					// to allocate a mapping structure. Restart.
					continue;
				}

				break;
			}
		} else if (context.page->State() == PAGE_STATE_INACTIVE)
			vm_page_set_state(context.page, PAGE_STATE_ACTIVE);

		// also wire the page, if requested
		if (wirePage != NULL && status == B_OK) {
			increment_page_wired_count(context.page);
			*wirePage = context.page;
		}

		context.page->Cache()->IncrementFaultCount();

		DEBUG_PAGE_ACCESS_END(context.page);
		break;
	}

	return status;
}


status_t
vm_get_physical_page(phys_addr_t paddr, addr_t* _vaddr, void** _handle)
{
	return sPhysicalPageMapper->GetPage(paddr, _vaddr, _handle);
}

status_t
vm_put_physical_page(addr_t vaddr, void* handle)
{
	return sPhysicalPageMapper->PutPage(vaddr, handle);
}


status_t
vm_get_physical_page_current_cpu(phys_addr_t paddr, addr_t* _vaddr,
	void** _handle)
{
	return sPhysicalPageMapper->GetPageCurrentCPU(paddr, _vaddr, _handle);
}

status_t
vm_put_physical_page_current_cpu(addr_t vaddr, void* handle)
{
	return sPhysicalPageMapper->PutPageCurrentCPU(vaddr, handle);
}


status_t
vm_get_physical_page_debug(phys_addr_t paddr, addr_t* _vaddr, void** _handle)
{
	return sPhysicalPageMapper->GetPageDebug(paddr, _vaddr, _handle);
}

status_t
vm_put_physical_page_debug(addr_t vaddr, void* handle)
{
	return sPhysicalPageMapper->PutPageDebug(vaddr, handle);
}


void
vm_get_info(system_info* info)
{
	swap_get_info(info);

	InterruptsWriteSpinLocker locker(sAvailableMemoryLock);
	info->needed_memory = sNeededMemory;
	info->free_memory = sAvailableMemory;
}


uint32
vm_num_page_faults(void)
{
	return sPageFaults;
}


off_t
vm_available_memory(void)
{
	InterruptsWriteSpinLocker locker(sAvailableMemoryLock);
	return sAvailableMemory;
}


/*!	Like vm_available_memory(), but only for use in the kernel
	debugger.
*/
off_t
vm_available_memory_debug(void)
{
	return sAvailableMemory;
}


off_t
vm_available_not_needed_memory(void)
{
	InterruptsWriteSpinLocker locker(sAvailableMemoryLock);
	return sAvailableMemory - sNeededMemory;
}


/*!	Like vm_available_not_needed_memory(), but only for use in the kernel
	debugger.
*/
off_t
vm_available_not_needed_memory_debug(void)
{
	return sAvailableMemory - sNeededMemory;
}


size_t
vm_kernel_address_space_left(void)
{
	return VMAddressSpace::Kernel()->FreeSpace();
}


void
vm_unreserve_memory(size_t amount)
{
	ASSERT((amount % B_PAGE_SIZE) == 0);

	if (amount == 0)
		return;

	InterruptsReadSpinLocker readLocker(sAvailableMemoryLock);
	atomic_add64(&sAvailableMemory, amount);
}


status_t
vm_try_reserve_memory(size_t amount, int priority, bigtime_t timeout)
{
	ASSERT((amount % B_PAGE_SIZE) == 0);
	ASSERT(priority >= 0 && priority < (int)B_COUNT_OF(kMemoryReserveForPriority));
	TRACE(("try to reserve %lu bytes, %Lu left\n", amount, sAvailableMemory));

	const size_t reserve = kMemoryReserveForPriority[priority];
	const off_t amountPlusReserve = amount + reserve;

	// Try with a read-lock and atomics first, but only if there's more than double
	// the amount of memory we're trying to reserve available, to avoid races.
	InterruptsReadSpinLocker readLocker(sAvailableMemoryLock);
	if (atomic_get64(&sAvailableMemory) > (off_t)(amountPlusReserve + amount)) {
		if (atomic_add64(&sAvailableMemory, -amount) >= amountPlusReserve)
			return B_OK;

		// There wasn't actually enough, we must've raced. Undo what we just did.
		atomic_add64(&sAvailableMemory, amount);
	}
	readLocker.Unlock();

	InterruptsWriteSpinLocker writeLocker(sAvailableMemoryLock);

	if (sAvailableMemory >= amountPlusReserve) {
		sAvailableMemory -= amount;
		return B_OK;
	}

	if (timeout <= 0)
		return B_NO_MEMORY;

	// turn timeout into an absolute timeout
	timeout += system_time();

	// loop until we're out of retries or the timeout occurs
	int32 retries = 3;
	do {
		sNeededMemory += amount;

		// call the low resource manager
		uint64 requirement = sNeededMemory - (sAvailableMemory - reserve);
		writeLocker.Unlock();
		low_resource(B_KERNEL_RESOURCE_MEMORY, requirement,
			B_ABSOLUTE_TIMEOUT, timeout);
		writeLocker.Lock();

		sNeededMemory -= amount;

		if (sAvailableMemory >= amountPlusReserve) {
			sAvailableMemory -= amount;
			return B_OK;
		}
	} while (--retries > 0 && timeout > system_time());

	return B_NO_MEMORY;
}


status_t
vm_set_area_memory_type(area_id id, phys_addr_t physicalBase, uint32 type)
{
	// NOTE: The caller is responsible for synchronizing calls to this function!

	AddressSpaceReadLocker locker;
	VMArea* area;
	status_t status = locker.SetFromArea(id, area);
	if (status != B_OK)
		return status;

	// nothing to do, if the type doesn't change
	uint32 oldType = area->MemoryType();
	if (type == oldType)
		return B_OK;

	// set the memory type of the area and the mapped pages
	VMTranslationMap* map = area->address_space->TranslationMap();
	map->Lock();
	area->SetMemoryType(type);
	map->ProtectArea(area, area->protection);
	map->Unlock();

	// set the physical memory type
	status_t error = arch_vm_set_memory_type(area, physicalBase, type, NULL);
	if (error != B_OK) {
		// reset the memory type of the area and the mapped pages
		map->Lock();
		area->SetMemoryType(oldType);
		map->ProtectArea(area, area->protection);
		map->Unlock();
		return error;
	}

	return B_OK;

}


/*!	This function enforces some protection properties:
	 - kernel areas must be W^X (after kernel startup)
	 - if B_WRITE_AREA is set, B_KERNEL_WRITE_AREA is set as well
	 - if B_READ_AREA has been set, B_KERNEL_READ_AREA is also set
*/
static void
fix_protection(uint32* protection)
{
	if ((*protection & B_KERNEL_EXECUTE_AREA) != 0
		&& ((*protection & B_KERNEL_WRITE_AREA) != 0
			|| (*protection & B_WRITE_AREA) != 0)
		&& !gKernelStartup)
		panic("kernel areas cannot be both writable and executable!");

	if ((*protection & B_KERNEL_PROTECTION) == 0) {
		if ((*protection & B_WRITE_AREA) != 0)
			*protection |= B_KERNEL_WRITE_AREA;
		if ((*protection & B_READ_AREA) != 0)
			*protection |= B_KERNEL_READ_AREA;
	}
}


static void
fill_area_info(struct VMArea* area, area_info* info, size_t size)
{
	strlcpy(info->name, area->name, B_OS_NAME_LENGTH);
	info->area = area->id;
	info->address = (void*)area->Base();
	info->size = area->Size();
	info->protection = area->protection;
	info->lock = area->wiring;
	info->team = area->address_space->ID();

	VMCache* cache = vm_area_get_locked_cache(area);

	// Note, this is a simplification; the cache could be larger than this area
	info->ram_size = cache->page_count * B_PAGE_SIZE;

	info->copy_count = cache->CopiedPagesCount();

	info->in_count = 0;
	info->out_count = 0;
		// TODO: retrieve real values here!

	vm_area_put_locked_cache(cache);
}


static status_t
vm_resize_area(area_id areaID, size_t newSize, bool kernel)
{
	// is newSize a multiple of B_PAGE_SIZE?
	if ((newSize & (B_PAGE_SIZE - 1)) != 0)
		return B_BAD_VALUE;

	// lock all affected address spaces and the cache
	VMArea* area;
	VMCache* cache;

	MultiAddressSpaceLocker locker;
	AreaCacheLocker cacheLocker;

	status_t status;
	size_t oldSize;
	bool anyKernelArea;
	bool restart;

	do {
		anyKernelArea = false;
		restart = false;

		locker.Unset();
		status = locker.AddAreaCacheAndLock(areaID, true, true, area, &cache);
		if (status != B_OK)
			return status;
		cacheLocker.SetTo(cache, true);	// already locked

		// enforce restrictions
		const team_id team = team_get_current_team_id();
		if (!kernel && (area->address_space == VMAddressSpace::Kernel()
				|| (area->protection & B_KERNEL_AREA) != 0)) {
			dprintf("vm_resize_area: team %" B_PRId32 " tried to "
				"resize kernel area %" B_PRId32 " (%s)\n",
				team, areaID, area->name);
			return B_NOT_ALLOWED;
		}
		if (!kernel && area->address_space->ID() != team) {
			// unless you're the kernel, you're only allowed to resize your own areas
			return B_NOT_ALLOWED;
		}

		oldSize = area->Size();
		if (newSize == oldSize)
			return B_OK;

		if (cache->type != CACHE_TYPE_RAM)
			return B_NOT_ALLOWED;

		if (oldSize < newSize) {
			// We need to check if all areas of this cache can be resized.
			for (VMArea* current = cache->areas.First(); current != NULL;
					current = cache->areas.GetNext(current)) {
				if (!current->address_space->CanResizeArea(current, newSize))
					return B_ERROR;
				anyKernelArea
					|= current->address_space == VMAddressSpace::Kernel();
			}
		} else {
			// We're shrinking the areas, so we must make sure the affected
			// ranges are not wired.
			for (VMArea* current = cache->areas.First(); current != NULL;
					current = cache->areas.GetNext(current)) {
				anyKernelArea
					|= current->address_space == VMAddressSpace::Kernel();

				if (wait_if_area_range_is_wired(current,
						current->Base() + newSize, oldSize - newSize, &locker,
						&cacheLocker)) {
					restart = true;
					break;
				}
			}
		}
	} while (restart);

	// Okay, looks good so far, so let's do it

	int priority = kernel && anyKernelArea
		? VM_PRIORITY_SYSTEM : VM_PRIORITY_USER;
	uint32 allocationFlags = kernel && anyKernelArea
		? HEAP_DONT_WAIT_FOR_MEMORY | HEAP_DONT_LOCK_KERNEL_SPACE : 0;

	if (oldSize < newSize) {
		// Growing the cache can fail, so we do it first.
		status = cache->Resize(cache->virtual_base + newSize, priority);
		if (status != B_OK)
			return status;
	}

	for (VMArea* current = cache->areas.First(); current != NULL;
			current = cache->areas.GetNext(current)) {
		status = current->address_space->ResizeArea(current, newSize,
			allocationFlags);
		if (status != B_OK)
			break;

		// We also need to unmap all pages beyond the new size, if the area has
		// shrunk
		if (newSize < oldSize) {
			VMCacheChainLocker cacheChainLocker(cache);
			cacheChainLocker.LockAllSourceCaches();

			unmap_pages(current, current->Base() + newSize,
				oldSize - newSize);

			cacheChainLocker.Unlock(cache);
		}
	}

	if (status == B_OK) {
		// Shrink or grow individual page protections if in use.
		if (area->page_protections != NULL) {
			size_t bytes = area_page_protections_size(newSize);
			uint8* newProtections
				= (uint8*)realloc(area->page_protections, bytes);
			if (newProtections == NULL)
				status = B_NO_MEMORY;
			else {
				area->page_protections = newProtections;

				if (oldSize < newSize) {
					// init the additional page protections to that of the area
					uint32 offset = area_page_protections_size(oldSize);
					uint32 areaProtection = area->protection
						& (B_READ_AREA | B_WRITE_AREA | B_EXECUTE_AREA);
					memset(area->page_protections + offset,
						areaProtection | (areaProtection << 4), bytes - offset);
					if ((oldSize / B_PAGE_SIZE) % 2 != 0) {
						uint8& entry = area->page_protections[offset - 1];
						entry = (entry & 0x0f) | (areaProtection << 4);
					}
				}
			}
		}
	}

	// shrinking the cache can't fail, so we do it now
	if (status == B_OK && newSize < oldSize)
		status = cache->Resize(cache->virtual_base + newSize, priority);

	if (status != B_OK) {
		// Something failed -- resize the areas back to their original size.
		// This can fail, too, in which case we're seriously screwed.
		for (VMArea* current = cache->areas.First(); current != NULL;
				current = cache->areas.GetNext(current)) {
			if (current->address_space->ResizeArea(current, oldSize,
					allocationFlags) != B_OK) {
				panic("vm_resize_area(): Failed and not being able to restore "
					"original state.");
			}
		}

		cache->Resize(cache->virtual_base + oldSize, priority);
	}

	// TODO: we must honour the lock restrictions of this area
	return status;
}


status_t
vm_memset_physical(phys_addr_t address, int value, phys_size_t length)
{
	return sPhysicalPageMapper->MemsetPhysical(address, value, length);
}


status_t
vm_memcpy_from_physical(void* to, phys_addr_t from, size_t length, bool user)
{
	return sPhysicalPageMapper->MemcpyFromPhysical(to, from, length, user);
}


status_t
vm_memcpy_to_physical(phys_addr_t to, const void* _from, size_t length,
	bool user)
{
	return sPhysicalPageMapper->MemcpyToPhysical(to, _from, length, user);
}


void
vm_memcpy_physical_page(phys_addr_t to, phys_addr_t from)
{
	return sPhysicalPageMapper->MemcpyPhysicalPage(to, from);
}


/** Validate that a memory range is either fully in kernel space, or fully in
 *  userspace */
static inline bool
validate_memory_range(const void* addr, size_t size)
{
	addr_t address = (addr_t)addr;

	// Check for overflows on all addresses.
	if ((address + size) < address)
		return false;

	// Validate that the address range does not cross the kernel/user boundary.
	return IS_USER_ADDRESS(address) == IS_USER_ADDRESS(address + size - 1);
}


//	#pragma mark - kernel public API


status_t
user_memcpy(void* to, const void* from, size_t size)
{
	if (!validate_memory_range(to, size) || !validate_memory_range(from, size))
		return B_BAD_ADDRESS;

	if (arch_cpu_user_memcpy(to, from, size) < B_OK)
		return B_BAD_ADDRESS;

	return B_OK;
}


/*!	\brief Copies at most (\a size - 1) characters from the string in \a from to
	the string in \a to, NULL-terminating the result.

	\param to Pointer to the destination C-string.
	\param from Pointer to the source C-string.
	\param size Size in bytes of the string buffer pointed to by \a to.

	\return strlen(\a from).
*/
ssize_t
user_strlcpy(char* to, const char* from, size_t size)
{
	if (to == NULL && size != 0)
		return B_BAD_VALUE;
	if (from == NULL)
		return B_BAD_ADDRESS;

	// Protect the source address from overflows.
	size_t maxSize = size;
	if ((addr_t)from + maxSize < (addr_t)from)
		maxSize -= (addr_t)from + maxSize;
	if (IS_USER_ADDRESS(from) && !IS_USER_ADDRESS((addr_t)from + maxSize))
		maxSize = USER_TOP - (addr_t)from;

	if (!validate_memory_range(to, maxSize))
		return B_BAD_ADDRESS;

	ssize_t result = arch_cpu_user_strlcpy(to, from, maxSize);
	if (result < 0)
		return result;

	// If we hit the address overflow boundary, fail.
	if ((size_t)result >= maxSize && maxSize < size)
		return B_BAD_ADDRESS;

	return result;
}


status_t
user_memset(void* s, char c, size_t count)
{
	if (!validate_memory_range(s, count))
		return B_BAD_ADDRESS;

	if (arch_cpu_user_memset(s, c, count) < B_OK)
		return B_BAD_ADDRESS;

	return B_OK;
}


/*!	Wires a single page at the given address.

	\param team The team whose address space the address belongs to. Supports
		also \c B_CURRENT_TEAM. If the given address is a kernel address, the
		parameter is ignored.
	\param address address The virtual address to wire down. Does not need to
		be page aligned.
	\param writable If \c true the page shall be writable.
	\param info On success the info is filled in, among other things
		containing the physical address the given virtual one translates to.
	\return \c B_OK, when the page could be wired, another error code otherwise.
*/
status_t
vm_wire_page(team_id team, addr_t address, bool writable,
	VMPageWiringInfo* info)
{
	addr_t pageAddress = ROUNDDOWN((addr_t)address, B_PAGE_SIZE);
	info->range.SetTo(pageAddress, B_PAGE_SIZE, writable, false);

	// compute the page protection that is required
	bool isUser = IS_USER_ADDRESS(address);
	uint32 requiredProtection = PAGE_PRESENT
		| B_KERNEL_READ_AREA | (isUser ? B_READ_AREA : 0);
	if (writable)
		requiredProtection |= B_KERNEL_WRITE_AREA | (isUser ? B_WRITE_AREA : 0);

	// get and read lock the address space
	VMAddressSpace* addressSpace = NULL;
	if (isUser) {
		if (team == B_CURRENT_TEAM)
			addressSpace = VMAddressSpace::GetCurrent();
		else
			addressSpace = VMAddressSpace::Get(team);
	} else
		addressSpace = VMAddressSpace::GetKernel();
	if (addressSpace == NULL)
		return B_ERROR;

	AddressSpaceReadLocker addressSpaceLocker(addressSpace, true);

	VMTranslationMap* map = addressSpace->TranslationMap();
	status_t error = B_OK;

	// get the area
	VMArea* area = addressSpace->LookupArea(pageAddress);
	if (area == NULL) {
		addressSpace->Put();
		return B_BAD_ADDRESS;
	}

	// Lock the area's top cache. This is a requirement for VMArea::Wire().
	VMCacheChainLocker cacheChainLocker(vm_area_get_locked_cache(area));

	// mark the area range wired
	area->Wire(&info->range);

	// Lock the area's cache chain and the translation map. Needed to look
	// up the page and play with its wired count.
	cacheChainLocker.LockAllSourceCaches();
	map->Lock();

	phys_addr_t physicalAddress;
	uint32 flags;
	vm_page* page;
	if (map->Query(pageAddress, &physicalAddress, &flags) == B_OK
		&& (flags & requiredProtection) == requiredProtection
		&& (page = vm_lookup_page(physicalAddress / B_PAGE_SIZE))
			!= NULL) {
		// Already mapped with the correct permissions -- just increment
		// the page's wired count.
		increment_page_wired_count(page);

		map->Unlock();
		cacheChainLocker.Unlock();
		addressSpaceLocker.Unlock();
	} else {
		// Let vm_soft_fault() map the page for us, if possible. We need
		// to fully unlock to avoid deadlocks. Since we have already
		// wired the area itself, nothing disturbing will happen with it
		// in the meantime.
		map->Unlock();
		cacheChainLocker.Unlock();
		addressSpaceLocker.Unlock();

		error = vm_soft_fault(addressSpace, pageAddress, writable, false,
			isUser, &page);

		if (error != B_OK) {
			// The page could not be mapped -- clean up.
			VMCache* cache = vm_area_get_locked_cache(area);
			area->Unwire(&info->range);
			cache->ReleaseRefAndUnlock();
			addressSpace->Put();
			return error;
		}
	}

	info->physicalAddress
		= (phys_addr_t)page->physical_page_number * B_PAGE_SIZE
			+ address % B_PAGE_SIZE;
	info->page = page;

	return B_OK;
}


/*!	Unwires a single page previously wired via vm_wire_page().

	\param info The same object passed to vm_wire_page() before.
*/
void
vm_unwire_page(VMPageWiringInfo* info)
{
	// lock the address space
	VMArea* area = info->range.area;
	AddressSpaceReadLocker addressSpaceLocker(area->address_space, false);
		// takes over our reference

	// lock the top cache
	VMCache* cache = vm_area_get_locked_cache(area);
	VMCacheChainLocker cacheChainLocker(cache);

	if (info->page->Cache() != cache) {
		// The page is not in the top cache, so we lock the whole cache chain
		// before touching the page's wired count.
		cacheChainLocker.LockAllSourceCaches();
	}

	decrement_page_wired_count(info->page);

	// remove the wired range from the range
	area->Unwire(&info->range);

	cacheChainLocker.Unlock();
}


/*!	Wires down the given address range in the specified team's address space.

	If successful the function
	- acquires a reference to the specified team's address space,
	- adds respective wired ranges to all areas that intersect with the given
	  address range,
	- makes sure all pages in the given address range are mapped with the
	  requested access permissions and increments their wired count.

	It fails, when \a team doesn't specify a valid address space, when any part
	of the specified address range is not covered by areas, when the concerned
	areas don't allow mapping with the requested permissions, or when mapping
	failed for another reason.

	When successful the call must be balanced by a unlock_memory_etc() call with
	the exact same parameters.

	\param team Identifies the address (via team ID). \c B_CURRENT_TEAM is
		supported.
	\param address The start of the address range to be wired.
	\param numBytes The size of the address range to be wired.
	\param flags Flags. Currently only \c B_READ_DEVICE is defined, which
		requests that the range must be wired writable ("read from device
		into memory").
	\return \c B_OK on success, another error code otherwise.
*/
status_t
lock_memory_etc(team_id team, void* address, size_t numBytes, uint32 flags)
{
	addr_t lockBaseAddress = ROUNDDOWN((addr_t)address, B_PAGE_SIZE);
	addr_t lockEndAddress = ROUNDUP((addr_t)address + numBytes, B_PAGE_SIZE);

	// compute the page protection that is required
	bool isUser = IS_USER_ADDRESS(address);
	bool writable = (flags & B_READ_DEVICE) == 0;
	uint32 requiredProtection = PAGE_PRESENT
		| B_KERNEL_READ_AREA | (isUser ? B_READ_AREA : 0);
	if (writable)
		requiredProtection |= B_KERNEL_WRITE_AREA | (isUser ? B_WRITE_AREA : 0);

	uint32 mallocFlags = isUser
		? 0 : HEAP_DONT_WAIT_FOR_MEMORY | HEAP_DONT_LOCK_KERNEL_SPACE;

	// get and read lock the address space
	VMAddressSpace* addressSpace = NULL;
	if (isUser) {
		if (team == B_CURRENT_TEAM)
			addressSpace = VMAddressSpace::GetCurrent();
		else
			addressSpace = VMAddressSpace::Get(team);
	} else
		addressSpace = VMAddressSpace::GetKernel();
	if (addressSpace == NULL)
		return B_ERROR;

	AddressSpaceReadLocker addressSpaceLocker(addressSpace, true);
		// We get a new address space reference here. The one we got above will
		// be freed by unlock_memory_etc().

	VMTranslationMap* map = addressSpace->TranslationMap();
	status_t error = B_OK;

	// iterate through all concerned areas
	addr_t nextAddress = lockBaseAddress;
	while (nextAddress != lockEndAddress) {
		// get the next area
		VMArea* area = addressSpace->LookupArea(nextAddress);
		if (area == NULL) {
			error = B_BAD_ADDRESS;
			break;
		}

		addr_t areaStart = nextAddress;
		addr_t areaEnd = std::min(lockEndAddress, area->Base() + area->Size());

		// allocate the wired range (do that before locking the cache to avoid
		// deadlocks)
		VMAreaWiredRange* range = new(malloc_flags(mallocFlags))
			VMAreaWiredRange(areaStart, areaEnd - areaStart, writable, true);
		if (range == NULL) {
			error = B_NO_MEMORY;
			break;
		}

		// Lock the area's top cache. This is a requirement for VMArea::Wire().
		VMCacheChainLocker cacheChainLocker(vm_area_get_locked_cache(area));

		// mark the area range wired
		area->Wire(range);

		// Depending on the area cache type and the wiring, we may not need to
		// look at the individual pages.
		if (area->cache_type == CACHE_TYPE_NULL
			|| area->cache_type == CACHE_TYPE_DEVICE
			|| area->wiring == B_FULL_LOCK
			|| area->wiring == B_CONTIGUOUS) {
			nextAddress = areaEnd;
			continue;
		}

		// Lock the area's cache chain and the translation map. Needed to look
		// up pages and play with their wired count.
		cacheChainLocker.LockAllSourceCaches();
		map->Lock();

		// iterate through the pages and wire them
		for (; nextAddress != areaEnd; nextAddress += B_PAGE_SIZE) {
			phys_addr_t physicalAddress;
			uint32 flags;

			vm_page* page;
			if (map->Query(nextAddress, &physicalAddress, &flags) == B_OK
				&& (flags & requiredProtection) == requiredProtection
				&& (page = vm_lookup_page(physicalAddress / B_PAGE_SIZE))
					!= NULL) {
				// Already mapped with the correct permissions -- just increment
				// the page's wired count.
				increment_page_wired_count(page);
			} else {
				// Let vm_soft_fault() map the page for us, if possible. We need
				// to fully unlock to avoid deadlocks. Since we have already
				// wired the area itself, nothing disturbing will happen with it
				// in the meantime.
				map->Unlock();
				cacheChainLocker.Unlock();
				addressSpaceLocker.Unlock();

				error = vm_soft_fault(addressSpace, nextAddress, writable,
					false, isUser, &page);

				addressSpaceLocker.Lock();
				cacheChainLocker.SetTo(vm_area_get_locked_cache(area));
				cacheChainLocker.LockAllSourceCaches();
				map->Lock();
			}

			if (error != B_OK)
				break;
		}

		map->Unlock();

		if (error == B_OK) {
			cacheChainLocker.Unlock();
		} else {
			// An error occurred, so abort right here. If the current address
			// is the first in this area, unwire the area, since we won't get
			// to it when reverting what we've done so far.
			if (nextAddress == areaStart) {
				area->Unwire(range);
				cacheChainLocker.Unlock();
				range->~VMAreaWiredRange();
				free_etc(range, mallocFlags);
			} else
				cacheChainLocker.Unlock();

			break;
		}
	}

	if (error != B_OK) {
		// An error occurred, so unwire all that we've already wired. Note that
		// even if not a single page was wired, unlock_memory_etc() is called
		// to put the address space reference.
		addressSpaceLocker.Unlock();
		unlock_memory_etc(team, (void*)lockBaseAddress,
			nextAddress - lockBaseAddress, flags);
	}

	return error;
}


status_t
lock_memory(void* address, size_t numBytes, uint32 flags)
{
	return lock_memory_etc(B_CURRENT_TEAM, address, numBytes, flags);
}


/*!	Unwires an address range previously wired with lock_memory_etc().

	Note that a call to this function must balance a previous lock_memory_etc()
	call with exactly the same parameters.
*/
status_t
unlock_memory_etc(team_id team, void* address, size_t numBytes, uint32 flags)
{
	addr_t lockBaseAddress = ROUNDDOWN((addr_t)address, B_PAGE_SIZE);
	addr_t lockEndAddress = ROUNDUP((addr_t)address + numBytes, B_PAGE_SIZE);

	// compute the page protection that is required
	bool isUser = IS_USER_ADDRESS(address);
	bool writable = (flags & B_READ_DEVICE) == 0;
	uint32 requiredProtection = PAGE_PRESENT
		| B_KERNEL_READ_AREA | (isUser ? B_READ_AREA : 0);
	if (writable)
		requiredProtection |= B_KERNEL_WRITE_AREA | (isUser ? B_WRITE_AREA : 0);

	uint32 mallocFlags = isUser
		? 0 : HEAP_DONT_WAIT_FOR_MEMORY | HEAP_DONT_LOCK_KERNEL_SPACE;

	// get and read lock the address space
	VMAddressSpace* addressSpace = NULL;
	if (isUser) {
		if (team == B_CURRENT_TEAM)
			addressSpace = VMAddressSpace::GetCurrent();
		else
			addressSpace = VMAddressSpace::Get(team);
	} else
		addressSpace = VMAddressSpace::GetKernel();
	if (addressSpace == NULL)
		return B_ERROR;

	AddressSpaceReadLocker addressSpaceLocker(addressSpace, false);
		// Take over the address space reference. We don't unlock until we're
		// done.

	VMTranslationMap* map = addressSpace->TranslationMap();
	status_t error = B_OK;

	// iterate through all concerned areas
	addr_t nextAddress = lockBaseAddress;
	while (nextAddress != lockEndAddress) {
		// get the next area
		VMArea* area = addressSpace->LookupArea(nextAddress);
		if (area == NULL) {
			error = B_BAD_ADDRESS;
			break;
		}

		addr_t areaStart = nextAddress;
		addr_t areaEnd = std::min(lockEndAddress, area->Base() + area->Size());

		// Lock the area's top cache. This is a requirement for
		// VMArea::Unwire().
		VMCacheChainLocker cacheChainLocker(vm_area_get_locked_cache(area));

		// Depending on the area cache type and the wiring, we may not need to
		// look at the individual pages.
		if (area->cache_type == CACHE_TYPE_NULL
			|| area->cache_type == CACHE_TYPE_DEVICE
			|| area->wiring == B_FULL_LOCK
			|| area->wiring == B_CONTIGUOUS) {
			// unwire the range (to avoid deadlocks we delete the range after
			// unlocking the cache)
			nextAddress = areaEnd;
			VMAreaWiredRange* range = area->Unwire(areaStart,
				areaEnd - areaStart, writable);
			cacheChainLocker.Unlock();
			if (range != NULL) {
				range->~VMAreaWiredRange();
				free_etc(range, mallocFlags);
			}
			continue;
		}

		// Lock the area's cache chain and the translation map. Needed to look
		// up pages and play with their wired count.
		cacheChainLocker.LockAllSourceCaches();
		map->Lock();

		// iterate through the pages and unwire them
		for (; nextAddress != areaEnd; nextAddress += B_PAGE_SIZE) {
			phys_addr_t physicalAddress;
			uint32 flags;

			vm_page* page;
			if (map->Query(nextAddress, &physicalAddress, &flags) == B_OK
				&& (flags & PAGE_PRESENT) != 0
				&& (page = vm_lookup_page(physicalAddress / B_PAGE_SIZE))
					!= NULL) {
				// Already mapped with the correct permissions -- just increment
				// the page's wired count.
				decrement_page_wired_count(page);
			} else {
				panic("unlock_memory_etc(): Failed to unwire page: address "
					"space %p, address: %#" B_PRIxADDR, addressSpace,
					nextAddress);
				error = B_BAD_VALUE;
				break;
			}
		}

		map->Unlock();

		// All pages are unwired. Remove the area's wired range as well (to
		// avoid deadlocks we delete the range after unlocking the cache).
		VMAreaWiredRange* range = area->Unwire(areaStart,
			areaEnd - areaStart, writable);

		cacheChainLocker.Unlock();

		if (range != NULL) {
			range->~VMAreaWiredRange();
			free_etc(range, mallocFlags);
		}

		if (error != B_OK)
			break;
	}

	// get rid of the address space reference lock_memory_etc() acquired
	addressSpace->Put();

	return error;
}


status_t
unlock_memory(void* address, size_t numBytes, uint32 flags)
{
	return unlock_memory_etc(B_CURRENT_TEAM, address, numBytes, flags);
}


/*!	Similar to get_memory_map(), but also allows to specify the address space
	for the memory in question and has a saner semantics.
	Returns \c B_OK when the complete range could be translated or
	\c B_BUFFER_OVERFLOW, if the provided array wasn't big enough. In either
	case the actual number of entries is written to \c *_numEntries. Any other
	error case indicates complete failure; \c *_numEntries will be set to \c 0
	in this case.
*/
status_t
get_memory_map_etc(team_id team, const void* address, size_t numBytes,
	physical_entry* table, uint32* _numEntries)
{
	uint32 numEntries = *_numEntries;
	*_numEntries = 0;

	VMAddressSpace* addressSpace;
	addr_t virtualAddress = (addr_t)address;
	addr_t pageOffset = virtualAddress & (B_PAGE_SIZE - 1);
	phys_addr_t physicalAddress;
	status_t status = B_OK;
	int32 index = -1;
	addr_t offset = 0;
	bool interrupts = are_interrupts_enabled();

	TRACE(("get_memory_map_etc(%" B_PRId32 ", %p, %lu bytes, %" B_PRIu32 " "
		"entries)\n", team, address, numBytes, numEntries));

	if (numEntries == 0 || numBytes == 0)
		return B_BAD_VALUE;

	// in which address space is the address to be found?
	if (IS_USER_ADDRESS(virtualAddress)) {
		if (team == B_CURRENT_TEAM)
			addressSpace = VMAddressSpace::GetCurrent();
		else
			addressSpace = VMAddressSpace::Get(team);
	} else
		addressSpace = VMAddressSpace::GetKernel();

	if (addressSpace == NULL)
		return B_ERROR;

	VMTranslationMap* map = addressSpace->TranslationMap();

	if (interrupts)
		map->Lock();

	while (offset < numBytes) {
		addr_t bytes = min_c(numBytes - offset, B_PAGE_SIZE);
		uint32 flags;

		if (interrupts) {
			status = map->Query((addr_t)address + offset, &physicalAddress,
				&flags);
		} else {
			status = map->QueryInterrupt((addr_t)address + offset,
				&physicalAddress, &flags);
		}
		if (status < B_OK)
			break;
		if ((flags & PAGE_PRESENT) == 0) {
			panic("get_memory_map() called on unmapped memory!");
			return B_BAD_ADDRESS;
		}

		if (index < 0 && pageOffset > 0) {
			physicalAddress += pageOffset;
			if (bytes > B_PAGE_SIZE - pageOffset)
				bytes = B_PAGE_SIZE - pageOffset;
		}

		// need to switch to the next physical_entry?
		if (index < 0 || table[index].address
				!= physicalAddress - table[index].size) {
			if ((uint32)++index + 1 > numEntries) {
				// table to small
				break;
			}
			table[index].address = physicalAddress;
			table[index].size = bytes;
		} else {
			// page does fit in current entry
			table[index].size += bytes;
		}

		offset += bytes;
	}

	if (interrupts)
		map->Unlock();

	if (status != B_OK)
		return status;

	if ((uint32)index + 1 > numEntries) {
		*_numEntries = index;
		return B_BUFFER_OVERFLOW;
	}

	*_numEntries = index + 1;
	return B_OK;
}


/*!	According to the BeBook, this function should always succeed.
	This is no longer the case.
*/
extern "C" int32
__get_memory_map_haiku(const void* address, size_t numBytes,
	physical_entry* table, int32 numEntries)
{
	uint32 entriesRead = numEntries;
	status_t error = get_memory_map_etc(B_CURRENT_TEAM, address, numBytes,
		table, &entriesRead);
	if (error != B_OK)
		return error;

	// close the entry list

	// if it's only one entry, we will silently accept the missing ending
	if (numEntries == 1)
		return B_OK;

	if (entriesRead + 1 > (uint32)numEntries)
		return B_BUFFER_OVERFLOW;

	table[entriesRead].address = 0;
	table[entriesRead].size = 0;

	return B_OK;
}


area_id
area_for(void* address)
{
	return vm_area_for((addr_t)address, true);
}


area_id
find_area(const char* name)
{
	return VMAreas::Find(name);
}


status_t
_get_area_info(area_id id, area_info* info, size_t size)
{
	if (size != sizeof(area_info) || info == NULL)
		return B_BAD_VALUE;

	AddressSpaceReadLocker locker;
	VMArea* area;
	status_t status = locker.SetFromArea(id, area);
	if (status != B_OK)
		return status;

	fill_area_info(area, info, size);
	return B_OK;
}


status_t
_get_next_area_info(team_id team, ssize_t* cookie, area_info* info, size_t size)
{
	addr_t nextBase = *(addr_t*)cookie;

	// we're already through the list
	if (nextBase == (addr_t)-1)
		return B_ENTRY_NOT_FOUND;

	if (team == B_CURRENT_TEAM)
		team = team_get_current_team_id();

	AddressSpaceReadLocker locker(team);
	if (!locker.IsLocked())
		return B_BAD_TEAM_ID;

	VMArea* area = locker.AddressSpace()->FindClosestArea(nextBase, false);
	if (area == NULL) {
		nextBase = (addr_t)-1;
		return B_ENTRY_NOT_FOUND;
	}

	fill_area_info(area, info, size);
	*cookie = (ssize_t)(area->Base() + 1);

	return B_OK;
}


status_t
set_area_protection(area_id area, uint32 newProtection)
{
	return vm_set_area_protection(VMAddressSpace::KernelID(), area,
		newProtection, true);
}


status_t
resize_area(area_id areaID, size_t newSize)
{
	return vm_resize_area(areaID, newSize, true);
}


/*!	Transfers the specified area to a new team. The caller must be the owner
	of the area.
*/
area_id
transfer_area(area_id id, void** _address, uint32 addressSpec, team_id target,
	bool kernel)
{
	area_info info;
	status_t status = get_area_info(id, &info);
	if (status != B_OK)
		return status;

	// enforce restrictions
	if (!kernel && (info.team != team_get_current_team_id()
			|| (info.protection & B_KERNEL_AREA) != 0)) {
		return B_NOT_ALLOWED;
	}

	// We need to mark the area cloneable so the following operations work.
	status = set_area_protection(id, info.protection | B_CLONEABLE_AREA);
	if (status != B_OK)
		return status;

	area_id clonedArea = vm_clone_area(target, info.name, _address,
		addressSpec, info.protection, REGION_NO_PRIVATE_MAP, id, kernel);
	if (clonedArea < 0)
		return clonedArea;

	status = vm_delete_area(info.team, id, kernel);
	if (status != B_OK) {
		vm_delete_area(target, clonedArea, kernel);
		return status;
	}

	// Now we can reset the protection to whatever it was before.
	set_area_protection(clonedArea, info.protection);

	// TODO: The clonedArea is B_SHARED_AREA, which is not really desired.

	return clonedArea;
}


extern "C" area_id
__map_physical_memory_haiku(const char* name, phys_addr_t physicalAddress,
	size_t numBytes, uint32 addressSpec, uint32 protection,
	void** _virtualAddress)
{
	if (!arch_vm_supports_protection(protection))
		return B_NOT_SUPPORTED;

	fix_protection(&protection);

	return vm_map_physical_memory(VMAddressSpace::KernelID(), name,
		_virtualAddress, addressSpec, numBytes, protection, physicalAddress,
		false);
}


area_id
clone_area(const char* name, void** _address, uint32 addressSpec,
	uint32 protection, area_id source)
{
	if ((protection & B_KERNEL_PROTECTION) == 0)
		protection |= B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA;

	return vm_clone_area(VMAddressSpace::KernelID(), name, _address,
		addressSpec, protection, REGION_NO_PRIVATE_MAP, source, true);
}


area_id
create_area_etc(team_id team, const char* name, size_t size, uint32 lock,
	uint32 protection, uint32 flags, uint32 guardSize,
	const virtual_address_restrictions* virtualAddressRestrictions,
	const physical_address_restrictions* physicalAddressRestrictions,
	void** _address)
{
	fix_protection(&protection);

	return vm_create_anonymous_area(team, name, size, lock, protection, flags,
		guardSize, virtualAddressRestrictions, physicalAddressRestrictions,
		true, _address);
}


extern "C" area_id
__create_area_haiku(const char* name, void** _address, uint32 addressSpec,
	size_t size, uint32 lock, uint32 protection)
{
	fix_protection(&protection);

	virtual_address_restrictions virtualRestrictions = {};
	virtualRestrictions.address = *_address;
	virtualRestrictions.address_specification = addressSpec;
	physical_address_restrictions physicalRestrictions = {};
	return vm_create_anonymous_area(VMAddressSpace::KernelID(), name, size,
		lock, protection, 0, 0, &virtualRestrictions, &physicalRestrictions,
		true, _address);
}


status_t
delete_area(area_id area)
{
	return vm_delete_area(VMAddressSpace::KernelID(), area, true);
}


//	#pragma mark - Userland syscalls


status_t
_user_reserve_address_range(addr_t* userAddress, uint32 addressSpec,
	addr_t size)
{
	// filter out some unavailable values (for userland)
	switch (addressSpec) {
		case B_ANY_KERNEL_ADDRESS:
		case B_ANY_KERNEL_BLOCK_ADDRESS:
			return B_BAD_VALUE;
	}

	addr_t address;

	if (!IS_USER_ADDRESS(userAddress)
		|| user_memcpy(&address, userAddress, sizeof(address)) != B_OK)
		return B_BAD_ADDRESS;

	status_t status = vm_reserve_address_range(
		VMAddressSpace::CurrentID(), (void**)&address, addressSpec, size,
		RESERVED_AVOID_BASE);
	if (status != B_OK)
		return status;

	if (user_memcpy(userAddress, &address, sizeof(address)) != B_OK) {
		vm_unreserve_address_range(VMAddressSpace::CurrentID(),
			(void*)address, size);
		return B_BAD_ADDRESS;
	}

	return B_OK;
}


status_t
_user_unreserve_address_range(addr_t address, addr_t size)
{
	return vm_unreserve_address_range(VMAddressSpace::CurrentID(),
		(void*)address, size);
}


area_id
_user_area_for(void* address)
{
	return vm_area_for((addr_t)address, false);
}


area_id
_user_find_area(const char* userName)
{
	char name[B_OS_NAME_LENGTH];

	if (!IS_USER_ADDRESS(userName)
		|| user_strlcpy(name, userName, B_OS_NAME_LENGTH) < B_OK)
		return B_BAD_ADDRESS;

	return find_area(name);
}


status_t
_user_get_area_info(area_id area, area_info* userInfo)
{
	if (!IS_USER_ADDRESS(userInfo))
		return B_BAD_ADDRESS;

	area_info info;
	status_t status = get_area_info(area, &info);
	if (status < B_OK)
		return status;

	if (geteuid() != 0) {
		if (info.team != team_get_current_team_id()) {
			if (team_geteuid(info.team) != geteuid())
				return B_NOT_ALLOWED;
		}

		info.protection &= B_USER_AREA_FLAGS;
	}

	if (user_memcpy(userInfo, &info, sizeof(area_info)) < B_OK)
		return B_BAD_ADDRESS;

	return status;
}


status_t
_user_get_next_area_info(team_id team, ssize_t* userCookie, area_info* userInfo)
{
	ssize_t cookie;

	if (!IS_USER_ADDRESS(userCookie)
		|| !IS_USER_ADDRESS(userInfo)
		|| user_memcpy(&cookie, userCookie, sizeof(ssize_t)) < B_OK)
		return B_BAD_ADDRESS;

	area_info info;
	status_t status = _get_next_area_info(team, &cookie, &info,
		sizeof(area_info));
	if (status != B_OK)
		return status;

	if (geteuid() != 0) {
		if (info.team != team_get_current_team_id()) {
			if (team_geteuid(info.team) != geteuid())
				return B_NOT_ALLOWED;
		}

		info.protection &= B_USER_AREA_FLAGS;
	}

	if (user_memcpy(userCookie, &cookie, sizeof(ssize_t)) < B_OK
		|| user_memcpy(userInfo, &info, sizeof(area_info)) < B_OK)
		return B_BAD_ADDRESS;

	return status;
}


status_t
_user_set_area_protection(area_id area, uint32 newProtection)
{
	if ((newProtection & ~(B_USER_PROTECTION | B_CLONEABLE_AREA)) != 0)
		return B_BAD_VALUE;

	return vm_set_area_protection(VMAddressSpace::CurrentID(), area,
		newProtection, false);
}


status_t
_user_resize_area(area_id area, size_t newSize)
{
	return vm_resize_area(area, newSize, false);
}


area_id
_user_transfer_area(area_id area, void** userAddress, uint32 addressSpec,
	team_id target)
{
	// filter out some unavailable values (for userland)
	switch (addressSpec) {
		case B_ANY_KERNEL_ADDRESS:
		case B_ANY_KERNEL_BLOCK_ADDRESS:
			return B_BAD_VALUE;
	}

	void* address;
	if (!IS_USER_ADDRESS(userAddress)
		|| user_memcpy(&address, userAddress, sizeof(address)) < B_OK)
		return B_BAD_ADDRESS;

	area_id newArea = transfer_area(area, &address, addressSpec, target, false);
	if (newArea < B_OK)
		return newArea;

	if (user_memcpy(userAddress, &address, sizeof(address)) < B_OK)
		return B_BAD_ADDRESS;

	return newArea;
}


area_id
_user_clone_area(const char* userName, void** userAddress, uint32 addressSpec,
	uint32 protection, area_id sourceArea)
{
	char name[B_OS_NAME_LENGTH];
	void* address;

	// filter out some unavailable values (for userland)
	switch (addressSpec) {
		case B_ANY_KERNEL_ADDRESS:
		case B_ANY_KERNEL_BLOCK_ADDRESS:
			return B_BAD_VALUE;
	}
	if ((protection & ~B_USER_AREA_FLAGS) != 0)
		return B_BAD_VALUE;

	if (!IS_USER_ADDRESS(userName)
		|| !IS_USER_ADDRESS(userAddress)
		|| user_strlcpy(name, userName, sizeof(name)) < B_OK
		|| user_memcpy(&address, userAddress, sizeof(address)) < B_OK)
		return B_BAD_ADDRESS;

	fix_protection(&protection);

	area_id clonedArea = vm_clone_area(VMAddressSpace::CurrentID(), name,
		&address, addressSpec, protection, REGION_NO_PRIVATE_MAP, sourceArea,
		false);
	if (clonedArea < B_OK)
		return clonedArea;

	if (user_memcpy(userAddress, &address, sizeof(address)) < B_OK) {
		delete_area(clonedArea);
		return B_BAD_ADDRESS;
	}

	return clonedArea;
}


area_id
_user_create_area(const char* userName, void** userAddress, uint32 addressSpec,
	size_t size, uint32 lock, uint32 protection)
{
	char name[B_OS_NAME_LENGTH];
	void* address;

	// filter out some unavailable values (for userland)
	switch (addressSpec) {
		case B_ANY_KERNEL_ADDRESS:
		case B_ANY_KERNEL_BLOCK_ADDRESS:
			return B_BAD_VALUE;
	}
	if ((protection & ~B_USER_AREA_FLAGS) != 0)
		return B_BAD_VALUE;

	if (!IS_USER_ADDRESS(userName)
		|| !IS_USER_ADDRESS(userAddress)
		|| user_strlcpy(name, userName, sizeof(name)) < B_OK
		|| user_memcpy(&address, userAddress, sizeof(address)) < B_OK)
		return B_BAD_ADDRESS;

	if (addressSpec == B_EXACT_ADDRESS && IS_KERNEL_ADDRESS(address))
		return B_BAD_VALUE;

	fix_protection(&protection);

	virtual_address_restrictions virtualRestrictions = {};
	virtualRestrictions.address = address;
	virtualRestrictions.address_specification = addressSpec;
	physical_address_restrictions physicalRestrictions = {};
	area_id area = vm_create_anonymous_area(VMAddressSpace::CurrentID(), name,
		size, lock, protection, 0, 0, &virtualRestrictions,
		&physicalRestrictions, false, &address);

	if (area >= B_OK
		&& user_memcpy(userAddress, &address, sizeof(address)) < B_OK) {
		delete_area(area);
		return B_BAD_ADDRESS;
	}

	return area;
}


status_t
_user_delete_area(area_id area)
{
	// Unlike the BeOS implementation, you can now only delete areas
	// that you have created yourself from userland.
	// The documentation to delete_area() explicitly states that this
	// will be restricted in the future, and so it will.
	return vm_delete_area(VMAddressSpace::CurrentID(), area, false);
}


// TODO: create a BeOS style call for this!

area_id
_user_map_file(const char* userName, void** userAddress, uint32 addressSpec,
	size_t size, uint32 protection, uint32 mapping, bool unmapAddressRange,
	int fd, off_t offset)
{
	char name[B_OS_NAME_LENGTH];
	void* address;
	area_id area;

	if ((protection & ~B_USER_AREA_FLAGS) != 0)
		return B_BAD_VALUE;

	fix_protection(&protection);

	if (!IS_USER_ADDRESS(userName) || !IS_USER_ADDRESS(userAddress)
		|| user_strlcpy(name, userName, B_OS_NAME_LENGTH) < B_OK
		|| user_memcpy(&address, userAddress, sizeof(address)) < B_OK)
		return B_BAD_ADDRESS;

	if (addressSpec == B_EXACT_ADDRESS) {
		if ((addr_t)address + size < (addr_t)address
				|| (addr_t)address % B_PAGE_SIZE != 0) {
			return B_BAD_VALUE;
		}
		if (!IS_USER_ADDRESS(address)
				|| !IS_USER_ADDRESS((addr_t)address + size - 1)) {
			return B_BAD_ADDRESS;
		}
	}

	area = _vm_map_file(VMAddressSpace::CurrentID(), name, &address,
		addressSpec, size, protection, mapping, unmapAddressRange, fd, offset,
		false);
	if (area < B_OK)
		return area;

	if (user_memcpy(userAddress, &address, sizeof(address)) < B_OK)
		return B_BAD_ADDRESS;

	return area;
}


status_t
_user_unmap_memory(void* _address, size_t size)
{
	addr_t address = (addr_t)_address;

	// check params
	if (size == 0 || (addr_t)address + size < (addr_t)address
		|| (addr_t)address % B_PAGE_SIZE != 0) {
		return B_BAD_VALUE;
	}

	if (!IS_USER_ADDRESS(address)
		|| !IS_USER_ADDRESS((addr_t)address + size - 1)) {
		return B_BAD_ADDRESS;
	}

	// Write lock the address space and ensure the address range is not wired.
	AddressSpaceWriteLocker locker;
	do {
		status_t status = locker.SetTo(team_get_current_team_id());
		if (status != B_OK)
			return status;
	} while (wait_if_address_range_is_wired(locker.AddressSpace(), address,
			size, &locker));

	// unmap
	return unmap_address_range(locker.AddressSpace(), address, size, false);
}


status_t
_user_set_memory_protection(void* _address, size_t size, uint32 protection)
{
	// check address range
	addr_t address = (addr_t)_address;
	size = PAGE_ALIGN(size);

	if ((address % B_PAGE_SIZE) != 0)
		return B_BAD_VALUE;
	if (!is_user_address_range(_address, size)) {
		// weird error code required by POSIX
		return ENOMEM;
	}

	// extend and check protection
	if ((protection & ~B_USER_PROTECTION) != 0)
		return B_BAD_VALUE;

	fix_protection(&protection);

	// We need to write lock the address space, since we're going to play with
	// the areas. Also make sure that none of the areas is wired and that we're
	// actually allowed to change the protection.
	AddressSpaceWriteLocker locker;

	bool restart;
	do {
		restart = false;

		status_t status = locker.SetTo(team_get_current_team_id());
		if (status != B_OK)
			return status;

		// First round: Check whether the whole range is covered by areas and we
		// are allowed to modify them.
		addr_t currentAddress = address;
		size_t sizeLeft = size;
		while (sizeLeft > 0) {
			VMArea* area = locker.AddressSpace()->LookupArea(currentAddress);
			if (area == NULL)
				return B_NO_MEMORY;

			if ((area->protection & B_KERNEL_AREA) != 0)
				return B_NOT_ALLOWED;
			if (area->protection_max != 0
				&& (protection & area->protection_max) != (protection & B_USER_PROTECTION)) {
				return B_NOT_ALLOWED;
			}

			addr_t offset = currentAddress - area->Base();
			size_t rangeSize = min_c(area->Size() - offset, sizeLeft);

			AreaCacheLocker cacheLocker(area);

			if (wait_if_area_range_is_wired(area, currentAddress, rangeSize,
					&locker, &cacheLocker)) {
				restart = true;
				break;
			}

			cacheLocker.Unlock();

			currentAddress += rangeSize;
			sizeLeft -= rangeSize;
		}
	} while (restart);

	// Second round: If the protections differ from that of the area, create a
	// page protection array and re-map mapped pages.
	VMTranslationMap* map = locker.AddressSpace()->TranslationMap();
	addr_t currentAddress = address;
	size_t sizeLeft = size;
	while (sizeLeft > 0) {
		VMArea* area = locker.AddressSpace()->LookupArea(currentAddress);
		if (area == NULL)
			return B_NO_MEMORY;

		addr_t offset = currentAddress - area->Base();
		size_t rangeSize = min_c(area->Size() - offset, sizeLeft);

		currentAddress += rangeSize;
		sizeLeft -= rangeSize;

		if (area->page_protections == NULL) {
			if (area->protection == protection)
				continue;
			if (offset == 0 && rangeSize == area->Size()) {
				// The whole area is covered: let set_area_protection handle it.
				status_t status = vm_set_area_protection(area->address_space->ID(),
					area->id, protection, false);
				if (status != B_OK)
					return status;
				continue;
			}

			status_t status = allocate_area_page_protections(area);
			if (status != B_OK)
				return status;
		}

		// We need to lock the complete cache chain, since we potentially unmap
		// pages of lower caches.
		VMCache* topCache = vm_area_get_locked_cache(area);
		VMCacheChainLocker cacheChainLocker(topCache);
		cacheChainLocker.LockAllSourceCaches();

		// Adjust the committed size, if necessary.
		if (topCache->temporary && !topCache->CanOvercommit()) {
			const bool becomesWritable = (protection & B_WRITE_AREA) != 0;
			ssize_t commitmentChange = 0;
			const off_t areaCacheBase = area->Base() - area->cache_offset;
			for (addr_t pageAddress = area->Base() + offset;
					pageAddress < currentAddress; pageAddress += B_PAGE_SIZE) {
				if (topCache->LookupPage(pageAddress - areaCacheBase) != NULL) {
					// This page should already be accounted for in the commitment.
					continue;
				}

				const bool isWritable
					= (get_area_page_protection(area, pageAddress) & B_WRITE_AREA) != 0;

				if (becomesWritable && !isWritable)
					commitmentChange += B_PAGE_SIZE;
				else if (!becomesWritable && isWritable)
					commitmentChange -= B_PAGE_SIZE;
			}

			if (commitmentChange != 0) {
				off_t newCommitment = topCache->committed_size + commitmentChange;
				if (newCommitment > PAGE_ALIGN(topCache->virtual_end - topCache->virtual_base)) {
					// This should only happen in the case where this process fork()ed,
					// duplicating the commitment, and then the child exited, resulting
					// in the commitments being merged along with the caches.
					KDEBUG_ONLY(dprintf("set_memory_protection(area %d): new commitment "
						"greater than cache size, recomputing\n", area->id));
					newCommitment = (compute_area_page_commitment(area) * B_PAGE_SIZE)
						+ commitmentChange;
				}
				status_t status = topCache->Commit(newCommitment, VM_PRIORITY_USER);
				if (status != B_OK)
					return status;
			}
		}

		for (addr_t pageAddress = area->Base() + offset;
				pageAddress < currentAddress; pageAddress += B_PAGE_SIZE) {
			map->Lock();

			set_area_page_protection(area, pageAddress, protection);

			phys_addr_t physicalAddress;
			uint32 flags;

			status_t error = map->Query(pageAddress, &physicalAddress, &flags);
			if (error != B_OK || (flags & PAGE_PRESENT) == 0) {
				map->Unlock();
				continue;
			}

			vm_page* page = vm_lookup_page(physicalAddress / B_PAGE_SIZE);
			if (page == NULL) {
				panic("area %p looking up page failed for pa %#" B_PRIxPHYSADDR
					"\n", area, physicalAddress);
				map->Unlock();
				return B_ERROR;
			}

			// If the page is not in the topmost cache and write access is
			// requested, we have to unmap it. Otherwise we can re-map it with
			// the new protection.
			bool unmapPage = page->Cache() != topCache
				&& (protection & B_WRITE_AREA) != 0;

			if (!unmapPage)
				map->ProtectPage(area, pageAddress, protection);

			map->Unlock();

			if (unmapPage) {
				DEBUG_PAGE_ACCESS_START(page);
				unmap_page(area, pageAddress);
				DEBUG_PAGE_ACCESS_END(page);
			}
		}
	}

	return B_OK;
}


status_t
_user_sync_memory(void* _address, size_t size, uint32 flags)
{
	addr_t address = (addr_t)_address;
	size = PAGE_ALIGN(size);

	// check params
	if ((address % B_PAGE_SIZE) != 0)
		return B_BAD_VALUE;
	if (!is_user_address_range(_address, size)) {
		// weird error code required by POSIX
		return ENOMEM;
	}

	bool writeSync = (flags & MS_SYNC) != 0;
	bool writeAsync = (flags & MS_ASYNC) != 0;
	if (writeSync && writeAsync)
		return B_BAD_VALUE;

	if (size == 0 || (!writeSync && !writeAsync))
		return B_OK;

	// iterate through the range and sync all concerned areas
	while (size > 0) {
		// read lock the address space
		AddressSpaceReadLocker locker;
		status_t error = locker.SetTo(team_get_current_team_id());
		if (error != B_OK)
			return error;

		// get the first area
		VMArea* area = locker.AddressSpace()->LookupArea(address);
		if (area == NULL)
			return B_NO_MEMORY;

		uint32 offset = address - area->Base();
		size_t rangeSize = min_c(area->Size() - offset, size);
		offset += area->cache_offset;

		// lock the cache
		AreaCacheLocker cacheLocker(area);
		if (!cacheLocker)
			return B_BAD_VALUE;
		VMCache* cache = area->cache;

		locker.Unlock();

		uint32 firstPage = offset >> PAGE_SHIFT;
		uint32 endPage = firstPage + (rangeSize >> PAGE_SHIFT);

		// write the pages
		if (cache->type == CACHE_TYPE_VNODE) {
			if (writeSync) {
				// synchronous
				error = vm_page_write_modified_page_range(cache, firstPage,
					endPage);
				if (error != B_OK)
					return error;
			} else {
				// asynchronous
				vm_page_schedule_write_page_range(cache, firstPage, endPage);
				// TODO: This is probably not quite what is supposed to happen.
				// Especially when a lot has to be written, it might take ages
				// until it really hits the disk.
			}
		}

		address += rangeSize;
		size -= rangeSize;
	}

	// NOTE: If I understand it correctly the purpose of MS_INVALIDATE is to
	// synchronize multiple mappings of the same file. In our VM they never get
	// out of sync, though, so we don't have to do anything.

	return B_OK;
}


status_t
_user_memory_advice(void* _address, size_t size, uint32 advice)
{
	addr_t address = (addr_t)_address;
	if ((address % B_PAGE_SIZE) != 0)
		return B_BAD_VALUE;

	size = PAGE_ALIGN(size);
	if (!is_user_address_range(_address, size)) {
		// weird error code required by POSIX
		return B_NO_MEMORY;
	}

	switch (advice) {
		case MADV_NORMAL:
		case MADV_SEQUENTIAL:
		case MADV_RANDOM:
		case MADV_WILLNEED:
		case MADV_DONTNEED:
			// TODO: Implement!
			break;

		case MADV_FREE:
		{
			AddressSpaceWriteLocker locker;
			do {
				status_t status = locker.SetTo(team_get_current_team_id());
				if (status != B_OK)
					return status;
			} while (wait_if_address_range_is_wired(locker.AddressSpace(),
					address, size, &locker));

			discard_address_range(locker.AddressSpace(), address, size, false);
			break;
		}

		default:
			return B_BAD_VALUE;
	}

	return B_OK;
}


status_t
_user_get_memory_properties(team_id teamID, const void* address,
	uint32* _protected, uint32* _lock)
{
	if (!IS_USER_ADDRESS(_protected) || !IS_USER_ADDRESS(_lock))
		return B_BAD_ADDRESS;

	if (teamID != B_CURRENT_TEAM && teamID != team_get_current_team_id()
			&& geteuid() != 0)
		return B_NOT_ALLOWED;

	AddressSpaceReadLocker locker;
	status_t error = locker.SetTo(teamID);
	if (error != B_OK)
		return error;

	VMArea* area = locker.AddressSpace()->LookupArea((addr_t)address);
	if (area == NULL)
		return B_NO_MEMORY;

	uint32 protection = get_area_page_protection(area, (addr_t)address);
	uint32 wiring = area->wiring;

	locker.Unlock();

	error = user_memcpy(_protected, &protection, sizeof(protection));
	if (error != B_OK)
		return error;

	error = user_memcpy(_lock, &wiring, sizeof(wiring));

	return error;
}


static status_t
user_set_memory_swappable(const void* _address, size_t size, bool swappable)
{
#if ENABLE_SWAP_SUPPORT
	// check address range
	addr_t address = (addr_t)_address;
	size = PAGE_ALIGN(size);

	if ((address % B_PAGE_SIZE) != 0)
		return EINVAL;
	if (!is_user_address_range(_address, size))
		return EINVAL;

	const addr_t endAddress = address + size;

	AddressSpaceReadLocker addressSpaceLocker;
	status_t error = addressSpaceLocker.SetTo(team_get_current_team_id());
	if (error != B_OK)
		return error;
	VMAddressSpace* addressSpace = addressSpaceLocker.AddressSpace();

	// iterate through all concerned areas
	addr_t nextAddress = address;
	while (nextAddress != endAddress) {
		// get the next area
		VMArea* area = addressSpace->LookupArea(nextAddress);
		if (area == NULL) {
			error = B_BAD_ADDRESS;
			break;
		}

		const addr_t areaStart = nextAddress;
		const addr_t areaEnd = std::min(endAddress, area->Base() + area->Size());
		nextAddress = areaEnd;

		error = lock_memory_etc(addressSpace->ID(), (void*)areaStart, areaEnd - areaStart, 0);
		if (error != B_OK) {
			// We don't need to unset or reset things on failure.
			break;
		}

		VMCacheChainLocker cacheChainLocker(vm_area_get_locked_cache(area));
		VMAnonymousCache* anonCache = NULL;
		if (dynamic_cast<VMAnonymousNoSwapCache*>(area->cache) != NULL) {
			// This memory will aready never be swapped. Nothing to do.
		} else if ((anonCache = dynamic_cast<VMAnonymousCache*>(area->cache)) != NULL) {
			error = anonCache->SetCanSwapPages(areaStart - area->Base(),
				areaEnd - areaStart, swappable);
		} else {
			// Some other cache type? We cannot affect anything here.
			error = EINVAL;
		}

		cacheChainLocker.Unlock();

		unlock_memory_etc(addressSpace->ID(), (void*)areaStart, areaEnd - areaStart, 0);
		if (error != B_OK)
			break;
	}

	return error;
#else
	// No swap support? Nothing to do.
	return B_OK;
#endif
}


status_t
_user_mlock(const void* _address, size_t size)
{
	return user_set_memory_swappable(_address, size, false);
}


status_t
_user_munlock(const void* _address, size_t size)
{
	// TODO: B_SHARED_AREAs need to be handled a bit differently:
	// if multiple clones of an area had mlock() called on them,
	// munlock() must also be called on all of them to actually unlock.
	// (At present, the first munlock() will unlock all.)
	// TODO: fork() should automatically unlock memory in the child.
	return user_set_memory_swappable(_address, size, true);
}


// #pragma mark -- compatibility


#if defined(__i386__) && B_HAIKU_PHYSICAL_BITS > 32


struct physical_entry_beos {
	uint32	address;
	uint32	size;
};


/*!	The physical_entry structure has changed. We need to translate it to the
	old one.
*/
extern "C" int32
__get_memory_map_beos(const void* _address, size_t numBytes,
	physical_entry_beos* table, int32 numEntries)
{
	if (numEntries <= 0)
		return B_BAD_VALUE;

	const uint8* address = (const uint8*)_address;

	int32 count = 0;
	while (numBytes > 0 && count < numEntries) {
		physical_entry entry;
		status_t result = __get_memory_map_haiku(address, numBytes, &entry, 1);
		if (result < 0) {
			if (result != B_BUFFER_OVERFLOW)
				return result;
		}

		if (entry.address >= (phys_addr_t)1 << 32) {
			panic("get_memory_map(): Address is greater 4 GB!");
			return B_ERROR;
		}

		table[count].address = entry.address;
		table[count++].size = entry.size;

		address += entry.size;
		numBytes -= entry.size;
	}

	// null-terminate the table, if possible
	if (count < numEntries) {
		table[count].address = 0;
		table[count].size = 0;
	}

	return B_OK;
}


/*!	The type of the \a physicalAddress parameter has changed from void* to
	phys_addr_t.
*/
extern "C" area_id
__map_physical_memory_beos(const char* name, void* physicalAddress,
	size_t numBytes, uint32 addressSpec, uint32 protection,
	void** _virtualAddress)
{
	return __map_physical_memory_haiku(name, (addr_t)physicalAddress, numBytes,
		addressSpec, protection, _virtualAddress);
}


/*! The caller might not be able to deal with physical addresses >= 4 GB, so
	we meddle with the \a lock parameter to force 32 bit.
*/
extern "C" area_id
__create_area_beos(const char* name, void** _address, uint32 addressSpec,
	size_t size, uint32 lock, uint32 protection)
{
	switch (lock) {
		case B_NO_LOCK:
			break;
		case B_FULL_LOCK:
		case B_LAZY_LOCK:
			lock = B_32_BIT_FULL_LOCK;
			break;
		case B_CONTIGUOUS:
			lock = B_32_BIT_CONTIGUOUS;
			break;
	}

	return __create_area_haiku(name, _address, addressSpec, size, lock,
		protection);
}


DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("__get_memory_map_beos", "get_memory_map@",
	"BASE");
DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("__map_physical_memory_beos",
	"map_physical_memory@", "BASE");
DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("__create_area_beos", "create_area@",
	"BASE");

DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("__get_memory_map_haiku",
	"get_memory_map@@", "1_ALPHA3");
DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("__map_physical_memory_haiku",
	"map_physical_memory@@", "1_ALPHA3");
DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("__create_area_haiku", "create_area@@",
	"1_ALPHA3");


#else


DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("__get_memory_map_haiku",
	"get_memory_map@@", "BASE");
DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("__map_physical_memory_haiku",
	"map_physical_memory@@", "BASE");
DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("__create_area_haiku", "create_area@@",
	"BASE");


#endif	// defined(__i386__) && B_HAIKU_PHYSICAL_BITS > 32
