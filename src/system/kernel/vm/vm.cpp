/*
 * Copyright 2002-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <vm.h>

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <OS.h>
#include <KernelExport.h>

#include <AutoDeleter.h>

#include <fs/fd.h>
#include <vm_address_space.h>
#include <vm_priv.h>
#include <vm_page.h>
#include <vm_cache.h>
#include <vm_low_memory.h>
#include <file_cache.h>
#include <heap.h>
#include <condition_variable.h>
#include <debug.h>
#include <console.h>
#include <int.h>
#include <smp.h>
#include <lock.h>
#include <thread.h>
#include <team.h>
#include <util/AutoLock.h>
#include <util/khash.h>

#include <boot/stage2.h>
#include <boot/elf.h>

#include <arch/cpu.h>
#include <arch/vm.h>

#include "vm_store_anonymous_noswap.h"
#include "vm_store_device.h"
#include "vm_store_null.h"


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

#define ROUNDUP(a, b) (((a) + ((b)-1)) & ~((b)-1))
#define ROUNDOWN(a, b) (((a) / (b)) * (b))


class AddressSpaceReadLocker {
public:
	AddressSpaceReadLocker(team_id team);
	AddressSpaceReadLocker(vm_address_space* space);
	AddressSpaceReadLocker();
	~AddressSpaceReadLocker();

	status_t SetTo(team_id team);
	void SetTo(vm_address_space* space);
	status_t SetFromArea(area_id areaID, vm_area*& area);

	bool IsLocked() const { return fLocked; }
	void Unlock();

	void Unset();

	vm_address_space* AddressSpace() { return fSpace; }

private:
	vm_address_space* fSpace;
	bool	fLocked;
};

class AddressSpaceWriteLocker {
public:
	AddressSpaceWriteLocker(team_id team);
	AddressSpaceWriteLocker();
	~AddressSpaceWriteLocker();

	status_t SetTo(team_id team);
	status_t SetFromArea(area_id areaID, vm_area*& area);
	status_t SetFromArea(team_id team, area_id areaID, bool allowKernel,
		vm_area*& area);
	status_t SetFromArea(team_id team, area_id areaID, vm_area*& area);

	bool IsLocked() const { return fLocked; }
	void Unlock();

	void DegradeToReadLock();
	void Unset();

	vm_address_space* AddressSpace() { return fSpace; }

private:
	vm_address_space* fSpace;
	bool	fLocked;
	bool	fDegraded;
};

class MultiAddressSpaceLocker {
public:
	MultiAddressSpaceLocker();
	~MultiAddressSpaceLocker();

	inline status_t AddTeam(team_id team, bool writeLock,
		vm_address_space** _space = NULL);
	inline status_t AddArea(area_id area, bool writeLock,
		vm_address_space** _space = NULL);

	status_t AddAreaCacheAndLock(area_id areaID, bool writeLockThisOne,
		bool writeLockOthers, vm_area*& _area, vm_cache** _cache = NULL,
		bool checkNoCacheChange = false);

	status_t Lock();
	void Unlock();
	bool IsLocked() const { return fLocked; }

	void Unset();

private:
	struct lock_item {
		vm_address_space*	space;
		bool				write_lock;
	};

	bool _ResizeIfNeeded();
	int32 _IndexOfAddressSpace(vm_address_space* space) const;
	status_t _AddAddressSpace(vm_address_space* space, bool writeLock,
		vm_address_space** _space);

	static int _CompareItems(const void* _a, const void* _b);

	lock_item*	fItems;
	int32		fCapacity;
	int32		fCount;
	bool		fLocked;
};


class AreaCacheLocking {
public:
	inline bool Lock(vm_cache* lockable)
	{
		return false;
	}

	inline void Unlock(vm_cache* lockable)
	{
		vm_area_put_locked_cache(lockable);
	}
};

class AreaCacheLocker : public AutoLocker<vm_cache, AreaCacheLocking> {
public:
	inline AreaCacheLocker(vm_cache* cache = NULL)
		: AutoLocker<vm_cache, AreaCacheLocking>(cache, true)
	{
	}

	inline AreaCacheLocker(vm_area* area)
		: AutoLocker<vm_cache, AreaCacheLocking>()
	{
		SetTo(area);
	}

	inline void SetTo(vm_area* area)
	{
		return AutoLocker<vm_cache, AreaCacheLocking>::SetTo(
			area != NULL ? vm_area_get_locked_cache(area) : NULL, true, true);
	}
};


#define REGION_HASH_TABLE_SIZE 1024
static area_id sNextAreaID;
static hash_table *sAreaHash;
static sem_id sAreaHashLock;
static mutex sMappingLock;
static mutex sAreaCacheLock;

static off_t sAvailableMemory;
static benaphore sAvailableMemoryLock;

// function declarations
static void delete_area(vm_address_space *addressSpace, vm_area *area);
static vm_address_space *get_address_space_by_area_id(area_id id);
static status_t vm_soft_fault(addr_t address, bool isWrite, bool isUser);


//	#pragma mark -


AddressSpaceReadLocker::AddressSpaceReadLocker(team_id team)
	:
	fSpace(NULL),
	fLocked(false)
{
	SetTo(team);
}


//! Takes over the reference of the address space
AddressSpaceReadLocker::AddressSpaceReadLocker(vm_address_space* space)
	:
	fSpace(NULL),
	fLocked(false)
{
	SetTo(space);
}


AddressSpaceReadLocker::AddressSpaceReadLocker()
	:
	fSpace(NULL),
	fLocked(false)
{
}


AddressSpaceReadLocker::~AddressSpaceReadLocker()
{
	Unset();
}


void
AddressSpaceReadLocker::Unset()
{
	Unlock();
	if (fSpace != NULL)
		vm_put_address_space(fSpace);
}


status_t
AddressSpaceReadLocker::SetTo(team_id team)
{
	fSpace = vm_get_address_space_by_id(team);
	if (fSpace == NULL)
		return B_BAD_TEAM_ID;

	acquire_sem_etc(fSpace->sem, READ_COUNT, 0, 0);
	fLocked = true;
	return B_OK;
}


//! Takes over the reference of the address space
void
AddressSpaceReadLocker::SetTo(vm_address_space* space)
{
	fSpace = space;
	acquire_sem_etc(fSpace->sem, READ_COUNT, 0, 0);
	fLocked = true;
}


status_t
AddressSpaceReadLocker::SetFromArea(area_id areaID, vm_area*& area)
{
	fSpace = get_address_space_by_area_id(areaID);
	if (fSpace == NULL)
		return B_BAD_TEAM_ID;

	acquire_sem_etc(fSpace->sem, READ_COUNT, 0, 0);

	acquire_sem_etc(sAreaHashLock, READ_COUNT, 0, 0);
	area = (vm_area *)hash_lookup(sAreaHash, &areaID);
	release_sem_etc(sAreaHashLock, READ_COUNT, 0);

	if (area == NULL || area->address_space != fSpace) {
		release_sem_etc(fSpace->sem, READ_COUNT, 0);
		return B_BAD_VALUE;
	}

	fLocked = true;
	return B_OK;
}


void
AddressSpaceReadLocker::Unlock()
{
	if (fLocked) {
		release_sem_etc(fSpace->sem, READ_COUNT, 0);
		fLocked = false;
	}
}


//	#pragma mark -


AddressSpaceWriteLocker::AddressSpaceWriteLocker(team_id team)
	:
	fSpace(NULL),
	fLocked(false),
	fDegraded(false)
{
	SetTo(team);
}


AddressSpaceWriteLocker::AddressSpaceWriteLocker()
	:
	fSpace(NULL),
	fLocked(false),
	fDegraded(false)
{
}


AddressSpaceWriteLocker::~AddressSpaceWriteLocker()
{
	Unset();
}


void
AddressSpaceWriteLocker::Unset()
{
	Unlock();
	if (fSpace != NULL)
		vm_put_address_space(fSpace);
}


status_t
AddressSpaceWriteLocker::SetTo(team_id team)
{
	fSpace = vm_get_address_space_by_id(team);
	if (fSpace == NULL)
		return B_BAD_TEAM_ID;

	acquire_sem_etc(fSpace->sem, WRITE_COUNT, 0, 0);
	fLocked = true;
	return B_OK;
}


status_t
AddressSpaceWriteLocker::SetFromArea(area_id areaID, vm_area*& area)
{
	fSpace = get_address_space_by_area_id(areaID);
	if (fSpace == NULL)
		return B_BAD_VALUE;

	acquire_sem_etc(fSpace->sem, WRITE_COUNT, 0, 0);

	acquire_sem_etc(sAreaHashLock, READ_COUNT, 0, 0);
	area = (vm_area*)hash_lookup(sAreaHash, &areaID);
	release_sem_etc(sAreaHashLock, READ_COUNT, 0);

	if (area == NULL || area->address_space != fSpace) {
		release_sem_etc(fSpace->sem, WRITE_COUNT, 0);
		return B_BAD_VALUE;
	}

	fLocked = true;
	return B_OK;
}


status_t
AddressSpaceWriteLocker::SetFromArea(team_id team, area_id areaID,
	bool allowKernel, vm_area*& area)
{
	acquire_sem_etc(sAreaHashLock, READ_COUNT, 0, 0);

	area = (vm_area *)hash_lookup(sAreaHash, &areaID);
	if (area != NULL
		&& (area->address_space->id == team
			|| allowKernel && team == vm_kernel_address_space_id())) {
		fSpace = area->address_space;
		atomic_add(&fSpace->ref_count, 1);
	}

	release_sem_etc(sAreaHashLock, READ_COUNT, 0);

	if (fSpace == NULL)
		return B_BAD_VALUE;

	// Second try to get the area -- this time with the address space
	// write lock held

	acquire_sem_etc(fSpace->sem, WRITE_COUNT, 0, 0);

	acquire_sem_etc(sAreaHashLock, READ_COUNT, 0, 0);
	area = (vm_area *)hash_lookup(sAreaHash, &areaID);
	release_sem_etc(sAreaHashLock, READ_COUNT, 0);

	if (area == NULL) {
		release_sem_etc(fSpace->sem, WRITE_COUNT, 0);
		return B_BAD_VALUE;
	}

	fLocked = true;
	return B_OK;
}


status_t
AddressSpaceWriteLocker::SetFromArea(team_id team, area_id areaID,
	vm_area*& area)
{
	return SetFromArea(team, areaID, false, area);
}


void
AddressSpaceWriteLocker::Unlock()
{
	if (fLocked) {
		release_sem_etc(fSpace->sem, fDegraded ? READ_COUNT : WRITE_COUNT, 0);
		fLocked = false;
		fDegraded = false;
	}
}


void
AddressSpaceWriteLocker::DegradeToReadLock()
{
	release_sem_etc(fSpace->sem, WRITE_COUNT - READ_COUNT, 0);
	fDegraded = true;
}


//	#pragma mark -


MultiAddressSpaceLocker::MultiAddressSpaceLocker()
	:
	fItems(NULL),
	fCapacity(0),
	fCount(0),
	fLocked(false)
{
}


MultiAddressSpaceLocker::~MultiAddressSpaceLocker()
{
	Unset();
	free(fItems);
}


/*static*/ int
MultiAddressSpaceLocker::_CompareItems(const void* _a, const void* _b)
{
	lock_item* a = (lock_item*)_a;
	lock_item* b = (lock_item*)_b;
	return a->space->id - b->space->id;
}


bool
MultiAddressSpaceLocker::_ResizeIfNeeded()
{
	if (fCount == fCapacity) {
		lock_item* items = (lock_item*)realloc(fItems,
			(fCapacity + 4) * sizeof(lock_item));
		if (items == NULL)
			return false;

		fCapacity += 4;
		fItems = items;
	}

	return true;
}


int32
MultiAddressSpaceLocker::_IndexOfAddressSpace(vm_address_space* space) const
{
	for (int32 i = 0; i < fCount; i++) {
		if (fItems[i].space == space)
			return i;
	}

	return -1;
}


status_t
MultiAddressSpaceLocker::_AddAddressSpace(vm_address_space* space,
	bool writeLock, vm_address_space** _space)
{
	if (!space)
		return B_BAD_VALUE;

	int32 index = _IndexOfAddressSpace(space);
	if (index < 0) {
		if (!_ResizeIfNeeded()) {
			vm_put_address_space(space);
			return B_NO_MEMORY;
		}

		lock_item& item = fItems[fCount++];
		item.space = space;
		item.write_lock = writeLock;
	} else {

		// one reference is enough
		vm_put_address_space(space);

		fItems[index].write_lock |= writeLock;
	}

	if (_space != NULL)
		*_space = space;

	return B_OK;
}


inline status_t
MultiAddressSpaceLocker::AddTeam(team_id team, bool writeLock,
	vm_address_space** _space)
{
	return _AddAddressSpace(vm_get_address_space_by_id(team), writeLock,
		_space);
}


inline status_t
MultiAddressSpaceLocker::AddArea(area_id area, bool writeLock,
	vm_address_space** _space)
{
	return _AddAddressSpace(get_address_space_by_area_id(area), writeLock,
		_space);
}


void
MultiAddressSpaceLocker::Unset()
{
	Unlock();

	for (int32 i = 0; i < fCount; i++)
		vm_put_address_space(fItems[i].space);

	fCount = 0;
}


status_t
MultiAddressSpaceLocker::Lock()
{
	ASSERT(!fLocked);

	qsort(fItems, fCount, sizeof(lock_item), &_CompareItems);

	for (int32 i = 0; i < fCount; i++) {
		status_t status = acquire_sem_etc(fItems[i].space->sem,
			fItems[i].write_lock ? WRITE_COUNT : READ_COUNT, 0, 0);
		if (status < B_OK) {
			while (--i >= 0) {
				release_sem_etc(fItems[i].space->sem,
					fItems[i].write_lock ? WRITE_COUNT : READ_COUNT, 0);
			}
			return status;
		}
	}

	fLocked = true;
	return B_OK;
}


void
MultiAddressSpaceLocker::Unlock()
{
	if (!fLocked)
		return;

	for (int32 i = 0; i < fCount; i++) {
		release_sem_etc(fItems[i].space->sem,
			fItems[i].write_lock ? WRITE_COUNT : READ_COUNT, 0);
	}

	fLocked = false;
}


/*!	Adds all address spaces of the areas associated with the given area's cache,
	locks them, and locks the cache (including a reference to it). It retries
	until the situation is stable (i.e. the neither cache nor cache's areas
	changed) or an error occurs. If \c checkNoCacheChange ist \c true it does
	not return until all areas' \c no_cache_change flags is clear.
*/
status_t
MultiAddressSpaceLocker::AddAreaCacheAndLock(area_id areaID,
	bool writeLockThisOne, bool writeLockOthers, vm_area*& _area,
	vm_cache** _cache, bool checkNoCacheChange)
{
	// remember the original state
	int originalCount = fCount;
	lock_item* originalItems = NULL;
	if (fCount > 0) {
		originalItems = new(nothrow) lock_item[fCount];
		if (originalItems == NULL)
			return B_NO_MEMORY;
		memcpy(originalItems, fItems, fCount * sizeof(lock_item));
	}
	ArrayDeleter<lock_item> _(originalItems);

	// get the cache
	vm_cache* cache;
	vm_area* area;
	status_t error;
	{
		AddressSpaceReadLocker locker;
		error = locker.SetFromArea(areaID, area);
		if (error != B_OK)
			return error;

		cache = vm_area_get_locked_cache(area);
	}

	while (true) {
		// add all areas
		vm_area* firstArea = cache->areas;
		for (vm_area* current = firstArea; current;
				current = current->cache_next) {
			error = AddArea(current->id,
				current == area ? writeLockThisOne : writeLockOthers);
			if (error != B_OK) {
				vm_area_put_locked_cache(cache);
				return error;
			}
		}

		// unlock the cache and attempt to lock the address spaces
		vm_area_put_locked_cache(cache);

		error = Lock();
		if (error != B_OK)
			return error;

		// lock the cache again and check whether anything has changed

		// check whether the area is gone in the meantime
		acquire_sem_etc(sAreaHashLock, READ_COUNT, 0, 0);
		area = (vm_area *)hash_lookup(sAreaHash, &areaID);
		release_sem_etc(sAreaHashLock, READ_COUNT, 0);

		if (area == NULL) {
			Unlock();
			return B_BAD_VALUE;
		}

		// lock the cache
		vm_cache* oldCache = cache;
		cache = vm_area_get_locked_cache(area);

		// If neither the area's cache has changed nor its area list we're
		// done...
		bool done = (cache == oldCache || firstArea == cache->areas);

		// ... unless we're supposed to check the areas' "no_cache_change" flag
		bool yield = false;
		if (done && checkNoCacheChange) {
			for (vm_area *tempArea = cache->areas; tempArea != NULL;
					tempArea = tempArea->cache_next) {
				if (tempArea->no_cache_change) {
					done = false;
					yield = true;
					break;
				}
			}
		}

		// If everything looks dandy, return the values.
		if (done) {
			_area = area;
			if (_cache != NULL)
				*_cache = cache;
			return B_OK;
		}

		// Restore the original state and try again.

		// Unlock the address spaces, but keep the cache locked for the next
		// iteration.
		Unlock();

		// Get an additional reference to the original address spaces.
		for (int32 i = 0; i < originalCount; i++)
			atomic_add(&originalItems[i].space->ref_count, 1);

		// Release all references to the current address spaces.
		for (int32 i = 0; i < fCount; i++)
			vm_put_address_space(fItems[i].space);

		// Copy over the original state.
		fCount = originalCount;
		if (originalItems != NULL)
			memcpy(fItems, originalItems, fCount * sizeof(lock_item));

		if (yield)
			thread_yield(true);
	}
}


//	#pragma mark -


static int
area_compare(void *_area, const void *key)
{
	vm_area *area = (vm_area *)_area;
	const area_id *id = (const area_id *)key;

	if (area->id == *id)
		return 0;

	return -1;
}


static uint32
area_hash(void *_area, const void *key, uint32 range)
{
	vm_area *area = (vm_area *)_area;
	const area_id *id = (const area_id *)key;

	if (area != NULL)
		return area->id % range;

	return (uint32)*id % range;
}


static vm_address_space *
get_address_space_by_area_id(area_id id)
{
	vm_address_space* addressSpace = NULL;

	acquire_sem_etc(sAreaHashLock, READ_COUNT, 0, 0);

	vm_area *area = (vm_area *)hash_lookup(sAreaHash, &id);
	if (area != NULL) {
		addressSpace = area->address_space;
		atomic_add(&addressSpace->ref_count, 1);
	}

	release_sem_etc(sAreaHashLock, READ_COUNT, 0);

	return addressSpace;
}


