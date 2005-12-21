/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "block_cache_private.h"

#include <KernelExport.h>

#include <util/AutoLock.h>
//#include <vm.h>
#include <vm_address_space.h>
#include <vm_page.h>

#include <string.h>


//#define TRACE_BLOCK_ALLOCATOR
#ifdef TRACE_BLOCK_ALLOCATOR
#	define TRACE(x)	dprintf x
#else
#	define TRACE(x) ;
#endif


/**	The BlockAddressPool class manages a block of virtual memory address
 *	ranges.
 *	The actual block of kBlockAddressSize bytes, currently 128 MB (as defined
 *	in block_cache_private.h) is divided into kBlockRangeSize or 64 kB
 *	ranges.
 *	You can ask for a free range, and when you're done, you return it into
 *	the pool. You usually only do that when the block cache owning the
 *	ranges is deleted, or if memory becomes tight, a new volume is mounted
 *	or whatever.
 *
 *	The block_range class on the other side manages each range. Only allocated
 *	ranges have a block_range object.
 *	In the block cache, the ranges are put into a hash table - they are
 *	accessed within that hash by address. That way, a cached_block allocated
 *	in one range doesn't need to know its range.
 *
 *	If you actually need physical memory, you must allocate it using the
 *	block_range Allocate() methods.
 *
 *	The block_range is further divided into block_chunks. A block_chunk is
 *	a multiple of the page size - for block sizes below the page size, it's
 *	just one page. Pages for the block range are actually allocated and
 *	mapped per chunk, and not for the whole range.
 *
 *	The block_range only exists to avoid fragmentation of the virtual memory
 *	region reserved for the block cache, since all allocations have the same
 *	size there can't be any fragmentation at all.
 *
 *	NOTE: This code is a bit complicated, and maybe a slab allocator would have been
 *	the better choice. What this allows for (and what would not be easily possible
 *	with a slab allocator) is to keep blocks in physical memory, but don't
 *	have them mapped. This, of course, would only be important if you have
 *	more memory than address space available - which sounds like a good idea
 *	now, but with 64-bit address spaces it looks a bit different :-)
 */

class BlockAddressPool {
	public:
		BlockAddressPool();
		~BlockAddressPool();

		status_t InitCheck() const { return fArea >= B_OK ? B_OK : fArea; }

		size_t RangeSize() const { return kBlockRangeSize; }
		size_t RangeShift() const { return kBlockRangeShift; }
		addr_t BaseAddress() const { return fBase; }

		addr_t Get();
		void Put(addr_t address);

	private:
		benaphore	fLock;
		area_id		fArea;
		addr_t		fBase;
		addr_t		fFirstFree;
		int32		fNextFree;
		int32		fFreeList[kBlockAddressSize / kBlockRangeSize];
};


static class BlockAddressPool sBlockAddressPool;


BlockAddressPool::BlockAddressPool()
{
	benaphore_init(&fLock, "block address pool");

	fBase = 0xa0000000;
		// directly after the I/O space area
	fArea = vm_create_null_area(vm_kernel_address_space_id(), "block cache",
		(void **)&fBase, B_BASE_ADDRESS, kBlockAddressSize);

	fFirstFree = fBase;
	fNextFree = -1;
}


BlockAddressPool::~BlockAddressPool()
{
	delete_area(fArea);
}


addr_t
BlockAddressPool::Get()
{
	BenaphoreLocker locker(&fLock);

	// ToDo: we must make sure that a single volume will never eat
	//	all available ranges! Every mounted volume should have a
	//	guaranteed minimum available for its own use.

	addr_t address = fFirstFree;
	if (address != NULL) {
		// Blocks are first taken from the initial free chunk, and
		// when that is empty, from the free list.
		// This saves us the initialization of the free list array,
		// dunno if it's such a good idea, though.
		if ((fFirstFree += RangeSize()) >= fBase + kBlockAddressSize)
			fFirstFree = NULL;

		return address;
	}

	if (fNextFree == -1)
		return NULL;

	address = (fNextFree << RangeShift()) + fBase;
	fNextFree = fFreeList[fNextFree];

	return address;
}


