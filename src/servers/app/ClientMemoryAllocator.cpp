/*
 * Copyright 2006-2012, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


/*!	This class manages a pool of areas for one client. The client is supposed
	to clone these areas into its own address space to access the data.
	This mechanism is only used for bitmaps for far.

	Note, this class doesn't provide any real locking - you need to have the
	ServerApp locked when interacting with any method of this class.

	The Lock()/Unlock() methods are needed whenever you access a pointer that
	lies within an area allocated using this class. This is needed because an
	area might be temporarily unavailable or might be relocated at any time.
*/


//	TODO: right now, areas will always stay static until they are deleted;
//		locking is not yet done or enforced!


#include "ClientMemoryAllocator.h"

#include <stdio.h>
#include <stdlib.h>

#include "ServerApp.h"


typedef block_list::Iterator block_iterator;
typedef chunk_list::Iterator chunk_iterator;


ClientMemoryAllocator::ClientMemoryAllocator(ServerApp* application)
	:
	fApplication(application)
{
}


ClientMemoryAllocator::~ClientMemoryAllocator()
{
	// delete all areas and chunks/blocks that are still allocated

	while (true) {
		struct block* block = fFreeBlocks.RemoveHead();
		if (block == NULL)
			break;

		free(block);
	}

	while (true) {
		struct chunk* chunk = fChunks.RemoveHead();
		if (chunk == NULL)
			break;

		delete_area(chunk->area);
		free(chunk);
	}
}


void*
ClientMemoryAllocator::Allocate(size_t size, block** _address, bool& newArea)
{
	// Search best matching free block from the list

	block_iterator iterator = fFreeBlocks.GetIterator();
	struct block* block;
	struct block* best = NULL;

	while ((block = iterator.Next()) != NULL) {
		if (block->size >= size && (best == NULL || block->size < best->size))
			best = block;
	}

	if (best == NULL) {
		// We didn't find a free block - we need to allocate
		// another chunk, or resize an existing chunk
		best = _AllocateChunk(size, newArea);
		if (best == NULL)
			return NULL;
	} else
		newArea = false;

	// We need to split the chunk into two parts: the one to keep
	// and the one to give away

	if (best->size == size) {
		// The simple case: the free block has exactly the size we wanted to have
		fFreeBlocks.Remove(best);
		*_address = best;
		return best->base;
	}

	// TODO: maybe we should have the user reserve memory in its object
	//	for us, so we don't have to do this here...

	struct block* usedBlock = (struct block*)malloc(sizeof(struct block));
	if (usedBlock == NULL)
		return NULL;

	usedBlock->base = best->base;
	usedBlock->size = size;
	usedBlock->chunk = best->chunk;

	best->base += size;
	best->size -= size;

	*_address = usedBlock;
	return usedBlock->base;
}


void
ClientMemoryAllocator::Free(block* freeBlock)
{
	if (freeBlock == NULL)
		return;

	// search for an adjacent free block

	block_iterator iterator = fFreeBlocks.GetIterator();
	struct block* before = NULL;
	struct block* after = NULL;
	bool inFreeList = true;

	if (freeBlock->size != freeBlock->chunk->size) {
		// TODO: this could be done better if free blocks are sorted,
		//	and if we had one free blocks list per chunk!
		//	IOW this is a bit slow...

		while (struct block* block = iterator.Next()) {
			if (block->chunk != freeBlock->chunk)
				continue;

			if (block->base + block->size == freeBlock->base)
				before = block;

			if (block->base == freeBlock->base + freeBlock->size)
				after = block;
		}

		if (before != NULL && after != NULL) {
			// merge with adjacent blocks
			before->size += after->size + freeBlock->size;
			fFreeBlocks.Remove(after);
			free(after);
			free(freeBlock);
			freeBlock = before;
		} else if (before != NULL) {
			before->size += freeBlock->size;
			free(freeBlock);
			freeBlock = before;
		} else if (after != NULL) {
			after->base -= freeBlock->size;
			after->size += freeBlock->size;
			free(freeBlock);
			freeBlock = after;
		} else
			fFreeBlocks.Add(freeBlock);
	} else
		inFreeList = false;

	if (freeBlock->size == freeBlock->chunk->size) {
		// We can delete the chunk now
		struct chunk* chunk = freeBlock->chunk;

		if (inFreeList)
			fFreeBlocks.Remove(freeBlock);
		free(freeBlock);

		fChunks.Remove(chunk);
		delete_area(chunk->area);
		free(chunk);
	}
}


