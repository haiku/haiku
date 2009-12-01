/*
 * Copyright 2003-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <boot/heap.h>
#include <boot/platform.h>
#include <util/kernel_cpp.h>

#ifdef HEAP_TEST
#	include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>


//#define TRACE_HEAP
#ifdef TRACE_HEAP
#	define TRACE(format...)	dprintf(format)
#else
#	define TRACE(format...)	do { } while (false)
#endif


/*!	This is a very simple malloc()/free() implementation - it only
	manages a free list.
	After heap_init() is called, all free memory is contained in one
	big chunk, the only entry in the free link list (which is a single
	linked list).
	When memory is allocated, the smallest free chunk that contains
	the requested size is split (or taken as a whole if it can't be
	splitted anymore), and it's lower half will be removed from the
	free list.
	The free list is ordered by size, starting with the smallest
	free chunk available. When a chunk is freed, it will be joint
	with its predecessor or successor, if possible.
	To ease list handling, the list anchor itself is a free chunk with
	size 0 that can't be allocated.
*/

#define DEBUG_ALLOCATIONS
	// if defined, freed memory is filled with 0xcc

struct free_chunk {
	uint32		size;
	free_chunk*	next;

	uint32		Size() const;
	free_chunk*	Split(uint32 splitSize);
	bool		IsTouching(free_chunk* link);
	free_chunk*	Join(free_chunk* link);
	void		Remove(free_chunk* previous = NULL);
	void		Enqueue();

	void*		AllocatedAddress() const;
	static free_chunk* SetToAllocated(void* allocated);
};


const static uint32 kAlignment = 4;
	// all memory chunks will be a multiple of this

static void* sHeapBase;
static uint32 /*sHeapSize,*/ sMaxHeapSize, sAvailable;
static free_chunk sFreeAnchor;


/*!	Returns the amount of bytes that can be allocated
	in this chunk.
*/
uint32
free_chunk::Size() const
{
	return size - sizeof(uint32);
}


/*!	Splits the upper half at the requested location
	and returns it.
*/
free_chunk*
free_chunk::Split(uint32 splitSize)
{
	free_chunk* chunk = (free_chunk*)((uint8*)this + sizeof(uint32) + splitSize);
	chunk->size = size - splitSize - sizeof(uint32);
	chunk->next = next;

	size = splitSize + sizeof(uint32);

	return chunk;
}


/*!	Checks if the specified chunk touches this chunk, so
	that they could be joined.
*/
bool
free_chunk::IsTouching(free_chunk* chunk)
{
	return chunk
		&& (((uint8*)this + size == (uint8*)chunk)
			|| (uint8*)chunk + chunk->size == (uint8*)this);
}


/*!	Joins the chunk to this chunk and returns the pointer
	to the new chunk - which will either be one of the
	two chunks.
	Note, the chunks must be joinable, or else this method
	doesn't work correctly. Use free_chunk::IsTouching()
	to check if this method can be applied.
*/
free_chunk*
free_chunk::Join(free_chunk* chunk)
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
free_chunk::Remove(free_chunk* previous)
{
	if (previous == NULL) {
		// find the previous chunk in the list
		free_chunk* chunk = sFreeAnchor.next;

		while (chunk != NULL && chunk != this) {
			previous = chunk;
			chunk = chunk->next;
		}

		if (chunk == NULL)
			panic("try to remove chunk that's not in list");
	}

	previous->next = this->next;
	this->next = NULL;
}


void
free_chunk::Enqueue()
{
	free_chunk* chunk = sFreeAnchor.next;
	free_chunk* last = &sFreeAnchor;
	while (chunk && chunk->Size() < size) {
		last = chunk;
		chunk = chunk->next;
	}

	this->next = chunk;
	last->next = this;

#ifdef DEBUG_ALLOCATIONS
	memset((uint8*)this + sizeof(free_chunk), 0xcc, this->size - sizeof(free_chunk));
#endif
}


void*
free_chunk::AllocatedAddress() const
{
	return (void*)&next;
}


free_chunk*
free_chunk::SetToAllocated(void* allocated)
{
	return (free_chunk*)((uint8*)allocated - sizeof(uint32));
}


//	#pragma mark -


void
heap_release(stage2_args* args)
{
	platform_release_heap(args, sHeapBase);
}


status_t
heap_init(stage2_args* args)
{
	void* base;
	void* top;
	if (platform_init_heap(args, &base, &top) < B_OK)
		return B_ERROR;

	sHeapBase = base;
	sMaxHeapSize = (uint8*)top - (uint8*)base;
	sAvailable = sMaxHeapSize - sizeof(uint32);

	// declare the whole heap as one chunk, and add it
	// to the free list

	free_chunk* chunk = (free_chunk*)base;
	chunk->size = sMaxHeapSize;
	chunk->next = NULL;

	sFreeAnchor.size = 0;
	sFreeAnchor.next = chunk;

	return B_OK;
}


