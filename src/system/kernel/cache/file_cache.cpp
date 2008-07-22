/*
 * Copyright 2004-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "vnode_store.h"

#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <KernelExport.h>
#include <fs_cache.h>

#include <condition_variable.h>
#include <file_cache.h>
#include <generic_syscall.h>
#include <low_resource_manager.h>
#include <util/AutoLock.h>
#include <util/kernel_cpp.h>
#include <vfs.h>
#include <vm.h>
#include <vm_page.h>
#include <vm_cache.h>


//#define TRACE_FILE_CACHE
#ifdef TRACE_FILE_CACHE
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

// maximum number of iovecs per request
#define MAX_IO_VECS			32	// 128 kB
#define MAX_FILE_IO_VECS	32

#define BYPASS_IO_SIZE		65536
#define LAST_ACCESSES		3

struct file_cache_ref {
	vm_cache		*cache;
	struct vnode	*vnode;
	off_t			last_access[LAST_ACCESSES];
		// TODO: it would probably be enough to only store the least
		//	significant 31 bits, and make this uint32 (one bit for
		//	write vs. read)
	int32			last_access_index;
	bool			last_access_was_write;
};

typedef status_t (*cache_func)(file_cache_ref *ref, void *cookie, off_t offset,
	int32 pageOffset, addr_t buffer, size_t bufferSize, bool useBuffer,
	size_t lastReservedPages, size_t reservePages);


static struct cache_module_info *sCacheModule;


//	#pragma mark -


static void
add_to_iovec(iovec *vecs, int32 &index, int32 max, addr_t address, size_t size)
{
	if (index > 0 && (addr_t)vecs[index - 1].iov_base
			+ vecs[index - 1].iov_len == address) {
		// the iovec can be combined with the previous one
		vecs[index - 1].iov_len += size;
		return;
	}

	if (index == max)
		panic("no more space for iovecs!");

	// we need to start a new iovec
	vecs[index].iov_base = (void *)address;
	vecs[index].iov_len = size;
	index++;
}


static inline bool
access_is_sequential(file_cache_ref *ref)
{
	return ref->last_access[ref->last_access_index] != 0;
}


static inline void
push_access(file_cache_ref *ref, off_t offset, size_t bytes, bool isWrite)
{
	TRACE(("%p: push %Ld, %ld, %s\n", ref, offset, bytes,
		isWrite ? "write" : "read"));

	int32 index = ref->last_access_index;
	int32 previous = index - 1;
	if (previous < 0)
		previous = LAST_ACCESSES - 1;

	if (offset != ref->last_access[previous])
		ref->last_access[previous] = 0;

	// we remember writes as negative offsets
	if (isWrite)
		ref->last_access[index] = -offset - bytes;
	else
		ref->last_access[index] = offset + bytes;

	if (++index >= LAST_ACCESSES)
		index = 0;
	ref->last_access_index = index;
}


static void
reserve_pages(file_cache_ref *ref, size_t reservePages, bool isWrite)
{
	if (low_resource_state(B_KERNEL_RESOURCE_PAGES) != B_NO_LOW_RESOURCE) {
		vm_cache *cache = ref->cache;
		cache->Lock();

		if (list_is_empty(&cache->consumers) && cache->areas == NULL
			&& access_is_sequential(ref)) {
			// we are not mapped, and we're accessed sequentially

			if (isWrite) {
				// just schedule some pages to be written back
				for (VMCachePagesTree::Iterator it = cache->pages.GetIterator();
						vm_page* page = it.Next();) {
					if (page->state == PAGE_STATE_MODIFIED) {
						// TODO: for now, we only schedule one
						vm_page_schedule_write_page(page);
						break;
					}
				}
			} else {
				// free some pages from our cache
				// TODO: start with oldest
				uint32 left = reservePages;
				vm_page *page;
				for (VMCachePagesTree::Iterator it = cache->pages.GetIterator();
						(page = it.Next()) != NULL && left > 0;) {
					if (page->state != PAGE_STATE_MODIFIED
						&& page->state != PAGE_STATE_BUSY) {
						cache->RemovePage(page);
						vm_page_set_state(page, PAGE_STATE_FREE);
						left--;
					}
				}
			}
		}
		cache->Unlock();
	}

	vm_page_reserve_pages(reservePages);
}


/*!	Reads the requested amount of data into the cache, and allocates
	pages needed to fulfill that request. This function is called by cache_io().
	It can only handle a certain amount of bytes, and the caller must make
	sure that it matches that criterion.
	The cache_ref lock must be hold when calling this function; during
	operation it will unlock the cache, though.
*/
static status_t
read_into_cache(file_cache_ref *ref, void *cookie, off_t offset,
	int32 pageOffset, addr_t buffer, size_t bufferSize, bool useBuffer,
	size_t lastReservedPages, size_t reservePages)
{
	TRACE(("read_into_cache(offset = %Ld, pageOffset = %ld, buffer = %#lx, "
		"bufferSize = %lu\n", offset, pageOffset, buffer, bufferSize));

	vm_cache *cache = ref->cache;

	// TODO: We're using way too much stack! Rather allocate a sufficiently
	// large chunk on the heap.
	iovec vecs[MAX_IO_VECS];
	int32 vecCount = 0;

	size_t numBytes = PAGE_ALIGN(pageOffset + bufferSize);
	vm_page *pages[MAX_IO_VECS];
	ConditionVariable busyConditions[MAX_IO_VECS];
	int32 pageIndex = 0;

	// allocate pages for the cache and mark them busy
	for (size_t pos = 0; pos < numBytes; pos += B_PAGE_SIZE) {
		vm_page *page = pages[pageIndex++] = vm_page_allocate_page(
			PAGE_STATE_FREE, true);
		if (page == NULL)
			panic("no more pages!");

		busyConditions[pageIndex - 1].Publish(page, "page");

		cache->InsertPage(page, offset + pos);

		addr_t virtualAddress;
		if (vm_get_physical_page(page->physical_page_number * B_PAGE_SIZE,
				&virtualAddress, PHYSICAL_PAGE_CAN_WAIT) < B_OK)
			panic("could not get physical page");

		add_to_iovec(vecs, vecCount, MAX_IO_VECS, virtualAddress, B_PAGE_SIZE);
			// TODO: check if the array is large enough (currently panics)!
	}

	push_access(ref, offset, bufferSize, false);
	cache->Unlock();
	vm_page_unreserve_pages(lastReservedPages);

	// read file into reserved pages
	status_t status = vfs_read_pages(ref->vnode, cookie, offset, vecs,
		vecCount, &numBytes, false);
	if (status < B_OK) {
		// reading failed, free allocated pages

		dprintf("file_cache: read pages failed: %s\n", strerror(status));

		for (int32 i = 0; i < vecCount; i++) {
			addr_t base = (addr_t)vecs[i].iov_base;
			size_t size = vecs[i].iov_len;

			for (size_t pos = 0; pos < size;
					pos += B_PAGE_SIZE, base += B_PAGE_SIZE) {
				vm_put_physical_page(base);
			}
		}

		cache->Lock();

		for (int32 i = 0; i < pageIndex; i++) {
			busyConditions[i].Unpublish();
			cache->RemovePage(pages[i]);
			vm_page_set_state(pages[i], PAGE_STATE_FREE);
		}

		return status;
	}

	// copy the pages if needed and unmap them again

	for (int32 i = 0; i < vecCount; i++) {
		addr_t base = (addr_t)vecs[i].iov_base;
		size_t size = vecs[i].iov_len;

		// copy to user buffer if necessary
		if (useBuffer && bufferSize != 0) {
			size_t bytes = min_c(bufferSize, size - pageOffset);

			user_memcpy((void *)buffer, (void *)(base + pageOffset), bytes);
			buffer += bytes;
			bufferSize -= bytes;
			pageOffset = 0;
		}

		for (size_t pos = 0; pos < size; pos += B_PAGE_SIZE,
				base += B_PAGE_SIZE) {
			vm_put_physical_page(base);
		}
	}

	reserve_pages(ref, reservePages, false);
	cache->Lock();

	// make the pages accessible in the cache
	for (int32 i = pageIndex; i-- > 0;) {
		pages[i]->state = PAGE_STATE_ACTIVE;

		busyConditions[i].Unpublish();
	}

	return B_OK;
}