void
BlockAddressPool::Put(addr_t address)
{
	BenaphoreLocker locker(&fLock);

	int32 index = (address - fBase) >> RangeShift();
	fFreeList[index] = fNextFree;
	fNextFree = index;
}


//	#pragma mark -


/* static */
int
block_range::Compare(void *_blockRange, const void *_address)
{
	block_range *range = (block_range *)_blockRange;
	addr_t address = (addr_t)_address;

	return ((range->base - sBlockAddressPool.BaseAddress()) >> sBlockAddressPool.RangeShift())
		- ((address - sBlockAddressPool.BaseAddress()) >> sBlockAddressPool.RangeShift());
}


/* static */
uint32
block_range::Hash(void *_blockRange, const void *_address, uint32 range)
{
	block_range *blockRange = (block_range *)_blockRange;
	addr_t address = (addr_t)_address;

	if (blockRange != NULL) {
		return ((blockRange->base - sBlockAddressPool.BaseAddress())
			>> sBlockAddressPool.RangeShift()) % range;
	}

	return ((address - sBlockAddressPool.BaseAddress())
		>> sBlockAddressPool.RangeShift()) % range;
}


/* static */
status_t
block_range::New(block_cache *cache, block_range **_range)
{
	addr_t address = sBlockAddressPool.Get();
	if (address == NULL)
		return B_ENTRY_NOT_FOUND;

	block_range *range = (block_range *)malloc(sizeof(block_range)
		+ cache->chunks_per_range * sizeof(block_chunk));
	if (range == NULL) {
		sBlockAddressPool.Put(address);
		return B_NO_MEMORY;
	}

	TRACE(("new block range %p, base = %p!\n", range, (void *)address));
	memset(range, 0, sizeof(block_range) + cache->chunks_per_range * sizeof(block_chunk));
	range->base = address;

	// insert into free ranges list in cache
	range->free_next = cache->free_ranges;
	cache->free_ranges = range;

	*_range = range;
	return B_OK;
}


/*static*/
void
block_range::Delete(block_cache *cache, block_range *range)
{
	TRACE(("delete block range %p, base = %p!\n", range, (void *)range->base));

	// unmap the memory

	vm_address_space *addressSpace = vm_kernel_address_space();
	vm_translation_map *map = &addressSpace->translation_map;

	map->ops->lock(map);
	map->ops->unmap(map, range->base, range->base + kBlockRangeSize - 1);
	map->ops->unlock(map);

	sBlockAddressPool.Put(range->base);

	// free pages

	uint32 numPages = kBlockRangeSize / B_PAGE_SIZE;
	for (uint32 i = 0; i < numPages; i++) {
		if (range->pages[i] == NULL)
			continue;

		vm_page_set_state(range->pages[i], PAGE_STATE_FREE);
	}

	free(range);
}


uint32
block_range::BlockIndex(block_cache *cache, void *address)
{
	return (((addr_t)address - base) % cache->chunk_size) / cache->block_size;
}


uint32
block_range::ChunkIndex(block_cache *cache, void *address)
{
	return ((addr_t)address - base) / cache->chunk_size;
}


block_chunk *
block_range::Chunk(block_cache *cache, void *address)
{
	return &chunks[ChunkIndex(cache, address)];
}


status_t
block_range::Allocate(block_cache *cache, cached_block *block)
{
	block_chunk *chunk;

	void *address = Allocate(cache, &chunk);
	if (address == NULL)
		return B_NO_MEMORY;

	block->data = address;

	// add the block to the chunk
	block->chunk_next = chunk->blocks;
	chunk->blocks = block;

	return B_OK;
}


void
block_range::Free(block_cache *cache, cached_block *block)
{
	Free(cache, block->data);
	block_chunk *chunk = Chunk(cache, block->data);

	// remove block from list

	cached_block *last = NULL, *next = chunk->blocks;
	while (next != NULL && next != block) {
		last = next;
		next = next->chunk_next;
	}
	if (next == NULL) {
		panic("cached_block %p was not in chunk %p\n", block, chunk);
	} else {
		if (last)
			last->chunk_next = block->chunk_next;
		else
			chunk->blocks = block->chunk_next;
	}

	block->data = NULL;
}


