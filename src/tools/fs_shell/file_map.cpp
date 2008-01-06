/*
 * Copyright 2004-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "fssh_fs_cache.h"

#include <new>
#include <stdlib.h>

#include "fssh_kernel_export.h"
#include "vfs.h"


//#define TRACE_FILE_MAP
#ifdef TRACE_FILE_MAP
#	define TRACE(x) fssh_dprintf x
#else
#	define TRACE(x) ;
#endif

#define CACHED_FILE_EXTENTS	2
	// must be smaller than MAX_FILE_IO_VECS
	// ToDo: find out how much of these are typically used

using namespace FSShell;

namespace FSShell {

struct file_extent {
	fssh_off_t			offset;
	fssh_file_io_vec	disk;
};

struct file_map {
	file_map(fssh_off_t size);
	~file_map();

	file_extent *operator[](uint32_t index);
	file_extent *ExtentAt(uint32_t index);
	fssh_status_t Add(fssh_file_io_vec *vecs, fssh_size_t vecCount,
		fssh_off_t &lastOffset);
	void Free();

	union {
		file_extent	direct[CACHED_FILE_EXTENTS];
		file_extent	*array;
	};
	fssh_size_t		count;
	void			*vnode;
	fssh_off_t		size;
};


file_map::file_map(fssh_off_t _size)
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
file_map::operator[](uint32_t index)
{
	return ExtentAt(index);
}


file_extent *
file_map::ExtentAt(uint32_t index)
{
	if (index >= count)
		return NULL;

	if (count > CACHED_FILE_EXTENTS)
		return &array[index];

	return &direct[index];
}


fssh_status_t
file_map::Add(fssh_file_io_vec *vecs, fssh_size_t vecCount,
	fssh_off_t &lastOffset)
{
	TRACE(("file_map::Add(vecCount = %u)\n", vecCount));

	fssh_off_t offset = 0;

	if (vecCount <= CACHED_FILE_EXTENTS && count == 0) {
		// just use the reserved area in the file_cache_ref structure
	} else {
		// TODO: once we can invalidate only parts of the file map,
		//	we might need to copy the previously cached file extends
		//	from the direct range
		file_extent *newMap = (file_extent *)realloc(array,
			(count + vecCount) * sizeof(file_extent));
		if (newMap == NULL)
			return FSSH_B_NO_MEMORY;

		array = newMap;

		if (count != 0) {
			file_extent *extent = ExtentAt(count - 1);
			offset = extent->offset + extent->disk.length;
		}
	}

	int32_t start = count;
	count += vecCount;

	for (uint32_t i = 0; i < vecCount; i++) {
		file_extent *extent = ExtentAt(start + i);

		extent->offset = offset;
		extent->disk = vecs[i];

		offset += extent->disk.length;
	}

#ifdef TRACE_FILE_MAP
	for (uint32_t i = 0; i < count; i++) {
		file_extent *extent = ExtentAt(i);
		fssh_dprintf("[%u] extend offset %Ld, disk offset %Ld, length %Ld\n",
			i, extent->offset, extent->disk.offset, extent->disk.length);
	}
#endif

	lastOffset = offset;
	return FSSH_B_OK;
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
find_file_extent(file_map &map, fssh_off_t offset, uint32_t *_index)
{
	// TODO: do binary search

	for (uint32_t index = 0; index < map.count; index++) {
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


}	// namespace FSShell


//	#pragma mark - public FS API


extern "C" void *
fssh_file_map_create(fssh_mount_id mountID, fssh_vnode_id vnodeID,
	fssh_off_t size)
{
	TRACE(("file_map_create(mountID = %d, vnodeID = %Ld)\n", mountID, vnodeID));

	file_map *map = new file_map(size);
	if (map == NULL)
		return NULL;

	// Get the vnode for the object
	// (note, this does not grab a reference to the node)
	if (vfs_lookup_vnode(mountID, vnodeID, &map->vnode) != FSSH_B_OK) {
		delete map;
		return NULL;
	}

	return map;
}


extern "C" void
fssh_file_map_delete(void *_map)
{
	file_map *map = (file_map *)_map;
	if (map == NULL)
		return;

	TRACE(("file_map_delete(map = %p)\n", map));
	delete map;
}


extern "C" void
fssh_file_map_set_size(void *_map, fssh_off_t size)
{
	if (_map == NULL)
		return;

	// TODO: honour offset/size parameters
	file_map *map = (file_map *)_map;
	//if (size < map->size)
		map->Free();
	map->size = size;
}


extern "C" void
fssh_file_map_invalidate(void *_map, fssh_off_t offset, fssh_off_t size)
{
	if (_map == NULL)
		return;

	// TODO: honour offset/size parameters
	file_map *map = (file_map *)_map;
	map->Free();
}


extern "C" fssh_status_t
fssh_file_map_translate(void *_map, fssh_off_t offset, fssh_size_t size,
	fssh_file_io_vec *vecs, fssh_size_t *_count)
{
	if (_map == NULL)
		return FSSH_B_BAD_VALUE;

	file_map &map = *(file_map *)_map;
	fssh_size_t maxVecs = *_count;
	fssh_status_t status = FSSH_B_OK;

	if (offset >= map.size) {
		*_count = 0;
		return FSSH_B_OK;
	}
	if (offset + size > map.size)
		size = map.size - offset;

	if (map.count == 0) {
		// we don't yet have the map of this file, so let's grab it
		// (ordered by offset, so that we can do a binary search on them)
		fssh_size_t vecCount = maxVecs;
		fssh_off_t mapOffset = 0;

		while (true) {
			status = vfs_get_file_map(map.vnode, mapOffset, ~0UL, vecs,
				&vecCount);
			if (status < FSSH_B_OK && status != FSSH_B_BUFFER_OVERFLOW)
				return status;

			fssh_status_t addStatus = map.Add(vecs, vecCount, mapOffset);
			if (addStatus != FSSH_B_OK) {
				// only clobber the status in case of failure
				status = addStatus;
			}

			if (status != FSSH_B_BUFFER_OVERFLOW)
				break;

			// when we are here, the map has been stored in the array, and
			// the array size was still too small to cover the whole file
			vecCount = maxVecs;
		}
	}

	if (status != FSSH_B_OK) {
		// We must invalidate the (part of the) map we already
		// have, as we cannot know if it's complete or not
		map.Free();
		return status;
	}

	// We now have cached the map of this file, we now need to
	// translate it for the requested access.

	uint32_t index;
	file_extent *fileExtent = find_file_extent(map, offset, &index);
	if (fileExtent == NULL) {
		// access outside file bounds? But that's not our problem
		*_count = 0;
		return FSSH_B_OK;
	}

	offset -= fileExtent->offset;
	vecs[0].offset = fileExtent->disk.offset + offset;
	vecs[0].length = fileExtent->disk.length - offset;

	if (vecs[0].length >= size || index >= map.count - 1) {
		*_count = 1;
		return FSSH_B_OK;
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
			return FSSH_B_BUFFER_OVERFLOW;
		}

		size -= fileExtent->disk.length;
	}

	*_count = index;
	return FSSH_B_OK;
}

