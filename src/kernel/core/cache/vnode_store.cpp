/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include "vnode_store.h"

#include <file_cache.h>
#include <vfs.h>

#include <stdlib.h>
#include <string.h>


static void
store_destroy(struct vm_store *store)
{
	free(store);
}


static off_t
store_commit(struct vm_store *_store, off_t size)
{
	vnode_store *store = (vnode_store *)_store;

	store->vm.committed_size = size;
	return size;
}


static bool
store_has_page(struct vm_store *_store, off_t offset)
{
	// We always pretend to have the page - even if it's beyond the size of
	// the file. The read function will only cut down the size of the read,
	// it won't fail because of that.
	return true;
}


static status_t
store_read(struct vm_store *_store, off_t offset, const iovec *vecs, size_t count, size_t *_numBytes)
{
	vnode_store *store = (vnode_store *)_store;
	size_t bytesUntouched = *_numBytes;

	status_t status = vfs_read_pages(store->vnode, NULL, offset, vecs, count, _numBytes);

	bytesUntouched -= *_numBytes;

	// if the request could be filled completely, or an error occured, we're done here
	if (status < B_OK || bytesUntouched == 0)
		return status;

	// Clear out any leftovers that were not touched by the above read - we're
	// doing this here so that not every file system/device has to implement
	// this
	for (int32 i = count; i-- > 0 && bytesUntouched != 0;) {
		size_t length = min_c(bytesUntouched, vecs[i].iov_len);

		// ToDo: will have to map the pages in later (when we switch to physical pages)
		memset((void *)((addr_t)vecs[i].iov_base + vecs[i].iov_len - length), 0, length);
		bytesUntouched -= length;
	}

	return B_OK;
}


static status_t
store_write(struct vm_store *_store, off_t offset, const iovec *vecs, size_t count, size_t *_numBytes)
{
	vnode_store *store = (vnode_store *)_store;
	return vfs_write_pages(store->vnode, NULL, offset, vecs, count, _numBytes);
}


static void
store_acquire_ref(struct vm_store *_store)
{
	vnode_store *store = (vnode_store *)_store;
	vfs_vnode_acquire_ref(store->vnode);
}


static void
store_release_ref(struct vm_store *_store)
{
	vnode_store *store = (vnode_store *)_store;
	vfs_vnode_release_ref(store->vnode);
}


static vm_store_ops sStoreOps = {
	&store_destroy,
	&store_commit,
	&store_has_page,
	&store_read,
	&store_write,
	NULL,	/* fault */
	&store_acquire_ref,
	&store_release_ref
};


//	#pragma mark -


extern "C" vm_store *
vm_create_vnode_store(void *vnode)
{
	vnode_store *store = (vnode_store *)malloc(sizeof(struct vnode_store));
	if (store == NULL) {
		vfs_vnode_release_ref(vnode);
		return NULL;
	}

	store->vm.ops = &sStoreOps;
	store->vm.cache = NULL;
	store->vm.committed_size = 0;

	store->vnode = vnode;
	store->size = 0;
		// the file system will maintain this field through the file cache API

	return &store->vm;
}