static status_t
read_from_file(file_cache_ref *ref, void *cookie, off_t offset,
	int32 pageOffset, addr_t buffer, size_t bufferSize, bool useBuffer,
	size_t lastReservedPages, size_t reservePages)
{
	TRACE(("read_from_file(offset = %Ld, pageOffset = %ld, buffer = %#lx, "
		"bufferSize = %lu\n", offset, pageOffset, buffer, bufferSize));

	if (!useBuffer)
		return B_OK;

	iovec vec;
	vec.iov_base = (void *)buffer;
	vec.iov_len = bufferSize;

	push_access(ref, offset, bufferSize, false);
	ref->cache->Unlock();
	vm_page_unreserve_pages(lastReservedPages);

	status_t status = vfs_read_pages(ref->vnode, cookie, offset + pageOffset,
		&vec, 1, &bufferSize, false);
	if (status == B_OK)
		reserve_pages(ref, reservePages, false);

	ref->cache->Lock();

	return status;
}


/*!	Like read_into_cache() but writes data into the cache.
	To preserve data consistency, it might also read pages into the cache,
	though, if only a partial page gets written.
	The same restrictions apply.
*/
static status_t
write_to_cache(file_cache_ref *ref, void *cookie, off_t offset,
	int32 pageOffset, addr_t buffer, size_t bufferSize, bool useBuffer,
	size_t lastReservedPages, size_t reservePages)
{
	// TODO: We're using way too much stack! Rather allocate a sufficiently
	// large chunk on the heap.
	iovec vecs[MAX_IO_VECS];
	int32 vecCount = 0;
	size_t numBytes = PAGE_ALIGN(pageOffset + bufferSize);
	vm_page *pages[MAX_IO_VECS];
	int32 pageIndex = 0;
	status_t status = B_OK;
	ConditionVariable busyConditions[MAX_IO_VECS];

	// ToDo: this should be settable somewhere
	bool writeThrough = false;

	// allocate pages for the cache and mark them busy
	for (size_t pos = 0; pos < numBytes; pos += B_PAGE_SIZE) {
		// TODO: if space is becoming tight, and this cache is already grown
		//	big - shouldn't we better steal the pages directly in that case?
		//	(a working set like approach for the file cache)
		// TODO: the pages we allocate here should have been reserved upfront
		//	in cache_io()
		vm_page *page = pages[pageIndex++] = vm_page_allocate_page(
			PAGE_STATE_FREE, true);
		busyConditions[pageIndex - 1].Publish(page, "page");

		ref->cache->InsertPage(page, offset + pos);

		addr_t virtualAddress;
		vm_get_physical_page(page->physical_page_number * B_PAGE_SIZE,
			&virtualAddress, PHYSICAL_PAGE_CAN_WAIT);

		add_to_iovec(vecs, vecCount, MAX_IO_VECS, virtualAddress, B_PAGE_SIZE);
		// ToDo: check if the array is large enough!
	}

	push_access(ref, offset, bufferSize, true);
	ref->cache->Unlock();
	vm_page_unreserve_pages(lastReservedPages);

	// copy contents (and read in partially written pages first)

	if (pageOffset != 0) {
		// This is only a partial write, so we have to read the rest of the page
		// from the file to have consistent data in the cache
		iovec readVec = { vecs[0].iov_base, B_PAGE_SIZE };
		size_t bytesRead = B_PAGE_SIZE;

		status = vfs_read_pages(ref->vnode, cookie, offset, &readVec, 1,
			&bytesRead, false);
		// ToDo: handle errors for real!
		if (status < B_OK)
			panic("1. vfs_read_pages() failed: %s!\n", strerror(status));
	}

	addr_t lastPageOffset = (pageOffset + bufferSize) & (B_PAGE_SIZE - 1);
	if (lastPageOffset != 0) {
		// get the last page in the I/O vectors
		addr_t last = (addr_t)vecs[vecCount - 1].iov_base
			+ vecs[vecCount - 1].iov_len - B_PAGE_SIZE;

		if (offset + pageOffset + bufferSize == ref->cache->virtual_end) {
			// the space in the page after this write action needs to be cleaned
			memset((void *)(last + lastPageOffset), 0,
				B_PAGE_SIZE - lastPageOffset);
		} else {
			// the end of this write does not happen on a page boundary, so we
			// need to fetch the last page before we can update it
			iovec readVec = { (void *)last, B_PAGE_SIZE };
			size_t bytesRead = B_PAGE_SIZE;

			status = vfs_read_pages(ref->vnode, cookie,
				PAGE_ALIGN(offset + pageOffset + bufferSize) - B_PAGE_SIZE,
				&readVec, 1, &bytesRead, false);
			// ToDo: handle errors for real!
			if (status < B_OK)
				panic("vfs_read_pages() failed: %s!\n", strerror(status));

			if (bytesRead < B_PAGE_SIZE) {
				// the space beyond the file size needs to be cleaned
				memset((void *)(last + bytesRead), 0, B_PAGE_SIZE - bytesRead);
			}
		}
	}

	for (int32 i = 0; i < vecCount; i++) {
		addr_t base = (addr_t)vecs[i].iov_base;
		size_t bytes = min_c(bufferSize, size_t(vecs[i].iov_len - pageOffset));

		if (useBuffer) {
			// copy data from user buffer
			user_memcpy((void *)(base + pageOffset), (void *)buffer, bytes);
		} else {
			// clear buffer instead
			memset((void *)(base + pageOffset), 0, bytes);
		}

		bufferSize -= bytes;
		if (bufferSize == 0)
			break;

		buffer += bytes;
		pageOffset = 0;
	}

	if (writeThrough) {
		// write cached pages back to the file if we were asked to do that
		status_t status = vfs_write_pages(ref->vnode, cookie, offset, vecs,
			vecCount, &numBytes, false);
		if (status < B_OK) {
			// ToDo: remove allocated pages, ...?
			panic("file_cache: remove allocated pages! write pages failed: %s\n",
				strerror(status));
		}
	}

	if (status == B_OK)
		reserve_pages(ref, reservePages, true);

	ref->cache->Lock();

	// unmap the pages again

	for (int32 i = 0; i < vecCount; i++) {
		addr_t base = (addr_t)vecs[i].iov_base;
		size_t size = vecs[i].iov_len;
		for (size_t pos = 0; pos < size; pos += B_PAGE_SIZE,
				base += B_PAGE_SIZE) {
			vm_put_physical_page(base);
		}
	}

	// make the pages accessible in the cache
	for (int32 i = pageIndex; i-- > 0;) {
		busyConditions[i].Unpublish();

		if (writeThrough)
			pages[i]->state = PAGE_STATE_ACTIVE;
		else
			vm_page_set_state(pages[i], PAGE_STATE_MODIFIED);
	}

	return status;
}


