/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "vnode_store.h"

#include <KernelExport.h>
#include <fs_cache.h>

#include <util/kernel_cpp.h>
#include <file_cache.h>
#include <vfs.h>
#include <vm.h>
#include <vm_page.h>
#include <vm_cache.h>
#include <generic_syscall.h>

#include <unistd.h>
#include <stdlib.h>
#include <string.h>


//#define TRACE_FILE_CACHE
#ifdef TRACE_FILE_CACHE
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

// maximum number of iovecs per request
#define MAX_IO_VECS			64	// 256 kB
#define MAX_FILE_IO_VECS	32

#define CACHED_FILE_EXTENTS	2
	// must be smaller than MAX_FILE_IO_VECS
	// ToDo: find out how much of these are typically used

struct file_extent {
	off_t			offset;
	file_io_vec		disk;
};

struct file_map {
	file_map();
	~file_map();

	file_extent *operator[](uint32 index);
	file_extent *ExtentAt(uint32 index);
	status_t Add(file_io_vec *vecs, size_t vecCount);
	void Free();

	union {
		file_extent	direct[CACHED_FILE_EXTENTS];
		file_extent	*array;
	};
	size_t			count;
};

struct file_cache_ref {
	vm_cache_ref	*cache;
	void			*vnode;
	void			*device;
	void			*cookie;
	file_map		map;
};


static struct cache_module_info *sCacheModule;


file_map::file_map()
{
	array = NULL;
	count = 0;
}


file_map::~file_map()
{
	Free();
}


file_extent *
file_map::operator[](uint32 index)
{
	return ExtentAt(index);
}


file_extent *
file_map::ExtentAt(uint32 index)
{
	if (index >= count)
		return NULL;

	if (count > CACHED_FILE_EXTENTS)
		return &array[index];

	return &direct[index];
}


status_t
file_map::Add(file_io_vec *vecs, size_t vecCount)
{
	off_t offset = 0;

	if (vecCount <= CACHED_FILE_EXTENTS && count == 0) {
		// just use the reserved area in the file_cache_ref structure
	} else {
		file_extent *newMap = (file_extent *)realloc(array,
			(count + vecCount) * sizeof(file_extent));
		if (newMap == NULL)
			return B_NO_MEMORY;

		array = newMap;

		if (count != 0) {
			file_extent *extent = ExtentAt(count - 1);
			offset = extent->offset + extent->disk.length;
		}
	}

	count += vecCount;

	for (uint32 i = 0; i < vecCount; i++) {
		file_extent *extent = ExtentAt(i);

		extent->offset = offset;
		extent->disk = vecs[i];

		offset += extent->disk.length;
	}

	return B_OK;
}


void
file_map::Free()
{
	if (count > CACHED_FILE_EXTENTS)
		free(array);

	array = NULL;
	count = 0;
}


//	#pragma mark -


