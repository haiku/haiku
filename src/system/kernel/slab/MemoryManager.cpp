/*
 * Copyright 2010, Ingo Weinhold <ingo_weinhold@gmx.de>.
 * Distributed under the terms of the MIT License.
 */


#include "MemoryManager.h"

#include <algorithm>

#include <debug.h>
#include <kernel.h>
#include <util/AutoLock.h>
#include <vm/vm.h>
#include <vm/vm_page.h>
#include <vm/vm_priv.h>
#include <vm/VMAddressSpace.h>
#include <vm/VMArea.h>
#include <vm/VMCache.h>
#include <vm/VMTranslationMap.h>

#include "ObjectCache.h"
#include "slab_private.h"


//#define TRACE_MEMORY_MANAGER
#ifdef TRACE_MEMORY_MANAGER
#	define TRACE(x...)	dprintf(x)
#else
#	define TRACE(x...)	do {} while (false)
#endif


static const char* const kSlabAreaName = "slab area";

static void* sAreaTableBuffer[1024];

mutex MemoryManager::sLock;
rw_lock MemoryManager::sAreaTableLock;
kernel_args* MemoryManager::sKernelArgs;
MemoryManager::AreaPool MemoryManager::sSmallChunkAreas;
MemoryManager::AreaPool MemoryManager::sMiddleChunkAreas;
MemoryManager::AreaPool MemoryManager::sLargeChunkAreas;
MemoryManager::AreaTable MemoryManager::sAreaTable;
MemoryManager::Area* MemoryManager::sFreeAreas;
MemoryManager::AllocationEntry* MemoryManager::sAllocationEntryCanWait;
MemoryManager::AllocationEntry* MemoryManager::sAllocationEntryDontWait;


/*static*/ void
MemoryManager::Init(kernel_args* args)
{
	mutex_init(&sLock, "slab memory manager");
	rw_lock_init(&sAreaTableLock, "slab memory manager area table");
	sKernelArgs = args;

	new(&sSmallChunkAreas) AreaPool;
	new(&sMiddleChunkAreas) AreaPool;
	new(&sLargeChunkAreas) AreaPool;

	sSmallChunkAreas.chunkSize = SLAB_CHUNK_SIZE_SMALL;
	sMiddleChunkAreas.chunkSize = SLAB_CHUNK_SIZE_MIDDLE;
	sLargeChunkAreas.chunkSize = SLAB_CHUNK_SIZE_LARGE;

	new(&sAreaTable) AreaTable;
	sAreaTable.Resize(sAreaTableBuffer, sizeof(sAreaTableBuffer), true);
		// A bit hacky: The table now owns the memory. Since we never resize or
		// free it, that's not a problem, though.

	sFreeAreas = NULL;
}


/*static*/ void
MemoryManager::InitPostArea()
{
	sKernelArgs = NULL;

	// Convert all areas to actual areas. This loop might look a bit weird, but
	// is necessary since creating the actual area involves memory allocations,
	// which in turn can change the situation.
	while (true) {
		if (!_ConvertEarlyAreas(sSmallChunkAreas.partialAreas)
			&& !_ConvertEarlyAreas(sSmallChunkAreas.fullAreas)
			&& !_ConvertEarlyAreas(sMiddleChunkAreas.partialAreas)
			&& !_ConvertEarlyAreas(sMiddleChunkAreas.fullAreas)
			&& !_ConvertEarlyAreas(sLargeChunkAreas.partialAreas)
			&& !_ConvertEarlyAreas(sLargeChunkAreas.fullAreas)) {
			break;
		}
	}

	// just "leak" the free areas -- the VM will automatically free all
	// unclaimed memory
	sFreeAreas = NULL;

	add_debugger_command_etc("slab_area", &_DumpArea,
		"Dump information on a given slab area",
		"<area>\n"
		"Dump information on a given slab area specified by its base "
			"address.\n", 0);
	add_debugger_command_etc("slab_areas", &_DumpAreas,
		"List all slab areas",
		"\n"
		"Lists all slab areas.\n", 0);
}


