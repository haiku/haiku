/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


/*!	A simple allocator that works directly on an area, based on the boot
	loader's heap. See there for more information about its inner workings.
*/


#include <RealtimeAlloc.h>

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <OS.h>

#include <locks.h>
#include <kernel/util/DoublyLinkedList.h>


//#define TRACE_RTM
#ifdef TRACE_RTM
#	define TRACE(x...) printf(x);
#else
#	define TRACE(x...) ;
#endif


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
			void				Remove(rtm_pool* pool,
									FreeChunk* previous = NULL);
			void				Enqueue(rtm_pool* pool);

			void*				AllocatedAddress() const;
	static	FreeChunk*			SetToAllocated(void* allocated);
	static	addr_t				NextOffset() { return sizeof(uint32); }

private:
			uint32				fSize;
			FreeChunk*			fNext;
};


struct rtm_pool : DoublyLinkedListLinkImpl<rtm_pool> {
	area_id		area;
	void*		heap_base;
	size_t		max_size;
	size_t		available;
	FreeChunk	free_anchor;
	mutex		lock;

	bool Contains(void* buffer) const;
	void Free(void* buffer);
};

typedef DoublyLinkedList<rtm_pool> PoolList;


const static uint32 kAlignment = 256;
	// all memory chunks will be a multiple of this

static mutex sPoolsLock = MUTEX_INITIALIZER("rtm pools");
static PoolList sPools;


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
FreeChunk::Remove(rtm_pool* pool, FreeChunk* previous)
{
	if (previous == NULL) {
		// find the previous chunk in the list
		FreeChunk* chunk = pool->free_anchor.fNext;

		while (chunk != NULL && chunk != this) {
			previous = chunk;
			chunk = chunk->fNext;
		}

		if (chunk == NULL)
			return;
	}

	previous->fNext = fNext;
	fNext = NULL;
}