static status_t
write_to_file(file_cache_ref *ref, void *cookie, off_t offset, int32 pageOffset,
	addr_t buffer, size_t bufferSize, bool useBuffer, size_t lastReservedPages,
	size_t reservePages)
{
	size_t chunkSize;
	if (!useBuffer) {
		// we need to allocate a zero buffer
		// TODO: use smaller buffers if this fails
		chunkSize = min_c(bufferSize, B_PAGE_SIZE);
		buffer = (addr_t)malloc(chunkSize);
		if (buffer == 0)
			return B_NO_MEMORY;

		memset((void *)buffer, 0, chunkSize);
	}

	iovec vec;
	vec.iov_base = (void *)buffer;
	vec.iov_len = bufferSize;

	push_access(ref, offset, bufferSize, true);
	ref->cache->Unlock();
	vm_page_unreserve_pages(lastReservedPages);

	status_t status = B_OK;

	if (!useBuffer) {
		while (bufferSize > 0) {
			if (bufferSize < chunkSize)
				chunkSize = bufferSize;

			status = vfs_write_pages(ref->vnode, cookie, offset + pageOffset,
				&vec, 1, &chunkSize, false);
			if (status < B_OK)
				break;

			bufferSize -= chunkSize;
			pageOffset += chunkSize;
		}

		free((void*)buffer);
	} else {
		status = vfs_write_pages(ref->vnode, cookie, offset + pageOffset,
			&vec, 1, &bufferSize, false);
	}

	if (status == B_OK)
		reserve_pages(ref, reservePages, true);

	ref->cache->Lock();

	return status;
}