/*static*/ status_t
MemoryManager::Allocate(ObjectCache* cache, uint32 flags, void*& _pages)
{
	// TODO: Support CACHE_UNLOCKED_PAGES!

	size_t chunkSize = cache->slab_size;

	TRACE("MemoryManager::Allocate(%p, %#" B_PRIx32 "): chunkSize: %"
		B_PRIuSIZE "\n", cache, flags, chunkSize);

	// get the right area pool
	AreaPool* areaPool = _AreaPoolFor(chunkSize);
	if (areaPool == NULL) {
		panic("Invalid slab size: %" B_PRIuSIZE, chunkSize);
		return B_BAD_VALUE;
	}

	MutexLocker locker(sLock);

	// get an area
	Area* area;
	status_t error = _GetPartialArea(areaPool, flags, area);
	if (error != B_OK)
		return error;

	// allocate a chunk
	bool chunkMapped = false;
	Chunk* chunk;
	if (area->mappedFreeChunks != NULL) {
		chunk = _pop(area->mappedFreeChunks);
		chunkMapped = true;
	} else
		chunk = _pop(area->unmappedFreeChunks);


	if (++area->usedChunkCount == area->chunkCount) {
		areaPool->partialAreas.Remove(area);
		areaPool->fullAreas.Add(area);
	}

	// If the chunk is not mapped yet, do it now.
	addr_t chunkAddress = _ChunkAddress(area, chunk);

	if (!chunkMapped) {
		locker.Unlock();
		error = _MapChunk(area->vmArea, chunkAddress, chunkSize, 0, flags);
		locker.Lock();
		if (error != B_OK) {
			// something failed -- free the chunk
			_FreeChunk(areaPool, area, chunk, chunkAddress, true, flags);
			return error;
		}
	}


	chunk->cache = cache;
	_pages = (void*)chunkAddress;

	TRACE("MemoryManager::Allocate() done: %p (chunk %p)\n", _pages, chunk);
	return B_OK;
}


/*static*/ void
MemoryManager::Free(void* pages, uint32 flags)
{
	TRACE("MemoryManager::Free(%p, %#" B_PRIx32 ")\n", pages, flags);

	// get the area and the pool
	Area* area = (Area*)ROUNDDOWN((addr_t)pages, SLAB_AREA_SIZE);

	AreaPool* areaPool = _AreaPoolFor(area->chunkSize);
	if (areaPool == NULL) {
		panic("Invalid area: %p", area);
		return;
	}

	// get the chunk
	uint16 chunkIndex = _ChunkIndexForAddress(area, (addr_t)pages);
	ASSERT(chunkIndex < area->chunkCount);
	ASSERT((addr_t)pages % area->chunkSize == 0);
	Chunk* chunk = &area->chunks[chunkIndex];

	// and free it
	MutexLocker locker(sLock);
	_FreeChunk(areaPool, area, chunk, (addr_t)pages, false, flags);
}


/*static*/ size_t
MemoryManager::AcceptableChunkSize(size_t size)
{
	if (size <= SLAB_CHUNK_SIZE_SMALL)
		return SLAB_CHUNK_SIZE_SMALL;
	if (size <= SLAB_CHUNK_SIZE_MIDDLE)
		return SLAB_CHUNK_SIZE_MIDDLE;
	return SLAB_CHUNK_SIZE_LARGE;
}


/*static*/ ObjectCache*
MemoryManager::CacheForAddress(void* address)
{
	// get the area
	addr_t areaBase = ROUNDDOWN((addr_t)address, SLAB_AREA_SIZE);

	ReadLocker readLocker(sAreaTableLock);
	Area* area = sAreaTable.Lookup(areaBase);
	readLocker.Unlock();

	if (area == NULL)
		return NULL;

	// get the chunk
	uint16 chunkIndex = _ChunkIndexForAddress(area, (addr_t)address);
	ASSERT(chunkIndex < area->chunkCount);

	return area->chunks[chunkIndex].cache;
}


/*static*/ MemoryManager::AreaPool*
MemoryManager::_AreaPoolFor(size_t chunkSize)
{
	if (chunkSize == SLAB_CHUNK_SIZE_SMALL)
		return &sSmallChunkAreas;

	if (chunkSize == SLAB_CHUNK_SIZE_MIDDLE)
		return &sMiddleChunkAreas;

	if (chunkSize == SLAB_CHUNK_SIZE_LARGE)
		return &sLargeChunkAreas;

	return NULL;
}


