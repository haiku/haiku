/*
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include "vm_store_anonymous_noswap.h"
#include "vm_store_device.h"
#include "vm_store_null.h"

#include <OS.h>
#include <KernelExport.h>

#include <vm.h>
#include <vm_address_space.h>
#include <vm_priv.h>
#include <vm_page.h>
#include <vm_cache.h>
#include <vm_low_memory.h>
#include <file_cache.h>
#include <memheap.h>
#include <debug.h>
#include <console.h>
#include <int.h>
#include <smp.h>
#include <lock.h>
#include <thread.h>
#include <team.h>

#include <boot/stage2.h>
#include <boot/elf.h>

#include <arch/cpu.h>
#include <arch/vm.h>

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

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


extern vm_address_space *kernel_aspace;

#define REGION_HASH_TABLE_SIZE 1024
static area_id sNextAreaID;
static hash_table *sAreaHash;
static sem_id sAreaHashLock;

static off_t sAvailableMemory;
static benaphore sAvailableMemoryLock;

// function declarations
static status_t vm_soft_fault(addr_t address, bool is_write, bool is_user);
static bool vm_put_area(vm_area *area);


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


static vm_area *
vm_get_area(area_id id)
{
	vm_area *area;

	acquire_sem_etc(sAreaHashLock, READ_COUNT, 0, 0);

	area = (vm_area *)hash_lookup(sAreaHash, &id);
	if (area != NULL)
		atomic_add(&area->ref_count, 1);

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
	vm_area *area = NULL;

	// restrict the area name to B_OS_NAME_LENGTH
	size_t length = strlen(name) + 1;
	if (length > B_OS_NAME_LENGTH)
		length = B_OS_NAME_LENGTH;

	area = (vm_area *)malloc(sizeof(vm_area));
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
	area->ref_count = 1;

	area->cache_ref = NULL;
	area->cache_offset = 0;

	area->address_space = addressSpace;
	area->address_space_next = NULL;
	area->cache_next = area->cache_prev = NULL;
	area->hash_next = NULL;

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

		// there was no reserved area, and the slot doesn't seem to be used already
		// ToDo: this could be further optimized.
	}

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
				if (!next || (next->base >= addressSpace->base + size)) {
					foundSpot = true;
					area->base = addressSpace->base;
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
				area->base = last->base + last->size;
				break;
			} else {
				// we didn't find a free spot - if there were any reserved areas with
				// the RESERVED_AVOID_BASE flag set, we can now test those for free
				// space
				// ToDo: it would make sense to start with the biggest of them
				next = addressSpace->areas;
				last = NULL;
				for (last = NULL; next; next = next->address_space_next, last = next) {
					// ToDo: take free space after the reserved area into account!
					if (next->size == size) {
						// the reserved area is entirely covered, and thus, removed
						if (last)
							last->address_space_next = next->address_space_next;
						else
							addressSpace->areas = next->address_space_next;

						foundSpot = true;
						area->base = next->base;
						free(next);
						break;
					}
					if (next->size >= size) {
						// the new area will be placed at the end of the reserved
						// area, and the reserved area will be resized to make space
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


static status_t
map_backing_store(vm_address_space *addressSpace, vm_cache_ref *cacheRef,
	void **_virtualAddress, off_t offset, addr_t size, uint32 addressSpec,
	int wiring, int protection, int mapping, vm_area **_area, const char *areaName)
{
	TRACE(("map_backing_store: aspace %p, cacheref %p, *vaddr %p, offset 0x%Lx, size %lu, addressSpec %ld, wiring %d, protection %d, _area %p, area_name '%s'\n",
		addressSpace, cacheRef, *_virtualAddress, offset, size, addressSpec,
		wiring, protection, _area, areaName));

	vm_area *area = create_area_struct(addressSpace, areaName, wiring, protection);
	if (area == NULL)
		return B_NO_MEMORY;

	mutex_lock(&cacheRef->lock);

	vm_cache *cache = cacheRef->cache;
	vm_store *store = cache->store;
	bool unlock = true;
	status_t status;

	// if this is a private map, we need to create a new cache & store object
	// pair to handle the private copies of pages as they are written to
	if (mapping == REGION_PRIVATE_MAP) {
		vm_cache_ref *newCacheRef;
		vm_cache *newCache;
		vm_store *newStore;

		// create an anonymous store object
		newStore = vm_store_create_anonymous_noswap((protection & B_STACK_AREA) != 0,
			0, USER_STACK_GUARD_PAGES);
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
		status = vm_cache_ref_create(newCache);
		if (status < B_OK) {
			newStore->ops->destroy(newStore);
			free(newCache);
			goto err1;
		}

		newCacheRef = newCache->ref;
		newCache->temporary = 1;
		newCache->scan_skip = cache->scan_skip;

		vm_cache_add_consumer_locked(cacheRef, newCache);

		mutex_unlock(&cacheRef->lock);
		mutex_lock(&newCacheRef->lock);

		cache = newCache;
		cacheRef = newCache->ref;
		store = newStore;
		cache->virtual_base = offset;
		cache->virtual_size = offset + size;
	}

	status = vm_cache_set_minimal_commitment_locked(cacheRef, offset + size);
	if (status != B_OK)
		goto err2;

	acquire_sem_etc(addressSpace->sem, WRITE_COUNT, 0, 0);

	// check to see if this address space has entered DELETE state
	if (addressSpace->state == VM_ASPACE_STATE_DELETION) {
		// okay, someone is trying to delete this address space now, so we can't
		// insert the area, so back out
		status = B_BAD_TEAM_ID;
		goto err3;
	}

	status = insert_area(addressSpace, _virtualAddress, addressSpec, size, area);
	if (status < B_OK)
		goto err3;

	// attach the cache to the area
	area->cache_ref = cacheRef;
	area->cache_offset = offset;

	// point the cache back to the area
	vm_cache_insert_area_locked(cacheRef, area);
	mutex_unlock(&cacheRef->lock);

	// insert the area in the global area hash table
	acquire_sem_etc(sAreaHashLock, WRITE_COUNT, 0 ,0);
	hash_insert(sAreaHash, area);
	release_sem_etc(sAreaHashLock, WRITE_COUNT, 0);

	// grab a ref to the address space (the area holds this)
	atomic_add(&addressSpace->ref_count, 1);

	release_sem_etc(addressSpace->sem, WRITE_COUNT, 0);

	*_area = area;
	return B_OK;

err3:
	release_sem_etc(addressSpace->sem, WRITE_COUNT, 0);
err2:
	if (mapping == REGION_PRIVATE_MAP) {
		// we created this cache, so we must delete it again
		mutex_unlock(&cacheRef->lock);
		vm_cache_release_ref(cacheRef);
		unlock = false;
	}
err1:
	if (unlock)
		mutex_unlock(&cacheRef->lock);
	free(area->name);
	free(area);
	return status;
}


status_t
vm_unreserve_address_range(team_id team, void *address, addr_t size)
{
	vm_address_space *addressSpace;
	vm_area *area, *last = NULL;
	status_t status = B_OK;

	addressSpace = vm_get_address_space_by_id(team);
	if (addressSpace == NULL)
		return B_BAD_TEAM_ID;

	acquire_sem_etc(addressSpace->sem, WRITE_COUNT, 0, 0);

	// check to see if this address space has entered DELETE state
	if (addressSpace->state == VM_ASPACE_STATE_DELETION) {
		// okay, someone is trying to delete this address space now, so we can't
		// insert the area, so back out
		status = B_BAD_TEAM_ID;
		goto out;
	}

	// search area list and remove any matching reserved ranges

	area = addressSpace->areas;
	while (area) {
		// the area must be completely part of the reserved range
		if (area->id == RESERVED_AREA_ID && area->base >= (addr_t)address
			&& area->base + area->size <= (addr_t)address + size) {
			// remove reserved range
			vm_area *reserved = area;
			if (last)
				last->address_space_next = reserved->address_space_next;
			else
				addressSpace->areas = reserved->address_space_next;

			area = reserved->address_space_next;
			free(reserved);
			continue;
		}

		last = area;
		area = area->address_space_next;
	}
	
out:
	release_sem_etc(addressSpace->sem, WRITE_COUNT, 0);
	vm_put_address_space(addressSpace);
	return status;
}


status_t
vm_reserve_address_range(team_id team, void **_address, uint32 addressSpec, 
	addr_t size, uint32 flags)
{
	vm_address_space *addressSpace;
	vm_area *area;
	status_t status = B_OK;

	if (size == 0)
		return B_BAD_VALUE;

	addressSpace = vm_get_address_space_by_id(team);
	if (addressSpace == NULL)
		return B_BAD_TEAM_ID;

	area = create_reserved_area_struct(addressSpace, flags);
	if (area == NULL) {
		status = B_NO_MEMORY;
		goto err1;
	}

	acquire_sem_etc(addressSpace->sem, WRITE_COUNT, 0, 0);

	// check to see if this address space has entered DELETE state
	if (addressSpace->state == VM_ASPACE_STATE_DELETION) {
		// okay, someone is trying to delete this address space now, so we can't
		// insert the area, let's back out
		status = B_BAD_TEAM_ID;
		goto err2;
	}

	status = insert_area(addressSpace, _address, addressSpec, size, area);
	if (status < B_OK)
		goto err2;

	// the area is now reserved!

	area->cache_offset = area->base;
		// we cache the original base address here

	release_sem_etc(addressSpace->sem, WRITE_COUNT, 0);
	return B_OK;

err2:
	release_sem_etc(addressSpace->sem, WRITE_COUNT, 0);
	free(area);
err1:
	vm_put_address_space(addressSpace);
	return status;
}


area_id
vm_create_anonymous_area(team_id aid, const char *name, void **address, 
	uint32 addressSpec, addr_t size, uint32 wiring, uint32 protection)
{
	vm_cache_ref *cacheRef;
	vm_area *area;
	vm_cache *cache;
	vm_store *store;
	vm_page *page = NULL;
	bool isStack = (protection & B_STACK_AREA) != 0;
	bool canOvercommit = false;
	status_t status;

	TRACE(("create_anonymous_area %s: size 0x%lx\n", name, size));

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

	vm_address_space *addressSpace = vm_get_address_space_by_id(aid);
	if (addressSpace == NULL)
		return B_BAD_TEAM_ID;

	size = PAGE_ALIGN(size);

	if (wiring == B_CONTIGUOUS) {
		// we try to allocate the page run here upfront as this may easily
		// fail for obvious reasons
		page = vm_page_allocate_page_run(PAGE_STATE_CLEAR, size / B_PAGE_SIZE);
		if (page == NULL) {
			vm_put_address_space(addressSpace);
			return B_NO_MEMORY;
		}
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
	status = vm_cache_ref_create(cache);
	if (status < B_OK)
		goto err3;

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

	cacheRef = cache->ref;

	status = map_backing_store(addressSpace, cacheRef, address, 0, size,
		addressSpec, wiring, protection, REGION_NO_PRIVATE_MAP, &area, name);
	if (status < B_OK) {
		vm_cache_release_ref(cacheRef);
		goto err1;
	}

	switch (wiring) {
		case B_NO_LOCK:
		case B_LAZY_LOCK:
			// do nothing - the pages are mapped in as needed
			break;

		case B_FULL_LOCK:
		{
			// Allocate and map all pages for this area
			mutex_lock(&cacheRef->lock);

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
				vm_page *page = vm_page_allocate_page(PAGE_STATE_CLEAR);
				if (page == NULL) {
					// this shouldn't really happen, as we reserve the memory upfront
					panic("couldn't fulfill B_FULL lock!");
				}

				vm_cache_insert_page(cacheRef, page, offset);
				vm_map_page(area, page, address, protection);
			}

			mutex_unlock(&cacheRef->lock);
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

			mutex_lock(&cacheRef->lock);
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
				vm_cache_insert_page(cacheRef, page, offset);
			}

			map->ops->unlock(map);
			mutex_unlock(&cacheRef->lock);
			break;
		}

		case B_CONTIGUOUS:
		{
			// We have already allocated our continuous pages run, so we can now just
			// map them in the address space
			vm_translation_map *map = &addressSpace->translation_map;
			addr_t physicalAddress = page->physical_page_number * B_PAGE_SIZE;
			addr_t virtualAddress;
			off_t offset = 0;

			mutex_lock(&cacheRef->lock);
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
				vm_cache_insert_page(cacheRef, page, offset);
			}

			map->ops->unlock(map);
			mutex_unlock(&cacheRef->lock);
			break;
		}

		default:
			break;
	}
	vm_put_address_space(addressSpace);

	TRACE(("vm_create_anonymous_area: done\n"));

	area->cache_type = CACHE_TYPE_RAM;
	return area->id;

err3:
	free(cache);
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

	vm_put_address_space(addressSpace);
	return status;	
}


area_id
vm_map_physical_memory(team_id aspaceID, const char *name, void **_address,
	uint32 addressSpec, addr_t size, uint32 protection, addr_t physicalAddress)
{
	vm_cache_ref *cacheRef;
	vm_area *area;
	vm_cache *cache;
	vm_store *store;
	addr_t mapOffset;
	status_t status;

	TRACE(("vm_map_physical_memory(aspace = %ld, \"%s\", virtual = %p, spec = %ld,"
		" size = %lu, protection = %ld, phys = %p)\n",
		aspaceID, name, _address, addressSpec, size, protection,
		(void *)physicalAddress));

	if (!arch_vm_supports_protection(protection))
		return B_NOT_SUPPORTED;

	vm_address_space *addressSpace = vm_get_address_space_by_id(aspaceID);
	if (addressSpace == NULL)
		return B_BAD_TEAM_ID;

	// if the physical address is somewhat inside a page,
	// move the actual area down to align on a page boundary
	mapOffset = physicalAddress % B_PAGE_SIZE;
	size += mapOffset;
	physicalAddress -= mapOffset;

	size = PAGE_ALIGN(size);

	// create an device store object

	store = vm_store_create_device(physicalAddress);
	if (store == NULL) {
		status = B_NO_MEMORY;
		goto err1;
	}
	cache = vm_cache_create(store);
	if (cache == NULL) {
		status = B_NO_MEMORY;
		goto err2;
	}
	status = vm_cache_ref_create(cache);
	if (status < B_OK)
		goto err3;

	// tell the page scanner to skip over this area, it's pages are special
	cache->scan_skip = 1;
	cache->type = CACHE_TYPE_DEVICE;
	cache->virtual_size = size;

	cacheRef = cache->ref;

	status = map_backing_store(addressSpace, cacheRef, _address, 0, size,
		addressSpec & ~B_MTR_MASK, B_FULL_LOCK, protection,
		REGION_NO_PRIVATE_MAP, &area, name);
	if (status < B_OK)
		vm_cache_release_ref(cacheRef);

	if (status >= B_OK && (addressSpec & B_MTR_MASK) != 0) {
		// set requested memory type
		status = arch_vm_set_memory_type(area, physicalAddress,
			addressSpec & B_MTR_MASK);
		if (status < B_OK)
			vm_put_area(area);
	}

	if (status >= B_OK) {
		// make sure our area is mapped in completely

		vm_translation_map *map = &addressSpace->translation_map;
		map->ops->lock(map);

		for (addr_t offset = 0; offset < size; offset += B_PAGE_SIZE) {
			map->ops->map(map, area->base + offset, physicalAddress + offset,
				protection);
		}

		map->ops->unlock(map);
	}

	vm_put_address_space(addressSpace);
	if (status < B_OK)
		return status;

	// modify the pointer returned to be offset back into the new area
	// the same way the physical address in was offset
	*_address = (void *)((addr_t)*_address + mapOffset);

	area->cache_type = CACHE_TYPE_DEVICE;
	return area->id;

err3:
	free(cache);
err2:
	store->ops->destroy(store);
err1:
	vm_put_address_space(addressSpace);
	return status;
}


area_id
vm_create_null_area(team_id team, const char *name, void **address,
	uint32 addressSpec, addr_t size)
{
	vm_area *area;
	vm_cache *cache;
	vm_cache_ref *cacheRef;
	vm_store *store;
	status_t status;

	vm_address_space *addressSpace = vm_get_address_space_by_id(team);
	if (addressSpace == NULL)
		return B_BAD_TEAM_ID;

	size = PAGE_ALIGN(size);

	// create an null store object

	store = vm_store_create_null();
	if (store == NULL) {
		status = B_NO_MEMORY;
		goto err1;
	}
	cache = vm_cache_create(store);
	if (cache == NULL) {
		status = B_NO_MEMORY;
		goto err2;
	}
	status = vm_cache_ref_create(cache);
	if (status < B_OK)
		goto err3;

	// tell the page scanner to skip over this area, no pages will be mapped here
	cache->scan_skip = 1;
	cache->type = CACHE_TYPE_NULL;
	cache->virtual_size = size;

	cacheRef = cache->ref;

	status = map_backing_store(addressSpace, cacheRef, address, 0, size, addressSpec, 0,
		B_KERNEL_READ_AREA, REGION_NO_PRIVATE_MAP, &area, name);

	vm_put_address_space(addressSpace);

	if (status < B_OK) {
		vm_cache_release_ref(cacheRef);
		return status;
	}

	area->cache_type = CACHE_TYPE_NULL;
	return area->id;

err3:
	free(cache);
err2:
	store->ops->destroy(store);
err1:
	vm_put_address_space(addressSpace);
	return status;
}


/**	Creates the vnode cache for the specified \a vnode.
 *	The vnode has to be marked busy when calling this function.
 *	If successful, it will also acquire an extra reference to
 *	the vnode (as the vnode store itself can't do this
 *	automatically).
 */