//! You need to have the address space locked when calling this function
static vm_area *
lookup_area(vm_address_space* addressSpace, area_id id)
{
	acquire_sem_etc(sAreaHashLock, READ_COUNT, 0, 0);

	vm_area *area = (vm_area *)hash_lookup(sAreaHash, &id);
	if (area != NULL && area->address_space != addressSpace)
		area = NULL;

	release_sem_etc(sAreaHashLock, READ_COUNT, 0);

	return area;
}


static vm_area *
create_reserved_area_struct(vm_address_space *addressSpace, uint32 flags)
{
	vm_area *reserved = (vm_area *)malloc(sizeof(vm_area));
	if (reserved == NULL)
		return NULL;

	memset(reserved, 0, sizeof(vm_area));
	reserved->id = RESERVED_AREA_ID;
		// this marks it as reserved space
	reserved->protection = flags;
	reserved->address_space = addressSpace;

	return reserved;
}


static vm_area *
create_area_struct(vm_address_space *addressSpace, const char *name,
	uint32 wiring, uint32 protection)
{
	// restrict the area name to B_OS_NAME_LENGTH
	size_t length = strlen(name) + 1;
	if (length > B_OS_NAME_LENGTH)
		length = B_OS_NAME_LENGTH;

	vm_area *area = (vm_area *)malloc(sizeof(vm_area));
	if (area == NULL)
		return NULL;

	area->name = (char *)malloc(length);
	if (area->name == NULL) {
		free(area);
		return NULL;
	}
	strlcpy(area->name, name, length);

	area->id = atomic_add(&sNextAreaID, 1);
	area->base = 0;
	area->size = 0;
	area->protection = protection;
	area->wiring = wiring;
	area->memory_type = 0;

	area->cache = NULL;
	area->no_cache_change = 0;
	area->cache_offset = 0;

	area->address_space = addressSpace;
	area->address_space_next = NULL;
	area->cache_next = area->cache_prev = NULL;
	area->hash_next = NULL;
	new (&area->mappings) vm_area_mappings;

	return area;
}


/**	Finds a reserved area that covers the region spanned by \a start and
 *	\a size, inserts the \a area into that region and makes sure that
 *	there are reserved regions for the remaining parts.
 */

static status_t
find_reserved_area(vm_address_space *addressSpace, addr_t start,
	addr_t size, vm_area *area)
{
	vm_area *next, *last = NULL;

	next = addressSpace->areas;
	while (next) {
		if (next->base <= start && next->base + next->size >= start + size) {
			// this area covers the requested range
			if (next->id != RESERVED_AREA_ID) {
				// but it's not reserved space, it's a real area
				return B_BAD_VALUE;
			}

			break;
		}
		last = next;
		next = next->address_space_next;
	}
	if (next == NULL)
		return B_ENTRY_NOT_FOUND;

	// now we have to transfer the requested part of the reserved
	// range to the new area - and remove, resize or split the old
	// reserved area.

	if (start == next->base) {
		// the area starts at the beginning of the reserved range
		if (last)
			last->address_space_next = area;
		else
			addressSpace->areas = area;

		if (size == next->size) {
			// the new area fully covers the reversed range
			area->address_space_next = next->address_space_next;
			vm_put_address_space(addressSpace);
			free(next);
		} else {
			// resize the reserved range behind the area
			area->address_space_next = next;
			next->base += size;
			next->size -= size;
		}
	} else if (start + size == next->base + next->size) {
		// the area is at the end of the reserved range
		area->address_space_next = next->address_space_next;
		next->address_space_next = area;

		// resize the reserved range before the area
		next->size = start - next->base;
	} else {
		// the area splits the reserved range into two separate ones
		// we need a new reserved area to cover this space
		vm_area *reserved = create_reserved_area_struct(addressSpace,
			next->protection);
		if (reserved == NULL)
			return B_NO_MEMORY;

		atomic_add(&addressSpace->ref_count, 1);
		reserved->address_space_next = next->address_space_next;
		area->address_space_next = reserved;
		next->address_space_next = area;

		// resize regions
		reserved->size = next->base + next->size - start - size;
		next->size = start - next->base;
		reserved->base = start + size;
		reserved->cache_offset = next->cache_offset;
	}

	area->base = start;
	area->size = size;
	addressSpace->change_count++;

	return B_OK;
}


/*!	Must be called with this address space's sem held */
static status_t
find_and_insert_area_slot(vm_address_space *addressSpace, addr_t start,
	addr_t size, addr_t end, uint32 addressSpec, vm_area *area)
{
	vm_area *last = NULL;
	vm_area *next;
	bool foundSpot = false;

	TRACE(("find_and_insert_area_slot: address space %p, start 0x%lx, "
		"size %ld, end 0x%lx, addressSpec %ld, area %p\n", addressSpace, start,
		size, end, addressSpec, area));

	// do some sanity checking
	if (start < addressSpace->base || size == 0
		|| (end - 1) > (addressSpace->base + (addressSpace->size - 1))
		|| start + size > end)
		return B_BAD_ADDRESS;

	if (addressSpec == B_EXACT_ADDRESS) {
		// search for a reserved area
		status_t status = find_reserved_area(addressSpace, start, size, area);
		if (status == B_OK || status == B_BAD_VALUE)
			return status;

		// There was no reserved area, and the slot doesn't seem to be used
		// already
		// ToDo: this could be further optimized.
	}

	size_t alignment = B_PAGE_SIZE;
	if (addressSpec == B_ANY_KERNEL_BLOCK_ADDRESS) {
		// align the memory to the next power of two of the size
		while (alignment < size)
			alignment <<= 1;
	}

	start = ROUNDUP(start, alignment);

	// walk up to the spot where we should start searching
second_chance:
	next = addressSpace->areas;
	while (next) {
		if (next->base >= start + size) {
			// we have a winner
			break;
		}
		last = next;
		next = next->address_space_next;
	}

	// find the right spot depending on the address specification - the area
	// will be inserted directly after "last" ("next" is not referenced anymore)

	switch (addressSpec) {
		case B_ANY_ADDRESS:
		case B_ANY_KERNEL_ADDRESS:
		case B_ANY_KERNEL_BLOCK_ADDRESS:
			// find a hole big enough for a new area
			if (!last) {
				// see if we can build it at the beginning of the virtual map
				if (!next || (next->base >= ROUNDUP(addressSpace->base,
						alignment) + size)) {
					foundSpot = true;
					area->base = ROUNDUP(addressSpace->base, alignment);
					break;
				}
				last = next;
				next = next->address_space_next;
			}
			// keep walking
			while (next) {
				if (next->base >= ROUNDUP(last->base + last->size, alignment)
						+ size) {
					// we found a spot (it'll be filled up below)
					break;
				}
				last = next;
				next = next->address_space_next;
			}

			if ((addressSpace->base + (addressSpace->size - 1)) >= (ROUNDUP(
					last->base + last->size, alignment) + (size - 1))) {
				// got a spot
				foundSpot = true;
				area->base = ROUNDUP(last->base + last->size, alignment);
				break;
			} else {
				// We didn't find a free spot - if there were any reserved areas
				// with the RESERVED_AVOID_BASE flag set, we can now test those
				// for free space
				// ToDo: it would make sense to start with the biggest of them
				next = addressSpace->areas;
				last = NULL;
				for (last = NULL; next; next = next->address_space_next,
						last = next) {
					// ToDo: take free space after the reserved area into account!
					if (next->base == ROUNDUP(next->base, alignment)
						&& next->size == size) {
						// The reserved area is entirely covered, and thus,
						// removed
						if (last)
							last->address_space_next = next->address_space_next;
						else
							addressSpace->areas = next->address_space_next;

						foundSpot = true;
						area->base = next->base;
						free(next);
						break;
					}
					if (next->size - (ROUNDUP(next->base, alignment)
							- next->base) >= size) {
						// The new area will be placed at the end of the
						// reserved area, and the reserved area will be resized
						// to make space
						foundSpot = true;
						next->size -= size;
						last = next;
						area->base = next->base + next->size;
						break;
					}
				}
			}
			break;

		case B_BASE_ADDRESS:
			// find a hole big enough for a new area beginning with "start"
			if (!last) {
				// see if we can build it at the beginning of the specified start
				if (!next || (next->base >= start + size)) {
					foundSpot = true;
					area->base = start;
					break;
				}
				last = next;
				next = next->address_space_next;
			}
			// keep walking
			while (next) {
				if (next->base >= last->base + last->size + size) {
					// we found a spot (it'll be filled up below)
					break;
				}
				last = next;
				next = next->address_space_next;
			}

			if ((addressSpace->base + (addressSpace->size - 1))
					>= (last->base + last->size + (size - 1))) {
				// got a spot
				foundSpot = true;
				if (last->base + last->size <= start)
					area->base = start;
				else
					area->base = last->base + last->size;
				break;
			}
			// we didn't find a free spot in the requested range, so we'll
			// try again without any restrictions
			start = addressSpace->base;
			addressSpec = B_ANY_ADDRESS;
			last = NULL;
			goto second_chance;

		case B_EXACT_ADDRESS:
			// see if we can create it exactly here
			if (!last) {
				if (!next || (next->base >= start + size)) {
					foundSpot = true;
					area->base = start;
					break;
				}
			} else {
				if (next) {
					if (last->base + last->size <= start && next->base >= start + size) {
						foundSpot = true;
						area->base = start;
						break;
					}
				} else {
					if ((last->base + (last->size - 1)) <= start - 1) {
						foundSpot = true;
						area->base = start;
					}
				}
			}
			break;
		default:
			return B_BAD_VALUE;
	}

	if (!foundSpot)
		return addressSpec == B_EXACT_ADDRESS ? B_BAD_VALUE : B_NO_MEMORY;

	area->size = size;
	if (last) {
		area->address_space_next = last->address_space_next;
		last->address_space_next = area;
	} else {
		area->address_space_next = addressSpace->areas;
		addressSpace->areas = area;
	}
	addressSpace->change_count++;
	return B_OK;
}


/**	This inserts the area you pass into the specified address space.
 *	It will also set the "_address" argument to its base address when
 *	the call succeeds.
 *	You need to hold the vm_address_space semaphore.
 */

static status_t
insert_area(vm_address_space *addressSpace, void **_address,
	uint32 addressSpec, addr_t size, vm_area *area)
{
	addr_t searchBase, searchEnd;
	status_t status;

	switch (addressSpec) {
		case B_EXACT_ADDRESS:
			searchBase = (addr_t)*_address;
			searchEnd = (addr_t)*_address + size;
			break;

		case B_BASE_ADDRESS:
			searchBase = (addr_t)*_address;
			searchEnd = addressSpace->base + (addressSpace->size - 1);
			break;

		case B_ANY_ADDRESS:
		case B_ANY_KERNEL_ADDRESS:
		case B_ANY_KERNEL_BLOCK_ADDRESS:
			searchBase = addressSpace->base;
			searchEnd = addressSpace->base + (addressSpace->size - 1);
			break;

		default:
			return B_BAD_VALUE;
	}

	status = find_and_insert_area_slot(addressSpace, searchBase, size,
				searchEnd, addressSpec, area);
	if (status == B_OK) {
		// ToDo: do we have to do anything about B_ANY_KERNEL_ADDRESS
		//		vs. B_ANY_KERNEL_BLOCK_ADDRESS here?
		*_address = (void *)area->base;
	}

	return status;
}


/*!	Deletes all areas in the given address range.
	The address space must be write-locked.
	NOTE: At the moment deleting only complete areas is supported.
*/
static status_t
unmap_address_range(vm_address_space *addressSpace, addr_t address, addr_t size)
{
	// TODO: Support deleting partial areas!

	addr_t lastAddress = address + (size - 1);

	// check whether any areas are only partially covered
	vm_area* area = vm_area_lookup(addressSpace, address);
	if (area != NULL && area->base < address)
		return B_UNSUPPORTED;

	area = vm_area_lookup(addressSpace, lastAddress);
	if (area != NULL && lastAddress - area->base < area->size - 1)
		return B_UNSUPPORTED;

	// all areas (if any) are fully covered; let's delete them
	area = addressSpace->areas;
	while (area != NULL) {
		vm_area* nextArea = area->address_space_next;

		if (area->id != RESERVED_AREA_ID) {
			if (area->base >= address && area->base < lastAddress)
				delete_area(addressSpace, area);
		}

		area = nextArea;
	}

	return B_OK;
}


/*! You need to hold the lock of the cache and the write lock of the address
	space when calling this function.
	Note, that in case of error your cache will be temporarily unlocked.
*/
static status_t
map_backing_store(vm_address_space *addressSpace, vm_cache *cache,
	void **_virtualAddress, off_t offset, addr_t size, uint32 addressSpec,
	int wiring, int protection, int mapping, vm_area **_area,
	const char *areaName, bool unmapAddressRange)
{
	TRACE(("map_backing_store: aspace %p, cache %p, *vaddr %p, offset 0x%Lx, size %lu, addressSpec %ld, wiring %d, protection %d, _area %p, area_name '%s'\n",
		addressSpace, cache, *_virtualAddress, offset, size, addressSpec,
		wiring, protection, _area, areaName));
	ASSERT_LOCKED_MUTEX(&cache->lock);

	vm_area *area = create_area_struct(addressSpace, areaName, wiring,
		protection);
	if (area == NULL)
		return B_NO_MEMORY;

	vm_store *store = cache->store;
	status_t status;

	// if this is a private map, we need to create a new cache & store object
	// pair to handle the private copies of pages as they are written to
	vm_cache* sourceCache = cache;
	if (mapping == REGION_PRIVATE_MAP) {
		vm_cache *newCache;
		vm_store *newStore;

		// create an anonymous store object
		newStore = vm_store_create_anonymous_noswap(
			(protection & B_STACK_AREA) != 0, 0, USER_STACK_GUARD_PAGES);
		if (newStore == NULL) {
			status = B_NO_MEMORY;
			goto err1;
		}
		newCache = vm_cache_create(newStore);
		if (newCache == NULL) {
			status = B_NO_MEMORY;
			newStore->ops->destroy(newStore);
			goto err1;
		}

		mutex_lock(&newCache->lock);
		newCache->type = CACHE_TYPE_RAM;
		newCache->temporary = 1;
		newCache->scan_skip = cache->scan_skip;
		newCache->virtual_base = offset;
		newCache->virtual_size = offset + size;

		vm_cache_add_consumer_locked(cache, newCache);

		cache = newCache;
		store = newStore;
	}

	status = vm_cache_set_minimal_commitment_locked(cache, offset + size);
	if (status != B_OK)
		goto err2;

	// check to see if this address space has entered DELETE state
	if (addressSpace->state == VM_ASPACE_STATE_DELETION) {
		// okay, someone is trying to delete this address space now, so we can't
		// insert the area, so back out
		status = B_BAD_TEAM_ID;
		goto err2;
	}

	if (addressSpec == B_EXACT_ADDRESS && unmapAddressRange) {
		status = unmap_address_range(addressSpace, (addr_t)*_virtualAddress,
			size);
		if (status != B_OK)
			goto err2;
	}

	status = insert_area(addressSpace, _virtualAddress, addressSpec, size, area);
	if (status < B_OK)
		goto err2;

	// attach the cache to the area
	area->cache = cache;
	area->cache_offset = offset;

	// point the cache back to the area
	vm_cache_insert_area_locked(cache, area);
	if (mapping == REGION_PRIVATE_MAP)
		mutex_unlock(&cache->lock);

	// insert the area in the global area hash table
	acquire_sem_etc(sAreaHashLock, WRITE_COUNT, 0 ,0);
	hash_insert(sAreaHash, area);
	release_sem_etc(sAreaHashLock, WRITE_COUNT, 0);

	// grab a ref to the address space (the area holds this)
	atomic_add(&addressSpace->ref_count, 1);

	*_area = area;
	return B_OK;

err2:
	if (mapping == REGION_PRIVATE_MAP) {
		// We created this cache, so we must delete it again. Note, that we
		// need to temporarily unlock the source cache or we'll otherwise
		// deadlock, since vm_cache_remove_consumer will try to lock it too.
		mutex_unlock(&cache->lock);
		mutex_unlock(&sourceCache->lock);
		vm_cache_release_ref(cache);
		mutex_lock(&sourceCache->lock);
	}
err1:
	free(area->name);
	free(area);
	return status;
}


status_t
vm_unreserve_address_range(team_id team, void *address, addr_t size)
{
	AddressSpaceWriteLocker locker(team);
	if (!locker.IsLocked())
		return B_BAD_TEAM_ID;

	// check to see if this address space has entered DELETE state
	if (locker.AddressSpace()->state == VM_ASPACE_STATE_DELETION) {
		// okay, someone is trying to delete this address space now, so we can't
		// insert the area, so back out
		return B_BAD_TEAM_ID;
	}

	// search area list and remove any matching reserved ranges

	vm_area* area = locker.AddressSpace()->areas;
	vm_area* last = NULL;
	while (area) {
		// the area must be completely part of the reserved range
		if (area->id == RESERVED_AREA_ID && area->base >= (addr_t)address
			&& area->base + area->size <= (addr_t)address + size) {
			// remove reserved range
			vm_area *reserved = area;
			if (last)
				last->address_space_next = reserved->address_space_next;
			else
				locker.AddressSpace()->areas = reserved->address_space_next;

			area = reserved->address_space_next;
			vm_put_address_space(locker.AddressSpace());
			free(reserved);
			continue;
		}

		last = area;
		area = area->address_space_next;
	}

	return B_OK;
}


status_t
vm_reserve_address_range(team_id team, void **_address, uint32 addressSpec,
	addr_t size, uint32 flags)
{
	if (size == 0)
		return B_BAD_VALUE;

	AddressSpaceWriteLocker locker(team);
	if (!locker.IsLocked())
		return B_BAD_TEAM_ID;

	// check to see if this address space has entered DELETE state
	if (locker.AddressSpace()->state == VM_ASPACE_STATE_DELETION) {
		// okay, someone is trying to delete this address space now, so we
		// can't insert the area, let's back out
		return B_BAD_TEAM_ID;
	}

	vm_area *area = create_reserved_area_struct(locker.AddressSpace(), flags);
	if (area == NULL)
		return B_NO_MEMORY;

	status_t status = insert_area(locker.AddressSpace(), _address, addressSpec,
		size, area);
	if (status < B_OK) {
		free(area);
		return status;
	}

	// the area is now reserved!

	area->cache_offset = area->base;
		// we cache the original base address here

	atomic_add(&locker.AddressSpace()->ref_count, 1);
	return B_OK;
}


