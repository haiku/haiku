/*
 * Copyright 2003-2013, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2005-2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2025, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SIMPLE_ALLOCATOR_H
#define _SIMPLE_ALLOCATOR_H


#include <SupportDefs.h>
#include <util/SplayTree.h>


/*!	This is a very simple malloc()/free() implementation - it only
	manages a free list using a splay tree.

	After heap_init() is called, all free memory is contained in one
	big chunk, the only entry in the free chunk tree.

	When memory is allocated, the smallest free chunk that contains
	the requested size is split (or taken as a whole if it can't be
	splitted anymore), and its lower half will be removed from the
	free list.

	The free list is ordered by size, starting with the smallest
	free chunk available. When a chunk is freed, it will be joined
	with its predecessor or successor, if possible.
*/

template<uint32 Alignment = 8>
class SimpleAllocator {
	class Chunk {
	public:
		size_t CompleteSize() const
		{
			return fSize;
		}

	protected:
		union {
			uint32	fSize;
			char	fAlignment[Alignment];
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
		void SetTo(size_t size)
		{
			Chunk::fSize = size;
			FreeChunkData::fNext = NULL;
		}

		/*!	Returns the amount of bytes that can be allocated
			in this chunk.
		*/
		size_t Size() const
		{
			return (addr_t)this + Chunk::fSize - (addr_t)AllocatedAddress();
		}

		/*!	Splits the upper half at the requested location and returns it. This chunk
			will no longer be a valid FreeChunk object; only its fSize will be valid.
		 */
		FreeChunk* Split(size_t splitSize)
		{
			splitSize = Align(splitSize);

			FreeChunk* chunk = (FreeChunk*)((addr_t)AllocatedAddress() + splitSize);
			size_t newSize = (addr_t)chunk - (addr_t)this;
			chunk->fSize = Chunk::fSize - newSize;
			chunk->fNext = NULL;

			Chunk::fSize = newSize;

			return chunk;
		}

		/*!	Checks if the specified chunk touches this chunk, so
			that they could be joined.
		*/
		bool IsTouching(FreeChunk* chunk)
		{
			return chunk
				&& (((uint8*)this + Chunk::fSize == (uint8*)chunk)
					|| (uint8*)chunk + chunk->fSize == (uint8*)this);
		}

		/*!	Joins the chunk to this chunk and returns the pointer
			to the new chunk - which will either be one of the
			two chunks.
			Note, the chunks must be joinable, or else this method
			doesn't work correctly. Use FreeChunk::IsTouching()
			to check if this method can be applied.
		*/
		FreeChunk* Join(FreeChunk* chunk)
		{
			if (chunk < this) {
				chunk->fSize += Chunk::fSize;
				chunk->fNext = FreeChunkData::fNext;

				return chunk;
			}

			Chunk::fSize += chunk->fSize;
			FreeChunkData::fNext = chunk->fNext;

			return this;
		}

		void* AllocatedAddress() const
		{
			return (void*)static_cast<const FreeChunkData*>(this);
		}

		static FreeChunk* SetToAllocated(void* allocated)
		{
			return static_cast<FreeChunk*>((FreeChunkData*)allocated);
		}
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

public:
	static inline size_t Align(size_t size, size_t alignment = Alignment)
	{
		return (size + alignment - 1) & ~(alignment - 1);
	}

public:
	SimpleAllocator()
		:
		fAvailable(0)
	{
#ifdef DEBUG_MAX_HEAP_USAGE
		fMaxHeapSize = fMaxHeapUsage = 0;
#endif
	}

	~SimpleAllocator()
	{
		// Releasing memory is the caller's responsibility.
	}

	void AddChunk(void* base, uint32 size)
	{
		FreeChunk* chunk = (FreeChunk*)base;
		chunk->SetTo(size);
#ifdef DEBUG_MAX_HEAP_USAGE
		fMaxHeapSize += chunk->Size();
#endif
		_InsertChunk(chunk);
	}

	uint32 Available() const { return fAvailable; }

