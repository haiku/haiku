/*
 * Copyright 2004-2008, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Axel Dörfler <axeld@pinc-software.de>
 * 		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 */

#include "fssh_fs_cache.h"

#include <new>

#include <stdlib.h>

#include "DoublyLinkedList.h"
#include "fssh_kernel_export.h"
#include "fssh_lock.h"
#include "fssh_stdio.h"
#include "fssh_string.h"
#include "fssh_uio.h"
#include "fssh_unistd.h"
#include "hash.h"
#include "vfs.h"


#undef TRACE
//#define TRACE_FILE_CACHE
#ifdef TRACE_FILE_CACHE
#	define TRACE(x) fssh_dprintf x
#else
#	define TRACE(x) ;
#endif

using std::nothrow;


// This is a hacked version of the kernel file cache implementation. The main
// part of the implementation didn't change that much -- some code not needed
// in userland has been removed, most notably everything on the page level.
// On the downside, the cache now never caches anything, but will always
// directly work on the underlying device. Since that is usually cached by the
// host operating system, it shouldn't hurt much, though.

// maximum number of iovecs per request
#define MAX_IO_VECS			64	// 256 kB
#define MAX_FILE_IO_VECS	32
#define MAX_TEMP_IO_VECS	8

#define user_memcpy(a, b, c) fssh_memcpy(a, b, c)

#define PAGE_ALIGN(x) (((x) + (FSSH_B_PAGE_SIZE - 1)) & ~(FSSH_B_PAGE_SIZE - 1))
#define ASSERT(x)

namespace FSShell {

struct file_cache_ref;

typedef fssh_status_t (*cache_func)(file_cache_ref *ref, void *cookie,
	fssh_off_t offset, int32_t pageOffset, fssh_addr_t buffer,
	fssh_size_t bufferSize);

struct file_cache_ref {
	fssh_mutex					lock;
	fssh_mount_id				mountID;
	fssh_vnode_id				nodeID;
	struct vnode*				node;
	fssh_off_t					virtual_size;
};


fssh_status_t
file_cache_init()
{
	return FSSH_B_OK;
}


//	#pragma mark -


static fssh_status_t
read_from_file(file_cache_ref *ref, void *cookie, fssh_off_t offset,
	int32_t pageOffset, fssh_addr_t buffer, fssh_size_t bufferSize)
{
	fssh_iovec vec;
	vec.iov_base = (void *)buffer;
	vec.iov_len = bufferSize;

	fssh_mutex_unlock(&ref->lock);

	fssh_status_t status = vfs_read_pages(ref->node, cookie,
		offset + pageOffset, &vec, 1, &bufferSize);

	fssh_mutex_lock(&ref->lock);

	return status;
}


static fssh_status_t
write_to_file(file_cache_ref *ref, void *cookie, fssh_off_t offset,
	int32_t pageOffset, fssh_addr_t buffer, fssh_size_t bufferSize)
{
	fssh_iovec vec;
	vec.iov_base = (void *)buffer;
	vec.iov_len = bufferSize;

	fssh_mutex_unlock(&ref->lock);

	fssh_status_t status = vfs_write_pages(ref->node, cookie,
		offset + pageOffset, &vec, 1, &bufferSize);

	fssh_mutex_lock(&ref->lock);

	return status;
}


static inline fssh_status_t
satisfy_cache_io(file_cache_ref *ref, void *cookie, cache_func function,
	fssh_off_t offset, fssh_addr_t buffer, int32_t &pageOffset,
	fssh_size_t bytesLeft, fssh_off_t &lastOffset,
	fssh_addr_t &lastBuffer, int32_t &lastPageOffset, fssh_size_t &lastLeft)
{
	if (lastBuffer == buffer)
		return FSSH_B_OK;

	fssh_size_t requestSize = buffer - lastBuffer;

	fssh_status_t status = function(ref, cookie, lastOffset, lastPageOffset,
		lastBuffer, requestSize);
	if (status == FSSH_B_OK) {
		lastBuffer = buffer;
		lastLeft = bytesLeft;
		lastOffset = offset;
		lastPageOffset = 0;
		pageOffset = 0;
	}
	return status;
}


static fssh_status_t
cache_io(void *_cacheRef, void *cookie, fssh_off_t offset, fssh_addr_t buffer,
	fssh_size_t *_size, bool doWrite)
{
	if (_cacheRef == NULL)
		fssh_panic("cache_io() called with NULL ref!\n");

	file_cache_ref *ref = (file_cache_ref *)_cacheRef;
	fssh_off_t fileSize = ref->virtual_size;

	TRACE(("cache_io(ref = %p, offset = %Ld, buffer = %p, size = %u, %s)\n",
		ref, offset, (void *)buffer, *_size, doWrite ? "write" : "read"));

	// out of bounds access?
	if (offset >= fileSize || offset < 0) {
		*_size = 0;
		return FSSH_B_OK;
	}

	int32_t pageOffset = offset & (FSSH_B_PAGE_SIZE - 1);
	fssh_size_t size = *_size;
	offset -= pageOffset;

	if ((uint64_t)offset + pageOffset + size > (uint64_t)fileSize) {
		// adapt size to be within the file's offsets
		size = fileSize - pageOffset - offset;
		*_size = size;
	}
	if (size == 0)
		return FSSH_B_OK;

	cache_func function;
	if (doWrite) {
		// in low memory situations, we bypass the cache beyond a
		// certain I/O size
		function = write_to_file;
	} else {
		function = read_from_file;
	}

	// "offset" and "lastOffset" are always aligned to B_PAGE_SIZE,
	// the "last*" variables always point to the end of the last
	// satisfied request part

	const uint32_t kMaxChunkSize = MAX_IO_VECS * FSSH_B_PAGE_SIZE;
	fssh_size_t bytesLeft = size, lastLeft = size;
	int32_t lastPageOffset = pageOffset;
	fssh_addr_t lastBuffer = buffer;
	fssh_off_t lastOffset = offset;

	MutexLocker locker(ref->lock);

	while (bytesLeft > 0) {
		// check if this page is already in memory
		fssh_size_t bytesInPage = fssh_min_c(
			fssh_size_t(FSSH_B_PAGE_SIZE - pageOffset), bytesLeft);

		if (bytesLeft <= bytesInPage)
			break;

		buffer += bytesInPage;
		bytesLeft -= bytesInPage;
		pageOffset = 0;
		offset += FSSH_B_PAGE_SIZE;

		if (buffer - lastBuffer + lastPageOffset >= kMaxChunkSize) {
			fssh_status_t status = satisfy_cache_io(ref, cookie, function,
				offset, buffer, pageOffset, bytesLeft, lastOffset,
				lastBuffer, lastPageOffset, lastLeft);
			if (status != FSSH_B_OK)
				return status;
		}
	}

	// fill the last remaining bytes of the request (either write or read)

	return function(ref, cookie, lastOffset, lastPageOffset, lastBuffer,
		lastLeft);
}


}	// namespace FSShell


