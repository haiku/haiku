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

#include <AutoDeleter.h>

#include <symbol_versioning.h>

#include <arch/cpu.h>
#include <arch/vm.h>
#include <boot/elf.h>
#include <boot/stage2.h>
#include <condition_variable.h>
#include <console.h>
#include <debug.h>
#include <file_cache.h>
#include <fs/fd.h>
#include <heap.h>
#include <kernel.h>
#include <int.h>
#include <lock.h>
#include <low_resource_manager.h>
#include <slab/Slab.h>
#include <smp.h>
#include <system_info.h>
#include <thread.h>
#include <team.h>
#include <tracing.h>
#include <util/AutoLock.h>
#include <util/khash.h>
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


// The memory reserve an allocation of the certain priority must not touch.
static const size_t kMemoryReserveForPriority[] = {
	VM_MEMORY_RESERVE_USER,		// user
	VM_MEMORY_RESERVE_SYSTEM,	// system
	0							// VIP
};


ObjectCache* gPageMappingsObjectCache;

static rw_lock sAreaCacheLock = RW_LOCK_INITIALIZER("area->cache");

static off_t sAvailableMemory;
static off_t sNeededMemory;
static mutex sAvailableMemoryLock = MUTEX_INITIALIZER("available memory lock");
static uint32 sPageFaults;

static VMPhysicalPageMapper* sPhysicalPageMapper;

#if DEBUG_CACHE_LIST

struct cache_info {
	VMCache*	cache;
	addr_t		page_count;
	addr_t		committed;
};

static const int kCacheInfoTableCount = 100 * 1024;
static cache_info* sCacheInfoTable;

#endif	// DEBUG_CACHE_LIST


// function declarations
static void delete_area(VMAddressSpace* addressSpace, VMArea* area,
	bool addressSpaceCleanup);
static status_t vm_soft_fault(VMAddressSpace* addressSpace, addr_t address,
	bool isWrite, bool isUser, vm_page** wirePage,
	VMAreaWiredRange* wiredRange = NULL);
static status_t map_backing_store(VMAddressSpace* addressSpace,
	VMCache* cache, off_t offset, const char* areaName, addr_t size, int wiring,
	int protection, int mapping, uint32 flags,
	const virtual_address_restrictions* addressRestrictions, bool kernel,
	VMArea** _area, void** _virtualAddress);


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


//! You need to have the address space locked when calling this function
static VMArea*
lookup_area(VMAddressSpace* addressSpace, area_id id)
{
	VMAreaHash::ReadLock();

	VMArea* area = VMAreaHash::LookupLocked(id);
	if (area != NULL && area->address_space != addressSpace)
		area = NULL;

	VMAreaHash::ReadUnlock();

	return area;
}


static status_t
allocate_area_page_protections(VMArea* area)
{
	// In the page protections we store only the three user protections,
	// so we use 4 bits per page.
	uint32 bytes = (area->Size() / B_PAGE_SIZE + 1) / 2;
	area->page_protections = (uint8*)malloc_etc(bytes,
		HEAP_DONT_LOCK_KERNEL_SPACE);
	if (area->page_protections == NULL)
		return B_NO_MEMORY;

	// init the page protections for all pages to that of the area
	uint32 areaProtection = area->protection
		& (B_READ_AREA | B_WRITE_AREA | B_EXECUTE_AREA);
	memset(area->page_protections, areaProtection | (areaProtection << 4),
		bytes);
	return B_OK;
}