area_id
vm_create_anonymous_area(team_id team, const char *name, void **address,
	uint32 addressSpec, addr_t size, uint32 wiring, uint32 protection,
	bool unmapAddressRange)
{
	vm_area *area;
	vm_cache *cache;
	vm_store *store;
	vm_page *page = NULL;
	bool isStack = (protection & B_STACK_AREA) != 0;
	bool canOvercommit = false;

	TRACE(("create_anonymous_area %s: size 0x%lx\n", name, size));

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

	/* check parameters */
	switch (addressSpec) {
		case B_ANY_ADDRESS:
		case B_EXACT_ADDRESS:
		case B_BASE_ADDRESS:
		case B_ANY_KERNEL_ADDRESS:
		case B_ANY_KERNEL_BLOCK_ADDRESS:
			break;

		default:
			return B_BAD_VALUE;
	}

	switch (wiring) {
		case B_NO_LOCK:
		case B_FULL_LOCK:
		case B_LAZY_LOCK:
		case B_CONTIGUOUS:
		case B_ALREADY_WIRED:
			break;
		case B_LOMEM:
		//case B_SLOWMEM:
			dprintf("B_LOMEM/SLOWMEM is not yet supported!\n");
			wiring = B_FULL_LOCK;
			break;
		default:
			return B_BAD_VALUE;
	}

	AddressSpaceWriteLocker locker;
	status_t status = locker.SetTo(team);
	if (status != B_OK)
		return status;

	vm_address_space *addressSpace = locker.AddressSpace();
	size = PAGE_ALIGN(size);

	if (wiring == B_CONTIGUOUS) {
		// we try to allocate the page run here upfront as this may easily
		// fail for obvious reasons
		page = vm_page_allocate_page_run(PAGE_STATE_CLEAR, size / B_PAGE_SIZE);
		if (page == NULL)
			return B_NO_MEMORY;
	}

	// create an anonymous store object
	// if it's a stack, make sure that two pages are available at least
	store = vm_store_create_anonymous_noswap(canOvercommit, isStack ? 2 : 0,
		isStack ? ((protection & B_USER_PROTECTION) != 0 ?
			USER_STACK_GUARD_PAGES : KERNEL_STACK_GUARD_PAGES) : 0);
	if (store == NULL) {
		status = B_NO_MEMORY;
		goto err1;
	}
	cache = vm_cache_create(store);
	if (cache == NULL) {
		status = B_NO_MEMORY;
		goto err2;
	}

	cache->temporary = 1;
	cache->type = CACHE_TYPE_RAM;
	cache->virtual_size = size;

	switch (wiring) {
		case B_LAZY_LOCK:
		case B_FULL_LOCK:
		case B_CONTIGUOUS:
		case B_ALREADY_WIRED:
			cache->scan_skip = 1;
			break;
		case B_NO_LOCK:
			cache->scan_skip = 0;
			break;
	}

	mutex_lock(&cache->lock);

	status = map_backing_store(addressSpace, cache, address, 0, size,
		addressSpec, wiring, protection, REGION_NO_PRIVATE_MAP, &area, name,
		unmapAddressRange);

	mutex_unlock(&cache->lock);

	if (status < B_OK) {
		vm_cache_release_ref(cache);
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
			vm_translation_map *map = &addressSpace->translation_map;
			size_t reservePages = map->ops->map_max_pages_need(map,
				area->base, area->base + (area->size - 1));
			vm_page_reserve_pages(reservePages);

			// Allocate and map all pages for this area
			mutex_lock(&cache->lock);

			off_t offset = 0;
			for (addr_t address = area->base; address < area->base + (area->size - 1);
					address += B_PAGE_SIZE, offset += B_PAGE_SIZE) {
#ifdef DEBUG_KERNEL_STACKS
#	ifdef STACK_GROWS_DOWNWARDS
				if (isStack && address < area->base + KERNEL_STACK_GUARD_PAGES
						* B_PAGE_SIZE)
#	else
				if (isStack && address >= area->base + area->size
						- KERNEL_STACK_GUARD_PAGES * B_PAGE_SIZE)
#	endif
					continue;
#endif
				vm_page *page = vm_page_allocate_page(PAGE_STATE_CLEAR, false);
				if (page == NULL) {
					// this shouldn't really happen, as we reserve the memory upfront
					panic("couldn't fulfill B_FULL lock!");
				}

				vm_cache_insert_page(cache, page, offset);
				vm_map_page(area, page, address, protection);
			}

			mutex_unlock(&cache->lock);
			vm_page_unreserve_pages(reservePages);
			break;
		}

		case B_ALREADY_WIRED:
		{
			// the pages should already be mapped. This is only really useful during
			// boot time. Find the appropriate vm_page objects and stick them in
			// the cache object.
			vm_translation_map *map = &addressSpace->translation_map;
			off_t offset = 0;

			if (!kernel_startup)
				panic("ALREADY_WIRED flag used outside kernel startup\n");

			mutex_lock(&cache->lock);
			map->ops->lock(map);

			for (addr_t virtualAddress = area->base; virtualAddress < area->base
					+ (area->size - 1); virtualAddress += B_PAGE_SIZE,
					offset += B_PAGE_SIZE) {
				addr_t physicalAddress;
				uint32 flags;
				status = map->ops->query(map, virtualAddress,
					&physicalAddress, &flags);
				if (status < B_OK) {
					panic("looking up mapping failed for va 0x%lx\n",
						virtualAddress);
				}
				page = vm_lookup_page(physicalAddress / B_PAGE_SIZE);
				if (page == NULL) {
					panic("looking up page failed for pa 0x%lx\n",
						physicalAddress);
				}

				page->wired_count++;
					// TODO: needs to be atomic on all platforms!
				vm_page_set_state(page, PAGE_STATE_WIRED);
				vm_cache_insert_page(cache, page, offset);
			}

			map->ops->unlock(map);
			mutex_unlock(&cache->lock);
			break;
		}

		case B_CONTIGUOUS:
		{
			// We have already allocated our continuous pages run, so we can now just
			// map them in the address space
			vm_translation_map *map = &addressSpace->translation_map;
			addr_t physicalAddress = page->physical_page_number * B_PAGE_SIZE;
			addr_t virtualAddress = area->base;
			size_t reservePages = map->ops->map_max_pages_need(map,
				virtualAddress, virtualAddress + (area->size - 1));
			off_t offset = 0;

			vm_page_reserve_pages(reservePages);
			mutex_lock(&cache->lock);
			map->ops->lock(map);

			for (virtualAddress = area->base; virtualAddress < area->base
					+ (area->size - 1); virtualAddress += B_PAGE_SIZE,
					offset += B_PAGE_SIZE, physicalAddress += B_PAGE_SIZE) {
				page = vm_lookup_page(physicalAddress / B_PAGE_SIZE);
				if (page == NULL)
					panic("couldn't lookup physical page just allocated\n");

				status = map->ops->map(map, virtualAddress, physicalAddress,
					protection);
				if (status < B_OK)
					panic("couldn't map physical page in page run\n");

				page->wired_count++;
					// TODO: needs to be atomic on all platforms!
				vm_page_set_state(page, PAGE_STATE_WIRED);
				vm_cache_insert_page(cache, page, offset);
			}

			map->ops->unlock(map);
			mutex_unlock(&cache->lock);
			vm_page_unreserve_pages(reservePages);
			break;
		}

		default:
			break;
	}

	TRACE(("vm_create_anonymous_area: done\n"));

	area->cache_type = CACHE_TYPE_RAM;
	return area->id;

err2:
	store->ops->destroy(store);
err1:
	if (wiring == B_CONTIGUOUS) {
		// we had reserved the area space upfront...
		addr_t pageNumber = page->physical_page_number;
		int32 i;
		for (i = size / B_PAGE_SIZE; i-- > 0; pageNumber++) {
			page = vm_lookup_page(pageNumber);
			if (page == NULL)
				panic("couldn't lookup physical page just allocated\n");

			vm_page_set_state(page, PAGE_STATE_FREE);
		}
	}

	return status;
}


area_id
vm_map_physical_memory(team_id team, const char *name, void **_address,
	uint32 addressSpec, addr_t size, uint32 protection, addr_t physicalAddress)
{
	vm_area *area;
	vm_cache *cache;
	vm_store *store;
	addr_t mapOffset;

	TRACE(("vm_map_physical_memory(aspace = %ld, \"%s\", virtual = %p, "
		"spec = %ld, size = %lu, protection = %ld, phys = %#lx)\n", team,
		name, _address, addressSpec, size, protection, physicalAddress));

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

	// create an device store object

	store = vm_store_create_device(physicalAddress);
	if (store == NULL)
		return B_NO_MEMORY;

	cache = vm_cache_create(store);
	if (cache == NULL) {
		store->ops->destroy(store);
		return B_NO_MEMORY;
	}

	// tell the page scanner to skip over this area, it's pages are special
	cache->scan_skip = 1;
	cache->type = CACHE_TYPE_DEVICE;
	cache->virtual_size = size;

	mutex_lock(&cache->lock);

	status_t status = map_backing_store(locker.AddressSpace(), cache, _address,
		0, size, addressSpec & ~B_MTR_MASK, B_FULL_LOCK, protection,
		REGION_NO_PRIVATE_MAP, &area, name, false);

	mutex_unlock(&cache->lock);

	if (status < B_OK)
		vm_cache_release_ref(cache);

	if (status >= B_OK && (addressSpec & B_MTR_MASK) != 0) {
		// set requested memory type
		status = arch_vm_set_memory_type(area, physicalAddress,
			addressSpec & B_MTR_MASK);
		if (status < B_OK)
			delete_area(locker.AddressSpace(), area);
	}

	if (status >= B_OK) {
		// make sure our area is mapped in completely

		vm_translation_map *map = &locker.AddressSpace()->translation_map;
		size_t reservePages = map->ops->map_max_pages_need(map, area->base,
			area->base + (size - 1));

		vm_page_reserve_pages(reservePages);
		map->ops->lock(map);

		for (addr_t offset = 0; offset < size; offset += B_PAGE_SIZE) {
			map->ops->map(map, area->base + offset, physicalAddress + offset,
				protection);
		}

		map->ops->unlock(map);
		vm_page_unreserve_pages(reservePages);
	}

	if (status < B_OK)
		return status;

	// modify the pointer returned to be offset back into the new area
	// the same way the physical address in was offset
	*_address = (void *)((addr_t)*_address + mapOffset);

	area->cache_type = CACHE_TYPE_DEVICE;
	return area->id;
}


area_id
vm_create_null_area(team_id team, const char *name, void **address,
	uint32 addressSpec, addr_t size)
{
	vm_area *area;
	vm_cache *cache;
	vm_store *store;
	status_t status;

	AddressSpaceWriteLocker locker(team);
	if (!locker.IsLocked())
		return B_BAD_TEAM_ID;

	size = PAGE_ALIGN(size);

	// create an null store object

	store = vm_store_create_null();
	if (store == NULL)
		return B_NO_MEMORY;

	cache = vm_cache_create(store);
	if (cache == NULL) {
		store->ops->destroy(store);
		return B_NO_MEMORY;
	}

	// tell the page scanner to skip over this area, no pages will be mapped here
	cache->scan_skip = 1;
	cache->type = CACHE_TYPE_NULL;
	cache->virtual_size = size;

	mutex_lock(&cache->lock);

	status = map_backing_store(locker.AddressSpace(), cache, address, 0, size,
		addressSpec, 0, B_KERNEL_READ_AREA, REGION_NO_PRIVATE_MAP, &area, name,
		false);

	mutex_unlock(&cache->lock);

	if (status < B_OK) {
		vm_cache_release_ref(cache);
		return status;
	}

	area->cache_type = CACHE_TYPE_NULL;
	return area->id;
}


/*!	Creates the vnode cache for the specified \a vnode.
	The vnode has to be marked busy when calling this function.
*/
status_t
vm_create_vnode_cache(struct vnode *vnode, struct vm_cache **_cache)
{
	status_t status;

	// create a vnode store object
	vm_store *store = vm_create_vnode_store(vnode);
	if (store == NULL)
		return B_NO_MEMORY;

	vm_cache *cache = vm_cache_create(store);
	if (cache == NULL) {
		status = B_NO_MEMORY;
		goto err1;
	}

	cache->type = CACHE_TYPE_VNODE;

	*_cache = cache;
	return B_OK;

err1:
	store->ops->destroy(store);
	return status;
}


/*!	Will map the file specified by \a fd to an area in memory.
	The file will be mirrored beginning at the specified \a offset. The
	\a offset and \a size arguments have to be page aligned.
*/
static area_id
_vm_map_file(team_id team, const char *name, void **_address, uint32 addressSpec,
	size_t size, uint32 protection, uint32 mapping, int fd, off_t offset,
	bool kernel)
{
	// TODO: for binary files, we want to make sure that they get the
	//	copy of a file at a given time, ie. later changes should not
	//	make it into the mapped copy -- this will need quite some changes
	//	to be done in a nice way
	TRACE(("_vm_map_file(\"%s\", offset = %Ld, size = %lu, mapping %ld)\n",
		path, offset, size, mapping));

	offset = ROUNDOWN(offset, B_PAGE_SIZE);
	size = PAGE_ALIGN(size);

	if (mapping == REGION_NO_PRIVATE_MAP)
		protection |= B_SHARED_AREA;

	if (fd < 0) {
		return vm_create_anonymous_area(team, name, _address, addressSpec, size,
			B_NO_LOCK, protection, addressSpec == B_EXACT_ADDRESS);
	}

	// get the open flags of the FD
	file_descriptor *descriptor = get_fd(get_current_io_context(kernel), fd);
	if (descriptor == NULL)
		return EBADF;
	int32 openMode = descriptor->open_mode;
	put_fd(descriptor);

	// The FD must open for reading at any rate. For shared mapping with write
	// access, additionally the FD must be open for writing.
	if ((openMode & O_ACCMODE) == O_WRONLY
		|| mapping == REGION_NO_PRIVATE_MAP
			&& (protection & (B_WRITE_AREA | B_KERNEL_WRITE_AREA)) != 0
			&& (openMode & O_ACCMODE) == O_RDONLY) {
		return EACCES;
	}

	// get the vnode for the object, this also grabs a ref to it
	struct vnode *vnode = NULL;
	status_t status = vfs_get_vnode_from_fd(fd, kernel, &vnode);
	if (status < B_OK)
		return status;
	CObjectDeleter<struct vnode> vnodePutter(vnode, vfs_put_vnode);

	AddressSpaceWriteLocker locker(team);
	if (!locker.IsLocked())
		return B_BAD_TEAM_ID;

	// TODO: this only works for file systems that use the file cache
	vm_cache *cache;
	status = vfs_get_vnode_cache(vnode, &cache, false);
	if (status < B_OK)
		return status;

	mutex_lock(&cache->lock);

	vm_area *area;
	status = map_backing_store(locker.AddressSpace(), cache, _address,
		offset, size, addressSpec, 0, protection, mapping, &area, name,
		addressSpec == B_EXACT_ADDRESS);

	mutex_unlock(&cache->lock);

	if (status < B_OK || mapping == REGION_PRIVATE_MAP) {
		// map_backing_store() cannot know we no longer need the ref
		vm_cache_release_ref(cache);
	}
	if (status < B_OK)
		return status;

	area->cache_type = CACHE_TYPE_VNODE;
	return area->id;
}


area_id
vm_map_file(team_id aid, const char *name, void **address, uint32 addressSpec,
	addr_t size, uint32 protection, uint32 mapping, int fd, off_t offset)
{
	if (!arch_vm_supports_protection(protection))
		return B_NOT_SUPPORTED;

	return _vm_map_file(aid, name, address, addressSpec, size, protection,
		mapping, fd, offset, true);
}


vm_cache *
vm_area_get_locked_cache(vm_area *area)
{
	MutexLocker locker(sAreaCacheLock);
	while (true) {
		vm_cache* cache = area->cache;
		vm_cache_acquire_ref(cache);
		locker.Unlock();

		mutex_lock(&cache->lock);

		locker.Lock();
		if (cache == area->cache)
			return cache;

		// the cache changed in the meantime
		mutex_unlock(&cache->lock);
		vm_cache_release_ref(cache);
	}
}


void
vm_area_put_locked_cache(vm_cache *cache)
{
	mutex_unlock(&cache->lock);
	vm_cache_release_ref(cache);
}


area_id
vm_clone_area(team_id team, const char *name, void **address,
	uint32 addressSpec, uint32 protection, uint32 mapping, area_id sourceID)
{
	vm_area *newArea = NULL;
	vm_area *sourceArea;

	MultiAddressSpaceLocker locker;
	vm_address_space *sourceAddressSpace;
	status_t status = locker.AddArea(sourceID, false, &sourceAddressSpace);
	if (status != B_OK)
		return status;

	vm_address_space *targetAddressSpace;
	status = locker.AddTeam(team, true, &targetAddressSpace);
	if (status != B_OK)
		return status;

	status = locker.Lock();
	if (status != B_OK)
		return status;

	sourceArea = lookup_area(sourceAddressSpace, sourceID);
	if (sourceArea == NULL)
		return B_BAD_VALUE;

	vm_cache *cache = vm_area_get_locked_cache(sourceArea);

	// ToDo: for now, B_USER_CLONEABLE is disabled, until all drivers
	//	have been adapted. Maybe it should be part of the kernel settings,
	//	anyway (so that old drivers can always work).
#if 0
	if (sourceArea->aspace == vm_kernel_address_space() && addressSpace != vm_kernel_address_space()
		&& !(sourceArea->protection & B_USER_CLONEABLE_AREA)) {
		// kernel areas must not be cloned in userland, unless explicitly
		// declared user-cloneable upon construction
		status = B_NOT_ALLOWED;
	} else
#endif
	if (sourceArea->cache_type == CACHE_TYPE_NULL)
		status = B_NOT_ALLOWED;
	else {
		status = map_backing_store(targetAddressSpace, cache, address,
			sourceArea->cache_offset, sourceArea->size, addressSpec,
			sourceArea->wiring, protection, mapping, &newArea, name, false);
	}
	if (status == B_OK && mapping != REGION_PRIVATE_MAP) {
		// If the mapping is REGION_PRIVATE_MAP, map_backing_store() needed
		// to create a new ref, and has therefore already acquired a reference
		// to the source cache - but otherwise it has no idea that we need
		// one.
		vm_cache_acquire_ref(cache);
	}
	if (status == B_OK && newArea->wiring == B_FULL_LOCK) {
		// we need to map in everything at this point
		if (sourceArea->cache_type == CACHE_TYPE_DEVICE) {
			// we don't have actual pages to map but a physical area
			vm_translation_map *map = &sourceArea->address_space->translation_map;
			map->ops->lock(map);

			addr_t physicalAddress;
			uint32 oldProtection;
			map->ops->query(map, sourceArea->base, &physicalAddress,
				&oldProtection);

			map->ops->unlock(map);

			map = &targetAddressSpace->translation_map;
			size_t reservePages = map->ops->map_max_pages_need(map,
				newArea->base, newArea->base + (newArea->size - 1));

			vm_page_reserve_pages(reservePages);
			map->ops->lock(map);

			for (addr_t offset = 0; offset < newArea->size;
					offset += B_PAGE_SIZE) {
				map->ops->map(map, newArea->base + offset,
					physicalAddress + offset, protection);
			}

			map->ops->unlock(map);
			vm_page_unreserve_pages(reservePages);
		} else {
			vm_translation_map *map = &targetAddressSpace->translation_map;
			size_t reservePages = map->ops->map_max_pages_need(map,
				newArea->base, newArea->base + (newArea->size - 1));
			vm_page_reserve_pages(reservePages);

			// map in all pages from source
			for (vm_page *page = cache->page_list; page != NULL;
					page = page->cache_next) {
				vm_map_page(newArea, page, newArea->base
					+ ((page->cache_offset << PAGE_SHIFT) - newArea->cache_offset),
					protection);
			}

			vm_page_unreserve_pages(reservePages);
		}
	}
	if (status == B_OK)
		newArea->cache_type = sourceArea->cache_type;

	vm_area_put_locked_cache(cache);

	if (status < B_OK)
		return status;

	return newArea->id;
}