status_t
vm_create_vnode_cache(void *vnode, struct vm_cache_ref **_cacheRef)
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
	status = vm_cache_ref_create(cache);
	if (status < B_OK)
		goto err2;

	cache->type = CACHE_TYPE_VNODE;

	*_cacheRef = cache->ref;
	vfs_acquire_vnode(vnode);
	return B_OK;

err2:
	free(cache);
err1:
	store->ops->destroy(store);
	return status;
}


/** Will map the file at the path specified by \a name to an area in memory.
 *	The file will be mirrored beginning at the specified \a offset. The \a offset
 *	and \a size arguments have to be page aligned.
 */

static area_id
_vm_map_file(team_id team, const char *name, void **_address, uint32 addressSpec,
	size_t size, uint32 protection, uint32 mapping, const char *path,
	off_t offset, bool kernel)
{
	vm_cache_ref *cacheRef;
	vm_area *area;
	void *vnode;
	status_t status;

	// ToDo: maybe attach to an FD, not a path (or both, like VFS calls)
	// ToDo: check file access permissions (would be already done if the above were true)
	// ToDo: for binary files, we want to make sure that they get the
	//	copy of a file at a given time, ie. later changes should not
	//	make it into the mapped copy -- this will need quite some changes
	//	to be done in a nice way

	vm_address_space *addressSpace = vm_get_address_space_by_id(team);
	if (addressSpace == NULL)
		return B_BAD_TEAM_ID;

	TRACE(("_vm_map_file(\"%s\", offset = %Ld, size = %lu, mapping %ld)\n",
		path, offset, size, mapping));

	offset = ROUNDOWN(offset, B_PAGE_SIZE);
	size = PAGE_ALIGN(size);

	// get the vnode for the object, this also grabs a ref to it
	status = vfs_get_vnode_from_path(path, kernel, &vnode);
	if (status < B_OK)
		goto err1;

	// ToDo: this only works for file systems that use the file cache
	status = vfs_get_vnode_cache(vnode, &cacheRef, false);

	vfs_put_vnode(vnode);
		// we don't need this vnode anymore - if the above call was
		// successful, the store already has a ref to it

	if (status < B_OK)
		goto err1;

	status = map_backing_store(addressSpace, cacheRef, _address,
		offset, size, addressSpec, 0, protection, mapping, &area, name);
	if (status < B_OK || mapping == REGION_PRIVATE_MAP) {
		// map_backing_store() cannot know we no longer need the ref
		vm_cache_release_ref(cacheRef);
	}
	if (status < B_OK)
		goto err1;

	vm_put_address_space(addressSpace);
	area->cache_type = CACHE_TYPE_VNODE;
	return area->id;

err1:
	vm_put_address_space(addressSpace);
	return status;
}


