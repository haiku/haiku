/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <Errors.h>
#include <kernel.h>
#include <vm.h>
#include <vm_priv.h>
#include <vm_page.h>
#include <vm_cache.h>
#include <vm_store_anonymous_noswap.h>
#include <vm_store_device.h>
#include <vm_store_null.h>
#include <vm_store_vnode.h>
#include <memheap.h>
#include <debug.h>
#include <console.h>
#include <int.h>
#include <smp.h>
#include <OS.h>
#include <lock.h>
#include <khash.h>
#include <thread.h>
#include <atomic.h>

#include <stage2.h>

#include <arch/cpu.h>
#include <arch/vm.h>

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

#define HEAP_SIZE	0x00400000

#define ROUNDUP(a, b) (((a) + ((b)-1)) & ~((b)-1))
#define ROUNDOWN(a, b) (((a) / (b)) * (b))

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

static vm_address_space *kernel_aspace;

#define REGION_HASH_TABLE_SIZE 1024
static region_id next_region_id;
static void *region_table;
static sem_id region_hash_sem;

#define ASPACE_HASH_TABLE_SIZE 1024
static aspace_id next_aspace_id;
static void *aspace_table;
static sem_id aspace_hash_sem;

static int max_commit;
static spinlock_t max_commit_lock;

// function declarations
static vm_region *_vm_create_region_struct(vm_address_space *aspace, const char *name, int wiring, int lock);
static int map_backing_store(vm_address_space *aspace, vm_store *store, void **vaddr,
	off_t offset, addr size, int addr_type, int wiring, int lock, int mapping, vm_region **_region, const char *region_name);
static int vm_soft_fault(addr address, bool is_write, bool is_user);
static vm_region *vm_virtual_map_lookup(vm_virtual_map *map, addr address);
//static int vm_region_acquire_ref(vm_region *region);
//static void vm_region_release_ref(vm_region *region);
//static void vm_region_release_ref2(vm_region *region);

static int region_compare(void *_r, const void *key)
{
	vm_region *r = _r;
	const region_id *id = key;

	if(r->id == *id)
		return 0;
	else
		return -1;
}

static unsigned int region_hash(void *_r, const void *key, unsigned int range)
{
	vm_region *r = _r;
	const region_id *id = key;

	if(r != NULL)
		return (r->id % range);
	else
		return (*id % range);
}

static int aspace_compare(void *_a, const void *key)
{
	vm_address_space *aspace = _a;
	const aspace_id *id = key;

	if(aspace->id == *id)
		return 0;
	else
		return -1;
}

static unsigned int aspace_hash(void *_a, const void *key, unsigned int range)
{
	vm_address_space *aspace = _a;
	const aspace_id *id = key;

	if(aspace != NULL)
		return (aspace->id % range);
	else
		return (*id % range);
}

vm_address_space *vm_get_aspace_by_id(aspace_id aid)
{
	vm_address_space *aspace;

	acquire_sem_etc(aspace_hash_sem, READ_COUNT, 0, 0);
	aspace = hash_lookup(aspace_table, &aid);
	if(aspace)
		atomic_add(&aspace->ref_count, 1);
	release_sem_etc(aspace_hash_sem, READ_COUNT, 0);

	return aspace;
}

vm_region *vm_get_region_by_id(region_id rid)
{
	vm_region *region;

	acquire_sem_etc(region_hash_sem, READ_COUNT, 0, 0);
	region = hash_lookup(region_table, &rid);
	if(region)
		atomic_add(&region->ref_count, 1);
	release_sem_etc(region_hash_sem, READ_COUNT, 0);

	return region;
}

region_id vm_find_region_by_name(aspace_id aid, const char *name)
{
	vm_region *region = NULL;
	vm_address_space *aspace;
	region_id id = B_NAME_NOT_FOUND;

	aspace = vm_get_aspace_by_id(aid);
	if(aspace == NULL)
		return ERR_VM_INVALID_ASPACE;

	acquire_sem_etc(aspace->virtual_map.sem, READ_COUNT, 0, 0);

	region = aspace->virtual_map.region_list;
	while(region != NULL) {
		if(strcmp(region->name, name) == 0) {
			id = region->id;
			break;
		}
		region = region->aspace_next;
	}

	release_sem_etc(aspace->virtual_map.sem, READ_COUNT, 0);
	vm_put_aspace(aspace);
	return id;
}

static vm_region *_vm_create_region_struct(vm_address_space *aspace, const char *name, int wiring, int lock)
{
	vm_region *region = NULL;

	region = (vm_region *)kmalloc(sizeof(vm_region));
	if(region == NULL)
		return NULL;
	region->name = (char *)kmalloc(strlen(name) + 1);
	if(region->name == NULL) {
		kfree(region);
		return NULL;
	}
	strcpy(region->name, name);
	region->id = atomic_add(&next_region_id, 1);
	region->base = 0;
	region->size = 0;
	region->lock = lock;
	region->wiring = wiring;
	region->ref_count = 1;

	region->cache_ref = NULL;
	region->cache_offset = 0;

	region->aspace = aspace;
	region->aspace_next = NULL;
	region->map = &aspace->virtual_map;
	region->cache_next = region->cache_prev = NULL;
	region->hash_next = NULL;

	return region;
}

// must be called with this address space's virtual_map.sem held
static int find_and_insert_region_slot(vm_virtual_map *map, addr start, addr size, addr end, int addr_type, vm_region *region)
{
	vm_region *last_r = NULL;
	vm_region *next_r;
	bool foundspot = false;

	dprintf("find_and_insert_region_slot: map %p, start 0x%lx, size %ld, end 0x%lx, addr_type %d, region %p\n",
		map, start, size, end, addr_type, region);
//	dprintf("map->base 0x%x, map->size 0x%x\n", map->base, map->size);

	// do some sanity checking
	if(start < map->base || size == 0 || (end - 1) > (map->base + (map->size - 1)) || start + size > end)
		return ERR_VM_BAD_ADDRESS;

	// walk up to the spot where we should start searching
	next_r = map->region_list;
	while(next_r) {
		if(next_r->base >= start + size) {
			// we have a winner
			break;
		}
		last_r = next_r;
		next_r = next_r->aspace_next;
	}

#if 0
	dprintf("last_r 0x%x, next_r 0x%x\n", last_r, next_r);
	if(last_r) dprintf("last_r->base 0x%x, last_r->size 0x%x\n", last_r->base, last_r->size);
	if(next_r) dprintf("next_r->base 0x%x, next_r->size 0x%x\n", next_r->base, next_r->size);
#endif

	switch(addr_type) {
		case REGION_ADDR_ANY_ADDRESS:
			// find a hole big enough for a new region
			if(!last_r) {
				// see if we can build it at the beginning of the virtual map
				if(!next_r || (next_r->base >= map->base + size)) {
					foundspot = true;
					region->base = map->base;
					break;
				}
				last_r = next_r;
				next_r = next_r->aspace_next;
			}
			// keep walking
			while(next_r) {
				if(next_r->base >= last_r->base + last_r->size + size) {
					// we found a spot
					foundspot = true;
					region->base = last_r->base + last_r->size;
					break;
				}
				last_r = next_r;
				next_r = next_r->aspace_next;
			}
			if((map->base + (map->size - 1)) >= (last_r->base + last_r->size + (size - 1))) {
				// found a spot
				foundspot = true;
				region->base = last_r->base + last_r->size;
				break;
			}
			break;
		case REGION_ADDR_EXACT_ADDRESS:
			// see if we can create it exactly here
			if(!last_r) {
				if(!next_r || (next_r->base >= start + size)) {
					foundspot = true;
					region->base = start;
					break;
				}
			} else {
				if(next_r) {
					if(last_r->base + last_r->size <= start && next_r->base >= start + size) {
						foundspot = true;
						region->base = start;
						break;
					}
				} else {
					if((last_r->base + (last_r->size - 1)) <= start - 1) {
						foundspot = true;
						region->base = start;
					}
				}
			}
			break;
		default:
			return EINVAL;
	}

	if(foundspot) {
		region->size = size;
		if(last_r) {
			region->aspace_next = last_r->aspace_next;
			last_r->aspace_next = region;
		} else {
			region->aspace_next = map->region_list;
			map->region_list = region;
		}
		map->change_count++;
		return B_NO_ERROR;
	} else {
		return ERR_VM_NO_REGION_SLOT;
	}
}