void
ClientMemoryAllocator::Dump()
{
	debug_printf("Application %ld, %s: chunks:\n", fApplication->ClientTeam(),
		fApplication->Signature());

	chunk_list::Iterator iterator = fChunks.GetIterator();
	int32 i = 0;
	while (struct chunk* chunk = iterator.Next()) {
		debug_printf("  [%4ld] %p, area %ld, base %p, size %lu\n", i++, chunk,
			chunk->area, chunk->base, chunk->size);
	}

	debug_printf("free blocks:\n");

	block_list::Iterator blockIterator = fFreeBlocks.GetIterator();
	i = 0;
	while (struct block* block = blockIterator.Next()) {
		debug_printf("  [%6ld] %p, chunk %p, base %p, size %lu\n", i++, block,
			block->chunk, block->base, block->size);
	}
}


struct block*
ClientMemoryAllocator::_AllocateChunk(size_t size, bool& newArea)
{
	// round up to multiple of page size
	size = (size + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);

	// At first, try to resize our existing areas

	chunk_iterator iterator = fChunks.GetIterator();
	struct chunk* chunk;
	while ((chunk = iterator.Next()) != NULL) {
		status_t status = resize_area(chunk->area, chunk->size + size);
		if (status == B_OK) {
			newArea = false;
			break;
		}
	}

	// TODO: resize and relocate while holding the write lock

	struct block* block;
	uint8* address;

	if (chunk == NULL) {
		// TODO: temporary measurement as long as resizing areas doesn't
		//	work the way we need (with relocating the area, if needed)
		if (size < B_PAGE_SIZE * 32)
			size = B_PAGE_SIZE * 32;

		// create new area for this allocation
		chunk = (struct chunk*)malloc(sizeof(struct chunk));
		if (chunk == NULL)
			return NULL;

		block = (struct block*)malloc(sizeof(struct block));
		if (block == NULL) {
			free(chunk);
			return NULL;
		}

		char name[B_OS_NAME_LENGTH];
#ifdef HAIKU_TARGET_PLATFORM_LIBBE_TEST
		strcpy(name, "client heap");
#else
		snprintf(name, sizeof(name), "heap:%ld:%s", fApplication->ClientTeam(),
			fApplication->SignatureLeaf());
#endif
		area_id area = create_area(name, (void**)&address, B_ANY_ADDRESS, size,
			B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
		if (area < B_OK) {
			free(block);
			free(chunk);
			return NULL;
		}

		// add chunk to list

		chunk->area = area;
		chunk->base = address;
		chunk->size = size;

		fChunks.Add(chunk);
		newArea = true;
	} else {
		// create new free block for this chunk
		block = (struct block *)malloc(sizeof(struct block));
		if (block == NULL)
			return NULL;

		address = chunk->base + chunk->size;
		chunk->size += size;
	}

	// add block to free list

	block->chunk = chunk;
	block->base = address;
	block->size = size;

	fFreeBlocks.Add(block);

	return block;
}


// #pragma mark -


ClientMemory::ClientMemory()
	:
	fBlock(NULL)
{
}


ClientMemory::~ClientMemory()
{
	if (fBlock != NULL)
		fAllocator->Free(fBlock);
}


void*
ClientMemory::Allocate(ClientMemoryAllocator* allocator, size_t size,
	bool& newArea)
{
	fAllocator = allocator;
	return fAllocator->Allocate(size, &fBlock, newArea);
}


area_id
ClientMemory::Area()
{
	if (fBlock != NULL)
		return fBlock->chunk->area;
	return B_ERROR;
}


uint8*
ClientMemory::Address()
{
	if (fBlock != NULL)
		return fBlock->base;
	return 0;
}


uint32
ClientMemory::AreaOffset()
{
	if (fBlock != NULL)
		return fBlock->base - fBlock->chunk->base;
	return 0;
}


// #pragma mark -


ClonedAreaMemory::ClonedAreaMemory()
	:
	fClonedArea(-1),
	fOffset(0),
	fBase(NULL)
{
}


ClonedAreaMemory::~ClonedAreaMemory()
{
	if (fClonedArea >= 0)
		delete_area(fClonedArea);
}


void*
ClonedAreaMemory::Clone(area_id area, uint32 offset)
{
	fClonedArea = clone_area("server_memory", (void**)&fBase, B_ANY_ADDRESS,
		B_READ_AREA | B_WRITE_AREA, area);
	if (fBase == NULL)
		return NULL;
	fOffset = offset;
	return Address();
}


area_id
ClonedAreaMemory::Area()
{
	return fClonedArea;
}


uint8*
ClonedAreaMemory::Address()
{
	return fBase + fOffset;
}


uint32
ClonedAreaMemory::AreaOffset()
{
	return fOffset;
}