area_id
vm_map_file(team_id aid, const char *name, void **address, uint32 addressSpec,
	addr_t size, uint32 protection, uint32 mapping, const char *path, off_t offset)
{
	if (!arch_vm_supports_protection(protection))
		return B_NOT_SUPPORTED;

	return _vm_map_file(aid, name, address, addressSpec, size, protection,
		mapping, path, offset, true);
}


// ToDo: create a BeOS style call for this!

area_id
_user_vm_map_file(const char *userName, void **userAddress, int addressSpec,
	addr_t size, int protection, int mapping, const char *userPath, off_t offset)
{
	char name[B_OS_NAME_LENGTH];
	char path[B_PATH_NAME_LENGTH];
	void *address;
	area_id area;

	if (!IS_USER_ADDRESS(userName) || !IS_USER_ADDRESS(userAddress)
		|| !IS_USER_ADDRESS(userPath)
		|| user_strlcpy(name, userName, B_OS_NAME_LENGTH) < B_OK
		|| user_strlcpy(path, userPath, B_PATH_NAME_LENGTH) < B_OK
		|| user_memcpy(&address, userAddress, sizeof(address)) < B_OK)
		return B_BAD_ADDRESS;

	// userland created areas can always be accessed by the kernel
	protection |= B_KERNEL_READ_AREA | (protection & B_WRITE_AREA ? B_KERNEL_WRITE_AREA : 0);

	area = _vm_map_file(vm_current_user_address_space_id(), name, &address,
		addressSpec, size, protection, mapping, path, offset, false);
	if (area < B_OK)
		return area;

	if (user_memcpy(userAddress, &address, sizeof(address)) < B_OK)
		return B_BAD_ADDRESS;

	return area;
}


area_id
vm_clone_area(team_id team, const char *name, void **address, uint32 addressSpec,
	uint32 protection, uint32 mapping, area_id sourceID)
{
	vm_area *newArea = NULL;
	vm_area *sourceArea;
	status_t status;

	vm_address_space *addressSpace = vm_get_address_space_by_id(team);
	if (addressSpace == NULL)
		return B_BAD_TEAM_ID;

	sourceArea = vm_get_area(sourceID);
	if (sourceArea == NULL) {
		vm_put_address_space(addressSpace);
		return B_BAD_VALUE;
	}

	vm_cache_acquire_ref(sourceArea->cache_ref);

	// ToDo: for now, B_USER_CLONEABLE is disabled, until all drivers
	//	have been adapted. Maybe it should be part of the kernel settings,
	//	anyway (so that old drivers can always work).
#if 0
	if (sourceArea->aspace == kernel_aspace && addressSpace != kernel_aspace
		&& !(sourceArea->protection & B_USER_CLONEABLE_AREA)) {
		// kernel areas must not be cloned in userland, unless explicitly
		// declared user-cloneable upon construction
		status = B_NOT_ALLOWED;
	} else
#endif
	if (sourceArea->cache_type == CACHE_TYPE_NULL)
		status = B_NOT_ALLOWED;
	else {
		status = map_backing_store(addressSpace, sourceArea->cache_ref,
			address, sourceArea->cache_offset, sourceArea->size, addressSpec,
			sourceArea->wiring, protection, mapping, &newArea, name);
	}
	if (status == B_OK && mapping != REGION_PRIVATE_MAP) {
		// If the mapping is REGION_PRIVATE_MAP, map_backing_store() needed
		// to create a new ref, and has therefore already acquired a reference
		// to the source cache - but otherwise it has no idea that we need
		// one.
		vm_cache_acquire_ref(sourceArea->cache_ref);
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

			map = &addressSpace->translation_map;
			map->ops->lock(map);

			for (addr_t offset = 0; offset < newArea->size;
					offset += B_PAGE_SIZE) {
				map->ops->map(map, newArea->base + offset,
					physicalAddress + offset, protection);
			}

			map->ops->unlock(map);
		} else {
			// map in all pages from source
			mutex_lock(&sourceArea->cache_ref->lock);

			for (vm_page *page = sourceArea->cache_ref->cache->page_list;
					page != NULL; page = page->cache_next) {
				vm_map_page(newArea, page, newArea->base
					+ ((page->cache_offset << PAGE_SHIFT) - newArea->cache_offset),
					protection);
			}

			mutex_unlock(&sourceArea->cache_ref->lock);
		}
	}
	if (status == B_OK)
		newArea->cache_type = sourceArea->cache_type;

	vm_cache_release_ref(sourceArea->cache_ref);

	vm_put_area(sourceArea);
	vm_put_address_space(addressSpace);

	if (status < B_OK)
		return status;

	return newArea->id;
}


static status_t
_vm_delete_area(vm_address_space *addressSpace, area_id id)
{
	status_t status = B_OK;
	vm_area *area;

	TRACE(("vm_delete_area: aspace id 0x%lx, area id 0x%lx\n", addressSpace->id, id));

	area = vm_get_area(id);
	if (area == NULL)
		return B_BAD_VALUE;

	if (area->address_space == addressSpace) {
		vm_put_area(area);
			// next put below will actually delete it
	} else
		status = B_NOT_ALLOWED;

	vm_put_area(area);
	return status;
}


status_t
vm_delete_area(team_id team, area_id id)
{
	vm_address_space *addressSpace;
	status_t err;

	addressSpace = vm_get_address_space_by_id(team);
	if (addressSpace == NULL)
		return B_BAD_TEAM_ID;

	err = _vm_delete_area(addressSpace, id);
	vm_put_address_space(addressSpace);
	return err;
}


static void
remove_area_from_address_space(vm_address_space *addressSpace, vm_area *area, bool locked)
{
	vm_area *temp, *last = NULL;

	if (!locked)
		acquire_sem_etc(addressSpace->sem, WRITE_COUNT, 0, 0);

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

	if (!locked)
		release_sem_etc(addressSpace->sem, WRITE_COUNT, 0);

	if (temp == NULL)
		panic("vm_area_release_ref: area not found in aspace's area list\n");
}


static bool
_vm_put_area(vm_area *area, bool aspaceLocked)
{
	vm_address_space *addressSpace;
	bool removeit = false;

	//TRACE(("_vm_put_area(area = %p, aspaceLocked = %s)\n",
	//	area, aspaceLocked ? "yes" : "no"));

	// we should never get here, but if we do, we can handle it
	if (area->id == RESERVED_AREA_ID)
		return false;

	addressSpace = area->address_space;

	// grab a write lock on the address space around the removal of the area
	// from the global hash table to avoid a race with vm_soft_fault()
	if (!aspaceLocked)
		acquire_sem_etc(addressSpace->sem, WRITE_COUNT, 0, 0);

	acquire_sem_etc(sAreaHashLock, WRITE_COUNT, 0, 0);
	if (atomic_add(&area->ref_count, -1) == 1) {
		hash_remove(sAreaHash, area);
		removeit = true;
	}
	release_sem_etc(sAreaHashLock, WRITE_COUNT, 0);

	if (!aspaceLocked)
		release_sem_etc(addressSpace->sem, WRITE_COUNT, 0);

	if (!removeit)
		return false;

	// at this point the area is removed from the global hash table, but still
	// exists in the area list. it's ref_count is zero, and is guaranteed not to
	// be incremented anymore (by a direct hash lookup, or vm_area_lookup()).

	// unmap the virtual address space the area occupied. any page faults at this
	// point should fail in vm_area_lookup().
	vm_unmap_pages(area, area->base, area->size);

	// ToDo: do that only for vnode stores
	vm_cache_write_modified(area->cache_ref, false);

	arch_vm_unset_memory_type(area);
	remove_area_from_address_space(addressSpace, area, aspaceLocked);

	vm_cache_remove_area(area->cache_ref, area);
	vm_cache_release_ref(area->cache_ref);

	// now we can give up the area's reference to the address space
	vm_put_address_space(addressSpace);

	free(area->name);
	free(area);
	return true;
}


static bool
vm_put_area(vm_area *area)
{
	return _vm_put_area(area, false);
}


