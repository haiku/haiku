/*
 * Copyright 2003-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "runtime_loader_private.h"

#include <syscalls.h>
#include <util/kernel_cpp.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static const size_t kInitialHeapSize = 65536;


/* This is a very simple malloc()/free() implementation - it only
 * manages a free list.
 * After heap_init() is called, all free memory is contained in one
 * big chunk, the only entry in the free link list (which is a single
 * linked list).
 * When memory is allocated, the smallest free chunk that contains
 * the requested size is split (or taken as a whole if it can't be
 * splitted anymore), and it's lower half will be removed from the
 * free list.
 * The free list is ordered by size, starting with the smallest
 * free chunk available. When a chunk is freed, it will be joint
 * with its predecessor or successor, if possible.
 * To ease list handling, the list anchor itself is a free chunk with
 * size 0 that can't be allocated.
 */


struct free_chunk {
	uint32		size;
	free_chunk	*next;

	uint32 Size() const;
	free_chunk *Split(uint32 splitSize);
	bool IsTouching(free_chunk *link);
	free_chunk *Join(free_chunk *link);
	void Remove(free_chunk *previous = NULL);
	void Enqueue();

	void *AllocatedAddress() const;
	static free_chunk *SetToAllocated(void *allocated);
};


static uint32 sAvailable;
static free_chunk sFreeAnchor;


/** Returns the amount of bytes that can be allocated
 *	in this chunk.
 */

uint32
free_chunk::Size() const
{
	return size - sizeof(uint32);
}


/**	Splits the upper half at the requested location
 *	and returns it.
 */

free_chunk *
free_chunk::Split(uint32 splitSize)
{
	free_chunk *chunk = (free_chunk *)((uint8 *)this + sizeof(uint32) + splitSize);
	chunk->size = size - splitSize - sizeof(uint32);
	chunk->next = next;

	size = splitSize + sizeof(uint32);

	return chunk;
}


/**	Checks if the specified chunk touches this chunk, so
 *	that they could be joined.
 */

bool
free_chunk::IsTouching(free_chunk *chunk)
{
	return chunk
		&& (((uint8 *)this + size == (uint8 *)chunk)
			|| (uint8 *)chunk + chunk->size == (uint8 *)this);
}


/**	Joins the chunk to this chunk and returns the pointer
 *	to the new chunk - which will either be one of the
 *	two chunks.
 *	Note, the chunks must be joinable, or else this method
 *	doesn't work correctly. Use free_chunk::IsTouching()
 *	to check if this method can be applied.
 */

free_chunk *
free_chunk::Join(free_chunk *chunk)
{
	if (chunk < this) {
		chunk->size += size;
		chunk->next = next;

		return chunk;
	}

	size += chunk->size;
	next = chunk->next;

	return this;
}


void
free_chunk::Remove(free_chunk *previous)
{
	if (previous == NULL) {
		// find the previous chunk in the list
		free_chunk *chunk = sFreeAnchor.next;

		while (chunk != NULL && chunk != this) {
			previous = chunk;
			chunk = chunk->next;
		}

		if (chunk == NULL) {
			printf("runtime_loader: try to remove chunk that's not in list");
			return;
		}
	}

	previous->next = this->next;
	this->next = NULL;
}


void
free_chunk::Enqueue()
{
	free_chunk *chunk = sFreeAnchor.next, *last = &sFreeAnchor;
	while (chunk && chunk->Size() < size) {
		last = chunk;
		chunk = chunk->next;
	}

	this->next = chunk;
	last->next = this;
}


void *
free_chunk::AllocatedAddress() const
{
	return (void *)&next;
}


free_chunk *
free_chunk::SetToAllocated(void *allocated)
{
	return (free_chunk *)((uint8 *)allocated - sizeof(uint32));
}


//	#pragma mark - private functions


