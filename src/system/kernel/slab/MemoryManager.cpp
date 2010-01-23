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
MemoryManager::AreaTable MemoryManager::sAreaTable;
MemoryManager::Area* MemoryManager::sFreeAreas;
int MemoryManager::sFreeAreaCount;
MemoryManager::MetaChunkList MemoryManager::sFreeCompleteMetaChunks;
MemoryManager::MetaChunkList MemoryManager::sFreeShortMetaChunks;
MemoryManager::MetaChunkList MemoryManager::sPartialMetaChunksSmall;
MemoryManager::MetaChunkList MemoryManager::sPartialMetaChunksMedium;
MemoryManager::AllocationEntry* MemoryManager::sAllocationEntryCanWait;
MemoryManager::AllocationEntry* MemoryManager::sAllocationEntryDontWait;
bool MemoryManager::sMaintenanceNeeded;


/*static*/ void
MemoryManager::Init(kernel_args* args)
{
	mutex_init(&sLock, "slab memory manager");
	rw_lock_init(&sAreaTableLock, "slab memory manager area table");
	sKernelArgs = args;

	new(&sFreeCompleteMetaChunks) MetaChunkList;
	new(&sFreeShortMetaChunks) MetaChunkList;
	new(&sPartialMetaChunksSmall) MetaChunkList;
	new(&sPartialMetaChunksMedium) MetaChunkList;

	new(&sAreaTable) AreaTable;
	sAreaTable.Resize(sAreaTableBuffer, sizeof(sAreaTableBuffer), true);
		// A bit hacky: The table now owns the memory. Since we never resize or
		// free it, that's not a problem, though.

	sFreeAreas = NULL;
	sFreeAreaCount = 0;
	sMaintenanceNeeded = false;
}


/*static*/ void
MemoryManager::InitPostArea()
{
	sKernelArgs = NULL;

	// Convert all areas to actual areas. This loop might look a bit weird, but
	// is necessary since creating the actual area involves memory allocations,
	// which in turn can change the situation.
	bool done;
	do {
		done = true;

		for (AreaTable::Iterator it = sAreaTable.GetIterator();
				Area* area = it.Next();) {
			if (area->vmArea == NULL) {
				_ConvertEarlyArea(area);
				done = false;
				break;
			}
		}
	} while (!done);

	// unmap and free unused pages
	if (sFreeAreas != NULL) {
		// Just "leak" all but the first of the free areas -- the VM will
		// automatically free all unclaimed memory.
		sFreeAreas->next = NULL;
		sFreeAreaCount = 1;

		Area* area = sFreeAreas;
		_ConvertEarlyArea(area);
		_UnmapFreeChunksEarly(area);
	}

	for (AreaTable::Iterator it = sAreaTable.GetIterator();
			Area* area = it.Next();) {
		_UnmapFreeChunksEarly(area);
	}

	sMaintenanceNeeded = true;
		// might not be necessary, but doesn't harm

	add_debugger_command_etc("slab_area", &_DumpArea,
		"Dump information on a given slab area",
		"[ -c ] <area>\n"
		"Dump information on a given slab area specified by its base "
			"address.\n"
		"If \"-c\" is given, the chunks of all meta chunks area printed as "
			"well.\n", 0);
	add_debugger_command_etc("slab_areas", &_DumpAreas,
		"List all slab areas",
		"\n"
		"Lists all slab areas.\n", 0);
	add_debugger_command_etc("slab_meta_chunk", &_DumpMetaChunk,
		"Dump information on a given slab meta chunk",
		"<meta chunk>\n"
		"Dump information on a given slab meta chunk specified by its base "
			"or object address.\n", 0);
	add_debugger_command_etc("slab_meta_chunks", &_DumpMetaChunks,
		"List all non-full slab meta chunks",
		"[ -c ]\n"
		"Lists all non-full slab meta chunks.\n"
		"If \"-c\" is given, the chunks of all meta chunks area printed as "
			"well.\n", 0);
}