static inline status_t
satisfy_cache_io(file_cache_ref *ref, void *cookie, cache_func function,
	off_t offset, addr_t buffer, bool useBuffer, int32 &pageOffset,
	size_t bytesLeft, size_t &reservePages, off_t &lastOffset,
	addr_t &lastBuffer, int32 &lastPageOffset, size_t &lastLeft,
	size_t &lastReservedPages)
{
	if (lastBuffer == buffer)
		return B_OK;

	size_t requestSize = buffer - lastBuffer;
	reservePages = min_c(MAX_IO_VECS, (lastLeft - requestSize
		+ lastPageOffset + B_PAGE_SIZE - 1) >> PAGE_SHIFT);

	status_t status = function(ref, cookie, lastOffset, lastPageOffset,
		lastBuffer, requestSize, useBuffer, lastReservedPages, reservePages);
	if (status == B_OK) {
		lastReservedPages = reservePages;
		lastBuffer = buffer;
		lastLeft = bytesLeft;
		lastOffset = offset;
		lastPageOffset = 0;
		pageOffset = 0;
	}
	return status;
}


static status_t
cache_io(void *_cacheRef, void *cookie, off_t offset, addr_t buffer,
	size_t *_size, bool doWrite)
{
	if (_cacheRef == NULL)
		panic("cache_io() called with NULL ref!\n");

	file_cache_ref *ref = (file_cache_ref *)_cacheRef;
	vm_cache *cache = ref->cache;
	off_t fileSize = cache->virtual_end;
	bool useBuffer = buffer != 0;

	TRACE(("cache_io(ref = %p, offset = %Ld, buffer = %p, size = %lu, %s)\n",
		ref, offset, (void *)buffer, *_size, doWrite ? "write" : "read"));

	// out of bounds access?
	if (offset >= fileSize || offset < 0) {
		*_size = 0;
		return B_OK;
	}

	int32 pageOffset = offset & (B_PAGE_SIZE - 1);
	size_t size = *_size;
	offset -= pageOffset;

	if (offset + pageOffset + size > fileSize) {
		// adapt size to be within the file's offsets
		size = fileSize - pageOffset - offset;
		*_size = size;
	}
	if (size == 0)
		return B_OK;

	cache_func function;
	if (doWrite) {
		// in low memory situations, we bypass the cache beyond a
		// certain I/O size
		if (size >= BYPASS_IO_SIZE
			&& low_resource_state(B_KERNEL_RESOURCE_PAGES)
				!= B_NO_LOW_RESOURCE) {
			function = write_to_file;
		} else
			function = write_to_cache;
	} else {
		if (size >= BYPASS_IO_SIZE
			&& low_resource_state(B_KERNEL_RESOURCE_PAGES)
				!= B_NO_LOW_RESOURCE) {
			function = read_from_file;
		} else
			function = read_into_cache;
	}

	// "offset" and "lastOffset" are always aligned to B_PAGE_SIZE,
	// the "last*" variables always point to the end of the last
	// satisfied request part

	const uint32 kMaxChunkSize = MAX_IO_VECS * B_PAGE_SIZE;
	size_t bytesLeft = size, lastLeft = size;
	int32 lastPageOffset = pageOffset;
	addr_t lastBuffer = buffer;
	off_t lastOffset = offset;
	size_t lastReservedPages = min_c(MAX_IO_VECS, (pageOffset + bytesLeft
		+ B_PAGE_SIZE - 1) >> PAGE_SHIFT);
	size_t reservePages = 0;

	reserve_pages(ref, lastReservedPages, doWrite);
	AutoLocker<VMCache> locker(cache);

	while (bytesLeft > 0) {
		// check if this page is already in memory
		vm_page *page = cache->LookupPage(offset);
		if (page != NULL) {
			// The page may be busy - since we need to unlock the cache sometime
			// in the near future, we need to satisfy the request of the pages
			// we didn't get yet (to make sure no one else interferes in the
			// mean time).
			status_t status = satisfy_cache_io(ref, cookie, function, offset,
				buffer, useBuffer, pageOffset, bytesLeft, reservePages,
				lastOffset, lastBuffer, lastPageOffset, lastLeft,
				lastReservedPages);
			if (status != B_OK)
				return status;

			if (page->state == PAGE_STATE_BUSY) {
				ConditionVariableEntry entry;
				entry.Add(page);
				locker.Unlock();
				entry.Wait();
				locker.Lock();
				continue;
			}
		}

		size_t bytesInPage = min_c(size_t(B_PAGE_SIZE - pageOffset), bytesLeft);
		addr_t virtualAddress;

		TRACE(("lookup page from offset %Ld: %p, size = %lu, pageOffset "
			"= %lu\n", offset, page, bytesLeft, pageOffset));

		if (page != NULL) {
			vm_get_physical_page(page->physical_page_number * B_PAGE_SIZE,
				&virtualAddress, PHYSICAL_PAGE_CAN_WAIT);

			// Since we don't actually map pages as part of an area, we have
			// to manually maintain its usage_count
			page->usage_count = 2;

			// and copy the contents of the page already in memory
			if (doWrite) {
				if (useBuffer) {
					user_memcpy((void *)(virtualAddress + pageOffset),
						(void *)buffer, bytesInPage);
				} else {
					user_memset((void *)(virtualAddress + pageOffset),
						0, bytesInPage);
				}

				// make sure the page is in the modified list
				if (page->state != PAGE_STATE_MODIFIED)
					vm_page_set_state(page, PAGE_STATE_MODIFIED);
			} else if (useBuffer) {
				user_memcpy((void *)buffer,
					(void *)(virtualAddress + pageOffset), bytesInPage);
			}

			vm_put_physical_page(virtualAddress);

			if (bytesLeft <= bytesInPage) {
				// we've read the last page, so we're done!
				locker.Unlock();
				vm_page_unreserve_pages(lastReservedPages);
				return B_OK;
			}

			// prepare a potential gap request
			lastBuffer = buffer + bytesInPage;
			lastLeft = bytesLeft - bytesInPage;
			lastOffset = offset + B_PAGE_SIZE;
			lastPageOffset = 0;
		}

		if (bytesLeft <= bytesInPage)
			break;

		buffer += bytesInPage;
		bytesLeft -= bytesInPage;
		pageOffset = 0;
		offset += B_PAGE_SIZE;

		if (buffer - lastBuffer + lastPageOffset >= kMaxChunkSize) {
			status_t status = satisfy_cache_io(ref, cookie, function, offset,
				buffer, useBuffer, pageOffset, bytesLeft, reservePages,
				lastOffset, lastBuffer, lastPageOffset, lastLeft,
				lastReservedPages);
			if (status != B_OK)
				return status;
		}
	}

	// fill the last remaining bytes of the request (either write or read)

	return function(ref, cookie, lastOffset, lastPageOffset, lastBuffer,
		lastLeft, useBuffer, lastReservedPages, 0);
}


