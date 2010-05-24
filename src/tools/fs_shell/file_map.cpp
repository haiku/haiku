/*
 * Copyright 2004-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "fssh_fs_cache.h"

#include <new>
#include <stdlib.h>
#include <string.h>

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

struct file_extent_array {
	file_extent*	array;
	fssh_size_t		max_count;
};

class FileMap {
public:
							FileMap(void* vnode, fssh_off_t size);
							~FileMap();

			void			Invalidate(fssh_off_t offset, fssh_off_t size);
			void			SetSize(fssh_off_t size);

			fssh_status_t	Translate(fssh_off_t offset, fssh_size_t size,
								fssh_file_io_vec* vecs, fssh_size_t* _count,
								fssh_size_t align);

			file_extent*	ExtentAt(uint32_t index);

			fssh_size_t		Count() const { return fCount; }
			void*			Vnode() const { return fVnode; }
			fssh_off_t		Size() const { return fSize; }

			fssh_status_t	SetMode(uint32_t mode);

private:
			file_extent*	_FindExtent(fssh_off_t offset, uint32_t* _index);
			fssh_status_t	_MakeSpace(fssh_size_t count);
			fssh_status_t	_Add(fssh_file_io_vec* vecs, fssh_size_t vecCount,
								fssh_off_t& lastOffset);
			fssh_status_t	_Cache(fssh_off_t offset, fssh_off_t size);
			void			_InvalidateAfter(fssh_off_t offset);
			void			_Free();

	union {
		file_extent	fDirect[CACHED_FILE_EXTENTS];
		file_extent_array fIndirect;
	};
	fssh_mutex		fLock;
	fssh_size_t		fCount;
	void*			fVnode;
	fssh_off_t		fSize;
	bool			fCacheAll;
};


FileMap::FileMap(void* vnode, fssh_off_t size)
	:
	fCount(0),
	fVnode(vnode),
	fSize(size),
	fCacheAll(false)
{
	fssh_mutex_init(&fLock, "file map");
}


FileMap::~FileMap()
{
	_Free();
	fssh_mutex_destroy(&fLock);
}


file_extent*
FileMap::ExtentAt(uint32_t index)
{
	if (index >= fCount)
		return NULL;

	if (fCount > CACHED_FILE_EXTENTS)
		return &fIndirect.array[index];

	return &fDirect[index];
}


file_extent*
FileMap::_FindExtent(fssh_off_t offset, uint32_t *_index)
{
	int32_t left = 0;
	int32_t right = fCount - 1;

	while (left <= right) {
		int32_t index = (left + right) / 2;
		file_extent* extent = ExtentAt(index);

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


fssh_status_t
FileMap::_MakeSpace(fssh_size_t count)
{
	if (count <= CACHED_FILE_EXTENTS) {
		// just use the reserved area in the file_cache_ref structure
		if (fCount > CACHED_FILE_EXTENTS) {
			// the new size is smaller than the minimal array size
			file_extent *array = fIndirect.array;
			memcpy(fDirect, array, sizeof(file_extent) * count);
			free(array);
		}
	} else {
		// resize array if needed
		file_extent* oldArray = NULL;
		fssh_size_t maxCount = CACHED_FILE_EXTENTS;
		if (fCount > CACHED_FILE_EXTENTS) {
			oldArray = fIndirect.array;
			maxCount = fIndirect.max_count;
		}

		if (count > maxCount) {
			// allocate new array
			while (maxCount < count) {
				if (maxCount < 32768)
					maxCount <<= 1;
				else
					maxCount += 32768;
			}

			file_extent* newArray = (file_extent *)realloc(oldArray,
				maxCount * sizeof(file_extent));
			if (newArray == NULL)
				return FSSH_B_NO_MEMORY;

			if (fCount > 0 && fCount <= CACHED_FILE_EXTENTS)
				memcpy(newArray, fDirect, sizeof(file_extent) * fCount);

			fIndirect.array = newArray;
			fIndirect.max_count = maxCount;
		}
	}

	fCount = count;
	return FSSH_B_OK;
}


fssh_status_t
FileMap::_Add(fssh_file_io_vec* vecs, fssh_size_t vecCount,
	fssh_off_t& lastOffset)
{
	TRACE(("FileMap@%p::Add(vecCount = %ld)\n", this, vecCount));

	uint32_t start = fCount;
	fssh_off_t offset = 0;

	fssh_status_t status = _MakeSpace(fCount + vecCount);
	if (status != FSSH_B_OK)
		return status;

	file_extent* lastExtent = NULL;
	if (start != 0) {
		lastExtent = ExtentAt(start - 1);
		offset = lastExtent->offset + lastExtent->disk.length;
	}

	for (uint32_t i = 0; i < vecCount; i++) {
		if (lastExtent != NULL) {
			if (lastExtent->disk.offset + lastExtent->disk.length
					== vecs[i].offset) {
				lastExtent->disk.length += vecs[i].length;
				offset += vecs[i].length;
				start--;
				_MakeSpace(fCount - 1);
				continue;
			}
		}

		file_extent* extent = ExtentAt(start + i);
		extent->offset = offset;
		extent->disk = vecs[i];

		offset += extent->disk.length;
		lastExtent = extent;
	}

#ifdef TRACE_FILE_MAP
	for (uint32 i = 0; i < fCount; i++) {
		file_extent* extent = ExtentAt(i);
		dprintf("[%ld] extent offset %Ld, disk offset %Ld, length %Ld\n",
			i, extent->offset, extent->disk.offset, extent->disk.length);
	}
#endif

	lastOffset = offset;
	return FSSH_B_OK;
}


void
FileMap::_InvalidateAfter(fssh_off_t offset)
{
	uint32_t index;
	file_extent* extent = _FindExtent(offset, &index);
	if (extent != NULL) {
		_MakeSpace(index + 1);

		if (extent->offset + extent->disk.length > offset) {
			extent->disk.length = offset - extent->offset;
			if (extent->disk.length == 0)
				_MakeSpace(index);
		}
	}
}


/*!	Invalidates or removes the specified part of the file map.
*/
void
FileMap::Invalidate(fssh_off_t offset, fssh_off_t size)
{
	MutexLocker _(fLock);

	// TODO: honour size, we currently always remove everything after "offset"
	if (offset == 0) {
		_Free();
		return;
	}

	_InvalidateAfter(offset);
}


