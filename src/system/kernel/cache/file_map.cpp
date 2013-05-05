/*
 * Copyright 2004-2012, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <new>
#include <stdlib.h>
#include <string.h>

#ifdef FS_SHELL
#	include "vfs.h"
#	include "fssh_api_wrapper.h"

using namespace FSShell;
#else
#	include <unistd.h>

#	include <KernelExport.h>
#	include <fs_cache.h>

#	include <condition_variable.h>
#	include <file_cache.h>
#	include <generic_syscall.h>
#	include <util/AutoLock.h>
#	include <util/DoublyLinkedList.h>
#	include <vfs.h>
#	include <vm/vm.h>
#	include <vm/vm_page.h>
#	include <vm/VMCache.h>

#	include "kernel_debug_config.h"
#endif


//#define TRACE_FILE_MAP
#ifdef TRACE_FILE_MAP
#	define TRACE(x...) dprintf_no_syslog(x)
#else
#	define TRACE(x...) ;
#endif

// TODO: use a sparse array - eventually, the unused BlockMap would be something
//	to reuse for this. We could also have an upperbound of memory consumption
//	for the whole map.
// TODO: it would be nice if we could free a file map in low memory situations.


#define CACHED_FILE_EXTENTS	2
	// must be smaller than MAX_FILE_IO_VECS
	// TODO: find out how much of these are typically used

struct file_extent {
	off_t			offset;
	file_io_vec		disk;
};

struct file_extent_array {
	file_extent*	array;
	size_t			max_count;
};

class FileMap
#if DEBUG_FILE_MAP
	: public DoublyLinkedListLinkImpl<FileMap>
#endif
{
public:
							FileMap(struct vnode* vnode, off_t size);
							~FileMap();

			void			Invalidate(off_t offset, off_t size);
			void			SetSize(off_t size);

			status_t		Translate(off_t offset, size_t size,
								file_io_vec* vecs, size_t* _count,
								size_t align);

			file_extent*	ExtentAt(uint32 index);

			size_t			Count() const { return fCount; }
			struct vnode*	Vnode() const { return fVnode; }
			off_t			Size() const { return fSize; }

			status_t		SetMode(uint32 mode);

private:
			file_extent*	_FindExtent(off_t offset, uint32* _index);
			status_t		_MakeSpace(size_t count);
			status_t		_Add(file_io_vec* vecs, size_t vecCount,
								off_t& lastOffset);
			status_t		_Cache(off_t offset, off_t size);
			void			_InvalidateAfter(off_t offset);
			void			_Free();

	union {
		file_extent	fDirect[CACHED_FILE_EXTENTS];
		file_extent_array fIndirect;
	};
	mutex			fLock;
	size_t			fCount;
	struct vnode*	fVnode;
	off_t			fSize;
	bool			fCacheAll;
};

#if DEBUG_FILE_MAP
typedef DoublyLinkedList<FileMap> FileMapList;

static FileMapList sList;
static mutex sLock;
#endif


FileMap::FileMap(struct vnode* vnode, off_t size)
	:
	fCount(0),
	fVnode(vnode),
	fSize(size),
	fCacheAll(false)
{
	mutex_init(&fLock, "file map");

#if DEBUG_FILE_MAP
	MutexLocker _(sLock);
	sList.Add(this);
#endif
}


FileMap::~FileMap()
{
	_Free();
	mutex_destroy(&fLock);

#if DEBUG_FILE_MAP
	MutexLocker _(sLock);
	sList.Remove(this);
#endif
}


file_extent*
FileMap::ExtentAt(uint32 index)
{
	if (index >= fCount)
		return NULL;

	if (fCount > CACHED_FILE_EXTENTS)
		return &fIndirect.array[index];

	return &fDirect[index];
}


file_extent*
FileMap::_FindExtent(off_t offset, uint32 *_index)
{
	int32 left = 0;
	int32 right = fCount - 1;

	while (left <= right) {
		int32 index = (left + right) / 2;
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


status_t
FileMap::_MakeSpace(size_t count)
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
		size_t maxCount = CACHED_FILE_EXTENTS;
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
				return B_NO_MEMORY;

			if (fCount > 0 && fCount <= CACHED_FILE_EXTENTS)
				memcpy(newArray, fDirect, sizeof(file_extent) * fCount);

			fIndirect.array = newArray;
			fIndirect.max_count = maxCount;
		}
	}

	fCount = count;
	return B_OK;
}


status_t
FileMap::_Add(file_io_vec* vecs, size_t vecCount, off_t& lastOffset)
{
	TRACE("FileMap@%p::Add(vecCount = %ld)\n", this, vecCount);

	uint32 start = fCount;
	off_t offset = 0;

	status_t status = _MakeSpace(fCount + vecCount);
	if (status != B_OK)
		return status;

	file_extent* lastExtent = NULL;
	if (start != 0) {
		lastExtent = ExtentAt(start - 1);
		offset = lastExtent->offset + lastExtent->disk.length;
	}

	for (uint32 i = 0; i < vecCount; i++) {
		if (lastExtent != NULL) {
			if (lastExtent->disk.offset + lastExtent->disk.length
					== vecs[i].offset
				|| (lastExtent->disk.offset == -1 && vecs[i].offset == -1)) {

				lastExtent->disk.length += vecs[i].length;
				offset += vecs[i].length;

				_MakeSpace(fCount - 1);
				if (fCount == CACHED_FILE_EXTENTS) {
					// We moved the indirect array into the direct one, making
					// lastExtent a stale pointer, re-get it.
					lastExtent = ExtentAt(start - 1);
				}

				continue;
			}
		}

		file_extent* extent = ExtentAt(start++);
		extent->offset = offset;
		extent->disk = vecs[i];

		offset += extent->disk.length;
		lastExtent = extent;
	}

#ifdef TRACE_FILE_MAP
	for (uint32 i = 0; i < fCount; i++) {
		file_extent* extent = ExtentAt(i);
		TRACE("[%ld] extent offset %Ld, disk offset %Ld, length %Ld\n",
			i, extent->offset, extent->disk.offset, extent->disk.length);
	}
#endif

	lastOffset = offset;
	return B_OK;
}


void
FileMap::_InvalidateAfter(off_t offset)
{
	uint32 index;
	file_extent* extent = _FindExtent(offset, &index);
	if (extent != NULL) {
		uint32 resizeTo = index + 1;

		if (extent->offset + extent->disk.length > offset) {
			extent->disk.length = offset - extent->offset;
			if (extent->disk.length == 0)
				resizeTo = index;
		}

		_MakeSpace(resizeTo);
	}
}


/*!	Invalidates or removes the specified part of the file map.
*/
void
FileMap::Invalidate(off_t offset, off_t size)
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
FileMap::SetSize(off_t size)
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