void
FreeChunk::Enqueue(rtm_pool* pool)
{
	FreeChunk* chunk = pool->free_anchor.fNext;
	FreeChunk* last = &pool->free_anchor;
	while (chunk && chunk->Size() < fSize) {
		last = chunk;
		chunk = chunk->fNext;
	}

	fNext = chunk;
	last->fNext = this;
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


// #pragma mark - rtm_pool


bool
rtm_pool::Contains(void* buffer) const
{
	return (addr_t)heap_base <= (addr_t)buffer
		&& (addr_t)heap_base - 1 + max_size >= (addr_t)buffer;
}


void
rtm_pool::Free(void* allocated)
{
	FreeChunk* freedChunk = FreeChunk::SetToAllocated(allocated);
	available += freedChunk->CompleteSize();

	// try to join the new free chunk with an existing one
	// it may be joined with up to two chunks

	FreeChunk* chunk = free_anchor.Next();
	FreeChunk* last = &free_anchor;
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

	freedChunk->Enqueue(this);
}


// #pragma mark -


static rtm_pool*
pool_for(void* buffer)
{
	MutexLocker _(&sPoolsLock);

	PoolList::Iterator iterator = sPools.GetIterator();
	while (rtm_pool* pool = iterator.Next()) {
		if (pool->Contains(buffer))
			return pool;
	}

	return NULL;
}


// #pragma mark - public API


status_t
rtm_create_pool(rtm_pool** _pool, size_t totalSize, const char* name)
{
	rtm_pool* pool = (rtm_pool*)malloc(sizeof(rtm_pool));
	if (pool == NULL)
		return B_NO_MEMORY;

	if (name != NULL)
		mutex_init_etc(&pool->lock, name, MUTEX_FLAG_CLONE_NAME);
	else
		mutex_init(&pool->lock, "realtime pool");

	// Allocate enough space for at least one allocation over \a totalSize
	pool->max_size = (totalSize + sizeof(FreeChunk) - 1 + B_PAGE_SIZE)
		& ~(B_PAGE_SIZE - 1);

	area_id area = create_area(name, &pool->heap_base, B_ANY_ADDRESS,
		pool->max_size, B_LAZY_LOCK, B_READ_AREA | B_WRITE_AREA);
	if (area < 0) {
		mutex_destroy(&pool->lock);
		free(pool);
		return area;
	}

	pool->area = area;
	pool->available = pool->max_size - FreeChunk::NextOffset();

	// declare the whole heap as one chunk, and add it
	// to the free list

	FreeChunk* chunk = (FreeChunk*)pool->heap_base;
	chunk->SetTo(pool->max_size, NULL);

	pool->free_anchor.SetTo(0, chunk);

	*_pool = pool;

	MutexLocker _(&sPoolsLock);
	sPools.Add(pool);
	return B_OK;
}


status_t
rtm_delete_pool(rtm_pool* pool)
{
	if (pool == NULL)
		return B_BAD_VALUE;

	mutex_lock(&pool->lock);

	{
		MutexLocker _(&sPoolsLock);
		sPools.Remove(pool);
	}

	delete_area(pool->area);
	mutex_destroy(&pool->lock);
	free(pool);

	return B_OK;
}


void*
rtm_alloc(rtm_pool* pool, size_t size)
{
	if (pool == NULL)
		return malloc(size);

	if (pool->heap_base == NULL || size == 0)
		return NULL;

	MutexLocker _(&pool->lock);

	// align the size requirement to a kAlignment bytes boundary
	size = (size - 1 + kAlignment) & ~(size_t)(kAlignment - 1);

	if (size > pool->available) {
		TRACE("malloc(): Out of memory!\n");
		return NULL;
	}

	FreeChunk* chunk = pool->free_anchor.Next();
	FreeChunk* last = &pool->free_anchor;
	while (chunk && chunk->Size() < size) {
		last = chunk;
		chunk = chunk->Next();
	}

	if (chunk == NULL) {
		// could not find a free chunk as large as needed
		TRACE("malloc(): Out of memory!\n");
		return NULL;
	}

	if (chunk->Size() > size + sizeof(FreeChunk) + kAlignment) {
		// if this chunk is bigger than the requested size,
		// we split it to form two chunks (with a minimal
		// size of kAlignment allocatable bytes).

		FreeChunk* freeChunk = chunk->Split(size);
		last->SetNext(freeChunk);

		// re-enqueue the free chunk at the correct position
		freeChunk->Remove(pool, last);
		freeChunk->Enqueue(pool);
	} else {
		// remove the chunk from the free list

		last->SetNext(chunk->Next());
	}

	pool->available -= size + sizeof(uint32);

	TRACE("malloc(%lu) -> %p\n", size, chunk->AllocatedAddress());
	return chunk->AllocatedAddress();
}


status_t
rtm_free(void* allocated)
{
	if (allocated == NULL)
		return B_OK;

	TRACE("rtm_free(%p)\n", allocated);

	// find pool
	rtm_pool* pool = pool_for(allocated);
	if (pool == NULL) {
		free(allocated);
		return B_OK;
	}

	MutexLocker _(&pool->lock);
	pool->Free(allocated);
	return B_OK;
}


status_t
rtm_realloc(void** _buffer, size_t newSize)
{
	if (_buffer == NULL)
		return B_BAD_VALUE;

	TRACE("rtm_realloc(%p, %lu)\n", *_buffer, newSize);

	void* oldBuffer = *_buffer;

	// find pool
	rtm_pool* pool = pool_for(oldBuffer);
	if (pool == NULL) {
		void* buffer = realloc(oldBuffer, newSize);
		if (buffer != NULL) {
			*_buffer = buffer;
			return B_OK;
		}
		return B_NO_MEMORY;
	}

	MutexLocker _(&pool->lock);

	if (newSize == 0) {
		TRACE("realloc(%p, %lu) -> NULL\n", oldBuffer, newSize);
		pool->Free(oldBuffer);
		*_buffer = NULL;
		return B_OK;
	}

	size_t copySize = newSize;
	if (oldBuffer != NULL) {
		FreeChunk* oldChunk = FreeChunk::SetToAllocated(oldBuffer);

		// Check if the old buffer still fits, and if it makes sense to keep it
		if (oldChunk->Size() >= newSize && newSize > oldChunk->Size() / 3) {
			TRACE("realloc(%p, %lu) old buffer is large enough\n",
				oldBuffer, newSize);
			return B_OK;
		}

		if (copySize > oldChunk->Size())
			copySize = oldChunk->Size();
	}

	void* newBuffer = rtm_alloc(pool, newSize);
	if (newBuffer == NULL)
		return B_NO_MEMORY;

	if (oldBuffer != NULL) {
		memcpy(newBuffer, oldBuffer, copySize);
		pool->Free(oldBuffer);
	}

	TRACE("realloc(%p, %lu) -> %p\n", oldBuffer, newSize, newBuffer);
	*_buffer = newBuffer;
	return B_OK;
}


status_t
rtm_size_for(void* buffer)
{
	if (buffer == NULL)
		return 0;

	FreeChunk* chunk = FreeChunk::SetToAllocated(buffer);
	// TODO: we currently always return the actual chunk size, not the allocated
	// one
	return chunk->Size();
}


status_t
rtm_phys_size_for(void* buffer)
{
	if (buffer == NULL)
		return 0;

	FreeChunk* chunk = FreeChunk::SetToAllocated(buffer);
	return chunk->Size();
}


size_t
rtm_available(rtm_pool* pool)
{
	if (pool == NULL) {
		// whatever - might want to use system_info instead
		return 1024 * 1024;
	}

	return pool->available;
}


rtm_pool*
rtm_default_pool()
{
	// We always return NULL - the default pool will just use malloc()/free()
	return NULL;
}


#if 0
extern "C" {

// undocumented symbols that BeOS exports
status_t rtm_create_pool_etc(rtm_pool ** out_pool, size_t total_size, const char * name, int32 param4, int32 param5, ...);
void rtm_get_pool(rtm_pool *pool,void *data,int32 param3,int32 param4, ...);

}
#endif
