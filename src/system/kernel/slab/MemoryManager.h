/*
 * Copyright 2010, Ingo Weinhold <ingo_weinhold@gmx.de>.
 * Distributed under the terms of the MIT License.
 */
#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H


#include <KernelExport.h>

#include <condition_variable.h>
#include <kernel.h>
#include <lock.h>
#include <util/DoublyLinkedList.h>
#include <util/OpenHashTable.h>

#include "slab_debug.h"


class AbstractTraceEntryWithStackTrace;
struct kernel_args;
struct ObjectCache;
struct VMArea;


#define SLAB_CHUNK_SIZE_SMALL	B_PAGE_SIZE
#define SLAB_CHUNK_SIZE_MEDIUM	(16 * B_PAGE_SIZE)
#define SLAB_CHUNK_SIZE_LARGE	(128 * B_PAGE_SIZE)
#define SLAB_AREA_SIZE			(2048 * B_PAGE_SIZE)
	// TODO: These sizes have been chosen with 4 KB pages in mind.
#define SLAB_AREA_STRUCT_OFFSET	B_PAGE_SIZE
	// The offset from the start of the area to the Area structure. This space
	// is not mapped and will trip code writing beyond the previous area's
	// bounds.

#define SLAB_META_CHUNKS_PER_AREA	(SLAB_AREA_SIZE / SLAB_CHUNK_SIZE_LARGE)
#define SLAB_SMALL_CHUNKS_PER_META_CHUNK	\
	(SLAB_CHUNK_SIZE_LARGE / SLAB_CHUNK_SIZE_SMALL)


class MemoryManager {
public:
	static	void				Init(kernel_args* args);
	static	void				InitPostArea();

	static	status_t			Allocate(ObjectCache* cache, uint32 flags,
									void*& _pages);
	static	void				Free(void* pages, uint32 flags);

	static	status_t			AllocateRaw(size_t size, uint32 flags,
									void*& _pages);
	static	ObjectCache*		FreeRawOrReturnCache(void* pages,
									uint32 flags);

	static	size_t				AcceptableChunkSize(size_t size);
	static	ObjectCache*		GetAllocationInfo(void* address,
									size_t& _size);
	static	ObjectCache*		CacheForAddress(void* address);

	static	bool				MaintenanceNeeded();
	static	void				PerformMaintenance();

#if SLAB_MEMORY_MANAGER_ALLOCATION_TRACKING
	static	bool				AnalyzeAllocationCallers(
									AllocationTrackingCallback& callback);
#endif

	static	ObjectCache*		DebugObjectCacheForAddress(void* address);

private:
			struct Tracing;

			struct Area;

			struct Chunk {
				union {
					Chunk*		next;
					addr_t		reference;
				};
			};

			struct MetaChunk : DoublyLinkedListLinkImpl<MetaChunk> {
				size_t			chunkSize;
				addr_t			chunkBase;
				size_t			totalSize;
				uint16			chunkCount;
				uint16			usedChunkCount;
				uint16			firstFreeChunk;	// *some* free range
				uint16			lastFreeChunk;	// inclusive
				Chunk			chunks[SLAB_SMALL_CHUNKS_PER_META_CHUNK];
				Chunk*			freeChunks;

				Area*			GetArea() const;
			};

			friend class MetaChunk;
			typedef DoublyLinkedList<MetaChunk> MetaChunkList;

			struct Area : DoublyLinkedListLinkImpl<Area> {
				Area*			next;
				VMArea*			vmArea;
				size_t			reserved_memory_for_mapping;
				uint16			usedMetaChunkCount;
				bool			fullyMapped;
				MetaChunk		metaChunks[SLAB_META_CHUNKS_PER_AREA];

				addr_t BaseAddress() const
				{
					return (addr_t)this - SLAB_AREA_STRUCT_OFFSET;
				}
			};

			typedef DoublyLinkedList<Area> AreaList;

			struct AreaHashDefinition {
				typedef addr_t		KeyType;
				typedef	Area		ValueType;

				size_t HashKey(addr_t key) const
				{
					return key / SLAB_AREA_SIZE;
				}

				size_t Hash(const Area* value) const
				{
					return HashKey(value->BaseAddress());
				}

				bool Compare(addr_t key, const Area* value) const
				{
					return key == value->BaseAddress();
				}

				Area*& GetLink(Area* value) const
				{
					return value->next;
				}
			};

			typedef BOpenHashTable<AreaHashDefinition> AreaTable;

			struct AllocationEntry {
				ConditionVariable	condition;
				thread_id			thread;
			};

private:
	static	status_t			_AllocateChunks(size_t chunkSize,
									uint32 chunkCount, uint32 flags,
									MetaChunk*& _metaChunk, Chunk*& _chunk);
	static	bool				_GetChunks(MetaChunkList* metaChunkList,
									size_t chunkSize, uint32 chunkCount,
									MetaChunk*& _metaChunk, Chunk*& _chunk);
	static	bool				_GetChunk(MetaChunkList* metaChunkList,
									size_t chunkSize, MetaChunk*& _metaChunk,
									Chunk*& _chunk);
	static	void				_FreeChunk(Area* area, MetaChunk* metaChunk,
									Chunk* chunk, addr_t chunkAddress,
									bool alreadyUnmapped, uint32 flags);

