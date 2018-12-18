/*
 * Copyright 2003-2013, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "runtime_loader_private.h"

#include <syscalls.h>

#ifdef HEAP_TEST
#	include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>

#include <algorithm>

#include <util/SplayTree.h>

#include <locks.h>


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
const static size_t kAlignment = 8;
	// all memory chunks will be a multiple of this

const static size_t kInitialHeapSize = 64 * 1024;
const static size_t kHeapGrowthAlignment = 32 * 1024;

static const char* const kLockName = "runtime_loader heap";
static recursive_lock sLock = RECURSIVE_LOCK_INITIALIZER(kLockName);


class Chunk {
public:
	size_t CompleteSize() const
	{
		return fSize;
	}

protected:
	union {
		size_t	fSize;
		char	fAlignment[kAlignment];
	};
};


class FreeChunk;


struct FreeChunkData : SplayTreeLink<FreeChunk> {

	FreeChunk* Next() const
	{
		return fNext;
	}

	FreeChunk** NextLink()
	{
		return &fNext;
	}

protected:
	FreeChunk*	fNext;
};


class FreeChunk : public Chunk, public FreeChunkData {
public:
			void				SetTo(size_t size);

			size_t				Size() const;

			FreeChunk*			Split(size_t splitSize);
			bool				IsTouching(FreeChunk* link);
			FreeChunk*			Join(FreeChunk* link);

			void*				AllocatedAddress() const;
	static	FreeChunk*			SetToAllocated(void* allocated);
};


struct FreeChunkKey {
	FreeChunkKey(size_t size)
		:
		fSize(size),
		fChunk(NULL)
	{
	}

	FreeChunkKey(const FreeChunk* chunk)
		:
		fSize(chunk->Size()),
		fChunk(chunk)
	{
	}

	int Compare(const FreeChunk* chunk) const
	{
		size_t chunkSize = chunk->Size();
		if (chunkSize != fSize)
			return fSize < chunkSize ? -1 : 1;

		if (fChunk == chunk)
			return 0;
		return fChunk < chunk ? -1 : 1;
	}

private:
	size_t				fSize;
	const FreeChunk*	fChunk;
};


struct FreeChunkTreeDefinition {
	typedef FreeChunkKey	KeyType;
	typedef FreeChunk		NodeType;

	static FreeChunkKey GetKey(const FreeChunk* node)
	{
		return FreeChunkKey(node);
	}

	static SplayTreeLink<FreeChunk>* GetLink(FreeChunk* node)
	{
		return node;
	}

	static int Compare(const FreeChunkKey& key, const FreeChunk* node)
	{
		return key.Compare(node);
	}

	static FreeChunk** GetListLink(FreeChunk* node)
	{
		return node->NextLink();
	}
};


typedef IteratableSplayTree<FreeChunkTreeDefinition> FreeChunkTree;


static size_t sAvailable;
static FreeChunkTree sFreeChunkTree;


static inline size_t
align(size_t size, size_t alignment = kAlignment)
{
	return (size + alignment - 1) & ~(alignment - 1);
}


void
FreeChunk::SetTo(size_t size)
{
	fSize = size;
	fNext = NULL;
}


/*!	Returns the amount of bytes that can be allocated
	in this chunk.
*/
size_t
FreeChunk::Size() const
{
	return (addr_t)this + fSize - (addr_t)AllocatedAddress();
}


/*!	Splits the upper half at the requested location and returns it. This chunk
	will no longer be a valid FreeChunk object; only its fSize will be valid.
 */
