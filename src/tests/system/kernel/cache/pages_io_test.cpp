/*
 * Copyright 2004-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <OS.h>
#include <fs_interface.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>

#define TRACE_FILE_CACHE
#define TRACE(x) printf x
#define dprintf printf

#ifndef ASSERT
#	define ASSERT(x) ;
#endif

// maximum number of iovecs per request
#define MAX_IO_VECS			64	// 256 kB
#define MAX_FILE_IO_VECS	4
#define MAX_TEMP_IO_VECS	8

#define CACHED_FILE_EXTENTS	2
	// must be smaller than MAX_FILE_IO_VECS
	// ToDo: find out how much of these are typically used

struct vm_cache_ref;

struct file_extent {
	off_t			offset;
	file_io_vec		disk;
};

struct file_map {
	file_map();
	~file_map();

	file_extent *operator[](uint32 index);
	file_extent *ExtentAt(uint32 index);
	status_t Add(file_io_vec *vecs, size_t vecCount, off_t &lastOffset);
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


const uint32 kMaxFileVecs = 1024;

file_io_vec gFileVecs[kMaxFileVecs];
size_t gFileVecCount;
off_t gFileSize;


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
file_map::Add(file_io_vec *vecs, size_t vecCount, off_t &lastOffset)
{
	TRACE(("file_map::Add(vecCount = %ld)\n", vecCount));

	off_t offset = 0;

	if (vecCount <= CACHED_FILE_EXTENTS && count == 0) {
		// just use the reserved area in the file_cache_ref structure
	} else {
		// TODO: once we can invalidate only parts of the file map,
		//	we might need to copy the previously cached file extends
		//	from the direct range
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

	int32 start = count;
	count += vecCount;

	for (uint32 i = 0; i < vecCount; i++) {
		file_extent *extent = ExtentAt(start + i);

		extent->offset = offset;
		extent->disk = vecs[i];

		offset += extent->disk.length;
	}

#ifdef TRACE_FILE_CACHE
	for (uint32 i = 0; i < count; i++) {
		file_extent *extent = ExtentAt(i);
		dprintf("  [%ld] extend offset %Ld, disk offset %Ld, length %Ld\n",
			i, extent->offset, extent->disk.offset, extent->disk.length);
	}
#endif

	lastOffset = offset;
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


void
set_vecs(iovec *vecs, size_t *_count, ...)
{
	uint32 base = 0;
	size_t count = 0;

	va_list args;
	va_start(args, _count);

	while (count < MAX_IO_VECS) {
		int32 length = va_arg(args, int32);
		if (length < 0)
			break;

		vecs[count].iov_base = (void *)base;
		vecs[count].iov_len = length;

		base += length;
		count++;
	}

	va_end(args);
	*_count = count;
}


void
set_file_map(int32 base, int32 length, ...)
{
	gFileVecs[0].offset = base;
	gFileVecs[0].length = length;

	gFileSize = length;
	gFileVecCount = 1;

	va_list args;
	va_start(args, length);

	while (gFileVecCount < kMaxFileVecs) {
		off_t offset = va_arg(args, int32);
		if (offset < 0)
			break;

		length = va_arg(args, int32);

		gFileVecs[gFileVecCount].offset = offset;
		gFileVecs[gFileVecCount].length = length;

		gFileSize += length;
		gFileVecCount++;
	}

	va_end(args);
}


status_t
find_map_base(off_t offset, off_t &diskOffset, off_t &diskLength,
	off_t &fileOffset)
{
	fileOffset = 0;

	for (uint32 i = 0; i < gFileVecCount; i++) {
		if (offset < gFileVecs[i].length) {
			diskOffset = gFileVecs[i].offset;
			diskLength = gFileVecs[i].length;
			return B_OK;
		}

		fileOffset += gFileVecs[i].length;
		offset -= gFileVecs[i].length;
	}

	return B_ENTRY_NOT_FOUND;
}


//	#pragma mark - VFS functions


static status_t
vfs_get_file_map(void *vnode, off_t offset, size_t size, file_io_vec *vecs,
	size_t *_count)
{
	off_t diskOffset, diskLength, fileOffset;
	size_t max = *_count;
	uint32 index = 0;

	printf("vfs_get_file_map(offset = %Ld, size = %lu, count = %lu)\n",
		offset, size, *_count);

	while (true) {
		status_t status = find_map_base(offset, diskOffset, diskLength, fileOffset);
		//status_t status = inode->FindBlockRun(offset, run, fileOffset);
		if (status != B_OK)
			return status;

		vecs[index].offset = diskOffset + offset - fileOffset;
		vecs[index].length = diskLength - offset + fileOffset;
		offset += vecs[index].length;

		// are we already done?
		if (size <= vecs[index].length
			|| offset >= gFileSize) {
			if (offset > gFileSize) {
				// make sure the extent ends with the last official file
				// block (without taking any preallocations into account)
				vecs[index].length = gFileSize - fileOffset;
			}
			*_count = index + 1;
			return B_OK;
		}

		size -= vecs[index].length;
		index++;

		if (index >= max) {
			// we're out of file_io_vecs; let's bail out
			*_count = index;
			return B_BUFFER_OVERFLOW;
		}
	}
}


static status_t
vfs_read_pages(void *device, void *cookie, off_t offset,
	const iovec *vecs, size_t count, size_t *bytes, bool kernel)
{
	printf("read offset %Ld, length %lu\n", offset, *bytes);
	for (uint32 i = 0; i < count; i++) {
		printf("  [%lu] base %lu, length %lu\n",
			i, (uint32)vecs[i].iov_base, vecs[i].iov_len);
	}
	return B_OK;
}


static status_t
vfs_write_pages(void *device, void *cookie, off_t offset,
	const iovec *vecs, size_t count, size_t *bytes, bool kernel)
{
	printf("write offset %Ld, length %lu\n", offset, *bytes);
	for (uint32 i = 0; i < count; i++) {
		printf("  [%lu] base %lu, length %lu\n",
			i, (uint32)vecs[i].iov_base, vecs[i].iov_len);
	}
	return B_OK;
}


//	#pragma mark - file_cache.cpp copies


static file_extent *
find_file_extent(file_cache_ref *ref, off_t offset, uint32 *_index)
{
	// TODO: do binary search

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
	status_t status = B_OK;

	if (ref->map.count == 0) {
		// we don't yet have the map of this file, so let's grab it
		// (ordered by offset, so that we can do a binary search on them)

		//mutex_lock(&ref->cache->lock);

		// the file map could have been requested in the mean time
		if (ref->map.count == 0) {
			size_t vecCount = maxVecs;
			off_t mapOffset = 0;

			while (true) {
				status = vfs_get_file_map(ref->vnode, mapOffset, ~0UL, vecs, &vecCount);
				if (status < B_OK && status != B_BUFFER_OVERFLOW) {
					//mutex_unlock(&ref->cache->lock);
					return status;
				}

				status_t addStatus = ref->map.Add(vecs, vecCount, mapOffset);
				if (addStatus != B_OK) {
					// only clobber the status in case of failure
					status = addStatus;
				}

				if (status != B_BUFFER_OVERFLOW)
					break;

				// when we are here, the map has been stored in the array, and
				// the array size was still too small to cover the whole file
				vecCount = maxVecs;
			}
		}

		//mutex_unlock(&ref->cache->lock);
	}

	if (status != B_OK) {
		// We must invalidate the (part of the) map we already
		// have, as we cannot know if it's complete or not
		ref->map.Free();
		return status;
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

		if (size <= fileExtent->disk.length)
			break;

		if (index >= maxVecs) {
			*_count = index;
			return B_BUFFER_OVERFLOW;
		}

		size -= fileExtent->disk.length;
	}

	*_count = index;
	return B_OK;
}


/*!
	Does the dirty work of translating the request into actual disk offsets
	and reads to or writes from the supplied iovecs as specified by \a doWrite.
*/
static status_t
pages_io(file_cache_ref *ref, off_t offset, const iovec *vecs, size_t count,
	size_t *_numBytes, bool doWrite)
{
	TRACE(("pages_io: ref = %p, offset = %Ld, size = %lu, vecCount = %lu, %s\n", ref, offset,
		*_numBytes, count, doWrite ? "write" : "read"));

	// translate the iovecs into direct device accesses
	file_io_vec fileVecs[MAX_FILE_IO_VECS];
	size_t fileVecCount = MAX_FILE_IO_VECS;
	size_t numBytes = *_numBytes;

	status_t status = get_file_map(ref, offset, numBytes, fileVecs,
		&fileVecCount);
	if (status < B_OK && status != B_BUFFER_OVERFLOW) {
		TRACE(("get_file_map(offset = %Ld, numBytes = %lu) failed: %s\n", offset,
			numBytes, strerror(status)));
		return status;
	}

	bool bufferOverflow = status == B_BUFFER_OVERFLOW;

#ifdef TRACE_FILE_CACHE
	dprintf("got %lu file vecs for %Ld:%lu%s:\n", fileVecCount, offset, numBytes,
		bufferOverflow ? " (array too small)" : "");
	for (size_t i = 0; i < fileVecCount; i++) {
		dprintf("  [%lu] offset = %Ld, size = %Ld\n",
			i, fileVecs[i].offset, fileVecs[i].length);
	}
#endif

	if (fileVecCount == 0) {
		// There are no file vecs at this offset, so we're obviously trying
		// to access the file outside of its bounds
		TRACE(("pages_io: access outside of vnode %p at offset %Ld\n",
			ref->vnode, offset));
		return B_BAD_VALUE;
	}

	uint32 fileVecIndex;
	size_t size;

	if (!doWrite) {
		// now directly read the data from the device
		// the first file_io_vec can be read directly

		size = fileVecs[0].length;
		if (size > numBytes)
			size = numBytes;

		status = vfs_read_pages(ref->device, ref->cookie, fileVecs[0].offset, vecs,
			count, &size, false);
		if (status < B_OK)
			return status;

		// TODO: this is a work-around for buggy device drivers!
		//	When our own drivers honour the length, we can:
		//	a) also use this direct I/O for writes (otherwise, it would
		//	   overwrite precious data)
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
		if (size < vecs[i].iov_len)
			break;

		size -= vecs[i].iov_len;
	}

	size_t vecOffset = size;
	size_t bytesLeft = numBytes - size;

	while (true) {
		for (; fileVecIndex < fileVecCount; fileVecIndex++) {
			file_io_vec &fileVec = fileVecs[fileVecIndex];
			off_t fileOffset = fileVec.offset;
			off_t fileLeft = min_c(fileVec.length, bytesLeft);

			TRACE(("FILE VEC [%lu] length %Ld\n", fileVecIndex, fileLeft));

			// process the complete fileVec
			while (fileLeft > 0) {
				iovec tempVecs[MAX_TEMP_IO_VECS];
				uint32 tempCount = 0;

				// size tracks how much of what is left of the current fileVec
				// (fileLeft) has been assigned to tempVecs 
				size = 0;

				// assign what is left of the current fileVec to the tempVecs
				for (size = 0; size < fileLeft && i < count
						&& tempCount < MAX_TEMP_IO_VECS;) {
					// try to satisfy one iovec per iteration (or as much as
					// possible)

					// bytes left of the current iovec
					size_t vecLeft = vecs[i].iov_len - vecOffset;
					if (vecLeft == 0) {
						vecOffset = 0;
						i++;
						continue;
					}

					TRACE(("fill vec %ld, offset = %lu, size = %lu\n",
						i, vecOffset, size));

					// actually available bytes
					size_t tempVecSize = min_c(vecLeft, fileLeft - size);

					tempVecs[tempCount].iov_base
						= (void *)((addr_t)vecs[i].iov_base + vecOffset);
					tempVecs[tempCount].iov_len = tempVecSize;
					tempCount++;

					size += tempVecSize;
					vecOffset += tempVecSize;
				}

				size_t bytes = size;
				if (doWrite) {
					status = vfs_write_pages(ref->device, ref->cookie,
						fileOffset, tempVecs, tempCount, &bytes, false);
				} else {
					status = vfs_read_pages(ref->device, ref->cookie,
						fileOffset, tempVecs, tempCount, &bytes, false);
				}
				if (status < B_OK)
					return status;

				totalSize += bytes;
				bytesLeft -= size;
				fileOffset += size;
				fileLeft -= size;
				//dprintf("-> file left = %Lu\n", fileLeft);

				if (size != bytes || i >= count) {
					// there are no more bytes or iovecs, let's bail out
					*_numBytes = totalSize;
					return B_OK;
				}
			}
		}

		if (bufferOverflow) {
			status = get_file_map(ref, offset + totalSize, bytesLeft, fileVecs,
				&fileVecCount);
			if (status < B_OK && status != B_BUFFER_OVERFLOW) {
				TRACE(("get_file_map(offset = %Ld, numBytes = %lu) failed: %s\n",
					offset, numBytes, strerror(status)));
				return status;
			}

			bufferOverflow = status == B_BUFFER_OVERFLOW;
			fileVecIndex = 0;

#ifdef TRACE_FILE_CACHE
			dprintf("got %lu file vecs for %Ld:%lu%s:\n", fileVecCount,
				offset + totalSize, numBytes,
				bufferOverflow ? " (array too small)" : "");
			for (size_t i = 0; i < fileVecCount; i++) {
				dprintf("  [%lu] offset = %Ld, size = %Ld\n",
					i, fileVecs[i].offset, fileVecs[i].length);
			}
#endif
		} else
			break;
	}

	*_numBytes = totalSize;
	return B_OK;
}


//	#pragma mark -


int
main(int argc, char **argv)
{
	file_cache_ref ref;
	iovec vecs[MAX_IO_VECS];
	size_t count = 1;
	size_t numBytes = 10000;
	off_t offset = 4999;

	set_vecs(vecs, &count, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
		16, 4096, 8192, 16384, 4096, 4096, -1);
	set_file_map(0, 2000, 5000, 3000, 10000, 800, 11000, 20, 12000, 30,
		13000, 70, 14000, 100, 15000, 900, 20000, 30000, -1);

	status_t status = pages_io(&ref, offset, vecs, count, &numBytes, false);
	if (status < B_OK)
		fprintf(stderr, "pages_io() returned: %s\n", strerror(status));

	return 0;
}

