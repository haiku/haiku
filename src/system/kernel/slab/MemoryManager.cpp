/*
 * Copyright 2010, Ingo Weinhold <ingo_weinhold@gmx.de>.
 * Distributed under the terms of the MIT License.
 */


#include "MemoryManager.h"

#include <algorithm>

#include <debug.h>
#include <tracing.h>
#include <util/AutoLock.h>
#include <vm/vm.h>
#include <vm/vm_page.h>
#include <vm/vm_priv.h>
#include <vm/VMAddressSpace.h>
#include <vm/VMArea.h>
#include <vm/VMCache.h>
#include <vm/VMTranslationMap.h>

#include "kernel_debug_config.h"

#include "ObjectCache.h"
#include "slab_private.h"


//#define TRACE_MEMORY_MANAGER
#ifdef TRACE_MEMORY_MANAGER
#	define TRACE(x...)	dprintf(x)
#else
#	define TRACE(x...)	do {} while (false)
#endif

#if DEBUG_SLAB_MEMORY_MANAGER_PARANOID_CHECKS
#	define PARANOID_CHECKS_ONLY(x)	x
#else
#	define PARANOID_CHECKS_ONLY(x)
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


// #pragma mark - kernel tracing


#if SLAB_MEMORY_MANAGER_TRACING


//namespace SlabMemoryManagerCacheTracing {
struct MemoryManager::Tracing {

class MemoryManagerTraceEntry : public AbstractTraceEntry {
public:
	MemoryManagerTraceEntry()
	{
	}
};


class Allocate : public MemoryManagerTraceEntry {
public:
	Allocate(ObjectCache* cache, uint32 flags)
		:
		MemoryManagerTraceEntry(),
		fCache(cache),
		fFlags(flags)
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("slab memory manager alloc: cache: %p, flags: %#" B_PRIx32,
			fCache, fFlags);
	}

private:
	ObjectCache*	fCache;
	uint32			fFlags;
};


class Free : public MemoryManagerTraceEntry {
public:
	Free(void* address, uint32 flags)
		:
		MemoryManagerTraceEntry(),
		fAddress(address),
		fFlags(flags)
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("slab memory manager free: address: %p, flags: %#" B_PRIx32,
			fAddress, fFlags);
	}

private:
	void*	fAddress;
	uint32	fFlags;
};


class AllocateRaw : public MemoryManagerTraceEntry {
public:
	AllocateRaw(size_t size, uint32 flags)
		:
		MemoryManagerTraceEntry(),
		fSize(size),
		fFlags(flags)
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("slab memory manager alloc raw: size: %" B_PRIuSIZE
			", flags: %#" B_PRIx32, fSize, fFlags);
	}

private:
	size_t	fSize;
	uint32	fFlags;
};


class FreeRawOrReturnCache : public MemoryManagerTraceEntry {
public:
	FreeRawOrReturnCache(void* address, uint32 flags)
		:
		MemoryManagerTraceEntry(),
		fAddress(address),
		fFlags(flags)
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("slab memory manager free raw/return: address: %p, flags: %#"
			B_PRIx32, fAddress, fFlags);
	}

private:
	void*	fAddress;
	uint32	fFlags;
};


class AllocateArea : public MemoryManagerTraceEntry {
public:
	AllocateArea(Area* area, uint32 flags)
		:
		MemoryManagerTraceEntry(),
		fArea(area),
		fFlags(flags)
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("slab memory manager alloc area: flags: %#" B_PRIx32
			" -> %p", fFlags, fArea);
	}

private:
	Area*	fArea;
	uint32	fFlags;
};


class AddArea : public MemoryManagerTraceEntry {
public:
	AddArea(Area* area)
		:
		MemoryManagerTraceEntry(),
		fArea(area)
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("slab memory manager add area: %p", fArea);
	}

private:
	Area*	fArea;
};


class FreeArea : public MemoryManagerTraceEntry {
public:
	FreeArea(Area* area, bool areaRemoved, uint32 flags)
		:
		MemoryManagerTraceEntry(),
		fArea(area),
		fFlags(flags),
		fRemoved(areaRemoved)
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("slab memory manager free area: %p%s, flags: %#" B_PRIx32,
			fArea, fRemoved ? " (removed)" : "", fFlags);
	}

private:
	Area*	fArea;
	uint32	fFlags;
	bool	fRemoved;
};


class AllocateMetaChunk : public MemoryManagerTraceEntry {
public:
	AllocateMetaChunk(MetaChunk* metaChunk)
		:
		MemoryManagerTraceEntry(),
		fMetaChunk(metaChunk->chunkBase)
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("slab memory manager alloc meta chunk: %#" B_PRIxADDR,
			fMetaChunk);
	}

private:
	addr_t	fMetaChunk;
};


class FreeMetaChunk : public MemoryManagerTraceEntry {
public:
	FreeMetaChunk(MetaChunk* metaChunk)
		:
		MemoryManagerTraceEntry(),
		fMetaChunk(metaChunk->chunkBase)
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("slab memory manager free meta chunk: %#" B_PRIxADDR,
			fMetaChunk);
	}

private:
	addr_t	fMetaChunk;
};


class AllocateChunk : public MemoryManagerTraceEntry {
public:
	AllocateChunk(size_t chunkSize, MetaChunk* metaChunk, Chunk* chunk)
		:
		MemoryManagerTraceEntry(),
		fChunkSize(chunkSize),
		fMetaChunk(metaChunk->chunkBase),
		fChunk(chunk - metaChunk->chunks)
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("slab memory manager alloc chunk: size: %" B_PRIuSIZE
			" -> meta chunk: %#" B_PRIxADDR ", chunk: %" B_PRIu32, fChunkSize,
			fMetaChunk, fChunk);
	}