static status_t
file_cache_control(const char *subsystem, uint32 function, void *buffer,
	size_t bufferSize)
{
	switch (function) {
		case CACHE_CLEAR:
			// ToDo: clear the cache
			dprintf("cache_control: clear cache!\n");
			return B_OK;

		case CACHE_SET_MODULE:
		{
			cache_module_info *module = sCacheModule;

			// unset previous module

			if (sCacheModule != NULL) {
				sCacheModule = NULL;
				snooze(100000);	// 0.1 secs
				put_module(module->info.name);
			}

			// get new module, if any

			if (buffer == NULL)
				return B_OK;

			char name[B_FILE_NAME_LENGTH];
			if (!IS_USER_ADDRESS(buffer)
				|| user_strlcpy(name, (char *)buffer,
						B_FILE_NAME_LENGTH) < B_OK)
				return B_BAD_ADDRESS;

			if (strncmp(name, CACHE_MODULES_NAME, strlen(CACHE_MODULES_NAME)))
				return B_BAD_VALUE;

			dprintf("cache_control: set module %s!\n", name);

			status_t status = get_module(name, (module_info **)&module);
			if (status == B_OK)
				sCacheModule = module;

			return status;
		}
	}

	return B_BAD_HANDLER;
}


//	#pragma mark - private kernel API


