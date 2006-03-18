/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */

/*!
	This class manages a pool of areas for one client. The client is supposed
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
#include "ServerApp.h"

#include <stdlib.h>


typedef block_list::Iterator block_iterator;
typedef chunk_list::Iterator chunk_iterator;


ClientMemoryAllocator::ClientMemoryAllocator(ServerApp* application)
	:
	fApplication(application),
	fLock("client memory lock")
{
}


ClientMemoryAllocator::~ClientMemoryAllocator()
{
}


status_t
ClientMemoryAllocator::InitCheck()
{
	return fLock.InitCheck() < B_OK ? fLock.InitCheck() : B_OK;
}


void *
ClientMemoryAllocator::Allocate(size_t size, void** _address, bool& newArea)
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
		*_address = best->base;
		return best;
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

	*_address = usedBlock->base;
	return usedBlock;
}


void
ClientMemoryAllocator::Free(void *cookie)
{
	if (cookie == NULL)
		return;

	// TODO: implement me!!!
}


area_id
ClientMemoryAllocator::Area(void* cookie)
{
	struct block* block = (struct block*)cookie;

	if (block != NULL)
		return block->chunk->area;

	return B_ERROR;
}


uint32
ClientMemoryAllocator::AreaOffset(void* cookie)
{
	struct block* block = (struct block*)cookie;

	if (block != NULL)
		return block->base - block->chunk->base;

	return 0;
}


bool
ClientMemoryAllocator::Lock()
{
	return fLock.ReadLock();
}


void
ClientMemoryAllocator::Unlock()
{
	fLock.ReadUnlock();
}


struct block *
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
		snprintf(name, sizeof(name), "heap:%ld:%s", fApplication->ClientTeam(),
			fApplication->SignatureLeaf());
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