// a ref to the cache holding this store must be held before entering here
static int map_backing_store(vm_address_space *aspace, vm_store *store, 
                             void **vaddr, off_t offset, addr size, 
                             int addr_type, int wiring, int lock, int mapping, 
                             vm_region **_region, const char *region_name)
{
	vm_cache *cache;
	vm_cache_ref *cache_ref;
	vm_region *region;
	vm_cache *nu_cache;
	vm_cache_ref *nu_cache_ref = NULL;
	vm_store *nu_store;

	int err;

//	dprintf("map_backing_store: aspace 0x%x, store 0x%x, *vaddr 0x%x, offset 0x%Lx, size %d, addr_type %d, wiring %d, lock %d, _region 0x%x, region_name '%s'\n",
//		aspace, store, *vaddr, offset, size, addr_type, wiring, lock, _region, region_name);

	region = _vm_create_region_struct(aspace, region_name, wiring, lock);
	if(!region)
		return ENOMEM;

	cache = store->cache;
	cache_ref = cache->ref;

	// if this is a private map, we need to create a new cache & store object
	// pair to handle the private copies of pages as they are written to
	if(mapping == REGION_PRIVATE_MAP) {
		// create an anonymous store object
		nu_store = vm_store_create_anonymous_noswap();
		if(nu_store == NULL)
			panic("map_backing_store: vm_create_store_anonymous_noswap returned NULL");
		nu_cache = vm_cache_create(nu_store);
		if(nu_cache == NULL)
			panic("map_backing_store: vm_cache_create returned NULL");
		nu_cache_ref = vm_cache_ref_create(nu_cache);
		if(nu_cache_ref == NULL)
			panic("map_backing_store: vm_cache_ref_create returned NULL");
		nu_cache->temporary = 1;
		nu_cache->scan_skip = 0;

		nu_cache->source = cache;

		// grab a ref to the cache object we're now linked to as a source
		vm_cache_acquire_ref(cache_ref, true);

		cache = nu_cache;
		cache_ref = cache->ref;
		store = nu_store;
	}

	mutex_lock(&cache_ref->lock);
	// If we don't have enough committed space to cover through to the new end of region...
	if(store->committed_size < offset + size) {
		// try to commit more memory
		off_t old_store_commitment = store->committed_size; // Note what we had
		off_t commitment = (store->ops->commit)(store, offset + size); // Commit through to the new end
		if(commitment < offset + size) { // Uh oh - didn't work
			if(cache->temporary) { // If this is a temporary cache, Check to see if we ran out of space and return error.
				int state = int_disable_interrupts();
				acquire_spinlock(&max_commit_lock);

				if (max_commit - old_store_commitment + commitment < offset + size) {
					release_spinlock(&max_commit_lock);
					int_restore_interrupts(state);
					mutex_unlock(&cache_ref->lock);
					err = ERR_VM_WOULD_OVERCOMMIT;
					goto err1a;
				}

				max_commit += (commitment - old_store_commitment) - (offset + size - cache->virtual_size);
				cache->virtual_size = offset + size;
				release_spinlock(&max_commit_lock);
				int_restore_interrupts(state);
			} else {
				mutex_unlock(&cache_ref->lock);
				err = ENOMEM;
				goto err1a;
			}
		}
	}

	mutex_unlock(&cache_ref->lock);

	vm_cache_acquire_ref(cache_ref, true);

	acquire_sem_etc(aspace->virtual_map.sem, WRITE_COUNT, 0, 0);

	// check to see if this aspace has entered DELETE state
	if(aspace->state == VM_ASPACE_STATE_DELETION) {
		// okay, someone is trying to delete this aspace now, so we can't
		// insert the region, so back out
		err = ERR_VM_INVALID_ASPACE;
		goto err1b;
	}

	{
		addr search_addr, search_end;

		if(addr_type == REGION_ADDR_EXACT_ADDRESS) {
			search_addr = (addr)*vaddr;
			search_end = (addr)*vaddr + size;
		} else if(addr_type == REGION_ADDR_ANY_ADDRESS) {
			search_addr = aspace->virtual_map.base;
			search_end = aspace->virtual_map.base + (aspace->virtual_map.size - 1);
		} else {
			err = EINVAL;
			goto err1b;
		}

		err = find_and_insert_region_slot(&aspace->virtual_map, 
		                                  search_addr, size, 
		                                  search_end, addr_type, 
		                                  region);
		if(err < 0)
			goto err1b;
		*vaddr = (addr *)region->base;
	}

	// attach the cache to the region
	region->cache_ref = cache_ref;
	region->cache_offset = offset;
	// point the cache back to the region
	vm_cache_insert_region(cache_ref, region);

	// insert the region in the global region hash table
	acquire_sem_etc(region_hash_sem, WRITE_COUNT, 0 ,0);
	hash_insert(region_table, region);
	release_sem_etc(region_hash_sem, WRITE_COUNT, 0);

	// grab a ref to the aspace (the region holds this)
	atomic_add(&aspace->ref_count, 1);

	release_sem_etc(aspace->virtual_map.sem, WRITE_COUNT, 0);

	*_region = region;

	return B_NO_ERROR;

err1b:
	release_sem_etc(aspace->virtual_map.sem, WRITE_COUNT, 0);
	vm_cache_release_ref(cache_ref);
	goto err;
err1a:
	if(nu_cache_ref) {
		// had never acquired it's initial ref, so acquire and then release it
		// this should clean up all the objects it references
		vm_cache_acquire_ref(cache_ref, true);
		vm_cache_release_ref(cache_ref);
	}
err:
	kfree(region->name);
	kfree(region);
	return err;
}