static status_t
add_area(size_t size)
{
	void *base;
	area_id area = _kern_create_area("rld heap", &base, B_ANY_ADDRESS, size,
		B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
	if (area < B_OK)
		return area;

	sAvailable += size - sizeof(uint32);

	// declare the whole heap as one chunk, and add it
	// to the free list

	free_chunk *chunk = (free_chunk *)base;
	chunk->size = size;
	chunk->next = sFreeAnchor.next;

	sFreeAnchor.next = chunk;
	return B_OK;
}


static status_t
grow_heap(uint32 bytes)
{
	// align the area size to an 32768 bytes boundary
	bytes = (bytes + 32767) & ~32767;
	return add_area(bytes);
}


//	#pragma mark - public API


status_t
heap_init(void)
{
	status_t status = add_area(kInitialHeapSize);
	if (status < B_OK)
		return status;

	sFreeAnchor.size = 0;
	return B_OK;
}


#ifdef HEAP_TEST
void
dump_chunks(void)
{
	free_chunk *chunk = sFreeAnchor.next, *last = &sFreeAnchor;
	while (chunk != NULL) {
		last = chunk;

		printf("\t%p: chunk size = %ld, end = %p, next = %p\n", chunk, chunk->size, (uint8 *)chunk + chunk->size, chunk->next);
		chunk = chunk->next;
	}
}
#endif


void *
realloc(void *allocation, size_t newSize)
{
	// free, if new size is 0
	if (newSize == 0) {
		free(allocation);
		return NULL;
	}

	// just malloc(), if no previous allocation
	if (allocation == NULL)
		return malloc(newSize);

	// we're lazy and don't shrink allocations
	free_chunk* chunk = free_chunk::SetToAllocated(allocation);
	if (chunk->Size() >= newSize)
		return allocation;

	// the allocation needs to grow -- allocate a new one and memcpy()
	void* newAllocation = malloc(newSize);
	if (newAllocation != NULL) {
		memcpy(newAllocation, allocation, chunk->Size());
		free(allocation);
	}

	return newAllocation;
}


void *
malloc(size_t size)
{
	if (size == 0)
		return NULL;

	// align the size requirement to an 8 bytes boundary
	size = (size + 7) & ~7;

restart:
	if (size > sAvailable) {
		// try to enlarge heap
		if (grow_heap(size) < B_OK)
			return NULL;
	}

	free_chunk *chunk = sFreeAnchor.next, *last = &sFreeAnchor;
	while (chunk && chunk->Size() < size) {
		last = chunk;
		chunk = chunk->next;
	}

	if (chunk == NULL) {
		// could not find a free chunk as large as needed
		if (grow_heap(size) < B_OK)
			return NULL;

		goto restart;
	}

	if (chunk->Size() > size + sizeof(free_chunk) + 4) {
		// if this chunk is bigger than the requested size,
		// we split it to form two chunks (with a minimal
		// size of 4 allocatable bytes).

		free_chunk *freeChunk = chunk->Split(size);
		last->next = freeChunk;

		// re-enqueue the free chunk at the correct position
		freeChunk->Remove(last);
		freeChunk->Enqueue();
	} else {
		// remove the chunk from the free list

		last->next = chunk->next;
	}

	sAvailable -= size + sizeof(uint32);

	return chunk->AllocatedAddress();
}


void
free(void *allocated)
{
	if (allocated == NULL)
		return;

	free_chunk *freedChunk = free_chunk::SetToAllocated(allocated);
	sAvailable += freedChunk->size;

	// try to join the new free chunk with an existing one
	// it may be joined with up to two chunks

	free_chunk *chunk = sFreeAnchor.next, *last = &sFreeAnchor;
	int32 joinCount = 0;

	while (chunk) {
		if (chunk->IsTouching(freedChunk)) {
			// almost "insert" it into the list before joining
			// because the next pointer is inherited by the chunk
			freedChunk->next = chunk->next;
			freedChunk = chunk->Join(freedChunk);

			// remove the joined chunk from the list
			last->next = freedChunk->next;
			chunk = last;

			if (++joinCount == 2)
				break;
		}

		last = chunk;
		chunk = chunk->next;
	}

	// enqueue the link at the right position; the
	// free link queue is ordered by size

	freedChunk->Enqueue();
}

