/*
 * Copyright 2004-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <KernelExport.h>
#include <fs_cache.h>

#include <condition_variable.h>
#include <file_cache.h>
#include <generic_syscall.h>
#include <util/AutoLock.h>
#include <util/kernel_cpp.h>
#include <vfs.h>
#include <vm.h>
#include <vm_page.h>
#include <vm_cache.h>
#include <vm_low_memory.h>


//#define TRACE_FILE_MAP
#ifdef TRACE_FILE_MAP
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

#define CACHED_FILE_EXTENTS	2
	// must be smaller than MAX_FILE_IO_VECS
	// ToDo: find out how much of these are typically used

struct file_extent {
	off_t			offset;
	file_io_vec		disk;
};

struct file_map {
	file_map(off_t size);
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
	struct vnode	*vnode;
	off_t			size;
};


file_map::file_map(off_t _size)
{
	array = NULL;
	count = 0;
	size = _size;
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

#ifdef TRACE_FILE_MAP
	for (uint32 i = 0; i < count; i++) {
		file_extent *extent = ExtentAt(i);
		dprintf("[%ld] extend offset %Ld, disk offset %Ld, length %Ld\n",
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


static file_extent *
find_file_extent(file_map &map, off_t offset, uint32 *_index)
{
	// TODO: do binary search

	for (uint32 index = 0; index < map.count; index++) {
		file_extent *extent = map[index];

		if (extent->offset <= offset
			&& extent->offset + extent->disk.length > offset) {
			if (_index)
				*_index = index;
			return extent;
		}
	}

	return NULL;
}


//	#pragma mark - public FS API


extern "C" void *
file_map_create(dev_t mountID, ino_t vnodeID, off_t size)
{
	TRACE(("file_map_create(mountID = %ld, vnodeID = %Ld, size = %Ld)\n",
		mountID, vnodeID, size));

	file_map *map = new file_map(size);
	if (map == NULL)
		return NULL;

	// Get the vnode for the object
	// (note, this does not grab a reference to the node)
	if (vfs_lookup_vnode(mountID, vnodeID, &map->vnode) != B_OK) {
		delete map;
		return NULL;
	}

	return map;
}


extern "C" void
file_map_delete(void *_map)
{
	file_map *map = (file_map *)_map;
	if (map == NULL)
		return;

	TRACE(("file_map_delete(map = %p)\n", map));
	delete map;
}


extern "C" void
file_map_set_size(void *_map, off_t size)
{
	if (_map == NULL)
		return;

	// TODO: honour offset/size parameters
	file_map *map = (file_map *)_map;
	if (size < map->size)
		map->Free();
	map->size = size;
}


extern "C" void
file_map_invalidate(void *_map, off_t offset, off_t size)
{
	if (_map == NULL)
		return;

	// TODO: honour offset/size parameters
	file_map *map = (file_map *)_map;
	map->Free();
}


extern "C" status_t
file_map_translate(void *_map, off_t offset, size_t size, file_io_vec *vecs,
	size_t *_count)
{
	if (_map == NULL)
		return B_BAD_VALUE;

	TRACE(("file_map_translate(map %p, offset %Ld, size %ld)\n",
		_map, offset, size));

	file_map &map = *(file_map *)_map;
	size_t maxVecs = *_count;
	status_t status = B_OK;

	if (offset > map.size) {
		*_count = 0;
		return B_OK;
	}
	if (offset + size > map.size)
		size = map.size - offset;

	if (map.count == 0) {
		// we don't yet have the map of this file, so let's grab it
		// (ordered by offset, so that we can do a binary search on them)
		size_t vecCount = maxVecs;
		off_t mapOffset = 0;

		while (true) {
			status = vfs_get_file_map(map.vnode, mapOffset, ~0UL, vecs,
				&vecCount);
			if (status < B_OK && status != B_BUFFER_OVERFLOW)
				return status;

			status_t addStatus = map.Add(vecs, vecCount, mapOffset);
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

	if (status != B_OK) {
		// We must invalidate the (part of the) map we already
		// have, as we cannot know if it's complete or not
		map.Free();
		return status;
	}

	// We now have cached the map of this file, we now need to
	// translate it for the requested access.

	uint32 index;
	file_extent *fileExtent = find_file_extent(map, offset, &index);
	if (fileExtent == NULL) {
		// access outside file bounds? But that's not our problem
		*_count = 0;
		return B_OK;
	}

	offset -= fileExtent->offset;
	vecs[0].offset = fileExtent->disk.offset + offset;
	vecs[0].length = fileExtent->disk.length - offset;

	if (vecs[0].length >= size || index >= map.count - 1) {
		*_count = 1;
		return B_OK;
	}

	// copy the rest of the vecs

	size -= vecs[0].length;

	for (index = 1; index < map.count;) {
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