/*static*/ status_t
MemoryManager::_GetPartialArea(AreaPool* areaPool, uint32 flags, Area*& _area)
{
	while (true) {
		Area* area = areaPool->partialAreas.Head();
		if (area != NULL) {
			_area = area;
			return B_OK;
		}

		// We need to allocate a new area. Wait, if someone else is trying the
		// same.
		AllocationEntry* allocationEntry = NULL;
		if (sAllocationEntryDontWait != NULL) {
			allocationEntry = sAllocationEntryDontWait;
		} else if (sAllocationEntryCanWait != NULL
				&& (flags & CACHE_DONT_SLEEP) == 0) {
			allocationEntry = sAllocationEntryCanWait;
		} else
			break;

		ConditionVariableEntry entry;
		allocationEntry->condition.Add(&entry);

		mutex_unlock(&sLock);
		entry.Wait();
		mutex_lock(&sLock);
	}

	// prepare the allocation entry others can wait on
	AllocationEntry*& allocationEntry = (flags & CACHE_DONT_SLEEP) != 0
		? sAllocationEntryDontWait : sAllocationEntryCanWait;

	AllocationEntry myResizeEntry;
	allocationEntry = &myResizeEntry;
	allocationEntry->condition.Init(areaPool, "wait for slab area");
	allocationEntry->thread = find_thread(NULL);

	Area* area;
	status_t error = _AllocateArea(areaPool->chunkSize, flags, area);

	allocationEntry->condition.NotifyAll();
	allocationEntry = NULL;

	if (error != B_OK)
		return error;

	areaPool->partialAreas.Add(area);
		// TODO: If something was freed in the meantime, we might rather
		// want to delete the area again. Alternatively we could keep
		// one global free area.

	_area = area;
	return B_OK;
}


/*static*/ status_t
MemoryManager::_AllocateArea(size_t chunkSize, uint32 flags, Area*& _area)
{
	// TODO: Support reusing free areas!

	TRACE("MemoryManager::_AllocateArea(%" B_PRIuSIZE ", %#" B_PRIx32 ")\n",
		chunkSize, flags);

	if ((flags & CACHE_DONT_SLEEP) != 0)
		return B_WOULD_BLOCK;
		// TODO: Support CACHE_DONT_SLEEP for real! We already consider it in
		// most cases below, but not for vm_create_null_area().

	mutex_unlock(&sLock);

	uint32 chunkCount = (SLAB_AREA_SIZE - sizeof(Area))
		/ (chunkSize + sizeof(Chunk));
	size_t adminMemory = sizeof(Area) + chunkCount * sizeof(Chunk);
	adminMemory = ROUNDUP(adminMemory, B_PAGE_SIZE);

	size_t pagesNeededToMap = 0;
	Area* area;
	VMArea* vmArea = NULL;

	if (sKernelArgs == NULL) {
		// create an area
		area_id areaID = vm_create_null_area(B_SYSTEM_TEAM, kSlabAreaName,
			(void**)&area, B_ANY_KERNEL_BLOCK_ADDRESS, SLAB_AREA_SIZE);
		if (areaID < 0) {
			mutex_lock(&sLock);
			return areaID;
		}

		// map the memory for the administrative structure
		VMAddressSpace* addressSpace = VMAddressSpace::Kernel();
		VMTranslationMap* translationMap = addressSpace->TranslationMap();

		pagesNeededToMap = translationMap->MaxPagesNeededToMap((addr_t)area,
			(addr_t)area + SLAB_AREA_SIZE - 1);

		vmArea = VMAreaHash::Lookup(areaID);
		status_t error = _MapChunk(vmArea, (addr_t)area, adminMemory,
			pagesNeededToMap, flags);
		if (error != B_OK) {
			delete_area(areaID);
			mutex_lock(&sLock);
			return error;
		}

		TRACE("MemoryManager::_AllocateArea(): allocated area %p (%" B_PRId32
			")\n", area, areaID);
	} else {
		// no areas yet -- allocate raw memory
		area = (Area*)vm_allocate_early(sKernelArgs, SLAB_AREA_SIZE,
			SLAB_AREA_SIZE, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, true);
		if (area == NULL) {
			mutex_lock(&sLock);
			return B_NO_MEMORY;
		}

		TRACE("MemoryManager::_AllocateArea(): allocated early area %p\n",
			area);
	}

	// init the area structure
	area->vmArea = vmArea;
	area->chunkSize = chunkSize;
	area->reserved_memory_for_mapping = pagesNeededToMap * B_PAGE_SIZE;
	area->firstUsableChunk = (addr_t)area + ROUNDUP(adminMemory, chunkSize);
	area->chunkCount = chunkCount;
	area->usedChunkCount = 0;
	area->mappedFreeChunks = NULL;
	area->unmappedFreeChunks = NULL;

	for (uint32 i = 0; i < chunkCount; i++)
		_push(area->unmappedFreeChunks, area->chunks + i);

	// in the early boot process everything is mapped already
	if (sKernelArgs != NULL)
		std::swap(area->mappedFreeChunks, area->unmappedFreeChunks);

	// add the area to the hash table
	WriteLocker writeLocker(sAreaTableLock);
	sAreaTable.InsertUnchecked(area);
	writeLocker.Unlock();

	mutex_lock(&sLock);
	_area = area;
	return B_OK;
}


