/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <kernel.h>
#include <vm.h>
#include <vm_priv.h>
#include <vm_cache.h>
#include <vm_page.h>
#include <memheap.h>
#include <int.h>
#include <khash.h>
#include <lock.h>
#include <debug.h>
#include <lock.h>
#include <smp.h>
#include <arch/cpu.h>
#include <Errors.h>
#include <atomic.h>

/* hash table of pages keyed by cache they're in and offset */
#define PAGE_TABLE_SIZE 1024 /* make this dynamic */
static void *page_cache_table;
static spinlock_t page_cache_table_lock;

struct page_lookup_key {
	off_t offset;
	vm_cache_ref *ref;
};

static int page_compare_func(void *_p, const void *_key)
{
	vm_page *p = _p;
	const struct page_lookup_key *key = _key;

//	dprintf("page_compare_func: p 0x%x, key 0x%x\n", p, key);

	if(p->cache_ref == key->ref && p->offset == key->offset)
		return 0;
	else
		return -1;
}

#define HASH(offset, ref) ((unsigned int)(offset >> 12) ^ ((unsigned int)(ref)>>4))

static unsigned int page_hash_func(void *_p, const void *_key, unsigned int range)
{
	vm_page *p = _p;
	const struct page_lookup_key *key = _key;
#if 0
	if(p)
		dprintf("page_hash_func: p 0x%x, key 0x%x, HASH = 0x%x\n", p, key, HASH(p->offset, p->cache_ref) % range);
	else
		dprintf("page_hash_func: p 0x%x, key 0x%x, HASH = 0x%x\n", p, key, HASH(key->offset, key->ref) % range);
#endif
	if(p)
		return HASH(p->offset, p->cache_ref) % range;
	else
		return HASH(key->offset, key->ref) % range;
}

int vm_cache_init(kernel_args *ka)
{
	vm_page p;

	page_cache_table = hash_init(PAGE_TABLE_SIZE,
		(int)&p.hash_next - (int)&p,
		&page_compare_func,
		&page_hash_func);
	if(!page_cache_table)
		panic("vm_cache_init: cannot allocate memory for page cache hash table\n");
	page_cache_table_lock = 0;

	return 0;
}

vm_cache *vm_cache_create(vm_store *store)
{
	vm_cache *cache;

	cache = kmalloc(sizeof(vm_cache));
	if(cache == NULL)
		return NULL;

	cache->page_list = NULL;
	cache->ref = NULL;
	cache->source = NULL;
	cache->store = store;
	if(store != NULL)
		store->cache = cache;
	cache->virtual_size = 0;
	cache->temporary = 0;
	cache->scan_skip = 0;

	return cache;
}

vm_cache_ref *vm_cache_ref_create(vm_cache *cache)
{
	vm_cache_ref *ref;

	ref = kmalloc(sizeof(vm_cache_ref));
	if(ref == NULL)
		return NULL;

	ref->cache = cache;
	mutex_init(&ref->lock, "cache_ref_mutex");
	ref->region_list = NULL;
	ref->ref_count = 0;
	cache->ref = ref;

	return ref;
}

void vm_cache_acquire_ref(vm_cache_ref *cache_ref, bool acquire_store_ref)
{
//	dprintf("vm_cache_acquire_ref: cache_ref 0x%x, ref will be %d\n", cache_ref, cache_ref->ref_count+1);

	if(cache_ref == NULL)
		panic("vm_cache_acquire_ref: passed NULL\n");
	if(acquire_store_ref && cache_ref->cache->store->ops->acquire_ref) {
		cache_ref->cache->store->ops->acquire_ref(cache_ref->cache->store);
	}
	atomic_add(&cache_ref->ref_count, 1);
}

