/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <kernel.h>
#include <vm.h>
#include <memheap.h>
#include <debug.h>
#include <lock.h>
#include <vm_store_vnode.h>
#include <vfs.h>
#include <Errors.h>

#define STORE_DATA(x) ((struct vnode_store_data *)(x->data))

struct vnode_store_data {
	void *vn;
};

static void vnode_destroy(struct vm_store *store)
{
	if(store) {
		kfree(store);
	}
}

/* vnode_commit
 * We're being asked to commit some memory, so record the
 * fact here.
 */
static off_t vnode_commit(struct vm_store *store, off_t size)
{
	store->committed_size = size;
	return size;
}

static int vnode_has_page(struct vm_store *store, off_t offset)
{
	return 1; // we always have the page, man
}

static ssize_t vnode_read(struct vm_store *store, off_t offset, iovecs *vecs)
{
	return vfs_readpage(STORE_DATA(store)->vn, vecs, offset);
}

static ssize_t vnode_write(struct vm_store *store, off_t offset, iovecs *vecs)
{
	return vfs_writepage(STORE_DATA(store)->vn, vecs, offset);
}

/* unused
static int vnode_fault(struct vm_store *store, struct vm_address_space *aspace, off_t offset)
{
	return 0;
}
*/

static void vnode_acquire_ref(struct vm_store *store)
{
	vfs_vnode_acquire_ref(STORE_DATA(store)->vn);
}

static void vnode_release_ref(struct vm_store *store)
{
	vfs_vnode_release_ref(STORE_DATA(store)->vn);
}

static vm_store_ops vnode_ops = {
	&vnode_destroy,
	&vnode_commit,
	&vnode_has_page,
	&vnode_read,
	&vnode_write,
	NULL,
	&vnode_acquire_ref,
	&vnode_release_ref
};

vm_store *vm_store_create_vnode(void *vnode)
{
	vm_store *store;
	struct vnode_store_data *d;

	store = kmalloc(sizeof(vm_store) + sizeof(struct vnode_store_data));
	if(store == NULL) {
		vfs_put_vnode_ptr(vnode);
		return NULL;
	}

	store->ops = &vnode_ops;
	store->cache = NULL;
	store->data = (void *)((addr)store + sizeof(vm_store));
	store->committed_size = 0;

	d = (struct vnode_store_data *)store->data;
	d->vn = vnode;

	return store;
}