void
FileMap::SetSize(fssh_off_t size)
{
	MutexLocker _(fLock);

	if (size < fSize)
		_InvalidateAfter(size);

	fSize = size;
}


void
FileMap::_Free()
{
	if (fCount > CACHED_FILE_EXTENTS)
		free(fIndirect.array);

	fCount = 0;
}


fssh_status_t
FileMap::_Cache(fssh_off_t offset, fssh_off_t size)
{
	file_extent* lastExtent = NULL;
	if (fCount > 0)
		lastExtent = ExtentAt(fCount - 1);

	fssh_off_t mapEnd = 0;
	if (lastExtent != NULL)
		mapEnd = lastExtent->offset + lastExtent->disk.length;

	fssh_off_t end = offset + size;

	if (fCacheAll && mapEnd < end)
		return FSSH_B_ERROR;

	fssh_status_t status = FSSH_B_OK;
	fssh_file_io_vec vecs[8];
	const fssh_size_t kMaxVecs = 8;

	while (status == FSSH_B_OK && mapEnd < end) {
		// We don't have the requested extents yet, retrieve them
		fssh_size_t vecCount = kMaxVecs;
		status = vfs_get_file_map(Vnode(), mapEnd, FSSH_SIZE_MAX, vecs,
			&vecCount);
		if (status == FSSH_B_OK || status == FSSH_B_BUFFER_OVERFLOW)
			status = _Add(vecs, vecCount, mapEnd);
	}

	return status;
}


fssh_status_t
FileMap::SetMode(uint32_t mode)
{
	if (mode != FSSH_FILE_MAP_CACHE_ALL
		&& mode != FSSH_FILE_MAP_CACHE_ON_DEMAND)
		return FSSH_B_BAD_VALUE;

	MutexLocker _(fLock);

	if ((mode == FSSH_FILE_MAP_CACHE_ALL && fCacheAll)
		|| (mode == FSSH_FILE_MAP_CACHE_ON_DEMAND && !fCacheAll))
		return FSSH_B_OK;

	if (mode == FSSH_FILE_MAP_CACHE_ALL) {
		fssh_status_t status = _Cache(0, fSize);
		if (status != FSSH_B_OK)
			return status;

		fCacheAll = true;
	} else
		fCacheAll = false;

	return FSSH_B_OK;
}