void *
block_range::Allocate(block_cache *cache, block_chunk **_chunk)
{
	// get free chunk in range
	
	uint32 chunk;
	for (chunk = 0; chunk < cache->chunks_per_range; chunk++) {
		if ((used_mask & (1UL << chunk)) == 0)
			break;
	}
	if (chunk == cache->chunks_per_range) {
		panic("block_range %p pretended to be free but isn't\n", this);
		return NULL;
	}

	// get free block in chunk

	uint32 numBlocks = cache->chunk_size / cache->block_size;
	uint32 block;
	for (block = 0; block < numBlocks; block++) {
		if ((chunks[chunk].used_mask & (1UL << block)) == 0)
			break;
	}
	if (block == numBlocks) {
		panic("block_chunk %lu in range %p pretended to be free but isn't\n", block, this);
		return NULL;
	}

	if (!chunks[chunk].mapped) {
		// allocate pages if needed
		uint32 numPages = cache->chunk_size / B_PAGE_SIZE;
		uint32 pageBaseIndex = numPages * chunk;
		if (pages[pageBaseIndex] == NULL) {
			// there are no pages for us yet
			for (uint32 i = 0; i < numPages; i++) {
				vm_page *page = vm_page_allocate_page(PAGE_STATE_FREE);
				if (page == NULL) {
					// ToDo: handle this gracefully
					panic("no memory for block!!\n");
					return NULL;
				}

				pages[pageBaseIndex + i] = page;
			}
		}

		// map the memory

		vm_address_space *addressSpace = vm_kernel_address_space();
		vm_translation_map *map = &addressSpace->translation_map;
		map->ops->lock(map);

		for (uint32 i = 0; i < numPages; i++) {
			map->ops->map(map, base + chunk * cache->chunk_size + i * B_PAGE_SIZE,
				pages[pageBaseIndex + i]->physical_page_number * B_PAGE_SIZE,
				B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
		}

		map->ops->unlock(map);

		chunks[chunk].mapped = true;
	}

	chunks[chunk].used_mask |= 1UL << block;
	if (chunks[chunk].used_mask == cache->chunk_mask) {
		// all blocks are used in this chunk, propagate usage bit
		used_mask |= 1UL << chunk;

		if (used_mask == cache->range_mask) {
			// range is full, remove it from the free list

			// usually, the first entry will be ourself, but we don't count on it
			block_range *last = NULL, *range = cache->free_ranges;
			while (range != NULL && range != this) {
				last = range;
				range = range->free_next;
			}
			if (range == NULL) {
				panic("block_range %p was free but not in the free list\n", this);
			} else {
				if (last)
					last->free_next = free_next;
				else
					cache->free_ranges = free_next;
			}
		}
	}
	TRACE(("Allocate: used masks: chunk = %x, range = %lx\n", chunks[chunk].used_mask, used_mask));

	if (_chunk)
		*_chunk = &chunks[chunk];
	return (void *)(base + cache->chunk_size * chunk + cache->block_size * block);
}


void
block_range::Free(block_cache *cache, void *address)
{
	uint32 chunk = ChunkIndex(cache, address);

	if (chunks[chunk].used_mask == cache->chunk_mask) {
		if (used_mask == cache->range_mask) {
			// range was full before, add it to the free list
			free_next = cache->free_ranges;
			cache->free_ranges = this;
		}
		// chunk was full before, propagate usage bit to range
		used_mask &= ~(1UL << chunk);
	}
	chunks[chunk].used_mask &= ~(1UL << BlockIndex(cache, address));

	TRACE(("Free: used masks: chunk = %x, range = %lx\n", chunks[chunk].used_mask, used_mask));
}


bool
block_range::Unused(const block_cache *cache) const
{
	if (used_mask != 0)
		return false;

	for (int32 chunk = cache->chunks_per_range; chunk-- > 0;) {
		if (chunks[chunk].used_mask != 0)
			return false;
	}

	return true;
}


//	#pragma mark -


extern "C" status_t
init_block_allocator(void)
{
	new(&sBlockAddressPool) BlockAddressPool;
		// static initializers do not work in the kernel,
		// so we have to do it here, manually

	return sBlockAddressPool.InitCheck();
}