private:
	size_t	fChunkSize;
	addr_t	fMetaChunk;
	uint32	fChunk;
};


class AllocateChunks : public MemoryManagerTraceEntry {
public:
	AllocateChunks(size_t chunkSize, uint32 chunkCount, MetaChunk* metaChunk,
		Chunk* chunk)
		:
		MemoryManagerTraceEntry(),
		fMetaChunk(metaChunk->chunkBase),
		fChunkSize(chunkSize),
		fChunkCount(chunkCount),
		fChunk(chunk - metaChunk->chunks)
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("slab memory manager alloc chunks: size: %" B_PRIuSIZE
			", count %" B_PRIu32 " -> meta chunk: %#" B_PRIxADDR ", chunk: %"
			B_PRIu32, fChunkSize, fChunkCount, fMetaChunk, fChunk);
	}

private:
	addr_t	fMetaChunk;
	size_t	fChunkSize;
	uint32	fChunkCount;
	uint32	fChunk;
};


class FreeChunk : public MemoryManagerTraceEntry {
public:
	FreeChunk(MetaChunk* metaChunk, Chunk* chunk)
		:
		MemoryManagerTraceEntry(),
		fMetaChunk(metaChunk->chunkBase),
		fChunk(chunk - metaChunk->chunks)
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("slab memory manager free chunk: meta chunk: %#" B_PRIxADDR
			", chunk: %" B_PRIu32, fMetaChunk, fChunk);
	}

private:
	addr_t	fMetaChunk;
	uint32	fChunk;
};


class Map : public MemoryManagerTraceEntry {
public:
	Map(addr_t address, size_t size, uint32 flags)
		:
		MemoryManagerTraceEntry(),
		fAddress(address),
		fSize(size),
		fFlags(flags)
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("slab memory manager map: %#" B_PRIxADDR ", size: %"
			B_PRIuSIZE ", flags: %#" B_PRIx32, fAddress, fSize, fFlags);
	}

private:
	addr_t	fAddress;
	size_t	fSize;
	uint32	fFlags;
};


class Unmap : public MemoryManagerTraceEntry {
public:
	Unmap(addr_t address, size_t size, uint32 flags)
		:
		MemoryManagerTraceEntry(),
		fAddress(address),
		fSize(size),
		fFlags(flags)
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("slab memory manager unmap: %#" B_PRIxADDR ", size: %"
			B_PRIuSIZE ", flags: %#" B_PRIx32, fAddress, fSize, fFlags);
	}

private:
	addr_t	fAddress;
	size_t	fSize;
	uint32	fFlags;
};


//}	// namespace SlabMemoryManagerCacheTracing
};	// struct MemoryManager::Tracing


//#	define T(x)	new(std::nothrow) SlabMemoryManagerCacheTracing::x
#	define T(x)	new(std::nothrow) MemoryManager::Tracing::x

#else
#	define T(x)
#endif	// SLAB_MEMORY_MANAGER_TRACING


// #pragma mark - MemoryManager


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
	add_debugger_command_etc("slab_raw_allocations", &_DumpRawAllocations,
		"List all raw allocations in slab areas",
		"\n"
		"Lists all raw allocations in slab areas.\n", 0);
}