#if 0
char*
grow_heap(uint32 bytes)
{
	char* start;

	if (sHeapSize + bytes > sMaxHeapSize)
		return NULL;

	start = (char*)sHeapBase + sHeapSize;
	memset(start, 0, bytes);
	sHeapSize += bytes;

	return start;
}
#endif

#ifdef HEAP_TEST
void
dump_chunks(void)
{
	free_chunk* chunk = sFreeAnchor.next;
	free_chunk* last = &sFreeAnchor;
	while (chunk != NULL) {
		last = chunk;

		printf("\t%p: chunk size = %ld, end = %p, next = %p\n", chunk,
			chunk->size, (uint8*)chunk + chunk->size, chunk->next);
		chunk = chunk->next;
	}
}
#endif

uint32
heap_available(void)
{
	return sAvailable;
}


void*
malloc(size_t size)
{
	if (sHeapBase == NULL || size == 0)
		return NULL;

	// align the size requirement to a kAlignment bytes boundary
	size = (size - 1 + kAlignment) & ~(size_t)(kAlignment - 1);

	if (size > sAvailable) {
		dprintf("malloc(): Out of memory!\n");
		return NULL;
	}

	free_chunk* chunk = sFreeAnchor.next;
	free_chunk* last = &sFreeAnchor;
	while (chunk && chunk->Size() < size) {
		last = chunk;
		chunk = chunk->next;
	}

	if (chunk == NULL) {
		// could not find a free chunk as large as needed
		dprintf("malloc(): Out of memory!\n");
		return NULL;
	}

	if (chunk->Size() > size + sizeof(free_chunk) + kAlignment) {
		// if this chunk is bigger than the requested size,
		// we split it to form two chunks (with a minimal
		// size of kAlignment allocatable bytes).

		free_chunk* freeChunk = chunk->Split(size);
		last->next = freeChunk;

		// re-enqueue the free chunk at the correct position
		freeChunk->Remove(last);
		freeChunk->Enqueue();
	} else {
		// remove the chunk from the free list

		last->next = chunk->next;
	}

	sAvailable -= size + sizeof(uint32);

	TRACE("malloc(%lu) -> %p\n", size, chunk->AllocatedAddress());
	return chunk->AllocatedAddress();
}


void*
realloc(void* oldBuffer, size_t newSize)
{
	if (newSize == 0) {
		TRACE("realloc(%p, %lu) -> NULL\n", oldBuffer, newSize);
		free(oldBuffer);
		return NULL;
	}

	size_t copySize = newSize;
	if (oldBuffer != NULL) {
		free_chunk* oldChunk = free_chunk::SetToAllocated(oldBuffer);

		// Check if the old buffer still fits, and if it makes sense to keep it
		if (oldChunk->size >= newSize
			&& (oldChunk->size < 128 || newSize > oldChunk->size / 3)) {
			TRACE("realloc(%p, %lu) old buffer is large enough\n",
				oldBuffer, newSize);
			return oldChunk->AllocatedAddress();
		}

		if (copySize > oldChunk->size)
			copySize = oldChunk->size;
	}

	void* newBuffer = malloc(newSize);
	if (newBuffer == NULL)
		return NULL;

	if (oldBuffer != NULL) {
		memcpy(newBuffer, oldBuffer, copySize);
		free(oldBuffer);
	}

	TRACE("realloc(%p, %lu) -> %p\n", oldBuffer, newSize, newBuffer);
	return newBuffer;
}


void
free(void* allocated)
{
	if (allocated == NULL)
		return;

	TRACE("free(%p)\n", allocated);

	free_chunk* freedChunk = free_chunk::SetToAllocated(allocated);

#ifdef DEBUG_ALLOCATIONS
	if (freedChunk->size > sMaxHeapSize)
		panic("freed chunk %p clobbered (%lx)!\n", freedChunk, freedChunk->size);
{
	free_chunk* chunk = sFreeAnchor.next;
	while (chunk) {
		if (chunk->size > sMaxHeapSize || freedChunk == chunk)
			panic("invalid chunk in free list, or double free\n");
		chunk = chunk->next;
	}
}
#endif

	sAvailable += freedChunk->size;

	// try to join the new free chunk with an existing one
	// it may be joined with up to two chunks

	free_chunk* chunk = sFreeAnchor.next;
	free_chunk* last = &sFreeAnchor;
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