extern "C" void
cache_prefetch_vnode(struct vnode *vnode, off_t offset, size_t size)
{
	vm_cache *cache;
	if (vfs_get_vnode_cache(vnode, &cache, false) != B_OK)
		return;

	file_cache_ref *ref = ((VMVnodeCache*)cache)->FileCacheRef();
	off_t fileSize = cache->virtual_end;

	if (size > fileSize)
		size = fileSize;

	// we never fetch more than 4 MB at once
	if (size > 4 * 1024 * 1024)
		size = 4 * 1024 * 1024;

	cache_io(ref, NULL, offset, 0, &size, false);
	cache->Lock();
	cache->ReleaseRefAndUnlock();
}


extern "C" void
cache_prefetch(dev_t mountID, ino_t vnodeID, off_t offset, size_t size)
{
	// ToDo: schedule prefetch

	TRACE(("cache_prefetch(vnode %ld:%Ld)\n", mountID, vnodeID));

	// get the vnode for the object, this also grabs a ref to it
	struct vnode *vnode;
	if (vfs_get_vnode(mountID, vnodeID, true, &vnode) != B_OK)
		return;

	cache_prefetch_vnode(vnode, offset, size);
	vfs_put_vnode(vnode);
}


extern "C" void
cache_node_opened(struct vnode *vnode, int32 fdType, vm_cache *cache,
	dev_t mountID, ino_t parentID, ino_t vnodeID, const char *name)
{
	if (sCacheModule == NULL || sCacheModule->node_opened == NULL)
		return;

	off_t size = -1;
	if (cache != NULL) {
		file_cache_ref *ref = ((VMVnodeCache*)cache)->FileCacheRef();
		if (ref != NULL)
			size = cache->virtual_end;
	}

	sCacheModule->node_opened(vnode, fdType, mountID, parentID, vnodeID, name,
		size);
}