/*static*/ status_t
MemoryManager::Allocate(ObjectCache* cache, uint32 flags, void*& _pages)
{
	// TODO: Support CACHE_UNLOCKED_PAGES!

	T(Allocate(cache, flags));

	size_t chunkSize = cache->slab_size;

	TRACE("MemoryManager::Allocate(%p, %#" B_PRIx32 "): chunkSize: %"
		B_PRIuSIZE "\n", cache, flags, chunkSize);

	MutexLocker locker(sLock);

	// allocate a chunk
	MetaChunk* metaChunk;
	Chunk* chunk;
	status_t error = _AllocateChunks(chunkSize, 1, flags, metaChunk, chunk);
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

	chunk->reference = (addr_t)cache;
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

	T(Free(pages, flags));

	// get the area and the meta chunk
	Area* area = _AreaForAddress((addr_t)pages);
	MetaChunk* metaChunk = &area->metaChunks[
		((addr_t)pages % SLAB_AREA_SIZE) / SLAB_CHUNK_SIZE_LARGE];

	ASSERT(metaChunk->chunkSize > 0);
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


/*static*/ status_t
MemoryManager::AllocateRaw(size_t size, uint32 flags, void*& _pages)
{
	T(AllocateRaw(size, flags));

	size = ROUNDUP(size, SLAB_CHUNK_SIZE_SMALL);

	TRACE("MemoryManager::AllocateRaw(%" B_PRIuSIZE ", %#" B_PRIx32 ")\n", size,
		  flags);

	if (size > SLAB_CHUNK_SIZE_LARGE || (flags & CACHE_ALIGN_ON_SIZE) != 0) {
		// Requested size greater than a large chunk or an aligned allocation.
		// Allocate as an area.
		if ((flags & CACHE_DONT_LOCK_KERNEL_SPACE) != 0)
			return B_WOULD_BLOCK;

		virtual_address_restrictions virtualRestrictions = {};
		virtualRestrictions.address_specification
			= (flags & CACHE_ALIGN_ON_SIZE) != 0
				? B_ANY_KERNEL_BLOCK_ADDRESS : B_ANY_KERNEL_ADDRESS;
		physical_address_restrictions physicalRestrictions = {};
		area_id area = create_area_etc(VMAddressSpace::KernelID(),
			"slab large raw allocation", size, B_FULL_LOCK,
			B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
			((flags & CACHE_DONT_WAIT_FOR_MEMORY) != 0
					? CREATE_AREA_DONT_WAIT : 0)
				| CREATE_AREA_DONT_CLEAR,
			&virtualRestrictions, &physicalRestrictions, &_pages);

		status_t result = area >= 0 ? B_OK : area;
		if (result == B_OK)
			fill_allocated_block(_pages, size);
		return result;
	}

	// determine chunk size (small or medium)
	size_t chunkSize = SLAB_CHUNK_SIZE_SMALL;
	uint32 chunkCount = size / SLAB_CHUNK_SIZE_SMALL;

	if (size % SLAB_CHUNK_SIZE_MEDIUM == 0) {
		chunkSize = SLAB_CHUNK_SIZE_MEDIUM;
		chunkCount = size / SLAB_CHUNK_SIZE_MEDIUM;
	}

	MutexLocker locker(sLock);

	// allocate the chunks
	MetaChunk* metaChunk;
	Chunk* chunk;
	status_t error = _AllocateChunks(chunkSize, chunkCount, flags, metaChunk,
		chunk);
	if (error != B_OK)
		return error;

	// map the chunks
	Area* area = metaChunk->GetArea();
	addr_t chunkAddress = _ChunkAddress(metaChunk, chunk);

	locker.Unlock();
	error = _MapChunk(area->vmArea, chunkAddress, size, 0, flags);
	locker.Lock();
	if (error != B_OK) {
		// something failed -- free the chunks
		for (uint32 i = 0; i < chunkCount; i++)
			_FreeChunk(area, metaChunk, chunk + i, chunkAddress, true, flags);
		return error;
	}

	chunk->reference = (addr_t)chunkAddress + size - 1;
	_pages = (void*)chunkAddress;

	fill_allocated_block(_pages, size);

	TRACE("MemoryManager::AllocateRaw() done: %p (meta chunk: %d, chunk %d)\n",
		_pages, int(metaChunk - area->metaChunks),
		int(chunk - metaChunk->chunks));
	return B_OK;
}


/*static*/ ObjectCache*
MemoryManager::FreeRawOrReturnCache(void* pages, uint32 flags)
{
	TRACE("MemoryManager::FreeRawOrReturnCache(%p, %#" B_PRIx32 ")\n", pages,
		flags);

	T(FreeRawOrReturnCache(pages, flags));

	// get the area
	addr_t areaBase = _AreaBaseAddressForAddress((addr_t)pages);

	ReadLocker readLocker(sAreaTableLock);
	Area* area = sAreaTable.Lookup(areaBase);
	readLocker.Unlock();

	if (area == NULL) {
		// Probably a large allocation. Look up the VM area.
		VMAddressSpace* addressSpace = VMAddressSpace::Kernel();
		addressSpace->ReadLock();
		VMArea* area = addressSpace->LookupArea((addr_t)pages);
		addressSpace->ReadUnlock();

		if (area != NULL && (addr_t)pages == area->Base())
			delete_area(area->id);
		else
			panic("freeing unknown block %p from area %p", pages, area);

		return NULL;
	}

	MetaChunk* metaChunk = &area->metaChunks[
		((addr_t)pages % SLAB_AREA_SIZE) / SLAB_CHUNK_SIZE_LARGE];

	// get the chunk
	ASSERT(metaChunk->chunkSize > 0);
	ASSERT((addr_t)pages >= metaChunk->chunkBase);
	uint16 chunkIndex = _ChunkIndexForAddress(metaChunk, (addr_t)pages);
	Chunk* chunk = &metaChunk->chunks[chunkIndex];

	addr_t reference = chunk->reference;
	if ((reference & 1) == 0)
		return (ObjectCache*)reference;

	// Seems we have a raw chunk allocation.
	ASSERT((addr_t)pages == _ChunkAddress(metaChunk, chunk));
	ASSERT(reference > (addr_t)pages);
	ASSERT(reference <= areaBase + SLAB_AREA_SIZE - 1);
	size_t size = reference - (addr_t)pages + 1;
	ASSERT((size % SLAB_CHUNK_SIZE_SMALL) == 0);

	// unmap the chunks
	_UnmapChunk(area->vmArea, (addr_t)pages, size, flags);

	// and free them
	MutexLocker locker(sLock);
	uint32 chunkCount = size / metaChunk->chunkSize;
	for (uint32 i = 0; i < chunkCount; i++)
		_FreeChunk(area, metaChunk, chunk + i, (addr_t)pages, true, flags);

	return NULL;
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
MemoryManager::GetAllocationInfo(void* address, size_t& _size)
{
	// get the area
	ReadLocker readLocker(sAreaTableLock);
	Area* area = sAreaTable.Lookup(_AreaBaseAddressForAddress((addr_t)address));
	readLocker.Unlock();

	if (area == NULL) {
		VMAddressSpace* addressSpace = VMAddressSpace::Kernel();
		addressSpace->ReadLock();
		VMArea* area = addressSpace->LookupArea((addr_t)address);
		if (area != NULL && (addr_t)address == area->Base())
			_size = area->Size();
		else
			_size = 0;
		addressSpace->ReadUnlock();

		return NULL;
	}

	MetaChunk* metaChunk = &area->metaChunks[
		((addr_t)address % SLAB_AREA_SIZE) / SLAB_CHUNK_SIZE_LARGE];

	// get the chunk
	ASSERT(metaChunk->chunkSize > 0);
	ASSERT((addr_t)address >= metaChunk->chunkBase);
	uint16 chunkIndex = _ChunkIndexForAddress(metaChunk, (addr_t)address);

	addr_t reference = metaChunk->chunks[chunkIndex].reference;
	if ((reference & 1) == 0) {
		ObjectCache* cache = (ObjectCache*)reference;
		_size = cache->object_size;
		return cache;
	}

	_size = reference - (addr_t)address + 1;
	return NULL;
}


/*static*/ ObjectCache*
MemoryManager::CacheForAddress(void* address)
{
	// get the area
	ReadLocker readLocker(sAreaTableLock);
	Area* area = sAreaTable.Lookup(_AreaBaseAddressForAddress((addr_t)address));
	readLocker.Unlock();

	if (area == NULL)
		return NULL;

	MetaChunk* metaChunk = &area->metaChunks[
		((addr_t)address % SLAB_AREA_SIZE) / SLAB_CHUNK_SIZE_LARGE];

	// get the chunk
	ASSERT(metaChunk->chunkSize > 0);
	ASSERT((addr_t)address >= metaChunk->chunkBase);
	uint16 chunkIndex = _ChunkIndexForAddress(metaChunk, (addr_t)address);

	addr_t reference = metaChunk->chunks[chunkIndex].reference;
	return (reference & 1) == 0 ? (ObjectCache*)reference : NULL;
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
MemoryManager::_AllocateChunks(size_t chunkSize, uint32 chunkCount,
	uint32 flags, MetaChunk*& _metaChunk, Chunk*& _chunk)
{
	MetaChunkList* metaChunkList = NULL;
	if (chunkSize == SLAB_CHUNK_SIZE_SMALL) {
		metaChunkList = &sPartialMetaChunksSmall;
	} else if (chunkSize == SLAB_CHUNK_SIZE_MEDIUM) {
		metaChunkList = &sPartialMetaChunksMedium;
	} else if (chunkSize != SLAB_CHUNK_SIZE_LARGE) {
		panic("MemoryManager::_AllocateChunks(): Unsupported chunk size: %"
			B_PRIuSIZE, chunkSize);
		return B_BAD_VALUE;
	}

	if (_GetChunks(metaChunkList, chunkSize, chunkCount, _metaChunk, _chunk))
		return B_OK;

	if (sFreeAreas != NULL) {
		_AddArea(_pop(sFreeAreas));
		sFreeAreaCount--;
		_RequestMaintenance();

		_GetChunks(metaChunkList, chunkSize, chunkCount, _metaChunk, _chunk);
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

		if (_GetChunks(metaChunkList, chunkSize, chunkCount, _metaChunk,
				_chunk)) {
			return B_OK;
		}
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
	if (_GetChunks(metaChunkList, chunkSize, chunkCount, _metaChunk, _chunk)) {
		_FreeArea(area, true, flags);
		return B_OK;
	}

	_AddArea(area);
	_GetChunks(metaChunkList, chunkSize, chunkCount, _metaChunk, _chunk);
	return B_OK;
}


/*static*/ bool
MemoryManager::_GetChunks(MetaChunkList* metaChunkList, size_t chunkSize,
	uint32 chunkCount, MetaChunk*& _metaChunk, Chunk*& _chunk)
{
	// the common and less complicated special case
	if (chunkCount == 1)
		return _GetChunk(metaChunkList, chunkSize, _metaChunk, _chunk);

	ASSERT(metaChunkList != NULL);

	// Iterate through the partial meta chunk list and try to find a free
	// range that is large enough.
	MetaChunk* metaChunk = NULL;
	for (MetaChunkList::Iterator it = metaChunkList->GetIterator();
			(metaChunk = it.Next()) != NULL;) {
		if (metaChunk->firstFreeChunk + chunkCount - 1
				<= metaChunk->lastFreeChunk) {
			break;
		}
	}

	if (metaChunk == NULL) {
		// try to get a free meta chunk
		if ((SLAB_CHUNK_SIZE_LARGE - SLAB_AREA_STRUCT_OFFSET - kAreaAdminSize)
				/ chunkSize >= chunkCount) {
			metaChunk = sFreeShortMetaChunks.RemoveHead();
		}
		if (metaChunk == NULL)
			metaChunk = sFreeCompleteMetaChunks.RemoveHead();

		if (metaChunk == NULL)
			return false;

		metaChunkList->Add(metaChunk);
		metaChunk->GetArea()->usedMetaChunkCount++;
		_PrepareMetaChunk(metaChunk, chunkSize);

		T(AllocateMetaChunk(metaChunk));
	}

	// pull the chunks out of the free list
	Chunk* firstChunk = metaChunk->chunks + metaChunk->firstFreeChunk;
	Chunk* lastChunk = firstChunk + (chunkCount - 1);
	Chunk** chunkPointer = &metaChunk->freeChunks;
	uint32 remainingChunks = chunkCount;
	while (remainingChunks > 0) {
		ASSERT_PRINT(chunkPointer, "remaining: %" B_PRIu32 "/%" B_PRIu32
			", area: %p, meta chunk: %" B_PRIdSSIZE "\n", remainingChunks,
			chunkCount, metaChunk->GetArea(),
			metaChunk - metaChunk->GetArea()->metaChunks);
		Chunk* chunk = *chunkPointer;
		if (chunk >= firstChunk && chunk <= lastChunk) {
			*chunkPointer = chunk->next;
			chunk->reference = 1;
			remainingChunks--;
		} else
			chunkPointer = &chunk->next;
	}

	// allocate the chunks
	metaChunk->usedChunkCount += chunkCount;
	if (metaChunk->usedChunkCount == metaChunk->chunkCount) {
		// meta chunk is full now -- remove it from its list
		if (metaChunkList != NULL)
			metaChunkList->Remove(metaChunk);
	}

	// update the free range
	metaChunk->firstFreeChunk += chunkCount;

	PARANOID_CHECKS_ONLY(_CheckMetaChunk(metaChunk));

	_chunk = firstChunk;
	_metaChunk = metaChunk;

	T(AllocateChunks(chunkSize, chunkCount, metaChunk, firstChunk));

	return true;
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

		T(AllocateMetaChunk(metaChunk));
	}

	// allocate the chunk
	if (++metaChunk->usedChunkCount == metaChunk->chunkCount) {
		// meta chunk is full now -- remove it from its list
		if (metaChunkList != NULL)
			metaChunkList->Remove(metaChunk);
	}

	_chunk = _pop(metaChunk->freeChunks);
	_metaChunk = metaChunk;

	_chunk->reference = 1;

	// update the free range
	uint32 chunkIndex = _chunk - metaChunk->chunks;
	if (chunkIndex >= metaChunk->firstFreeChunk
			&& chunkIndex <= metaChunk->lastFreeChunk) {
		if (chunkIndex - metaChunk->firstFreeChunk
				<= metaChunk->lastFreeChunk - chunkIndex) {
			metaChunk->firstFreeChunk = chunkIndex + 1;
		} else
			metaChunk->lastFreeChunk = chunkIndex - 1;
	}

	PARANOID_CHECKS_ONLY(_CheckMetaChunk(metaChunk));

	T(AllocateChunk(chunkSize, metaChunk, _chunk));

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

	T(FreeChunk(metaChunk, chunk));

	_push(metaChunk->freeChunks, chunk);

	uint32 chunkIndex = chunk - metaChunk->chunks;

	// free the meta chunk, if it is unused now
	PARANOID_CHECKS_ONLY(bool areaDeleted = false;)
	ASSERT(metaChunk->usedChunkCount > 0);
	if (--metaChunk->usedChunkCount == 0) {
		T(FreeMetaChunk(metaChunk));

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
		if (--area->usedMetaChunkCount == 0) {
			_FreeArea(area, false, flags);
			PARANOID_CHECKS_ONLY(areaDeleted = true;)
		}
	} else if (metaChunk->usedChunkCount == metaChunk->chunkCount - 1) {
		// the meta chunk was full before -- add it back to its partial chunk
		// list
		if (metaChunk->chunkSize == SLAB_CHUNK_SIZE_SMALL)
			sPartialMetaChunksSmall.Add(metaChunk, false);
		else if (metaChunk->chunkSize == SLAB_CHUNK_SIZE_MEDIUM)
			sPartialMetaChunksMedium.Add(metaChunk, false);

		metaChunk->firstFreeChunk = chunkIndex;
		metaChunk->lastFreeChunk = chunkIndex;
	} else {
		// extend the free range, if the chunk adjoins
		if (chunkIndex + 1 == metaChunk->firstFreeChunk) {
			uint32 firstFree = chunkIndex;
			for (; firstFree > 0; firstFree--) {
				Chunk* previousChunk = &metaChunk->chunks[firstFree - 1];
				if (!_IsChunkFree(metaChunk, previousChunk))
					break;
			}
			metaChunk->firstFreeChunk = firstFree;
		} else if (chunkIndex == (uint32)metaChunk->lastFreeChunk + 1) {
			uint32 lastFree = chunkIndex;
			for (; lastFree + 1 < metaChunk->chunkCount; lastFree++) {
				Chunk* nextChunk = &metaChunk->chunks[lastFree + 1];
				if (!_IsChunkFree(metaChunk, nextChunk))
					break;
			}
			metaChunk->lastFreeChunk = lastFree;
		}
	}

	PARANOID_CHECKS_ONLY(
		if (!areaDeleted)
			_CheckMetaChunk(metaChunk);
	)
}


/*static*/ void
MemoryManager::_PrepareMetaChunk(MetaChunk* metaChunk, size_t chunkSize)
{
	Area* area = metaChunk->GetArea();

	if (metaChunk == area->metaChunks) {
		// the first chunk is shorter
		size_t unusableSize = ROUNDUP(SLAB_AREA_STRUCT_OFFSET + kAreaAdminSize,
			chunkSize);
		metaChunk->chunkBase = area->BaseAddress() + unusableSize;
		metaChunk->totalSize = SLAB_CHUNK_SIZE_LARGE - unusableSize;
	}

	metaChunk->chunkSize = chunkSize;
	metaChunk->chunkCount = metaChunk->totalSize / chunkSize;
	metaChunk->usedChunkCount = 0;

	metaChunk->freeChunks = NULL;
	for (int32 i = metaChunk->chunkCount - 1; i >= 0; i--)
		_push(metaChunk->freeChunks, metaChunk->chunks + i);

	metaChunk->firstFreeChunk = 0;
	metaChunk->lastFreeChunk = metaChunk->chunkCount - 1;

	PARANOID_CHECKS_ONLY(_CheckMetaChunk(metaChunk));
}


/*static*/ void
MemoryManager::_AddArea(Area* area)
{
	T(AddArea(area));

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
	void* areaBase;
	Area* area;
	VMArea* vmArea = NULL;

	if (sKernelArgs == NULL) {
		// create an area
		uint32 areaCreationFlags = (flags & CACHE_PRIORITY_VIP) != 0
			? CREATE_AREA_PRIORITY_VIP : 0;
		area_id areaID = vm_create_null_area(B_SYSTEM_TEAM, kSlabAreaName,
			&areaBase, B_ANY_KERNEL_BLOCK_ADDRESS, SLAB_AREA_SIZE,
			areaCreationFlags);
		if (areaID < 0) {
			mutex_lock(&sLock);
			return areaID;
		}

		area = _AreaForAddress((addr_t)areaBase);

		// map the memory for the administrative structure
		VMAddressSpace* addressSpace = VMAddressSpace::Kernel();
		VMTranslationMap* translationMap = addressSpace->TranslationMap();

		pagesNeededToMap = translationMap->MaxPagesNeededToMap(
			(addr_t)area, (addr_t)areaBase + SLAB_AREA_SIZE - 1);

		vmArea = VMAreaHash::Lookup(areaID);
		status_t error = _MapChunk(vmArea, (addr_t)area, kAreaAdminSize,
			pagesNeededToMap, flags);
		if (error != B_OK) {
			delete_area(areaID);
			mutex_lock(&sLock);
			return error;
		}

		dprintf("slab memory manager: created area %p (%" B_PRId32 ")\n", area,
			areaID);
	} else {
		// no areas yet -- allocate raw memory
		areaBase = (void*)vm_allocate_early(sKernelArgs, SLAB_AREA_SIZE,
			SLAB_AREA_SIZE, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
			SLAB_AREA_SIZE);
		if (areaBase == NULL) {
			mutex_lock(&sLock);
			return B_NO_MEMORY;
		}
		area = _AreaForAddress((addr_t)areaBase);

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
		metaChunk->chunkBase = (addr_t)areaBase + i * SLAB_CHUNK_SIZE_LARGE;
		metaChunk->totalSize = SLAB_CHUNK_SIZE_LARGE;
			// Note: chunkBase and totalSize aren't correct for the first
			// meta chunk. They will be set in _PrepareMetaChunk().
		metaChunk->chunkCount = 0;
		metaChunk->usedChunkCount = 0;
		metaChunk->freeChunks = NULL;
	}

	mutex_lock(&sLock);
	_area = area;

	T(AllocateArea(area, flags));

	return B_OK;
}


/*static*/ void
MemoryManager::_FreeArea(Area* area, bool areaRemoved, uint32 flags)
{
	TRACE("MemoryManager::_FreeArea(%p, %#" B_PRIx32 ")\n", area, flags);

	T(FreeArea(area, areaRemoved, flags));

	ASSERT(area->usedMetaChunkCount == 0);

	if (!areaRemoved) {
		// remove the area's meta chunks from the free lists
		ASSERT(area->metaChunks[0].usedChunkCount == 0);
		sFreeShortMetaChunks.Remove(&area->metaChunks[0]);

		for (int32 i = 1; i < SLAB_META_CHUNKS_PER_AREA; i++) {
			ASSERT(area->metaChunks[i].usedChunkCount == 0);
			sFreeCompleteMetaChunks.Remove(&area->metaChunks[i]);
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

	dprintf("slab memory manager: deleting area %p (%" B_PRId32 ")\n", area,
		area->vmArea->id);

	size_t memoryToUnreserve = area->reserved_memory_for_mapping;
	delete_area(area->vmArea->id);
	vm_unreserve_memory(memoryToUnreserve);

	mutex_lock(&sLock);
}


/*static*/ status_t
MemoryManager::_MapChunk(VMArea* vmArea, addr_t address, size_t size,
	size_t reserveAdditionalMemory, uint32 flags)
{
	TRACE("MemoryManager::_MapChunk(%p, %#" B_PRIxADDR ", %#" B_PRIxSIZE
		")\n", vmArea, address, size);

	T(Map(address, size, flags));

	if (vmArea == NULL) {
		// everything is mapped anyway
		return B_OK;
	}

	VMAddressSpace* addressSpace = VMAddressSpace::Kernel();
	VMTranslationMap* translationMap = addressSpace->TranslationMap();

	// reserve memory for the chunk
	int priority = (flags & CACHE_PRIORITY_VIP) != 0
		? VM_PRIORITY_VIP : VM_PRIORITY_SYSTEM;
	size_t reservedMemory = size + reserveAdditionalMemory;
	status_t error = vm_try_reserve_memory(size, priority,
		(flags & CACHE_DONT_WAIT_FOR_MEMORY) != 0 ? 0 : 1000000);
	if (error != B_OK)
		return error;

	// reserve the pages we need now
	size_t reservedPages = size / B_PAGE_SIZE
		+ translationMap->MaxPagesNeededToMap(address, address + size - 1);
	vm_page_reservation reservation;
	if ((flags & CACHE_DONT_WAIT_FOR_MEMORY) != 0) {
		if (!vm_page_try_reserve_pages(&reservation, reservedPages, priority)) {
			vm_unreserve_memory(reservedMemory);
			return B_WOULD_BLOCK;
		}
	} else
		vm_page_reserve_pages(&reservation, reservedPages, priority);

	VMCache* cache = vm_area_get_locked_cache(vmArea);

	// map the pages
	translationMap->Lock();

	addr_t areaOffset = address - vmArea->Base();
	addr_t endAreaOffset = areaOffset + size;
	for (size_t offset = areaOffset; offset < endAreaOffset;
			offset += B_PAGE_SIZE) {
		vm_page* page = vm_page_allocate_page(&reservation, PAGE_STATE_WIRED);
		cache->InsertPage(page, offset);

		page->IncrementWiredCount();
		atomic_add(&gMappedPagesCount, 1);
		DEBUG_PAGE_ACCESS_END(page);

		translationMap->Map(vmArea->Base() + offset,
			page->physical_page_number * B_PAGE_SIZE,
			B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
			vmArea->MemoryType(), &reservation);
	}

	translationMap->Unlock();

	cache->ReleaseRefAndUnlock();

	vm_page_unreserve_pages(&reservation);

	return B_OK;
}


/*static*/ status_t
MemoryManager::_UnmapChunk(VMArea* vmArea, addr_t address, size_t size,
	uint32 flags)
{
	T(Unmap(address, size, flags));

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

		page->DecrementWiredCount();

		cache->RemovePage(page);
			// the iterator is remove-safe
		vm_page_free(cache, page);
	}

	cache->ReleaseRefAndUnlock();

	vm_unreserve_memory(size);

	return B_OK;
}


/*static*/ void
MemoryManager::_UnmapFreeChunksEarly(Area* area)
{
	if (!area->fullyMapped)
		return;

	TRACE("MemoryManager::_UnmapFreeChunksEarly(%p)\n", area);

	// unmap the space before the Area structure
	#if SLAB_AREA_STRUCT_OFFSET > 0
		_UnmapChunk(area->vmArea, area->BaseAddress(), SLAB_AREA_STRUCT_OFFSET,
			0);
	#endif

	for (int32 i = 0; i < SLAB_META_CHUNKS_PER_AREA; i++) {
		MetaChunk* metaChunk = area->metaChunks + i;
		if (metaChunk->chunkSize == 0) {
			// meta chunk is free -- unmap it completely
			if (i == 0) {
				_UnmapChunk(area->vmArea, (addr_t)area + kAreaAdminSize,
					SLAB_CHUNK_SIZE_LARGE - kAreaAdminSize, 0);
			} else {
				_UnmapChunk(area->vmArea,
					area->BaseAddress() + i * SLAB_CHUNK_SIZE_LARGE,
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
	void* address = (void*)area->BaseAddress();
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


/*static*/ bool
MemoryManager::_IsChunkInFreeList(const MetaChunk* metaChunk,
	const Chunk* chunk)
{
	Chunk* freeChunk = metaChunk->freeChunks;
	while (freeChunk != NULL) {
		if (freeChunk == chunk)
			return true;
		freeChunk = freeChunk->next;
	}

	return false;
}


#if DEBUG_SLAB_MEMORY_MANAGER_PARANOID_CHECKS

/*static*/ void
MemoryManager::_CheckMetaChunk(MetaChunk* metaChunk)
{
	Area* area = metaChunk->GetArea();
	int32 metaChunkIndex = metaChunk - area->metaChunks;
	if (metaChunkIndex < 0 || metaChunkIndex >= SLAB_META_CHUNKS_PER_AREA) {
		panic("invalid meta chunk %p!", metaChunk);
		return;
	}

	switch (metaChunk->chunkSize) {
		case 0:
			// unused
			return;
		case SLAB_CHUNK_SIZE_SMALL:
		case SLAB_CHUNK_SIZE_MEDIUM:
		case SLAB_CHUNK_SIZE_LARGE:
			break;
		default:
			panic("meta chunk %p has invalid chunk size: %" B_PRIuSIZE,
				metaChunk, metaChunk->chunkSize);
			return;
	}

	if (metaChunk->totalSize > SLAB_CHUNK_SIZE_LARGE) {
		panic("meta chunk %p has invalid total size: %" B_PRIuSIZE,
			metaChunk, metaChunk->totalSize);
		return;
	}

	addr_t expectedBase = area->BaseAddress()
		+ metaChunkIndex * SLAB_CHUNK_SIZE_LARGE;
	if (metaChunk->chunkBase < expectedBase
		|| metaChunk->chunkBase - expectedBase + metaChunk->totalSize
			> SLAB_CHUNK_SIZE_LARGE) {
		panic("meta chunk %p has invalid base address: %" B_PRIxADDR, metaChunk,
			metaChunk->chunkBase);
		return;
	}

	if (metaChunk->chunkCount != metaChunk->totalSize / metaChunk->chunkSize) {
		panic("meta chunk %p has invalid chunk count: %u", metaChunk,
			metaChunk->chunkCount);
		return;
	}

	if (metaChunk->usedChunkCount > metaChunk->chunkCount) {
		panic("meta chunk %p has invalid unused chunk count: %u", metaChunk,
			metaChunk->usedChunkCount);
		return;
	}

	if (metaChunk->firstFreeChunk > metaChunk->chunkCount) {
		panic("meta chunk %p has invalid first free chunk: %u", metaChunk,
			metaChunk->firstFreeChunk);
		return;
	}

	if (metaChunk->lastFreeChunk >= metaChunk->chunkCount) {
		panic("meta chunk %p has invalid last free chunk: %u", metaChunk,
			metaChunk->lastFreeChunk);
		return;
	}

	// check free list for structural sanity
	uint32 freeChunks = 0;
	for (Chunk* chunk = metaChunk->freeChunks; chunk != NULL;
			chunk = chunk->next) {
		if ((addr_t)chunk % sizeof(Chunk) != 0 || chunk < metaChunk->chunks
			|| chunk >= metaChunk->chunks + metaChunk->chunkCount) {
			panic("meta chunk %p has invalid element in free list, chunk: %p",
				metaChunk, chunk);
			return;
		}

		if (++freeChunks > metaChunk->chunkCount) {
			panic("meta chunk %p has cyclic free list", metaChunk);
			return;
		}
	}

	if (freeChunks + metaChunk->usedChunkCount > metaChunk->chunkCount) {
		panic("meta chunk %p has mismatching free/used chunk counts: total: "
			"%u, used: %u, free: %" B_PRIu32, metaChunk, metaChunk->chunkCount,
			metaChunk->usedChunkCount, freeChunks);
		return;
	}

	// count used chunks by looking at their reference/next field
	uint32 usedChunks = 0;
	for (uint32 i = 0; i < metaChunk->chunkCount; i++) {
		if (!_IsChunkFree(metaChunk, metaChunk->chunks + i))
			usedChunks++;
	}

	if (usedChunks != metaChunk->usedChunkCount) {
		panic("meta chunk %p has used chunks that appear free: total: "
			"%u, used: %u, appearing used: %" B_PRIu32, metaChunk,
			metaChunk->chunkCount, metaChunk->usedChunkCount, usedChunks);
		return;
	}

	// check free range
	for (uint32 i = metaChunk->firstFreeChunk; i < metaChunk->lastFreeChunk;
			i++) {
		if (!_IsChunkFree(metaChunk, metaChunk->chunks + i)) {
			panic("meta chunk %p has used chunk in free range, chunk: %p (%"
				B_PRIu32 ", free range: %u - %u)", metaChunk,
				metaChunk->chunks + i, i, metaChunk->firstFreeChunk,
				metaChunk->lastFreeChunk);
			return;
		}
	}
}

#endif	// DEBUG_SLAB_MEMORY_MANAGER_PARANOID_CHECKS


/*static*/ int
MemoryManager::_DumpRawAllocations(int argc, char** argv)
{
	kprintf("area        meta chunk  chunk  base        size (KB)\n");

	size_t totalSize = 0;

	for (AreaTable::Iterator it = sAreaTable.GetIterator();
			Area* area = it.Next();) {
		for (int32 i = 0; i < SLAB_META_CHUNKS_PER_AREA; i++) {
			MetaChunk* metaChunk = area->metaChunks + i;
			if (metaChunk->chunkSize == 0)
				continue;
			for (uint32 k = 0; k < metaChunk->chunkCount; k++) {
				Chunk* chunk = metaChunk->chunks + k;

				// skip free chunks
				if (_IsChunkFree(metaChunk, chunk))
					continue;

				addr_t reference = chunk->reference;
				if ((reference & 1) == 0 || reference == 1)
					continue;

				addr_t chunkAddress = _ChunkAddress(metaChunk, chunk);
				size_t size = reference - chunkAddress + 1;
				totalSize += size;

				kprintf("%p  %10" B_PRId32 "  %5" B_PRIu32 "  %p  %9"
					B_PRIuSIZE "\n", area, i, k, (void*)chunkAddress,
					size / 1024);
			}
		}
	}

	kprintf("total:                                     %9" B_PRIuSIZE "\n",
		totalSize / 1024);

	return 0;
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
		kprintf(": %4u/%4u used, %-4u-%4u free ------------\n",
			metaChunk->usedChunkCount, metaChunk->chunkCount,
			metaChunk->firstFreeChunk, metaChunk->lastFreeChunk);
	} else
		kprintf(" --------------------------------------------\n");

	if (metaChunk->chunkSize == 0 || !printChunks)
		return;

	for (uint32 i = 0; i < metaChunk->chunkCount; i++) {
		Chunk* chunk = metaChunk->chunks + i;

		// skip free chunks
		if (_IsChunkFree(metaChunk, chunk)) {
			if (!_IsChunkInFreeList(metaChunk, chunk)) {
				kprintf("%5" B_PRIu32 "  %p  appears free, but isn't in free "
					"list!\n", i, (void*)_ChunkAddress(metaChunk, chunk));
			}

			continue;
		}

		addr_t reference = chunk->reference;
		if ((reference & 1) == 0) {
			ObjectCache* cache = (ObjectCache*)reference;
			kprintf("%5" B_PRIu32 "  %p  %p  %11" B_PRIuSIZE "  %s\n", i,
				(void*)_ChunkAddress(metaChunk, chunk), cache,
				cache != NULL ? cache->object_size : 0,
				cache != NULL ? cache->name : "");
		} else if (reference != 1) {
			kprintf("%5" B_PRIu32 "  %p  raw allocation up to %p\n", i,
				(void*)_ChunkAddress(metaChunk, chunk), (void*)reference);
		}
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

	Area* area = _AreaForAddress(address);

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

	Area* area = _AreaForAddress((addr_t)address);

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

	size_t totalTotalSmall = 0;
	size_t totalUsedSmall = 0;
	size_t totalTotalMedium = 0;
	size_t totalUsedMedium = 0;
	size_t totalUsedLarge = 0;
	uint32 areaCount = 0;

	for (AreaTable::Iterator it = sAreaTable.GetIterator();
			Area* area = it.Next();) {
		areaCount++;

		// sum up the free/used counts for the chunk sizes
		int totalSmall = 0;
		int usedSmall = 0;
		int totalMedium = 0;
		int usedMedium = 0;
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
					usedLarge += metaChunk->usedChunkCount;
					break;
			}
		}

		kprintf("%p  %p  %2u/%2u  %4d/%4d  %3d/%3d  %5d\n",
			area, area->vmArea, area->usedMetaChunkCount,
			SLAB_META_CHUNKS_PER_AREA, usedSmall, totalSmall, usedMedium,
			totalMedium, usedLarge);

		totalTotalSmall += totalSmall;
		totalUsedSmall += usedSmall;
		totalTotalMedium += totalMedium;
		totalUsedMedium += usedMedium;
		totalUsedLarge += usedLarge;
	}

	kprintf("%d free area%s:\n", sFreeAreaCount,
		sFreeAreaCount == 1 ? "" : "s");
	for (Area* area = sFreeAreas; area != NULL; area = area->next) {
		areaCount++;
		kprintf("%p  %p\n", area, area->vmArea);
	}

	kprintf("total usage:\n");
	kprintf("  small:    %" B_PRIuSIZE "/%" B_PRIuSIZE "\n", totalUsedSmall,
		totalTotalSmall);
	kprintf("  medium:   %" B_PRIuSIZE "/%" B_PRIuSIZE "\n", totalUsedMedium,
		totalTotalMedium);
	kprintf("  large:    %" B_PRIuSIZE "\n", totalUsedLarge);
	kprintf("  memory:   %" B_PRIuSIZE "/%" B_PRIuSIZE " KB\n",
		(totalUsedSmall * SLAB_CHUNK_SIZE_SMALL
			+ totalUsedMedium * SLAB_CHUNK_SIZE_MEDIUM
			+ totalUsedLarge * SLAB_CHUNK_SIZE_LARGE) / 1024,
		areaCount * SLAB_AREA_SIZE / 1024);
	kprintf("  overhead: %" B_PRIuSIZE " KB\n",
		areaCount * kAreaAdminSize / 1024);

	return 0;
}