static inline void
set_area_page_protection(VMArea* area, addr_t pageAddress, uint32 protection)
{
	protection &= B_READ_AREA | B_WRITE_AREA | B_EXECUTE_AREA;
	uint32 pageIndex = (pageAddress - area->Base()) / B_PAGE_SIZE;
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

	// If this is a kernel area we translate the user flags to kernel flags.
	if (area->address_space == VMAddressSpace::Kernel()) {
		uint32 kernelProtection = 0;
		if ((protection & B_READ_AREA) != 0)
			kernelProtection |= B_KERNEL_READ_AREA;
		if ((protection & B_WRITE_AREA) != 0)
			kernelProtection |= B_KERNEL_WRITE_AREA;

		return kernelProtection;
	}

	return protection | B_KERNEL_READ_AREA
		| (protection & B_WRITE_AREA ? B_KERNEL_WRITE_AREA : 0);
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
		vm_page_mapping* mapping = (vm_page_mapping*)object_cache_alloc(
			gPageMappingsObjectCache,
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
	addr_t lastAddress, VMArea** _secondArea, bool kernel)
{
	// Does the cut range intersect with the area at all?
	addr_t areaLast = area->Base() + (area->Size() - 1);
	if (area->Base() > lastAddress || areaLast < address)
		return B_OK;

	// Is the area fully covered?
	if (area->Base() >= address && areaLast <= lastAddress) {
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

	VMCache* cache = vm_area_get_locked_cache(area);
	VMCacheChainLocker cacheChainLocker(cache);
	cacheChainLocker.LockAllSourceCaches();

	// Cut the end only?
	if (areaLast <= lastAddress) {
		size_t oldSize = area->Size();
		size_t newSize = address - area->Base();

		status_t error = addressSpace->ShrinkAreaTail(area, newSize,
			allocationFlags);
		if (error != B_OK)
			return error;

		// unmap pages
		unmap_pages(area, address, oldSize - newSize);

		// If no one else uses the area's cache, we can resize it, too.
		if (cache->areas == area && area->cache_next == NULL
			&& cache->consumers.IsEmpty()
			&& cache->type == CACHE_TYPE_RAM) {
			// Since VMCache::Resize() can temporarily drop the lock, we must
			// unlock all lower caches to prevent locking order inversion.
			cacheChainLocker.Unlock(cache);
			cache->Resize(cache->virtual_base + newSize, priority);
			cache->ReleaseRefAndUnlock();
		}

		return B_OK;
	}

	// Cut the beginning only?
	if (area->Base() >= address) {
		addr_t oldBase = area->Base();
		addr_t newBase = lastAddress + 1;
		size_t newSize = areaLast - lastAddress;

		// unmap pages
		unmap_pages(area, oldBase, newBase - oldBase);

		// resize the area
		status_t error = addressSpace->ShrinkAreaHead(area, newSize,
			allocationFlags);
		if (error != B_OK)
			return error;

		// TODO: If no one else uses the area's cache, we should resize it, too!

		area->cache_offset += newBase - oldBase;

		return B_OK;
	}

	// The tough part -- cut a piece out of the middle of the area.
	// We do that by shrinking the area to the begin section and creating a
	// new area for the end section.

	addr_t firstNewSize = address - area->Base();
	addr_t secondBase = lastAddress + 1;
	addr_t secondSize = areaLast - lastAddress;

	// unmap pages
	unmap_pages(area, address, area->Size() - firstNewSize);

	// resize the area
	addr_t oldSize = area->Size();
	status_t error = addressSpace->ShrinkAreaTail(area, firstNewSize,
		allocationFlags);
	if (error != B_OK)
		return error;

	// TODO: If no one else uses the area's cache, we might want to create a
	// new cache for the second area, transfer the concerned pages from the
	// first cache to it and resize the first cache.

	// map the second area
	virtual_address_restrictions addressRestrictions = {};
	addressRestrictions.address = (void*)secondBase;
	addressRestrictions.address_specification = B_EXACT_ADDRESS;
	VMArea* secondArea;
	error = map_backing_store(addressSpace, cache,
		area->cache_offset + (secondBase - area->Base()), area->name,
		secondSize, area->wiring, area->protection, REGION_NO_PRIVATE_MAP, 0,
		&addressRestrictions, kernel, &secondArea, NULL);
	if (error != B_OK) {
		addressSpace->ShrinkAreaTail(area, oldSize, allocationFlags);
		return error;
	}

	// We need a cache reference for the new area.
	cache->AcquireRefLocked();

	if (_secondArea != NULL)
		*_secondArea = secondArea;

	return B_OK;
}


/*!	Deletes all areas in the given address range.
	The address space must be write-locked.
	The caller must ensure that no part of the given range is wired.
*/
static status_t
unmap_address_range(VMAddressSpace* addressSpace, addr_t address, addr_t size,
	bool kernel)
{
	size = PAGE_ALIGN(size);
	addr_t lastAddress = address + (size - 1);

	// Check, whether the caller is allowed to modify the concerned areas.
	if (!kernel) {
		for (VMAddressSpace::AreaIterator it = addressSpace->GetAreaIterator();
				VMArea* area = it.Next();) {
			addr_t areaLast = area->Base() + (area->Size() - 1);
			if (area->Base() < lastAddress && address < areaLast) {
				if ((area->protection & B_KERNEL_AREA) != 0)
					return B_NOT_ALLOWED;
			}
		}
	}

	for (VMAddressSpace::AreaIterator it = addressSpace->GetAreaIterator();
			VMArea* area = it.Next();) {
		addr_t areaLast = area->Base() + (area->Size() - 1);
		if (area->Base() < lastAddress && address < areaLast) {
			status_t error = cut_area(addressSpace, area, address,
				lastAddress, NULL, kernel);
			if (error != B_OK)
				return error;
				// Failing after already messing with areas is ugly, but we
				// can't do anything about it.
		}
	}

	return B_OK;
}


/*! You need to hold the lock of the cache and the write lock of the address
	space when calling this function.
	Note, that in case of error your cache will be temporarily unlocked.
	If \a addressSpec is \c B_EXACT_ADDRESS and the
	\c CREATE_AREA_UNMAP_ADDRESS_RANGE flag is specified, the caller must ensure
	that no part of the specified address range (base \c *_virtualAddress, size
	\a size) is wired.
*/
static status_t
map_backing_store(VMAddressSpace* addressSpace, VMCache* cache, off_t offset,
	const char* areaName, addr_t size, int wiring, int protection, int mapping,
	uint32 flags, const virtual_address_restrictions* addressRestrictions,
	bool kernel, VMArea** _area, void** _virtualAddress)
{
	TRACE(("map_backing_store: aspace %p, cache %p, virtual %p, offset 0x%Lx, "
		"size %lu, addressSpec %ld, wiring %d, protection %d, area %p, areaName "
		"'%s'\n", addressSpace, cache, addressRestrictions->address, offset,
		size, addressRestrictions->address_specification, wiring, protection,
		_area, areaName));
	cache->AssertLocked();

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

	status_t status;

	// if this is a private map, we need to create a new cache
	// to handle the private copies of pages as they are written to
	VMCache* sourceCache = cache;
	if (mapping == REGION_PRIVATE_MAP) {
		VMCache* newCache;

		// create an anonymous cache
		bool isStack = (protection & B_STACK_AREA) != 0;
		status = VMCacheFactory::CreateAnonymousCache(newCache,
			isStack || (protection & B_OVERCOMMITTING_AREA) != 0, 0,
			isStack ? USER_STACK_GUARD_PAGES : 0, true, VM_PRIORITY_USER);
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
		status = unmap_address_range(addressSpace,
			(addr_t)addressRestrictions->address, size, kernel);
		if (status != B_OK)
			goto err2;
	}

	status = addressSpace->InsertArea(area, size, addressRestrictions,
		allocationFlags, _virtualAddress);
	if (status != B_OK) {
		// TODO: wait and try again once this is working in the backend
#if 0
		if (status == B_NO_MEMORY && addressSpec == B_ANY_KERNEL_ADDRESS) {
			low_resource(B_KERNEL_RESOURCE_ADDRESS_SPACE, size,
				0, 0);
		}
#endif
		goto err2;
	}

	// attach the cache to the area
	area->cache = cache;
	area->cache_offset = offset;

	// point the cache back to the area
	cache->InsertAreaLocked(area);
	if (mapping == REGION_PRIVATE_MAP)
		cache->Unlock();

	// insert the area in the global area hash table
	VMAreaHash::Insert(area);

	// grab a ref to the address space (the area holds this)
	addressSpace->Get();

//	ktrace_printf("map_backing_store: cache: %p (source: %p), \"%s\" -> %p",
//		cache, sourceCache, areaName, area);

	*_area = area;
	return B_OK;

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
	addr_t end = base + size - 1;
	for (VMAddressSpace::AreaIterator it = addressSpace->GetAreaIterator();
			VMArea* area = it.Next();) {
		// TODO: Introduce a VMAddressSpace method to get a close iterator!
		if (area->Base() > end)
			return false;

		if (base >= area->Base() + area->Size() - 1)
			continue;

		AreaCacheLocker cacheLocker(vm_area_get_locked_cache(area));

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
	if (!arch_vm_supports_protection(0))
		return B_NOT_SUPPORTED;

	AddressSpaceWriteLocker locker;
	status_t status = locker.SetTo(VMAddressSpace::KernelID());
	if (status != B_OK)
		return status;

	VMAddressSpace* addressSpace = locker.AddressSpace();

	// create an anonymous cache
	VMCache* cache;
	status = VMCacheFactory::CreateAnonymousCache(cache, false, 0, 0, false,
		VM_PRIORITY_SYSTEM);
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
		B_ALREADY_WIRED, B_ALREADY_WIRED, REGION_NO_PRIVATE_MAP, 0,
		&addressRestrictions, true, &area, NULL);
	if (status != B_OK) {
		cache->ReleaseRefAndUnlock();
		return status;
	}

	cache->Unlock();
	area->cache_type = CACHE_TYPE_RAM;
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
	uint32 wiring, uint32 protection, uint32 flags,
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

	TRACE(("create_anonymous_area [%ld] %s: size 0x%lx\n", team, name, size));

	size = PAGE_ALIGN(size);

	if (size == 0)
		return B_BAD_VALUE;
	if (!arch_vm_supports_protection(protection))
		return B_NOT_SUPPORTED;

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
	switch (wiring) {
		case B_NO_LOCK:
			break;
		case B_FULL_LOCK:
		case B_LAZY_LOCK:
		case B_CONTIGUOUS:
			doReserveMemory = true;
			break;
		case B_ALREADY_WIRED:
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
	addr_t reservedMemory = 0;
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
	guardPages = isStack ? ((protection & B_USER_PROTECTION) != 0
		? USER_STACK_GUARD_PAGES : KERNEL_STACK_GUARD_PAGES) : 0;
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
		protection, REGION_NO_PRIVATE_MAP, flags, virtualAddressRestrictions,
		kernel, &area, _address);

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

			vm_page_set_state(page, PAGE_STATE_FREE);
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

	TRACE(("vm_map_physical_memory(aspace = %ld, \"%s\", virtual = %p, "
		"spec = %ld, size = %lu, protection = %ld, phys = %#" B_PRIxPHYSADDR
		")\n", team, name, *_address, addressSpec, size, protection,
		physicalAddress));

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
	addressRestrictions.address_specification = addressSpec & ~B_MTR_MASK;
	status = map_backing_store(locker.AddressSpace(), cache, 0, name, size,
		B_FULL_LOCK, protection, REGION_NO_PRIVATE_MAP, 0, &addressRestrictions,
		true, &area, _address);

	if (status < B_OK)
		cache->ReleaseRefLocked();

	cache->Unlock();

	if (status == B_OK) {
		// set requested memory type -- use uncached, if not given
		uint32 memoryType = addressSpec & B_MTR_MASK;
		if (memoryType == 0)
			memoryType = B_MTR_UC;

		area->SetMemoryType(memoryType);

		status = arch_vm_set_memory_type(area, physicalAddress, memoryType);
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
	TRACE(("vm_map_physical_memory_vecs(team = %ld, \"%s\", virtual = %p, "
		"spec = %ld, _size = %p, protection = %ld, vecs = %p, "
		"vecCount = %ld)\n", team, name, *_address, addressSpec, _size,
		protection, vecs, vecCount));

	if (!arch_vm_supports_protection(protection)
		|| (addressSpec & B_MTR_MASK) != 0) {
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
	addressRestrictions.address_specification = addressSpec & ~B_MTR_MASK;
	result = map_backing_store(locker.AddressSpace(), cache, 0, name,
		size, B_FULL_LOCK, protection, REGION_NO_PRIVATE_MAP, 0,
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
		B_LAZY_LOCK, B_KERNEL_READ_AREA, REGION_NO_PRIVATE_MAP, flags,
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
	vm_page_reservation* reservation)
{
	addr_t baseAddress = area->Base();
	addr_t cacheOffset = area->cache_offset;
	page_num_t firstPage = cacheOffset / B_PAGE_SIZE;
	page_num_t endPage = firstPage + area->Size() / B_PAGE_SIZE;

	for (VMCachePagesTree::Iterator it
				= cache->pages.GetIterator(firstPage, true, true);
			vm_page* page = it.Next();) {
		if (page->cache_offset >= endPage)
			break;

		// skip busy and inactive pages
		if (page->busy || page->usage_count == 0)
			continue;

		DEBUG_PAGE_ACCESS_START(page);
		map_page(area, page,
			baseAddress + (page->cache_offset * B_PAGE_SIZE - cacheOffset),
			B_READ_AREA | B_KERNEL_READ_AREA, reservation);
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
	TRACE(("_vm_map_file(fd = %d, offset = %Ld, size = %lu, mapping %ld)\n",
		fd, offset, size, mapping));

	offset = ROUNDDOWN(offset, B_PAGE_SIZE);
	size = PAGE_ALIGN(size);

	if (mapping == REGION_NO_PRIVATE_MAP)
		protection |= B_SHARED_AREA;
	if (addressSpec != B_EXACT_ADDRESS)
		unmapAddressRange = false;

	if (fd < 0) {
		uint32 flags = unmapAddressRange ? CREATE_AREA_UNMAP_ADDRESS_RANGE : 0;
		virtual_address_restrictions virtualRestrictions = {};
		virtualRestrictions.address = *_address;
		virtualRestrictions.address_specification = addressSpec;
		physical_address_restrictions physicalRestrictions = {};
		return vm_create_anonymous_area(team, name, size, B_NO_LOCK, protection,
			flags, &virtualRestrictions, &physicalRestrictions, kernel,
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

	// get the vnode for the object, this also grabs a ref to it
	struct vnode* vnode = NULL;
	status_t status = vfs_get_vnode_from_fd(fd, kernel, &vnode);
	if (status < B_OK)
		return status;
	CObjectDeleter<struct vnode> vnodePutter(vnode, vfs_put_vnode);

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

	VMArea* area;
	virtual_address_restrictions addressRestrictions = {};
	addressRestrictions.address = *_address;
	addressRestrictions.address_specification = addressSpec;
	status = map_backing_store(locker.AddressSpace(), cache, offset, name, size,
		0, protection, mapping,
		unmapAddressRange ? CREATE_AREA_UNMAP_ADDRESS_RANGE : 0,
		&addressRestrictions, kernel, &area, _address);

	if (status != B_OK || mapping == REGION_PRIVATE_MAP) {
		// map_backing_store() cannot know we no longer need the ref
		cache->ReleaseRefLocked();
	}

	if (status == B_OK && (protection & B_READ_AREA) != 0)
		pre_map_area_pages(area, cache, &reservation);

	cache->Unlock();

	if (status == B_OK) {
		// TODO: this probably deserves a smarter solution, ie. don't always
		// prefetch stuff, and also, probably don't trigger it at this place.
		cache_prefetch_vnode(vnode, offset, min_c(size, 10LL * 1024 * 1024));
			// prefetches at max 10 MB starting from "offset"
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
	VMArea* newArea = NULL;
	VMArea* sourceArea;

	// Check whether the source area exists and is cloneable. If so, mark it
	// B_SHARED_AREA, so that we don't get problems with copy-on-write.
	{
		AddressSpaceWriteLocker locker;
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

	sourceArea = lookup_area(sourceAddressSpace, sourceID);
	if (sourceArea == NULL)
		return B_BAD_VALUE;

	if (!kernel && (sourceArea->protection & B_KERNEL_AREA) != 0)
		return B_NOT_ALLOWED;

	VMCache* cache = vm_area_get_locked_cache(sourceArea);

	// TODO: for now, B_USER_CLONEABLE is disabled, until all drivers
	//	have been adapted. Maybe it should be part of the kernel settings,
	//	anyway (so that old drivers can always work).
#if 0
	if (sourceArea->aspace == VMAddressSpace::Kernel()
		&& addressSpace != VMAddressSpace::Kernel()
		&& !(sourceArea->protection & B_USER_CLONEABLE_AREA)) {
		// kernel areas must not be cloned in userland, unless explicitly
		// declared user-cloneable upon construction
		status = B_NOT_ALLOWED;
	} else
#endif
	if (sourceArea->cache_type == CACHE_TYPE_NULL)
		status = B_NOT_ALLOWED;
	else {
		virtual_address_restrictions addressRestrictions = {};
		addressRestrictions.address = *address;
		addressRestrictions.address_specification = addressSpec;
		status = map_backing_store(targetAddressSpace, cache,
			sourceArea->cache_offset, name, sourceArea->Size(),
			sourceArea->wiring, protection, mapping, 0, &addressRestrictions,
			kernel, &newArea, address);
	}
	if (status == B_OK && mapping != REGION_PRIVATE_MAP) {
		// If the mapping is REGION_PRIVATE_MAP, map_backing_store() needed
		// to create a new cache, and has therefore already acquired a reference
		// to the source cache - but otherwise it has no idea that we need
		// one.
		cache->AcquireRefLocked();
	}
	if (status == B_OK && newArea->wiring == B_FULL_LOCK) {
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
	if (status == B_OK)
		newArea->cache_type = sourceArea->cache_type;

	vm_area_put_locked_cache(cache);

	if (status < B_OK)
		return status;

	return newArea->id;
}


/*!	Deletes the specified area of the given address space.

	The address space must be write-locked.
	The caller must ensure that the area does not have any wired ranges.

	\param addressSpace The address space containing the area.
	\param area The area to be deleted.
	\param deletingAddressSpace \c true, if the address space is in the process
		of being deleted.
*/
static void
delete_area(VMAddressSpace* addressSpace, VMArea* area,
	bool deletingAddressSpace)
{
	ASSERT(!area->IsWired());

	VMAreaHash::Remove(area);

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
	TRACE(("vm_delete_area(team = 0x%lx, area = 0x%lx)\n", team, id));

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
	VMCache* upperCache;

	TRACE(("vm_copy_on_write_area(cache = %p)\n", lowerCache));

	// We need to separate the cache from its areas. The cache goes one level
	// deeper and we create a new cache inbetween.

	// create an anonymous cache
	status_t status = VMCacheFactory::CreateAnonymousCache(upperCache, false, 0,
		0, dynamic_cast<VMAnonymousNoSwapCache*>(lowerCache) == NULL,
		VM_PRIORITY_USER);
	if (status != B_OK)
		return status;

	upperCache->Lock();

	upperCache->temporary = 1;
	upperCache->virtual_base = lowerCache->virtual_base;
	upperCache->virtual_end = lowerCache->virtual_end;

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
				for (VMArea* tempArea = upperCache->areas; tempArea != NULL;
						tempArea = tempArea->cache_next) {
					// The area must be readable in the same way it was
					// previously writable.
					uint32 protection = B_KERNEL_READ_AREA;
					if ((tempArea->protection & B_READ_AREA) != 0)
						protection |= B_READ_AREA;

					VMTranslationMap* map
						= tempArea->address_space->TranslationMap();
					map->Lock();
					map->ProtectPage(tempArea,
						virtual_page_address(tempArea, page), protection);
					map->Unlock();
				}
			}
		}
	} else {
		ASSERT(lowerCache->WiredPagesCount() == 0);

		// just change the protection of all areas
		for (VMArea* tempArea = upperCache->areas; tempArea != NULL;
				tempArea = tempArea->cache_next) {
			// The area must be readable in the same way it was previously
			// writable.
			uint32 protection = B_KERNEL_READ_AREA;
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
	uint32 addressSpec, uint32 protection, area_id sourceID)
{
	bool writableCopy = (protection & (B_KERNEL_WRITE_AREA | B_WRITE_AREA)) != 0;

	if ((protection & B_KERNEL_PROTECTION) == 0) {
		// set the same protection for the kernel as for userland
		protection |= B_KERNEL_READ_AREA;
		if (writableCopy)
			protection |= B_KERNEL_WRITE_AREA;
	}

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
	struct PagesUnreserver {
		PagesUnreserver(vm_page_reservation* reservation)
			:
			fReservation(reservation)
		{
		}

		~PagesUnreserver()
		{
			if (fReservation != NULL)
				vm_page_unreserve_pages(fReservation);
		}

	private:
		vm_page_reservation*	fReservation;
	} pagesUnreserver(wiredPages > 0 ? &wiredPagesReservation : NULL);

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
		name, source->Size(), source->wiring, protection,
		sharedArea ? REGION_NO_PRIVATE_MAP : REGION_PRIVATE_MAP,
		writableCopy ? 0 : CREATE_AREA_DONT_COMMIT_MEMORY,
		&addressRestrictions, true, &target, _address);
	if (status < B_OK)
		return status;

	if (sharedArea) {
		// The new area uses the old area's cache, but map_backing_store()
		// hasn't acquired a ref. So we have to do that now.
		cache->AcquireRefLocked();
	}

	// If the source area is writable, we need to move it one layer up as well

	if (!sharedArea) {
		if ((source->protection & (B_KERNEL_WRITE_AREA | B_WRITE_AREA)) != 0) {
			// TODO: do something more useful if this fails!
			if (vm_copy_on_write_area(cache,
					wiredPages > 0 ? &wiredPagesReservation : NULL) < B_OK) {
				panic("vm_copy_on_write_area() failed!\n");
			}
		}
	}

	// we return the ID of the newly created area
	return target->id;
}


static status_t
vm_set_area_protection(team_id team, area_id areaID, uint32 newProtection,
	bool kernel)
{
	TRACE(("vm_set_area_protection(team = %#lx, area = %#lx, protection = "
		"%#lx)\n", team, areaID, newProtection));

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

		if (!kernel && (area->protection & B_KERNEL_AREA) != 0)
			return B_NOT_ALLOWED;

		if (area->protection == newProtection)
			return B_OK;

		if (team != VMAddressSpace::KernelID()
			&& area->address_space->ID() != team) {
			// unless you're the kernel, you are only allowed to set
			// the protection of your own areas
			return B_NOT_ALLOWED;
		}

		isWritable
			= (area->protection & (B_WRITE_AREA | B_KERNEL_WRITE_AREA)) != 0;

		// Make sure the area (respectively, if we're going to call
		// vm_copy_on_write_area(), all areas of the cache) doesn't have any
		// wired ranges.
		if (!isWritable && becomesWritable && !cache->consumers.IsEmpty()) {
			for (VMArea* otherArea = cache->areas; otherArea != NULL;
					otherArea = otherArea->cache_next) {
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
			if (cache->source != NULL && cache->temporary) {
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


static int
display_mem(int argc, char** argv)
{
	bool physical = false;
	addr_t copyAddress;
	int32 displayWidth;
	int32 itemSize;
	int32 num = -1;
	addr_t address;
	int i = 1, j;

	if (argc > 1 && argv[1][0] == '-') {
		if (!strcmp(argv[1], "-p") || !strcmp(argv[1], "--physical")) {
			physical = true;
			i++;
		} else
			i = 99;
	}

	if (argc < i + 1 || argc > i + 2) {
		kprintf("usage: dl/dw/ds/db/string [-p|--physical] <address> [num]\n"
			"\tdl - 8 bytes\n"
			"\tdw - 4 bytes\n"
			"\tds - 2 bytes\n"
			"\tdb - 1 byte\n"
			"\tstring - a whole string\n"
			"  -p or --physical only allows memory from a single page to be "
			"displayed.\n");
		return 0;
	}

	address = parse_expression(argv[i]);

	if (argc > i + 1)
		num = parse_expression(argv[i + 1]);

	// build the format string
	if (strcmp(argv[0], "db") == 0) {
		itemSize = 1;
		displayWidth = 16;
	} else if (strcmp(argv[0], "ds") == 0) {
		itemSize = 2;
		displayWidth = 8;
	} else if (strcmp(argv[0], "dw") == 0) {
		itemSize = 4;
		displayWidth = 4;
	} else if (strcmp(argv[0], "dl") == 0) {
		itemSize = 8;
		displayWidth = 2;
	} else if (strcmp(argv[0], "string") == 0) {
		itemSize = 1;
		displayWidth = -1;
	} else {
		kprintf("display_mem called in an invalid way!\n");
		return 0;
	}

	if (num <= 0)
		num = displayWidth;

	void* physicalPageHandle = NULL;

	if (physical) {
		int32 offset = address & (B_PAGE_SIZE - 1);
		if (num * itemSize + offset > B_PAGE_SIZE) {
			num = (B_PAGE_SIZE - offset) / itemSize;
			kprintf("NOTE: number of bytes has been cut to page size\n");
		}

		address = ROUNDDOWN(address, B_PAGE_SIZE);

		if (vm_get_physical_page_debug(address, &copyAddress,
				&physicalPageHandle) != B_OK) {
			kprintf("getting the hardware page failed.");
			return 0;
		}

		address += offset;
		copyAddress += offset;
	} else
		copyAddress = address;

	if (!strcmp(argv[0], "string")) {
		kprintf("%p \"", (char*)copyAddress);

		// string mode
		for (i = 0; true; i++) {
			char c;
			if (debug_memcpy(B_CURRENT_TEAM, &c, (char*)copyAddress + i, 1)
					!= B_OK
				|| c == '\0') {
				break;
			}

			if (c == '\n')
				kprintf("\\n");
			else if (c == '\t')
				kprintf("\\t");
			else {
				if (!isprint(c))
					c = '.';

				kprintf("%c", c);
			}
		}

		kprintf("\"\n");
	} else {
		// number mode
		for (i = 0; i < num; i++) {
			uint32 value;

			if ((i % displayWidth) == 0) {
				int32 displayed = min_c(displayWidth, (num-i)) * itemSize;
				if (i != 0)
					kprintf("\n");

				kprintf("[0x%lx]  ", address + i * itemSize);

				for (j = 0; j < displayed; j++) {
					char c;
					if (debug_memcpy(B_CURRENT_TEAM, &c,
							(char*)copyAddress + i * itemSize + j, 1) != B_OK) {
						displayed = j;
						break;
					}
					if (!isprint(c))
						c = '.';

					kprintf("%c", c);
				}
				if (num > displayWidth) {
					// make sure the spacing in the last line is correct
					for (j = displayed; j < displayWidth * itemSize; j++)
						kprintf(" ");
				}
				kprintf("  ");
			}

			if (debug_memcpy(B_CURRENT_TEAM, &value,
					(uint8*)copyAddress + i * itemSize, itemSize) != B_OK) {
				kprintf("read fault");
				break;
			}

			switch (itemSize) {
				case 1:
					kprintf(" %02x", *(uint8*)&value);
					break;
				case 2:
					kprintf(" %04x", *(uint16*)&value);
					break;
				case 4:
					kprintf(" %08lx", *(uint32*)&value);
					break;
				case 8:
					kprintf(" %016Lx", *(uint64*)&value);
					break;
			}
		}

		kprintf("\n");
	}

	if (physical) {
		copyAddress = ROUNDDOWN(copyAddress, B_PAGE_SIZE);
		vm_put_physical_page_debug(copyAddress, physicalPageHandle);
	}
	return 0;
}


static void
dump_cache_tree_recursively(VMCache* cache, int level,
	VMCache* highlightCache)
{
	// print this cache
	for (int i = 0; i < level; i++)
		kprintf("  ");
	if (cache == highlightCache)
		kprintf("%p <--\n", cache);
	else
		kprintf("%p\n", cache);

	// recursively print its consumers
	for (VMCache::ConsumerList::Iterator it = cache->consumers.GetIterator();
			VMCache* consumer = it.Next();) {
		dump_cache_tree_recursively(consumer, level + 1, highlightCache);
	}
}


static int
dump_cache_tree(int argc, char** argv)
{
	if (argc != 2 || !strcmp(argv[1], "--help")) {
		kprintf("usage: %s <address>\n", argv[0]);
		return 0;
	}

	addr_t address = parse_expression(argv[1]);
	if (address == 0)
		return 0;

	VMCache* cache = (VMCache*)address;
	VMCache* root = cache;

	// find the root cache (the transitive source)
	while (root->source != NULL)
		root = root->source;

	dump_cache_tree_recursively(root, 0, cache);

	return 0;
}


const char*
vm_cache_type_to_string(int32 type)
{
	switch (type) {
		case CACHE_TYPE_RAM:
			return "RAM";
		case CACHE_TYPE_DEVICE:
			return "device";
		case CACHE_TYPE_VNODE:
			return "vnode";
		case CACHE_TYPE_NULL:
			return "null";

		default:
			return "unknown";
	}
}


#if DEBUG_CACHE_LIST

static void
update_cache_info_recursively(VMCache* cache, cache_info& info)
{
	info.page_count += cache->page_count;
	if (cache->type == CACHE_TYPE_RAM)
		info.committed += cache->committed_size;

	// recurse
	for (VMCache::ConsumerList::Iterator it = cache->consumers.GetIterator();
			VMCache* consumer = it.Next();) {
		update_cache_info_recursively(consumer, info);
	}
}


static int
cache_info_compare_page_count(const void* _a, const void* _b)
{
	const cache_info* a = (const cache_info*)_a;
	const cache_info* b = (const cache_info*)_b;
	if (a->page_count == b->page_count)
		return 0;
	return a->page_count < b->page_count ? 1 : -1;
}


static int
cache_info_compare_committed(const void* _a, const void* _b)
{
	const cache_info* a = (const cache_info*)_a;
	const cache_info* b = (const cache_info*)_b;
	if (a->committed == b->committed)
		return 0;
	return a->committed < b->committed ? 1 : -1;
}


static void
dump_caches_recursively(VMCache* cache, cache_info& info, int level)
{
	for (int i = 0; i < level; i++)
		kprintf("  ");

	kprintf("%p: type: %s, base: %lld, size: %lld, pages: %lu", cache,
		vm_cache_type_to_string(cache->type), cache->virtual_base,
		cache->virtual_end, cache->page_count);

	if (level == 0)
		kprintf("/%lu", info.page_count);

	if (cache->type == CACHE_TYPE_RAM || (level == 0 && info.committed > 0)) {
		kprintf(", committed: %lld", cache->committed_size);

		if (level == 0)
			kprintf("/%lu", info.committed);
	}

	// areas
	if (cache->areas != NULL) {
		VMArea* area = cache->areas;
		kprintf(", areas: %ld (%s, team: %ld)", area->id, area->name,
			area->address_space->ID());

		while (area->cache_next != NULL) {
			area = area->cache_next;
			kprintf(", %ld", area->id);
		}
	}

	kputs("\n");

	// recurse
	for (VMCache::ConsumerList::Iterator it = cache->consumers.GetIterator();
			VMCache* consumer = it.Next();) {
		dump_caches_recursively(consumer, info, level + 1);
	}
}


static int
dump_caches(int argc, char** argv)
{
	if (sCacheInfoTable == NULL) {
		kprintf("No cache info table!\n");
		return 0;
	}

	bool sortByPageCount = true;

	for (int32 i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-c") == 0) {
			sortByPageCount = false;
		} else {
			print_debugger_command_usage(argv[0]);
			return 0;
		}
	}

	uint32 totalCount = 0;
	uint32 rootCount = 0;
	off_t totalCommitted = 0;
	page_num_t totalPages = 0;

	VMCache* cache = gDebugCacheList;
	while (cache) {
		totalCount++;
		if (cache->source == NULL) {
			cache_info stackInfo;
			cache_info& info = rootCount < (uint32)kCacheInfoTableCount
				? sCacheInfoTable[rootCount] : stackInfo;
			rootCount++;
			info.cache = cache;
			info.page_count = 0;
			info.committed = 0;
			update_cache_info_recursively(cache, info);
			totalCommitted += info.committed;
			totalPages += info.page_count;
		}

		cache = cache->debug_next;
	}

	if (rootCount <= (uint32)kCacheInfoTableCount) {
		qsort(sCacheInfoTable, rootCount, sizeof(cache_info),
			sortByPageCount
				? &cache_info_compare_page_count
				: &cache_info_compare_committed);
	}

	kprintf("total committed memory: %" B_PRIdOFF ", total used pages: %"
		B_PRIuPHYSADDR "\n", totalCommitted, totalPages);
	kprintf("%lu caches (%lu root caches), sorted by %s per cache "
		"tree...\n\n", totalCount, rootCount,
		sortByPageCount ? "page count" : "committed size");

	if (rootCount <= (uint32)kCacheInfoTableCount) {
		for (uint32 i = 0; i < rootCount; i++) {
			cache_info& info = sCacheInfoTable[i];
			dump_caches_recursively(info.cache, info, 0);
		}
	} else
		kprintf("Cache info table too small! Can't sort and print caches!\n");

	return 0;
}

#endif	// DEBUG_CACHE_LIST


static int
dump_cache(int argc, char** argv)
{
	VMCache* cache;
	bool showPages = false;
	int i = 1;

	if (argc < 2 || !strcmp(argv[1], "--help")) {
		kprintf("usage: %s [-ps] <address>\n"
			"  if -p is specified, all pages are shown, if -s is used\n"
			"  only the cache info is shown respectively.\n", argv[0]);
		return 0;
	}
	while (argv[i][0] == '-') {
		char* arg = argv[i] + 1;
		while (arg[0]) {
			if (arg[0] == 'p')
				showPages = true;
			arg++;
		}
		i++;
	}
	if (argv[i] == NULL) {
		kprintf("%s: invalid argument, pass address\n", argv[0]);
		return 0;
	}

	addr_t address = parse_expression(argv[i]);
	if (address == 0)
		return 0;

	cache = (VMCache*)address;

	cache->Dump(showPages);

	set_debug_variable("_sourceCache", (addr_t)cache->source);

	return 0;
}


static void
dump_area_struct(VMArea* area, bool mappings)
{
	kprintf("AREA: %p\n", area);
	kprintf("name:\t\t'%s'\n", area->name);
	kprintf("owner:\t\t0x%lx\n", area->address_space->ID());
	kprintf("id:\t\t0x%lx\n", area->id);
	kprintf("base:\t\t0x%lx\n", area->Base());
	kprintf("size:\t\t0x%lx\n", area->Size());
	kprintf("protection:\t0x%lx\n", area->protection);
	kprintf("wiring:\t\t0x%x\n", area->wiring);
	kprintf("memory_type:\t%#" B_PRIx32 "\n", area->MemoryType());
	kprintf("cache:\t\t%p\n", area->cache);
	kprintf("cache_type:\t%s\n", vm_cache_type_to_string(area->cache_type));
	kprintf("cache_offset:\t0x%Lx\n", area->cache_offset);
	kprintf("cache_next:\t%p\n", area->cache_next);
	kprintf("cache_prev:\t%p\n", area->cache_prev);

	VMAreaMappings::Iterator iterator = area->mappings.GetIterator();
	if (mappings) {
		kprintf("page mappings:\n");
		while (iterator.HasNext()) {
			vm_page_mapping* mapping = iterator.Next();
			kprintf("  %p", mapping->page);
		}
		kprintf("\n");
	} else {
		uint32 count = 0;
		while (iterator.Next() != NULL) {
			count++;
		}
		kprintf("page mappings:\t%lu\n", count);
	}
}


static int
dump_area(int argc, char** argv)
{
	bool mappings = false;
	bool found = false;
	int32 index = 1;
	VMArea* area;
	addr_t num;

	if (argc < 2 || !strcmp(argv[1], "--help")) {
		kprintf("usage: area [-m] [id|contains|address|name] <id|address|name>\n"
			"All areas matching either id/address/name are listed. You can\n"
			"force to check only a specific item by prefixing the specifier\n"
			"with the id/contains/address/name keywords.\n"
			"-m shows the area's mappings as well.\n");
		return 0;
	}

	if (!strcmp(argv[1], "-m")) {
		mappings = true;
		index++;
	}

	int32 mode = 0xf;
	if (!strcmp(argv[index], "id"))
		mode = 1;
	else if (!strcmp(argv[index], "contains"))
		mode = 2;
	else if (!strcmp(argv[index], "name"))
		mode = 4;
	else if (!strcmp(argv[index], "address"))
		mode = 0;
	if (mode != 0xf)
		index++;

	if (index >= argc) {
		kprintf("No area specifier given.\n");
		return 0;
	}

	num = parse_expression(argv[index]);

	if (mode == 0) {
		dump_area_struct((struct VMArea*)num, mappings);
	} else {
		// walk through the area list, looking for the arguments as a name

		VMAreaHashTable::Iterator it = VMAreaHash::GetIterator();
		while ((area = it.Next()) != NULL) {
			if (((mode & 4) != 0 && area->name != NULL
					&& !strcmp(argv[index], area->name))
				|| (num != 0 && (((mode & 1) != 0 && (addr_t)area->id == num)
					|| (((mode & 2) != 0 && area->Base() <= num
						&& area->Base() + area->Size() > num))))) {
				dump_area_struct(area, mappings);
				found = true;
			}
		}

		if (!found)
			kprintf("could not find area %s (%ld)\n", argv[index], num);
	}

	return 0;
}


static int
dump_area_list(int argc, char** argv)
{
	VMArea* area;
	const char* name = NULL;
	int32 id = 0;

	if (argc > 1) {
		id = parse_expression(argv[1]);
		if (id == 0)
			name = argv[1];
	}

	kprintf("addr          id  base\t\tsize    protect lock  name\n");

	VMAreaHashTable::Iterator it = VMAreaHash::GetIterator();
	while ((area = it.Next()) != NULL) {
		if ((id != 0 && area->address_space->ID() != id)
			|| (name != NULL && strstr(area->name, name) == NULL))
			continue;

		kprintf("%p %5lx  %p\t%p %4lx\t%4d  %s\n", area, area->id,
			(void*)area->Base(), (void*)area->Size(), area->protection,
			area->wiring, area->name);
	}
	return 0;
}


static int
dump_available_memory(int argc, char** argv)
{
	kprintf("Available memory: %" B_PRIdOFF "/%" B_PRIuPHYSADDR " bytes\n",
		sAvailableMemory, (phys_addr_t)vm_page_num_pages() * B_PAGE_SIZE);
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
	TRACE(("vm_delete_areas: called on address space 0x%lx\n",
		addressSpace->ID()));

	addressSpace->WriteLock();

	// remove all reserved areas in this address space
	addressSpace->UnreserveAllAddressRanges(0);

	// delete all the areas in this address space
	while (VMArea* area = addressSpace->FirstArea()) {
		ASSERT(!area->IsWired());
		delete_area(addressSpace, area, deletingAddressSpace);
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
		if (!kernel && (area->protection & (B_READ_AREA | B_WRITE_AREA)) == 0)
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
				vm_page_set_state(page, PAGE_STATE_FREE);
			}
		}
	}

	// unmap the memory
	map->Unmap(start, end);
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
create_preloaded_image_areas(struct preloaded_image* image)
{
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
	uint32 i;

	TRACE(("vm_free_kernel_args()\n"));

	for (i = 0; i < args->num_kernel_args_ranges; i++) {
		area_id area = area_for((void*)args->kernel_args_range[i].start);
		if (area >= B_OK)
			delete_area(area);
	}
}


static void
allocate_kernel_args(kernel_args* args)
{
	TRACE(("allocate_kernel_args()\n"));

	for (uint32 i = 0; i < args->num_kernel_args_ranges; i++) {
		void* address = (void*)args->kernel_args_range[i].start;

		create_area("_kernel args_", &address, B_EXACT_ADDRESS,
			args->kernel_args_range[i].size, B_ALREADY_WIRED,
			B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	}
}


static void
unreserve_boot_loader_ranges(kernel_args* args)
{
	TRACE(("unreserve_boot_loader_ranges()\n"));

	for (uint32 i = 0; i < args->num_virtual_allocated_ranges; i++) {
		vm_unreserve_address_range(VMAddressSpace::KernelID(),
			(void*)args->virtual_allocated_range[i].start,
			args->virtual_allocated_range[i].size);
	}
}


static void
reserve_boot_loader_ranges(kernel_args* args)
{
	TRACE(("reserve_boot_loader_ranges()\n"));

	for (uint32 i = 0; i < args->num_virtual_allocated_ranges; i++) {
		void* address = (void*)args->virtual_allocated_range[i].start;

		// If the address is no kernel address, we just skip it. The
		// architecture specific code has to deal with it.
		if (!IS_KERNEL_ADDRESS(address)) {
			dprintf("reserve_boot_loader_ranges(): Skipping range: %p, %lu\n",
				address, args->virtual_allocated_range[i].size);
			continue;
		}

		status_t status = vm_reserve_address_range(VMAddressSpace::KernelID(),
			&address, B_EXACT_ADDRESS, args->virtual_allocated_range[i].size, 0);
		if (status < B_OK)
			panic("could not reserve boot loader ranges\n");
	}
}


static addr_t
allocate_early_virtual(kernel_args* args, size_t size, addr_t alignment)
{
	size = PAGE_ALIGN(size);

	// find a slot in the virtual allocation addr range
	for (uint32 i = 1; i < args->num_virtual_allocated_ranges; i++) {
		// check to see if the space between this one and the last is big enough
		addr_t rangeStart = args->virtual_allocated_range[i].start;
		addr_t previousRangeEnd = args->virtual_allocated_range[i - 1].start
			+ args->virtual_allocated_range[i - 1].size;

		addr_t base = alignment > 0
			? ROUNDUP(previousRangeEnd, alignment) : previousRangeEnd;

		if (base >= KERNEL_BASE && base < rangeStart
				&& rangeStart - base >= size) {
			args->virtual_allocated_range[i - 1].size
				+= base + size - previousRangeEnd;
			return base;
		}
	}

	// we hadn't found one between allocation ranges. this is ok.
	// see if there's a gap after the last one
	int lastEntryIndex = args->num_virtual_allocated_ranges - 1;
	addr_t lastRangeEnd = args->virtual_allocated_range[lastEntryIndex].start
		+ args->virtual_allocated_range[lastEntryIndex].size;
	addr_t base = alignment > 0
		? ROUNDUP(lastRangeEnd, alignment) : lastRangeEnd;
	if (KERNEL_BASE + (KERNEL_SIZE - 1) - base >= size) {
		args->virtual_allocated_range[lastEntryIndex].size
			+= base + size - lastRangeEnd;
		return base;
	}

	// see if there's a gap before the first one
	addr_t rangeStart = args->virtual_allocated_range[0].start;
	if (rangeStart > KERNEL_BASE && rangeStart - KERNEL_BASE >= size) {
		base = rangeStart - size;
		if (alignment > 0)
			base = ROUNDDOWN(base, alignment);

		if (base >= KERNEL_BASE) {
			args->virtual_allocated_range[0].start = base;
			args->virtual_allocated_range[0].size += rangeStart - base;
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
		if (address >= args->physical_memory_range[i].start
			&& address < args->physical_memory_range[i].start
				+ args->physical_memory_range[i].size)
			return true;
	}
	return false;
}


page_num_t
vm_allocate_early_physical_page(kernel_args* args)
{
	for (uint32 i = 0; i < args->num_physical_allocated_ranges; i++) {
		phys_addr_t nextPage;

		nextPage = args->physical_allocated_range[i].start
			+ args->physical_allocated_range[i].size;
		// see if the page after the next allocated paddr run can be allocated
		if (i + 1 < args->num_physical_allocated_ranges
			&& args->physical_allocated_range[i + 1].size != 0) {
			// see if the next page will collide with the next allocated range
			if (nextPage >= args->physical_allocated_range[i+1].start)
				continue;
		}
		// see if the next physical page fits in the memory block
		if (is_page_in_physical_memory_range(args, nextPage)) {
			// we got one!
			args->physical_allocated_range[i].size += B_PAGE_SIZE;
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

	// map the pages
	for (uint32 i = 0; i < PAGE_ALIGN(physicalSize) / B_PAGE_SIZE; i++) {
		page_num_t physicalAddress = vm_allocate_early_physical_page(args);
		if (physicalAddress == 0)
			panic("error allocating early page!\n");

		//dprintf("vm_allocate_early: paddr 0x%lx\n", physicalAddress);

		arch_vm_translation_map_early_map(args, virtualBase + i * B_PAGE_SIZE,
			physicalAddress * B_PAGE_SIZE, attributes,
			&vm_allocate_early_physical_page);
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
	sAvailableMemory = vm_page_num_pages() * B_PAGE_SIZE;

	slab_init(args);

#if USE_DEBUG_HEAP_FOR_MALLOC || USE_GUARDED_HEAP_FOR_MALLOC
	size_t heapSize = INITIAL_HEAP_SIZE;
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
		status_t error = VMAreaHash::Init();
		if (error != B_OK)
			panic("vm_init: error initializing area hash table\n");
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

	create_preloaded_image_areas(&args->kernel_image);

	// allocate areas for preloaded images
	for (image = args->preloaded_images; image != NULL; image = image->next)
		create_preloaded_image_areas(image);

	// allocate kernel stacks
	for (i = 0; i < args->num_cpus; i++) {
		char name[64];

		sprintf(name, "idle thread %lu kstack", i + 1);
		address = (void*)args->cpu_kstack[i].start;
		create_area(name, &address, B_EXACT_ADDRESS, args->cpu_kstack[i].size,
			B_ALREADY_WIRED, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	}

	void* lastPage = (void*)ROUNDDOWN(~(addr_t)0, B_PAGE_SIZE);
	vm_block_address_range("overflow protection", lastPage, B_PAGE_SIZE);

#if PARANOID_KERNEL_MALLOC
	vm_block_address_range("uninitialized heap memory",
		(void *)ROUNDDOWN(0xcccccccc, B_PAGE_SIZE), B_PAGE_SIZE * 64);
#endif
#if PARANOID_KERNEL_FREE
	vm_block_address_range("freed heap memory",
		(void *)ROUNDDOWN(0xdeadbeef, B_PAGE_SIZE), B_PAGE_SIZE * 64);
#endif

	// create the object cache for the page mappings
	gPageMappingsObjectCache = create_object_cache_etc("page mappings",
		sizeof(vm_page_mapping), 0, 0, 64, 128, CACHE_LARGE_SLAB, NULL, NULL,
		NULL, NULL);
	if (gPageMappingsObjectCache == NULL)
		panic("failed to create page mappings object cache");

	object_cache_set_minimum_reserve(gPageMappingsObjectCache, 1024);

#if DEBUG_CACHE_LIST
	if (vm_page_num_free_pages() >= 200 * 1024 * 1024 / B_PAGE_SIZE) {
		virtual_address_restrictions virtualRestrictions = {};
		virtualRestrictions.address_specification = B_ANY_KERNEL_ADDRESS;
		physical_address_restrictions physicalRestrictions = {};
		create_area_etc(VMAddressSpace::KernelID(), "cache info table",
			ROUNDUP(kCacheInfoTableCount * sizeof(cache_info), B_PAGE_SIZE),
			B_FULL_LOCK, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
			CREATE_AREA_DONT_WAIT, &virtualRestrictions, &physicalRestrictions,
			(void**)&sCacheInfoTable);
	}
#endif	// DEBUG_CACHE_LIST

	// add some debugger commands
	add_debugger_command("areas", &dump_area_list, "Dump a list of all areas");
	add_debugger_command("area", &dump_area,
		"Dump info about a particular area");
	add_debugger_command("cache", &dump_cache, "Dump VMCache");
	add_debugger_command("cache_tree", &dump_cache_tree, "Dump VMCache tree");
#if DEBUG_CACHE_LIST
	if (sCacheInfoTable != NULL) {
		add_debugger_command_etc("caches", &dump_caches,
			"List all VMCache trees",
			"[ \"-c\" ]\n"
			"All cache trees are listed sorted in decreasing order by number "
				"of\n"
			"used pages or, if \"-c\" is specified, by size of committed "
				"memory.\n",
			0);
	}
#endif
	add_debugger_command("avail", &dump_available_memory,
		"Dump available memory");
	add_debugger_command("dl", &display_mem, "dump memory long words (64-bit)");
	add_debugger_command("dw", &display_mem, "dump memory words (32-bit)");
	add_debugger_command("ds", &display_mem, "dump memory shorts (16-bit)");
	add_debugger_command("db", &display_mem, "dump memory bytes (8-bit)");
	add_debugger_command("string", &display_mem, "dump strings");

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
vm_page_fault(addr_t address, addr_t faultAddress, bool isWrite, bool isUser,
	addr_t* newIP)
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
		status = vm_soft_fault(addressSpace, pageAddress, isWrite, isUser,
			NULL);
	}

	if (status < B_OK) {
		dprintf("vm_page_fault: vm_soft_fault returned error '%s' on fault at "
			"0x%lx, ip 0x%lx, write %d, user %d, thread 0x%lx\n",
			strerror(status), address, faultAddress, isWrite, isUser,
			thread_get_current_thread_id());
		if (!isUser) {
			Thread* thread = thread_get_current_thread();
			if (thread != NULL && thread->fault_handler != 0) {
				// this will cause the arch dependant page fault handler to
				// modify the IP on the interrupt frame or whatever to return
				// to this address
				*newIP = thread->fault_handler;
			} else {
				// unhandled page fault in the kernel
				panic("vm_page_fault: unhandled page fault in kernel space at "
					"0x%lx, ip 0x%lx\n", address, faultAddress);
			}
		} else {
#if 1
			addressSpace->ReadLock();

			// TODO: remove me once we have proper userland debugging support
			// (and tools)
			VMArea* area = addressSpace->LookupArea(faultAddress);

			Thread* thread = thread_get_current_thread();
			dprintf("vm_page_fault: thread \"%s\" (%ld) in team \"%s\" (%ld) "
				"tried to %s address %#lx, ip %#lx (\"%s\" +%#lx)\n",
				thread->name, thread->id, thread->team->Name(),
				thread->team->id, isWrite ? "write" : "read", address,
				faultAddress, area ? area->name : "???",
				faultAddress - (area ? area->Base() : 0x0));

			// We can print a stack trace of the userland thread here.
// TODO: The user_memcpy() below can cause a deadlock, if it causes a page
// fault and someone is already waiting for a write lock on the same address
// space. This thread will then try to acquire the lock again and will
// be queued after the writer.
#	if 0
			if (area) {
				struct stack_frame {
					#if defined(__INTEL__) || defined(__POWERPC__) || defined(__M68K__)
						struct stack_frame*	previous;
						void*				return_address;
					#else
						// ...
					#warning writeme
					#endif
				} frame;
#		ifdef __INTEL__
				struct iframe* iframe = i386_get_user_iframe();
				if (iframe == NULL)
					panic("iframe is NULL!");

				status_t status = user_memcpy(&frame, (void*)iframe->ebp,
					sizeof(struct stack_frame));
#		elif defined(__POWERPC__)
				struct iframe* iframe = ppc_get_user_iframe();
				if (iframe == NULL)
					panic("iframe is NULL!");

				status_t status = user_memcpy(&frame, (void*)iframe->r1,
					sizeof(struct stack_frame));
#		else
#			warning "vm_page_fault() stack trace won't work"
				status = B_ERROR;
#		endif

				dprintf("stack trace:\n");
				int32 maxFrames = 50;
				while (status == B_OK && --maxFrames >= 0
						&& frame.return_address != NULL) {
					dprintf("  %p", frame.return_address);
					area = addressSpace->LookupArea(
						(addr_t)frame.return_address);
					if (area) {
						dprintf(" (%s + %#lx)", area->name,
							(addr_t)frame.return_address - area->Base());
					}
					dprintf("\n");

					status = user_memcpy(&frame, frame.previous,
						sizeof(struct stack_frame));
				}
			}
#	endif	// 0 (stack trace)

			addressSpace->ReadUnlock();
#endif

			// TODO: the fault_callback is a temporary solution for vm86
			if (thread->fault_callback == NULL
				|| thread->fault_callback(address, faultAddress, isWrite)) {
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
	VMCache* lastCache = NULL;
	vm_page* page = NULL;

	while (cache != NULL) {
		// We already hold the lock of the cache at this point.

		lastCache = cache;

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
		if (cache->HasPage(context.cacheOffset)) {
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
				vm_page_set_state(page, PAGE_STATE_FREE);

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
		// There was no adequate page, determine the cache for a clean one.
		// Read-only pages come in the deepest cache, only the top most cache
		// may have direct write access.
		cache = context.isWrite ? context.topCache : lastCache;

		// allocate a clean page
		page = vm_page_allocate_page(&context.reservation,
			PAGE_STATE_ACTIVE | VM_PAGE_ALLOC_CLEAR);
		FTRACE(("vm_soft_fault: just allocated page 0x%" B_PRIxPHYSADDR "\n",
			page->physical_page_number));

		// insert the new page into our cache
		cache->InsertPage(page, context.cacheOffset);
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

		// insert the new page into our cache
		context.topCache->InsertPage(page, context.cacheOffset);
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
	\param wiredRange If given, this wiredRange is ignored when checking whether
		an already mapped page at the virtual address can be unmapped.
	\return \c B_OK on success, another error code otherwise.
*/
static status_t
vm_soft_fault(VMAddressSpace* addressSpace, addr_t originalAddress,
	bool isWrite, bool isUser, vm_page** wirePage, VMAreaWiredRange* wiredRange)
{
	FTRACE(("vm_soft_fault: thid 0x%lx address 0x%lx, isWrite %d, isUser %d\n",
		thread_get_current_thread_id(), originalAddress, isWrite, isUser));

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

	while (true) {
		context.addressSpaceLocker.Lock();

		// get the area the fault was in
		VMArea* area = addressSpace->LookupArea(address);
		if (area == NULL) {
			dprintf("vm_soft_fault: va 0x%lx not covered by area in address "
				"space\n", originalAddress);
			TPF(PageFaultError(-1,
				VMPageFaultTracing::PAGE_FAULT_ERROR_NO_AREA));
			status = B_BAD_ADDRESS;
			break;
		}

		// check permissions
		uint32 protection = get_area_page_protection(area, address);
		if (isUser && (protection & B_USER_PROTECTION) == 0) {
			dprintf("user access on kernel area 0x%lx at %p\n", area->id,
				(void*)originalAddress);
			TPF(PageFaultError(area->id,
				VMPageFaultTracing::PAGE_FAULT_ERROR_KERNEL_ONLY));
			status = B_PERMISSION_DENIED;
			break;
		}
		if (isWrite && (protection
				& (B_WRITE_AREA | (isUser ? 0 : B_KERNEL_WRITE_AREA))) == 0) {
			dprintf("write access attempted on write-protected area 0x%lx at"
				" %p\n", area->id, (void*)originalAddress);
			TPF(PageFaultError(area->id,
				VMPageFaultTracing::PAGE_FAULT_ERROR_WRITE_PROTECTED));
			status = B_PERMISSION_DENIED;
			break;
		} else if (!isWrite && (protection
				& (B_READ_AREA | (isUser ? 0 : B_KERNEL_READ_AREA))) == 0) {
			dprintf("read access attempted on read-protected area 0x%lx at"
				" %p\n", area->id, (void*)originalAddress);
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
			// again and restart.
			VMAreaUnwiredWaiter waiter;
			if (area->AddWaiterIfWired(&waiter, address, B_PAGE_SIZE,
					wiredRange)) {
				// unlock everything and wait
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

				if (object_cache_reserve(gPageMappingsObjectCache, 1, 0)
						!= B_OK) {
					// Apparently the situation is serious. Let's get ourselves
					// killed.
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
vm_get_info(system_memory_info* info)
{
	swap_get_info(info);

	info->max_memory = vm_page_num_pages() * B_PAGE_SIZE;
	info->page_faults = sPageFaults;

	MutexLocker locker(sAvailableMemoryLock);
	info->free_memory = sAvailableMemory;
	info->needed_memory = sNeededMemory;
}


uint32
vm_num_page_faults(void)
{
	return sPageFaults;
}


off_t
vm_available_memory(void)
{
	MutexLocker locker(sAvailableMemoryLock);
	return sAvailableMemory;
}


off_t
vm_available_not_needed_memory(void)
{
	MutexLocker locker(sAvailableMemoryLock);
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
	mutex_lock(&sAvailableMemoryLock);

	sAvailableMemory += amount;

	mutex_unlock(&sAvailableMemoryLock);
}


status_t
vm_try_reserve_memory(size_t amount, int priority, bigtime_t timeout)
{
	size_t reserve = kMemoryReserveForPriority[priority];

	MutexLocker locker(sAvailableMemoryLock);

	//dprintf("try to reserve %lu bytes, %Lu left\n", amount, sAvailableMemory);

	if (sAvailableMemory >= amount + reserve) {
		sAvailableMemory -= amount;
		return B_OK;
	}

	if (timeout <= 0)
		return B_NO_MEMORY;

	// turn timeout into an absolute timeout
	timeout += system_time();

	// loop until we've got the memory or the timeout occurs
	do {
		sNeededMemory += amount;

		// call the low resource manager
		locker.Unlock();
		low_resource(B_KERNEL_RESOURCE_MEMORY, sNeededMemory - sAvailableMemory,
			B_ABSOLUTE_TIMEOUT, timeout);
		locker.Lock();

		sNeededMemory -= amount;

		if (sAvailableMemory >= amount + reserve) {
			sAvailableMemory -= amount;
			return B_OK;
		}
	} while (timeout > system_time());

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
	status_t error = arch_vm_set_memory_type(area, physicalBase, type);
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
	 - if B_WRITE_AREA is set, B_WRITE_KERNEL_AREA is set as well
	 - if only B_READ_AREA has been set, B_KERNEL_READ_AREA is also set
	 - if no protection is specified, it defaults to B_KERNEL_READ_AREA
	   and B_KERNEL_WRITE_AREA.
*/
static void
fix_protection(uint32* protection)
{
	if ((*protection & B_KERNEL_PROTECTION) == 0) {
		if ((*protection & B_USER_PROTECTION) == 0
			|| (*protection & B_WRITE_AREA) != 0)
			*protection |= B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA;
		else
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
	info->lock = B_FULL_LOCK;
	info->team = area->address_space->ID();
	info->copy_count = 0;
	info->in_count = 0;
	info->out_count = 0;
		// TODO: retrieve real values here!

	VMCache* cache = vm_area_get_locked_cache(area);

	// Note, this is a simplification; the cache could be larger than this area
	info->ram_size = cache->page_count * B_PAGE_SIZE;

	vm_area_put_locked_cache(cache);
}


static status_t
vm_resize_area(area_id areaID, size_t newSize, bool kernel)
{
	// is newSize a multiple of B_PAGE_SIZE?
	if (newSize & (B_PAGE_SIZE - 1))
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
		if (!kernel) {
			if ((area->protection & B_KERNEL_AREA) != 0)
				return B_NOT_ALLOWED;
			// TODO: Enforce all restrictions (team, etc.)!
		}

		oldSize = area->Size();
		if (newSize == oldSize)
			return B_OK;

		if (cache->type != CACHE_TYPE_RAM)
			return B_NOT_ALLOWED;

		if (oldSize < newSize) {
			// We need to check if all areas of this cache can be resized.
			for (VMArea* current = cache->areas; current != NULL;
					current = current->cache_next) {
				if (!current->address_space->CanResizeArea(current, newSize))
					return B_ERROR;
				anyKernelArea
					|= current->address_space == VMAddressSpace::Kernel();
			}
		} else {
			// We're shrinking the areas, so we must make sure the affected
			// ranges are not wired.
			for (VMArea* current = cache->areas; current != NULL;
					current = current->cache_next) {
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

	for (VMArea* current = cache->areas; current != NULL;
			current = current->cache_next) {
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
			uint32 bytes = (newSize / B_PAGE_SIZE + 1) / 2;
			uint8* newProtections
				= (uint8*)realloc(area->page_protections, bytes);
			if (newProtections == NULL)
				status = B_NO_MEMORY;
			else {
				area->page_protections = newProtections;

				if (oldSize < newSize) {
					// init the additional page protections to that of the area
					uint32 offset = (oldSize / B_PAGE_SIZE + 1) / 2;
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
		for (VMArea* current = cache->areas; current != NULL;
				current = current->cache_next) {
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
vm_memset_physical(phys_addr_t address, int value, size_t length)
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


/*!	Copies a range of memory directly from/to a page that might not be mapped
	at the moment.

	For \a unsafeMemory the current mapping (if any is ignored). The function
	walks through the respective area's cache chain to find the physical page
	and copies from/to it directly.
	The memory range starting at \a unsafeMemory with a length of \a size bytes
	must not cross a page boundary.

	\param teamID The team ID identifying the address space \a unsafeMemory is
		to be interpreted in. Ignored, if \a unsafeMemory is a kernel address
		(the kernel address space is assumed in this case). If \c B_CURRENT_TEAM
		is passed, the address space of the thread returned by
		debug_get_debugged_thread() is used.
	\param unsafeMemory The start of the unsafe memory range to be copied
		from/to.
	\param buffer A safely accessible kernel buffer to be copied from/to.
	\param size The number of bytes to be copied.
	\param copyToUnsafe If \c true, memory is copied from \a buffer to
		\a unsafeMemory, the other way around otherwise.
*/
status_t
vm_debug_copy_page_memory(team_id teamID, void* unsafeMemory, void* buffer,
	size_t size, bool copyToUnsafe)
{
	if (size > B_PAGE_SIZE || ROUNDDOWN((addr_t)unsafeMemory, B_PAGE_SIZE)
			!= ROUNDDOWN((addr_t)unsafeMemory + size - 1, B_PAGE_SIZE)) {
		return B_BAD_VALUE;
	}

	// get the address space for the debugged thread
	VMAddressSpace* addressSpace;
	if (IS_KERNEL_ADDRESS(unsafeMemory)) {
		addressSpace = VMAddressSpace::Kernel();
	} else if (teamID == B_CURRENT_TEAM) {
		Thread* thread = debug_get_debugged_thread();
		if (thread == NULL || thread->team == NULL)
			return B_BAD_ADDRESS;

		addressSpace = thread->team->address_space;
	} else
		addressSpace = VMAddressSpace::DebugGet(teamID);

	if (addressSpace == NULL)
		return B_BAD_ADDRESS;

	// get the area
	VMArea* area = addressSpace->LookupArea((addr_t)unsafeMemory);
	if (area == NULL)
		return B_BAD_ADDRESS;

	// search the page
	off_t cacheOffset = (addr_t)unsafeMemory - area->Base()
		+ area->cache_offset;
	VMCache* cache = area->cache;
	vm_page* page = NULL;
	while (cache != NULL) {
		page = cache->DebugLookupPage(cacheOffset);
		if (page != NULL)
			break;

		// Page not found in this cache -- if it is paged out, we must not try
		// to get it from lower caches.
		if (cache->DebugHasPage(cacheOffset))
			break;

		cache = cache->source;
	}

	if (page == NULL)
		return B_UNSUPPORTED;

	// copy from/to physical memory
	phys_addr_t physicalAddress = page->physical_page_number * B_PAGE_SIZE
		+ (addr_t)unsafeMemory % B_PAGE_SIZE;

	if (copyToUnsafe) {
		if (page->Cache() != area->cache)
			return B_UNSUPPORTED;

		return vm_memcpy_to_physical(physicalAddress, buffer, size, false);
	}

	return vm_memcpy_from_physical(buffer, physicalAddress, size, false);
}


//	#pragma mark - kernel public API


status_t
user_memcpy(void* to, const void* from, size_t size)
{
	// don't allow address overflows
	if ((addr_t)from + size < (addr_t)from || (addr_t)to + size < (addr_t)to)
		return B_BAD_ADDRESS;

	if (arch_cpu_user_memcpy(to, from, size,
			&thread_get_current_thread()->fault_handler) < B_OK)
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

	// limit size to avoid address overflows
	size_t maxSize = std::min(size,
		~(addr_t)0 - std::max((addr_t)from, (addr_t)to) + 1);
		// NOTE: Since arch_cpu_user_strlcpy() determines the length of \a from,
		// the source address might still overflow.

	ssize_t result = arch_cpu_user_strlcpy(to, from, maxSize,
		&thread_get_current_thread()->fault_handler);

	// If we hit the address overflow boundary, fail.
	if (result >= 0 && (size_t)result >= maxSize && maxSize < size)
		return B_BAD_ADDRESS;

	return result;
}


status_t
user_memset(void* s, char c, size_t count)
{
	// don't allow address overflows
	if ((addr_t)s + count < (addr_t)s)
		return B_BAD_ADDRESS;

	if (arch_cpu_user_memset(s, c, count,
			&thread_get_current_thread()->fault_handler) < B_OK)
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

		error = vm_soft_fault(addressSpace, pageAddress, writable, isUser,
			&page, &info->range);

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
					isUser, &page, range);

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
		unlock_memory_etc(team, (void*)address, nextAddress - lockBaseAddress,
			flags);
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

	AddressSpaceReadLocker addressSpaceLocker(addressSpace, true);

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

	// get rid of the address space reference
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

	TRACE(("get_memory_map_etc(%ld, %p, %lu bytes, %ld entries)\n", team,
		address, numBytes, numEntries));

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
	return VMAreaHash::Find(name);
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
_get_next_area_info(team_id team, int32* cookie, area_info* info, size_t size)
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

	VMArea* area;
	for (VMAddressSpace::AreaIterator it
				= locker.AddressSpace()->GetAreaIterator();
			(area = it.Next()) != NULL;) {
		if (area->Base() > nextBase)
			break;
	}

	if (area == NULL) {
		nextBase = (addr_t)-1;
		return B_ENTRY_NOT_FOUND;
	}

	fill_area_info(area, info, size);
	*cookie = (int32)(area->Base());
		// TODO: Not 64 bit safe!

	return B_OK;
}


status_t
set_area_protection(area_id area, uint32 newProtection)
{
	fix_protection(&newProtection);

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

	if (info.team != thread_get_current_thread()->team->id)
		return B_PERMISSION_DENIED;

	area_id clonedArea = vm_clone_area(target, info.name, _address,
		addressSpec, info.protection, REGION_NO_PRIVATE_MAP, id, kernel);
	if (clonedArea < 0)
		return clonedArea;

	status = vm_delete_area(info.team, id, kernel);
	if (status != B_OK) {
		vm_delete_area(target, clonedArea, kernel);
		return status;
	}

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
create_area_etc(team_id team, const char* name, uint32 size, uint32 lock,
	uint32 protection, uint32 flags,
	const virtual_address_restrictions* virtualAddressRestrictions,
	const physical_address_restrictions* physicalAddressRestrictions,
	void** _address)
{
	fix_protection(&protection);

	return vm_create_anonymous_area(team, name, size, lock, protection, flags,
		virtualAddressRestrictions, physicalAddressRestrictions, true,
		_address);
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
		lock, protection, 0, &virtualRestrictions, &physicalRestrictions, true,
		_address);
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

	// TODO: do we want to prevent userland from seeing kernel protections?
	//info.protection &= B_USER_PROTECTION;

	if (user_memcpy(userInfo, &info, sizeof(area_info)) < B_OK)
		return B_BAD_ADDRESS;

	return status;
}


status_t
_user_get_next_area_info(team_id team, int32* userCookie, area_info* userInfo)
{
	int32 cookie;

	if (!IS_USER_ADDRESS(userCookie)
		|| !IS_USER_ADDRESS(userInfo)
		|| user_memcpy(&cookie, userCookie, sizeof(int32)) < B_OK)
		return B_BAD_ADDRESS;

	area_info info;
	status_t status = _get_next_area_info(team, &cookie, &info,
		sizeof(area_info));
	if (status != B_OK)
		return status;

	//info.protection &= B_USER_PROTECTION;

	if (user_memcpy(userCookie, &cookie, sizeof(int32)) < B_OK
		|| user_memcpy(userInfo, &info, sizeof(area_info)) < B_OK)
		return B_BAD_ADDRESS;

	return status;
}


status_t
_user_set_area_protection(area_id area, uint32 newProtection)
{
	if ((newProtection & ~B_USER_PROTECTION) != 0)
		return B_BAD_VALUE;

	fix_protection(&newProtection);

	return vm_set_area_protection(VMAddressSpace::CurrentID(), area,
		newProtection, false);
}


status_t
_user_resize_area(area_id area, size_t newSize)
{
	// TODO: Since we restrict deleting of areas to those owned by the team,
	// we should also do that for resizing (check other functions, too).
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

	if (addressSpec == B_EXACT_ADDRESS
		&& IS_KERNEL_ADDRESS(address))
		return B_BAD_VALUE;

	fix_protection(&protection);

	virtual_address_restrictions virtualRestrictions = {};
	virtualRestrictions.address = address;
	virtualRestrictions.address_specification = addressSpec;
	physical_address_restrictions physicalRestrictions = {};
	area_id area = vm_create_anonymous_area(VMAddressSpace::CurrentID(), name,
		size, lock, protection, 0, &virtualRestrictions, &physicalRestrictions,
		false, &address);

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
				|| !IS_USER_ADDRESS((addr_t)address + size)) {
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

	if (!IS_USER_ADDRESS(address) || !IS_USER_ADDRESS((addr_t)address + size))
		return B_BAD_ADDRESS;

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
	if ((addr_t)address + size < (addr_t)address || !IS_USER_ADDRESS(address)
		|| !IS_USER_ADDRESS((addr_t)address + size)) {
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

			// TODO: For (shared) mapped files we should check whether the new
			// protections are compatible with the file permissions. We don't
			// have a way to do that yet, though.

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

			status_t status = allocate_area_page_protections(area);
			if (status != B_OK)
				return status;
		}

		// We need to lock the complete cache chain, since we potentially unmap
		// pages of lower caches.
		VMCache* topCache = vm_area_get_locked_cache(area);
		VMCacheChainLocker cacheChainLocker(topCache);
		cacheChainLocker.LockAllSourceCaches();

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
	if ((addr_t)address + size < (addr_t)address || !IS_USER_ADDRESS(address)
		|| !IS_USER_ADDRESS((addr_t)address + size)) {
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
_user_memory_advice(void* address, size_t size, uint32 advice)
{
	// TODO: Implement!
	return B_OK;
}


status_t
_user_get_memory_properties(team_id teamID, const void* address,
	uint32* _protected, uint32* _lock)
{
	if (!IS_USER_ADDRESS(_protected) || !IS_USER_ADDRESS(_lock))
		return B_BAD_ADDRESS;

	AddressSpaceReadLocker locker;
	status_t error = locker.SetTo(teamID);
	if (error != B_OK)
		return error;

	VMArea* area = locker.AddressSpace()->LookupArea((addr_t)address);
	if (area == NULL)
		return B_NO_MEMORY;


	uint32 protection = area->protection;
	if (area->page_protections != NULL)
		protection = get_area_page_protection(area, (addr_t)address);

	uint32 wiring = area->wiring;

	locker.Unlock();

	error = user_memcpy(_protected, &protection, sizeof(protection));
	if (error != B_OK)
		return error;

	error = user_memcpy(_lock, &wiring, sizeof(wiring));

	return error;
}


// #pragma mark -- compatibility


#if defined(__INTEL__) && B_HAIKU_PHYSICAL_BITS > 32


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


#endif	// defined(__INTEL__) && B_HAIKU_PHYSICAL_BITS > 32
