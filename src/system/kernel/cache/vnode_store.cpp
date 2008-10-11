/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2004-2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */

#include "vnode_store.h"

#include <stdlib.h>
#include <string.h>

#include <file_cache.h>
#include <vfs.h>
#include <vm.h>

#include "io_requests.h"


static inline status_t
memset_physical(addr_t address, int value, size_t length)
{
	while (length > 0) {
		addr_t pageOffset = address % B_PAGE_SIZE;
		addr_t virtualAddress;
		status_t error = vm_get_physical_page(address - pageOffset,
			&virtualAddress, 0);
		if (error != B_OK)
			return error;

		size_t toSet = min_c(length, B_PAGE_SIZE - pageOffset);
		memset((void*)(virtualAddress + pageOffset), value, toSet);

		vm_put_physical_page(virtualAddress);

		length -= toSet;
		address += toSet;
	}

	return B_OK;
}


status_t
VMVnodeCache::Init(struct vnode *vnode)
{
	status_t error = VMCache::Init(CACHE_TYPE_VNODE);
	if (error != B_OK)
		return error;

	fVnode = vnode;
	fFileCacheRef = NULL;

	vfs_vnode_to_node_ref(fVnode, &fDevice, &fInode);

	return B_OK;
}


bool
VMVnodeCache::HasPage(off_t offset)
{
	// We always pretend to have the page - even if it's beyond the size of
	// the file. The read function will only cut down the size of the read,
	// it won't fail because of that.
	return true;
}


status_t
VMVnodeCache::Read(off_t offset, const iovec *vecs, size_t count,
	uint32 flags, size_t *_numBytes)
{
	size_t bytesUntouched = *_numBytes;

	status_t status = vfs_read_pages(fVnode, NULL, offset, vecs, count,
		flags, _numBytes);

	bytesUntouched -= *_numBytes;

	// If the request could be filled completely, or an error occured,
	// we're done here
	if (status < B_OK || bytesUntouched == 0)
		return status;

	// Clear out any leftovers that were not touched by the above read - we're
	// doing this here so that not every file system/device has to implement
	// this
	for (int32 i = count; i-- > 0 && bytesUntouched != 0;) {
		size_t length = min_c(bytesUntouched, vecs[i].iov_len);

		addr_t address = (addr_t)vecs[i].iov_base + vecs[i].iov_len - length;
		if ((flags & B_PHYSICAL_IO_REQUEST) != 0)
			memset_physical(address, 0, length);
		else
			memset((void*)address, 0, length);

		bytesUntouched -= length;
	}

	return B_OK;
}


status_t
VMVnodeCache::Write(off_t offset, const iovec *vecs, size_t count,
	uint32 flags, size_t *_numBytes)
{
	return vfs_write_pages(fVnode, NULL, offset, vecs, count, flags, _numBytes);
}


status_t
VMVnodeCache::WriteAsync(off_t offset, const iovec* vecs, size_t count,
	size_t numBytes, uint32 flags, AsyncIOCallback* callback)
{
	return vfs_asynchronous_write_pages(fVnode, NULL, offset, vecs, count,
		numBytes, flags, callback);
}


status_t
VMVnodeCache::Fault(struct vm_address_space *aspace, off_t offset)
{
	return B_BAD_HANDLER;
}


status_t
VMVnodeCache::AcquireUnreferencedStoreRef()
{
	struct vnode *vnode;
	status_t status = vfs_get_vnode(fDevice, fInode, false, &vnode);

	// If successful, update the store's vnode pointer, so that release_ref()
	// won't use a stale pointer.
	if (status == B_OK)
		fVnode = vnode;

	return status;
}


void
VMVnodeCache::AcquireStoreRef()
{
	vfs_acquire_vnode(fVnode);
}


void
VMVnodeCache::ReleaseStoreRef()
{
	vfs_put_vnode(fVnode);
}

