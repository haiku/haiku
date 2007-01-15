/*
 * Copyright 2004-2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include "vm_store_device.h"

#include <KernelExport.h>
#include <vm_priv.h>

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


static status_t
device_commit(struct vm_store *store, off_t size)
{
	store->committed_size = size;
	return B_OK;
}


static bool
device_has_page(struct vm_store *store, off_t offset)
{
	// this should never be called
	return false;
}


static status_t
device_read(struct vm_store *store, off_t offset, const iovec *vecs, size_t count,
	size_t *_numBytes, bool fsReenter)
{
	panic("device_store: read called. Invalid!\n");
	return B_ERROR;
}


static status_t
device_write(struct vm_store *store, off_t offset, const iovec *vecs, size_t count,
	size_t *_numBytes, bool fsReenter)
{
	// no place to write, this will cause the page daemon to skip this store
	return B_OK;
}


/** this fault handler should take over the page fault routine and map the page in
 *
 *	setup: the cache that this store is part of has a ref being held and will be
 *	released after this handler is done
 */

static status_t
device_fault(struct vm_store *_store, struct vm_address_space *aspace, off_t offset)
{
	struct device_store *store = (struct device_store *)_store;
	vm_cache_ref *cache_ref = store->vm.cache->ref;
	vm_translation_map *map = &aspace->translation_map;
	vm_area *area;

	// figure out which page needs to be mapped where
	map->ops->lock(map);

	// cycle through all of the regions that map this cache and map the page in
	for (area = cache_ref->areas; area != NULL; area = area->cache_next) {
		// make sure this page in the cache that was faulted on is covered in this area
		if (offset >= area->cache_offset && (offset - area->cache_offset) < area->size) {
			// don't map already mapped pages
			addr_t physicalAddress;
			uint32 flags;
			map->ops->query(map, area->base + (offset - area->cache_offset),
				&physicalAddress, &flags);
			if (flags & PAGE_PRESENT)
				continue;

			map->ops->map(map, area->base + (offset - area->cache_offset),
				store->base_address + offset, area->protection);
		}
	}

	map->ops->unlock(map);
	return B_OK;
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