status_t
FileMap::_Cache(off_t offset, off_t size)
{
	file_extent* lastExtent = NULL;
	if (fCount > 0)
		lastExtent = ExtentAt(fCount - 1);

	off_t mapEnd = 0;
	if (lastExtent != NULL)
		mapEnd = lastExtent->offset + lastExtent->disk.length;

	off_t end = offset + size;

	if (fCacheAll && mapEnd < end)
		return B_ERROR;

	status_t status = B_OK;
	file_io_vec vecs[8];
	const size_t kMaxVecs = 8;

	while (status == B_OK && mapEnd < end) {
		// We don't have the requested extents yet, retrieve them
		size_t vecCount = kMaxVecs;
		status = vfs_get_file_map(Vnode(), mapEnd, ~0UL, vecs, &vecCount);
		if (status == B_OK || status == B_BUFFER_OVERFLOW)
			status = _Add(vecs, vecCount, mapEnd);
	}

	return status;
}


status_t
FileMap::SetMode(uint32 mode)
{
	if (mode != FILE_MAP_CACHE_ALL && mode != FILE_MAP_CACHE_ON_DEMAND)
		return B_BAD_VALUE;

	MutexLocker _(fLock);

	if ((mode == FILE_MAP_CACHE_ALL && fCacheAll)
		|| (mode == FILE_MAP_CACHE_ON_DEMAND && !fCacheAll))
		return B_OK;

	if (mode == FILE_MAP_CACHE_ALL) {
		status_t status = _Cache(0, fSize);
		if (status != B_OK)
			return status;

		fCacheAll = true;
	} else
		fCacheAll = false;

	return B_OK;
}


