/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/


#include <KernelExport.h>
#include <vm_store_device.h>

#include <stdlib.h>


struct device_store {
	vm_store	vm;
	addr_t		base_address;
};


static void
device_destroy(struct vm_store *store)
{
	free(store);
}


static off_t
device_commit(struct vm_store *store, off_t size)
{
	store->committed_size = size;
	return size;
}


static int
device_has_page(struct vm_store *store, off_t offset)
{
	// this should never be called
	return 0;
}


static ssize_t
device_read(struct vm_store *store, off_t offset, iovecs *vecs)
{
	panic("device_store: read called. Invalid!\n");
	return B_ERROR;
}


static ssize_t
device_write(struct vm_store *store, off_t offset, iovecs *vecs)
{
	// no place to write, this will cause the page daemon to skip this store
	return 0;
}

/** this fault handler should take over the page fault routine and map the page in
 *
 *	setup: the cache that this store is part of has a ref being held and will be
 *	released after this handler is done
 */

static int
device_fault(struct vm_store *_store, struct vm_address_space *aspace, off_t offset)
{
	struct device_store *store = (struct device_store *)_store;
	vm_cache_ref *cache_ref = store->vm.cache->ref;
	vm_region *region;

//	dprintf("device_fault: offset 0x%x 0x%x + base_addr 0x%x\n", offset, d->base_addr);

	// figure out which page needs to be mapped where
	mutex_lock(&cache_ref->lock);
	(*aspace->translation_map.ops->lock)(&aspace->translation_map);

	// cycle through all of the regions that map this cache and map the page in
	for (region = cache_ref->region_list; region != NULL; region = region->cache_next) {
		// make sure this page in the cache that was faulted on is covered in this region
		if (offset >= region->cache_offset && (offset - region->cache_offset) < region->size) {
//			dprintf("device_fault: mapping paddr 0x%x to vaddr 0x%x\n",
//				(addr)(d->base_addr + offset),
//				(addr)(region->base + (offset - region->cache_offset)));
			(*aspace->translation_map.ops->map)(&aspace->translation_map,
				region->base + (offset - region->cache_offset),
				store->base_address + offset, region->lock);
		}
	}

	(*aspace->translation_map.ops->unlock)(&aspace->translation_map);
	mutex_unlock(&cache_ref->lock);

//	dprintf("device_fault: done\n");

	return 0;
}


static vm_store_ops device_ops = {
	&device_destroy,
	&device_commit,
	&device_has_page,
	&device_read,
	&device_write,
	&device_fault,
	NULL,
	NULL
};


vm_store *
vm_store_create_device(addr_t baseAddress)
{
	struct device_store *store = malloc(sizeof(struct device_store));
	if (store == NULL)
		return NULL;

	store->vm.ops = &device_ops;
	store->vm.cache = NULL;
	store->vm.committed_size = 0;

	store->base_address = baseAddress;

	return &store->vm;
}