/*static*/ status_t
MemoryManager::Allocate(ObjectCache* cache, uint32 flags, void*& _pages)
{
	// TODO: Support CACHE_UNLOCKED_PAGES!

	size_t chunkSize = cache->slab_size;

	TRACE("MemoryManager::Allocate(%p, %#" B_PRIx32 "): chunkSize: %"
		B_PRIuSIZE "\n", cache, flags, chunkSize);

	MutexLocker locker(sLock);

	// allocate a chunk
	MetaChunk* metaChunk;
	Chunk* chunk;
	status_t error = _AllocateChunk(chunkSize, flags, metaChunk, chunk);
	if (error != B_OK)
		return error;

	// map the chunk
	Area* area = metaChunk->GetArea();
	addr_t chunkAddress = _ChunkAddress(metaChunk, chunk);

	locker.Unlock();
	error = _MapChunk(area->vmArea, chunkAddress, chunkSize, 0, flags);
	locker.Lock();
	if (error != B_OK) {
		// something failed -- free the chunk
		_FreeChunk(area, metaChunk, chunk, chunkAddress, true, flags);
		return error;
	}

	chunk->cache = cache;
	_pages = (void*)chunkAddress;

	TRACE("MemoryManager::Allocate() done: %p (meta chunk: %d, chunk %d)\n",
		_pages, int(metaChunk - area->metaChunks),
		int(chunk - metaChunk->chunks));
	return B_OK;
}


/*static*/ void
MemoryManager::Free(void* pages, uint32 flags)
{
	TRACE("MemoryManager::Free(%p, %#" B_PRIx32 ")\n", pages, flags);

	// get the area and the meta chunk
	Area* area = (Area*)ROUNDDOWN((addr_t)pages, SLAB_AREA_SIZE);
	MetaChunk* metaChunk = &area->metaChunks[
		((addr_t)pages % SLAB_AREA_SIZE) / SLAB_CHUNK_SIZE_LARGE];

	ASSERT((addr_t)pages >= metaChunk->chunkBase);
	ASSERT(((addr_t)pages % metaChunk->chunkSize) == 0);

	// get the chunk
	uint16 chunkIndex = _ChunkIndexForAddress(metaChunk, (addr_t)pages);
	Chunk* chunk = &metaChunk->chunks[chunkIndex];

	ASSERT(chunk->next != NULL);
	ASSERT(chunk->next < metaChunk->chunks
		|| chunk->next
			>= metaChunk->chunks + SLAB_SMALL_CHUNKS_PER_META_CHUNK);

	// and free it
	MutexLocker locker(sLock);
	_FreeChunk(area, metaChunk, chunk, (addr_t)pages, false, flags);
}


/*static*/ size_t
MemoryManager::AcceptableChunkSize(size_t size)
{
	if (size <= SLAB_CHUNK_SIZE_SMALL)
		return SLAB_CHUNK_SIZE_SMALL;
	if (size <= SLAB_CHUNK_SIZE_MEDIUM)
		return SLAB_CHUNK_SIZE_MEDIUM;
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

	MetaChunk* metaChunk = &area->metaChunks[
		((addr_t)address % SLAB_AREA_SIZE) / SLAB_CHUNK_SIZE_LARGE];

	// get the chunk
	ASSERT((addr_t)address >= metaChunk->chunkBase);
	uint16 chunkIndex = _ChunkIndexForAddress(metaChunk, (addr_t)address);

	return metaChunk->chunks[chunkIndex].cache;
}


/*static*/ void
MemoryManager::PerformMaintenance()
{
	MutexLocker locker(sLock);

	while (sMaintenanceNeeded) {
		sMaintenanceNeeded = false;

		// We want to keep one or two areas as a reserve. This way we have at
		// least one area to use in situations when we aren't allowed to
		// allocate one and also avoid ping-pong effects.
		if (sFreeAreaCount > 0 && sFreeAreaCount <= 2)
			return;

		if (sFreeAreaCount == 0) {
			// try to allocate one
			Area* area;
			if (_AllocateArea(0, area) != B_OK)
				return;

			_push(sFreeAreas, area);
			if (++sFreeAreaCount > 2)
				sMaintenanceNeeded = true;
		} else {
			// free until we only have two free ones
			while (sFreeAreaCount > 2) {
				Area* area = _pop(sFreeAreas);
				_FreeArea(area, true, 0);
			}

			if (sFreeAreaCount == 0)
				sMaintenanceNeeded = true;
		}
	}
}