	static	void				_PrepareMetaChunk(MetaChunk* metaChunk,
									size_t chunkSize);

	static	void				_AddArea(Area* area);
	static	status_t			_AllocateArea(uint32 flags, Area*& _area);
	static	void				_FreeArea(Area* area, bool areaRemoved,
									uint32 flags);

	static	status_t			_MapChunk(VMArea* vmArea, addr_t address,
									size_t size, size_t reserveAdditionalMemory,
									uint32 flags);
	static	status_t			_UnmapChunk(VMArea* vmArea, addr_t address,
									size_t size, uint32 flags);

	static	void				_UnmapFreeChunksEarly(Area* area);
	static	void				_ConvertEarlyArea(Area* area);

	static	void				_RequestMaintenance();

	static	addr_t				_AreaBaseAddressForAddress(addr_t address);
	static	Area*				_AreaForAddress(addr_t address);
	static	uint32				_ChunkIndexForAddress(
									const MetaChunk* metaChunk, addr_t address);
	static	addr_t				_ChunkAddress(const MetaChunk* metaChunk,
									const Chunk* chunk);
	static	bool				_IsChunkFree(const MetaChunk* metaChunk,
									const Chunk* chunk);
	static	bool				_IsChunkInFreeList(const MetaChunk* metaChunk,
									const Chunk* chunk);
	static	void				_CheckMetaChunk(MetaChunk* metaChunk);

	static	int					_DumpRawAllocations(int argc, char** argv);
	static	void				_PrintMetaChunkTableHeader(bool printChunks);
	static	void				_DumpMetaChunk(MetaChunk* metaChunk,
									bool printChunks, bool printHeader);
	static	int					_DumpMetaChunk(int argc, char** argv);
	static	void				_DumpMetaChunks(const char* name,
									MetaChunkList& metaChunkList,
									bool printChunks);
	static	int					_DumpMetaChunks(int argc, char** argv);
	static	int					_DumpArea(int argc, char** argv);
	static	int					_DumpAreas(int argc, char** argv);

#if SLAB_MEMORY_MANAGER_ALLOCATION_TRACKING
	static	void				_AddTrackingInfo(void* allocation, size_t size,
									AbstractTraceEntryWithStackTrace* entry);
	static	AllocationTrackingInfo* _TrackingInfoFor(void* allocation,
									size_t size);
#endif

private:
	static	const size_t		kAreaAdminSize
									= ROUNDUP(sizeof(Area), B_PAGE_SIZE);

	static	mutex				sLock;
	static	rw_lock				sAreaTableLock;
	static	kernel_args*		sKernelArgs;
	static	AreaTable			sAreaTable;
	static	Area*				sFreeAreas;
	static	int					sFreeAreaCount;
	static	MetaChunkList		sFreeCompleteMetaChunks;
	static	MetaChunkList		sFreeShortMetaChunks;
	static	MetaChunkList		sPartialMetaChunksSmall;
	static	MetaChunkList		sPartialMetaChunksMedium;
	static	AllocationEntry*	sAllocationEntryCanWait;
	static	AllocationEntry*	sAllocationEntryDontWait;
	static	bool				sMaintenanceNeeded;
};


/*static*/ inline bool
MemoryManager::MaintenanceNeeded()
{
	return sMaintenanceNeeded;
}


/*static*/ inline addr_t
MemoryManager::_AreaBaseAddressForAddress(addr_t address)
{
	return ROUNDDOWN((addr_t)address, SLAB_AREA_SIZE);
}


/*static*/ inline MemoryManager::Area*
MemoryManager::_AreaForAddress(addr_t address)
{
	return (Area*)(_AreaBaseAddressForAddress(address)
		+ SLAB_AREA_STRUCT_OFFSET);
}


/*static*/ inline uint32
MemoryManager::_ChunkIndexForAddress(const MetaChunk* metaChunk, addr_t address)
{
	return (address - metaChunk->chunkBase) / metaChunk->chunkSize;
}


/*static*/ inline addr_t
MemoryManager::_ChunkAddress(const MetaChunk* metaChunk, const Chunk* chunk)
{
	return metaChunk->chunkBase
		+ (chunk - metaChunk->chunks) * metaChunk->chunkSize;
}


/*static*/ inline bool
MemoryManager::_IsChunkFree(const MetaChunk* metaChunk, const Chunk* chunk)
{
	return chunk->next == NULL
		|| (chunk->next >= metaChunk->chunks
			&& chunk->next < metaChunk->chunks + metaChunk->chunkCount);
}


inline MemoryManager::Area*
MemoryManager::MetaChunk::GetArea() const
{
	return _AreaForAddress((addr_t)this);
}


#if SLAB_MEMORY_MANAGER_ALLOCATION_TRACKING

/*static*/ inline AllocationTrackingInfo*
MemoryManager::_TrackingInfoFor(void* allocation, size_t size)
{
	return (AllocationTrackingInfo*)((uint8*)allocation + size
		- sizeof(AllocationTrackingInfo));
}

#endif	// SLAB_MEMORY_MANAGER_ALLOCATION_TRACKING


#endif	// MEMORY_MANAGER_H