status_t
FileMap::Translate(off_t offset, size_t size, file_io_vec* vecs, size_t* _count,
	size_t align)
{
	if (offset < 0)
		return B_BAD_VALUE;

	MutexLocker _(fLock);

	size_t maxVecs = *_count;
	size_t padLastVec = 0;

	if (offset >= Size()) {
		*_count = 0;
		return B_OK;
	}
	if ((off_t)(offset + size) > fSize) {
		if (align > 1) {
			off_t alignedSize = (fSize + align - 1) & ~(off_t)(align - 1);
			if ((off_t)(offset + size) >= alignedSize)
				padLastVec = alignedSize - fSize;
		}
		size = fSize - offset;
	}

	// First, we need to make sure that we have already cached all file
	// extents needed for this request.

	status_t status = _Cache(offset, size);
	if (status != B_OK)
		return status;

	// We now have cached the map of this file as far as we need it, now
	// we need to translate it for the requested access.

	uint32 index;
	file_extent* fileExtent = _FindExtent(offset, &index);

	offset -= fileExtent->offset;
	if (fileExtent->disk.offset != -1)
		vecs[0].offset = fileExtent->disk.offset + offset;
	else
		vecs[0].offset = -1;
	vecs[0].length = fileExtent->disk.length - offset;

	if (vecs[0].length >= (off_t)size) {
		vecs[0].length = size + padLastVec;
		*_count = 1;
		return B_OK;
	}

	// copy the rest of the vecs

	size -= vecs[0].length;
	uint32 vecIndex = 1;

	while (true) {
		fileExtent++;

		vecs[vecIndex++] = fileExtent->disk;

		if ((off_t)size <= fileExtent->disk.length) {
			vecs[vecIndex - 1].length = size + padLastVec;
			break;
		}

		if (vecIndex >= maxVecs) {
			*_count = vecIndex;
			return B_BUFFER_OVERFLOW;
		}

		size -= fileExtent->disk.length;
	}

	*_count = vecIndex;
	return B_OK;
}


//	#pragma mark -


#if DEBUG_FILE_MAP

static int
dump_file_map(int argc, char** argv)
{
	if (argc < 2) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	bool printExtents = false;
	if (argc > 2 && !strcmp(argv[1], "-p"))
		printExtents = true;

	FileMap* map = (FileMap*)parse_expression(argv[argc - 1]);
	if (map == NULL) {
		kprintf("invalid file map!\n");
		return 0;
	}

	kprintf("FileMap %p\n", map);
	kprintf("  size    %" B_PRIdOFF "\n", map->Size());
	kprintf("  count   %lu\n", map->Count());

	if (!printExtents)
		return 0;

	for (uint32 i = 0; i < map->Count(); i++) {
		file_extent* extent = map->ExtentAt(i);

		kprintf("  [%" B_PRIu32 "] offset %" B_PRIdOFF ", disk offset %"
			B_PRIdOFF ", length %" B_PRIdOFF "\n", i, extent->offset,
			extent->disk.offset, extent->disk.length);
	}

	return 0;
}