fssh_status_t
FileMap::Translate(fssh_off_t offset, fssh_size_t size, fssh_file_io_vec* vecs,
	fssh_size_t* _count, fssh_size_t align)
{
	MutexLocker _(fLock);

	fssh_size_t maxVecs = *_count;
	fssh_size_t padLastVec = 0;

	if ((uint64_t)offset >= (uint64_t)Size()) {
		*_count = 0;
		return FSSH_B_OK;
	}
	if ((uint64_t)offset + size > (uint64_t)fSize) {
		if (align > 1) {
			fssh_off_t alignedSize = (fSize + align - 1) & ~(fssh_off_t)(align - 1);
			if ((uint64_t)offset + size >= (uint64_t)alignedSize)
				padLastVec = alignedSize - fSize;
		}
		size = fSize - offset;
	}

	// First, we need to make sure that we have already cached all file
	// extents needed for this request.

	fssh_status_t status = _Cache(offset, size);
	if (status != FSSH_B_OK)
		return status;

	// We now have cached the map of this file as far as we need it, now
	// we need to translate it for the requested access.

	uint32_t index;
	file_extent* fileExtent = _FindExtent(offset, &index);

	offset -= fileExtent->offset;
	vecs[0].offset = fileExtent->disk.offset + offset;
	vecs[0].length = fileExtent->disk.length - offset;

	if ((uint64_t)vecs[0].length >= (uint64_t)size) {
		vecs[0].length = size + padLastVec;
		*_count = 1;
		return FSSH_B_OK;
	}

	// copy the rest of the vecs

	size -= vecs[0].length;
	uint32_t vecIndex = 1;

	while (true) {
		fileExtent++;

		vecs[vecIndex++] = fileExtent->disk;

		if ((uint64_t)size <= (uint64_t)fileExtent->disk.length) {
			vecs[vecIndex - 1].length = size + padLastVec;
			break;
		}

		if (vecIndex >= maxVecs) {
			*_count = vecIndex;
			return FSSH_B_BUFFER_OVERFLOW;
		}

		size -= fileExtent->disk.length;
	}

	*_count = vecIndex;
	return FSSH_B_OK;
}


}	// namespace FSShell


//	#pragma mark - public FS API


extern "C" void*
fssh_file_map_create(fssh_mount_id mountID, fssh_vnode_id vnodeID,
	fssh_off_t size)
{
	TRACE(("file_map_create(mountID = %ld, vnodeID = %Ld, size = %Ld)\n",
		mountID, vnodeID, size));

	// Get the vnode for the object
	// (note, this does not grab a reference to the node)
	void* vnode;
	if (vfs_lookup_vnode(mountID, vnodeID, &vnode) != FSSH_B_OK)
		return NULL;

	return new(std::nothrow) FileMap(vnode, size);
}


extern "C" void
fssh_file_map_delete(void* _map)
{
	FileMap* map = (FileMap*)_map;
	if (map == NULL)
		return;

	TRACE(("file_map_delete(map = %p)\n", map));
	delete map;
}


extern "C" void
fssh_file_map_set_size(void* _map, fssh_off_t size)
{
	FileMap* map = (FileMap*)_map;
	if (map == NULL)
		return;

	map->SetSize(size);
}


extern "C" void
fssh_file_map_invalidate(void* _map, fssh_off_t offset, fssh_off_t size)
{
	FileMap* map = (FileMap*)_map;
	if (map == NULL)
		return;

	map->Invalidate(offset, size);
}


extern "C" fssh_status_t
fssh_file_map_set_mode(void* _map, uint32_t mode)
{
	FileMap* map = (FileMap*)_map;
	if (map == NULL)
		return FSSH_B_BAD_VALUE;

	return map->SetMode(mode);
}


extern "C" fssh_status_t
fssh_file_map_translate(void* _map, fssh_off_t offset, fssh_size_t size,
	fssh_file_io_vec* vecs, fssh_size_t* _count, fssh_size_t align)
{
	TRACE(("file_map_translate(map %p, offset %Ld, size %ld)\n",
		_map, offset, size));

	FileMap* map = (FileMap*)_map;
	if (map == NULL)
		return FSSH_B_BAD_VALUE;

	return map->Translate(offset, size, vecs, _count, align);
}