	void* Allocate(uint32 size)
	{
		if (size == 0)
			return NULL;

		// align the size requirement to an Alignment bytes boundary
		if (size < sizeof(FreeChunkData))
			size = sizeof(FreeChunkData);
		size = Align(size);

		if (size > fAvailable)
			return NULL;

		FreeChunk* chunk = fFreeChunkTree.FindClosest(FreeChunkKey(size), true, true);
		if (chunk == NULL) {
			// could not find a free chunk as large as needed
			return NULL;
		}

		fFreeChunkTree.Remove(chunk);
		fAvailable -= chunk->Size();

		void* allocated = chunk->AllocatedAddress();

		// If this chunk is bigger than the requested size and there's enough space
		// left over for a new chunk, we split it.
		if (chunk->Size() >= (size + Align(sizeof(FreeChunk)))) {
			FreeChunk* freeChunk = chunk->Split(size);
			fFreeChunkTree.Insert(freeChunk);
			fAvailable += freeChunk->Size();
		}

#ifdef DEBUG_MAX_HEAP_USAGE
		fMaxHeapUsage = std::max(fMaxHeapUsage, fMaxHeapSize - fAvailable);
#endif
#ifdef DEBUG_ALLOCATIONS
		memset(allocated, 0xcc, chunk->Size());
#endif

		return allocated;
	}

	uint32 UsableSize(void* allocated)
	{
		FreeChunk* chunk = FreeChunk::SetToAllocated(allocated);
		return chunk->Size();
	}

	void* Reallocate(void* oldBuffer, uint32 newSize)
	{
		size_t oldSize = 0;
		if (oldBuffer != NULL) {
			oldSize = UsableSize(oldBuffer);

			// Check if the old buffer still fits, and if it makes sense to keep it.
			if (oldSize >= newSize && (oldSize < 128 || newSize > (oldSize / 3)))
				return oldBuffer;
		}

		void* newBuffer = Allocate(newSize);
		if (newBuffer == NULL)
			return NULL;

		if (oldBuffer != NULL) {
			memcpy(newBuffer, oldBuffer, (oldSize < newSize) ? oldSize : newSize);
			Free(oldBuffer);
		}

		return newBuffer;
	}

	void Free(void* allocated)
	{
		if (allocated == NULL)
			return;

		FreeChunk* freedChunk = FreeChunk::SetToAllocated(allocated);

#ifdef DEBUG_VALIDATE_HEAP_ON_FREE
		if (freedChunk->Size() > (fMaxHeapSize - fAvailable)) {
			panic("freed chunk %p clobbered (%#zx)!\n", freedChunk,
				freedChunk->Size());
		}
		{
			FreeChunk* chunk = fFreeChunkTree.FindMin();
			while (chunk) {
				if (chunk->Size() > fAvailable || freedChunk == chunk)
					panic("invalid chunk in free list (%p (%zu)), or double free\n",
						chunk, chunk->Size());
				chunk = chunk->Next();
			}
		}
#endif
#ifdef DEBUG_ALLOCATIONS
		for (uint32 i = 0; i < (freedChunk->Size() / 4); i++)
			((uint32*)allocated)[i] = 0xdeadbeef;
#endif

		_InsertChunk(freedChunk);
	}

#ifdef DEBUG_MAX_HEAP_USAGE
	uint32 MaxHeapSize() const { return fMaxHeapSize; }
	uint32 MaxHeapUsage() const { return fMaxHeapUsage; }
#endif

	void DumpChunks()
	{
		FreeChunk* chunk = fFreeChunkTree.FindMin();
		while (chunk != NULL) {
			printf("\t%p: chunk size = %ld, end = %p, next = %p\n", chunk,
				chunk->Size(), (uint8*)chunk + chunk->CompleteSize(),
				chunk->Next());
			chunk = chunk->Next();
		}
	}

private:
	void _InsertChunk(FreeChunk* freedChunk)
	{
		// try to join the new free chunk with an existing one
		// it may be joined with up to two chunks

		FreeChunk* chunk = fFreeChunkTree.FindMin();
		int32 joinCount = 0;

		while (chunk != NULL) {
			FreeChunk* nextChunk = chunk->Next();

			if (chunk->IsTouching(freedChunk)) {
				fFreeChunkTree.Remove(chunk);
				fAvailable -= chunk->Size();

				freedChunk = chunk->Join(freedChunk);

				if (++joinCount == 2)
					break;
			}

			chunk = nextChunk;
		}

		fFreeChunkTree.Insert(freedChunk);
		fAvailable += freedChunk->Size();
#ifdef DEBUG_MAX_HEAP_USAGE
		fMaxHeapUsage = std::max(fMaxHeapUsage, fMaxHeapSize - fAvailable);
#endif
	}

private:
	FreeChunkTree fFreeChunkTree;
	uint32 fAvailable;
#ifdef DEBUG_MAX_HEAP_USAGE
	uint32 fMaxHeapSize, fMaxHeapUsage;
#endif
};


#endif	/* _SIMPLE_ALLOCATOR_H */
