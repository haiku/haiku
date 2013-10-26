/*
 * Copyright 2003-2013, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <boot/heap.h>

#ifdef HEAP_TEST
#	include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>

#include <algorithm>

#include <boot/platform.h>
#include <util/OpenHashTable.h>
#include <util/SplayTree.h>


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
#define DEBUG_MAX_HEAP_USAGE
	// if defined, the maximum heap usage is determined and printed before
	// entering the kernel


const static size_t kAlignment = 8;
	// all memory chunks will be a multiple of this
const static size_t kLargeAllocationThreshold = 16 * 1024;
	// allocations of this size or larger are allocated via
	// platform_allocate_region()


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


struct LargeAllocation {
	LargeAllocation()
	{
	}

	void SetTo(void* address, size_t size)
	{
		fAddress = address;
		fSize = size;
	}

	status_t Allocate(size_t size)
	{
		fSize = size;
		return platform_allocate_region(&fAddress, fSize,
			B_READ_AREA | B_WRITE_AREA, false);
	}

	void Free()
	{
		platform_free_region(fAddress, fSize);
	}

	void* Address() const
	{
		return fAddress;
	}

	size_t Size() const
	{
		return fSize;
	}

	LargeAllocation*& HashNext()
	{
		return fHashNext;
	}

private:
	void*				fAddress;
	size_t				fSize;
	LargeAllocation*	fHashNext;
};


struct LargeAllocationHashDefinition {
	typedef void*			KeyType;
	typedef	LargeAllocation	ValueType;

	size_t HashKey(void* key) const
	{
		return size_t(key) / kAlignment;
	}

	size_t Hash(LargeAllocation* value) const
	{
		return HashKey(value->Address());
	}

	bool Compare(void* key, LargeAllocation* value) const
	{
		return value->Address() == key;
	}

	LargeAllocation*& GetLink(LargeAllocation* value) const
	{
		return value->HashNext();
	}
};


typedef BOpenHashTable<LargeAllocationHashDefinition> LargeAllocationHashTable;


static void* sHeapBase;
static void* sHeapEnd;
static size_t sMaxHeapSize, sAvailable, sMaxHeapUsage;
static FreeChunkTree sFreeChunkTree;

static LargeAllocationHashTable sLargeAllocations;


static inline size_t
align(size_t size)
{
	return (size + kAlignment - 1) & ~(kAlignment - 1);
}


static void*
malloc_large(size_t size)
{
	LargeAllocation* allocation = new(std::nothrow) LargeAllocation;
	if (allocation == NULL) {
		dprintf("malloc_large(): Out of memory!\n");
		return NULL;
	}

	if (allocation->Allocate(size) != B_OK) {
		dprintf("malloc_large(): Out of memory!\n");
		delete allocation;
		return NULL;
	}

	sLargeAllocations.InsertUnchecked(allocation);
	TRACE("malloc_large(%lu) -> %p\n", size, allocation->Address());
	return allocation->Address();
}


static void
free_large(void* address)
{
	LargeAllocation* allocation = sLargeAllocations.Lookup(address);
	if (allocation == NULL)
		panic("free_large(%p): unknown allocation!\n", address);

	sLargeAllocations.RemoveUnchecked(allocation);
	allocation->Free();
	delete allocation;
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


void
heap_release(stage2_args* args)
{
	platform_release_heap(args, sHeapBase);
}


void
heap_print_statistics()
{
#ifdef DEBUG_MAX_HEAP_USAGE
	dprintf("maximum boot loader heap usage: %zu, currently used: %zu\n",
		sMaxHeapUsage, sMaxHeapSize - sAvailable);
#endif
}


status_t
heap_init(stage2_args* args)
{
	void* base;
	void* top;
	if (platform_init_heap(args, &base, &top) < B_OK)
		return B_ERROR;

	sHeapBase = base;
	sHeapEnd = top;
	sMaxHeapSize = (uint8*)top - (uint8*)base;

	// declare the whole heap as one chunk, and add it
	// to the free list
	FreeChunk* chunk = (FreeChunk*)base;
	chunk->SetTo(sMaxHeapSize);
	sFreeChunkTree.Insert(chunk);

	sAvailable = chunk->Size();
#ifdef DEBUG_MAX_HEAP_USAGE
	sMaxHeapUsage = sMaxHeapSize - sAvailable;
#endif

	if (sLargeAllocations.Init(64) != B_OK)
		return B_NO_MEMORY;

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

uint32
heap_available(void)
{
	return (uint32)sAvailable;
}


void*
malloc(size_t size)
{
	if (sHeapBase == NULL || size == 0)
		return NULL;

	// align the size requirement to a kAlignment bytes boundary
	if (size < sizeof(FreeChunkData))
		size = sizeof(FreeChunkData);
	size = align(size);

	if (size >= kLargeAllocationThreshold)
		return malloc_large(size);

	if (size > sAvailable) {
		dprintf("malloc(): Out of memory!\n");
		return NULL;
	}

	FreeChunk* chunk = sFreeChunkTree.FindClosest(FreeChunkKey(size), true,
		true);

	if (chunk == NULL) {
		// could not find a free chunk as large as needed
		dprintf("malloc(): Out of memory!\n");
		return NULL;
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

#ifdef DEBUG_MAX_HEAP_USAGE
	sMaxHeapUsage = std::max(sMaxHeapUsage, sMaxHeapSize - sAvailable);
#endif

	TRACE("malloc(%lu) -> %p\n", size, allocatedAddress);
	return allocatedAddress;
}


void*
realloc(void* oldBuffer, size_t newSize)
{
	if (newSize == 0) {
		TRACE("realloc(%p, %lu) -> NULL\n", oldBuffer, newSize);
		free(oldBuffer);
		return NULL;
	}

	size_t oldSize = 0;
	if (oldBuffer != NULL) {
		if (oldBuffer >= sHeapBase && oldBuffer < sHeapEnd) {
			FreeChunk* oldChunk = FreeChunk::SetToAllocated(oldBuffer);
			oldSize = oldChunk->Size();
		} else {
			LargeAllocation* allocation = sLargeAllocations.Lookup(oldBuffer);
			if (allocation == NULL) {
				panic("realloc(%p, %zu): unknown allocation!\n", oldBuffer,
					newSize);
			}

			oldSize = allocation->Size();
		}

		// Check if the old buffer still fits, and if it makes sense to keep it.
		if (oldSize >= newSize
			&& (oldSize < 128 || newSize > oldSize / 3)) {
			TRACE("realloc(%p, %lu) old buffer is large enough\n",
				oldBuffer, newSize);
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

	TRACE("realloc(%p, %lu) -> %p\n", oldBuffer, newSize, newBuffer);
	return newBuffer;
}


void
free(void* allocated)
{
	if (allocated == NULL)
		return;

	TRACE("free(%p)\n", allocated);

	if (allocated < sHeapBase || allocated >= sHeapEnd) {
		free_large(allocated);
		return;
	}

	FreeChunk* freedChunk = FreeChunk::SetToAllocated(allocated);

#ifdef DEBUG_ALLOCATIONS
	if (freedChunk->Size() > sMaxHeapSize - sAvailable) {
		panic("freed chunk %p clobbered (%#zx)!\n", freedChunk,
			freedChunk->Size());
	}
	{
		FreeChunk* chunk = sFreeChunkTree.FindMin();
		while (chunk) {
			if (chunk->Size() > sAvailable || freedChunk == chunk)
				panic("invalid chunk in free list (%p (%zu)), or double free\n",
					chunk, chunk->Size());
			chunk = chunk->Next();
		}
	}
#endif

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
#ifdef DEBUG_MAX_HEAP_USAGE
	sMaxHeapUsage = std::max(sMaxHeapUsage, sMaxHeapSize - sAvailable);
#endif
}

