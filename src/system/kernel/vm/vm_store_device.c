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


static status_t
device_fault(struct vm_store *_store, struct vm_address_space *aspace, off_t offset)
{
	// devices are mapped in completely, so we shouldn't experience faults
	return B_BAD_ADDRESS;
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

