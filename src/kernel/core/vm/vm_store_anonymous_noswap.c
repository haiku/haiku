/* 
** Copyright 2002-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
**
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/


#include <KernelExport.h>
#include <vm_store_anonymous_noswap.h>
#include <vm_priv.h>

#include <stdlib.h>


//#define TRACE_VM
#ifdef TRACE_VM
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


static void
anonymous_destroy(struct vm_store *store)
{
	vm_unreserve_memory(store->committed_size);
	free(store);
}


static status_t
anonymous_commit(struct vm_store *store, off_t size)
{
	// Check to see how much we could commit - we need real memory

	if (size > store->committed_size) {
		// try to commit
		if (vm_try_reserve_memory(size - store->committed_size) != B_OK)
			return B_NO_MEMORY;
		
		store->committed_size = size;
	} else {
		// we can release some
		vm_unreserve_memory(store->committed_size - size);
	}

	return B_OK;
}


static bool
anonymous_has_page(struct vm_store *store, off_t offset)
{
	return false;
}


static status_t
anonymous_read(struct vm_store *store, off_t offset, const iovec *vecs, size_t count, size_t *_numBytes)
{
	panic("anonymous_store: read called. Invalid!\n");
	return B_ERROR;
}


static status_t
anonymous_write(struct vm_store *store, off_t offset, const iovec *vecs, size_t count, size_t *_numBytes)
{
	// no place to write, this will cause the page daemon to skip this store
	return 0;
}


static vm_store_ops anonymous_ops = {
	&anonymous_destroy,
	&anonymous_commit,
	&anonymous_has_page,
	&anonymous_read,
	&anonymous_write,
	NULL, // fault() is unused
	NULL,
	NULL
};


/* vm_store_create_anonymous
 * Create a new vm_store that uses anonymous noswap memory
 */

vm_store *
vm_store_create_anonymous_noswap()
{
	vm_store *store = malloc(sizeof(vm_store));
	if (store == NULL)
		return NULL;

	TRACE(("vm_store_create_anonymous (%p)\n", store));

	store->ops = &anonymous_ops;
	store->cache = NULL;
	store->committed_size = 0;

	return store;
}