extern "C" void
cache_node_closed(struct vnode *vnode, int32 fdType, vm_cache *cache,
	dev_t mountID, ino_t vnodeID)
{
	if (sCacheModule == NULL || sCacheModule->node_closed == NULL)
		return;

	int32 accessType = 0;
	if (cache != NULL) {
		// ToDo: set accessType
	}

	sCacheModule->node_closed(vnode, fdType, mountID, vnodeID, accessType);
}


extern "C" void
cache_node_launched(size_t argCount, char * const *args)
{
	if (sCacheModule == NULL || sCacheModule->node_launched == NULL)
		return;

	sCacheModule->node_launched(argCount, args);
}


extern "C" status_t
file_cache_init_post_boot_device(void)
{
	// ToDo: get cache module out of driver settings

	if (get_module("file_cache/launch_speedup/v1",
			(module_info **)&sCacheModule) == B_OK) {
		dprintf("** opened launch speedup: %Ld\n", system_time());
	}
	return B_OK;
}


extern "C" status_t
file_cache_init(void)
{
	register_generic_syscall(CACHE_SYSCALLS, file_cache_control, 1, 0);
	return B_OK;
}


//	#pragma mark - public FS API


extern "C" void *
file_cache_create(dev_t mountID, ino_t vnodeID, off_t size)
{
	TRACE(("file_cache_create(mountID = %ld, vnodeID = %Ld, size = %Ld)\n",
		mountID, vnodeID, size));

	file_cache_ref *ref = new file_cache_ref;
	if (ref == NULL)
		return NULL;

	memset(ref->last_access, 0, sizeof(ref->last_access));
	ref->last_access_index = 0;

	// TODO: delay vm_cache creation until data is
	//	requested/written for the first time? Listing lots of
	//	files in Tracker (and elsewhere) could be slowed down.
	//	Since the file_cache_ref itself doesn't have a lock,
	//	we would need to "rent" one during construction, possibly
	//	the vnode lock, maybe a dedicated one.
	//	As there shouldn't be too much contention, we could also
	//	use atomic_test_and_set(), and free the resources again
	//	when that fails...

	// Get the vnode for the object
	// (note, this does not grab a reference to the node)
	if (vfs_lookup_vnode(mountID, vnodeID, &ref->vnode) != B_OK)
		goto err1;

	// Gets (usually creates) the cache for the node
	if (vfs_get_vnode_cache(ref->vnode, &ref->cache, true) != B_OK)
		goto err1;

	ref->cache->virtual_end = size;
	((VMVnodeCache*)ref->cache)->SetFileCacheRef(ref);
	return ref;

err1:
	delete ref;
	return NULL;
}


