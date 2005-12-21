/*
 * Copyright 2002-2005, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <kernel.h>
#include <vm.h>
#include <vm_priv.h>
#include <vm_cache.h>
#include <vm_page.h>
#include <int.h>
#include <util/khash.h>
#include <lock.h>
#include <debug.h>
#include <lock.h>
#include <smp.h>
#include <arch/cpu.h>

#include <malloc.h>

//#define TRACE_VM_CACHE
#ifdef TRACE_VM_CACHE
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


/* hash table of pages keyed by cache they're in and offset */
#define PAGE_TABLE_SIZE 1024 /* TODO: make this dynamic */

static void *page_cache_table;
static spinlock page_cache_table_lock;

struct page_lookup_key {
	uint32	offset;
	vm_cache *cache;
};


static int
page_compare_func(void *_p, const void *_key)
{
	vm_page *page = _p;
	const struct page_lookup_key *key = _key;

	TRACE(("page_compare_func: page %p, key %p\n", page, key));

	if (page->cache == key->cache && page->cache_offset == key->offset)
		return 0;

	return -1;
}


static uint32
page_hash_func(void *_p, const void *_key, uint32 range)
{
	vm_page *page = _p;
	const struct page_lookup_key *key = _key;

#define HASH(offset, ref) ((offset) ^ ((uint32)(ref) >> 4))

	if (page)
		return HASH(page->cache_offset, page->cache) % range;

	return HASH(key->offset, key->cache) % range;
}


status_t
vm_cache_init(kernel_args *ka)
{
	vm_page p;

	page_cache_table = hash_init(PAGE_TABLE_SIZE, (int)&p.hash_next - (int)&p,
		&page_compare_func, &page_hash_func);
	if (!page_cache_table)
		panic("vm_cache_init: cannot allocate memory for page cache hash table\n");
	page_cache_table_lock = 0;

	return B_OK;
}


vm_cache *
vm_cache_create(vm_store *store)
{
	vm_cache *cache;

	if (store == NULL) {
		panic("vm_cache created with NULL store!");
		return NULL;
	}

	cache = malloc(sizeof(vm_cache));
	if (cache == NULL)
		return NULL;

	cache->page_list = NULL;
	cache->ref = NULL;
	cache->source = NULL;
	cache->virtual_size = 0;
	cache->temporary = 0;
	cache->scan_skip = 0;

	// connect the store to its cache
	cache->store = store;
	store->cache = cache;

	return cache;
}


vm_cache_ref *
vm_cache_ref_create(vm_cache *cache)
{
	vm_cache_ref *ref;

	ref = malloc(sizeof(vm_cache_ref));
	if (ref == NULL)
		return NULL;

	if (mutex_init(&ref->lock, "cache_ref_mutex") < B_OK) {
		free(ref);
		return NULL;
	}

	ref->areas = NULL;
	ref->ref_count = 1;

	// connect the cache to its ref
	ref->cache = cache;
	cache->ref = ref;

	return ref;
}


void
vm_cache_acquire_ref(vm_cache_ref *cache_ref)
{
	TRACE(("vm_cache_acquire_ref: cache_ref %p, ref will be %ld\n",
		cache_ref, cache_ref->ref_count + 1));

	if (cache_ref == NULL)
		panic("vm_cache_acquire_ref: passed NULL\n");

	if (cache_ref->cache->store->ops->acquire_ref != NULL)
		cache_ref->cache->store->ops->acquire_ref(cache_ref->cache->store);

	atomic_add(&cache_ref->ref_count, 1);
}


void
vm_cache_release_ref(vm_cache_ref *cache_ref)
{
	vm_page *page;

	TRACE(("vm_cache_release_ref: cache_ref %p, ref will be %ld\n",
		cache_ref, cache_ref->ref_count - 1));

	if (cache_ref == NULL)
		panic("vm_cache_release_ref: passed NULL\n");

	if (atomic_add(&cache_ref->ref_count, -1) != 1) {
		// the store ref is only released on the "working" refs, not
		// on the initial one (this is vnode specific)
		if (cache_ref->cache->store->ops->release_ref)
			cache_ref->cache->store->ops->release_ref(cache_ref->cache->store);

		return;
	}

	// delete this cache

	// delete the cache's backing store
	cache_ref->cache->store->ops->destroy(cache_ref->cache->store);

	// free all of the pages in the cache
	page = cache_ref->cache->page_list;
	while (page) {
		vm_page *oldPage = page;
		int state;

		page = page->cache_next;

		// remove it from the hash table
		state = disable_interrupts();
		acquire_spinlock(&page_cache_table_lock);

		hash_remove(page_cache_table, oldPage);

		release_spinlock(&page_cache_table_lock);
		restore_interrupts(state);

		TRACE(("vm_cache_release_ref: freeing page 0x%lx\n",
			oldPage->physical_page_number));
		vm_page_set_state(oldPage, PAGE_STATE_FREE);
	}

	// remove the ref to the source
	if (cache_ref->cache->source)
		vm_cache_release_ref(cache_ref->cache->source->ref);

	mutex_destroy(&cache_ref->lock);
	free(cache_ref->cache);
	free(cache_ref);
}