//	#pragma mark - public FS API


using namespace FSShell;


void *
fssh_file_cache_create(fssh_mount_id mountID, fssh_vnode_id vnodeID,
	fssh_off_t size)
{
	TRACE(("file_cache_create(mountID = %d, vnodeID = %Ld, size = %Ld)\n",
		mountID, vnodeID, size));

	file_cache_ref *ref = new(nothrow) file_cache_ref;
	if (ref == NULL)
		return NULL;

	ref->mountID = mountID;
	ref->nodeID = vnodeID;
	ref->virtual_size = size;

	// get vnode
	fssh_status_t error = vfs_lookup_vnode(mountID, vnodeID, &ref->node);
	if (error != FSSH_B_OK) {
		fssh_dprintf("file_cache_create(): Failed get vnode %d:%" FSSH_B_PRIdINO
			": %s\n", mountID, vnodeID, fssh_strerror(error));
		delete ref;
		return NULL;
	}

	// create lock
	char buffer[32];
	fssh_snprintf(buffer, sizeof(buffer), "file cache %d:%" FSSH_B_PRIdINO,
		(int)mountID, vnodeID);
	fssh_mutex_init(&ref->lock, buffer);

	return ref;
}


void
fssh_file_cache_delete(void *_cacheRef)
{
	file_cache_ref *ref = (file_cache_ref *)_cacheRef;

	if (ref == NULL)
		return;

	TRACE(("file_cache_delete(ref = %p)\n", ref));

	fssh_mutex_lock(&ref->lock);
	fssh_mutex_destroy(&ref->lock);

	delete ref;
}


void
fssh_file_cache_enable(void *_cacheRef)
{
	fssh_panic("fssh_file_cache_enable() called");
}


fssh_status_t
fssh_file_cache_disable(void *_cacheRef)
{
	fssh_panic("fssh_file_cache_disable() called");
	return FSSH_B_ERROR;
}


bool
fssh_file_cache_is_enabled(void *_cacheRef)
{
	return true;
}


fssh_status_t
fssh_file_cache_set_size(void *_cacheRef, fssh_off_t size)
{
	file_cache_ref *ref = (file_cache_ref *)_cacheRef;

	TRACE(("file_cache_set_size(ref = %p, size = %Ld)\n", ref, size));

	if (ref == NULL)
		return FSSH_B_OK;

	fssh_mutex_lock(&ref->lock);
	ref->virtual_size = size;
	fssh_mutex_unlock(&ref->lock);

	return FSSH_B_OK;
}


fssh_status_t
fssh_file_cache_sync(void *_cacheRef)
{
	file_cache_ref *ref = (file_cache_ref *)_cacheRef;
	if (ref == NULL)
		return FSSH_B_BAD_VALUE;

	return FSSH_B_OK;
}


fssh_status_t
fssh_file_cache_read(void *_cacheRef, void *cookie, fssh_off_t offset,
	void *bufferBase, fssh_size_t *_size)
{
	file_cache_ref *ref = (file_cache_ref *)_cacheRef;

	TRACE(("file_cache_read(ref = %p, offset = %Ld, buffer = %p, size = %u)\n",
		ref, offset, bufferBase, *_size));

	return cache_io(ref, cookie, offset, (fssh_addr_t)bufferBase, _size, false);
}


fssh_status_t
fssh_file_cache_write(void *_cacheRef, void *cookie, fssh_off_t offset,
	const void *buffer, fssh_size_t *_size)
{
	file_cache_ref *ref = (file_cache_ref *)_cacheRef;

	fssh_status_t status = cache_io(ref, cookie, offset,
		(fssh_addr_t)const_cast<void *>(buffer), _size, true);
	TRACE(("file_cache_write(ref = %p, offset = %Ld, buffer = %p, size = %u) = %d\n",
		ref, offset, buffer, *_size, status));

	return status;
}