FreeChunk*
FreeChunk::Split(size_t splitSize)
{
	splitSize = align(splitSize);

	FreeChunk* chunk = (FreeChunk*)((addr_t)AllocatedAddress() + splitSize);
	size_t newSize = (addr_t)chunk - (addr_t)this;
	chunk->fSize = fSize - newSize;
	chunk->fNext = NULL;

	fSize = newSize;

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


void*
FreeChunk::AllocatedAddress() const
{
	return (void*)static_cast<const FreeChunkData*>(this);
}


FreeChunk*
FreeChunk::SetToAllocated(void* allocated)
{
	return static_cast<FreeChunk*>((FreeChunkData*)allocated);
}


//	#pragma mark -


static status_t
add_area(size_t size)
{
	void* base;
	area_id area = _kern_create_area("rld heap", &base,
		B_RANDOMIZED_ANY_ADDRESS, size, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
	if (area < 0)
		return area;

	// declare the whole area as one chunk, and add it to the free tree
	FreeChunk* chunk = (FreeChunk*)base;
	chunk->SetTo(size);
	sFreeChunkTree.Insert(chunk);

	sAvailable += chunk->Size();
	return B_OK;
}


static status_t
grow_heap(size_t bytes)
{
	return add_area(align(align(sizeof(Chunk)) + bytes, kHeapGrowthAlignment));
}


//	#pragma mark -


status_t
heap_init()
{
	return add_area(kInitialHeapSize);
}


status_t
heap_reinit_after_fork()
{
	recursive_lock_init(&sLock, kLockName);
	return B_OK;
}


#ifdef HEAP_TEST
void
dump_chunks(void)
{
	FreeChunk* chunk = sFreeChunkTree.FindMin();
	while (chunk != NULL) {
		printf("\t%p: chunk size = %ld, end = %p, next = %p\n", chunk,
			chunk->Size(), (uint8*)chunk + chunk->CompleteSize(),
			chunk->Next());
		chunk = chunk->Next();
	}
}
#endif


void*
malloc(size_t size)
{
	if (size == 0)
		return NULL;

	RecursiveLocker _(sLock);

	// align the size requirement to a kAlignment bytes boundary
	if (size < sizeof(FreeChunkData))
		size = sizeof(FreeChunkData);
	size = align(size);

	if (size > sAvailable) {
		// try to enlarge heap
		if (grow_heap(size) != B_OK)
			return NULL;
	}

	FreeChunkKey key(size);
	FreeChunk* chunk = sFreeChunkTree.FindClosest(key, true, true);
	if (chunk == NULL) {
		// could not find a free chunk as large as needed
		if (grow_heap(size) != B_OK)
			return NULL;

		chunk = sFreeChunkTree.FindClosest(key, true, true);
		if (chunk == NULL) {
			TRACE(("no allocation chunk found after growing the heap\n"));
			return NULL;
		}
	}

	sFreeChunkTree.Remove(chunk);
	sAvailable -= chunk->Size();

	void* allocatedAddress = chunk->AllocatedAddress();

	// If this chunk is bigger than the requested size and there's enough space
	// left over for a new chunk, we split it.
	if (chunk->Size() >= size + align(sizeof(FreeChunk))) {
		FreeChunk* freeChunk = chunk->Split(size);
		sFreeChunkTree.Insert(freeChunk);
		sAvailable += freeChunk->Size();
	}

	TRACE(("malloc(%lu) -> %p\n", size, allocatedAddress));
	return allocatedAddress;
}


void*
realloc(void* oldBuffer, size_t newSize)
{
	if (newSize == 0) {
		TRACE(("realloc(%p, %lu) -> NULL\n", oldBuffer, newSize));
		free(oldBuffer);
		return NULL;
	}

	RecursiveLocker _(sLock);

	size_t oldSize = 0;
	if (oldBuffer != NULL) {
		FreeChunk* oldChunk = FreeChunk::SetToAllocated(oldBuffer);
		oldSize = oldChunk->Size();

		// Check if the old buffer still fits, and if it makes sense to keep it.
		if (oldSize >= newSize
			&& (oldSize < 128 || newSize > oldSize / 3)) {
			TRACE(("realloc(%p, %lu) old buffer is large enough\n",
				oldBuffer, newSize));
			return oldBuffer;
		}
	}

	void* newBuffer = malloc(newSize);
	if (newBuffer == NULL)
		return NULL;

	if (oldBuffer != NULL) {
		memcpy(newBuffer, oldBuffer, std::min(oldSize, newSize));
		free(oldBuffer);
	}

	TRACE(("realloc(%p, %lu) -> %p\n", oldBuffer, newSize, newBuffer));
	return newBuffer;
}


void
free(void* allocated)
{
	if (allocated == NULL)
		return;

	RecursiveLocker _(sLock);

	TRACE(("free(%p)\n", allocated));


	FreeChunk* freedChunk = FreeChunk::SetToAllocated(allocated);

	// try to join the new free chunk with an existing one
	// it may be joined with up to two chunks

	FreeChunk* chunk = sFreeChunkTree.FindMin();
	int32 joinCount = 0;

	while (chunk) {
		FreeChunk* nextChunk = chunk->Next();

		if (chunk->IsTouching(freedChunk)) {
			sFreeChunkTree.Remove(chunk);
			sAvailable -= chunk->Size();

			freedChunk = chunk->Join(freedChunk);

			if (++joinCount == 2)
				break;
		}

		chunk = nextChunk;
	}

	sFreeChunkTree.Insert(freedChunk);
	sAvailable += freedChunk->Size();
}