static status_t
vm_copy_on_write_area(vm_area *area)
{
	vm_store *store;
	vm_cache *upperCache, *lowerCache;
	vm_cache_ref *upperCacheRef, *lowerCacheRef;
	vm_translation_map *map;
	vm_page *page;
	uint32 protection;
	status_t status;

	TRACE(("vm_copy_on_write_area(area = %p)\n", area));

	// We need to separate the vm_cache from its vm_cache_ref: the area
	// and its cache_ref goes into a new layer on top of the old one.
	// So the old cache gets a new cache_ref and the area a new cache.

	upperCacheRef = area->cache_ref;

	// we will exchange the cache_ref's cache, so we better hold its lock
	mutex_lock(&upperCacheRef->lock);

	lowerCache = upperCacheRef->cache;

	// create an anonymous store object
	store = vm_store_create_anonymous_noswap(false, 0, 0);
	if (store == NULL) {
		status = B_NO_MEMORY;
		goto err1;
	}

	upperCache = vm_cache_create(store);
	if (upperCache == NULL) {
		status = B_NO_MEMORY;
		goto err2;
	}

	status = vm_cache_ref_create(lowerCache);
	if (status < B_OK)
		goto err3;

	lowerCacheRef = lowerCache->ref;

	// The area must be readable in the same way it was previously writable
	protection = B_KERNEL_READ_AREA;
	if (area->protection & B_READ_AREA)
		protection |= B_READ_AREA;

	// we need to hold the cache_ref lock when we want to switch its cache
	mutex_lock(&lowerCacheRef->lock);

	upperCache->temporary = 1;
	upperCache->scan_skip = lowerCache->scan_skip;
	upperCache->virtual_base = lowerCache->virtual_base;
	upperCache->virtual_size = lowerCache->virtual_size;

	upperCache->ref = upperCacheRef;
	upperCacheRef->cache = upperCache;

	// we need to manually alter the ref_count (divide it between the two)
	// the lower cache_ref has only known refs, so compute them
	{
		int32 count = 0;
		vm_cache *consumer = NULL;
		while ((consumer = (vm_cache *)list_get_next_item(
				&lowerCache->consumers, consumer)) != NULL) {
			count++;
		}
		lowerCacheRef->ref_count = count;
		atomic_add(&upperCacheRef->ref_count, -count);
	}

	vm_cache_add_consumer_locked(lowerCacheRef, upperCache);

	// We now need to remap all pages from the area read-only, so that
	// a copy will be created on next write access

	map = &area->address_space->translation_map;
	map->ops->lock(map);
	map->ops->unmap(map, area->base, area->base - 1 + area->size);
	map->ops->flush(map);

	for (page = lowerCache->page_list; page; page = page->cache_next) {
		map->ops->map(map, area->base + (page->cache_offset << PAGE_SHIFT)
			- area->cache_offset, page->physical_page_number << PAGE_SHIFT,
			protection);
	}

	map->ops->unlock(map);

	mutex_unlock(&lowerCacheRef->lock);
	mutex_unlock(&upperCacheRef->lock);

	return B_OK;

err3:
	free(upperCache);
err2:
	store->ops->destroy(store);
err1:
	mutex_unlock(&upperCacheRef->lock);
	return status;
}


area_id
vm_copy_area(team_id addressSpaceID, const char *name, void **_address, uint32 addressSpec,
	uint32 protection, area_id sourceID)
{
	vm_address_space *addressSpace;
	vm_cache_ref *cacheRef;
	vm_area *target, *source;
	status_t status;
	bool writableCopy = (protection & (B_KERNEL_WRITE_AREA | B_WRITE_AREA)) != 0;

	if ((protection & B_KERNEL_PROTECTION) == 0) {
		// set the same protection for the kernel as for userland
		protection |= B_KERNEL_READ_AREA;
		if (writableCopy)
			protection |= B_KERNEL_WRITE_AREA;
	}

	if ((source = vm_get_area(sourceID)) == NULL)
		return B_BAD_VALUE;

	addressSpace = vm_get_address_space_by_id(addressSpaceID);
	cacheRef = source->cache_ref;

	if (addressSpec == B_CLONE_ADDRESS) {
		addressSpec = B_EXACT_ADDRESS;
		*_address = (void *)source->base;
	}

	// First, create a cache on top of the source area

	if (!writableCopy) {
		// map_backing_store() cannot know it has to acquire a ref to
		// the store for REGION_NO_PRIVATE_MAP
		vm_cache_acquire_ref(cacheRef);
	}

	status = map_backing_store(addressSpace, cacheRef, _address,
		source->cache_offset, source->size, addressSpec, source->wiring, protection,
		writableCopy ? REGION_PRIVATE_MAP : REGION_NO_PRIVATE_MAP,
		&target, name);
	if (status < B_OK) {
		if (!writableCopy)
			vm_cache_release_ref(cacheRef);
		goto err;
	}

	// If the source area is writable, we need to move it one layer up as well

	if ((source->protection & (B_KERNEL_WRITE_AREA | B_WRITE_AREA)) != 0) {
		// ToDo: do something more useful if this fails!
		if (vm_copy_on_write_area(source) < B_OK)
			panic("vm_copy_on_write_area() failed!\n");
	}

	// we want to return the ID of the newly created area
	status = target->id;

err:
	vm_put_address_space(addressSpace);
	vm_put_area(source);

	return status;
}


static int32
count_writable_areas(vm_cache_ref *ref, vm_area *ignoreArea)
{
	struct vm_area *area = ref->areas;
	uint32 count = 0;

	for (; area != NULL; area = area->cache_next) {
		if (area != ignoreArea
			&& (area->protection & (B_WRITE_AREA | B_KERNEL_WRITE_AREA)) != 0)
			count++;
	}

	return count;
}


