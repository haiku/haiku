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

class FreeChunk {
public:
			void				SetTo(size_t size, FreeChunk* next);

			uint32				Size() const;
			uint32				CompleteSize() const { return fSize; }

			FreeChunk*			Next() const { return fNext; }
			void				SetNext(FreeChunk* next) { fNext = next; }

			FreeChunk*			Split(uint32 splitSize);
			bool				IsTouching(FreeChunk* link);
			FreeChunk*			Join(FreeChunk* link);
			void				Remove(FreeChunk* previous = NULL);
			void				Enqueue();

			void*				AllocatedAddress() const;
	static	FreeChunk*			SetToAllocated(void* allocated);
	static	addr_t				NextOffset() { return sizeof(uint32); }

private:
			uint32				fSize;
			FreeChunk*			fNext;
};


const static uint32 kAlignment = 4;
	// all memory chunks will be a multiple of this

static void* sHeapBase;
static uint32 /*sHeapSize,*/ sMaxHeapSize, sAvailable;
static FreeChunk sFreeAnchor;


void
FreeChunk::SetTo(size_t size, FreeChunk* next)
{
	fSize = size;
	fNext = next;
}


/*!	Returns the amount of bytes that can be allocated
	in this chunk.
*/
uint32
FreeChunk::Size() const
{
	return fSize - FreeChunk::NextOffset();
}


/*!	Splits the upper half at the requested location
	and returns it.
*/
FreeChunk*
FreeChunk::Split(uint32 splitSize)
{
	splitSize = (splitSize - 1 + kAlignment) & ~(kAlignment - 1);

	FreeChunk* chunk
		= (FreeChunk*)((uint8*)this + FreeChunk::NextOffset() + splitSize);
	chunk->fSize = fSize - splitSize - FreeChunk::NextOffset();
	chunk->fNext = fNext;

	fSize = splitSize + FreeChunk::NextOffset();

	return chunk;
}


/*!	Checks if the specified chunk touches this chunk, so
	that they could be joined.
*/
bool
FreeChunk::IsTouching(FreeChunk* chunk)
{
	return chunk
		&& (((uint8*)this + fSize == (uint8*)chunk)
			|| (uint8*)chunk + chunk->fSize == (uint8*)this);
}


/*!	Joins the chunk to this chunk and returns the pointer
	to the new chunk - which will either be one of the
	two chunks.
	Note, the chunks must be joinable, or else this method
	doesn't work correctly. Use FreeChunk::IsTouching()
	to check if this method can be applied.
*/
FreeChunk*
FreeChunk::Join(FreeChunk* chunk)
{
	if (chunk < this) {
		chunk->fSize += fSize;
		chunk->fNext = fNext;

		return chunk;
	}

	fSize += chunk->fSize;
	fNext = chunk->fNext;

	return this;
}


void
FreeChunk::Remove(FreeChunk* previous)
{
	if (previous == NULL) {
		// find the previous chunk in the list
		FreeChunk* chunk = sFreeAnchor.fNext;

		while (chunk != NULL && chunk != this) {
			previous = chunk;
			chunk = chunk->fNext;
		}

		if (chunk == NULL)
			panic("try to remove chunk that's not in list");
	}

	previous->fNext = fNext;
	fNext = NULL;
}


void
FreeChunk::Enqueue()
{
	FreeChunk* chunk = sFreeAnchor.fNext;
	FreeChunk* last = &sFreeAnchor;
	while (chunk && chunk->Size() < fSize) {
		last = chunk;
		chunk = chunk->fNext;
	}

	fNext = chunk;
	last->fNext = this;

#ifdef DEBUG_ALLOCATIONS
	memset((uint8*)this + sizeof(FreeChunk), 0xde,
		fSize - sizeof(FreeChunk));
#endif
}


void*
FreeChunk::AllocatedAddress() const
{
	return (void*)&fNext;
}


FreeChunk*
FreeChunk::SetToAllocated(void* allocated)
{
	return (FreeChunk*)((uint8*)allocated - FreeChunk::NextOffset());
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
	sAvailable = sMaxHeapSize - FreeChunk::NextOffset();

	// declare the whole heap as one chunk, and add it
	// to the free list

	FreeChunk* chunk = (FreeChunk*)base;
	chunk->SetTo(sMaxHeapSize, NULL);
	sFreeAnchor.SetTo(0, chunk);

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
	FreeChunk* chunk = sFreeAnchor.Next();
	FreeChunk* last = &sFreeAnchor;
	while (chunk != NULL) {
		last = chunk;

		printf("\t%p: chunk size = %ld, end = %p, next = %p\n", chunk,
			chunk->Size(), (uint8*)chunk + chunk->CompleteSize(),
			chunk->Next());
		chunk = chunk->Next();
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

	FreeChunk* chunk = sFreeAnchor.Next();
	FreeChunk* last = &sFreeAnchor;
	while (chunk && chunk->Size() < size) {
		last = chunk;
		chunk = chunk->Next();
	}

	if (chunk == NULL) {
		// could not find a free chunk as large as needed
		dprintf("malloc(): Out of memory!\n");
		return NULL;
	}

	if (chunk->Size() > size + sizeof(FreeChunk) + kAlignment) {
		// if this chunk is bigger than the requested size,
		// we split it to form two chunks (with a minimal
		// size of kAlignment allocatable bytes).

		FreeChunk* freeChunk = chunk->Split(size);
		last->SetNext(freeChunk);

		// re-enqueue the free chunk at the correct position
		freeChunk->Remove(last);
		freeChunk->Enqueue();
	} else {
		// remove the chunk from the free list

		last->SetNext(chunk->Next());
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
		FreeChunk* oldChunk = FreeChunk::SetToAllocated(oldBuffer);

		// Check if the old buffer still fits, and if it makes sense to keep it
		if (oldChunk->Size() >= newSize
			&& (oldChunk->Size() < 128 || newSize > oldChunk->Size() / 3)) {
			TRACE("realloc(%p, %lu) old buffer is large enough\n",
				oldBuffer, newSize);
			return oldChunk->AllocatedAddress();
		}

		if (copySize > oldChunk->Size())
			copySize = oldChunk->Size();
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

	FreeChunk* freedChunk = FreeChunk::SetToAllocated(allocated);

#ifdef DEBUG_ALLOCATIONS
	if (freedChunk->CompleteSize() > sMaxHeapSize) {
		panic("freed chunk %p clobbered (%lx)!\n", freedChunk,
			freedChunk->Size());
	}
	{
		FreeChunk* chunk = sFreeAnchor.Next();
		while (chunk) {
			if (chunk->CompleteSize() > sMaxHeapSize || freedChunk == chunk)
				panic("invalid chunk in free list, or double free\n");
			chunk = chunk->Next();
		}
	}
#endif

	sAvailable += freedChunk->CompleteSize();

	// try to join the new free chunk with an existing one
	// it may be joined with up to two chunks

	FreeChunk* chunk = sFreeAnchor.Next();
	FreeChunk* last = &sFreeAnchor;
	int32 joinCount = 0;

	while (chunk) {
		if (chunk->IsTouching(freedChunk)) {
			// almost "insert" it into the list before joining
			// because the next pointer is inherited by the chunk
			freedChunk->SetNext(chunk->Next());
			freedChunk = chunk->Join(freedChunk);

			// remove the joined chunk from the list
			last->SetNext(freedChunk->Next());
			chunk = last;

			if (++joinCount == 2)
				break;
		}

		last = chunk;
		chunk = chunk->Next();
	}

	// enqueue the link at the right position; the
	// free link queue is ordered by size

	freedChunk->Enqueue();
}