region_id user_vm_create_anonymous_region(char *uname, void **uaddress, int addr_type,
                                          addr size, int wiring, int lock)
{
	char name[SYS_MAX_OS_NAME_LEN];
	void *address;
	int rc, rc2;

	if((addr)uname >= KERNEL_BASE && (addr)uname <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;

	rc = user_strncpy(name, uname, SYS_MAX_OS_NAME_LEN-1);
	if(rc < 0)
		return rc;
	name[SYS_MAX_OS_NAME_LEN-1] = 0;

	rc = user_memcpy(&address, uaddress, sizeof(address));
	if(rc < 0)
		return rc;

	rc = vm_create_anonymous_region(vm_get_current_user_aspace_id(), name, &address, addr_type, size, wiring, lock);
	if(rc < 0)
		return rc;

	rc2 = user_memcpy(uaddress, &address, sizeof(address));
	if(rc2 < 0)
		return rc2;

	return rc;
}

region_id vm_create_anonymous_region(aspace_id aid, char *name, void **address, 
                                     int addr_type, addr size, int wiring, 
                                     int lock)
{
	int err;
	vm_region *region;
	vm_cache *cache;
	vm_store *store;
	vm_address_space *aspace;
	vm_cache_ref *cache_ref;

	dprintf("create_anonymous_region: %s: size 0x%lx\n", name, size);

	aspace = vm_get_aspace_by_id(aid);
	if(aspace == NULL)
		return ERR_VM_INVALID_ASPACE;

	size = PAGE_ALIGN(size);

	// create an anonymous store object
	store = vm_store_create_anonymous_noswap();
	if(store == NULL)
		panic("vm_create_anonymous_region: vm_create_store_anonymous_noswap returned NULL");
	cache = vm_cache_create(store);
	if(cache == NULL)
		panic("vm_create_anonymous_region: vm_cache_create returned NULL");
	cache_ref = vm_cache_ref_create(cache);
	if(cache_ref == NULL)
		panic("vm_create_anonymous_region: vm_cache_ref_create returned NULL");
	cache->temporary = 1;
	cache->scan_skip = 0;

//	dprintf("create_anonymous_region: calling map_backing store\n");

	vm_cache_acquire_ref(cache_ref, true);
	err = map_backing_store(aspace, store, address, 0, size, addr_type, wiring, lock, REGION_NO_PRIVATE_MAP, &region, name);
	vm_cache_release_ref(cache_ref);
	if(err < 0) {
		vm_put_aspace(aspace);
		return err;
	}

//	dprintf("create_anonymous_region: done calling map_backing store\n");

	cache_ref = store->cache->ref;
	switch(wiring) {
		case REGION_WIRING_LAZY:
			break; // do nothing
		case REGION_WIRING_WIRED: {
			// pages aren't mapped at this point, but we just simulate a fault on
			// every page, which should allocate them
			addr va;
			// XXX remove
			for(va = region->base; va < region->base + region->size; va += PAGE_SIZE) {
//				dprintf("mapping wired pages: region 0x%x, cache_ref 0x%x 0x%x\n", region, cache_ref, region->cache_ref);
				vm_soft_fault(va, false, false);
			}
			break;
		}
		case REGION_WIRING_WIRED_ALREADY: {
			// the pages should already be mapped. This is only really useful during
			// boot time. Find the appropriate vm_page objects and stick them in
			// the cache object.
			addr va;
			addr pa;
			unsigned int flags;
			int err;
			vm_page *page;
			off_t offset = 0;

			mutex_lock(&cache_ref->lock);
			(*aspace->translation_map.ops->lock)(&aspace->translation_map);
			for(va = region->base; va < region->base + region->size; va += PAGE_SIZE, offset += PAGE_SIZE) {
				err = (*aspace->translation_map.ops->query)(&aspace->translation_map,
					va, &pa, &flags);
				if(err < 0) {
//					dprintf("vm_create_anonymous_region: error looking up mapping for va 0x%x\n", va);
					continue;
				}
				page = vm_lookup_page(pa / PAGE_SIZE);
				if(page == NULL) {
//					dprintf("vm_create_anonymous_region: error looking up vm_page structure for pa 0x%x\n", pa);
					continue;
				}
				atomic_add(&page->ref_count, 1);
				vm_page_set_state(page, PAGE_STATE_WIRED);
				vm_cache_insert_page(cache_ref, page, offset);
			}
			(*aspace->translation_map.ops->unlock)(&aspace->translation_map);
			mutex_unlock(&cache_ref->lock);
			break;
		}
		case REGION_WIRING_WIRED_CONTIG: {
			addr va;
			addr phys_addr;
			int err;
			vm_page *page;
			off_t offset = 0;

			page = vm_page_allocate_page_run(PAGE_STATE_CLEAR, ROUNDUP(region->size, PAGE_SIZE) / PAGE_SIZE);
			if(page == NULL) {
				// XXX back out of this
				panic("couldn't allocate page run of size %ld\n", region->size);
			}
			phys_addr = page->ppn * PAGE_SIZE;

			mutex_lock(&cache_ref->lock);
			(*aspace->translation_map.ops->lock)(&aspace->translation_map);
			for(va = region->base; va < region->base + region->size; va += PAGE_SIZE, offset += PAGE_SIZE, phys_addr += PAGE_SIZE) {
				page = vm_lookup_page(phys_addr / PAGE_SIZE);
				if(page == NULL) {
					panic("couldn't lookup physical page just allocated\n");
				}
				atomic_add(&page->ref_count, 1);
				err = (*aspace->translation_map.ops->map)(&aspace->translation_map, va, phys_addr, lock);
				if(err < 0) {
					panic("couldn't map physical page in page run\n");
				}
				vm_page_set_state(page, PAGE_STATE_WIRED);
				vm_cache_insert_page(cache_ref, page, offset);
			}
			(*aspace->translation_map.ops->unlock)(&aspace->translation_map);
			mutex_unlock(&cache_ref->lock);

			break;
		}
		default:
			;
	}
	vm_put_aspace(aspace);

//	dprintf("create_anonymous_region: done\n");

	if(region)
		return region->id;
	else
		return ENOMEM;
}

region_id vm_map_physical_memory(aspace_id aid, char *name, void **address, int addr_type,
	addr size, int lock, addr phys_addr)
{
	vm_region *region;
	vm_cache *cache;
	vm_cache_ref *cache_ref;
	vm_store *store;
	addr map_offset;
	int err;

	vm_address_space *aspace = vm_get_aspace_by_id(aid);
	if(aspace == NULL)
		return ERR_VM_INVALID_ASPACE;

	// if the physical address is somewhat inside a page,
	// move the actual region down to align on a page boundary
	map_offset = phys_addr % PAGE_SIZE;
	size += map_offset;
	phys_addr -= map_offset;

	size = PAGE_ALIGN(size);

	// create an device store object
	store = vm_store_create_device(phys_addr);
	if(store == NULL)
		panic("vm_map_physical_memory: vm_store_create_device returned NULL");
	cache = vm_cache_create(store);
	if(cache == NULL)
		panic("vm_map_physical_memory: vm_cache_create returned NULL");
	cache_ref = vm_cache_ref_create(cache);
	if(cache_ref == NULL)
		panic("vm_map_physical_memory: vm_cache_ref_create returned NULL");
	// tell the page scanner to skip over this region, it's pages are special
	cache->scan_skip = 1;

	vm_cache_acquire_ref(cache_ref, true);
	err = map_backing_store(aspace, store, address, 0, size, addr_type, 0, lock, REGION_NO_PRIVATE_MAP, &region, name);
	vm_cache_release_ref(cache_ref);
	vm_put_aspace(aspace);
	if(err < 0) {
		return err;
	}

	// modify the pointer returned to be offset back into the new region
	// the same way the physical address in was offset
	(*address) += map_offset;
	return region->id;
}

region_id vm_create_null_region(aspace_id aid, char *name, void **address, int addr_type, addr size)
{
	vm_region *region;
	vm_cache *cache;
	vm_cache_ref *cache_ref;
	vm_store *store;
//	addr map_offset;
	int err;

	vm_address_space *aspace = vm_get_aspace_by_id(aid);
	if(aspace == NULL)
		return ERR_VM_INVALID_ASPACE;

	size = PAGE_ALIGN(size);

	// create an null store object
	store = vm_store_create_null();
	if(store == NULL)
		panic("vm_map_physical_memory: vm_store_create_null returned NULL");
	cache = vm_cache_create(store);
	if(cache == NULL)
		panic("vm_map_physical_memory: vm_cache_create returned NULL");
	cache_ref = vm_cache_ref_create(cache);
	if(cache_ref == NULL)
		panic("vm_map_physical_memory: vm_cache_ref_create returned NULL");
	// tell the page scanner to skip over this region, no pages will be mapped here
	cache->scan_skip = 1;

	vm_cache_acquire_ref(cache_ref, true);
	err = map_backing_store(aspace, store, address, 0, size, addr_type, 0, LOCK_RO, REGION_NO_PRIVATE_MAP, &region, name);
	vm_cache_release_ref(cache_ref);
	vm_put_aspace(aspace);
	if(err < 0)
		return err;

	return region->id;
}

static region_id _vm_map_file(aspace_id aid, char *name, void **address, int addr_type,
	addr size, int lock, int mapping, const char *path, off_t offset, bool kernel)
{
	vm_region *region;
	vm_cache *cache;
	vm_cache_ref *cache_ref;
	vm_store *store;
	void *v;
//	addr map_offset;
	int err;


	vm_address_space *aspace = vm_get_aspace_by_id(aid);
	if(aspace == NULL)
		return ERR_VM_INVALID_ASPACE;

	offset = ROUNDOWN(offset, PAGE_SIZE);
	size = PAGE_ALIGN(size);

restart:
	// get the vnode for the object, this also grabs a ref to it
	err = vfs_get_vnode_from_path(path, kernel, &v);
	if(err < 0) {
		vm_put_aspace(aspace);
		return err;
	}

	cache_ref = vfs_get_cache_ptr(v);
	if(!cache_ref) {
		// create a vnode store object
		store = vm_store_create_vnode(v);
		if(store == NULL)
			panic("vm_map_file: couldn't create vnode store");
		cache = vm_cache_create(store);
		if(cache == NULL)
			panic("vm_map_physical_memory: vm_cache_create returned NULL");
		cache_ref = vm_cache_ref_create(cache);
		if(cache_ref == NULL)
			panic("vm_map_physical_memory: vm_cache_ref_create returned NULL");

		// acquire the cache ref once to represent the ref that the vnode will have
		// this is one of the only places where we dont want to ref to ripple down to the store
		vm_cache_acquire_ref(cache_ref, false);

		// try to set the cache ptr in the vnode
		if(vfs_set_cache_ptr(v, cache_ref) < 0) {
			// the cache pointer was set between here and then
			// this can only happen if someone else tries to map it
			// at the same time. Rare enough to not worry about the
			// performance impact of undoing what we just did and retrying

			// this will delete the cache object and release the ref to the vnode we have
			vm_cache_release_ref(cache_ref);
			goto restart;
		}
	} else {
		cache = cache_ref->cache;
		store = cache->store;
	}

	// acquire a ref to the cache before we do work on it. Dont ripple the ref acquision to the vnode
	// below because we'll have to release it later anyway, since we grabbed a ref to the vnode at
	// vfs_get_vnode_from_path(). This puts the ref counts in sync.
	vm_cache_acquire_ref(cache_ref, false);
	err = map_backing_store(aspace, store, address, offset, size, addr_type, 0, lock, mapping, &region, name);
	vm_cache_release_ref(cache_ref);
	vm_put_aspace(aspace);
	if(err < 0) {
		return err;
	}

	// modify the pointer returned to be offset back into the new region
	// the same way the physical address in was offset
	return region->id;
}

region_id vm_map_file(aspace_id aid, char *name, void **address, int addr_type,
	addr size, int lock, int mapping, const char *path, off_t offset)
{
	return _vm_map_file(aid, name, address, addr_type, size, lock, mapping, path, offset, true);
}

region_id user_vm_map_file(char *uname, void **uaddress, int addr_type,
	addr size, int lock, int mapping, const char *upath, off_t offset)
{
	char name[SYS_MAX_OS_NAME_LEN];
	void *address;
	char path[SYS_MAX_PATH_LEN];
	int rc, rc2;

	if((addr)uname >= KERNEL_BASE && (addr)uname <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;

	if((addr)uaddress >= KERNEL_BASE && (addr)uaddress <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;

	if((addr)upath >= KERNEL_BASE && (addr)upath <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;

	rc = user_strncpy(name, uname, SYS_MAX_OS_NAME_LEN-1);
	if(rc < 0)
		return rc;
	name[SYS_MAX_OS_NAME_LEN-1] = 0;

	rc = user_strncpy(path, upath, SYS_MAX_PATH_LEN-1);
	if(rc < 0)
		return rc;
	path[SYS_MAX_PATH_LEN-1] = 0;

	rc = user_memcpy(&address, uaddress, sizeof(address));
	if(rc < 0)
		return rc;

	rc = _vm_map_file(vm_get_current_user_aspace_id(), name, &address, addr_type, size, lock, mapping, path, offset, false);
	if(rc < 0)
		return rc;

	rc2 = user_memcpy(uaddress, &address, sizeof(address));
	if(rc2 < 0)
		return rc2;

	return rc;
}

region_id user_vm_clone_region(char *uname, void **uaddress, int addr_type,
	region_id source_region, int mapping, int lock)
{
	char name[SYS_MAX_OS_NAME_LEN];
	void *address;
	int rc, rc2;

	if((addr)uname >= KERNEL_BASE && (addr)uname <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;

	if((addr)uaddress >= KERNEL_BASE && (addr)uaddress <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;

	rc = user_strncpy(name, uname, SYS_MAX_OS_NAME_LEN-1);
	if(rc < 0)
		return rc;
	name[SYS_MAX_OS_NAME_LEN-1] = 0;

	rc = user_memcpy(&address, uaddress, sizeof(address));
	if(rc < 0)
		return rc;

	rc = vm_clone_region(vm_get_current_user_aspace_id(), name, &address, addr_type, source_region, mapping, lock);
	if(rc < 0)
		return rc;

	rc2 = user_memcpy(uaddress, &address, sizeof(address));
	if(rc2 < 0)
		return rc2;

	return rc;
}

region_id vm_clone_region(aspace_id aid, char *name, void **address, int addr_type,
	region_id source_region, int mapping, int lock)
{
	vm_region *new_region;
	vm_region *src_region;
	int err;

	vm_address_space *aspace = vm_get_aspace_by_id(aid);
	if(aspace == NULL)
		return ERR_VM_INVALID_ASPACE;

	src_region = vm_get_region_by_id(source_region);
	if(src_region == NULL) {
		vm_put_aspace(aspace);
		return ERR_VM_INVALID_REGION;
	}

	vm_cache_acquire_ref(src_region->cache_ref, true);
	err = map_backing_store(aspace, src_region->cache_ref->cache->store, address, src_region->cache_offset, src_region->size,
		addr_type, src_region->wiring, lock, mapping, &new_region, name);
	vm_cache_release_ref(src_region->cache_ref);

	// release the ref on the old region
	vm_put_region(src_region);

	vm_put_aspace(aspace);

	if(err < 0)
		return err;
	else
		return new_region->id;
}

static int __vm_delete_region(vm_address_space *aspace, vm_region *region)
{
	if(region->aspace == aspace)
		vm_put_region(region);
	return B_NO_ERROR;
}

static int _vm_delete_region(vm_address_space *aspace, region_id rid)
{
//	vm_region *temp, *last = NULL;
	vm_region *region;

	dprintf("vm_delete_region: aspace id 0x%x, region id 0x%x\n", aspace->id, rid);

	region = vm_get_region_by_id(rid);
	if(region == NULL)
		return ERR_VM_INVALID_REGION;

	__vm_delete_region(aspace, region);
	vm_put_region(region);

	return 0;
}

int vm_delete_region(aspace_id aid, region_id rid)
{
	vm_address_space *aspace;
	int err;

	aspace = vm_get_aspace_by_id(aid);
	if(aspace == NULL)
		return ERR_VM_INVALID_ASPACE;

	err = _vm_delete_region(aspace, rid);
	vm_put_aspace(aspace);
	return err;
}

static void _vm_put_region(vm_region *region, bool aspace_locked)
{
	vm_region *temp, *last = NULL;
	vm_address_space *aspace;
	bool removeit = false;

	acquire_sem_etc(region_hash_sem, WRITE_COUNT, 0, 0);
	if(atomic_add(&region->ref_count, -1) == 1) {
		hash_remove(region_table, region);
		removeit = true;
	}
	release_sem_etc(region_hash_sem, WRITE_COUNT, 0);

	if(!removeit)
		return;

	aspace = region->aspace;

	// remove the region from the aspace's virtual map
	if(!aspace_locked)
		acquire_sem_etc(aspace->virtual_map.sem, WRITE_COUNT, 0, 0);
	temp = aspace->virtual_map.region_list;
	while(temp != NULL) {
		if(region == temp) {
			if(last != NULL) {
				last->aspace_next = temp->aspace_next;
			} else {
				aspace->virtual_map.region_list = temp->aspace_next;
			}
			aspace->virtual_map.change_count++;
			break;
		}
		last = temp;
		temp = temp->aspace_next;
	}
	if(region == aspace->virtual_map.region_hint)
		aspace->virtual_map.region_hint = NULL;
	if(!aspace_locked)
		release_sem_etc(aspace->virtual_map.sem, WRITE_COUNT, 0);

	if(temp == NULL)
		panic("vm_region_release_ref: region not found in aspace's region_list\n");

	vm_cache_remove_region(region->cache_ref, region);
	vm_cache_release_ref(region->cache_ref);

	(*aspace->translation_map.ops->lock)(&aspace->translation_map);
	(*aspace->translation_map.ops->unmap)(&aspace->translation_map, region->base,
		region->base + (region->size - 1));
	(*aspace->translation_map.ops->unlock)(&aspace->translation_map);

	// now we can give up the last ref to the aspace
	vm_put_aspace(aspace);

	if(region->name)
		kfree(region->name);
	kfree(region);

	return;
}

void vm_put_region(vm_region *region)
{
	return _vm_put_region(region, false);
}

int user_vm_get_region_info(region_id id, vm_region_info *uinfo)
{
	vm_region_info info;
	int rc, rc2;

	if((addr)uinfo >= KERNEL_BASE && (addr)uinfo <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;

	rc = vm_get_region_info(id, &info);
	if(rc < 0)
		return rc;

	rc2 = user_memcpy(uinfo, &info, sizeof(info));
	if(rc2 < 0)
		return rc2;

	return rc;
}

int vm_get_region_info(region_id id, vm_region_info *info)
{
	vm_region *region;

	if(info == NULL)
		return EINVAL;

	region = vm_get_region_by_id(id);
	if(region == NULL)
		return ERR_VM_INVALID_REGION;

	info->id = region->id;
	info->base = region->base;
	info->size = region->size;
	info->lock = region->lock;
	info->wiring = region->wiring;
	strncpy(info->name, region->name, SYS_MAX_OS_NAME_LEN-1);
	info->name[SYS_MAX_OS_NAME_LEN-1] = 0;

	vm_put_region(region);

	return 0;
}

int vm_get_page_mapping(aspace_id aid, addr vaddr, addr *paddr)
{
	vm_address_space *aspace;
	unsigned int null_flags;
	int err;

	aspace = vm_get_aspace_by_id(aid);
	if(aspace == NULL)
		return ERR_VM_INVALID_ASPACE;

	err = aspace->translation_map.ops->query(&aspace->translation_map,
		vaddr, paddr, &null_flags);
	vm_put_aspace(aspace);
	return err;
}

static void display_mem(int argc, char **argv)
{
	int item_size;
	int display_width;
	int num = 1;
	addr address;
	int i;
	int j;

	if(argc < 2) {
		dprintf("not enough arguments\n");
		return;
	}

	address = atoul(argv[1]);

	if(argc >= 3) {
		num = -1;
		num = atoi(argv[2]);
	}

	// build the format string
	if(strcmp(argv[0], "db") == 0) {
		item_size = 1;
		display_width = 16;
	} else if(strcmp(argv[0], "ds") == 0) {
		item_size = 2;
		display_width = 8;
	} else if(strcmp(argv[0], "dw") == 0) {
		item_size = 4;
		display_width = 4;
	} else {
		dprintf("display_mem called in an invalid way!\n");
		return;
	}

	dprintf("[0x%lx] '", address);
	for(j=0; j<min(display_width, num) * item_size; j++) {
		char c = *((char *)address + j);
		if(!isalnum(c)) {
			c = '.';
		}
		dprintf("%c", c);
	}
	dprintf("'");
	for(i=0; i<num; i++) {
		if((i % display_width) == 0 && i != 0) {
			dprintf("\n[0x%lx] '", address + i * item_size);
			for(j=0; j<min(display_width, (num-i)) * item_size; j++) {
				char c = *((char *)address + i * item_size + j);
				if(!isalnum(c)) {
					c = '.';
				}
				dprintf("%c", c);
			}
			dprintf("'");
		}

		switch(item_size) {
			case 1:
				dprintf(" 0x%02x", *((uint8 *)address + i));
				break;
			case 2:
				dprintf(" 0x%04x", *((uint16 *)address + i));
				break;
			case 4:
				dprintf(" 0x%08x", *((uint32 *)address + i));
				break;
			default:
				dprintf("huh?\n");
		}
	}
	dprintf("\n");
}

static void dump_cache_ref(int argc, char **argv)
{
	addr address;
	vm_region *region;
	vm_cache_ref *cache_ref;

	if(argc < 2) {
		dprintf("cache_ref: not enough arguments\n");
		return;
	}
	if(strlen(argv[1]) < 2 || argv[1][0] != '0' || argv[1][1] != 'x') {
		dprintf("cache_ref: invalid argument, pass address\n");
		return;
	}

	address = atoul(argv[1]);
	cache_ref = (vm_cache_ref *)address;

	dprintf("cache_ref at %p:\n", cache_ref);
	dprintf("cache: %p\n", cache_ref->cache);
	dprintf("lock.count: %d\n", cache_ref->lock.count);
	dprintf("lock.sem: 0x%x\n", cache_ref->lock.sem);
	dprintf("region_list:\n");
	for(region = cache_ref->region_list; region != NULL; region = region->cache_next) {
		dprintf(" region 0x%x: ", region->id);
		dprintf("base_addr = 0x%lx ", region->base);
		dprintf("size = 0x%lx ", region->size);
		dprintf("name = '%s' ", region->name);
		dprintf("lock = 0x%x\n", region->lock);
	}
	dprintf("ref_count: %d\n", cache_ref->ref_count);
}

static const char *page_state_to_text(int state)
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

static void dump_cache(int argc, char **argv)
{
	addr address;
	vm_cache *cache;
	vm_page *page;

	if(argc < 2) {
		dprintf("cache: not enough arguments\n");
		return;
	}
	if(strlen(argv[1]) < 2 || argv[1][0] != '0' || argv[1][1] != 'x') {
		dprintf("cache: invalid argument, pass address\n");
		return;
	}

	address = atoul(argv[1]);
	cache = (vm_cache *)address;

	dprintf("cache at %p:\n", cache);
	dprintf("cache_ref: %p\n", cache->ref);
	dprintf("source: %p\n", cache->source);
	dprintf("store: %p\n", cache->store);
	// XXX 64-bit
	dprintf("virtual_size: 0x%Lx\n", cache->virtual_size);
	dprintf("temporary: %d\n", cache->temporary);
	dprintf("scan_skip: %d\n", cache->scan_skip);
	dprintf("page_list:\n");
	for(page = cache->page_list; page != NULL; page = page->cache_next) {
		// XXX offset is 64-bit
		if(page->type == PAGE_TYPE_PHYSICAL)
			dprintf(" %p ppn 0x%lx offset 0x%Lx type %d state %d (%s) ref_count %d\n",
				page, page->ppn, page->offset, page->type, page->state, page_state_to_text(page->state), page->ref_count);
		else if(page->type == PAGE_TYPE_DUMMY)
			dprintf(" %p DUMMY PAGE state %d (%s)\n", page, page->state, page_state_to_text(page->state));
		else
			dprintf(" %p UNKNOWN PAGE type %d\n", page, page->type);
	}
}

static void _dump_region(vm_region *region)
{
	dprintf("dump of region at %p:\n", region);
	dprintf("name: '%s'\n", region->name);
	dprintf("id: 0x%x\n", region->id);
	dprintf("base: 0x%lx\n", region->base);
	dprintf("size: 0x%lx\n", region->size);
	dprintf("lock: 0x%x\n", region->lock);
	dprintf("wiring: 0x%x\n", region->wiring);
	dprintf("ref_count: %d\n", region->ref_count);
	dprintf("cache_ref: %p\n", region->cache_ref);
	// XXX 64-bit
	dprintf("cache_offset: 0x%Lx\n", region->cache_offset);
	dprintf("cache_next: %p\n", region->cache_next);
	dprintf("cache_prev: %p\n", region->cache_prev);
}

static void dump_region(int argc, char **argv)
{
//	int i;
	vm_region *region;

	if(argc < 2) {
		dprintf("region: not enough arguments\n");
		return;
	}

	// if the argument looks like a hex number, treat it as such
	if(strlen(argv[1]) > 2 && argv[1][0] == '0' && argv[1][1] == 'x') {
		unsigned long num = atoul(argv[1]);
		region_id id = num;

		region = hash_lookup(region_table, &id);
		if(region == NULL) {
			dprintf("invalid region id\n");
		} else {
			_dump_region(region);
		}
		return;
	} else {
		// walk through the region list, looking for the arguments as a name
		struct hash_iterator iter;

		hash_open(region_table, &iter);
		while((region = hash_next(region_table, &iter)) != NULL) {
			if(region->name != NULL && strcmp(argv[1], region->name) == 0) {
				_dump_region(region);
			}
		}
	}
}

region_id find_region_by_address (addr vaddress)
{
	vm_address_space *aspace;
	vm_region *region;
	region_id result=B_ERROR;

	aspace = vm_get_current_user_aspace();
	for(region = aspace->virtual_map.region_list; region != NULL; region = region->aspace_next) 
		{
		if ((vaddress>=region->base) && (vaddress<=(region->base+region->size)))
			result=region->id;
		}
	vm_put_aspace(aspace);
	return result;
}

region_id find_region_by_name(const char *name)
{
	vm_region *region;
	struct hash_iterator iter;
	hash_open(region_table, &iter);
	while((region = hash_next(region_table, &iter)) != NULL) 
		{
		if (!strcmp(region->name,name))
			return region->id;
		}
	hash_close(region_table, &iter, false);
	return B_NAME_NOT_FOUND;
}

static void dump_region_list(int argc, char **argv)
{
	vm_region *region;
	struct hash_iterator iter;

	dprintf("addr\tid\t%32s\tbase\t\tsize\tlock\twiring\n", "name");

	hash_open(region_table, &iter);
	while((region = hash_next(region_table, &iter)) != NULL) {
		dprintf("%p\t0x%x\t%32s\t0x%lx\t\t0x%lx\t%d\t%d\n",
			region, region->id, region->name, region->base, region->size, region->lock, region->wiring);
	}
	hash_close(region_table, &iter, false);
}

static void _dump_aspace(vm_address_space *aspace)
{
	vm_region *region;

	dprintf("dump of address space at %p:\n", aspace);
	dprintf("name: '%s'\n", aspace->name);
	dprintf("id: 0x%x\n", aspace->id);
	dprintf("ref_count: %d\n", aspace->ref_count);
	dprintf("fault_count: %d\n", aspace->fault_count);
	dprintf("working_set_size: 0x%lx\n", aspace->working_set_size);
	dprintf("translation_map: %p\n", &aspace->translation_map);
	dprintf("virtual_map.base: 0x%lx\n", aspace->virtual_map.base);
	dprintf("virtual_map.size: 0x%lx\n", aspace->virtual_map.size);
	dprintf("virtual_map.change_count: 0x%x\n", aspace->virtual_map.change_count);
	dprintf("virtual_map.sem: 0x%x\n", aspace->virtual_map.sem);
	dprintf("virtual_map.region_hint: %p\n", aspace->virtual_map.region_hint);
	dprintf("virtual_map.region_list:\n");
	for(region = aspace->virtual_map.region_list; region != NULL; region = region->aspace_next) {
		dprintf(" region 0x%x: ", region->id);
		dprintf("base_addr = 0x%lx ", region->base);
		dprintf("size = 0x%lx ", region->size);
		dprintf("name = '%s' ", region->name);
		dprintf("lock = 0x%x\n", region->lock);
	}
}

static void dump_aspace(int argc, char **argv)
{
//	int i;
	vm_address_space *aspace;

	if(argc < 2) {
		dprintf("aspace: not enough arguments\n");
		return;
	}

	// if the argument looks like a hex number, treat it as such
	if(strlen(argv[1]) > 2 && argv[1][0] == '0' && argv[1][1] == 'x') {
		unsigned long num = atoul(argv[1]);
		aspace_id id = num;

		aspace = hash_lookup(aspace_table, &id);
		if(aspace == NULL) {
			dprintf("invalid aspace id\n");
		} else {
			_dump_aspace(aspace);
		}
		return;
	} else {
		// walk through the aspace list, looking for the arguments as a name
		struct hash_iterator iter;

		hash_open(aspace_table, &iter);
		while((aspace = hash_next(aspace_table, &iter)) != NULL) {
			if(aspace->name != NULL && strcmp(argv[1], aspace->name) == 0) {
				_dump_aspace(aspace);
			}
		}
	}
}

static void dump_aspace_list(int argc, char **argv)
{
	vm_address_space *as;
	struct hash_iterator iter;

	dprintf("addr\tid\t%32s\tbase\t\tsize\n", "name");

	hash_open(aspace_table, &iter);
	while((as = hash_next(aspace_table, &iter)) != NULL) {
		dprintf("%p\t0x%x\t%32s\t0x%lx\t\t0x%lx\n",
			as, as->id, as->name, as->virtual_map.base, as->virtual_map.size);
	}
	hash_close(aspace_table, &iter, false);
}

vm_address_space *vm_get_kernel_aspace(void)
{
	/* we can treat this one a little differently since it can't be deleted */
	acquire_sem_etc(aspace_hash_sem, READ_COUNT, 0, 0);
	atomic_add(&kernel_aspace->ref_count, 1);
	release_sem_etc(aspace_hash_sem, READ_COUNT, 0);
	return kernel_aspace;
}

aspace_id vm_get_kernel_aspace_id(void)
{
	return kernel_aspace->id;
}

vm_address_space *vm_get_current_user_aspace(void)
{
	return vm_get_aspace_by_id(vm_get_current_user_aspace_id());
}

aspace_id vm_get_current_user_aspace_id(void)
{
	struct thread *t = thread_get_current_thread();

	if(t)
		return t->proc->_aspace_id;
	else
		return -1;
}

void vm_put_aspace(vm_address_space *aspace)
{
//	vm_region *region;
	bool removeit = false;

	acquire_sem_etc(aspace_hash_sem, WRITE_COUNT, 0, 0);
	if(atomic_add(&aspace->ref_count, -1) == 1) {
		hash_remove(aspace_table, aspace);
		removeit = true;
	}
	release_sem_etc(aspace_hash_sem, WRITE_COUNT, 0);

	if(!removeit)
		return;

	dprintf("vm_put_aspace: reached zero ref, deleting aspace\n");

	if(aspace == kernel_aspace)
		panic("vm_put_aspace: tried to delete the kernel aspace!\n");

	if(aspace->virtual_map.region_list)
		panic("vm_put_aspace: aspace at %p has zero ref count, but region list isn't empty!\n", aspace);

	(*aspace->translation_map.ops->destroy)(&aspace->translation_map);

	kfree(aspace->name);
	delete_sem(aspace->virtual_map.sem);
	kfree(aspace);

	return;
}

aspace_id vm_create_aspace(const char *name, addr base, addr size, bool kernel)
{
	vm_address_space *aspace;
	int err;


	aspace = (vm_address_space *)kmalloc(sizeof(vm_address_space));
	if(aspace == NULL)
		return ENOMEM;

	dprintf("vm_create_aspace: %s: %lx bytes starting at 0x%lx => %p\n", name, size, base, aspace);

	aspace->name = (char *)kmalloc(strlen(name) + 1);
	if(aspace->name == NULL ) {
		kfree(aspace);
		return ENOMEM;
	}
	strcpy(aspace->name, name);

	aspace->id = next_aspace_id++;
	aspace->ref_count = 1;
	aspace->state = VM_ASPACE_STATE_NORMAL;
	aspace->fault_count = 0;
	aspace->scan_va = base;
	aspace->working_set_size = kernel ? DEFAULT_KERNEL_WORKING_SET : DEFAULT_WORKING_SET;
	aspace->max_working_set = DEFAULT_MAX_WORKING_SET;
	aspace->min_working_set = DEFAULT_MIN_WORKING_SET;
	aspace->last_working_set_adjust = system_time();

	// initialize the corresponding translation map
	err = vm_translation_map_create(&aspace->translation_map, kernel);
	if(err < 0) {
		kfree(aspace->name);
		kfree(aspace);
		return err;
	}

	// initialize the virtual map
	aspace->virtual_map.base = base;
	aspace->virtual_map.size = size;
	aspace->virtual_map.region_list = NULL;
	aspace->virtual_map.region_hint = NULL;
	aspace->virtual_map.change_count = 0;
	aspace->virtual_map.sem = create_sem(WRITE_COUNT, "aspacelock");
	aspace->virtual_map.aspace = aspace;

	// add the aspace to the global hash table
	acquire_sem_etc(aspace_hash_sem, WRITE_COUNT, 0, 0);
	hash_insert(aspace_table, aspace);
	release_sem_etc(aspace_hash_sem, WRITE_COUNT, 0);

	return aspace->id;
}

int vm_delete_aspace(aspace_id aid)
{
	vm_region *region;
	vm_region *next;
	vm_address_space *aspace;

	aspace = vm_get_aspace_by_id(aid);
	if(aspace == NULL)
		return ERR_VM_INVALID_ASPACE;

	dprintf("vm_delete_aspace: called on aspace 0x%x\n", aid);

	// put this aspace in the deletion state
	// this guarantees that no one else will add regions to the list
	acquire_sem_etc(aspace->virtual_map.sem, WRITE_COUNT, 0, 0);
	if(aspace->state == VM_ASPACE_STATE_DELETION) {
		// abort, someone else is already deleting this aspace
		release_sem_etc(aspace->virtual_map.sem, WRITE_COUNT, 0);
		vm_put_aspace(aspace);
		return B_NO_ERROR;
	}
	aspace->state = VM_ASPACE_STATE_DELETION;

	// delete all the regions in this aspace
	region = aspace->virtual_map.region_list;
	while(region) {
		next = region->aspace_next;
		// decrement the ref on this region, may actually push the ref < 0, but that's okay
		_vm_put_region(region, true);
		region = next;
	}

	// unlock
	release_sem_etc(aspace->virtual_map.sem, WRITE_COUNT, 0);

	// release two refs on the address space
	vm_put_aspace(aspace);
	vm_put_aspace(aspace);

	return B_NO_ERROR;
}

int vm_resize_region (aspace_id aid, region_id rid, size_t newSize)
{
	vm_cache_ref *myCacheRef;
	vm_region *myRegion,*current;
	size_t oldSize;
	bool failed=false;
// Steps:
//1) Get the vm_cache_ref for the region 
	myRegion=vm_get_region_by_id(rid);

	if (!myRegion)
		return B_ERROR;

	myCacheRef=myRegion->cache_ref;
// 2) Resize all of the regions from the vm_cache_ref (fix them all and fail if they can't be resized)
	oldSize = myRegion->size;
	for (current=myCacheRef->region_list;current;current=current->cache_next)
		{
		if (current->aspace_next->base<=(current->base+newSize))
			{
			failed=true;
			break;
			}
		current->size=newSize;
		}
	if (failed) // OH NO! Go back and fix all of the broken ones...
		{
		for (current=myCacheRef->region_list;current;current=current->cache_next)
			current->size=oldSize;
		return B_ERROR;
		}
// 3) Update the vm_cache size
	myCacheRef->cache->virtual_size=newSize;
}

int vm_aspace_walk_start(struct hash_iterator *i)
{
	hash_open(aspace_table, i);
	return 0;
}

vm_address_space *vm_aspace_walk_next(struct hash_iterator *i)
{
	vm_address_space *aspace;

	acquire_sem_etc(aspace_hash_sem, READ_COUNT, 0, 0);
	aspace = hash_next(aspace_table, i);
	if(aspace)
		atomic_add(&aspace->ref_count, 1);
	release_sem_etc(aspace_hash_sem, READ_COUNT, 0);
	return aspace;
}

static int vm_thread_dump_max_commit(void *unused)
{
	int oldmax = -1;

	(void)(unused);

	for(;;) {
		thread_snooze(1000000);
		if(oldmax != max_commit)
			dprintf("max_commit 0x%x\n", max_commit);
		oldmax = max_commit;
	}
}

int vm_init(kernel_args *ka)
{
	int err = 0;
	unsigned int i;
//	int last_used_virt_range = -1;
//	int last_used_phys_range = -1;
	addr heap_base;
	void *null_addr;

	dprintf("vm_init: entry\n");
	err = vm_translation_map_module_init(ka);
	err = arch_vm_init(ka);

	// initialize some globals
	kernel_aspace = NULL;
	next_region_id = 0;
	region_hash_sem = -1;
	next_aspace_id = 0;
	aspace_hash_sem = -1;
	max_commit = 0; // will be increased in vm_page_init
	max_commit_lock = 0;

	// map in the new heap and initialize it
	heap_base = vm_alloc_from_ka_struct(ka, HEAP_SIZE, LOCK_KERNEL|LOCK_RW);
	dprintf("heap at 0x%lx\n", heap_base);
	heap_init(heap_base, HEAP_SIZE);

	// initialize the free page list and physical page mapper
	vm_page_init(ka);

	// initialize the hash table that stores the pages mapped to caches
	vm_cache_init(ka);

	// create the region and address space hash tables
	{
		vm_address_space *aspace;
		aspace_table = hash_init(ASPACE_HASH_TABLE_SIZE, (addr)&aspace->hash_next - (addr)aspace,
			&aspace_compare, &aspace_hash);
		if(aspace_table == NULL)
			panic("vm_init: error creating aspace hash table\n");
	}
	{
		vm_region *region;
		region_table = hash_init(REGION_HASH_TABLE_SIZE, (addr)&region->hash_next - (addr)region,
			&region_compare, &region_hash);
		if(region_table == NULL)
			panic("vm_init: error creating aspace hash table\n");
	}


	// create the initial kernel address space
	{
		aspace_id aid;
		aid = vm_create_aspace("kernel_land", KERNEL_BASE, KERNEL_SIZE, true);
		if(aid < 0)
			panic("vm_init: error creating kernel address space!\n");
		kernel_aspace = vm_get_aspace_by_id(aid);
		vm_put_aspace(kernel_aspace);
	}

	// do any further initialization that the architecture dependant layers may need now
	vm_translation_map_module_init2(ka);
	arch_vm_init2(ka);
	vm_page_init2(ka);

	// allocate regions to represent stuff that already exists
	null_addr = (void *)ROUNDOWN(heap_base, PAGE_SIZE);
	vm_create_anonymous_region(vm_get_kernel_aspace_id(), "kernel_heap", &null_addr, REGION_ADDR_EXACT_ADDRESS,
		HEAP_SIZE, REGION_WIRING_WIRED_ALREADY, LOCK_RW|LOCK_KERNEL);

	null_addr = (void *)ROUNDOWN(ka->kernel_seg0_addr.start, PAGE_SIZE);
	vm_create_anonymous_region(vm_get_kernel_aspace_id(), "kernel_ro", &null_addr, REGION_ADDR_EXACT_ADDRESS,
		PAGE_ALIGN(ka->kernel_seg0_addr.size), REGION_WIRING_WIRED_ALREADY, LOCK_RW|LOCK_KERNEL);

	if(ka->kernel_seg1_addr.size > 0) {
		null_addr = (void *)ROUNDOWN(ka->kernel_seg1_addr.start, PAGE_SIZE);
		vm_create_anonymous_region(vm_get_kernel_aspace_id(), "kernel_rw", &null_addr, REGION_ADDR_EXACT_ADDRESS,
			PAGE_ALIGN(ka->kernel_seg1_addr.size), REGION_WIRING_WIRED_ALREADY, LOCK_RW|LOCK_KERNEL);
	}
	for(i=0; i < ka->num_cpus; i++) {
		char temp[64];

		sprintf(temp, "idle_thread%d_kstack", i);
		null_addr = (void *)ka->cpu_kstack[i].start;
		vm_create_anonymous_region(vm_get_kernel_aspace_id(), temp, &null_addr, REGION_ADDR_EXACT_ADDRESS,
			ka->cpu_kstack[i].size, REGION_WIRING_WIRED_ALREADY, LOCK_RW|LOCK_KERNEL);
	}
	{
		void *null;
		vm_map_physical_memory(vm_get_kernel_aspace_id(), "bootdir", &null, REGION_ADDR_ANY_ADDRESS,
			ka->bootdir_addr.size, LOCK_RO|LOCK_KERNEL, ka->bootdir_addr.start);
	}

	arch_vm_init_endvm(ka);

	// add some debugger commands
	dbg_add_command(&dump_region_list, "regions", "Dump a list of all regions");
	dbg_add_command(&dump_region, "region", "Dump info about a particular region");
	dbg_add_command(&dump_aspace_list, "aspaces", "Dump a list of all address spaces");
	dbg_add_command(&dump_aspace, "aspace", "Dump info about a particular address space");
	dbg_add_command(&dump_cache_ref, "cache_ref", "Dump cache_ref data structure");
	dbg_add_command(&dump_cache, "cache", "Dump cache_ref data structure");
//	dbg_add_command(&display_mem, "dl", "dump memory long words (64-bit)");
	dbg_add_command(&display_mem, "dw", "dump memory words (32-bit)");
	dbg_add_command(&display_mem, "ds", "dump memory shorts (16-bit)");
	dbg_add_command(&display_mem, "db", "dump memory bytes (8-bit)");

	dprintf("vm_init: exit\n");

	return err;
}

int vm_init_postsem(kernel_args *ka)
{
	vm_region *region;

	// fill in all of the semaphores that were not allocated before
	// since we're still single threaded and only the kernel address space exists,
	// it isn't that hard to find all of the ones we need to create
	vm_translation_map_module_init_post_sem(ka);
	kernel_aspace->virtual_map.sem = create_sem(WRITE_COUNT, "kernel_aspacelock");
	kernel_aspace->translation_map.lock.sem = create_sem(1, "recursive_lock");

	for(region = kernel_aspace->virtual_map.region_list; region; region = region->aspace_next) {
		if(region->cache_ref->lock.sem < 0) {
			region->cache_ref->lock.sem = create_sem(1, "cache_ref_mutex");
		}
	}

	region_hash_sem = create_sem(WRITE_COUNT, "region_hash_sem");
	aspace_hash_sem = create_sem(WRITE_COUNT, "aspace_hash_sem");

	return heap_init_postsem(ka);
}

int vm_init_postthread(kernel_args *ka)
{
	vm_page_init_postthread(ka);

	{
		thread_id tid = thread_create_kernel_thread("max_commit_thread", &vm_thread_dump_max_commit, NULL);
		thread_resume_thread(tid);
	}

	vm_daemon_init();

	return 0;
}

int vm_page_fault(addr address, addr fault_address, bool is_write, bool is_user, addr *newip)
{
	int err;

//	dprintf("vm_page_fault: page fault at 0x%x, ip 0x%x\n", address, fault_address);

	*newip = 0;

	err = vm_soft_fault(address, is_write, is_user);
	if(err < 0) {
		dprintf("vm_page_fault: vm_soft_fault returned error %d on fault at 0x%lx, ip 0x%lx, write %d, user %d, thread 0x%x\n",
			err, address, fault_address, is_write, is_user, thread_get_current_thread_id());
		if(!is_user) {
			struct thread *t = thread_get_current_thread();
			if(t && t->fault_handler != 0) {
				// this will cause the arch dependant page fault handler to
				// modify the IP on the interrupt frame or whatever to return
				// to this address
				*newip = t->fault_handler;
			} else {
				// unhandled page fault in the kernel
				panic("vm_page_fault: unhandled page fault in kernel space at 0x%lx, ip 0x%lx\n",
					address, fault_address);
			}
		} else {
			dprintf("vm_page_fault: killing process 0x%x\n", thread_get_current_thread()->proc->id);
			proc_kill_proc(thread_get_current_thread()->proc->id);
		}
	}

	return INT_NO_RESCHEDULE;
}

#define TRACE_PFAULT 0

#if TRACE_PFAULT
#define TRACE dprintf("in pfault at line %d\n", __LINE__)
#else
#define TRACE
#endif

static int vm_soft_fault(addr address, bool is_write, bool is_user)
{
	vm_address_space *aspace;
	vm_virtual_map *map;
	vm_region *region;
	vm_cache_ref *cache_ref;
	vm_cache_ref *last_cache_ref;
	vm_cache_ref *top_cache_ref;
	off_t cache_offset;
	vm_page dummy_page;
	vm_page *page = NULL;
	int change_count;
	int err;

//	dprintf("vm_soft_fault: thid 0x%x address 0x%x, is_write %d, is_user %d\n",
//		thread_get_current_thread_id(), address, is_write, is_user);

	address = ROUNDOWN(address, PAGE_SIZE);

	if(address >= KERNEL_BASE && address <= KERNEL_TOP) {
		aspace = vm_get_kernel_aspace();
	} else if(address >= USER_BASE && address <= USER_TOP) {
		aspace = vm_get_current_user_aspace();
		if(aspace == NULL) {
			if(is_user == false) {
				dprintf("vm_soft_fault: kernel thread accessing invalid user memory!\n");
				return ERR_VM_PF_FATAL;
			} else {
				// XXX weird state.
				panic("vm_soft_fault: non kernel thread accessing user memory that doesn't exist!\n");
			}
		}
	} else {
		// the hit was probably in the 64k DMZ between kernel and user space
		// this keeps a user space thread from passing a buffer that crosses into kernel space
		return ERR_VM_PF_FATAL;
	}
	map = &aspace->virtual_map;
	atomic_add(&aspace->fault_count, 1);

	acquire_sem_etc(map->sem, READ_COUNT, 0, 0);
	region = vm_virtual_map_lookup(map, address);
	if(region == NULL) {
		release_sem_etc(map->sem, READ_COUNT, 0);
		vm_put_aspace(aspace);
		dprintf("vm_soft_fault: va 0x%lx not covered by region in address space\n", address);
		return ERR_VM_PF_BAD_ADDRESS; // BAD_ADDRESS
	}

	// check permissions
	if(is_user && (region->lock & LOCK_KERNEL) == LOCK_KERNEL) {
		release_sem_etc(map->sem, READ_COUNT, 0);
		vm_put_aspace(aspace);
		dprintf("user access on kernel region\n");
		return ERR_VM_PF_BAD_PERM; // BAD_PERMISSION
	}
	if(is_write && (region->lock & LOCK_RW) == 0) {
		release_sem_etc(map->sem, READ_COUNT, 0);
		vm_put_aspace(aspace);
		dprintf("write access attempted on read-only region\n");
		return ERR_VM_PF_BAD_PERM; // BAD_PERMISSION
	}

	TRACE;

	top_cache_ref = region->cache_ref;
	cache_offset = address - region->base + region->cache_offset;
	vm_cache_acquire_ref(top_cache_ref, true);
	change_count = map->change_count;
	release_sem_etc(map->sem, READ_COUNT, 0);

	// see if this cache has a fault handler
	if(top_cache_ref->cache->store->ops->fault) {
		int err = (*top_cache_ref->cache->store->ops->fault)(top_cache_ref->cache->store, aspace, cache_offset);
		vm_cache_release_ref(top_cache_ref);
		vm_put_aspace(aspace);
		return err;
	}

	TRACE;

	dummy_page.state = PAGE_STATE_INACTIVE;
	dummy_page.type = PAGE_TYPE_DUMMY;

	last_cache_ref = top_cache_ref;
	for(cache_ref = top_cache_ref; cache_ref; cache_ref = (cache_ref->cache->source) ? cache_ref->cache->source->ref : NULL) {
		mutex_lock(&cache_ref->lock);

		TRACE;

		for(;;) {
			page = vm_cache_lookup_page(cache_ref, cache_offset);
			if(page != NULL && page->state != PAGE_STATE_BUSY) {
				vm_page_set_state(page, PAGE_STATE_BUSY);
				mutex_unlock(&cache_ref->lock);
				break;
			}

			if(page == NULL)
				break;

			TRACE;

			// page must be busy
			mutex_unlock(&cache_ref->lock);
			thread_snooze(20000);
			mutex_lock(&cache_ref->lock);
		}

		TRACE;

		if(page != NULL)
			break;

		TRACE;

		// insert this dummy page here to keep other threads from faulting on the
		// same address and chasing us up the cache chain
		if(cache_ref == top_cache_ref) {
			dummy_page.state = PAGE_STATE_BUSY;
			vm_cache_insert_page(cache_ref, &dummy_page, cache_offset);
		}

		// see if the vm_store has it
		if(cache_ref->cache->store->ops->has_page) {
			if(cache_ref->cache->store->ops->has_page(cache_ref->cache->store, cache_offset)) {
				IOVECS(vecs, 1);

				TRACE;

				mutex_unlock(&cache_ref->lock);

				vecs->num = 1;
				vecs->total_len = PAGE_SIZE;
				vecs->vec[0].iov_len = PAGE_SIZE;

				page = vm_page_allocate_page(PAGE_STATE_FREE);
				(*aspace->translation_map.ops->get_physical_page)(page->ppn * PAGE_SIZE, (addr *)&vecs->vec[0].iov_base, PHYSICAL_PAGE_CAN_WAIT);
				// handle errors here
				err = cache_ref->cache->store->ops->read(cache_ref->cache->store, cache_offset, vecs);
				(*aspace->translation_map.ops->put_physical_page)((addr)vecs->vec[0].iov_base);

				mutex_lock(&cache_ref->lock);

				if(cache_ref == top_cache_ref) {
					vm_cache_remove_page(cache_ref, &dummy_page);
					dummy_page.state = PAGE_STATE_INACTIVE;
				}
				vm_cache_insert_page(cache_ref, page, cache_offset);
				mutex_unlock(&cache_ref->lock);
				break;
			}
		}
		mutex_unlock(&cache_ref->lock);
		last_cache_ref = cache_ref;
		TRACE;
	}

	TRACE;

	// we rolled off the end of the cache chain, so we need to decide which
	// cache will get the new page we're about to create
	if(!cache_ref) {
		if(!is_write)
			cache_ref = last_cache_ref; // put it in the deepest cache
		else
			cache_ref = top_cache_ref; // put it in the topmost cache
	}

	TRACE;

	if(page == NULL) {
		// still haven't found a page, so zero out a new one
		page = vm_page_allocate_page(PAGE_STATE_CLEAR);
//		dprintf("vm_soft_fault: just allocated page 0x%x\n", page->ppn);
		mutex_lock(&cache_ref->lock);
		if(dummy_page.state == PAGE_STATE_BUSY && dummy_page.cache_ref == cache_ref) {
			vm_cache_remove_page(cache_ref, &dummy_page);
			dummy_page.state = PAGE_STATE_INACTIVE;
		}
		vm_cache_insert_page(cache_ref, page, cache_offset);
		mutex_unlock(&cache_ref->lock);
		if(dummy_page.state == PAGE_STATE_BUSY) {
			vm_cache_ref *temp_cache = dummy_page.cache_ref;
			mutex_lock(&temp_cache->lock);
			vm_cache_remove_page(temp_cache, &dummy_page);
			mutex_unlock(&temp_cache->lock);
			dummy_page.state = PAGE_STATE_INACTIVE;
		}
	}

	TRACE;

	if(page->cache_ref != top_cache_ref && is_write) {
		// now we have a page that has the data we want, but in the wrong cache object
		// so we need to copy it and stick it into the top cache
		vm_page *src_page = page;
		void *src, *dest;

		page = vm_page_allocate_page(PAGE_STATE_FREE);

		// try to get a mapping for the src and dest page so we can copy it
		for(;;) {
			(*aspace->translation_map.ops->get_physical_page)(src_page->ppn * PAGE_SIZE, (addr *)&src, PHYSICAL_PAGE_CAN_WAIT);
			err = (*aspace->translation_map.ops->get_physical_page)(page->ppn * PAGE_SIZE, (addr *)&dest, PHYSICAL_PAGE_NO_WAIT);
			if(err == B_NO_ERROR)
				break;

			// it couldn't map the second one, so sleep and retry
			// keeps an extremely rare deadlock from occuring
			(*aspace->translation_map.ops->put_physical_page)((addr)src);
			thread_snooze(5000);
		}

		memcpy(dest, src, PAGE_SIZE);
		(*aspace->translation_map.ops->put_physical_page)((addr)src);
		(*aspace->translation_map.ops->put_physical_page)((addr)dest);

		vm_page_set_state(src_page, PAGE_STATE_ACTIVE);

		mutex_lock(&top_cache_ref->lock);
		if(dummy_page.state == PAGE_STATE_BUSY && dummy_page.cache_ref == top_cache_ref) {
			vm_cache_remove_page(top_cache_ref, &dummy_page);
			dummy_page.state = PAGE_STATE_INACTIVE;
		}
		vm_cache_insert_page(top_cache_ref, page, cache_offset);
		mutex_unlock(&top_cache_ref->lock);

		if(dummy_page.state == PAGE_STATE_BUSY) {
			vm_cache_ref *temp_cache = dummy_page.cache_ref;
			mutex_lock(&temp_cache->lock);
			vm_cache_remove_page(temp_cache, &dummy_page);
			mutex_unlock(&temp_cache->lock);
			dummy_page.state = PAGE_STATE_INACTIVE;
		}
	}

	TRACE;

	err = 0;
	acquire_sem_etc(map->sem, READ_COUNT, 0, 0);
	if(change_count != map->change_count) {
		// something may have changed, see if the address is still valid
		region = vm_virtual_map_lookup(map, address);
		if(region == NULL
		  || region->cache_ref != top_cache_ref
		  || (address - region->base + region->cache_offset) != cache_offset) {
			dprintf("vm_soft_fault: address space layout changed effecting ongoing soft fault\n");
			err = ERR_VM_PF_BAD_ADDRESS; // BAD_ADDRESS
		}
	}

	TRACE;

	if(err == 0) {
		int new_lock = region->lock;
		if(page->cache_ref != top_cache_ref && !is_write)
			new_lock &= ~LOCK_RW;

		atomic_add(&page->ref_count, 1);
		(*aspace->translation_map.ops->lock)(&aspace->translation_map);
		(*aspace->translation_map.ops->map)(&aspace->translation_map, address,
			page->ppn * PAGE_SIZE, new_lock);
		(*aspace->translation_map.ops->unlock)(&aspace->translation_map);
	}

	TRACE;

	release_sem_etc(map->sem, READ_COUNT, 0);

	TRACE;

	if(dummy_page.state == PAGE_STATE_BUSY) {
		vm_cache_ref *temp_cache = dummy_page.cache_ref;
		mutex_lock(&temp_cache->lock);
		vm_cache_remove_page(temp_cache, &dummy_page);
		mutex_unlock(&temp_cache->lock);
		dummy_page.state = PAGE_STATE_INACTIVE;
	}

	TRACE;

	vm_page_set_state(page, PAGE_STATE_ACTIVE);

	vm_cache_release_ref(top_cache_ref);
	vm_put_aspace(aspace);

	TRACE;

	return err;
}

static vm_region *vm_virtual_map_lookup(vm_virtual_map *map, addr address)
{
	vm_region *region;

	// check the region_list region first
	region = map->region_hint;
	if(region && region->base <= address && (region->base + region->size) > address)
		return region;

	for(region = map->region_list; region != NULL; region = region->aspace_next) {
		if(region->base <= address && (region->base + region->size) > address)
			break;
	}

	if(region)
		map->region_hint = region;
	return region;
}

int vm_get_physical_page(addr paddr, addr *vaddr, int flags)
{
	return (*kernel_aspace->translation_map.ops->get_physical_page)(paddr, vaddr, flags);
}

int vm_put_physical_page(addr vaddr)
{
	return (*kernel_aspace->translation_map.ops->put_physical_page)(vaddr);
}

void vm_increase_max_commit(addr delta)
{
	int state;

//	dprintf("vm_increase_max_commit: delta 0x%x\n", delta);

	state = int_disable_interrupts();
	acquire_spinlock(&max_commit_lock);
	max_commit += delta;
	release_spinlock(&max_commit_lock);
	int_restore_interrupts(state);
}

int user_memcpy(void *to, const void *from, size_t size)
{
	return arch_cpu_user_memcpy(to, from, size, &thread_get_current_thread()->fault_handler);
}

int user_strcpy(char *to, const char *from)
{
	return arch_cpu_user_strcpy(to, from, &thread_get_current_thread()->fault_handler);
}

int user_strncpy(char *to, const char *from, size_t size)
{
	return arch_cpu_user_strncpy(to, from, size, &thread_get_current_thread()->fault_handler);
}

int user_memset(void *s, char c, size_t count)
{
	return arch_cpu_user_memset(s, c, count, &thread_get_current_thread()->fault_handler);
}