//! The address space must be write locked at this point
static void
remove_area_from_address_space(vm_address_space *addressSpace, vm_area *area)
{
	vm_area *temp, *last = NULL;

	temp = addressSpace->areas;
	while (temp != NULL) {
		if (area == temp) {
			if (last != NULL) {
				last->address_space_next = temp->address_space_next;
			} else {
				addressSpace->areas = temp->address_space_next;
			}
			addressSpace->change_count++;
			break;
		}
		last = temp;
		temp = temp->address_space_next;
	}
	if (area == addressSpace->area_hint)
		addressSpace->area_hint = NULL;

	if (temp == NULL)
		panic("vm_area_release_ref: area not found in aspace's area list\n");
}


static void
delete_area(vm_address_space *addressSpace, vm_area *area)
{
	acquire_sem_etc(sAreaHashLock, WRITE_COUNT, 0, 0);
	hash_remove(sAreaHash, area);
	release_sem_etc(sAreaHashLock, WRITE_COUNT, 0);

	// At this point the area is removed from the global hash table, but
	// still exists in the area list.

	// Unmap the virtual address space the area occupied
	vm_unmap_pages(area, area->base, area->size, !area->cache->temporary);

	if (!area->cache->temporary)
		vm_cache_write_modified(area->cache, false);

	arch_vm_unset_memory_type(area);
	remove_area_from_address_space(addressSpace, area);
	vm_put_address_space(addressSpace);

	vm_cache_remove_area(area->cache, area);
	vm_cache_release_ref(area->cache);

	free(area->name);
	free(area);
}


status_t
vm_delete_area(team_id team, area_id id)
{
	TRACE(("vm_delete_area(team = 0x%lx, area = 0x%lx)\n", team, id));

	AddressSpaceWriteLocker locker;
	vm_area *area;
	status_t status = locker.SetFromArea(team, id, area);
	if (status < B_OK)
		return status;

	delete_area(locker.AddressSpace(), area);
	return B_OK;
}


/*!	Creates a new cache on top of given cache, moves all areas from
	the old cache to the new one, and changes the protection of all affected
	areas' pages to read-only.
	Preconditions:
	- The given cache must be locked.
	- All of the cache's areas' address spaces must be read locked.
	- All of the cache's areas must have a clear \c no_cache_change flags.
*/
static status_t
vm_copy_on_write_area(vm_cache* lowerCache)
{
	vm_store *store;
	vm_cache *upperCache;
	vm_page *page;
	status_t status;

	TRACE(("vm_copy_on_write_area(cache = %p)\n", lowerCache));

	// We need to separate the cache from its areas. The cache goes one level
	// deeper and we create a new cache inbetween.

	// create an anonymous store object
	store = vm_store_create_anonymous_noswap(false, 0, 0);
	if (store == NULL)
		return B_NO_MEMORY;

	upperCache = vm_cache_create(store);
	if (upperCache == NULL) {
		store->ops->destroy(store);
		return B_NO_MEMORY;
	}

	mutex_lock(&upperCache->lock);

	upperCache->type = CACHE_TYPE_RAM;
	upperCache->temporary = 1;
	upperCache->scan_skip = lowerCache->scan_skip;
	upperCache->virtual_base = lowerCache->virtual_base;
	upperCache->virtual_size = lowerCache->virtual_size;

	// transfer the lower cache areas to the upper cache
	mutex_lock(&sAreaCacheLock);

	upperCache->areas = lowerCache->areas;
	lowerCache->areas = NULL;

	for (vm_area *tempArea = upperCache->areas; tempArea != NULL;
			tempArea = tempArea->cache_next) {
		ASSERT(!tempArea->no_cache_change);

		tempArea->cache = upperCache;
		atomic_add(&upperCache->ref_count, 1);
		atomic_add(&lowerCache->ref_count, -1);
	}

	mutex_unlock(&sAreaCacheLock);

	vm_cache_add_consumer_locked(lowerCache, upperCache);

	// We now need to remap all pages from all of the cache's areas read-only, so that
	// a copy will be created on next write access

	for (vm_area *tempArea = upperCache->areas; tempArea != NULL;
			tempArea = tempArea->cache_next) {
		// The area must be readable in the same way it was previously writable
		uint32 protection = B_KERNEL_READ_AREA;
		if (tempArea->protection & B_READ_AREA)
			protection |= B_READ_AREA;

		vm_translation_map *map = &tempArea->address_space->translation_map;
		map->ops->lock(map);
		map->ops->protect(map, tempArea->base, tempArea->base - 1 + tempArea->size, protection);
		map->ops->unlock(map);
	}

	vm_area_put_locked_cache(upperCache);

	return B_OK;
}


area_id
vm_copy_area(team_id team, const char *name, void **_address,
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
	vm_address_space *targetAddressSpace;
	vm_cache *cache;
	vm_area* source;
	status_t status = locker.AddTeam(team, true, &targetAddressSpace);
	if (status == B_OK) {
		status = locker.AddAreaCacheAndLock(sourceID, false, false, source,
			&cache, true);
	}
	if (status != B_OK)
		return status;

	AreaCacheLocker cacheLocker(cache);	// already locked

	if (addressSpec == B_CLONE_ADDRESS) {
		addressSpec = B_EXACT_ADDRESS;
		*_address = (void *)source->base;
	}

	bool sharedArea = (source->protection & B_SHARED_AREA) != 0;

	// First, create a cache on top of the source area, respectively use the
	// existing one, if this is a shared area.

	vm_area *target;
	status = map_backing_store(targetAddressSpace, cache, _address,
		source->cache_offset, source->size, addressSpec, source->wiring,
		protection, sharedArea ? REGION_NO_PRIVATE_MAP : REGION_PRIVATE_MAP,
		&target, name, false);
	if (status < B_OK)
		return status;

	if (sharedArea) {
		// The new area uses the old area's cache, but map_backing_store()
		// hasn't acquired a ref. So we have to do that now.
		vm_cache_acquire_ref(cache);
	}

	// If the source area is writable, we need to move it one layer up as well

	if (!sharedArea) {
		if ((source->protection & (B_KERNEL_WRITE_AREA | B_WRITE_AREA)) != 0) {
			// TODO: do something more useful if this fails!
			if (vm_copy_on_write_area(cache) < B_OK)
				panic("vm_copy_on_write_area() failed!\n");
		}
	}

	// we return the ID of the newly created area
	return target->id;
}


//! You need to hold the cache lock when calling this function
static int32
count_writable_areas(vm_cache *cache, vm_area *ignoreArea)
{
	struct vm_area *area = cache->areas;
	uint32 count = 0;

	for (; area != NULL; area = area->cache_next) {
		if (area != ignoreArea
			&& (area->protection & (B_WRITE_AREA | B_KERNEL_WRITE_AREA)) != 0)
			count++;
	}

	return count;
}


static status_t
vm_set_area_protection(team_id team, area_id areaID, uint32 newProtection)
{
	TRACE(("vm_set_area_protection(team = %#lx, area = %#lx, protection = %#lx)\n",
		team, areaID, newProtection));

	if (!arch_vm_supports_protection(newProtection))
		return B_NOT_SUPPORTED;

	// lock address spaces and cache
	MultiAddressSpaceLocker locker;
	vm_cache *cache;
	vm_area* area;
	status_t status = locker.AddAreaCacheAndLock(areaID, true, false, area,
		&cache, true);
	AreaCacheLocker cacheLocker(cache);	// already locked

	if (area->protection == newProtection)
		return B_OK;

	if (team != vm_kernel_address_space_id()
		&& area->address_space->id != team) {
		// unless you're the kernel, you are only allowed to set
		// the protection of your own areas
		return B_NOT_ALLOWED;
	}

	bool changePageProtection = true;

	if ((area->protection & (B_WRITE_AREA | B_KERNEL_WRITE_AREA)) != 0
		&& (newProtection & (B_WRITE_AREA | B_KERNEL_WRITE_AREA)) == 0) {
		// writable -> !writable

		if (cache->source != NULL && cache->temporary) {
			if (count_writable_areas(cache, area) == 0) {
				// Since this cache now lives from the pages in its source cache,
				// we can change the cache's commitment to take only those pages
				// into account that really are in this cache.

				// count existing pages in this cache
				struct vm_page *page = cache->page_list;
				uint32 count = 0;

				for (; page != NULL; page = page->cache_next) {
					count++;
				}

				status = cache->store->ops->commit(cache->store,
					cache->virtual_base + count * B_PAGE_SIZE);

				// ToDo: we may be able to join with our source cache, if count == 0
			}
		}
	} else if ((area->protection & (B_WRITE_AREA | B_KERNEL_WRITE_AREA)) == 0
		&& (newProtection & (B_WRITE_AREA | B_KERNEL_WRITE_AREA)) != 0) {
		// !writable -> writable

		if (!list_is_empty(&cache->consumers)) {
			// There are consumers -- we have to insert a new cache. Fortunately
			// vm_copy_on_write_area() does everything that's needed.
			changePageProtection = false;
			status = vm_copy_on_write_area(cache);
		} else {
			// No consumers, so we don't need to insert a new one.
			if (cache->source != NULL && cache->temporary) {
				// the cache's commitment must contain all possible pages
				status = cache->store->ops->commit(cache->store,
					cache->virtual_size);
			}

			if (status == B_OK && cache->source != NULL) {
				// There's a source cache, hence we can't just change all pages'
				// protection or we might allow writing into pages belonging to
				// a lower cache.
				changePageProtection = false;

				struct vm_translation_map *map
					= &area->address_space->translation_map;
				map->ops->lock(map);

				vm_page* page = cache->page_list;
				while (page) {
					addr_t address = area->base
						+ (page->cache_offset << PAGE_SHIFT);
					map->ops->protect(map, address, address - 1 + B_PAGE_SIZE,
						newProtection);
					page = page->cache_next;
				}

				map->ops->unlock(map);
			}
		}
	} else {
		// we don't have anything special to do in all other cases
	}

	if (status == B_OK) {
		// remap existing pages in this cache
		struct vm_translation_map *map = &area->address_space->translation_map;

		if (changePageProtection) {
			map->ops->lock(map);
			map->ops->protect(map, area->base, area->base + area->size,
				newProtection);
			map->ops->unlock(map);
		}

		area->protection = newProtection;
	}

	return status;
}


status_t
vm_get_page_mapping(team_id team, addr_t vaddr, addr_t *paddr)
{
	vm_address_space *addressSpace = vm_get_address_space_by_id(team);
	if (addressSpace == NULL)
		return B_BAD_TEAM_ID;

	uint32 dummyFlags;
	status_t status = addressSpace->translation_map.ops->query(
		&addressSpace->translation_map, vaddr, paddr, &dummyFlags);

	vm_put_address_space(addressSpace);
	return status;
}


static inline addr_t
virtual_page_address(vm_area *area, vm_page *page)
{
	return area->base
		+ ((page->cache_offset << PAGE_SHIFT) - area->cache_offset);
}


bool
vm_test_map_modification(vm_page *page)
{
	MutexLocker locker(sMappingLock);

	vm_page_mappings::Iterator iterator = page->mappings.GetIterator();
	vm_page_mapping *mapping;
	while ((mapping = iterator.Next()) != NULL) {
		vm_area *area = mapping->area;
		vm_translation_map *map = &area->address_space->translation_map;

		addr_t physicalAddress;
		uint32 flags;
		map->ops->lock(map);
		map->ops->query(map, virtual_page_address(area, page),
			&physicalAddress, &flags);
		map->ops->unlock(map);

		if (flags & PAGE_MODIFIED)
			return true;
	}

	return false;
}


int32
vm_test_map_activation(vm_page *page, bool *_modified)
{
	int32 activation = 0;
	bool modified = false;

	MutexLocker locker(sMappingLock);

	vm_page_mappings::Iterator iterator = page->mappings.GetIterator();
	vm_page_mapping *mapping;
	while ((mapping = iterator.Next()) != NULL) {
		vm_area *area = mapping->area;
		vm_translation_map *map = &area->address_space->translation_map;

		addr_t physicalAddress;
		uint32 flags;
		map->ops->lock(map);
		map->ops->query(map, virtual_page_address(area, page),
			&physicalAddress, &flags);
		map->ops->unlock(map);

		if (flags & PAGE_ACCESSED)
			activation++;
		if (flags & PAGE_MODIFIED)
			modified = true;
	}

	if (_modified != NULL)
		*_modified = modified;

	return activation;
}


void
vm_clear_map_flags(vm_page *page, uint32 flags)
{
	MutexLocker locker(sMappingLock);

	vm_page_mappings::Iterator iterator = page->mappings.GetIterator();
	vm_page_mapping *mapping;
	while ((mapping = iterator.Next()) != NULL) {
		vm_area *area = mapping->area;
		vm_translation_map *map = &area->address_space->translation_map;

		map->ops->lock(map);
		map->ops->clear_flags(map, virtual_page_address(area, page), flags);
		map->ops->unlock(map);
	}
}


/*!	Removes all mappings from a page.
	After you've called this function, the page is unmapped from memory.
	The accumulated page flags of all mappings can be found in \a _flags.
*/
void
vm_remove_all_page_mappings(vm_page *page, uint32 *_flags)
{
	uint32 accumulatedFlags = 0;
	MutexLocker locker(sMappingLock);

	vm_page_mappings queue;
	queue.MoveFrom(&page->mappings);

	vm_page_mappings::Iterator iterator = queue.GetIterator();
	vm_page_mapping *mapping;
	while ((mapping = iterator.Next()) != NULL) {
		vm_area *area = mapping->area;
		vm_translation_map *map = &area->address_space->translation_map;
		addr_t physicalAddress;
		uint32 flags;

		map->ops->lock(map);
		addr_t address = virtual_page_address(area, page);
		map->ops->unmap(map, address, address + (B_PAGE_SIZE - 1));
		map->ops->flush(map);
		map->ops->query(map, address, &physicalAddress, &flags);
		map->ops->unlock(map);

		area->mappings.Remove(mapping);

		accumulatedFlags |= flags;
	}

	locker.Unlock();

	// free now unused mappings

	while ((mapping = queue.RemoveHead()) != NULL) {
		free(mapping);
	}

	if (_flags != NULL)
		*_flags = accumulatedFlags;
}


status_t
vm_unmap_pages(vm_area *area, addr_t base, size_t size, bool preserveModified)
{
	vm_translation_map *map = &area->address_space->translation_map;
	addr_t end = base + (size - 1);

	map->ops->lock(map);

	if (area->wiring != B_NO_LOCK && area->cache_type != CACHE_TYPE_DEVICE) {
		// iterate through all pages and decrease their wired count
		for (addr_t virtualAddress = base; virtualAddress < end;
				virtualAddress += B_PAGE_SIZE) {
			addr_t physicalAddress;
			uint32 flags;
			status_t status = map->ops->query(map, virtualAddress,
				&physicalAddress, &flags);
			if (status < B_OK || (flags & PAGE_PRESENT) == 0)
				continue;

			vm_page *page = vm_lookup_page(physicalAddress / B_PAGE_SIZE);
			if (page == NULL) {
				panic("area %p looking up page failed for pa 0x%lx\n", area,
					physicalAddress);
			}

			page->wired_count--;
				// TODO: needs to be atomic on all platforms!
		}
	}

	map->ops->unmap(map, base, end);
	if (preserveModified) {
		map->ops->flush(map);

		for (addr_t virtualAddress = base; virtualAddress < end;
				virtualAddress += B_PAGE_SIZE) {
			addr_t physicalAddress;
			uint32 flags;
			status_t status = map->ops->query(map, virtualAddress,
				&physicalAddress, &flags);
			if (status < B_OK || (flags & PAGE_PRESENT) == 0)
				continue;

			vm_page *page = vm_lookup_page(physicalAddress / B_PAGE_SIZE);
			if (page == NULL) {
				panic("area %p looking up page failed for pa 0x%lx\n", area,
					physicalAddress);
			}

			if ((flags & PAGE_MODIFIED) != 0
				&& page->state != PAGE_STATE_MODIFIED)
				vm_page_set_state(page, PAGE_STATE_MODIFIED);
		}
	}
	map->ops->unlock(map);

	if (area->wiring == B_NO_LOCK) {
		uint32 startOffset = (area->cache_offset + base - area->base)
			>> PAGE_SHIFT;
		uint32 endOffset = startOffset + (size >> PAGE_SHIFT);
		vm_page_mapping *mapping;
		vm_area_mappings queue;

		mutex_lock(&sMappingLock);
		map->ops->lock(map);

		vm_area_mappings::Iterator iterator = area->mappings.GetIterator();
		while (iterator.HasNext()) {
			mapping = iterator.Next();

			vm_page *page = mapping->page;
			if (page->cache_offset < startOffset
				|| page->cache_offset >= endOffset)
				continue;

			mapping->page->mappings.Remove(mapping);
			iterator.Remove();

			queue.Add(mapping);
		}

		map->ops->unlock(map);
		mutex_unlock(&sMappingLock);

		while ((mapping = queue.RemoveHead()) != NULL) {
			free(mapping);
		}
	}

	return B_OK;
}


/*!	When calling this function, you need to have pages reserved! */
status_t
vm_map_page(vm_area *area, vm_page *page, addr_t address, uint32 protection)
{
	vm_translation_map *map = &area->address_space->translation_map;
	vm_page_mapping *mapping = NULL;

	if (area->wiring == B_NO_LOCK) {
		mapping = (vm_page_mapping *)malloc(sizeof(vm_page_mapping));
		if (mapping == NULL)
			return B_NO_MEMORY;

		mapping->page = page;
		mapping->area = area;
	}

	map->ops->lock(map);
	map->ops->map(map, address, page->physical_page_number * B_PAGE_SIZE,
		protection);
	map->ops->unlock(map);

	if (area->wiring != B_NO_LOCK) {
		page->wired_count++;
			// TODO: needs to be atomic on all platforms!
	} else {
		// insert mapping into lists
		MutexLocker locker(sMappingLock);

		page->mappings.Add(mapping);
		area->mappings.Add(mapping);
	}

	if (page->usage_count < 0)
		page->usage_count = 1;

	if (page->state != PAGE_STATE_MODIFIED)
		vm_page_set_state(page, PAGE_STATE_ACTIVE);

	return B_OK;
}


