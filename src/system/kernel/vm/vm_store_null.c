/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/


#include "vm_store_null.h"

#include <stdlib.h>


static void
null_destroy(struct vm_store *store)
{
	free(store);
}


static status_t
null_commit(struct vm_store *store, off_t size)
{
	store->committed_size = size;
	return B_OK;
}


static bool
null_has_page(struct vm_store *store, off_t offset)
{
	return true; // we always have the page, man
}


static status_t
null_read(struct vm_store *store, off_t offset, const iovec *vecs, size_t count, size_t *_numBytes)
{
	return -1;
}


static status_t
null_write(struct vm_store *store, off_t offset, const iovec *vecs, size_t count, size_t *_numBytes)
{
	return -1;
}


static status_t
null_fault(struct vm_store *store, struct vm_address_space *aspace, off_t offset)
{
	/* we can't fault on this region, that's pretty much the point of the null store object */
	return B_BAD_ADDRESS;
}


static vm_store_ops null_ops = {
	&null_destroy,
	&null_commit,
	&null_has_page,
	&null_read,
	&null_write,
	&null_fault,
	NULL,
	NULL
};


vm_store *
vm_store_create_null(void)
{
	vm_store *store;

	store = malloc(sizeof(vm_store));
	if (store == NULL)
		return NULL;

	store->ops = &null_ops;
	store->cache = NULL;
	store->committed_size = 0;

	return store;
}

