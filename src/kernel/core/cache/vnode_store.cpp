/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include "vnode_store.h"

#include <file_cache.h>
#include <vfs.h>
#include <vm.h>

#include <stdlib.h>


static void
store_destroy(struct vm_store *store)
{
	free(store);
}


static off_t
store_commit(struct vm_store *_store, off_t size)
{
	vnode_store *store = (vnode_store *)_store;
	// ToDo: enable this again when "size" is maintained correctly
#if 0
	// we don't like committing more memory than we have
	if (size > store->size)
		size = store->size;
#endif
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
	return vfs_read_pages(store->vnode, offset, vecs, count, _numBytes);
		// ToDo: the file system must currently clear out the remainder of the last page...
}


static status_t
store_write(struct vm_store *_store, off_t offset, const iovec *vecs, size_t count, size_t *_numBytes)
{
	vnode_store *store = (vnode_store *)_store;
	return vfs_write_pages(store->vnode, offset, vecs, count, _numBytes);
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