static int
display_mem(int argc, char **argv)
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

	if (physical) {
		int32 offset = address & (B_PAGE_SIZE - 1);
		if (num * itemSize + offset > B_PAGE_SIZE) {
			num = (B_PAGE_SIZE - offset) / itemSize;
			kprintf("NOTE: number of bytes has been cut to page size\n");
		}

		address = ROUNDOWN(address, B_PAGE_SIZE);

		kernel_startup = true;
			// vm_get_physical_page() needs to lock...

		if (vm_get_physical_page(address, &copyAddress, PHYSICAL_PAGE_NO_WAIT) != B_OK) {
			kprintf("getting the hardware page failed.");
			kernel_startup = false;
			return 0;
		}

		kernel_startup = false;
		address += offset;
		copyAddress += offset;
	} else
		copyAddress = address;

	if (!strcmp(argv[0], "string")) {
		kprintf("%p \"", (char*)copyAddress);

		// string mode
		for (i = 0; true; i++) {
			char c;
			if (user_memcpy(&c, (char*)copyAddress + i, 1) != B_OK
				|| c == '\0')
				break;

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
					if (user_memcpy(&c, (char*)copyAddress + i * itemSize + j,
							1) != B_OK) {
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

			if (user_memcpy(&value, (uint8*)copyAddress + i * itemSize,
					itemSize) != B_OK) {
				kprintf("read fault");
				break;
			}

			switch (itemSize) {
				case 1:
					kprintf(" %02x", *(uint8 *)&value);
					break;
				case 2:
					kprintf(" %04x", *(uint16 *)&value);
					break;
				case 4:
					kprintf(" %08lx", *(uint32 *)&value);
					break;
				case 8:
					kprintf(" %016Lx", *(uint64 *)&value);
					break;
			}
		}

		kprintf("\n");
	}

	if (physical) {
		copyAddress = ROUNDOWN(copyAddress, B_PAGE_SIZE);
		kernel_startup = true;
		vm_put_physical_page(copyAddress);
		kernel_startup = false;
	}
	return 0;
}


static void
dump_cache_tree_recursively(vm_cache* cache, int level,
	vm_cache* highlightCache)
{
	// print this cache
	for (int i = 0; i < level; i++)
		kprintf("  ");
	if (cache == highlightCache)
		kprintf("%p <--\n", cache);
	else
		kprintf("%p\n", cache);

	// recursively print its consumers
	vm_cache* consumer = NULL;
	while ((consumer = (vm_cache *)list_get_next_item(&cache->consumers,
			consumer)) != NULL) {
		dump_cache_tree_recursively(consumer, level + 1, highlightCache);
	}
}


static int
dump_cache_tree(int argc, char **argv)
{
	if (argc != 2 || !strcmp(argv[1], "--help")) {
		kprintf("usage: %s <address>\n", argv[0]);
		return 0;
	}

	addr_t address = parse_expression(argv[1]);
	if (address == 0)
		return 0;

	vm_cache *cache = (vm_cache *)address;
	vm_cache *root = cache;

	// find the root cache (the transitive source)
	while (root->source != NULL)
		root = root->source;

	dump_cache_tree_recursively(root, 0, cache);

	return 0;
}


#if DEBUG_CACHE_LIST

static int
dump_caches(int argc, char **argv)
{
	kprintf("caches:");

	vm_cache* cache = gDebugCacheList;
	while (cache) {
		kprintf(" %p", cache);
		cache = cache->debug_next;
	}

	kprintf("\n");

	return 0;
}

#endif	// DEBUG_CACHE_LIST


static const char *
cache_type_to_string(int32 type)
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