void vm_cache_release_ref(vm_cache_ref *cache_ref)
{
	vm_page *page;

//	dprintf("vm_cache_release_ref: cache_ref 0x%x, ref will be %d\n", cache_ref, cache_ref->ref_count-1);

	if(cache_ref == NULL)
		panic("vm_cache_release_ref: passed NULL\n");
	if(atomic_add(&cache_ref->ref_count, -1) == 1) {
		// delete this cache
		// delete the cache's backing store, if it has one
		off_t store_committed_size = 0;
		if(cache_ref->cache->store) {
			store_committed_size = cache_ref->cache->store->committed_size;
			(*cache_ref->cache->store->ops->destroy)(cache_ref->cache->store);
		}

		// free all of the pages in the cache
		page = cache_ref->cache->page_list;
		while(page) {
			vm_page *old_page = page;
			int state;

			page = page->cache_next;

			// remove it from the hash table
			state = int_disable_interrupts();
			acquire_spinlock(&page_cache_table_lock);

			hash_remove(page_cache_table, old_page);

			release_spinlock(&page_cache_table_lock);
			int_restore_interrupts(state);

//			dprintf("vm_cache_release_ref: freeing page 0x%x\n", old_page->ppn);
			vm_page_set_state(old_page, PAGE_STATE_FREE);
		}
		vm_increase_max_commit(cache_ref->cache->virtual_size - store_committed_size);

		// remove the ref to the source
		if(cache_ref->cache->source)
			vm_cache_release_ref(cache_ref->cache->source->ref);

		mutex_destroy(&cache_ref->lock);
		kfree(cache_ref->cache);
		kfree(cache_ref);

		return;
	}
	if(cache_ref->cache->store->ops->release_ref) {
		cache_ref->cache->store->ops->release_ref(cache_ref->cache->store);
	}
}

vm_page *vm_cache_lookup_page(vm_cache_ref *cache_ref, off_t offset)
{
	vm_page *page;
	int state;
	struct page_lookup_key key;

	key.offset = offset;
	key.ref = cache_ref;

	state = int_disable_interrupts();
	acquire_spinlock(&page_cache_table_lock);

	page = hash_lookup(page_cache_table, &key);

	release_spinlock(&page_cache_table_lock);
	int_restore_interrupts(state);

	return page;
}

void vm_cache_insert_page(vm_cache_ref *cache_ref, vm_page *page, off_t offset)
{
	int state;

//	dprintf("vm_cache_insert_page: cache 0x%x, page 0x%x, offset 0x%x 0x%x\n", cache_ref, page, offset);

	page->offset = offset;

	if(cache_ref->cache->page_list != NULL) {
		cache_ref->cache->page_list->cache_prev = page;
	}
	page->cache_next = cache_ref->cache->page_list;
	page->cache_prev = NULL;
	cache_ref->cache->page_list = page;

	page->cache_ref = cache_ref;

	state = int_disable_interrupts();
	acquire_spinlock(&page_cache_table_lock);

	hash_insert(page_cache_table, page);

	release_spinlock(&page_cache_table_lock);
	int_restore_interrupts(state);

}

void vm_cache_remove_page(vm_cache_ref *cache_ref, vm_page *page)
{
	int state;

//	dprintf("vm_cache_remove_page: cache 0x%x, page 0x%x\n", cache_ref, page);

	state = int_disable_interrupts();
	acquire_spinlock(&page_cache_table_lock);

	hash_remove(page_cache_table, page);

	release_spinlock(&page_cache_table_lock);
	int_restore_interrupts(state);

	if(cache_ref->cache->page_list == page) {
		if(page->cache_next != NULL)
			page->cache_next->cache_prev = NULL;
		cache_ref->cache->page_list = page->cache_next;
	} else {
		if(page->cache_prev != NULL)
			page->cache_prev->cache_next = page->cache_next;
		if(page->cache_next != NULL)
			page->cache_next->cache_prev = page->cache_prev;
	}
	page->cache_ref = NULL;
}

int vm_cache_insert_region(vm_cache_ref *cache_ref, vm_region *region)
{
	mutex_lock(&cache_ref->lock);

	region->cache_next = cache_ref->region_list;
	if(region->cache_next)
		region->cache_next->cache_prev = region;
	region->cache_prev = NULL;
	cache_ref->region_list = region;

	mutex_unlock(&cache_ref->lock);
	return 0;
}

int vm_cache_remove_region(vm_cache_ref *cache_ref, vm_region *region)
{
	mutex_lock(&cache_ref->lock);

	if(region->cache_prev)
		region->cache_prev->cache_next = region->cache_next;
	if(region->cache_next)
		region->cache_next->cache_prev = region->cache_prev;
	if(cache_ref->region_list == region)
		cache_ref->region_list = region->cache_next;

	mutex_unlock(&cache_ref->lock);
	return 0;
}