/*static*/ status_t
MemoryManager::_AllocateChunk(size_t chunkSize, uint32 flags,
	MetaChunk*& _metaChunk, Chunk*& _chunk)
{
	MetaChunkList* metaChunkList = NULL;
	if (chunkSize == SLAB_CHUNK_SIZE_SMALL) {
		metaChunkList = &sPartialMetaChunksSmall;
	} else if (chunkSize == SLAB_CHUNK_SIZE_MEDIUM) {
		metaChunkList = &sPartialMetaChunksMedium;
	} else if (chunkSize != SLAB_CHUNK_SIZE_LARGE) {
		panic("MemoryManager::_AllocateChunk(): Unsupported chunk size: %"
			B_PRIuSIZE, chunkSize);
		return B_BAD_VALUE;
	}

	if (_GetChunk(metaChunkList, chunkSize, _metaChunk, _chunk))
		return B_OK;

	if (sFreeAreas != NULL) {
		_AddArea(_pop(sFreeAreas));
		sFreeAreaCount--;
		_RequestMaintenance();

		_GetChunk(metaChunkList, chunkSize, _metaChunk, _chunk);
		return B_OK;
	}

	if ((flags & CACHE_DONT_LOCK_KERNEL_SPACE) != 0) {
		// We can't create an area with this limitation and we must not wait for
		// someone else doing that.
		return B_WOULD_BLOCK;
	}

	// We need to allocate a new area. Wait, if someone else is trying to do
	// the same.
	while (true) {
		AllocationEntry* allocationEntry = NULL;
		if (sAllocationEntryDontWait != NULL) {
			allocationEntry = sAllocationEntryDontWait;
		} else if (sAllocationEntryCanWait != NULL
				&& (flags & CACHE_DONT_WAIT_FOR_MEMORY) == 0) {
			allocationEntry = sAllocationEntryCanWait;
		} else
			break;

		ConditionVariableEntry entry;
		allocationEntry->condition.Add(&entry);

		mutex_unlock(&sLock);
		entry.Wait();
		mutex_lock(&sLock);

		if (_GetChunk(metaChunkList, chunkSize, _metaChunk, _chunk))
			return B_OK;
	}

	// prepare the allocation entry others can wait on
	AllocationEntry*& allocationEntry
		= (flags & CACHE_DONT_WAIT_FOR_MEMORY) != 0
			? sAllocationEntryDontWait : sAllocationEntryCanWait;

	AllocationEntry myResizeEntry;
	allocationEntry = &myResizeEntry;
	allocationEntry->condition.Init(metaChunkList, "wait for slab area");
	allocationEntry->thread = find_thread(NULL);

	Area* area;
	status_t error = _AllocateArea(flags, area);

	allocationEntry->condition.NotifyAll();
	allocationEntry = NULL;

	if (error != B_OK)
		return error;

	// Try again to get a meta chunk. Something might have been freed in the
	// meantime. We can free the area in this case.
	if (_GetChunk(metaChunkList, chunkSize, _metaChunk, _chunk)) {
		_FreeArea(area, true, flags);
		return B_OK;
	}

	_AddArea(area);
	_GetChunk(metaChunkList, chunkSize, _metaChunk, _chunk);
	return B_OK;
}


/*static*/ bool
MemoryManager::_GetChunk(MetaChunkList* metaChunkList, size_t chunkSize,
	MetaChunk*& _metaChunk, Chunk*& _chunk)
{
	MetaChunk* metaChunk = metaChunkList != NULL
		? metaChunkList->Head() : NULL;
	if (metaChunk == NULL) {
		// no partial meta chunk -- maybe there's a free one
		if (chunkSize == SLAB_CHUNK_SIZE_LARGE) {
			metaChunk = sFreeCompleteMetaChunks.RemoveHead();
		} else {
			metaChunk = sFreeShortMetaChunks.RemoveHead();
			if (metaChunk == NULL)
				metaChunk = sFreeCompleteMetaChunks.RemoveHead();
			if (metaChunk != NULL)
				metaChunkList->Add(metaChunk);
		}

		if (metaChunk == NULL)
			return false;

		metaChunk->GetArea()->usedMetaChunkCount++;
		_PrepareMetaChunk(metaChunk, chunkSize);
	}

	// allocate the chunk
	if (++metaChunk->usedChunkCount == metaChunk->chunkCount) {
		// meta chunk is full now -- remove it from its list
		if (metaChunkList != NULL)
			metaChunkList->Remove(metaChunk);
	}

	_chunk = _pop(metaChunk->freeChunks);
	_metaChunk = metaChunk;
	return true;
}


