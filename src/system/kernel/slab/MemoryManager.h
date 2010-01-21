/*
 * Copyright 2010, Ingo Weinhold <ingo_weinhold@gmx.de>.
 * Distributed under the terms of the MIT License.
 */
#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H


#include <KernelExport.h>

#include <condition_variable.h>
#include <lock.h>
#include <util/DoublyLinkedList.h>
#include <util/OpenHashTable.h>


struct kernel_args;
struct ObjectCache;
struct VMArea;


#define SLAB_CHUNK_SIZE_SMALL	B_PAGE_SIZE
#define SLAB_CHUNK_SIZE_MIDDLE	16 * B_PAGE_SIZE
#define SLAB_CHUNK_SIZE_LARGE	128 * B_PAGE_SIZE
#define SLAB_AREA_SIZE			2048 * B_PAGE_SIZE
	// TODO: These sizes have been chosen with 4 KB pages is mind.


class MemoryManager {
public:
	static	void				Init(kernel_args* args);
	static	void				InitPostArea();

	static	status_t			Allocate(ObjectCache* cache, uint32 flags,
									void*& _pages);
	static	void				Free(void* pages, uint32 flags);

	static	size_t				AcceptableChunkSize(size_t size);
	static	ObjectCache*		CacheForAddress(void* address);

private:
			struct Chunk {
				union {
					Chunk*		next;
					ObjectCache* cache;
				};
			};

			struct Area : DoublyLinkedListLinkImpl<Area> {
				Area*			next;
				VMArea*			vmArea;
				size_t			chunkSize;
				size_t			reserved_memory_for_mapping;
				addr_t			firstUsableChunk;
				uint16			chunkCount;
				uint16			usedChunkCount;
				Chunk*			mappedFreeChunks;
				Chunk*			unmappedFreeChunks;
				Chunk			chunks[0];
			};

			typedef DoublyLinkedList<Area> AreaList;

			struct AreaPool {
				AreaList		partialAreas;
				AreaList		fullAreas;
				size_t			chunkSize;
			};

			struct AreaHashDefinition {
				typedef addr_t		KeyType;
				typedef	Area		ValueType;

				size_t HashKey(addr_t key) const
				{
					return key / SLAB_AREA_SIZE;
				}

				size_t Hash(const Area* value) const
				{
					return HashKey((addr_t)value);
				}

				bool Compare(addr_t key, const Area* value) const
				{
					return key == (addr_t)value;
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
	static	AreaPool*			_AreaPoolFor(size_t chunkSize);

	static	status_t			_GetPartialArea(AreaPool* areaPool,
									uint32 flags, Area*& _area);

	static	status_t			_AllocateArea(size_t chunkSize, uint32 flags,
									Area*& _area);
	static	void				_FreeArea(Area* area, uint32 flags);

	static	void				_FreeChunk(AreaPool* areaPool, Area* area,
									Chunk* chunk, addr_t chunkAddress,
									bool alreadyUnmapped, uint32 flags);
	static	status_t			_MapChunk(VMArea* vmArea, addr_t address,
									size_t size, size_t reserveAdditionalMemory,
									uint32 flags);
	static	status_t			_UnmapChunk(VMArea* vmArea,addr_t address,
									size_t size, uint32 flags);

	static	bool				_ConvertEarlyAreas(AreaList& areas);

	static	uint32				_ChunkIndexForAddress(const Area* area,
									addr_t address);
	static	addr_t				_ChunkAddress(const Area* area,
									const Chunk* chunk);

	static	int					_DumpArea(int argc, char** argv);
	static	int					_DumpAreas(int argc, char** argv);

private:
	static	mutex				sLock;
	static	rw_lock				sAreaTableLock;
	static	kernel_args*		sKernelArgs;
	static	AreaPool			sSmallChunkAreas;
	static	AreaPool			sMiddleChunkAreas;
	static	AreaPool			sLargeChunkAreas;
	static	AreaTable			sAreaTable;
	static	Area*				sFreeAreas;
	static	AllocationEntry*	sAllocationEntryCanWait;
	static	AllocationEntry*	sAllocationEntryDontWait;
};


/*static*/ inline uint32
MemoryManager::_ChunkIndexForAddress(const Area* area, addr_t address)
{
	return (address - area->firstUsableChunk) / area->chunkSize;
}


/*static*/ inline addr_t
MemoryManager::_ChunkAddress(const Area* area, const Chunk* chunk)
{
	return area->firstUsableChunk + (chunk - area->chunks) * area->chunkSize;
}


#endif	// MEMORY_MANAGER_H