extern "C" void
file_cache_delete(void *_cacheRef)
{
	file_cache_ref *ref = (file_cache_ref *)_cacheRef;

	if (ref == NULL)
		return;

	TRACE(("file_cache_delete(ref = %p)\n", ref));

	ref->cache->ReleaseRef();
	delete ref;
}


extern "C" status_t
file_cache_set_size(void *_cacheRef, off_t newSize)
{
	file_cache_ref *ref = (file_cache_ref *)_cacheRef;

	TRACE(("file_cache_set_size(ref = %p, size = %Ld)\n", ref, newSize));

	if (ref == NULL)
		return B_OK;

	AutoLocker<VMCache> _(ref->cache);

	off_t offset = ref->cache->virtual_end;
	off_t size = newSize;
	if (offset > newSize) {
		size = offset - newSize;
		offset = newSize;
	} else
		size = newSize - offset;

	return ref->cache->Resize(newSize);
}


extern "C" status_t
file_cache_sync(void *_cacheRef)
{
	file_cache_ref *ref = (file_cache_ref *)_cacheRef;
	if (ref == NULL)
		return B_BAD_VALUE;

	return ref->cache->WriteModified(true);
}


extern "C" status_t
file_cache_read(void *_cacheRef, void *cookie, off_t offset, void *buffer,
	size_t *_size)
{
	file_cache_ref *ref = (file_cache_ref *)_cacheRef;

	TRACE(("file_cache_read(ref = %p, offset = %Ld, buffer = %p, size = %lu)\n",
		ref, offset, buffer, *_size));

	return cache_io(ref, cookie, offset, (addr_t)buffer, _size, false);
}


extern "C" status_t
file_cache_write(void *_cacheRef, void *cookie, off_t offset,
	const void *buffer, size_t *_size)
{
	file_cache_ref *ref = (file_cache_ref *)_cacheRef;

	status_t status = cache_io(ref, cookie, offset,
		(addr_t)const_cast<void *>(buffer), _size, true);

	TRACE(("file_cache_write(ref = %p, offset = %Ld, buffer = %p, size = %lu)"
		" = %ld\n", ref, offset, buffer, *_size, status));

	return status;
}