/*static*/ void
MemoryManager::_FreeChunk(Area* area, MetaChunk* metaChunk, Chunk* chunk,
	addr_t chunkAddress, bool alreadyUnmapped, uint32 flags)
{
	// unmap the chunk
	if (!alreadyUnmapped) {
		mutex_unlock(&sLock);
		_UnmapChunk(area->vmArea, chunkAddress, metaChunk->chunkSize, flags);
		mutex_lock(&sLock);
	}

	_push(metaChunk->freeChunks, chunk);

	// free the meta chunk, if it is unused now
	ASSERT(metaChunk->usedChunkCount > 0);
	if (--metaChunk->usedChunkCount == 0) {
		// remove from partial meta chunk list
		if (metaChunk->chunkSize == SLAB_CHUNK_SIZE_SMALL)
			sPartialMetaChunksSmall.Remove(metaChunk);
		else if (metaChunk->chunkSize == SLAB_CHUNK_SIZE_MEDIUM)
			sPartialMetaChunksMedium.Remove(metaChunk);

		// mark empty
		metaChunk->chunkSize = 0;

		// add to free list
		if (metaChunk == area->metaChunks)
			sFreeShortMetaChunks.Add(metaChunk, false);
		else
			sFreeCompleteMetaChunks.Add(metaChunk, false);

		// free the area, if it is unused now
		ASSERT(area->usedMetaChunkCount > 0);
		if (--area->usedMetaChunkCount == 0)
			_FreeArea(area, false, flags);
	} else if (metaChunk->usedChunkCount == metaChunk->chunkCount - 1) {
		// the meta chunk was full before -- add it back to its partial chunk
		// list
		if (metaChunk->chunkSize == SLAB_CHUNK_SIZE_SMALL)
			sPartialMetaChunksSmall.Add(metaChunk, false);
		else if (metaChunk->chunkSize == SLAB_CHUNK_SIZE_MEDIUM)
			sPartialMetaChunksMedium.Add(metaChunk, false);
	}
}


/*static*/ void
MemoryManager::_PrepareMetaChunk(MetaChunk* metaChunk, size_t chunkSize)
{
	Area* area = metaChunk->GetArea();

	if (metaChunk == area->metaChunks) {
		// the first chunk is shorter
		size_t unusableSize = ROUNDUP(kAreaAdminSize, chunkSize);
		metaChunk->chunkBase = (addr_t)area + unusableSize;
		metaChunk->totalSize = SLAB_CHUNK_SIZE_LARGE - unusableSize;
	}

	metaChunk->chunkSize = chunkSize;
	metaChunk->chunkCount = metaChunk->totalSize / chunkSize;
	metaChunk->usedChunkCount = 0;

	metaChunk->freeChunks = NULL;
	for (uint32 i = 0; i < metaChunk->chunkCount; i++)
		_push(metaChunk->freeChunks, metaChunk->chunks + i);
}


/*static*/ void
MemoryManager::_AddArea(Area* area)
{
	// add the area to the hash table
	WriteLocker writeLocker(sAreaTableLock);
	sAreaTable.InsertUnchecked(area);
	writeLocker.Unlock();

	// add the area's meta chunks to the free lists
	sFreeShortMetaChunks.Add(&area->metaChunks[0]);
	for (int32 i = 1; i < SLAB_META_CHUNKS_PER_AREA; i++)
		sFreeCompleteMetaChunks.Add(&area->metaChunks[i]);
}