static void
add_to_iovec(iovec *vecs, int32 &index, int32 max, addr_t address, size_t size)
{
	if (index > 0 && (addr_t)vecs[index - 1].iov_base + vecs[index - 1].iov_len == address) {
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


static file_extent *
find_file_extent(file_cache_ref *ref, off_t offset, uint32 *_index)
{
	// ToDo: do binary search

	for (uint32 index = 0; index < ref->map.count; index++) {
		file_extent *extent = ref->map[index];

		if (extent->offset <= offset
			&& extent->offset + extent->disk.length > offset) {
			if (_index)
				*_index = index;
			return extent;
		}
	}

	return NULL;
}


static status_t
get_file_map(file_cache_ref *ref, off_t offset, size_t size,
	file_io_vec *vecs, size_t *_count)
{
	size_t maxVecs = *_count;

	if (ref->map.count == 0) {
		// we don't yet have the map of this file, so let's grab it
		// (ordered by offset, so that we can do a binary search on them)

		mutex_lock(&ref->cache->lock);

		// the file map could have been requested in the mean time
		if (ref->map.count == 0) {
			size_t vecCount = maxVecs;
			status_t status;
			off_t mapOffset = 0;
	
			while (true) {
				status = vfs_get_file_map(ref->vnode, mapOffset, ~0UL, vecs, &vecCount);
				if (status < B_OK && status != B_BUFFER_OVERFLOW) {
					mutex_unlock(&ref->cache->lock);
					return status;
				}

				ref->map.Add(vecs, vecCount);

				if (status != B_BUFFER_OVERFLOW)
					break;

				// when we are here, the map has been stored in the array, and
				// the array size was still too small to cover the whole file
				file_io_vec *last = &vecs[vecCount - 1];
				mapOffset += last->length;
				vecCount = maxVecs;
			}
		}

		mutex_unlock(&ref->cache->lock);
	}

	// We now have cached the map of this file, we now need to
	// translate it for the requested access.

	uint32 index;
	file_extent *fileExtent = find_file_extent(ref, offset, &index);
	if (fileExtent == NULL) {
		// access outside file bounds? But that's not our problem
		*_count = 0;
		return B_OK;
	}

	offset -= fileExtent->offset;
	vecs[0].offset = fileExtent->disk.offset + offset;
	vecs[0].length = fileExtent->disk.length - offset;

	if (vecs[0].length >= size || index >= ref->map.count - 1) {
		*_count = 1;
		return B_OK;
	}

	// copy the rest of the vecs

	size -= vecs[0].length;

	for (index = 1; index < ref->map.count;) {
		fileExtent++;

		vecs[index] = fileExtent->disk;
		index++;

		if (index >= maxVecs) {
			*_count = index;
			return B_BUFFER_OVERFLOW;
		}

		if (size <= fileExtent->disk.length)
			break;

		size -= fileExtent->disk.length;
	}

	*_count = index;
	return B_OK;
}


static status_t
pages_io(file_cache_ref *ref, off_t offset, const iovec *vecs, size_t count,
	size_t *_numBytes, bool doWrite)
{
	TRACE(("pages_io: ref = %p, offset = %Ld, size = %lu, %s\n", ref, offset,
		*_numBytes, doWrite ? "write" : "read"));

	// translate the iovecs into direct device accesses
	file_io_vec fileVecs[MAX_FILE_IO_VECS];
	size_t fileVecCount = MAX_FILE_IO_VECS;
	size_t numBytes = *_numBytes;

	status_t status = get_file_map(ref, offset, numBytes, fileVecs, &fileVecCount);
	if (status < B_OK) {
		TRACE(("get_file_map(offset = %Ld, numBytes = %lu) failed\n", offset,
			numBytes));
		return status;
	}

	// ToDo: handle array overflow gracefully!

#ifdef TRACE_FILE_CACHE
	dprintf("got %lu file vecs for %Ld:%lu:\n", fileVecCount, offset, numBytes);
	for (size_t i = 0; i < fileVecCount; i++)
		dprintf("[%lu] offset = %Ld, size = %Ld\n", i, fileVecs[i].offset, fileVecs[i].length);
#endif

	uint32 fileVecIndex;
	size_t size;

	if (!doWrite) {
		// now directly read the data from the device
		// the first file_io_vec can be read directly

		size = fileVecs[0].length;
		if (size > numBytes)
			size = numBytes;

		status = vfs_read_pages(ref->device, ref->cookie, fileVecs[0].offset, vecs, count, &size);
		if (status < B_OK)
			return status;

		// ToDo: this is a work-around for buggy device drivers!
		//	When our own drivers honour the length, we can:
		//	a) also use this direct I/O for writes (otherwise, it would overwrite precious data)
		//	b) panic if the term below is true (at least for writes)
		if (size > fileVecs[0].length) {
			//dprintf("warning: device driver %p doesn't respect total length in read_pages() call!\n", ref->device);
			size = fileVecs[0].length;
		}

		ASSERT(size <= fileVecs[0].length);

		// If the file portion was contiguous, we're already done now
		if (size == numBytes)
			return B_OK;

		// if we reached the end of the file, we can return as well
		if (size != fileVecs[0].length) {
			*_numBytes = size;
			return B_OK;
		}
		
		fileVecIndex = 1;
	} else {
		fileVecIndex = 0;
		size = 0;
	}

	// Too bad, let's process the rest of the file_io_vecs

	size_t totalSize = size;

	// first, find out where we have to continue in our iovecs
	uint32 i = 0;
	for (; i < count; i++) {
		if (size <= vecs[i].iov_len)
			break;

		size -= vecs[i].iov_len;
	}

	size_t vecOffset = size;

	for (; fileVecIndex < fileVecCount; fileVecIndex++) {
		file_io_vec &fileVec = fileVecs[fileVecIndex];
		iovec tempVecs[8];
		uint32 tempCount = 1;

		tempVecs[0].iov_base = (void *)((addr_t)vecs[i].iov_base + vecOffset);

		size = min_c(vecs[i].iov_len - vecOffset, fileVec.length);
		tempVecs[0].iov_len = size;

		TRACE(("fill vec %ld, offset = %lu, size = %lu\n", i, vecOffset, size));

		if (size >= fileVec.length)
			vecOffset += size;
		else
			vecOffset = 0;

		while (size < fileVec.length && ++i < count) {
			tempVecs[tempCount].iov_base = vecs[i].iov_base;
			tempCount++;

			// is this iovec larger than the file_io_vec?
			if (vecs[i].iov_len + size > fileVec.length) {
				size += tempVecs[tempCount].iov_len = vecOffset = fileVec.length - size;
				break;
			}

			size += tempVecs[tempCount].iov_len = vecs[i].iov_len;
		}

		size_t bytes = size;
		if (doWrite)
			status = vfs_write_pages(ref->device, ref->cookie, fileVec.offset, tempVecs, tempCount, &bytes);
		else
			status = vfs_read_pages(ref->device, ref->cookie, fileVec.offset, tempVecs, tempCount, &bytes);
		if (status < B_OK)
			return status;

		totalSize += size;

		if (size != bytes) {
			// there are no more bytes, let's bail out
			*_numBytes = totalSize;
			return B_OK;
		}
	}

	return B_OK;
}


/**	This function is called by read_into_cache() (and from there only) - it
 *	can only handle a certain amount of bytes, and read_into_cache() makes
 *	sure that it matches that criterion.
 */

static inline status_t
read_chunk_into_cache(file_cache_ref *ref, off_t offset, size_t size,
	int32 pageOffset, addr_t buffer, size_t bufferSize)
{
	TRACE(("read_chunk(offset = %Ld, size = %lu, pageOffset = %ld, buffer = %#lx, bufferSize = %lu\n",
		offset, size, pageOffset, buffer, bufferSize));

	vm_cache_ref *cache = ref->cache;

	iovec vecs[MAX_IO_VECS];
	int32 vecCount = 0;

	vm_page *pages[MAX_IO_VECS];
	int32 pageIndex = 0;

	// allocate pages for the cache and mark them busy
	for (size_t pos = 0; pos < size; pos += B_PAGE_SIZE) {
		vm_page *page = pages[pageIndex++] = vm_page_allocate_page(PAGE_STATE_FREE);
		if (page == NULL)
			panic("no more pages!");

		page->state = PAGE_STATE_BUSY;

		vm_cache_insert_page(cache, page, offset + pos);

		addr_t virtualAddress;
		if (vm_get_physical_page(page->physical_page_number * B_PAGE_SIZE, &virtualAddress, PHYSICAL_PAGE_CAN_WAIT) < B_OK)
			panic("could not get physical page");

		add_to_iovec(vecs, vecCount, MAX_IO_VECS, virtualAddress, B_PAGE_SIZE);
		// ToDo: check if the array is large enough!
	}

	mutex_unlock(&cache->lock);

	// read file into reserved pages
	status_t status = pages_io(ref, offset, vecs, vecCount, &size, false);
	if (status < B_OK) {
		// reading failed, free allocated pages

		dprintf("file_cache: read pages failed: %s\n", strerror(status));

		mutex_lock(&cache->lock);
		for (int32 i = 0; i < pageIndex; i++) {
			vm_cache_remove_page(cache, pages[i]);
			vm_page_set_state(pages[i], PAGE_STATE_FREE);
		}

		return status;
	}

	// copy the pages and unmap them again

	for (int32 i = 0; i < vecCount; i++) {
		addr_t base = (addr_t)vecs[i].iov_base;
		size_t size = vecs[i].iov_len;

		// copy to user buffer if necessary
		if (bufferSize != 0) {
			size_t bytes = min_c(bufferSize, size - pageOffset);

			user_memcpy((void *)buffer, (void *)(base + pageOffset), bytes);
			buffer += bytes;
			bufferSize -= bytes;
			pageOffset = 0;
		}

		for (size_t pos = 0; pos < size; pos += B_PAGE_SIZE, base += B_PAGE_SIZE)
			vm_put_physical_page(base);
	}

	mutex_lock(&cache->lock);

	// make the pages accessible in the cache
	for (int32 i = pageIndex; i-- > 0;)
		pages[i]->state = PAGE_STATE_ACTIVE;

	return B_OK;
}


/**	This function reads \a size bytes directly from the file into the cache.
 *	If \a bufferSize does not equal zero, \a bufferSize bytes from the data
 *	read in are also copied to the provided \a buffer.
 *	This function always allocates all pages; it is the responsibility of the
 *	calling function to only ask for yet uncached ranges.
 *	The cache_ref lock must be hold when calling this function.
 */

static status_t
read_into_cache(file_cache_ref *ref, off_t offset, size_t size, addr_t buffer, size_t bufferSize)
{
	TRACE(("read_from_cache: ref = %p, offset = %Ld, size = %lu, buffer = %p, bufferSize = %lu\n",
		ref, offset, size, (void *)buffer, bufferSize));

	// do we have to read in anything at all?
	if (size == 0)
		return B_OK;

	// make sure "offset" is page aligned - but also remember the page offset
	int32 pageOffset = offset & (B_PAGE_SIZE - 1);
	size = PAGE_ALIGN(size + pageOffset);
	offset -= pageOffset;

	while (true) {
		size_t chunkSize = size;
		if (chunkSize > (MAX_IO_VECS * B_PAGE_SIZE))
			chunkSize = MAX_IO_VECS * B_PAGE_SIZE;

		status_t status = read_chunk_into_cache(ref, offset, chunkSize, pageOffset,
								buffer, bufferSize);
		if (status != B_OK)
			return status;

		if ((size -= chunkSize) == 0)
			return B_OK;

		if (chunkSize >= bufferSize) {
			bufferSize = 0;
			buffer = NULL;
		} else {
			bufferSize -= chunkSize - pageOffset;
			buffer += chunkSize - pageOffset;
		}

		offset += chunkSize;
		pageOffset = 0;
	}

	return B_OK;
}


/**	Like read_chunk_into_cache() but writes data into the cache */

static inline status_t
write_chunk_to_cache(file_cache_ref *ref, off_t offset, size_t size,
	int32 pageOffset, addr_t buffer, size_t bufferSize)
{
	iovec vecs[MAX_IO_VECS];
	int32 vecCount = 0;
	vm_page *pages[MAX_IO_VECS];
	int32 pageIndex = 0;
	status_t status = B_OK;

	// ToDo: this should be settable somewhere
	bool writeThrough = false;

	// allocate pages for the cache and mark them busy
	for (size_t pos = 0; pos < size; pos += B_PAGE_SIZE) {
		// ToDo: if space is becoming tight, and this cache is already grown
		//	big - shouldn't we better steal the pages directly in that case?
		//	(a working set like approach for the file cache)
		vm_page *page = pages[pageIndex++] = vm_page_allocate_page(PAGE_STATE_FREE);
		page->state = PAGE_STATE_BUSY;

		vm_cache_insert_page(ref->cache, page, offset + pos);

		addr_t virtualAddress;
		vm_get_physical_page(page->physical_page_number * B_PAGE_SIZE, &virtualAddress,
			PHYSICAL_PAGE_CAN_WAIT);

		add_to_iovec(vecs, vecCount, MAX_IO_VECS, virtualAddress, B_PAGE_SIZE);
		// ToDo: check if the array is large enough!
	}

	mutex_unlock(&ref->cache->lock);

	// copy contents (and read in partially written pages first)

	if (pageOffset != 0) {
		// This is only a partial write, so we have to read the rest of the page
		// from the file to have consistent data in the cache
		iovec readVec = { vecs[0].iov_base, B_PAGE_SIZE };
		size_t bytesRead = B_PAGE_SIZE;

		status = pages_io(ref, offset, &readVec, 1, &bytesRead, false);
		// ToDo: handle errors for real!
		if (status < B_OK)
			panic("1. pages_io() failed: %s!\n", strerror(status));
	}

	addr_t lastPageOffset = (pageOffset + bufferSize) & (B_PAGE_SIZE - 1);
	if (lastPageOffset != 0) {
		// get the last page in the I/O vectors
		addr_t last = (addr_t)vecs[vecCount - 1].iov_base
			+ vecs[vecCount - 1].iov_len - B_PAGE_SIZE;

		if (offset + pageOffset + bufferSize == ref->cache->cache->virtual_size) {
			// the space in the page after this write action needs to be cleaned
			memset((void *)(last + lastPageOffset), 0, B_PAGE_SIZE - lastPageOffset);
		} else if (vecCount > 1) {
			// the end of this write does not happen on a page boundary, so we
			// need to fetch the last page before we can update it
			iovec readVec = { (void *)last, B_PAGE_SIZE };
			size_t bytesRead = B_PAGE_SIZE;

			status = pages_io(ref, offset + size - B_PAGE_SIZE, &readVec, 1,
				&bytesRead, false);
			// ToDo: handle errors for real!
			if (status < B_OK)
				panic("pages_io() failed: %s!\n", strerror(status));
		}
	}

	for (int32 i = 0; i < vecCount; i++) {
		addr_t base = (addr_t)vecs[i].iov_base;
		size_t bytes = min_c(bufferSize, size_t(vecs[i].iov_len - pageOffset));

		// copy data from user buffer
		user_memcpy((void *)(base + pageOffset), (void *)buffer, bytes);

		bufferSize -= bytes;
		if (bufferSize == 0)
			break;

		buffer += bytes;
		pageOffset = 0;
	}

	if (writeThrough) {
		// write cached pages back to the file if we were asked to do that
		status_t status = pages_io(ref, offset, vecs, vecCount, &size, true);
		if (status < B_OK) {
			// ToDo: remove allocated pages, ...?
			panic("file_cache: remove allocated pages! write pages failed: %s\n",
				strerror(status));
		}
	}

	mutex_lock(&ref->cache->lock);

	// unmap the pages again

	for (int32 i = 0; i < vecCount; i++) {
		addr_t base = (addr_t)vecs[i].iov_base;
		size_t size = vecs[i].iov_len;
		for (size_t pos = 0; pos < size; pos += B_PAGE_SIZE, base += B_PAGE_SIZE)
			vm_put_physical_page(base);
	}

	// make the pages accessible in the cache
	for (int32 i = pageIndex; i-- > 0;) {
		if (writeThrough)
			pages[i]->state = PAGE_STATE_ACTIVE;
		else
			vm_page_set_state(pages[i], PAGE_STATE_MODIFIED);
	}

	return status;
}


/**	Like read_into_cache() but writes data into the cache. To preserve data consistency,
 *	it might also read pages into the cache, though, if only a partial page gets written.
 *	The cache_ref lock must be hold when calling this function.
 */

static status_t
write_to_cache(file_cache_ref *ref, off_t offset, size_t size, addr_t buffer, size_t bufferSize)
{
	TRACE(("write_to_cache: ref = %p, offset = %Ld, size = %lu, buffer = %p, bufferSize = %lu\n",
		ref, offset, size, (void *)buffer, bufferSize));

	// make sure "offset" is page aligned - but also remember the page offset
	int32 pageOffset = offset & (B_PAGE_SIZE - 1);
	size = PAGE_ALIGN(size + pageOffset);
	offset -= pageOffset;

	while (true) {
		size_t chunkSize = size;
		if (chunkSize > (MAX_IO_VECS * B_PAGE_SIZE))
			chunkSize = MAX_IO_VECS * B_PAGE_SIZE;

		status_t status = write_chunk_to_cache(ref, offset, chunkSize, pageOffset, buffer, bufferSize);
		if (status != B_OK)
			return status;

		if ((size -= chunkSize) == 0)
			return B_OK;

		if (chunkSize >= bufferSize) {
			bufferSize = 0;
			buffer = NULL;
		} else {
			bufferSize -= chunkSize - pageOffset;
			buffer += chunkSize - pageOffset;
		}

		offset += chunkSize;
		pageOffset = 0;
	}

	return B_OK;
}


static status_t
cache_io(void *_cacheRef, off_t offset, addr_t buffer, size_t *_size, bool doWrite)
{
	if (_cacheRef == NULL)
		panic("cache_io() called with NULL ref!\n");

	file_cache_ref *ref = (file_cache_ref *)_cacheRef;
	vm_cache_ref *cache = ref->cache;
	off_t fileSize = cache->cache->virtual_size;

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

	// "offset" and "lastOffset" are always aligned to B_PAGE_SIZE,
	// the "last*" variables always point to the end of the last
	// satisfied request part

	size_t bytesLeft = size, lastLeft = size;
	int32 lastPageOffset = pageOffset;
	addr_t lastBuffer = buffer;
	off_t lastOffset = offset;

	mutex_lock(&cache->lock);

	for (; bytesLeft > 0; offset += B_PAGE_SIZE) {
		// check if this page is already in memory
		addr_t virtualAddress;
	restart:
		vm_page *page = vm_cache_lookup_page(cache, offset);
		if (page != NULL && page->state == PAGE_STATE_BUSY) {
			// ToDo: don't wait forever!
			mutex_unlock(&cache->lock);
			snooze(20000);
			mutex_lock(&cache->lock);
			goto restart;
		}

		size_t bytesInPage = min_c(size_t(B_PAGE_SIZE - pageOffset), bytesLeft);

		TRACE(("lookup page from offset %Ld: %p, size = %lu, pageOffset = %lu\n", offset, page, bytesLeft, pageOffset));
		if (page != NULL
			&& vm_get_physical_page(page->physical_page_number * B_PAGE_SIZE,
					&virtualAddress, PHYSICAL_PAGE_CAN_WAIT) == B_OK) {
			// it is, so let's satisfy the first part of the request, if we have to
			if (lastBuffer != buffer) {
				size_t requestSize = buffer - lastBuffer;
				status_t status;
				if (doWrite) {
					status = write_to_cache(ref, lastOffset + lastPageOffset,
						requestSize, lastBuffer, requestSize);
				} else {
					status = read_into_cache(ref, lastOffset + lastPageOffset,
						requestSize, lastBuffer, requestSize);
				}
				if (status != B_OK) {
					vm_put_physical_page(virtualAddress);
					mutex_unlock(&cache->lock);
					return B_IO_ERROR;
				}
			}

			// and copy the contents of the page already in memory
			if (doWrite) {
				user_memcpy((void *)(virtualAddress + pageOffset), (void *)buffer, bytesInPage);

				// make sure the page is in the modified list
				if (page->state != PAGE_STATE_MODIFIED)
					vm_page_set_state(page, PAGE_STATE_MODIFIED);
			} else
				user_memcpy((void *)buffer, (void *)(virtualAddress + pageOffset), bytesInPage);

			vm_put_physical_page(virtualAddress);

			if (bytesLeft <= bytesInPage) {
				// we've read the last page, so we're done!
				mutex_unlock(&cache->lock);
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
	}

	// fill the last remaining bytes of the request (either write or read)

	status_t status;
	if (doWrite)
		status = write_to_cache(ref, lastOffset + lastPageOffset, lastLeft, lastBuffer, lastLeft);
	else
		status = read_into_cache(ref, lastOffset + lastPageOffset, lastLeft, lastBuffer, lastLeft);

	mutex_unlock(&cache->lock);
	return status;
}


static status_t
file_cache_control(const char *subsystem, uint32 function, void *buffer, size_t bufferSize)
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
				|| user_strlcpy(name, (char *)buffer, B_FILE_NAME_LENGTH) < B_OK)
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


//	#pragma mark -
//	kernel public API


extern "C" void
cache_prefetch_vnode(void *vnode, off_t offset, size_t size)
{
	vm_cache_ref *cache;
	if (vfs_get_vnode_cache(vnode, &cache, false) != B_OK)
		return;

	file_cache_ref *ref = (struct file_cache_ref *)((vnode_store *)cache->cache->store)->file_cache_ref;
	off_t fileSize = cache->cache->virtual_size;

	if (size > fileSize)
		size = fileSize;

	// we never fetch more than 4 MB at once
	if (size > 4 * 1024 * 1024)
		size = 4 * 1024 * 1024;

	size_t bytesLeft = size, lastLeft = size;
	off_t lastOffset = offset;
	size_t lastSize = 0;

	mutex_lock(&cache->lock);

	for (; bytesLeft > 0; offset += B_PAGE_SIZE) {
		// check if this page is already in memory
		addr_t virtualAddress;
	restart:
		vm_page *page = vm_cache_lookup_page(cache, offset);
		if (page != NULL) {
			// it is, so let's satisfy in the first part of the request
			if (lastOffset < offset) {
				size_t requestSize = offset - lastOffset;
				read_into_cache(ref, lastOffset, requestSize, NULL, 0);
			}

			if (bytesLeft <= B_PAGE_SIZE) {
				// we've read the last page, so we're done!
				goto out;
			}

			// prepare a potential gap request
			lastOffset = offset + B_PAGE_SIZE;
			lastLeft = bytesLeft - B_PAGE_SIZE;
		}

		if (bytesLeft <= B_PAGE_SIZE)
			break;

		bytesLeft -= B_PAGE_SIZE;
	}

	// read in the last part
	read_into_cache(ref, lastOffset, lastLeft, NULL, 0);

out:
	mutex_unlock(&cache->lock);
	vm_cache_release_ref(cache);
}


extern "C" void
cache_prefetch(mount_id mountID, vnode_id vnodeID, off_t offset, size_t size)
{
	void *vnode;

	// ToDo: schedule prefetch

	TRACE(("cache_prefetch(vnode %ld:%Ld)\n", mountID, vnodeID));

	// get the vnode for the object, this also grabs a ref to it
	if (vfs_get_vnode(mountID, vnodeID, &vnode) != B_OK)
		return;

	cache_prefetch_vnode(vnode, offset, size);
	vfs_put_vnode(vnode);
}


extern "C" void
cache_node_opened(void *vnode, int32 fdType, vm_cache_ref *cache, mount_id mountID,
	vnode_id parentID, vnode_id vnodeID, const char *name)
{
	if (sCacheModule == NULL || sCacheModule->node_opened == NULL)
		return;

	off_t size = -1;
	if (cache != NULL) {
		file_cache_ref *ref = (file_cache_ref *)((vnode_store *)cache->cache->store)->file_cache_ref;
		if (ref != NULL)
			size = ref->cache->cache->virtual_size;
	}

	sCacheModule->node_opened(vnode, fdType, mountID, parentID, vnodeID, name, size);
}


extern "C" void
cache_node_closed(void *vnode, int32 fdType, vm_cache_ref *cache,
	mount_id mountID, vnode_id vnodeID)
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

	if (get_module("file_cache/launch_speedup/v1", (module_info **)&sCacheModule) == B_OK) {
		dprintf("** opened launch speedup: %Ld\n", system_time());
	} else
		dprintf("** could not open launch speedup!\n");

	return B_OK;
}


extern "C" status_t
file_cache_init(void)
{
	register_generic_syscall(CACHE_SYSCALLS, file_cache_control, 1, 0);
	return B_OK;
}


//	#pragma mark -
//	public FS API


extern "C" void *
file_cache_create(mount_id mountID, vnode_id vnodeID, off_t size, int fd)
{
	TRACE(("file_cache_create(mountID = %ld, vnodeID = %Ld, size = %Ld, fd = %d)\n", mountID, vnodeID, size, fd));

	file_cache_ref *ref = new file_cache_ref;
	if (ref == NULL)
		return NULL;

	// ToDo: delay vm_cache/vm_cache_ref creation until data is
	//	requested/written for the first time? Listing lots of
	//	files in Tracker (and elsewhere) could be slowed down.
	//	Since the file_cache_ref itself doesn't have a lock,
	//	we would need to "rent" one during construction, possibly
	//	the vnode lock, maybe a dedicated one.
	//	As there shouldn't be too much contention, we could also
	//	use atomic_test_and_set(), and free the resources again
	//	when that fails...

	// get the vnode of the underlying device
	if (vfs_get_vnode_from_fd(fd, true, &ref->device) != B_OK)
		goto err1;

	// we also need the cookie of the underlying device to properly access it
	if (vfs_get_cookie_from_fd(fd, &ref->cookie) != B_OK)
		goto err2;

	// get the vnode for the object (note, this does not grab a reference to the node)
	if (vfs_lookup_vnode(mountID, vnodeID, &ref->vnode) != B_OK)
		goto err2;

	if (vfs_get_vnode_cache(ref->vnode, &ref->cache, true) != B_OK)
		goto err3;

	ref->cache->cache->virtual_size = size;
	((vnode_store *)ref->cache->cache->store)->file_cache_ref = ref;
	return ref;

err3:
	vfs_put_vnode(ref->vnode);
err2:
	vfs_put_vnode(ref->device);
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

	vfs_put_vnode(ref->device);
	delete ref;
}


extern "C" status_t
file_cache_set_size(void *_cacheRef, off_t size)
{
	file_cache_ref *ref = (file_cache_ref *)_cacheRef;

	TRACE(("file_cache_set_size(ref = %p, size = %Ld)\n", ref, size));

	if (ref == NULL)
		return B_OK;

	file_cache_invalidate_file_map(_cacheRef, 0, size);
		// ToDo: make this better (we would only need to extend or shrink the map)

	mutex_lock(&ref->cache->lock);
	status_t status = vm_cache_resize(ref->cache, size);
	mutex_unlock(&ref->cache->lock);

	return status;
}


extern "C" status_t
file_cache_sync(void *_cacheRef)
{
	file_cache_ref *ref = (file_cache_ref *)_cacheRef;
	if (ref == NULL)
		return B_BAD_VALUE;

	return vm_cache_write_modified(ref->cache);
}


extern "C" status_t
file_cache_read_pages(void *_cacheRef, off_t offset, const iovec *vecs, size_t count, size_t *_numBytes)
{
	file_cache_ref *ref = (file_cache_ref *)_cacheRef;

	return pages_io(ref, offset, vecs, count, _numBytes, false);
}


extern "C" status_t
file_cache_write_pages(void *_cacheRef, off_t offset, const iovec *vecs, size_t count, size_t *_numBytes)
{
	file_cache_ref *ref = (file_cache_ref *)_cacheRef;

	status_t status = pages_io(ref, offset, vecs, count, _numBytes, true);
	TRACE(("file_cache_write_pages(ref = %p, offset = %Ld, vecs = %p, count = %lu, bytes = %lu) = %ld\n",
		ref, offset, vecs, count, *_numBytes, status));

	return status;
}


extern "C" status_t
file_cache_read(void *_cacheRef, off_t offset, void *bufferBase, size_t *_size)
{
	file_cache_ref *ref = (file_cache_ref *)_cacheRef;

	TRACE(("file_cache_read(ref = %p, offset = %Ld, buffer = %p, size = %lu)\n",
		ref, offset, bufferBase, *_size));

	return cache_io(ref, offset, (addr_t)bufferBase, _size, false);
}


extern "C" status_t
file_cache_write(void *_cacheRef, off_t offset, const void *buffer, size_t *_size)
{
	file_cache_ref *ref = (file_cache_ref *)_cacheRef;

	status_t status = cache_io(ref, offset, (addr_t)const_cast<void *>(buffer), _size, true);
	TRACE(("file_cache_write(ref = %p, offset = %Ld, buffer = %p, size = %lu) = %ld\n",
		ref, offset, buffer, *_size, status));

	return status;
}


extern "C" status_t
file_cache_invalidate_file_map(void *_cacheRef, off_t offset, off_t size)
{
	file_cache_ref *ref = (file_cache_ref *)_cacheRef;

	// ToDo: honour offset/size parameters

	TRACE(("file_cache_invalidate_file_map(offset = %Ld, size = %Ld)\n", offset, size));
	mutex_lock(&ref->cache->lock);
	ref->map.Free();
	mutex_unlock(&ref->cache->lock);
	return B_OK;
}
