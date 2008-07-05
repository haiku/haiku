/*
 * Copyright 2004-2008, Axel Dörfler, axeld@pinc-software.de.
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

struct file_extent_array {
	file_extent		*array;
	size_t			max_count;
};

struct file_map {
	file_map(off_t size);
	~file_map();

	file_extent *operator[](uint32 index);
	file_extent *ExtentAt(uint32 index);
	file_extent *FindExtent(off_t offset, uint32 *_index);
	status_t Add(file_io_vec *vecs, size_t vecCount, off_t &lastOffset);
	void Invalidate(off_t offset, off_t size);
	void Free();

	status_t _MakeSpace(size_t amount);

	union {
		file_extent	direct[CACHED_FILE_EXTENTS];
		file_extent_array indirect;
	};
	size_t			count;
	struct vnode	*vnode;
	off_t			size;
};


file_map::file_map(off_t _size)
{
	indirect.array = NULL;
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
		return &indirect.array[index];

	return &direct[index];
}


file_extent *
file_map::FindExtent(off_t offset, uint32 *_index)
{
	int32 left = 0;
	int32 right = count - 1;

	while (left <= right) {
		int32 index = (left + right) / 2;
		file_extent *extent = ExtentAt(index);

		if (extent->offset > offset) {
			// search in left part
			right = index - 1;
		} else if (extent->offset + extent->disk.length <= offset) {
			// search in right part
			left = index + 1;
		} else {
			// found extent
			if (_index)
				*_index = index;

			return extent;
		}
	}

	return NULL;
}


status_t
file_map::_MakeSpace(size_t amount)
{
	if (amount <= CACHED_FILE_EXTENTS) {
		// just use the reserved area in the file_cache_ref structure
		if (amount <= CACHED_FILE_EXTENTS && count > CACHED_FILE_EXTENTS) {
			// the new size is smaller than the minimal array size
			file_extent *array = indirect.array;
			memcpy(direct, array, sizeof(file_extent) * amount);
			free(array);
		}
	} else {
		// resize array if needed
		file_extent *oldArray = NULL;
		size_t maxCount = CACHED_FILE_EXTENTS;
		if (count > CACHED_FILE_EXTENTS) {
			oldArray = indirect.array;
			maxCount = indirect.max_count;
		}

		if (amount > maxCount) {
			// allocate new array
			while (maxCount < amount) {
				if (maxCount < 32768)
					maxCount <<= 1;
				else
					maxCount += 32768;
			}

			file_extent *newArray = (file_extent *)realloc(oldArray,
				maxCount * sizeof(file_extent));
			if (newArray == NULL)
				return B_NO_MEMORY;

			if (count > 0 && count <= CACHED_FILE_EXTENTS)
				memcpy(newArray, direct, sizeof(file_extent) * count);

			indirect.array = newArray;
			indirect.max_count = maxCount;
		}
	}

	count = amount;
	return B_OK;
}


status_t
file_map::Add(file_io_vec *vecs, size_t vecCount, off_t &lastOffset)
{
	TRACE(("file_map@%p::Add(vecCount = %ld)\n", this, vecCount));

	uint32 start = count;
	off_t offset = 0;

	status_t status = _MakeSpace(count + vecCount);
	if (status != B_OK)
		return status;

	file_extent *lastExtent = NULL;
	if (start != 0) {
		lastExtent = ExtentAt(start - 1);
		offset = lastExtent->offset + lastExtent->disk.length;
	}

	for (uint32 i = 0; i < vecCount; i++) {
		if (lastExtent != NULL) {
			if (lastExtent->disk.offset + lastExtent->disk.length
					== vecs[i].offset) {
				lastExtent->disk.length += vecs[i].length;
				offset += vecs[i].length;
				start--;
				_MakeSpace(count - 1);
				continue;
			}
		}

		file_extent *extent = ExtentAt(start + i);
		extent->offset = offset;
		extent->disk = vecs[i];

		offset += extent->disk.length;
		lastExtent = extent;
	}

#ifdef TRACE_FILE_MAP
	for (uint32 i = 0; i < count; i++) {
		file_extent *extent = ExtentAt(i);
		dprintf("[%ld] extent offset %Ld, disk offset %Ld, length %Ld\n",
			i, extent->offset, extent->disk.offset, extent->disk.length);
	}
#endif

	lastOffset = offset;
	return B_OK;
}


/*!	Invalidates or removes the specified part of the file map.
*/
void
file_map::Invalidate(off_t offset, off_t size)
{
	// TODO: honour size, we currently always remove everything after "offset"
	if (offset == 0) {
		Free();
		return;
	}

	uint32 index;
	file_extent *extent = FindExtent(offset, &index);
	if (extent != NULL) {
		_MakeSpace(index);

		if (extent->offset + extent->disk.length > offset)
			extent->disk.length = offset - extent->offset;
	}
}


void
file_map::Free()
{
	if (count > CACHED_FILE_EXTENTS)
		free(indirect.array);

	count = 0;
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

	file_map *map = (file_map *)_map;

	if (size < map->size)
		map->Invalidate(size, map->size - size);

	map->size = size;
}


extern "C" void
file_map_invalidate(void *_map, off_t offset, off_t size)
{
	if (_map == NULL)
		return;

	file_map *map = (file_map *)_map;
	map->Invalidate(offset, size);
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

	if (offset >= map.size) {
		*_count = 0;
		return B_OK;
	}
	if (offset + size > map.size)
		size = map.size - offset;

	// First, we need to make sure that we have already cached all file
	// extents needed for this request.

	file_extent *lastExtent = NULL;
	if (map.count > 0)
		lastExtent = map.ExtentAt(map.count - 1);

	off_t mapOffset = 0;
	if (lastExtent != NULL)
		mapOffset = lastExtent->offset + lastExtent->disk.length;

	off_t end = offset + size;

	while (mapOffset < end) {
		// We don't have the requested extents yet, retrieve them
		size_t vecCount = maxVecs;
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
	}

	if (status != B_OK)
		return status;

	// We now have cached the map of this file as far as we need it, now
	// we need to translate it for the requested access.

	uint32 index;
	file_extent *fileExtent = map.FindExtent(offset, &index);

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