/*static*/ void
MemoryManager::_FreeArea(Area* area, uint32 flags)
{
	TRACE("MemoryManager::_FreeArea(%p, %#" B_PRIx32 ")\n", area, flags);

	ASSERT(area->usedChunkCount == 0);

	// remove the area from the hash table
	WriteLocker writeLocker(sAreaTableLock);
	sAreaTable.RemoveUnchecked(area);
	writeLocker.Unlock();

	if (area->vmArea == NULL) {
		_push(sFreeAreas, area);
		return;
	}

	// TODO: Do we need to handle CACHE_DONT_SLEEP here? Is delete_area()
	// problematic?

	mutex_unlock(&sLock);

	size_t reservedMemory = area->reserved_memory_for_mapping;

	// count mapped free chunks
	for (Chunk* chunk = area->mappedFreeChunks; chunk != NULL;
			chunk = chunk->next) {
		reservedMemory += area->chunkSize;
	}

	delete_area(area->vmArea->id);
	vm_unreserve_memory(reservedMemory);

	mutex_lock(&sLock);
}


/*static*/ void
MemoryManager::_FreeChunk(AreaPool* areaPool, Area* area, Chunk* chunk,
	addr_t chunkAddress, bool alreadyUnmapped, uint32 flags)
{
	// unmap the chunk
	if (!alreadyUnmapped) {
		mutex_unlock(&sLock);
		alreadyUnmapped = _UnmapChunk(area->vmArea, chunkAddress,
			area->chunkSize, flags) == B_OK;
		mutex_lock(&sLock);
	}

	if (alreadyUnmapped)
		_push(area->unmappedFreeChunks, chunk);
	else
		_push(area->mappedFreeChunks, chunk);

	// free the area, if it is unused now
	ASSERT(area->usedChunkCount > 0);
	if (--area->usedChunkCount == 0) {
		areaPool->partialAreas.Remove(area);
		_FreeArea(area, flags);
	}
}