static int
dump_cache(int argc, char **argv)
{
	vm_cache *cache;
	bool showPages = false;
	int i = 1;

	if (argc < 2 || !strcmp(argv[1], "--help")) {
		kprintf("usage: %s [-ps] <address>\n"
			"  if -p is specified, all pages are shown, if -s is used\n"
			"  only the cache info is shown respectively.\n", argv[0]);
		return 0;
	}
	while (argv[i][0] == '-') {
		char *arg = argv[i] + 1;
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

	cache = (vm_cache *)address;

	kprintf("CACHE %p:\n", cache);
	kprintf("  ref_count:    %ld\n", cache->ref_count);
	kprintf("  source:       %p\n", cache->source);
	kprintf("  store:        %p\n", cache->store);
	kprintf("  type:         %s\n", cache_type_to_string(cache->type));
	kprintf("  virtual_base: 0x%Lx\n", cache->virtual_base);
	kprintf("  virtual_size: 0x%Lx\n", cache->virtual_size);
	kprintf("  temporary:    %ld\n", cache->temporary);
	kprintf("  scan_skip:    %ld\n", cache->scan_skip);
	kprintf("  lock.holder:  %ld\n", cache->lock.holder);
	kprintf("  lock.sem:     0x%lx\n", cache->lock.sem);
	kprintf("  areas:\n");

	for (vm_area *area = cache->areas; area != NULL; area = area->cache_next) {
		kprintf("    area 0x%lx, %s\n", area->id, area->name);
		kprintf("\tbase_addr:  0x%lx, size: 0x%lx\n", area->base, area->size);
		kprintf("\tprotection: 0x%lx\n", area->protection);
		kprintf("\towner:      0x%lx\n", area->address_space->id);
	}

	kprintf("  consumers:\n");
	vm_cache *consumer = NULL;
	while ((consumer = (vm_cache *)list_get_next_item(&cache->consumers, consumer)) != NULL) {
		kprintf("\t%p\n", consumer);
	}

	kprintf("  pages:\n");
	int32 count = 0;
	for (vm_page *page = cache->page_list; page != NULL; page = page->cache_next) {
		count++;
		if (!showPages)
			continue;

		if (page->type == PAGE_TYPE_PHYSICAL) {
			kprintf("\t%p ppn 0x%lx offset 0x%lx type %u state %u (%s) wired_count %u\n",
				page, page->physical_page_number, page->cache_offset, page->type, page->state,
				page_state_to_string(page->state), page->wired_count);
		} else if(page->type == PAGE_TYPE_DUMMY) {
			kprintf("\t%p DUMMY PAGE state %u (%s)\n",
				page, page->state, page_state_to_string(page->state));
		} else
			kprintf("\t%p UNKNOWN PAGE type %u\n", page, page->type);
	}

	if (!showPages)
		kprintf("\t%ld in cache\n", count);

	return 0;
}


static void
dump_area_struct(vm_area *area, bool mappings)
{
	kprintf("AREA: %p\n", area);
	kprintf("name:\t\t'%s'\n", area->name);
	kprintf("owner:\t\t0x%lx\n", area->address_space->id);
	kprintf("id:\t\t0x%lx\n", area->id);
	kprintf("base:\t\t0x%lx\n", area->base);
	kprintf("size:\t\t0x%lx\n", area->size);
	kprintf("protection:\t0x%lx\n", area->protection);
	kprintf("wiring:\t\t0x%x\n", area->wiring);
	kprintf("memory_type:\t0x%x\n", area->memory_type);
	kprintf("cache:\t\t%p\n", area->cache);
	kprintf("cache_type:\t%s\n", cache_type_to_string(area->cache_type));
	kprintf("cache_offset:\t0x%Lx\n", area->cache_offset);
	kprintf("cache_next:\t%p\n", area->cache_next);
	kprintf("cache_prev:\t%p\n", area->cache_prev);

	vm_area_mappings::Iterator iterator = area->mappings.GetIterator();
	if (mappings) {
		kprintf("page mappings:\n");
		while (iterator.HasNext()) {
			vm_page_mapping *mapping = iterator.Next();
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
dump_area(int argc, char **argv)
{
	bool mappings = false;
	bool found = false;
	int32 index = 1;
	vm_area *area;
	addr_t num;

	if (argc < 2 || !strcmp(argv[1], "--help")) {
		kprintf("usage: area [-m] <id|address|name>\n");
		return 0;
	}

	if (!strcmp(argv[1], "-m")) {
		mappings = true;
		index++;
	}

	num = parse_expression(argv[index]);

	// walk through the area list, looking for the arguments as a name
	struct hash_iterator iter;

	hash_open(sAreaHash, &iter);
	while ((area = (vm_area *)hash_next(sAreaHash, &iter)) != NULL) {
		if ((area->name != NULL && !strcmp(argv[index], area->name))
			|| num != 0
				&& ((addr_t)area->id == num
					|| area->base <= num && area->base + area->size > num)) {
			dump_area_struct(area, mappings);
			found = true;
		}
	}

	if (!found)
		kprintf("could not find area %s (%ld)\n", argv[index], num);
	return 0;
}


static int
dump_area_list(int argc, char **argv)
{
	vm_area *area;
	struct hash_iterator iter;
	const char *name = NULL;
	int32 id = 0;

	if (argc > 1) {
		id = parse_expression(argv[1]);
		if (id == 0)
			name = argv[1];
	}

	kprintf("addr          id  base\t\tsize    protect lock  name\n");

	hash_open(sAreaHash, &iter);
	while ((area = (vm_area *)hash_next(sAreaHash, &iter)) != NULL) {
		if (id != 0 && area->address_space->id != id
			|| name != NULL && strstr(area->name, name) == NULL)
			continue;

		kprintf("%p %5lx  %p\t%p %4lx\t%4d  %s\n", area, area->id, (void *)area->base,
			(void *)area->size, area->protection, area->wiring, area->name);
	}
	hash_close(sAreaHash, &iter, false);
	return 0;
}


static int
dump_available_memory(int argc, char **argv)
{
	kprintf("Available memory: %Ld/%lu bytes\n",
		sAvailableMemory, vm_page_num_pages() * B_PAGE_SIZE);
	return 0;
}


status_t
vm_delete_areas(struct vm_address_space *addressSpace)
{
	vm_area *area;
	vm_area *next, *last = NULL;

	TRACE(("vm_delete_areas: called on address space 0x%lx\n",
		addressSpace->id));

	acquire_sem_etc(addressSpace->sem, WRITE_COUNT, 0, 0);

	// remove all reserved areas in this address space

	for (area = addressSpace->areas; area; area = next) {
		next = area->address_space_next;

		if (area->id == RESERVED_AREA_ID) {
			// just remove it
			if (last)
				last->address_space_next = area->address_space_next;
			else
				addressSpace->areas = area->address_space_next;

			vm_put_address_space(addressSpace);
			free(area);
			continue;
		}

		last = area;
	}

	// delete all the areas in this address space

	for (area = addressSpace->areas; area; area = next) {
		next = area->address_space_next;
		delete_area(addressSpace, area);
	}

	release_sem_etc(addressSpace->sem, WRITE_COUNT, 0);
	return B_OK;
}


static area_id
vm_area_for(team_id team, addr_t address)
{
	AddressSpaceReadLocker locker(team);
	if (!locker.IsLocked())
		return B_BAD_TEAM_ID;

	vm_area *area = vm_area_lookup(locker.AddressSpace(), address);
	if (area != NULL)
		return area->id;

	return B_ERROR;
}


/*!
	Frees physical pages that were used during the boot process.
*/
static void
unmap_and_free_physical_pages(vm_translation_map *map, addr_t start, addr_t end)
{
	// free all physical pages in the specified range

	for (addr_t current = start; current < end; current += B_PAGE_SIZE) {
		addr_t physicalAddress;
		uint32 flags;

		if (map->ops->query(map, current, &physicalAddress, &flags) == B_OK) {
			vm_page *page = vm_lookup_page(current / B_PAGE_SIZE);
			if (page != NULL)
				vm_page_set_state(page, PAGE_STATE_FREE);
		}
	}

	// unmap the memory
	map->ops->unmap(map, start, end - 1);
}


void
vm_free_unused_boot_loader_range(addr_t start, addr_t size)
{
	vm_translation_map *map = &vm_kernel_address_space()->translation_map;
	addr_t end = start + size;
	addr_t lastEnd = start;
	vm_area *area;

	TRACE(("vm_free_unused_boot_loader_range(): asked to free %p - %p\n", (void *)start, (void *)end));

	// The areas are sorted in virtual address space order, so
	// we just have to find the holes between them that fall
	// into the area we should dispose

	map->ops->lock(map);

	for (area = vm_kernel_address_space()->areas; area; area = area->address_space_next) {
		addr_t areaStart = area->base;
		addr_t areaEnd = areaStart + area->size;

		if (area->id == RESERVED_AREA_ID)
			continue;

		if (areaEnd >= end) {
			// we are done, the areas are already beyond of what we have to free
			lastEnd = end;
			break;
		}

		if (areaStart > lastEnd) {
			// this is something we can free
			TRACE(("free boot range: get rid of %p - %p\n", (void *)lastEnd, (void *)areaStart));
			unmap_and_free_physical_pages(map, lastEnd, areaStart);
		}

		lastEnd = areaEnd;
	}

	if (lastEnd < end) {
		// we can also get rid of some space at the end of the area
		TRACE(("free boot range: also remove %p - %p\n", (void *)lastEnd, (void *)end));
		unmap_and_free_physical_pages(map, lastEnd, end);
	}

	map->ops->unlock(map);
}


static void
create_preloaded_image_areas(struct preloaded_image *image)
{
	char name[B_OS_NAME_LENGTH];
	void *address;
	int32 length;

	// use file name to create a good area name
	char *fileName = strrchr(image->name, '/');
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
	address = (void *)ROUNDOWN(image->text_region.start, B_PAGE_SIZE);
	image->text_region.id = create_area(name, &address, B_EXACT_ADDRESS,
		PAGE_ALIGN(image->text_region.size), B_ALREADY_WIRED,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
		// this will later be remapped read-only/executable by the
		// ELF initialization code

	strcpy(name + length, "_data");
	address = (void *)ROUNDOWN(image->data_region.start, B_PAGE_SIZE);
	image->data_region.id = create_area(name, &address, B_EXACT_ADDRESS,
		PAGE_ALIGN(image->data_region.size), B_ALREADY_WIRED,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
}


/**	Frees all previously kernel arguments areas from the kernel_args structure.
 *	Any boot loader resources contained in that arguments must not be accessed
 *	anymore past this point.
 */

void
vm_free_kernel_args(kernel_args *args)
{
	uint32 i;

	TRACE(("vm_free_kernel_args()\n"));

	for (i = 0; i < args->num_kernel_args_ranges; i++) {
		area_id area = area_for((void *)args->kernel_args_range[i].start);
		if (area >= B_OK)
			delete_area(area);
	}
}


static void
allocate_kernel_args(kernel_args *args)
{
	uint32 i;

	TRACE(("allocate_kernel_args()\n"));

	for (i = 0; i < args->num_kernel_args_ranges; i++) {
		void *address = (void *)args->kernel_args_range[i].start;

		create_area("_kernel args_", &address, B_EXACT_ADDRESS, args->kernel_args_range[i].size,
			B_ALREADY_WIRED, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	}
}


static void
unreserve_boot_loader_ranges(kernel_args *args)
{
	uint32 i;

	TRACE(("unreserve_boot_loader_ranges()\n"));

	for (i = 0; i < args->num_virtual_allocated_ranges; i++) {
		vm_unreserve_address_range(vm_kernel_address_space_id(),
			(void *)args->virtual_allocated_range[i].start,
			args->virtual_allocated_range[i].size);
	}
}


static void
reserve_boot_loader_ranges(kernel_args *args)
{
	uint32 i;

	TRACE(("reserve_boot_loader_ranges()\n"));

	for (i = 0; i < args->num_virtual_allocated_ranges; i++) {
		void *address = (void *)args->virtual_allocated_range[i].start;

		// If the address is no kernel address, we just skip it. The
		// architecture specific code has to deal with it.
		if (!IS_KERNEL_ADDRESS(address)) {
			dprintf("reserve_boot_loader_ranges(): Skipping range: %p, %lu\n",
				address, args->virtual_allocated_range[i].size);
			continue;
		}

		status_t status = vm_reserve_address_range(vm_kernel_address_space_id(), &address,
			B_EXACT_ADDRESS, args->virtual_allocated_range[i].size, 0);
		if (status < B_OK)
			panic("could not reserve boot loader ranges\n");
	}
}


static addr_t
allocate_early_virtual(kernel_args *args, size_t size)
{
	addr_t spot = 0;
	uint32 i;
	int last_valloc_entry = 0;

	size = PAGE_ALIGN(size);
	// find a slot in the virtual allocation addr range
	for (i = 1; i < args->num_virtual_allocated_ranges; i++) {
		addr_t previousRangeEnd = args->virtual_allocated_range[i - 1].start
			+ args->virtual_allocated_range[i - 1].size;
		last_valloc_entry = i;
		// check to see if the space between this one and the last is big enough
		if (previousRangeEnd >= KERNEL_BASE
			&& args->virtual_allocated_range[i].start
				- previousRangeEnd >= size) {
			spot = previousRangeEnd;
			args->virtual_allocated_range[i - 1].size += size;
			goto out;
		}
	}
	if (spot == 0) {
		// we hadn't found one between allocation ranges. this is ok.
		// see if there's a gap after the last one
		addr_t lastRangeEnd
			= args->virtual_allocated_range[last_valloc_entry].start
				+ args->virtual_allocated_range[last_valloc_entry].size;
		if (KERNEL_BASE + (KERNEL_SIZE - 1) - lastRangeEnd >= size) {
			spot = lastRangeEnd;
			args->virtual_allocated_range[last_valloc_entry].size += size;
			goto out;
		}
		// see if there's a gap before the first one
		if (args->virtual_allocated_range[0].start > KERNEL_BASE) {
			if (args->virtual_allocated_range[0].start - KERNEL_BASE >= size) {
				args->virtual_allocated_range[0].start -= size;
				spot = args->virtual_allocated_range[0].start;
				goto out;
			}
		}
	}

out:
	return spot;
}


static bool
is_page_in_physical_memory_range(kernel_args *args, addr_t address)
{
	// TODO: horrible brute-force method of determining if the page can be allocated
	for (uint32 i = 0; i < args->num_physical_memory_ranges; i++) {
		if (address >= args->physical_memory_range[i].start
			&& address < args->physical_memory_range[i].start
				+ args->physical_memory_range[i].size)
			return true;
	}
	return false;
}


static addr_t
allocate_early_physical_page(kernel_args *args)
{
	for (uint32 i = 0; i < args->num_physical_allocated_ranges; i++) {
		addr_t nextPage;

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


/*!
	This one uses the kernel_args' physical and virtual memory ranges to
	allocate some pages before the VM is completely up.
*/
addr_t
vm_allocate_early(kernel_args *args, size_t virtualSize, size_t physicalSize,
	uint32 attributes)
{
	if (physicalSize > virtualSize)
		physicalSize = virtualSize;

	// find the vaddr to allocate at
	addr_t virtualBase = allocate_early_virtual(args, virtualSize);
	//dprintf("vm_allocate_early: vaddr 0x%lx\n", virtualAddress);

	// map the pages
	for (uint32 i = 0; i < PAGE_ALIGN(physicalSize) / B_PAGE_SIZE; i++) {
		addr_t physicalAddress = allocate_early_physical_page(args);
		if (physicalAddress == 0)
			panic("error allocating early page!\n");

		//dprintf("vm_allocate_early: paddr 0x%lx\n", physicalAddress);

		arch_vm_translation_map_early_map(args, virtualBase + i * B_PAGE_SIZE,
			physicalAddress * B_PAGE_SIZE, attributes,
			&allocate_early_physical_page);
	}

	return virtualBase;
}


status_t
vm_init(kernel_args *args)
{
	struct preloaded_image *image;
	void *address;
	status_t err = 0;
	uint32 i;

	TRACE(("vm_init: entry\n"));
	err = arch_vm_translation_map_init(args);
	err = arch_vm_init(args);

	// initialize some globals
	sNextAreaID = 1;
	sAreaHashLock = -1;
	sAvailableMemoryLock.sem = -1;

	vm_page_init_num_pages(args);
	sAvailableMemory = vm_page_num_pages() * B_PAGE_SIZE;

	// map in the new heap and initialize it
	size_t heapSize = INITIAL_HEAP_SIZE;
	addr_t heapBase = vm_allocate_early(args, heapSize, heapSize,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	TRACE(("heap at 0x%lx\n", heapBase));
	heap_init(heapBase, heapSize);

	vm_low_memory_init();

	size_t slabInitialSize = args->num_cpus * 2 * B_PAGE_SIZE;
	addr_t slabInitialBase = vm_allocate_early(args, slabInitialSize,
		slabInitialSize, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	slab_init(args, slabInitialBase, slabInitialSize);

	// initialize the free page list and physical page mapper
	vm_page_init(args);

	// initialize the hash table that stores the pages mapped to caches
	vm_cache_init(args);

	{
		vm_area *area;
		sAreaHash = hash_init(REGION_HASH_TABLE_SIZE, (addr_t)&area->hash_next - (addr_t)area,
			&area_compare, &area_hash);
		if (sAreaHash == NULL)
			panic("vm_init: error creating aspace hash table\n");
	}

	vm_address_space_init();
	reserve_boot_loader_ranges(args);

	// do any further initialization that the architecture dependant layers may need now
	arch_vm_translation_map_init_post_area(args);
	arch_vm_init_post_area(args);
	vm_page_init_post_area(args);

	// allocate areas to represent stuff that already exists

	address = (void *)ROUNDOWN(heapBase, B_PAGE_SIZE);
	create_area("kernel heap", &address, B_EXACT_ADDRESS, heapSize,
		B_ALREADY_WIRED, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);

	address = (void *)ROUNDOWN(slabInitialBase, B_PAGE_SIZE);
	create_area("initial slab space", &address, B_EXACT_ADDRESS,
		slabInitialSize, B_ALREADY_WIRED, B_KERNEL_READ_AREA
		| B_KERNEL_WRITE_AREA);

	allocate_kernel_args(args);

	args->kernel_image.name = "kernel";
		// the lazy boot loader currently doesn't set the kernel's name...
	create_preloaded_image_areas(&args->kernel_image);

	// allocate areas for preloaded images
	for (image = args->preloaded_images; image != NULL; image = image->next) {
		create_preloaded_image_areas(image);
	}

	// allocate kernel stacks
	for (i = 0; i < args->num_cpus; i++) {
		char name[64];

		sprintf(name, "idle thread %lu kstack", i + 1);
		address = (void *)args->cpu_kstack[i].start;
		create_area(name, &address, B_EXACT_ADDRESS, args->cpu_kstack[i].size,
			B_ALREADY_WIRED, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	}

	// add some debugger commands
	add_debugger_command("areas", &dump_area_list, "Dump a list of all areas");
	add_debugger_command("area", &dump_area, "Dump info about a particular area");
	add_debugger_command("cache", &dump_cache, "Dump vm_cache");
	add_debugger_command("cache_tree", &dump_cache_tree, "Dump vm_cache tree");
#if DEBUG_CACHE_LIST
	add_debugger_command("caches", &dump_caches, "List vm_cache structures");
#endif
	add_debugger_command("avail", &dump_available_memory, "Dump available memory");
	add_debugger_command("dl", &display_mem, "dump memory long words (64-bit)");
	add_debugger_command("dw", &display_mem, "dump memory words (32-bit)");
	add_debugger_command("ds", &display_mem, "dump memory shorts (16-bit)");
	add_debugger_command("db", &display_mem, "dump memory bytes (8-bit)");
	add_debugger_command("string", &display_mem, "dump strings");

	TRACE(("vm_init: exit\n"));

	return err;
}


status_t
vm_init_post_sem(kernel_args *args)
{
	vm_area *area;

	// This frees all unused boot loader resources and makes its space available again
	arch_vm_init_end(args);
	unreserve_boot_loader_ranges(args);

	// fill in all of the semaphores that were not allocated before
	// since we're still single threaded and only the kernel address space exists,
	// it isn't that hard to find all of the ones we need to create

	benaphore_init(&sAvailableMemoryLock, "available memory lock");
	arch_vm_translation_map_init_post_sem(args);
	vm_address_space_init_post_sem();

	for (area = vm_kernel_address_space()->areas; area;
			area = area->address_space_next) {
		if (area->id == RESERVED_AREA_ID)
			continue;

		if (area->cache->lock.sem < 0)
			mutex_init(&area->cache->lock, "vm_cache");
	}

	sAreaHashLock = create_sem(WRITE_COUNT, "area hash");
	mutex_init(&sAreaCacheLock, "area->cache");
	mutex_init(&sMappingLock, "page mappings");

	slab_init_post_sem();
	return heap_init_post_sem();
}


status_t
vm_init_post_thread(kernel_args *args)
{
	vm_page_init_post_thread(args);
	vm_daemon_init();
	vm_low_memory_init_post_thread();
	return heap_init_post_thread();
}


status_t
vm_init_post_modules(kernel_args *args)
{
	return arch_vm_init_post_modules(args);
}


void
permit_page_faults(void)
{
	struct thread *thread = thread_get_current_thread();
	if (thread != NULL)
		atomic_add(&thread->page_faults_allowed, 1);
}


void
forbid_page_faults(void)
{
	struct thread *thread = thread_get_current_thread();
	if (thread != NULL)
		atomic_add(&thread->page_faults_allowed, -1);
}


status_t
vm_page_fault(addr_t address, addr_t faultAddress, bool isWrite, bool isUser,
	addr_t *newIP)
{
	FTRACE(("vm_page_fault: page fault at 0x%lx, ip 0x%lx\n", address, faultAddress));

	*newIP = 0;

	status_t status = vm_soft_fault(address, isWrite, isUser);
	if (status < B_OK) {
		dprintf("vm_page_fault: vm_soft_fault returned error '%s' on fault at 0x%lx, ip 0x%lx, write %d, user %d, thread 0x%lx\n",
			strerror(status), address, faultAddress, isWrite, isUser,
			thread_get_current_thread_id());
		if (!isUser) {
			struct thread *thread = thread_get_current_thread();
			if (thread != NULL && thread->fault_handler != 0) {
				// this will cause the arch dependant page fault handler to
				// modify the IP on the interrupt frame or whatever to return
				// to this address
				*newIP = thread->fault_handler;
			} else {
				// unhandled page fault in the kernel
				panic("vm_page_fault: unhandled page fault in kernel space at 0x%lx, ip 0x%lx\n",
					address, faultAddress);
			}
		} else {
#if 1
			// ToDo: remove me once we have proper userland debugging support (and tools)
			vm_address_space *addressSpace = vm_get_current_user_address_space();
			vm_area *area;

			acquire_sem_etc(addressSpace->sem, READ_COUNT, 0, 0);
// TODO: The user_memcpy() below can cause a deadlock, if it causes a page
// fault and someone is already waiting for a write lock on the same address
// space. This thread will then try to acquire the semaphore again and will
// be queued after the writer.
			area = vm_area_lookup(addressSpace, faultAddress);

			dprintf("vm_page_fault: sending team \"%s\" 0x%lx SIGSEGV, ip %#lx (\"%s\" +%#lx)\n",
				thread_get_current_thread()->team->name,
				thread_get_current_thread()->team->id, faultAddress,
				area ? area->name : "???", faultAddress - (area ? area->base : 0x0));

			// We can print a stack trace of the userland thread here.
#if 1
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
#ifdef __INTEL__
				struct iframe *iframe = i386_get_user_iframe();
				if (iframe == NULL)
					panic("iframe is NULL!");

				status_t status = user_memcpy(&frame, (void *)iframe->ebp,
					sizeof(struct stack_frame));
#elif defined(__POWERPC__)
				struct iframe *iframe = ppc_get_user_iframe();
				if (iframe == NULL)
					panic("iframe is NULL!");

				status_t status = user_memcpy(&frame, (void *)iframe->r1,
					sizeof(struct stack_frame));
#else
#	warning "vm_page_fault() stack trace won't work"
				status = B_ERROR;
#endif

				dprintf("stack trace:\n");
				int32 maxFrames = 50;
				while (status == B_OK && --maxFrames >= 0
						&& frame.return_address != NULL) {
					dprintf("  %p", frame.return_address);
					area = vm_area_lookup(addressSpace,
						(addr_t)frame.return_address);
					if (area) {
						dprintf(" (%s + %#lx)", area->name,
							(addr_t)frame.return_address - area->base);
					}
					dprintf("\n");

					status = user_memcpy(&frame, frame.previous,
						sizeof(struct stack_frame));
				}
			}
#endif	// 0 (stack trace)

			release_sem_etc(addressSpace->sem, READ_COUNT, 0);
			vm_put_address_space(addressSpace);
#endif
			if (user_debug_exception_occurred(B_SEGMENT_VIOLATION, SIGSEGV))
				send_signal(team_get_current_team_id(), SIGSEGV);
		}
	}

	return B_HANDLED_INTERRUPT;
}


static inline status_t
fault_acquire_locked_source(vm_cache *cache, vm_cache **_source)
{
retry:
	vm_cache *source = cache->source;
	if (source == NULL)
		return B_ERROR;
	if (source->busy)
		return B_BUSY;

	vm_cache_acquire_ref(source);

	mutex_lock(&source->lock);

	if (source->busy) {
		mutex_unlock(&source->lock);
		vm_cache_release_ref(source);
		goto retry;
	}

	*_source = source;
	return B_OK;
}


/*!
	Inserts a busy dummy page into a cache, and makes sure the cache won't go
	away by grabbing a reference to it.
*/
static inline void
fault_insert_dummy_page(vm_cache *cache, vm_dummy_page &dummyPage,
	off_t cacheOffset)
{
	dummyPage.state = PAGE_STATE_BUSY;
	vm_cache_acquire_ref(cache);
	vm_cache_insert_page(cache, &dummyPage, cacheOffset);
	dummyPage.busy_condition.Publish(&dummyPage, "page");
}


/*!
	Removes the busy dummy page from a cache, and releases its reference to
	the cache.
*/
static inline void
fault_remove_dummy_page(vm_dummy_page &dummyPage, bool isLocked)
{
	vm_cache *cache = dummyPage.cache;
	if (!isLocked)
		mutex_lock(&cache->lock);

	if (dummyPage.state == PAGE_STATE_BUSY) {
		vm_cache_remove_page(cache, &dummyPage);
		dummyPage.state = PAGE_STATE_INACTIVE;
		dummyPage.busy_condition.Unpublish();
	}

	if (!isLocked)
		mutex_unlock(&cache->lock);

	vm_cache_release_ref(cache);
}


/*!
	Finds a page at the specified \a cacheOffset in either the \a topCacheRef
	or in its source chain. Will also page in a missing page in case there is
	a cache that has the page.
	If it couldn't find a page, it will return the vm_cache that should get it,
	otherwise, it will return the vm_cache that contains the cache.
	It always grabs a reference to the vm_cache that it returns, and also locks it.
*/
static inline status_t
fault_find_page(vm_translation_map *map, vm_cache *topCache,
	off_t cacheOffset, bool isWrite, vm_dummy_page &dummyPage,
	vm_cache **_pageCache, vm_page** _page, bool* _restart)
{
	*_restart = false;
	vm_cache *cache = topCache;
	vm_cache *lastCache = NULL;
	vm_page *page = NULL;

	vm_cache_acquire_ref(cache);
	mutex_lock(&cache->lock);
		// we release this later in the loop

	while (cache != NULL) {
		if (lastCache != NULL)
			vm_cache_release_ref(lastCache);

		// we hold the lock of the cache at this point

		lastCache = cache;

		for (;;) {
			page = vm_cache_lookup_page(cache, cacheOffset);
			if (page != NULL && page->state != PAGE_STATE_BUSY) {
				// we found the page
				break;
			}
			if (page == NULL || page == &dummyPage)
				break;

			// page must be busy -- wait for it to become unbusy
			{
				ConditionVariableEntry entry;
				entry.Add(page);
				mutex_unlock(&cache->lock);
				entry.Wait();
				mutex_lock(&cache->lock);
			}

			if (cache->busy) {
				// The cache became busy, which means, it is about to be
				// removed by vm_cache_remove_consumer(). We start again with
				// the top cache.
				ConditionVariableEntry entry;
				entry.Add(cache);
				mutex_unlock(&cache->lock);
				vm_cache_release_ref(cache);
				entry.Wait();
				*_restart = true;
				return B_OK;
			}
		}

		if (page != NULL && page != &dummyPage)
			break;

		// The current cache does not contain the page we're looking for

		// see if the vm_store has it
		vm_store *store = cache->store;
		if (store->ops->has_page != NULL
			&& store->ops->has_page(store, cacheOffset)) {
			// insert a fresh page and mark it busy -- we're going to read it in
			page = vm_page_allocate_page(PAGE_STATE_FREE, true);
			vm_cache_insert_page(cache, page, cacheOffset);

			ConditionVariable busyCondition;
			busyCondition.Publish(page, "page");

			mutex_unlock(&cache->lock);

			// get a virtual address for the page
			iovec vec;
			map->ops->get_physical_page(
				page->physical_page_number * B_PAGE_SIZE,
				(addr_t *)&vec.iov_base, PHYSICAL_PAGE_CAN_WAIT);
			size_t bytesRead = vec.iov_len = B_PAGE_SIZE;

			// read it in
			status_t status = store->ops->read(store, cacheOffset, &vec, 1,
				&bytesRead, false);

			map->ops->put_physical_page((addr_t)vec.iov_base);

			mutex_lock(&cache->lock);

			if (status < B_OK) {
				// on error remove and free the page
				dprintf("reading page from store %p (cache %p) returned: %s!\n",
					store, cache, strerror(status));

				busyCondition.Unpublish();
				vm_cache_remove_page(cache, page);
				vm_page_set_state(page, PAGE_STATE_FREE);

				mutex_unlock(&cache->lock);
				vm_cache_release_ref(cache);
				return status;
			}

			// mark the page unbusy again
			page->state = PAGE_STATE_ACTIVE;
			busyCondition.Unpublish();
			break;
		}

		// If we're at the top most cache, insert the dummy page here to keep
		// other threads from faulting on the same address and chasing us up the
		// cache chain
		if (cache == topCache && dummyPage.state != PAGE_STATE_BUSY)
			fault_insert_dummy_page(cache, dummyPage, cacheOffset);

		vm_cache *nextCache;
		status_t status = fault_acquire_locked_source(cache, &nextCache);
		if (status == B_BUSY) {
			// the source cache is currently in the process of being merged
			// with his only consumer (cacheRef); since its pages are moved
			// upwards, too, we try this cache again
			mutex_unlock(&cache->lock);
			thread_yield(true);
			mutex_lock(&cache->lock);
			if (cache->busy) {
				// The cache became busy, which means, it is about to be
				// removed by vm_cache_remove_consumer(). We start again with
				// the top cache.
				ConditionVariableEntry entry;
				entry.Add(cache);
				mutex_unlock(&cache->lock);
				vm_cache_release_ref(cache);
				entry.Wait();
				*_restart = true;
				return B_OK;
			}
			lastCache = NULL;
			continue;
		} else if (status < B_OK)
			nextCache = NULL;

		mutex_unlock(&cache->lock);
			// at this point, we still hold a ref to this cache (through lastCacheRef)

		cache = nextCache;
	}

	if (page == NULL) {
		// there was no adequate page, determine the cache for a clean one
		if (cache == NULL) {
			// We rolled off the end of the cache chain, so we need to decide which
			// cache will get the new page we're about to create.
			cache = isWrite ? topCache : lastCache;
				// Read-only pages come in the deepest cache - only the
				// top most cache may have direct write access.
			vm_cache_acquire_ref(cache);
			mutex_lock(&cache->lock);

			if (cache->busy) {
				// The cache became busy, which means, it is about to be
				// removed by vm_cache_remove_consumer(). We start again with
				// the top cache.
				ConditionVariableEntry entry;
				entry.Add(cache);
				mutex_unlock(&cache->lock);
				vm_cache_release_ref(cache);
				entry.Wait();
				*_restart = true;
			} else {
				vm_page* newPage = vm_cache_lookup_page(cache, cacheOffset);
				if (newPage && newPage != &dummyPage) {
					// A new page turned up. It could be the one we're looking
					// for, but it could as well be a dummy page from someone
					// else or an otherwise busy page. We can't really handle
					// that here. Hence we completely restart this functions.
					mutex_unlock(&cache->lock);
					vm_cache_release_ref(cache);
					*_restart = true;
				}
			}
		}

		// release the reference of the last vm_cache we still have from the loop above
		if (lastCache != NULL)
			vm_cache_release_ref(lastCache);
	} else {
		// we still own a reference to the cache
	}

	*_pageCache = cache;
	*_page = page;
	return B_OK;
}


/*!
	Returns the page that should be mapped into the area that got the fault.
	It returns the owner of the page in \a sourceCache - it keeps a reference
	to it, and has also locked it on exit.
*/
static inline status_t
fault_get_page(vm_translation_map *map, vm_cache *topCache, off_t cacheOffset,
	bool isWrite, vm_dummy_page &dummyPage, vm_cache **_sourceCache,
	vm_cache **_copiedSource, vm_page** _page)
{
	vm_cache *cache;
	vm_page *page;
	bool restart;
	for (;;) {
		status_t status = fault_find_page(map, topCache, cacheOffset, isWrite,
			dummyPage, &cache, &page, &restart);
		if (status != B_OK)
			return status;

		if (!restart)
			break;

		// Remove the dummy page, if it has been inserted.
		mutex_lock(&topCache->lock);

		if (dummyPage.state == PAGE_STATE_BUSY) {
			ASSERT_PRINT(dummyPage.cache == topCache, "dummy page: %p\n",
				&dummyPage);
			fault_remove_dummy_page(dummyPage, true);
		}

		mutex_unlock(&topCache->lock);
	}

	if (page == NULL) {
		// we still haven't found a page, so we allocate a clean one

		page = vm_page_allocate_page(PAGE_STATE_CLEAR, true);
		FTRACE(("vm_soft_fault: just allocated page 0x%lx\n", page->physical_page_number));

		// Insert the new page into our cache, and replace it with the dummy page if necessary

		// If we inserted a dummy page into this cache (i.e. if it is the top
		// cache), we have to remove it now
		if (dummyPage.state == PAGE_STATE_BUSY && dummyPage.cache == cache) {
#ifdef DEBUG_PAGE_CACHE_TRANSITIONS
			page->debug_flags = dummyPage.debug_flags | 0x8;
			if (dummyPage.collided_page != NULL) {
				dummyPage.collided_page->collided_page = page;
				page->collided_page = dummyPage.collided_page;
			}
#endif	// DEBUG_PAGE_CACHE_TRANSITIONS

			fault_remove_dummy_page(dummyPage, true);
		}

		vm_cache_insert_page(cache, page, cacheOffset);

		if (dummyPage.state == PAGE_STATE_BUSY) {
#ifdef DEBUG_PAGE_CACHE_TRANSITIONS
			page->debug_flags = dummyPage.debug_flags | 0x10;
			if (dummyPage.collided_page != NULL) {
				dummyPage.collided_page->collided_page = page;
				page->collided_page = dummyPage.collided_page;
			}
#endif	// DEBUG_PAGE_CACHE_TRANSITIONS

			// This is not the top cache into which we inserted the dummy page,
			// let's remove it from there. We need to temporarily unlock our
			// cache to comply with the cache locking policy.
			mutex_unlock(&cache->lock);
			fault_remove_dummy_page(dummyPage, false);
			mutex_lock(&cache->lock);
		}
	}

	// We now have the page and a cache it belongs to - we now need to make
	// sure that the area's cache can access it, too, and sees the correct data

	if (page->cache != topCache && isWrite) {
		// Now we have a page that has the data we want, but in the wrong cache
		// object so we need to copy it and stick it into the top cache.
		// Note that this and the "if" before are mutual exclusive. If
		// fault_find_page() didn't find the page, it would return the top cache
		// for write faults.
		vm_page *sourcePage = page;
		void *source, *dest;

		// ToDo: if memory is low, it might be a good idea to steal the page
		//	from our source cache - if possible, that is
		FTRACE(("get new page, copy it, and put it into the topmost cache\n"));
		page = vm_page_allocate_page(PAGE_STATE_FREE, true);
#if 0
if (cacheOffset == 0x12000)
	dprintf("%ld: copy page %p to page %p from cache %p to cache %p\n", find_thread(NULL),
		sourcePage, page, sourcePage->cache, topCacheRef->cache);
#endif

		// try to get a mapping for the src and dest page so we can copy it
		for (;;) {
			map->ops->get_physical_page(sourcePage->physical_page_number * B_PAGE_SIZE,
				(addr_t *)&source, PHYSICAL_PAGE_CAN_WAIT);

			if (map->ops->get_physical_page(page->physical_page_number * B_PAGE_SIZE,
					(addr_t *)&dest, PHYSICAL_PAGE_NO_WAIT) == B_OK)
				break;

			// it couldn't map the second one, so sleep and retry
			// keeps an extremely rare deadlock from occuring
			map->ops->put_physical_page((addr_t)source);
			snooze(5000);
		}

		memcpy(dest, source, B_PAGE_SIZE);
		map->ops->put_physical_page((addr_t)source);
		map->ops->put_physical_page((addr_t)dest);

		if (sourcePage->state != PAGE_STATE_MODIFIED)
			vm_page_set_state(sourcePage, PAGE_STATE_ACTIVE);

		mutex_unlock(&cache->lock);
		mutex_lock(&topCache->lock);

		// Since the top cache has been unlocked for a while, someone else
		// (vm_cache_remove_consumer()) might have replaced our dummy page.
		vm_page* newPage = NULL;
		for (;;) {
			newPage = vm_cache_lookup_page(topCache, cacheOffset);
			if (newPage == NULL || newPage == &dummyPage) {
				newPage = NULL;
				break;
			}

			if (newPage->state != PAGE_STATE_BUSY)
				break;

			// The page is busy, wait till it becomes unbusy.
			ConditionVariableEntry entry;
			entry.Add(newPage);
			mutex_unlock(&topCache->lock);
			entry.Wait();
			mutex_lock(&topCache->lock);
		}

		if (newPage) {
			// Indeed someone else threw in a page. We free ours and are happy.
			vm_page_set_state(page, PAGE_STATE_FREE);
			page = newPage;
		} else {
			// Insert the new page into our cache and remove the dummy page, if
			// necessary.

			// if we inserted a dummy page into this cache, we have to remove it now
			if (dummyPage.state == PAGE_STATE_BUSY) {
				ASSERT_PRINT(dummyPage.cache == topCache, "dummy page: %p\n",
					&dummyPage);
				fault_remove_dummy_page(dummyPage, true);
			}

			vm_cache_insert_page(topCache, page, cacheOffset);
		}

		*_copiedSource = cache;

		cache = topCache;
		vm_cache_acquire_ref(cache);
	}

	*_sourceCache = cache;
	*_page = page;
	return B_OK;
}


static status_t
vm_soft_fault(addr_t originalAddress, bool isWrite, bool isUser)
{
	vm_address_space *addressSpace;

	FTRACE(("vm_soft_fault: thid 0x%lx address 0x%lx, isWrite %d, isUser %d\n",
		thread_get_current_thread_id(), originalAddress, isWrite, isUser));

	addr_t address = ROUNDOWN(originalAddress, B_PAGE_SIZE);

	if (IS_KERNEL_ADDRESS(address)) {
		addressSpace = vm_get_kernel_address_space();
	} else if (IS_USER_ADDRESS(address)) {
		addressSpace = vm_get_current_user_address_space();
		if (addressSpace == NULL) {
			if (!isUser) {
				dprintf("vm_soft_fault: kernel thread accessing invalid user memory!\n");
				return B_BAD_ADDRESS;
			} else {
				// XXX weird state.
				panic("vm_soft_fault: non kernel thread accessing user memory that doesn't exist!\n");
			}
		}
	} else {
		// the hit was probably in the 64k DMZ between kernel and user space
		// this keeps a user space thread from passing a buffer that crosses
		// into kernel space
		return B_BAD_ADDRESS;
	}

	AddressSpaceReadLocker locker(addressSpace);

	atomic_add(&addressSpace->fault_count, 1);

	// Get the area the fault was in

	vm_area *area = vm_area_lookup(addressSpace, address);
	if (area == NULL) {
		dprintf("vm_soft_fault: va 0x%lx not covered by area in address space\n",
			originalAddress);
		return B_BAD_ADDRESS;
	}

	// check permissions
	if (isUser && (area->protection & B_USER_PROTECTION) == 0) {
		dprintf("user access on kernel area 0x%lx at %p\n", area->id, (void *)originalAddress);
		return B_PERMISSION_DENIED;
	}
	if (isWrite && (area->protection & (B_WRITE_AREA | (isUser ? 0 : B_KERNEL_WRITE_AREA))) == 0) {
		dprintf("write access attempted on read-only area 0x%lx at %p\n",
			area->id, (void *)originalAddress);
		return B_PERMISSION_DENIED;
	}

	// We have the area, it was a valid access, so let's try to resolve the page fault now.
	// At first, the top most cache from the area is investigated

	vm_cache *topCache = vm_area_get_locked_cache(area);
	off_t cacheOffset = address - area->base + area->cache_offset;
	int32 changeCount = addressSpace->change_count;

	atomic_add(&area->no_cache_change, 1);
		// make sure the area's cache isn't replaced during the page fault

	// See if this cache has a fault handler - this will do all the work for us
	{
		vm_store *store = topCache->store;
		if (store->ops->fault != NULL) {
			// Note, since the page fault is resolved with interrupts enabled, the
			// fault handler could be called more than once for the same reason -
			// the store must take this into account
			status_t status = store->ops->fault(store, addressSpace, cacheOffset);
			if (status != B_BAD_HANDLER) {
				vm_area_put_locked_cache(topCache);
				return status;
			}
		}
	}

	mutex_unlock(&topCache->lock);

	// The top most cache has no fault handler, so let's see if the cache or its sources
	// already have the page we're searching for (we're going from top to bottom)

	vm_translation_map *map = &addressSpace->translation_map;
	size_t reservePages = 2 + map->ops->map_max_pages_need(map,
		originalAddress, originalAddress);
	vm_page_reserve_pages(reservePages);
		// we may need up to 2 pages - reserving them upfront makes sure
		// we don't have any cache locked, so that the page daemon/thief
		// can do their job without problems

	vm_dummy_page dummyPage;
	dummyPage.cache = NULL;
	dummyPage.state = PAGE_STATE_INACTIVE;
	dummyPage.type = PAGE_TYPE_DUMMY;
	dummyPage.wired_count = 0;
#ifdef DEBUG_PAGE_CACHE_TRANSITIONS
	dummyPage.debug_flags = 0;
	dummyPage.collided_page = NULL;
#endif	// DEBUG_PAGE_CACHE_TRANSITIONS

	vm_cache *copiedPageSource = NULL;
	vm_cache *pageSource;
	vm_page *page;
	// TODO: We keep the address space read lock during the whole operation
	// which might be rather expensive depending on where the data has to
	// be retrieved from.
	status_t status = fault_get_page(map, topCache, cacheOffset, isWrite,
		dummyPage, &pageSource, &copiedPageSource, &page);

	if (status == B_OK) {
		// All went fine, all there is left to do is to map the page into the address space

		// In case this is a copy-on-write page, we need to unmap it from the area now
		if (isWrite && page->cache == topCache)
			vm_unmap_pages(area, address, B_PAGE_SIZE, true);

		// TODO: there is currently no mechanism to prevent a page being mapped
		//	more than once in case of a second page fault!

		// If the page doesn't reside in the area's cache, we need to make sure it's
		// mapped in read-only, so that we cannot overwrite someone else's data (copy-on-write)
		uint32 newProtection = area->protection;
		if (page->cache != topCache && !isWrite)
			newProtection &= ~(B_WRITE_AREA | B_KERNEL_WRITE_AREA);

		vm_map_page(area, page, address, newProtection);

		mutex_unlock(&pageSource->lock);
		vm_cache_release_ref(pageSource);
	}

	atomic_add(&area->no_cache_change, -1);

	if (copiedPageSource)
		vm_cache_release_ref(copiedPageSource);

	if (dummyPage.state == PAGE_STATE_BUSY) {
		// We still have the dummy page in the cache - that happens if we didn't need
		// to allocate a new page before, but could use one in another cache
		fault_remove_dummy_page(dummyPage, false);
	}

	vm_cache_release_ref(topCache);
	vm_page_unreserve_pages(reservePages);

	return status;
}


/*! You must have the address space's sem held */
vm_area *
vm_area_lookup(vm_address_space *addressSpace, addr_t address)
{
	vm_area *area;

	// check the areas list first
	area = addressSpace->area_hint;
	if (area && area->base <= address && area->base + (area->size - 1) >= address)
		goto found;

	for (area = addressSpace->areas; area != NULL; area = area->address_space_next) {
		if (area->id == RESERVED_AREA_ID)
			continue;

		if (area->base <= address && area->base + (area->size - 1) >= address)
			break;
	}

found:
	if (area)
		addressSpace->area_hint = area;

	return area;
}


status_t
vm_get_physical_page(addr_t paddr, addr_t *_vaddr, uint32 flags)
{
	return (*vm_kernel_address_space()->translation_map.ops->get_physical_page)(paddr, _vaddr, flags);
}


off_t
vm_available_memory(void)
{
	return sAvailableMemory;
}


status_t
vm_put_physical_page(addr_t vaddr)
{
	return (*vm_kernel_address_space()->translation_map.ops->put_physical_page)(vaddr);
}


void
vm_unreserve_memory(size_t amount)
{
	benaphore_lock(&sAvailableMemoryLock);

	sAvailableMemory += amount;

	benaphore_unlock(&sAvailableMemoryLock);
}


status_t
vm_try_reserve_memory(size_t amount)
{
	status_t status;
	benaphore_lock(&sAvailableMemoryLock);

	//dprintf("try to reserve %lu bytes, %Lu left\n", amount, sAvailableMemory);

	if (sAvailableMemory > amount) {
		sAvailableMemory -= amount;
		status = B_OK;
	} else
		status = B_NO_MEMORY;

	benaphore_unlock(&sAvailableMemoryLock);
	return status;
}


status_t
vm_set_area_memory_type(area_id id, addr_t physicalBase, uint32 type)
{
	AddressSpaceReadLocker locker;
	vm_area *area;
	status_t status = locker.SetFromArea(id, area);
	if (status != B_OK)
		return status;

	return arch_vm_set_memory_type(area, physicalBase, type);
}


/**	This function enforces some protection properties:
 *	 - if B_WRITE_AREA is set, B_WRITE_KERNEL_AREA is set as well
 *	 - if only B_READ_AREA has been set, B_KERNEL_READ_AREA is also set
 *	 - if no protection is specified, it defaults to B_KERNEL_READ_AREA
 *	   and B_KERNEL_WRITE_AREA.
 */

static void
fix_protection(uint32 *protection)
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
fill_area_info(struct vm_area *area, area_info *info, size_t size)
{
	strlcpy(info->name, area->name, B_OS_NAME_LENGTH);
	info->area = area->id;
	info->address = (void *)area->base;
	info->size = area->size;
	info->protection = area->protection;
	info->lock = B_FULL_LOCK;
	info->team = area->address_space->id;
	info->copy_count = 0;
	info->in_count = 0;
	info->out_count = 0;
		// ToDo: retrieve real values here!

	vm_cache *cache = vm_area_get_locked_cache(area);

	// Note, this is a simplification; the cache could be larger than this area
	info->ram_size = cache->page_count * B_PAGE_SIZE;

	vm_area_put_locked_cache(cache);
}


/*!
	Tests wether or not the area that contains the specified address
	needs any kind of locking, and actually exists.
	Used by both lock_memory() and unlock_memory().
*/
static status_t
test_lock_memory(vm_address_space *addressSpace, addr_t address,
	bool &needsLocking)
{
	acquire_sem_etc(addressSpace->sem, READ_COUNT, 0, 0);

	vm_area *area = vm_area_lookup(addressSpace, address);
	if (area != NULL) {
		// This determines if we need to lock the memory at all
		needsLocking = area->cache_type != CACHE_TYPE_NULL
			&& area->cache_type != CACHE_TYPE_DEVICE
			&& area->wiring != B_FULL_LOCK
			&& area->wiring != B_CONTIGUOUS;
	}

	release_sem_etc(addressSpace->sem, READ_COUNT, 0);

	if (area == NULL)
		return B_BAD_ADDRESS;

	return B_OK;
}


//	#pragma mark - kernel public API


status_t
user_memcpy(void *to, const void *from, size_t size)
{
	if (arch_cpu_user_memcpy(to, from, size, &thread_get_current_thread()->fault_handler) < B_OK)
		return B_BAD_ADDRESS;
	return B_OK;
}


/**	\brief Copies at most (\a size - 1) characters from the string in \a from to
 *	the string in \a to, NULL-terminating the result.
 *
 *	\param to Pointer to the destination C-string.
 *	\param from Pointer to the source C-string.
 *	\param size Size in bytes of the string buffer pointed to by \a to.
 *
 *	\return strlen(\a from).
 */

ssize_t
user_strlcpy(char *to, const char *from, size_t size)
{
	return arch_cpu_user_strlcpy(to, from, size, &thread_get_current_thread()->fault_handler);
}


status_t
user_memset(void *s, char c, size_t count)
{
	if (arch_cpu_user_memset(s, c, count, &thread_get_current_thread()->fault_handler) < B_OK)
		return B_BAD_ADDRESS;
	return B_OK;
}


long
lock_memory(void *address, ulong numBytes, ulong flags)
{
	vm_address_space *addressSpace = NULL;
	struct vm_translation_map *map;
	addr_t unalignedBase = (addr_t)address;
	addr_t end = unalignedBase + numBytes;
	addr_t base = ROUNDOWN(unalignedBase, B_PAGE_SIZE);
	bool isUser = IS_USER_ADDRESS(address);
	bool needsLocking = true;

	if (isUser)
		addressSpace = vm_get_current_user_address_space();
	else
		addressSpace = vm_get_kernel_address_space();
	if (addressSpace == NULL)
		return B_ERROR;

	// test if we're on an area that allows faults at all

	map = &addressSpace->translation_map;

	status_t status = test_lock_memory(addressSpace, base, needsLocking);
	if (status < B_OK)
		goto out;
	if (!needsLocking)
		goto out;

	for (; base < end; base += B_PAGE_SIZE) {
		addr_t physicalAddress;
		uint32 protection;
		status_t status;

		map->ops->lock(map);
		status = map->ops->query(map, base, &physicalAddress, &protection);
		map->ops->unlock(map);

		if (status < B_OK)
			goto out;

		if ((protection & PAGE_PRESENT) != 0) {
			// if B_READ_DEVICE is set, the caller intents to write to the locked
			// memory, so if it hasn't been mapped writable, we'll try the soft
			// fault anyway
			if ((flags & B_READ_DEVICE) == 0
				|| (protection & (B_WRITE_AREA | B_KERNEL_WRITE_AREA)) != 0) {
				// update wiring
				vm_page *page = vm_lookup_page(physicalAddress / B_PAGE_SIZE);
				if (page == NULL)
					panic("couldn't lookup physical page just allocated\n");

				page->wired_count++;
					// TODO: needs to be atomic on all platforms!
				continue;
			}
		}

		status = vm_soft_fault(base, (flags & B_READ_DEVICE) != 0, isUser);
		if (status != B_OK)	{
			dprintf("lock_memory(address = %p, numBytes = %lu, flags = %lu) failed: %s\n",
				(void *)unalignedBase, numBytes, flags, strerror(status));
			goto out;
		}

		map->ops->lock(map);
		status = map->ops->query(map, base, &physicalAddress, &protection);
		map->ops->unlock(map);

		if (status < B_OK)
			goto out;

		// update wiring
		vm_page *page = vm_lookup_page(physicalAddress / B_PAGE_SIZE);
		if (page == NULL)
			panic("couldn't lookup physical page");

		page->wired_count++;
			// TODO: needs to be atomic on all platforms!
	}

out:
	vm_put_address_space(addressSpace);
	return status;
}


long
unlock_memory(void *address, ulong numBytes, ulong flags)
{
	vm_address_space *addressSpace = NULL;
	struct vm_translation_map *map;
	addr_t unalignedBase = (addr_t)address;
	addr_t end = unalignedBase + numBytes;
	addr_t base = ROUNDOWN(unalignedBase, B_PAGE_SIZE);
	bool needsLocking = true;

	if (IS_USER_ADDRESS(address))
		addressSpace = vm_get_current_user_address_space();
	else
		addressSpace = vm_get_kernel_address_space();
	if (addressSpace == NULL)
		return B_ERROR;

	map = &addressSpace->translation_map;

	status_t status = test_lock_memory(addressSpace, base, needsLocking);
	if (status < B_OK)
		goto out;
	if (!needsLocking)
		goto out;

	for (; base < end; base += B_PAGE_SIZE) {
		map->ops->lock(map);

		addr_t physicalAddress;
		uint32 protection;
		status = map->ops->query(map, base, &physicalAddress,
			&protection);

		map->ops->unlock(map);

		if (status < B_OK)
			goto out;
		if ((protection & PAGE_PRESENT) == 0)
			panic("calling unlock_memory() on unmapped memory!");

		// update wiring
		vm_page *page = vm_lookup_page(physicalAddress / B_PAGE_SIZE);
		if (page == NULL)
			panic("couldn't lookup physical page");

		page->wired_count--;
			// TODO: needs to be atomic on all platforms!
	}

out:
	vm_put_address_space(addressSpace);
	return status;
}


/** According to the BeBook, this function should always succeed.
 *	This is no longer the case.
 */

long
get_memory_map(const void *address, ulong numBytes, physical_entry *table,
	long numEntries)
{
	vm_address_space *addressSpace;
	addr_t virtualAddress = (addr_t)address;
	addr_t pageOffset = virtualAddress & (B_PAGE_SIZE - 1);
	addr_t physicalAddress;
	status_t status = B_OK;
	int32 index = -1;
	addr_t offset = 0;
	bool interrupts = are_interrupts_enabled();

	TRACE(("get_memory_map(%p, %lu bytes, %ld entries)\n", address, numBytes,
		numEntries));

	if (numEntries == 0 || numBytes == 0)
		return B_BAD_VALUE;

	// in which address space is the address to be found?
	if (IS_USER_ADDRESS(virtualAddress))
		addressSpace = thread_get_current_thread()->team->address_space;
	else
		addressSpace = vm_kernel_address_space();

	if (addressSpace == NULL)
		return B_ERROR;

	vm_translation_map *map = &addressSpace->translation_map;

	if (interrupts)
		map->ops->lock(map);

	while (offset < numBytes) {
		addr_t bytes = min_c(numBytes - offset, B_PAGE_SIZE);
		uint32 flags;

		if (interrupts) {
			status = map->ops->query(map, (addr_t)address + offset,
				&physicalAddress, &flags);
		} else {
			status = map->ops->query_interrupt(map, (addr_t)address + offset,
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
		if (index < 0 || (addr_t)table[index].address
				!= physicalAddress - table[index].size) {
			if (++index + 1 > numEntries) {
				// table to small
				status = B_BUFFER_OVERFLOW;
				break;
			}
			table[index].address = (void *)physicalAddress;
			table[index].size = bytes;
		} else {
			// page does fit in current entry
			table[index].size += bytes;
		}

		offset += bytes;
	}

	if (interrupts)
		map->ops->unlock(map);

	// close the entry list

	if (status == B_OK) {
		// if it's only one entry, we will silently accept the missing ending
		if (numEntries == 1)
			return B_OK;

		if (++index + 1 > numEntries)
			return B_BUFFER_OVERFLOW;

		table[index].address = NULL;
		table[index].size = 0;
	}

	return status;
}


area_id
area_for(void *address)
{
	team_id space;

	if (IS_USER_ADDRESS(address)) {
		// we try the user team address space, if any
		space = vm_current_user_address_space_id();
		if (space < B_OK)
			return space;
	} else
		space = vm_kernel_address_space_id();

	return vm_area_for(space, (addr_t)address);
}


area_id
find_area(const char *name)
{
	acquire_sem_etc(sAreaHashLock, READ_COUNT, 0, 0);
	struct hash_iterator iterator;
	hash_open(sAreaHash, &iterator);

	vm_area *area;
	area_id id = B_NAME_NOT_FOUND;
	while ((area = (vm_area *)hash_next(sAreaHash, &iterator)) != NULL) {
		if (area->id == RESERVED_AREA_ID)
			continue;

		if (!strcmp(area->name, name)) {
			id = area->id;
			break;
		}
	}

	hash_close(sAreaHash, &iterator, false);
	release_sem_etc(sAreaHashLock, READ_COUNT, 0);

	return id;
}


status_t
_get_area_info(area_id id, area_info *info, size_t size)
{
	if (size != sizeof(area_info) || info == NULL)
		return B_BAD_VALUE;

	AddressSpaceReadLocker locker;
	vm_area *area;
	status_t status = locker.SetFromArea(id, area);
	if (status != B_OK)
		return status;

	fill_area_info(area, info, size);
	return B_OK;
}


status_t
_get_next_area_info(team_id team, int32 *cookie, area_info *info, size_t size)
{
	addr_t nextBase = *(addr_t *)cookie;

	// we're already through the list
	if (nextBase == (addr_t)-1)
		return B_ENTRY_NOT_FOUND;

	if (team == B_CURRENT_TEAM)
		team = team_get_current_team_id();

	AddressSpaceReadLocker locker(team);
	if (!locker.IsLocked())
		return B_BAD_TEAM_ID;

	vm_area *area;
	for (area = locker.AddressSpace()->areas; area != NULL;
			area = area->address_space_next) {
		if (area->id == RESERVED_AREA_ID)
			continue;

		if (area->base > nextBase)
			break;
	}

	if (area == NULL) {
		nextBase = (addr_t)-1;
		return B_ENTRY_NOT_FOUND;
	}

	fill_area_info(area, info, size);
	*cookie = (int32)(area->base);

	return B_OK;
}


status_t
set_area_protection(area_id area, uint32 newProtection)
{
	fix_protection(&newProtection);

	return vm_set_area_protection(vm_kernel_address_space_id(), area,
		newProtection);
}


status_t
resize_area(area_id areaID, size_t newSize)
{
	// is newSize a multiple of B_PAGE_SIZE?
	if (newSize & (B_PAGE_SIZE - 1))
		return B_BAD_VALUE;

	// lock all affected address spaces and the cache
	vm_area* area;
	vm_cache* cache;

	MultiAddressSpaceLocker locker;
	status_t status = locker.AddAreaCacheAndLock(areaID, true, true, area,
		&cache);
	if (status != B_OK)
		return status;
	AreaCacheLocker cacheLocker(cache);	// already locked

	size_t oldSize = area->size;
	if (newSize == oldSize)
		return B_OK;

	// Resize all areas of this area's cache

	if (cache->type != CACHE_TYPE_RAM)
		return B_NOT_ALLOWED;

	if (oldSize < newSize) {
		// We need to check if all areas of this cache can be resized

		for (vm_area* current = cache->areas; current != NULL;
				current = current->cache_next) {
			if (current->address_space_next
				&& current->address_space_next->base <= (current->base
					+ newSize)) {
				// If the area was created inside a reserved area, it can
				// also be resized in that area
				// ToDo: if there is free space after the reserved area, it could be used as well...
				vm_area *next = current->address_space_next;
				if (next->id == RESERVED_AREA_ID
					&& next->cache_offset <= current->base
					&& next->base - 1 + next->size >= current->base - 1 + newSize)
					continue;

				return B_ERROR;
			}
		}
	}

	// Okay, looks good so far, so let's do it

	for (vm_area* current = cache->areas; current != NULL;
			current = current->cache_next) {
		if (current->address_space_next
			&& current->address_space_next->base <= (current->base + newSize)) {
			vm_area *next = current->address_space_next;
			if (next->id == RESERVED_AREA_ID
				&& next->cache_offset <= current->base
				&& next->base - 1 + next->size >= current->base - 1 + newSize) {
				// resize reserved area
				addr_t offset = current->base + newSize - next->base;
				if (next->size <= offset) {
					current->address_space_next = next->address_space_next;
					free(next);
				} else {
					next->size -= offset;
					next->base += offset;
				}
			} else {
				status = B_ERROR;
				break;
			}
		}

		current->size = newSize;

		// we also need to unmap all pages beyond the new size, if the area has shrinked
		if (newSize < oldSize) {
			vm_unmap_pages(current, current->base + newSize, oldSize - newSize,
				false);
		}
	}

	if (status == B_OK)
		status = vm_cache_resize(cache, newSize);

	if (status < B_OK) {
		// This shouldn't really be possible, but hey, who knows
		for (vm_area* current = cache->areas; current != NULL;
				current = current->cache_next) {
			current->size = oldSize;
		}
	}

	// ToDo: we must honour the lock restrictions of this area
	return status;
}


/**	Transfers the specified area to a new team. The caller must be the owner
 *	of the area (not yet enforced but probably should be).
 *	This function is currently not exported to the kernel namespace, but is
 *	only accessible using the _kern_transfer_area() syscall.
 */

static area_id
transfer_area(area_id id, void **_address, uint32 addressSpec, team_id target)
{
	area_info info;
	status_t status = get_area_info(id, &info);
	if (status < B_OK)
		return status;

	area_id clonedArea = vm_clone_area(target, info.name, _address,
		addressSpec, info.protection, REGION_NO_PRIVATE_MAP, id);
	if (clonedArea < B_OK)
		return clonedArea;

	status = vm_delete_area(info.team, id);
	if (status < B_OK) {
		vm_delete_area(target, clonedArea);
		return status;
	}

	return clonedArea;
}


area_id
map_physical_memory(const char *name, void *physicalAddress, size_t numBytes,
	uint32 addressSpec, uint32 protection, void **_virtualAddress)
{
	if (!arch_vm_supports_protection(protection))
		return B_NOT_SUPPORTED;

	fix_protection(&protection);

	return vm_map_physical_memory(vm_kernel_address_space_id(), name, _virtualAddress,
		addressSpec, numBytes, protection, (addr_t)physicalAddress);
}


area_id
clone_area(const char *name, void **_address, uint32 addressSpec,
	uint32 protection, area_id source)
{
	if ((protection & B_KERNEL_PROTECTION) == 0)
		protection |= B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA;

	return vm_clone_area(vm_kernel_address_space_id(), name, _address,
		addressSpec, protection, REGION_NO_PRIVATE_MAP, source);
}


area_id
create_area_etc(struct team *team, const char *name, void **address, uint32 addressSpec,
	uint32 size, uint32 lock, uint32 protection)
{
	fix_protection(&protection);

	return vm_create_anonymous_area(team->id, (char *)name, address,
		addressSpec, size, lock, protection, false);
}


area_id
create_area(const char *name, void **_address, uint32 addressSpec, size_t size, uint32 lock,
	uint32 protection)
{
	fix_protection(&protection);

	return vm_create_anonymous_area(vm_kernel_address_space_id(), (char *)name, _address,
		addressSpec, size, lock, protection, false);
}


status_t
delete_area_etc(struct team *team, area_id area)
{
	return vm_delete_area(team->id, area);
}


status_t
delete_area(area_id area)
{
	return vm_delete_area(vm_kernel_address_space_id(), area);
}


//	#pragma mark - Userland syscalls


status_t
_user_reserve_heap_address_range(addr_t* userAddress, uint32 addressSpec, addr_t size)
{
	// filter out some unavailable values (for userland)
	switch (addressSpec) {
		case B_ANY_KERNEL_ADDRESS:
		case B_ANY_KERNEL_BLOCK_ADDRESS:
			return B_BAD_VALUE;
	}

	addr_t address;

	if (!IS_USER_ADDRESS(userAddress)
		|| user_memcpy(&address, userAddress, sizeof(address)) < B_OK)
		return B_BAD_ADDRESS;

	status_t status = vm_reserve_address_range(vm_current_user_address_space_id(),
		(void **)&address, addressSpec, size, RESERVED_AVOID_BASE);
	if (status < B_OK)
		return status;

	if (user_memcpy(userAddress, &address, sizeof(address)) < B_OK) {
		vm_unreserve_address_range(vm_current_user_address_space_id(),
			(void *)address, size);
		return B_BAD_ADDRESS;
	}

	return B_OK;
}


area_id
_user_area_for(void *address)
{
	return vm_area_for(vm_current_user_address_space_id(), (addr_t)address);
}


area_id
_user_find_area(const char *userName)
{
	char name[B_OS_NAME_LENGTH];

	if (!IS_USER_ADDRESS(userName)
		|| user_strlcpy(name, userName, B_OS_NAME_LENGTH) < B_OK)
		return B_BAD_ADDRESS;

	return find_area(name);
}


status_t
_user_get_area_info(area_id area, area_info *userInfo)
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
_user_get_next_area_info(team_id team, int32 *userCookie, area_info *userInfo)
{
	int32 cookie;

	if (!IS_USER_ADDRESS(userCookie)
		|| !IS_USER_ADDRESS(userInfo)
		|| user_memcpy(&cookie, userCookie, sizeof(int32)) < B_OK)
		return B_BAD_ADDRESS;

	area_info info;
	status_t status = _get_next_area_info(team, &cookie, &info, sizeof(area_info));
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

	return vm_set_area_protection(vm_current_user_address_space_id(), area,
		newProtection);
}


status_t
_user_resize_area(area_id area, size_t newSize)
{
	// ToDo: Since we restrict deleting of areas to those owned by the team,
	// we should also do that for resizing (check other functions, too).
	return resize_area(area, newSize);
}


area_id
_user_transfer_area(area_id area, void **userAddress, uint32 addressSpec, team_id target)
{
	// filter out some unavailable values (for userland)
	switch (addressSpec) {
		case B_ANY_KERNEL_ADDRESS:
		case B_ANY_KERNEL_BLOCK_ADDRESS:
			return B_BAD_VALUE;
	}

	void *address;
	if (!IS_USER_ADDRESS(userAddress)
		|| user_memcpy(&address, userAddress, sizeof(address)) < B_OK)
		return B_BAD_ADDRESS;

	area_id newArea = transfer_area(area, &address, addressSpec, target);
	if (newArea < B_OK)
		return newArea;

	if (user_memcpy(userAddress, &address, sizeof(address)) < B_OK)
		return B_BAD_ADDRESS;

	return newArea;
}


area_id
_user_clone_area(const char *userName, void **userAddress, uint32 addressSpec,
	uint32 protection, area_id sourceArea)
{
	char name[B_OS_NAME_LENGTH];
	void *address;

	// filter out some unavailable values (for userland)
	switch (addressSpec) {
		case B_ANY_KERNEL_ADDRESS:
		case B_ANY_KERNEL_BLOCK_ADDRESS:
			return B_BAD_VALUE;
	}
	if ((protection & ~B_USER_PROTECTION) != 0)
		return B_BAD_VALUE;

	if (!IS_USER_ADDRESS(userName)
		|| !IS_USER_ADDRESS(userAddress)
		|| user_strlcpy(name, userName, sizeof(name)) < B_OK
		|| user_memcpy(&address, userAddress, sizeof(address)) < B_OK)
		return B_BAD_ADDRESS;

	fix_protection(&protection);

	area_id clonedArea = vm_clone_area(vm_current_user_address_space_id(), name, &address,
		addressSpec, protection, REGION_NO_PRIVATE_MAP, sourceArea);
	if (clonedArea < B_OK)
		return clonedArea;

	if (user_memcpy(userAddress, &address, sizeof(address)) < B_OK) {
		delete_area(clonedArea);
		return B_BAD_ADDRESS;
	}

	return clonedArea;
}


area_id
_user_create_area(const char *userName, void **userAddress, uint32 addressSpec,
	size_t size, uint32 lock, uint32 protection)
{
	char name[B_OS_NAME_LENGTH];
	void *address;

	// filter out some unavailable values (for userland)
	switch (addressSpec) {
		case B_ANY_KERNEL_ADDRESS:
		case B_ANY_KERNEL_BLOCK_ADDRESS:
			return B_BAD_VALUE;
	}
	if ((protection & ~B_USER_PROTECTION) != 0)
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

	area_id area = vm_create_anonymous_area(vm_current_user_address_space_id(),
		(char *)name, &address, addressSpec, size, lock, protection, false);

	if (area >= B_OK && user_memcpy(userAddress, &address, sizeof(address)) < B_OK) {
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
	// The documentation to delete_area() explicetly states that this
	// will be restricted in the future, and so it will.
	return vm_delete_area(vm_current_user_address_space_id(), area);
}


// ToDo: create a BeOS style call for this!

area_id
_user_map_file(const char *userName, void **userAddress, int addressSpec,
	addr_t size, int protection, int mapping, int fd, off_t offset)
{
	char name[B_OS_NAME_LENGTH];
	void *address;
	area_id area;

	if (!IS_USER_ADDRESS(userName) || !IS_USER_ADDRESS(userAddress)
		|| user_strlcpy(name, userName, B_OS_NAME_LENGTH) < B_OK
		|| user_memcpy(&address, userAddress, sizeof(address)) < B_OK)
		return B_BAD_ADDRESS;

	if (addressSpec == B_EXACT_ADDRESS) {
		if ((addr_t)address + size < (addr_t)address)
			return B_BAD_VALUE;
		if (!IS_USER_ADDRESS(address)
				|| !IS_USER_ADDRESS((addr_t)address + size)) {
			return B_BAD_ADDRESS;
		}
	}

	// userland created areas can always be accessed by the kernel
	protection |= B_KERNEL_READ_AREA
		| (protection & B_WRITE_AREA ? B_KERNEL_WRITE_AREA : 0);

	area = _vm_map_file(vm_current_user_address_space_id(), name, &address,
		addressSpec, size, protection, mapping, fd, offset, false);
	if (area < B_OK)
		return area;

	if (user_memcpy(userAddress, &address, sizeof(address)) < B_OK)
		return B_BAD_ADDRESS;

	return area;
}


status_t
_user_unmap_memory(void *_address, addr_t size)
{
	addr_t address = (addr_t)_address;

	// check params
	if (size == 0 || (addr_t)address + size < (addr_t)address)
		return B_BAD_VALUE;

	if (!IS_USER_ADDRESS(address) || !IS_USER_ADDRESS((addr_t)address + size))
		return B_BAD_ADDRESS;

	// write lock the address space
	AddressSpaceWriteLocker locker;
	status_t status = locker.SetTo(team_get_current_team_id());
	if (status != B_OK)
		return status;

	// unmap
	return unmap_address_range(locker.AddressSpace(), address, size);
}