vm_page *
vm_cache_lookup_page(vm_cache_ref *cache_ref, off_t offset)
{
	struct page_lookup_key key;
	cpu_status state;
	vm_page *page;

	ASSERT_LOCKED_MUTEX(&cache_ref->lock);

	key.offset = (uint32)(offset >> PAGE_SHIFT);
	key.cache = cache_ref->cache;

	state = disable_interrupts();
	acquire_spinlock(&page_cache_table_lock);

	page = hash_lookup(page_cache_table, &key);

	release_spinlock(&page_cache_table_lock);
	restore_interrupts(state);

	return page;
}


void
vm_cache_insert_page(vm_cache_ref *cache_ref, vm_page *page, off_t offset)
{
	cpu_status state;

	TRACE(("vm_cache_insert_page: cache_ref %p, page %p, offset %Ld\n", cache_ref, page, offset));
	ASSERT_LOCKED_MUTEX(&cache_ref->lock);

	page->cache_offset = (uint32)(offset >> PAGE_SHIFT);

	if (cache_ref->cache->page_list != NULL)
		cache_ref->cache->page_list->cache_prev = page;

	page->cache_next = cache_ref->cache->page_list;
	page->cache_prev = NULL;
	cache_ref->cache->page_list = page;

	page->cache = cache_ref->cache;

	state = disable_interrupts();
	acquire_spinlock(&page_cache_table_lock);

	hash_insert(page_cache_table, page);

	release_spinlock(&page_cache_table_lock);
	restore_interrupts(state);

}


/**	Removes the vm_page from this cache. Of course, the page must
 *	really be in this cache or evil things will happen.
 *	The vm_cache_ref lock must be held.
 */

void
vm_cache_remove_page(vm_cache_ref *cache_ref, vm_page *page)
{
	cpu_status state;

	TRACE(("vm_cache_remove_page: cache %p, page %p\n", cache_ref, page));
	ASSERT_LOCKED_MUTEX(&cache_ref->lock);

	state = disable_interrupts();
	acquire_spinlock(&page_cache_table_lock);

	hash_remove(page_cache_table, page);

	release_spinlock(&page_cache_table_lock);
	restore_interrupts(state);

	if (cache_ref->cache->page_list == page) {
		if (page->cache_next != NULL)
			page->cache_next->cache_prev = NULL;
		cache_ref->cache->page_list = page->cache_next;
	} else {
		if (page->cache_prev != NULL)
			page->cache_prev->cache_next = page->cache_next;
		if (page->cache_next != NULL)
			page->cache_next->cache_prev = page->cache_prev;
	}
	page->cache = NULL;
}


status_t
vm_cache_write_modified(vm_cache_ref *ref)
{
	status_t status;

	TRACE(("vm_cache_write_modified(ref = %p)\n", ref));

	mutex_lock(&ref->lock);
	status = vm_page_write_modified(ref->cache);
	mutex_unlock(&ref->lock);

	return status;
}


status_t
vm_cache_set_minimal_commitment(vm_cache_ref *ref, off_t commitment)
{
	status_t status = B_OK;
	vm_store *store;

	mutex_lock(&ref->lock);
	store = ref->cache->store;

	// If we don't have enough committed space to cover through to the new end of region...
	if (store->committed_size < commitment) {
		// ToDo: should we check if the cache's virtual size is large
		//	enough for a commitment of that size?

		// try to commit more memory
		status = store->ops->commit(store, commitment);
	}

	mutex_unlock(&ref->lock);
	return status;
}


/**	This function updates the size field of the vm_cache structure.
 *	If needed, it will free up all pages that don't belong to the cache anymore.
 *	The vm_cache_ref lock must be held when you call it.
 */

status_t
vm_cache_resize(vm_cache_ref *cacheRef, off_t newSize)
{
	vm_cache *cache = cacheRef->cache;
	status_t status;
	off_t oldSize;

	ASSERT_LOCKED_MUTEX(&cacheRef->lock);

	status = cache->store->ops->commit(cache->store, newSize);
	if (status != B_OK)
		return status;

	oldSize = cache->virtual_size;
	if (newSize < oldSize) {
		// we need to remove all pages in the cache outside of the new virtual size
		uint32 lastOffset = (uint32)(newSize >> PAGE_SHIFT);
		vm_page *page, *next;

		for (page = cache->page_list; page; page = next) {
			next = page->cache_next;

			if (page->cache_offset >= lastOffset) {
				// remove the page and put it into the free queue
				vm_cache_remove_page(cacheRef, page);
				vm_page_set_state(page, PAGE_STATE_FREE);
			}
		}
	}

	cache->virtual_size = newSize;
	return B_OK;
}


status_t
vm_cache_insert_area(vm_cache_ref *cache_ref, vm_area *area)
{
	mutex_lock(&cache_ref->lock);

	area->cache_next = cache_ref->areas;
	if (area->cache_next)
		area->cache_next->cache_prev = area;
	area->cache_prev = NULL;
	cache_ref->areas = area;

	mutex_unlock(&cache_ref->lock);
	return B_OK;
}


status_t
vm_cache_remove_area(vm_cache_ref *cache_ref, vm_area *area)
{
	mutex_lock(&cache_ref->lock);

	if (area->cache_prev)
		area->cache_prev->cache_next = area->cache_next;
	if (area->cache_next)
		area->cache_next->cache_prev = area->cache_prev;
	if (cache_ref->areas == area)
		cache_ref->areas = area->cache_next;

	mutex_unlock(&cache_ref->lock);
	return B_OK;
}