static int
dump_file_map_stats(int argc, char** argv)
{
	off_t minSize = 0;
	off_t maxSize = -1;

	if (argc == 2) {
		maxSize = parse_expression(argv[1]);
	} else if (argc > 2) {
		minSize = parse_expression(argv[1]);
		maxSize = parse_expression(argv[2]);
	}

	FileMapList::Iterator iterator = sList.GetIterator();
	off_t size = 0;
	off_t mapSize = 0;
	uint32 extents = 0;
	uint32 count = 0;
	uint32 emptyCount = 0;

	while (iterator.HasNext()) {
		FileMap* map = iterator.Next();

		if (minSize > map->Size() || (maxSize != -1 && maxSize < map->Size()))
			continue;

		if (map->Count() != 0) {
			file_extent* extent = map->ExtentAt(map->Count() - 1);
			if (extent != NULL)
				mapSize += extent->offset + extent->disk.length;

			extents += map->Count();
		} else
			emptyCount++;

		size += map->Size();
		count++;
	}

	kprintf("%" B_PRId32 " file maps (%" B_PRIu32 " empty), %" B_PRIdOFF " file"
		" bytes in total, %" B_PRIdOFF " bytes cached, %" B_PRIu32 " extents\n",
		count, emptyCount, size, mapSize, extents);
	kprintf("average %" B_PRIu32 " extents per map for %" B_PRIdOFF " bytes.\n",
		extents / (count - emptyCount), mapSize / (count - emptyCount));

	return 0;
}

#endif	// DEBUG_FILE_MAP


//	#pragma mark - private kernel API


extern "C" status_t
file_map_init(void)
{
#if DEBUG_FILE_MAP
	add_debugger_command_etc("file_map", &dump_file_map,
		"Dumps the specified file map.",
		"[-p] <file-map>\n"
		"  -p          - causes the file extents to be printed as well.\n"
		"  <file-map>  - pointer to the file map.\n", 0);
	add_debugger_command("file_map_stats", &dump_file_map_stats,
		"Dumps some file map statistics.");

	mutex_init(&sLock, "file map list");
#endif
	return B_OK;
}


//	#pragma mark - public FS API


extern "C" void*
file_map_create(dev_t mountID, ino_t vnodeID, off_t size)
{
	TRACE("file_map_create(mountID = %ld, vnodeID = %Ld, size = %Ld)\n",
		mountID, vnodeID, size);

	// Get the vnode for the object
	// (note, this does not grab a reference to the node)
	struct vnode* vnode;
	if (vfs_lookup_vnode(mountID, vnodeID, &vnode) != B_OK)
		return NULL;

	return new(std::nothrow) FileMap(vnode, size);
}


extern "C" void
file_map_delete(void* _map)
{
	FileMap* map = (FileMap*)_map;
	if (map == NULL)
		return;

	TRACE("file_map_delete(map = %p)\n", map);
	delete map;
}


extern "C" void
file_map_set_size(void* _map, off_t size)
{
	FileMap* map = (FileMap*)_map;
	if (map == NULL)
		return;

	map->SetSize(size);
}


extern "C" void
file_map_invalidate(void* _map, off_t offset, off_t size)
{
	FileMap* map = (FileMap*)_map;
	if (map == NULL)
		return;

	map->Invalidate(offset, size);
}


extern "C" status_t
file_map_set_mode(void* _map, uint32 mode)
{
	FileMap* map = (FileMap*)_map;
	if (map == NULL)
		return B_BAD_VALUE;

	return map->SetMode(mode);
}


extern "C" status_t
file_map_translate(void* _map, off_t offset, size_t size, file_io_vec* vecs,
	size_t* _count, size_t align)
{
	TRACE("file_map_translate(map %p, offset %Ld, size %ld)\n",
		_map, offset, size);

	FileMap* map = (FileMap*)_map;
	if (map == NULL)
		return B_BAD_VALUE;

	return map->Translate(offset, size, vecs, _count, align);
}