/*static*/ status_t
MemoryManager::_AllocateArea(uint32 flags, Area*& _area)
{
	TRACE("MemoryManager::_AllocateArea(%#" B_PRIx32 ")\n", flags);

	ASSERT((flags & CACHE_DONT_LOCK_KERNEL_SPACE) == 0);

	mutex_unlock(&sLock);

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
		status_t error = _MapChunk(vmArea, (addr_t)area, kAreaAdminSize,
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
	area->reserved_memory_for_mapping = pagesNeededToMap * B_PAGE_SIZE;
	area->usedMetaChunkCount = 0;
	area->fullyMapped = vmArea == NULL;

	// init the meta chunks
	for (int32 i = 0; i < SLAB_META_CHUNKS_PER_AREA; i++) {
		MetaChunk* metaChunk = area->metaChunks + i;
		metaChunk->chunkSize = 0;
		metaChunk->chunkBase = (addr_t)area + i * SLAB_CHUNK_SIZE_LARGE;
		metaChunk->totalSize = SLAB_CHUNK_SIZE_LARGE;
			// Note: chunkBase and totalSize aren't correct for the first
			// meta chunk. They will be set in _PrepareMetaChunk().
		metaChunk->chunkCount = 0;
		metaChunk->usedChunkCount = 0;
		metaChunk->freeChunks = NULL;
	}

	mutex_lock(&sLock);
	_area = area;
	return B_OK;
}


/*static*/ void
MemoryManager::_FreeArea(Area* area, bool areaRemoved, uint32 flags)
{
	TRACE("MemoryManager::_FreeArea(%p, %#" B_PRIx32 ")\n", area, flags);

	ASSERT(area->usedMetaChunkCount == 0);

	if (!areaRemoved) {
		// remove the area's meta chunks from the free lists
		ASSERT(area->metaChunks[0].usedChunkCount == 0);
		sFreeShortMetaChunks.Add(&area->metaChunks[0]);

		for (int32 i = 1; i < SLAB_META_CHUNKS_PER_AREA; i++) {
			ASSERT(area->metaChunks[i].usedChunkCount == 0);
			sFreeCompleteMetaChunks.Add(&area->metaChunks[i]);
		}

		// remove the area from the hash table
		WriteLocker writeLocker(sAreaTableLock);
		sAreaTable.RemoveUnchecked(area);
		writeLocker.Unlock();
	}

	// We want to keep one or two free areas as a reserve.
	if (sFreeAreaCount <= 1) {
		_push(sFreeAreas, area);
		sFreeAreaCount++;
		return;
	}

	if (area->vmArea == NULL || (flags & CACHE_DONT_LOCK_KERNEL_SPACE) != 0) {
		// This is either early in the boot process or we aren't allowed to
		// delete the area now.
		_push(sFreeAreas, area);
		sFreeAreaCount++;
		_RequestMaintenance();
		return;
	}

	mutex_unlock(&sLock);

	delete_area(area->vmArea->id);
	vm_unreserve_memory(area->reserved_memory_for_mapping);

	mutex_lock(&sLock);
}


/*static*/ status_t
MemoryManager::_MapChunk(VMArea* vmArea, addr_t address, size_t size,
	size_t reserveAdditionalMemory, uint32 flags)
{
	TRACE("MemoryManager::_MapChunk(%p, %#" B_PRIxADDR ", %#" B_PRIxSIZE
		")\n", vmArea, address, size);

	if (vmArea == NULL) {
		// everything is mapped anyway
		return B_OK;
	}

	VMAddressSpace* addressSpace = VMAddressSpace::Kernel();
	VMTranslationMap* translationMap = addressSpace->TranslationMap();

	// reserve memory for the chunk
	size_t reservedMemory = size + reserveAdditionalMemory;
	status_t error = vm_try_reserve_memory(size,
		(flags & CACHE_DONT_WAIT_FOR_MEMORY) != 0 ? 0 : 1000000);
	if (error != B_OK)
		return error;

	// reserve the pages we need now
	size_t reservedPages = size / B_PAGE_SIZE
		+ translationMap->MaxPagesNeededToMap(address, address + size - 1);
	if ((flags & CACHE_DONT_WAIT_FOR_MEMORY) != 0) {
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


/*static*/ void
MemoryManager::_UnmapChunkEarly(addr_t address, size_t size)
{
	VMAddressSpace* addressSpace = VMAddressSpace::Kernel();
	VMTranslationMap* translationMap = addressSpace->TranslationMap();

	translationMap->Lock();

	for (size_t offset = 0; offset < B_PAGE_SIZE; offset += B_PAGE_SIZE) {
		addr_t physicalAddress;
		uint32 flags;
		if (translationMap->Query(address + offset, &physicalAddress, &flags)
				== B_OK
			&& (flags & PAGE_PRESENT) != 0) {
			vm_page* page = vm_lookup_page(physicalAddress / B_PAGE_SIZE);
			DEBUG_PAGE_ACCESS_START(page);
			vm_page_set_state(page, PAGE_STATE_FREE);
		}
	}

	translationMap->Unmap(address, address + size - 1);

	translationMap->Unlock();
}


/*static*/ void
MemoryManager::_UnmapFreeChunksEarly(Area* area)
{
	if (!area->fullyMapped)
		return;

	for (int32 i = 0; i < SLAB_META_CHUNKS_PER_AREA; i++) {
		MetaChunk* metaChunk = area->metaChunks + i;
		if (metaChunk->chunkSize == 0) {
			// meta chunk is free -- unmap it completely
			if (i == 0) {
				_UnmapChunk(area->vmArea, (addr_t)area + kAreaAdminSize,
					SLAB_CHUNK_SIZE_LARGE - kAreaAdminSize, 0);
			} else {
				_UnmapChunk(area->vmArea,
					(addr_t)area + i * SLAB_CHUNK_SIZE_LARGE,
					SLAB_CHUNK_SIZE_LARGE, 0);
			}
		} else {
			// unmap free chunks
			for (Chunk* chunk = metaChunk->freeChunks; chunk != NULL;
					chunk = chunk->next) {
				_UnmapChunk(area->vmArea, _ChunkAddress(metaChunk, chunk),
					metaChunk->chunkSize, 0);
			}

			// The first meta chunk might have space before its first chunk.
			if (i == 0) {
				addr_t unusedStart = (addr_t)area + kAreaAdminSize;
				if (unusedStart < metaChunk->chunkBase) {
					_UnmapChunk(area->vmArea, unusedStart,
						metaChunk->chunkBase - unusedStart, 0);
				}
			}
		}
	}

	area->fullyMapped = false;
}


/*static*/ void
MemoryManager::_ConvertEarlyArea(Area* area)
{
	void* address = area;
	area_id areaID = create_area(kSlabAreaName, &address, B_EXACT_ADDRESS,
		SLAB_AREA_SIZE, B_ALREADY_WIRED,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	if (areaID < 0)
		panic("out of memory");

	area->vmArea = VMAreaHash::Lookup(areaID);
}


/*static*/ void
MemoryManager::_RequestMaintenance()
{
	if ((sFreeAreaCount > 0 && sFreeAreaCount <= 2) || sMaintenanceNeeded)
		return;

	sMaintenanceNeeded = true;
	request_memory_manager_maintenance();
}


/*static*/ void
MemoryManager::_PrintMetaChunkTableHeader(bool printChunks)
{
	if (printChunks)
		kprintf("chunk        base       cache  object size  cache name\n");
	else
		kprintf("chunk        base\n");
}

/*static*/ void
MemoryManager::_DumpMetaChunk(MetaChunk* metaChunk, bool printChunks,
	bool printHeader)
{
	if (printHeader)
		_PrintMetaChunkTableHeader(printChunks);

	const char* type = "empty";
	if (metaChunk->chunkSize != 0) {
		switch (metaChunk->chunkSize) {
			case SLAB_CHUNK_SIZE_SMALL:
				type = "small";
				break;
			case SLAB_CHUNK_SIZE_MEDIUM:
				type = "medium";
				break;
			case SLAB_CHUNK_SIZE_LARGE:
				type = "large";
				break;
		}
	}

	int metaChunkIndex = metaChunk - metaChunk->GetArea()->metaChunks;
	kprintf("%5d  %p  --- %6s meta chunk", metaChunkIndex,
		(void*)metaChunk->chunkBase, type);
	if (metaChunk->chunkSize != 0) {
		kprintf(": %4u/%4u used ----------------------------\n",
			metaChunk->usedChunkCount, metaChunk->chunkCount);
	} else
		kprintf(" --------------------------------------------\n");

	if (metaChunk->chunkSize == 0 || !printChunks)
		return;

	for (uint32 i = 0; i < metaChunk->chunkCount; i++) {
		Chunk* chunk = metaChunk->chunks + i;

		// skip free chunks
		if (chunk->next == NULL)
			continue;
		if (chunk->next >= metaChunk->chunks
			&& chunk->next < metaChunk->chunks + metaChunk->chunkCount) {
			continue;
		}

		ObjectCache* cache = chunk->cache;
		kprintf("%5" B_PRIu32 "  %p  %p  %11" B_PRIuSIZE "  %s\n", i,
			(void*)_ChunkAddress(metaChunk, chunk), cache,
			cache != NULL ? cache->object_size : 0,
			cache != NULL ? cache->name : "");
	}
}


/*static*/ int
MemoryManager::_DumpMetaChunk(int argc, char** argv)
{
	if (argc != 2) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	uint64 address;
	if (!evaluate_debug_expression(argv[1], &address, false))
		return 0;

	Area* area = (Area*)(addr_t)ROUNDDOWN(address, SLAB_AREA_SIZE);

	MetaChunk* metaChunk;
	if ((addr_t)address >= (addr_t)area->metaChunks
		&& (addr_t)address
			< (addr_t)(area->metaChunks + SLAB_META_CHUNKS_PER_AREA)) {
		metaChunk = (MetaChunk*)(addr_t)address;
	} else {
		metaChunk = area->metaChunks
			+ (address % SLAB_AREA_SIZE) / SLAB_CHUNK_SIZE_LARGE;
	}

	_DumpMetaChunk(metaChunk, true, true);

	return 0;
}


/*static*/ void
MemoryManager::_DumpMetaChunks(const char* name, MetaChunkList& metaChunkList,
	bool printChunks)
{
	kprintf("%s:\n", name);

	for (MetaChunkList::Iterator it = metaChunkList.GetIterator();
			MetaChunk* metaChunk = it.Next();) {
		_DumpMetaChunk(metaChunk, printChunks, false);
	}
}


/*static*/ int
MemoryManager::_DumpMetaChunks(int argc, char** argv)
{
	bool printChunks = argc > 1 && strcmp(argv[1], "-c") == 0;

	_PrintMetaChunkTableHeader(printChunks);
	_DumpMetaChunks("free complete", sFreeCompleteMetaChunks, printChunks);
	_DumpMetaChunks("free short", sFreeShortMetaChunks, printChunks);
	_DumpMetaChunks("partial small", sPartialMetaChunksSmall, printChunks);
	_DumpMetaChunks("partial medium", sPartialMetaChunksMedium, printChunks);

	return 0;
}


/*static*/ int
MemoryManager::_DumpArea(int argc, char** argv)
{
	bool printChunks = false;

	int argi = 1;
	while (argi < argc) {
		if (argv[argi][0] != '-')
			break;
		const char* arg = argv[argi++];
		if (strcmp(arg, "-c") == 0) {
			printChunks = true;
		} else {
			print_debugger_command_usage(argv[0]);
			return 0;
		}
	}

	if (argi + 1 != argc) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	uint64 address;
	if (!evaluate_debug_expression(argv[argi], &address, false))
		return 0;

	address = ROUNDDOWN(address, SLAB_AREA_SIZE);

	Area* area = (Area*)(addr_t)address;

	for (uint32 k = 0; k < SLAB_META_CHUNKS_PER_AREA; k++) {
		MetaChunk* metaChunk = area->metaChunks + k;
		_DumpMetaChunk(metaChunk, printChunks, k == 0);
	}

	return 0;
}


/*static*/ int
MemoryManager::_DumpAreas(int argc, char** argv)
{
	kprintf("      base        area   meta      small   medium  large\n");

	for (AreaTable::Iterator it = sAreaTable.GetIterator();
			Area* area = it.Next();) {
		// sum up the free/used counts for the chunk sizes
		int totalSmall = 0;
		int usedSmall = 0;
		int totalMedium = 0;
		int usedMedium = 0;
		int totalLarge = 0;
		int usedLarge = 0;

		for (int32 i = 0; i < SLAB_META_CHUNKS_PER_AREA; i++) {
			MetaChunk* metaChunk = area->metaChunks + i;
			if (metaChunk->chunkSize == 0)
				continue;

			switch (metaChunk->chunkSize) {
				case SLAB_CHUNK_SIZE_SMALL:
					totalSmall += metaChunk->chunkCount;
					usedSmall += metaChunk->usedChunkCount;
					break;
				case SLAB_CHUNK_SIZE_MEDIUM:
					totalMedium += metaChunk->chunkCount;
					usedMedium += metaChunk->usedChunkCount;
					break;
				case SLAB_CHUNK_SIZE_LARGE:
					totalLarge += metaChunk->chunkCount;
					usedLarge += metaChunk->usedChunkCount;
					break;
			}
		}

		kprintf("%p  %p  %2u/%2u  %4d/%4d  %3d/%3d  %2d/%2d\n",
			area, area->vmArea, area->usedMetaChunkCount,
			SLAB_META_CHUNKS_PER_AREA, usedSmall, totalSmall, usedMedium,
			totalMedium, usedLarge, totalLarge);
	}

	kprintf("%d free areas:\n", sFreeAreaCount);
	for (Area* area = sFreeAreas; area != NULL; area = area->next)
		kprintf("%p  %p\n", area, area->vmArea);

	return 0;
}
