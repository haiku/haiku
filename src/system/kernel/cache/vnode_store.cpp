/*
 * Copyright 2008-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2004-2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */

#include "vnode_store.h"

#include <stdlib.h>
#include <string.h>

#include <file_cache.h>
#include <vfs.h>
#include <vm/vm.h>

#include "IORequest.h"


status_t
VMVnodeCache::Init(struct vnode* vnode, uint32 allocationFlags)
{
	status_t error = VMCache::Init(CACHE_TYPE_VNODE, allocationFlags);
	if (error != B_OK)
		return error;

	fVnode = vnode;
	fFileCacheRef = NULL;
	fVnodeDeleted = false;

	vfs_vnode_to_node_ref(fVnode, &fDevice, &fInode);

	return B_OK;
}


bool
VMVnodeCache::HasPage(off_t offset)
{
	return ROUNDUP(offset, B_PAGE_SIZE) >= virtual_base
		&& offset < virtual_end;
}


status_t
VMVnodeCache::Read(off_t offset, const generic_io_vec* vecs, size_t count,
	uint32 flags, generic_size_t* _numBytes)
{
	generic_size_t bytesUntouched = *_numBytes;

	status_t status = vfs_read_pages(fVnode, NULL, offset, vecs, count,
		flags, _numBytes);

	generic_size_t bytesEnd = *_numBytes;

	if (offset + (off_t)bytesEnd > virtual_end)
		bytesEnd = virtual_end - offset;

	// If the request could be filled completely, or an error occured,
	// we're done here
	if (status != B_OK || bytesUntouched == bytesEnd)
		return status;

	bytesUntouched -= bytesEnd;

	// Clear out any leftovers that were not touched by the above read - we're
	// doing this here so that not every file system/device has to implement
	// this
	for (int32 i = count; i-- > 0 && bytesUntouched != 0;) {
		generic_size_t length = min_c(bytesUntouched, vecs[i].length);

		generic_addr_t address = vecs[i].base + vecs[i].length - length;
		if ((flags & B_PHYSICAL_IO_REQUEST) != 0)
			vm_memset_physical(address, 0, length);
		else
			memset((void*)(addr_t)address, 0, length);

		bytesUntouched -= length;
	}

	return B_OK;
}


status_t
VMVnodeCache::Write(off_t offset, const generic_io_vec* vecs, size_t count,
	uint32 flags, generic_size_t* _numBytes)
{
	return vfs_write_pages(fVnode, NULL, offset, vecs, count, flags, _numBytes);
}


status_t
VMVnodeCache::WriteAsync(off_t offset, const generic_io_vec* vecs, size_t count,
	generic_size_t numBytes, uint32 flags, AsyncIOCallback* callback)
{
	return vfs_asynchronous_write_pages(fVnode, NULL, offset, vecs, count,
		numBytes, flags, callback);
}


status_t
VMVnodeCache::Fault(struct VMAddressSpace* aspace, off_t offset)
{
	if (!HasPage(offset))
		return B_BAD_ADDRESS;

	// vm_soft_fault() reads the page in.
	return B_BAD_HANDLER;
}


bool
VMVnodeCache::CanWritePage(off_t offset)
{
	// all pages can be written
	return true;
}


status_t
VMVnodeCache::AcquireUnreferencedStoreRef()
{
	// Quick check whether getting a vnode reference is still allowed. Only
	// after a successful vfs_get_vnode() the check is safe (since then we've
	// either got the reference to our vnode, or have been notified that it is
	// toast), but the check is cheap and saves quite a bit of work in case the
	// condition holds.
	if (fVnodeDeleted)
		return B_BUSY;

	struct vnode* vnode;
	status_t status = vfs_get_vnode(fDevice, fInode, false, &vnode);

	// If successful, update the store's vnode pointer, so that release_ref()
	// won't use a stale pointer.
	if (status == B_OK && fVnodeDeleted) {
		vfs_put_vnode(vnode);
		status = B_BUSY;
	}

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


void
VMVnodeCache::Dump(bool showPages) const
{
	VMCache::Dump(showPages);

	kprintf("  vnode:        %p <%" B_PRIdDEV ", %" B_PRIdINO ">\n", fVnode,
		fDevice, fInode);
}