static status_t
vm_set_area_protection(team_id aspaceID, area_id areaID, uint32 newProtection)
{
	vm_cache_ref *cacheRef;
	vm_cache *cache;
	vm_area *area;
	status_t status = B_OK;

	TRACE(("vm_set_area_protection(aspace = %#lx, area = %#lx, protection = %#lx)\n",
		aspaceID, areaID, newProtection));

	if (!arch_vm_supports_protection(newProtection))
		return B_NOT_SUPPORTED;

	area = vm_get_area(areaID);
	if (area == NULL)
		return B_BAD_VALUE;

	if (aspaceID != vm_kernel_address_space_id() && area->address_space->id != aspaceID) {
		// unless you're the kernel, you are only allowed to set
		// the protection of your own areas
		vm_put_area(area);
		return B_NOT_ALLOWED;
	}

	cacheRef = area->cache_ref;
	mutex_lock(&cacheRef->lock);

	cache = cacheRef->cache;

	if ((area->protection & (B_WRITE_AREA | B_KERNEL_WRITE_AREA)) != 0
		&& (newProtection & (B_WRITE_AREA | B_KERNEL_WRITE_AREA)) == 0) {
		// change from read/write to read-only

		if (cache->source != NULL && cache->temporary) {
			if (count_writable_areas(cacheRef, area) == 0) {
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
		// change from read-only to read/write

		// ToDo: if this is a shared cache, insert new cache (we only know about other
		//	areas in this cache yet, though, not about child areas)
		//	-> use this call with care, it might currently have unwanted consequences
		//	   because of this. It should always be safe though, if there are no other
		//	   (child) areas referencing this area's cache (you just might not know).
		if (count_writable_areas(cacheRef, area) == 0
			&& (cacheRef->areas != area || area->cache_next)) {
			// ToDo: child areas are not tested for yet
			dprintf("set_area_protection(): warning, would need to insert a new cache_ref (not yet implemented)!\n");
			status = B_NOT_ALLOWED;
		} else
			dprintf("set_area_protection() may not work correctly yet in this direction!\n");

		if (status == B_OK && cache->source != NULL && cache->temporary) {
			// the cache's commitment must contain all possible pages
			status = cache->store->ops->commit(cache->store, cache->virtual_size);
		}
	} else {
		// we don't have anything special to do in all other cases
	}

	if (status == B_OK && area->protection != newProtection) {
		// remap existing pages in this cache
		struct vm_translation_map *map = &area->address_space->translation_map;

		map->ops->lock(map);
		map->ops->protect(map, area->base, area->base + area->size, newProtection);
		map->ops->unlock(map);

		area->protection = newProtection;
	}

	mutex_unlock(&cacheRef->lock);
	vm_put_area(area);

	return status;
}


status_t
vm_get_page_mapping(team_id aid, addr_t vaddr, addr_t *paddr)
{
	vm_address_space *addressSpace;
	uint32 null_flags;
	status_t err;

	addressSpace = vm_get_address_space_by_id(aid);
	if (addressSpace == NULL)
		return B_BAD_TEAM_ID;

	err = addressSpace->translation_map.ops->query(&addressSpace->translation_map,
		vaddr, paddr, &null_flags);

	vm_put_address_space(addressSpace);
	return err;
}


status_t
vm_unmap_pages(vm_area *area, addr_t base, size_t size)
{
	vm_translation_map *map = &area->address_space->translation_map;

	map->ops->lock(map);

	if (area->wiring != B_NO_LOCK && area->cache_type != CACHE_TYPE_DEVICE) {
		// iterate through all pages and decrease their wired count
		for (addr_t virtualAddress = base; virtualAddress < base + (size - 1);
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

	map->ops->unmap(map, base, base + (size - 1));
	map->ops->unlock(map);
	return B_OK;
}


status_t
vm_map_page(vm_area *area, vm_page *page, addr_t address, uint32 protection)
{
	vm_translation_map *map = &area->address_space->translation_map;

	map->ops->lock(map);
	map->ops->map(map, address, page->physical_page_number * B_PAGE_SIZE,
		protection);
	map->ops->unlock(map);

	if (area->wiring != B_NO_LOCK) {
		page->wired_count++;
			// TODO: needs to be atomic on all platforms!
	}

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
		kprintf("usage: dl/dw/ds/db [-p|--physical] <address> [num]\n"
			"\tdl - 8 bytes\n"
			"\tdw - 4 bytes\n"
			"\tds - 2 bytes\n"
			"\tdb - 1 byte\n"
			"  -p or --physical only allows memory from a single page to be displayed.\n");
		return 0;
	}

	address = strtoul(argv[i], NULL, 0);

	if (argc > i + 1)
		num = atoi(argv[i + 1]);

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

	for (i = 0; i < num; i++) {
		uint32 value;

		if ((i % displayWidth) == 0) {
			int32 displayed = min_c(displayWidth, (num-i)) * itemSize;
			if (i != 0)
				kprintf("\n");

			kprintf("[0x%lx]  ", address + i * itemSize);

			for (j = 0; j < displayed; j++) {
				char c;
				if (user_memcpy(&c, (char *)copyAddress + i * itemSize + j, 1) != B_OK) {
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

		if (user_memcpy(&value, (uint8 *)copyAddress + i * itemSize, itemSize) != B_OK) {
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

	if (physical) {
		copyAddress = ROUNDOWN(copyAddress, B_PAGE_SIZE);
		kernel_startup = true;
		vm_put_physical_page(copyAddress);
		kernel_startup = false;
	}
	return 0;
}


static const char *
page_state_to_string(int state)
{
	switch(state) {
		case PAGE_STATE_ACTIVE:
			return "active";
		case PAGE_STATE_INACTIVE:
			return "inactive";
		case PAGE_STATE_BUSY:
			return "busy";
		case PAGE_STATE_MODIFIED:
			return "modified";
		case PAGE_STATE_FREE:
			return "free";
		case PAGE_STATE_CLEAR:
			return "clear";
		case PAGE_STATE_WIRED:
			return "wired";
		case PAGE_STATE_UNUSED:
			return "unused";
		default:
			return "unknown";
	}
}


static int
dump_cache_chain(int argc, char **argv)
{
	if (argc < 2 || strlen(argv[1]) < 2
		|| argv[1][0] != '0'
		|| argv[1][1] != 'x') {
		kprintf("%s: invalid argument, pass address\n", argv[0]);
		return 0;
	}

	addr_t address = strtoul(argv[1], NULL, 0);
	if (address == NULL)
		return 0;

	vm_cache *cache = (vm_cache *)address;
	while (cache != NULL) {
		dprintf("%p  (ref %p)\n", cache, cache->ref);
		cache = cache->source;
	}

	return 0;
}


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
	vm_cache_ref *cacheRef;
	bool showPages = false;
	bool showCache = true;
	bool showCacheRef = true;
	int i = 1;

	if (argc < 2) {
		kprintf("usage: %s [-ps] <address>\n"
			"  if -p is specified, all pages are shown, if -s is used\n"
			"  only the cache/cache_ref info is shown respectively.\n", argv[0]);
		return 0;
	}
	while (argv[i][0] == '-') {
		char *arg = argv[i] + 1;
		while (arg[0]) {
			if (arg[0] == 'p')
				showPages = true;
			else if (arg[0] == 's') {
				if (!strcmp(argv[0], "cache"))
					showCacheRef = false;
				else
					showCache = false;
			}
			arg++;
		}
		i++;
	}
	if (argv[i] == NULL || strlen(argv[i]) < 2
		|| argv[i][0] != '0'
		|| argv[i][1] != 'x') {
		kprintf("%s: invalid argument, pass address\n", argv[0]);
		return 0;
	}

	addr_t address = strtoul(argv[i], NULL, 0);
	if (address == NULL)
		return 0;

	if (!strcmp(argv[0], "cache")) {
		cache = (vm_cache *)address;
		cacheRef = cache->ref;
	} else {
		cacheRef = (vm_cache_ref *)address;
		cache = cacheRef->cache;
	}

	if (showCacheRef) {
		kprintf("CACHE_REF %p:\n", cacheRef);
		if (!showCache)
			kprintf("  cache:        %p\n", cacheRef->cache);
		kprintf("  ref_count:    %ld\n", cacheRef->ref_count);
		kprintf("  lock.holder:  %ld\n", cacheRef->lock.holder);
		kprintf("  lock.sem:     0x%lx\n", cacheRef->lock.sem);
		kprintf("  areas:\n");

		for (vm_area *area = cacheRef->areas; area != NULL; area = area->cache_next) {
			kprintf("    area 0x%lx, %s\n", area->id, area->name);
			kprintf("\tbase_addr:  0x%lx, size: 0x%lx\n", area->base, area->size);
			kprintf("\tprotection: 0x%lx\n", area->protection);
			kprintf("\towner:      0x%lx\n", area->address_space->id);
		}
	}

	if (showCache) {
		kprintf("CACHE %p:\n", cache);
		if (!showCacheRef)
			kprintf("  cache_ref:    %p\n", cache->ref);
		kprintf("  source:       %p\n", cache->source);
		kprintf("  store:        %p\n", cache->store);
		kprintf("  type:         %s\n", cache_type_to_string(cache->type));
		kprintf("  virtual_base: 0x%Lx\n", cache->virtual_base);
		kprintf("  virtual_size: 0x%Lx\n", cache->virtual_size);
		kprintf("  temporary:    %ld\n", cache->temporary);
		kprintf("  scan_skip:    %ld\n", cache->scan_skip);

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
	}

	return 0;
}


static void
_dump_area(vm_area *area)
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
	kprintf("ref_count:\t%ld\n", area->ref_count);
	kprintf("cache_ref:\t%p\n", area->cache_ref);
	kprintf("cache_type:\t%s\n", cache_type_to_string(area->cache_type));
	kprintf("cache_offset:\t0x%Lx\n", area->cache_offset);
	kprintf("cache_next:\t%p\n", area->cache_next);
	kprintf("cache_prev:\t%p\n", area->cache_prev);
}


static int
dump_area(int argc, char **argv)
{
	bool found = false;
	vm_area *area;
	addr_t num;

	if (argc < 2) {
		kprintf("usage: area <id|address|name>\n");
		return 0;
	}

	num = strtoul(argv[1], NULL, 0);

	// walk through the area list, looking for the arguments as a name
	struct hash_iterator iter;

	hash_open(sAreaHash, &iter);
	while ((area = (vm_area *)hash_next(sAreaHash, &iter)) != NULL) {
		if ((area->name != NULL && !strcmp(argv[1], area->name))
			|| num != 0
				&& ((addr_t)area->id == num
					|| area->base <= num && area->base + area->size > num)) {
			_dump_area(area);
			found = true;
		}
	}

	if (!found)
		kprintf("could not find area %s (%ld)\n", argv[1], num);
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
		id = strtoul(argv[1], NULL, 0);
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

	TRACE(("vm_delete_areas: called on address space 0x%lx\n", addressSpace->id));

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

		// decrement the ref on this area, may actually push the ref < 0, if there
		// is a concurrent delete_area() on that specific area, but that's ok here
		if (!_vm_put_area(area, true))
			dprintf("vm_delete_areas() did not delete area %p\n", area);
	}

	release_sem_etc(addressSpace->sem, WRITE_COUNT, 0);

	return B_OK;
}


static area_id
vm_area_for(team_id team, addr_t address)
{
	vm_address_space *addressSpace;
	area_id id = B_ERROR;

	addressSpace = vm_get_address_space_by_id(team);
	if (addressSpace == NULL)
		return B_BAD_TEAM_ID;

	acquire_sem_etc(addressSpace->sem, READ_COUNT, 0, 0);

	vm_area *area = vm_area_lookup(addressSpace, address);
	if (area != NULL)
		id = area->id;

	release_sem_etc(addressSpace->sem, READ_COUNT, 0);
	vm_put_address_space(addressSpace);

	return id;
}


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
	vm_translation_map *map = &kernel_aspace->translation_map;
	addr_t end = start + size;
	addr_t lastEnd = start;
	vm_area *area;

	TRACE(("vm_free_unused_boot_loader_range(): asked to free %p - %p\n", (void *)start, (void *)end));

	// The areas are sorted in virtual address space order, so
	// we just have to find the holes between them that fall
	// into the area we should dispose

	map->ops->lock(map);

	for (area = kernel_aspace->areas; area; area = area->address_space_next) {
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

	// reduce the heap size if we have not so much RAM
	size_t heapSize = HEAP_SIZE;
	if (sAvailableMemory < 100 * 1024 * 1024)
		heapSize /= 4;
	else if (sAvailableMemory < 200 * 1024 * 1024)
		heapSize /= 2;

	// map in the new heap and initialize it
	addr_t heapBase = vm_allocate_early(args, heapSize, heapSize,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	TRACE(("heap at 0x%lx\n", heapBase));
	heap_init(heapBase, heapSize);

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
	add_debugger_command("cache_ref", &dump_cache, "Dump vm_cache");
	add_debugger_command("cache", &dump_cache, "Dump vm_cache");
	add_debugger_command("cache_chain", &dump_cache_chain, "Dump vm_cache chain");
	add_debugger_command("avail", &dump_available_memory, "Dump available memory");
	add_debugger_command("dl", &display_mem, "dump memory long words (64-bit)");
	add_debugger_command("dw", &display_mem, "dump memory words (32-bit)");
	add_debugger_command("ds", &display_mem, "dump memory shorts (16-bit)");
	add_debugger_command("db", &display_mem, "dump memory bytes (8-bit)");

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

	for (area = kernel_aspace->areas; area; area = area->address_space_next) {
		if (area->id == RESERVED_AREA_ID)
			continue;

		if (area->cache_ref->lock.sem < 0)
			mutex_init(&area->cache_ref->lock, "cache_ref_mutex");
	}

	sAreaHashLock = create_sem(WRITE_COUNT, "area hash");

	return heap_init_post_sem(args);
}


status_t
vm_init_post_thread(kernel_args *args)
{
	vm_page_init_post_thread(args);
	vm_daemon_init();
	vm_low_memory_init();

	return heap_init_post_thread(args);
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
			area = vm_area_lookup(addressSpace, faultAddress);

			dprintf("vm_page_fault: sending team \"%s\" 0x%lx SIGSEGV, ip %#lx (\"%s\" +%#lx)\n",
				thread_get_current_thread()->team->name,
				thread_get_current_thread()->team->id, faultAddress,
				area ? area->name : "???", faultAddress - (area ? area->base : 0x0));

			// We can print a stack trace of the userland thread here.
#if 1
			if (area) {
				struct stack_frame {
					#ifdef __INTEL__
						struct stack_frame*	previous;
						void*				return_address;
					#else
						// ...
					#endif
				};
				struct iframe *iframe = i386_get_user_iframe();
				if (iframe == NULL)
					panic("iframe is NULL!");

				struct stack_frame frame;
				status_t status = user_memcpy(&frame, (void *)iframe->ebp,
					sizeof(struct stack_frame));

				dprintf("stack trace:\n");
				while (status == B_OK) {
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
fault_acquire_locked_source(vm_cache *cache, vm_cache_ref **_sourceRef)
{
retry:
	vm_cache *source = cache->source;
	if (source == NULL)
		return B_ERROR;
	if (source->busy)
		return B_BUSY;

	vm_cache_ref *sourceRef = source->ref;
	vm_cache_acquire_ref(sourceRef);

	mutex_lock(&sourceRef->lock);

	if (sourceRef->cache != cache->source || sourceRef->cache->busy) {
		mutex_unlock(&sourceRef->lock);
		vm_cache_release_ref(sourceRef);
		goto retry;
	}

	*_sourceRef = sourceRef;
	return B_OK;
}


/*!
	Inserts a busy dummy page into a cache, and makes sure the cache won't go
	away by grabbing a reference to it.
*/
static inline void
fault_insert_dummy_page(vm_cache_ref *cacheRef, vm_page &dummyPage, off_t cacheOffset)
{
	dummyPage.state = PAGE_STATE_BUSY;
	vm_cache_acquire_ref(cacheRef);
	vm_cache_insert_page(cacheRef, &dummyPage, cacheOffset);
}


/*!
	Removes the busy dummy page from a cache, and releases its reference to
	the cache.
*/
static inline void
fault_remove_dummy_page(vm_page &dummyPage, bool isLocked)
{
	vm_cache_ref *cacheRef = dummyPage.cache->ref;
	if (!isLocked)
		mutex_lock(&cacheRef->lock);

	vm_cache_remove_page(cacheRef, &dummyPage);

	if (!isLocked)
		mutex_unlock(&cacheRef->lock);

	vm_cache_release_ref(cacheRef);

	dummyPage.state = PAGE_STATE_INACTIVE;
}


/*!
	Finds a page at the specified \a cacheOffset in either the \a topCacheRef
	or in its source chain. Will also page in a missing page in case there is
	a cache that has the page.
	If it couldn't find a page, it will return the vm_cache that should get it,
	otherwise, it will return the vm_cache that contains the cache.
	It always grabs a reference to the vm_cache that it returns, and also locks it.
*/
static inline vm_page *
fault_find_page(vm_translation_map *map, vm_cache_ref *topCacheRef,
	off_t cacheOffset, bool isWrite, vm_page &dummyPage, vm_cache_ref **_pageRef)
{
	vm_cache_ref *cacheRef = topCacheRef;
	vm_cache_ref *lastCacheRef = NULL;
	vm_page *page = NULL;

	vm_cache_acquire_ref(cacheRef);
	mutex_lock(&cacheRef->lock);
		// we release this later in the loop

	while (cacheRef != NULL) {
		if (lastCacheRef != NULL)
			vm_cache_release_ref(lastCacheRef);

		// we hold the lock of the cacheRef at this point

		lastCacheRef = cacheRef;

		for (;;) {
			page = vm_cache_lookup_page(cacheRef, cacheOffset);
			if (page != NULL && page->state != PAGE_STATE_BUSY) {
				vm_page_set_state(page, PAGE_STATE_BUSY);
				break;
			}

			if (page == NULL)
				break;

			// page must be busy
			// ToDo: don't wait forever!
			mutex_unlock(&cacheRef->lock);
			snooze(20000);
			mutex_lock(&cacheRef->lock);
		}

		if (page != NULL)
			break;

		// The current cache does not contain the page we're looking for

		// If we're at the top most cache, insert the dummy page here to keep other threads
		// from faulting on the same address and chasing us up the cache chain
		if (cacheRef == topCacheRef)
			fault_insert_dummy_page(cacheRef, dummyPage, cacheOffset);

		// see if the vm_store has it
		vm_store *store = cacheRef->cache->store;
		if (store->ops->has_page != NULL && store->ops->has_page(store, cacheOffset)) {
			size_t bytesRead;
			iovec vec;

			vec.iov_len = bytesRead = B_PAGE_SIZE;

			mutex_unlock(&cacheRef->lock);

			page = vm_page_allocate_page(PAGE_STATE_FREE);

			dummyPage.queue_next = page;
			dummyPage.busy_reading = true;
				// we mark that page busy reading, so that the file cache can ignore
				// us in case it works on the very same page

			map->ops->get_physical_page(page->physical_page_number * B_PAGE_SIZE, (addr_t *)&vec.iov_base, PHYSICAL_PAGE_CAN_WAIT);
			status_t status = store->ops->read(store, cacheOffset, &vec, 1, &bytesRead, false);
			if (status < B_OK) {
				// TODO: real error handling!
				panic("reading from store %p (cacheRef %p) returned: %s!\n", store, cacheRef, strerror(status));
			}
			map->ops->put_physical_page((addr_t)vec.iov_base);

			mutex_lock(&cacheRef->lock);

			if (cacheRef == topCacheRef)
				fault_remove_dummy_page(dummyPage, true);

			// We insert the queue_next here, because someone else could have
			// replaced our page
			vm_cache_insert_page(cacheRef, dummyPage.queue_next, cacheOffset);

			if (dummyPage.queue_next != page) {
				// Indeed, the page got replaced by someone else - we can safely
				// throw our page away now
				vm_page_set_state(page, PAGE_STATE_FREE);
				page = dummyPage.queue_next;
			}
			break;
		}

		vm_cache_ref *nextCacheRef;
		status_t status = fault_acquire_locked_source(cacheRef->cache, &nextCacheRef);
		if (status == B_BUSY) {
			// the source cache is currently in the process of being merged
			// with his only consumer (cacheRef); since its pages are moved
			// upwards, too, we try this cache again
			mutex_unlock(&cacheRef->lock);
			mutex_lock(&cacheRef->lock);
			lastCacheRef = NULL;
			continue;
		} else if (status < B_OK)
			nextCacheRef = NULL;

		mutex_unlock(&cacheRef->lock);
			// at this point, we still hold a ref to this cache (through lastCacheRef)

		cacheRef = nextCacheRef;
	}

	if (page == NULL) {
		// there was no adequate page, determine the cache for a clean one
		if (cacheRef == NULL) {
			// We rolled off the end of the cache chain, so we need to decide which
			// cache will get the new page we're about to create.
			cacheRef = isWrite ? topCacheRef : lastCacheRef;
				// Read-only pages come in the deepest cache - only the 
				// top most cache may have direct write access.
			vm_cache_acquire_ref(cacheRef);
			mutex_lock(&cacheRef->lock);
		}

		// release the reference of the last vm_cache_ref we still have from the loop above
		if (lastCacheRef != NULL)
			vm_cache_release_ref(lastCacheRef);
	} else {
		// we still own a reference to the cacheRef
	}

	*_pageRef = cacheRef;
	return page;
}


/*!
	Returns the page that should be mapped into the area that got the fault.
	It returns the owner of the page in \a sourceRef - it keeps a reference
	to it, and has also locked it on exit.
*/
static inline vm_page *
fault_get_page(vm_translation_map *map, vm_cache_ref *topCacheRef,
	off_t cacheOffset, bool isWrite, vm_page &dummyPage, vm_cache_ref **_sourceRef)
{
	vm_cache_ref *cacheRef;
	vm_page *page = fault_find_page(map, topCacheRef, cacheOffset, isWrite,
		dummyPage, &cacheRef);
	if (page == NULL) {
		// we still haven't found a page, so we allocate a clean one

		page = vm_page_allocate_page(PAGE_STATE_CLEAR);
		FTRACE(("vm_soft_fault: just allocated page 0x%lx\n", page->physical_page_number));

		// Insert the new page into our cache, and replace it with the dummy page if necessary

		// if we inserted a dummy page into this cache, we have to remove it now
		if (dummyPage.state == PAGE_STATE_BUSY && dummyPage.cache == cacheRef->cache)
			fault_remove_dummy_page(dummyPage, true);

		vm_cache_insert_page(cacheRef, page, cacheOffset);

		if (dummyPage.state == PAGE_STATE_BUSY) {
			// we had inserted the dummy cache in another cache, so let's remove it from there
			fault_remove_dummy_page(dummyPage, false);
		}
	}

	// We now have the page and a cache it belongs to - we now need to make
	// sure that the area's cache can access it, too, and sees the correct data

	if (page->cache != topCacheRef->cache && isWrite) {
		// now we have a page that has the data we want, but in the wrong cache object
		// so we need to copy it and stick it into the top cache
		vm_page *sourcePage = page;
		void *source, *dest;

		// ToDo: if memory is low, it might be a good idea to steal the page
		//	from our source cache - if possible, that is
		FTRACE(("get new page, copy it, and put it into the topmost cache\n"));
		page = vm_page_allocate_page(PAGE_STATE_FREE);

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

		vm_page_set_state(sourcePage, PAGE_STATE_ACTIVE);

		mutex_unlock(&cacheRef->lock);
		mutex_lock(&topCacheRef->lock);

		// Insert the new page into our cache, and replace it with the dummy page if necessary

		// if we inserted a dummy page into this cache, we have to remove it now
		if (dummyPage.state == PAGE_STATE_BUSY && dummyPage.cache == topCacheRef->cache)
			fault_remove_dummy_page(dummyPage, true);

		vm_cache_insert_page(topCacheRef, page, cacheOffset);

		if (dummyPage.state == PAGE_STATE_BUSY) {
			// we had inserted the dummy cache in another cache, so let's remove it from there
			fault_remove_dummy_page(dummyPage, false);
		}

		vm_cache_release_ref(cacheRef);

		cacheRef = topCacheRef;
		vm_cache_acquire_ref(cacheRef);
	}

	*_sourceRef = cacheRef;
	return page;
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

	atomic_add(&addressSpace->fault_count, 1);

	// Get the area the fault was in

	acquire_sem_etc(addressSpace->sem, READ_COUNT, 0, 0);

	vm_area *area = vm_area_lookup(addressSpace, address);
	if (area == NULL) {
		release_sem_etc(addressSpace->sem, READ_COUNT, 0);
		vm_put_address_space(addressSpace);
		dprintf("vm_soft_fault: va 0x%lx not covered by area in address space\n",
			originalAddress);
		return B_BAD_ADDRESS;
	}

	// check permissions
	if (isUser && (area->protection & B_USER_PROTECTION) == 0) {
		release_sem_etc(addressSpace->sem, READ_COUNT, 0);
		vm_put_address_space(addressSpace);
		dprintf("user access on kernel area 0x%lx at %p\n", area->id, (void *)originalAddress);
		return B_PERMISSION_DENIED;
	}
	if (isWrite && (area->protection & (B_WRITE_AREA | (isUser ? 0 : B_KERNEL_WRITE_AREA))) == 0) {
		release_sem_etc(addressSpace->sem, READ_COUNT, 0);
		vm_put_address_space(addressSpace);
		dprintf("write access attempted on read-only area 0x%lx at %p\n",
			area->id, (void *)originalAddress);
		return B_PERMISSION_DENIED;
	}

	// We have the area, it was a valid access, so let's try to resolve the page fault now.
	// At first, the top most cache from the area is investigated

	vm_cache_ref *topCacheRef = area->cache_ref;
	off_t cacheOffset = address - area->base + area->cache_offset;
	int32 changeCount = addressSpace->change_count;

	vm_cache_acquire_ref(topCacheRef);
	release_sem_etc(addressSpace->sem, READ_COUNT, 0);

	mutex_lock(&topCacheRef->lock);

	// See if this cache has a fault handler - this will do all the work for us
	{
		vm_store *store = topCacheRef->cache->store;
		if (store->ops->fault != NULL) {
			// Note, since the page fault is resolved with interrupts enabled, the
			// fault handler could be called more than once for the same reason -
			// the store must take this into account
			status_t status = store->ops->fault(store, addressSpace, cacheOffset);
			if (status != B_BAD_HANDLER) {
				mutex_unlock(&topCacheRef->lock);
				vm_cache_release_ref(topCacheRef);
				vm_put_address_space(addressSpace);
				return status;
			}
		}
	}

	mutex_unlock(&topCacheRef->lock);

	// The top most cache has no fault handler, so let's see if the cache or its sources
	// already have the page we're searching for (we're going from top to bottom)

	vm_translation_map *map = &addressSpace->translation_map;
	vm_page dummyPage;
	dummyPage.state = PAGE_STATE_INACTIVE;
	dummyPage.type = PAGE_TYPE_DUMMY;

	vm_cache_ref *pageSourceRef;
	vm_page *page = fault_get_page(map, topCacheRef, cacheOffset, isWrite,
		dummyPage, &pageSourceRef);

	status_t status = B_OK;

	acquire_sem_etc(addressSpace->sem, READ_COUNT, 0, 0);
	if (changeCount != addressSpace->change_count) {
		// something may have changed, see if the address is still valid
		area = vm_area_lookup(addressSpace, address);
		if (area == NULL
			|| area->cache_ref != topCacheRef
			|| (address - area->base + area->cache_offset) != cacheOffset) {
			dprintf("vm_soft_fault: address space layout changed effecting ongoing soft fault\n");
			status = B_BAD_ADDRESS;
		}
	}

	if (status == B_OK) {
		// All went fine, all there is left to do is to map the page into the address space

		// If the page doesn't reside in the area's cache, we need to make sure it's
		// mapped in read-only, so that we cannot overwrite someone else's data (copy-on-write)
		uint32 newProtection = area->protection;
		if (page->cache != topCacheRef->cache && !isWrite)
			newProtection &= ~(isUser ? B_WRITE_AREA : B_KERNEL_WRITE_AREA);

		vm_map_page(area, page, address, newProtection);
	}

	release_sem_etc(addressSpace->sem, READ_COUNT, 0);

	mutex_unlock(&pageSourceRef->lock);
	vm_cache_release_ref(pageSourceRef);

	if (dummyPage.state == PAGE_STATE_BUSY) {
		// We still have the dummy page in the cache - that happens if we didn't need
		// to allocate a new page before, but could use one in another cache
		fault_remove_dummy_page(dummyPage, false);
	}

	vm_cache_release_ref(topCacheRef);
	vm_put_address_space(addressSpace);

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
	// if the ref count is zero, the area is in the middle of being
	// destroyed in _vm_put_area. pretend it doesn't exist.
	if (area && area->ref_count == 0)
		return NULL;

	if (area)
		addressSpace->area_hint = area;

	return area;
}


status_t
vm_get_physical_page(addr_t paddr, addr_t *_vaddr, uint32 flags)
{
	return (*kernel_aspace->translation_map.ops->get_physical_page)(paddr, _vaddr, flags);
}


status_t
vm_put_physical_page(addr_t vaddr)
{
	return (*kernel_aspace->translation_map.ops->put_physical_page)(vaddr);
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
	vm_area *area = vm_get_area(id);
	if (area == NULL)
		return B_BAD_VALUE;

	status_t status = arch_vm_set_memory_type(area, physicalBase, type);

	vm_put_area(area);
	return status;
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

	mutex_lock(&area->cache_ref->lock);

	// Note, this is a simplification; the cache could be larger than this area
	info->ram_size = area->cache_ref->cache->page_count * B_PAGE_SIZE;

	mutex_unlock(&area->cache_ref->lock);
}


/*!
	Tests wether or not the area that contains the specified address
	needs any kind of locking, and actually exists.
	Used by both lock_memory() and unlock_memory().
*/
status_t
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


//	#pragma mark -


status_t
user_memcpy(void *to, const void *from, size_t size)
{
	return arch_cpu_user_memcpy(to, from, size, &thread_get_current_thread()->fault_handler);
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
	return arch_cpu_user_memset(s, c, count, &thread_get_current_thread()->fault_handler);
}


//	#pragma mark - kernel public API


long
lock_memory(void *address, ulong numBytes, ulong flags)
{
	vm_address_space *addressSpace = NULL;
	struct vm_translation_map *map;
	addr_t base = (addr_t)address;
	addr_t end = base + numBytes;
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
				address, numBytes, flags, strerror(status));
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
	addr_t base = (addr_t)address;
	addr_t end = base + numBytes;
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
get_memory_map(const void *address, ulong numBytes, physical_entry *table, long numEntries)
{
	vm_address_space *addressSpace;
	addr_t virtualAddress = (addr_t)address;
	addr_t pageOffset = virtualAddress & (B_PAGE_SIZE - 1);
	addr_t physicalAddress;
	status_t status = B_OK;
	int32 index = -1;
	addr_t offset = 0;
	bool interrupts = are_interrupts_enabled();

	TRACE(("get_memory_map(%p, %lu bytes, %ld entries)\n", address, numBytes, numEntries));

	if (numEntries == 0 || numBytes == 0)
		return B_BAD_VALUE;

	// in which address space is the address to be found?	
	if (IS_USER_ADDRESS(virtualAddress))
		addressSpace = vm_get_current_user_address_space();
	else
		addressSpace = vm_get_kernel_address_space();

	if (addressSpace == NULL)
		return B_ERROR;

	vm_translation_map *map = &addressSpace->translation_map;

	if (interrupts)
		map->ops->lock(map);

	while (offset < numBytes) {
		addr_t bytes = min_c(numBytes - offset, B_PAGE_SIZE);

		if (interrupts) {
			uint32 flags;
			status = map->ops->query(map, (addr_t)address + offset,
				&physicalAddress, &flags);
		} else {
			status = map->ops->query_interrupt(map, (addr_t)address + offset,
				&physicalAddress);
		}
		if (status < B_OK)
			break;

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
	return vm_area_for(vm_kernel_address_space_id(), (addr_t)address);
}


area_id
find_area(const char *name)
{
	struct hash_iterator iterator;
	vm_area *area;
	area_id id = B_NAME_NOT_FOUND;

	acquire_sem_etc(sAreaHashLock, READ_COUNT, 0, 0);
	hash_open(sAreaHash, &iterator);

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
	vm_area *area;

	if (size != sizeof(area_info) || info == NULL)
		return B_BAD_VALUE;

	area = vm_get_area(id);
	if (area == NULL)
		return B_BAD_VALUE;

	fill_area_info(area, info, size);
	vm_put_area(area);

	return B_OK;
}


status_t
_get_next_area_info(team_id team, int32 *cookie, area_info *info, size_t size)
{
	addr_t nextBase = *(addr_t *)cookie;
	vm_address_space *addressSpace;
	vm_area *area;

	// we're already through the list
	if (nextBase == (addr_t)-1)
		return B_ENTRY_NOT_FOUND;

	if (team == B_CURRENT_TEAM)
		team = team_get_current_team_id();

	if (!team_is_valid(team)
		|| team_get_address_space(team, &addressSpace) != B_OK)
		return B_BAD_VALUE;

	acquire_sem_etc(addressSpace->sem, READ_COUNT, 0, 0);

	for (area = addressSpace->areas; area; area = area->address_space_next) {
		if (area->id == RESERVED_AREA_ID)
			continue;

		if (area->base > nextBase)
			break;
	}

	// make sure this area won't go away
	if (area != NULL)
		area = vm_get_area(area->id);

	release_sem_etc(addressSpace->sem, READ_COUNT, 0);
	vm_put_address_space(addressSpace);

	if (area == NULL) {
		nextBase = (addr_t)-1;
		return B_ENTRY_NOT_FOUND;
	}

	fill_area_info(area, info, size);
	*cookie = (int32)(area->base);

	vm_put_area(area);

	return B_OK;
}


status_t
set_area_protection(area_id area, uint32 newProtection)
{
	fix_protection(&newProtection);

	return vm_set_area_protection(vm_kernel_address_space_id(), area, newProtection);
}


status_t
resize_area(area_id areaID, size_t newSize)
{
	vm_cache_ref *cacheRef;
	vm_area *area, *current;
	status_t status = B_OK;
	size_t oldSize;

	// is newSize a multiple of B_PAGE_SIZE?
	if (newSize & (B_PAGE_SIZE - 1))
		return B_BAD_VALUE;

	area = vm_get_area(areaID);
	if (area == NULL)
		return B_BAD_VALUE;

	cacheRef = area->cache_ref;
	mutex_lock(&cacheRef->lock);

	// Resize all areas of this area's cache

	oldSize = area->size;

	// ToDo: we should only allow to resize anonymous memory areas!
	if (!cacheRef->cache->temporary) {
		status = B_NOT_ALLOWED;
		goto out;
	}

	// ToDo: we must lock all address spaces here!

	if (oldSize < newSize) {
		// We need to check if all areas of this cache can be resized

		for (current = cacheRef->areas; current; current = current->cache_next) {
			if (current->address_space_next
				&& current->address_space_next->base <= (current->base
					+ newSize)) {
				// if the area was created inside a reserved area, it can also be
				// resized in that area
				// ToDo: if there is free space after the reserved area, it could be used as well...
				vm_area *next = current->address_space_next;
				if (next->id == RESERVED_AREA_ID
					&& next->cache_offset <= current->base
					&& next->base - 1 + next->size >= current->base - 1 + newSize)
					continue;

				status = B_ERROR;
				goto out;
			}
		}
	}

	// Okay, looks good so far, so let's do it

	for (current = cacheRef->areas; current; current = current->cache_next) {
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
			vm_translation_map *map = &current->address_space->translation_map;

			map->ops->lock(map);
			map->ops->unmap(map, current->base + newSize, current->base + oldSize - 1);
			map->ops->unlock(map);
		}
	}

	if (status == B_OK)
		status = vm_cache_resize(cacheRef, newSize);

	if (status < B_OK) {
		// This shouldn't really be possible, but hey, who knows
		for (current = cacheRef->areas; current; current = current->cache_next)
			current->size = oldSize;
	}

out:
	mutex_unlock(&cacheRef->lock);
	vm_put_area(area);

	// ToDo: we must honour the lock restrictions of this area
	return status;
}


/**	Transfers the specified area to a new team. The caller must be the owner
 *	of the area (not yet enforced but probably should be).
 *	This function is currently not exported to the kernel namespace, but is
 *	only accessible using the _kern_transfer_area() syscall.
 */

static status_t
transfer_area(area_id id, void **_address, uint32 addressSpec, team_id target)
{
	vm_address_space *sourceAddressSpace, *targetAddressSpace;
	vm_translation_map *map;
	vm_area *area, *reserved;
	void *reservedAddress;
	status_t status;

	area = vm_get_area(id);
	if (area == NULL)
		return B_BAD_VALUE;

	// ToDo: check if the current team owns the area

	status = team_get_address_space(target, &targetAddressSpace);
	if (status != B_OK)
		goto err1;

	// We will first remove the area, and then reserve its former
	// address range so that we can later reclaim it if the
	// transfer failed.

	sourceAddressSpace = area->address_space;

	reserved = create_reserved_area_struct(sourceAddressSpace, 0);
	if (reserved == NULL) {
		status = B_NO_MEMORY;
		goto err2;
	}

	acquire_sem_etc(sourceAddressSpace->sem, WRITE_COUNT, 0, 0);

	reservedAddress = (void *)area->base;
	remove_area_from_address_space(sourceAddressSpace, area, true);
	status = insert_area(sourceAddressSpace, &reservedAddress, B_EXACT_ADDRESS,
		area->size, reserved);
		// famous last words: this cannot fail :)

	release_sem_etc(sourceAddressSpace->sem, WRITE_COUNT, 0);

	if (status != B_OK)
		goto err3;

	// unmap the area in the source address space
	map = &sourceAddressSpace->translation_map;
	map->ops->lock(map);
	map->ops->unmap(map, area->base, area->base + (area->size - 1));
	map->ops->unlock(map);

	// insert the area into the target address space

	acquire_sem_etc(targetAddressSpace->sem, WRITE_COUNT, 0, 0);
	// check to see if this address space has entered DELETE state
	if (targetAddressSpace->state == VM_ASPACE_STATE_DELETION) {
		// okay, someone is trying to delete this adress space now, so we can't
		// insert the area, so back out
		status = B_BAD_TEAM_ID;
		goto err4;
	}

	status = insert_area(targetAddressSpace, _address, addressSpec, area->size, area);
	if (status < B_OK)
		goto err4;

	// The area was successfully transferred to the new team when we got here
	area->address_space = targetAddressSpace;

	release_sem_etc(targetAddressSpace->sem, WRITE_COUNT, 0);

	vm_unreserve_address_range(sourceAddressSpace->id, reservedAddress, area->size);
	vm_put_address_space(sourceAddressSpace);
		// we keep the reference of the target address space for the
		// area, so we only have to put the one from the source
	vm_put_area(area);

	return B_OK;

err4:
	release_sem_etc(targetAddressSpace->sem, WRITE_COUNT, 0);
err3:
	// insert the area again into the source address space
	acquire_sem_etc(sourceAddressSpace->sem, WRITE_COUNT, 0, 0);
	// check to see if this address space has entered DELETE state
	if (sourceAddressSpace->state == VM_ASPACE_STATE_DELETION
		|| insert_area(sourceAddressSpace, &reservedAddress, B_EXACT_ADDRESS, area->size, area) != B_OK) {
		// We can't insert the area anymore - we have to delete it manually
		vm_cache_remove_area(area->cache_ref, area);
		vm_cache_release_ref(area->cache_ref);
		free(area->name);
		free(area);
		area = NULL;
	}
	release_sem_etc(sourceAddressSpace->sem, WRITE_COUNT, 0);
err2:
	vm_put_address_space(targetAddressSpace);
err1:
	if (area != NULL)
		vm_put_area(area);
	return status;
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
clone_area(const char *name, void **_address, uint32 addressSpec, uint32 protection,
	area_id source)
{
	if ((protection & B_KERNEL_PROTECTION) == 0)
		protection |= B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA;

	return vm_clone_area(vm_kernel_address_space_id(), name, _address, addressSpec,
				protection, REGION_NO_PRIVATE_MAP, source);
}


area_id
create_area_etc(struct team *team, const char *name, void **address, uint32 addressSpec,
	uint32 size, uint32 lock, uint32 protection)
{
	fix_protection(&protection);

	return vm_create_anonymous_area(team->id, (char *)name, address, 
		addressSpec, size, lock, protection);
}


area_id
create_area(const char *name, void **_address, uint32 addressSpec, size_t size, uint32 lock,
	uint32 protection)
{
	fix_protection(&protection);

	return vm_create_anonymous_area(vm_kernel_address_space_id(), (char *)name, _address, 
		addressSpec, size, lock, protection);
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
	area_info info;
	status_t status;

	if (!IS_USER_ADDRESS(userInfo))
		return B_BAD_ADDRESS;

	status = get_area_info(area, &info);
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
	status_t status;
	area_info info;
	int32 cookie;

	if (!IS_USER_ADDRESS(userCookie)
		|| !IS_USER_ADDRESS(userInfo)
		|| user_memcpy(&cookie, userCookie, sizeof(int32)) < B_OK)
		return B_BAD_ADDRESS;

	status = _get_next_area_info(team, &cookie, &info, sizeof(area_info));
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


status_t
_user_transfer_area(area_id area, void **userAddress, uint32 addressSpec, team_id target)
{
	status_t status;
	void *address;

	// filter out some unavailable values (for userland)
	switch (addressSpec) {
		case B_ANY_KERNEL_ADDRESS:
		case B_ANY_KERNEL_BLOCK_ADDRESS:
			return B_BAD_VALUE;
	}

	if (!IS_USER_ADDRESS(userAddress)
		|| user_memcpy(&address, userAddress, sizeof(address)) < B_OK)
		return B_BAD_ADDRESS;

	status = transfer_area(area, &address, addressSpec, target);
	if (status < B_OK)
		return status;

	if (user_memcpy(userAddress, &address, sizeof(address)) < B_OK)
		return B_BAD_ADDRESS;

	return status;
}


area_id
_user_clone_area(const char *userName, void **userAddress, uint32 addressSpec,
	uint32 protection, area_id sourceArea)
{
	char name[B_OS_NAME_LENGTH];
	void *address;
	area_id clonedArea;

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

	clonedArea = vm_clone_area(vm_current_user_address_space_id(), name, &address,
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
	area_id area;
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

	area = vm_create_anonymous_area(vm_current_user_address_space_id(),
		(char *)name, &address, addressSpec, size, lock, protection);

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