/*static*/ status_t
MemoryManager::_MapChunk(VMArea* vmArea, addr_t address, size_t size,
	size_t reserveAdditionalMemory, uint32 flags)
{
	TRACE("MemoryManager::_MapChunk(%p, %#" B_PRIxADDR ", %#" B_PRIxSIZE
		")\n", vmArea, address, size);

	VMAddressSpace* addressSpace = VMAddressSpace::Kernel();
	VMTranslationMap* translationMap = addressSpace->TranslationMap();

	// reserve memory for the chunk
	size_t reservedMemory = size + reserveAdditionalMemory;
	status_t error = vm_try_reserve_memory(size,
		(flags & CACHE_DONT_SLEEP) != 0 ? 0 : 1000000);
	if (error != B_OK)
		return error;

	// reserve the pages we need now
	size_t reservedPages = size / B_PAGE_SIZE
		+ translationMap->MaxPagesNeededToMap(address, address + size - 1);
	if ((flags & CACHE_DONT_SLEEP) != 0) {
		if (!vm_page_try_reserve_pages(reservedPages)) {
			vm_unreserve_memory(reservedMemory);
			return B_WOULD_BLOCK;
		}
	} else
		vm_page_reserve_pages(reservedPages);

	VMCache* cache = vm_area_get_locked_cache(vmArea);

	// map the pages
	translationMap->Lock();

	addr_t areaOffset = address - vmArea->Base();
	addr_t endAreaOffset = areaOffset + size;
	for (size_t offset = areaOffset; offset < endAreaOffset;
			offset += B_PAGE_SIZE) {
		vm_page* page = vm_page_allocate_page(PAGE_STATE_FREE);
		cache->InsertPage(page, offset);
		vm_page_set_state(page, PAGE_STATE_WIRED);

		page->wired_count++;
		atomic_add(&gMappedPagesCount, 1);
		DEBUG_PAGE_ACCESS_END(page);

		translationMap->Map(vmArea->Base() + offset,
			page->physical_page_number * B_PAGE_SIZE,
			B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	}

	translationMap->Unlock();

	cache->ReleaseRefAndUnlock();

	vm_page_unreserve_pages(reservedPages);

	return B_OK;
}


/*static*/ status_t
MemoryManager::_UnmapChunk(VMArea* vmArea, addr_t address, size_t size,
	uint32 flags)
{
	if (vmArea == NULL)
		return B_ERROR;

	TRACE("MemoryManager::_UnmapChunk(%p, %#" B_PRIxADDR ", %#" B_PRIxSIZE
		")\n", vmArea, address, size);

	VMAddressSpace* addressSpace = VMAddressSpace::Kernel();
	VMTranslationMap* translationMap = addressSpace->TranslationMap();
	VMCache* cache = vm_area_get_locked_cache(vmArea);

	// unmap the pages
	translationMap->Lock();
	translationMap->Unmap(address, address + size - 1);
	atomic_add(&gMappedPagesCount, -(size / B_PAGE_SIZE));
	translationMap->Unlock();

	// free the pages
	addr_t areaPageOffset = (address - vmArea->Base()) / B_PAGE_SIZE;
	addr_t areaPageEndOffset = areaPageOffset + size / B_PAGE_SIZE;
	VMCachePagesTree::Iterator it = cache->pages.GetIterator(
		areaPageOffset, true, true);
	while (vm_page* page = it.Next()) {
		if (page->cache_offset >= areaPageEndOffset)
			break;

		DEBUG_PAGE_ACCESS_START(page);

		page->wired_count--;

		cache->RemovePage(page);
			// the iterator is remove-safe
		vm_page_free(cache, page);
	}

	cache->ReleaseRefAndUnlock();

	vm_unreserve_memory(size);

	return B_OK;
}


/*static*/ bool
MemoryManager::_ConvertEarlyAreas(AreaList& areas)
{
	for (AreaList::Iterator it = areas.GetIterator();
			Area* area = it.Next();) {
		if (area->vmArea != NULL) {
			// unmap mapped chunks
			while (area->mappedFreeChunks != NULL) {
				Chunk* chunk = _pop(area->mappedFreeChunks);
				_UnmapChunk(area->vmArea, _ChunkAddress(area, chunk),
					area->chunkSize, 0);
				_push(area->unmappedFreeChunks, chunk);
			}
			continue;
		}

		void* address = area;
		area_id areaID = create_area(kSlabAreaName, &address, B_EXACT_ADDRESS,
			SLAB_AREA_SIZE, B_ALREADY_WIRED,
			B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
		if (areaID < 0)
			panic("out of memory");

		area->vmArea = VMAreaHash::Lookup(areaID);
		return true;
	}

	return false;
}


/*static*/ int
MemoryManager::_DumpArea(int argc, char** argv)
{
	if (argc != 2) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	uint64 address;
	if (!evaluate_debug_expression(argv[1], &address, false))
		return 0;

	address = ROUNDDOWN(address, SLAB_AREA_SIZE);

	Area* area = (Area*)(addr_t)address;

	kprintf("chunk        base       cache  object size  cache name\n");

	// Get the last chunk in each of the free lists. This allows us to easily
	// identify free chunks, since besides these two all other free chunks
	// have a Chunk::next pointing to another chunk.
	Chunk* lastMappedFree = area->mappedFreeChunks;
	if (lastMappedFree != NULL) {
		while (lastMappedFree->next != NULL)
			lastMappedFree = lastMappedFree->next;
	}

	Chunk* lastUnmappedFree = area->unmappedFreeChunks;
	if (lastUnmappedFree != NULL) {
		while (lastUnmappedFree->next != NULL)
			lastUnmappedFree = lastUnmappedFree->next;
	}

	for (uint32 i = 0; i < area->chunkCount; i++) {
		Chunk* chunk = area->chunks + i;
		if (chunk == lastMappedFree || chunk == lastUnmappedFree)
			continue;
		if (chunk->next >= area->chunks
				&& chunk->next < area->chunks + area->chunkCount) {
			continue;
		}

		ObjectCache* cache = chunk->cache;
		kprintf("%5" B_PRIu32 "  %p  %p  %11" B_PRIuSIZE "  %s\n", i,
			(void*)_ChunkAddress(area, chunk), cache,
			cache != NULL ? cache->object_size : 0,
			cache != NULL ? cache->name : "");
	}

	return 0;
}


/*static*/ int
MemoryManager::_DumpAreas(int argc, char** argv)
{
	kprintf("      base        area  chunk size  count   used  mapped free\n");

	for (AreaTable::Iterator it = sAreaTable.GetIterator();
			Area* area = it.Next();) {
		// count the mapped free chunks
		int mappedFreeChunks = 0;
		for (Chunk* chunk = area->mappedFreeChunks; chunk != NULL;
				chunk = chunk->next) {
			mappedFreeChunks++;
		}

		kprintf("%p  %p  %10" B_PRIuSIZE "  %5u  %5u  %11d\n",
			area, area->vmArea, area->chunkSize, area->chunkCount,
			area->usedChunkCount, mappedFreeChunks);
	}

	return 0;
}
