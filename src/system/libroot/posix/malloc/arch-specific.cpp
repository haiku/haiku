///-*-C++-*-//////////////////////////////////////////////////////////////////
//
// Hoard: A Fast, Scalable, and Memory-Efficient Allocator
//        for Shared-Memory Multiprocessors
// Contact author: Emery Berger, http://www.cs.utexas.edu/users/emery
//
// Copyright (c) 1998-2000, The University of Texas at Austin.
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Library General Public License as
// published by the Free Software Foundation, http://www.fsf.org.
//
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
//////////////////////////////////////////////////////////////////////////////

#include "arch-specific.h"
#include "heap.h"

#include <OS.h>
#include <Debug.h>
#include <syscalls.h>

#include <stdlib.h>
#include <unistd.h>

//#define TRACE_CHUNKS
#ifdef TRACE_CHUNKS
#	define CTRACE(x) debug_printf x
#else
#	define CTRACE(x) ;
#endif

extern "C" void *(*sbrk_hook)(long);
void *(*sbrk_hook)(long) = &BPrivate::hoardSbrk;

using namespace BPrivate;

// How many iterations we spin waiting for a lock.
enum { SPIN_LIMIT = 50 };

// The values of a user-level lock.
enum { UNLOCKED = 0, LOCKED = 1 };

struct free_chunk {
	free_chunk	*next;
	size_t		size;
};


static const size_t kInitialHeapSize = 64 * B_PAGE_SIZE;
	// that's about what hoard allocates anyway (should be kHeapIncrement
	// aligned)

static const size_t kHeapIncrement = 16 * B_PAGE_SIZE;
	// the steps in which to increase the heap size (must be a power of 2)

#if B_HAIKU_64_BIT
static const addr_t kHeapReservationBase = 0x1000000000;
static const addr_t kHeapReservationSize = 0x1000000000;
#else
static const addr_t kHeapReservationBase = 0x18000000;
static const addr_t kHeapReservationSize = 0x48000000;
#endif

static area_id sHeapArea;
static hoardLockType sHeapLock;
static void *sHeapBase;
static addr_t sFreeHeapBase;
static size_t sFreeHeapSize, sHeapAreaSize;
static free_chunk *sFreeChunks;


static void
init_after_fork(void)
{
	// find the heap area
	sHeapArea = area_for((void*)sFreeHeapBase);
	if (sHeapArea < 0) {
		// Where is it gone?
		debug_printf("hoard: init_after_fork(): thread %" B_PRId32 ", Heap "
			"area not found! Base address: %p\n", find_thread(NULL),
			sHeapBase);
		exit(1);
	}
}


extern "C" status_t
__init_heap(void)
{
	hoardHeap::initNumProcs();

	// This will locate the heap base at 384 MB and reserve the next 1152 MB
	// for it. They may get reclaimed by other areas, though, but the maximum
	// size of the heap is guaranteed until the space is really needed.
	sHeapBase = (void *)kHeapReservationBase;
	status_t status = _kern_reserve_address_range((addr_t *)&sHeapBase,
		B_EXACT_ADDRESS, kHeapReservationSize);
	if (status != B_OK)
		sHeapBase = NULL;

	sHeapArea = create_area("heap", (void **)&sHeapBase,
		status == B_OK ? B_EXACT_ADDRESS : B_BASE_ADDRESS,
		kInitialHeapSize, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
	if (sHeapArea < B_OK)
		return sHeapArea;

	sFreeHeapBase = (addr_t)sHeapBase;
	sHeapAreaSize = kInitialHeapSize;

	hoardLockInit(sHeapLock, "heap");

	atfork(&init_after_fork);
		// Note: Needs malloc(). Hence we need to be fully initialized.
		// TODO: We should actually also install a hook that is called before
		// fork() is being executed. In a multithreaded app it would need to
		// acquire *all* allocator locks, so that we don't fork() an
		// inconsistent state.

	return B_OK;
}


extern "C" void
__init_heap_post_env(void)
{
	// no heap options available
}


static void
insert_chunk(free_chunk *newChunk)
{
	free_chunk *chunk = (free_chunk *)sFreeChunks, *smaller = NULL;
	for (; chunk != NULL; chunk = chunk->next) {
		if (chunk->size < newChunk->size)
			smaller = chunk;
		else
			break;
	}

	if (smaller) {
		newChunk->next = smaller->next;
		smaller->next = newChunk;
	} else {
		newChunk->next = sFreeChunks;
		sFreeChunks = newChunk;
	}
}


namespace BPrivate {

void *
hoardSbrk(long size)
{
	assert(size > 0);
	CTRACE(("sbrk: size = %ld\n", size));

	// align size request
	size = (size + hoardHeap::ALIGNMENT - 1) & ~(hoardHeap::ALIGNMENT - 1);

	hoardLock(sHeapLock);

	// find chunk in free list
	free_chunk *chunk = sFreeChunks, *last = NULL;
	for (; chunk != NULL; chunk = chunk->next) {
		CTRACE(("  chunk %p (%ld)\n", chunk, chunk->size));

		if (chunk->size < (size_t)size) {
			last = chunk;
			continue;
		}

		// this chunk is large enough to satisfy the request

		SERIAL_PRINT(("HEAP-%ld: found free chunk to hold %ld bytes\n",
			find_thread(NULL), size));

		void *address = (void *)chunk;

		if (chunk->size > (size_t)size + sizeof(free_chunk)) {
			// divide this chunk into smaller bits
			size_t newSize = chunk->size - size;
			free_chunk *next = chunk->next;

			chunk = (free_chunk *)((addr_t)chunk + size);
			chunk->next = next;
			chunk->size = newSize;

			if (last != NULL) {
				last->next = next;
				insert_chunk(chunk);
			} else
				sFreeChunks = chunk;
		} else {
			chunk = chunk->next;

			if (last != NULL)
				last->next = chunk;
			else
				sFreeChunks = chunk;
		}

		hoardUnlock(sHeapLock);
		return address;
	}

	// There was no chunk, let's see if the area is large enough

	size_t oldHeapSize = sFreeHeapSize;
	sFreeHeapSize += size;

	// round to next heap increment aligned size
	size_t incrementAlignedSize = (sFreeHeapSize + kHeapIncrement - 1)
		& ~(kHeapIncrement - 1);

	if (incrementAlignedSize <= sHeapAreaSize) {
		SERIAL_PRINT(("HEAP-%ld: heap area large enough for %ld\n",
			find_thread(NULL), size));
		// the area is large enough already
		hoardUnlock(sHeapLock);
		return (void *)(sFreeHeapBase + oldHeapSize);
	}

	// We need to grow the area

	SERIAL_PRINT(("HEAP-%ld: need to resize heap area to %ld (%ld requested)\n",
		find_thread(NULL), incrementAlignedSize, size));

	status_t status = resize_area(sHeapArea, incrementAlignedSize);
	if (status != B_OK) {
		// Either the system is out of memory or another area is in the way and
		// prevents ours from being resized. As a special case of the latter
		// the user might have mmap()ed something over malloc()ed memory. This
		// splits the heap area in two, the first one retaining the original
		// area ID. In either case, if there's still memory, it is a good idea
		// to try and allocate a new area.
		sFreeHeapSize = oldHeapSize;

		if (status == B_NO_MEMORY) {
			hoardUnlock(sHeapLock);
			return NULL;
		}

		size_t newHeapSize = (size + kHeapIncrement - 1) / kHeapIncrement
			* kHeapIncrement;

		// First try at the location directly after the current heap area, if
		// that is still in the reserved memory region.
		void* base = (void*)(sFreeHeapBase + sHeapAreaSize);
		area_id area = -1;
		if (sHeapBase != NULL
			&& base >= sHeapBase
			&& (addr_t)base + newHeapSize
				<= (addr_t)sHeapBase + kHeapReservationSize) {
			area = create_area("heap", &base, B_EXACT_ADDRESS, newHeapSize,
				B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);

			if (area == B_NO_MEMORY) {
				hoardUnlock(sHeapLock);
				return NULL;
			}
		}

		// If we don't have an area yet, try again with a free location
		// allocation.
		if (area < 0) {
			base = (void*)(sFreeHeapBase + sHeapAreaSize);
			area = create_area("heap", &base, B_BASE_ADDRESS, newHeapSize,
				B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
		}

		if (area < 0) {
			hoardUnlock(sHeapLock);
			return NULL;
		}

		// We have a new area, so make it the new heap area.
		sHeapArea = area;
		sFreeHeapBase = (addr_t)base;
		sHeapAreaSize = newHeapSize;
		sFreeHeapSize = size;
		oldHeapSize = 0;
	} else
		sHeapAreaSize = incrementAlignedSize;

	hoardUnlock(sHeapLock);
	return (void *)(sFreeHeapBase + oldHeapSize);
}


void
hoardUnsbrk(void *ptr, long size)
{
	CTRACE(("unsbrk: %p, %ld!\n", ptr, size));

	hoardLock(sHeapLock);

	// TODO: hoard always allocates and frees in typical sizes, so we could
	//	save a lot of effort if we just had a similar mechanism

	// We add this chunk to our free list - first, try to find an adjacent
	// chunk, so that we can merge them together

	free_chunk *chunk = (free_chunk *)sFreeChunks, *last = NULL, *smaller = NULL;
	for (; chunk != NULL; chunk = chunk->next) {
		if ((addr_t)chunk + chunk->size == (addr_t)ptr
			|| (addr_t)ptr + size == (addr_t)chunk) {
			// chunks are adjacent - merge them

			CTRACE(("  found adjacent chunks: %p, %ld\n", chunk, chunk->size));
			if (last)
				last->next = chunk->next;
			else
				sFreeChunks = chunk->next;

			if ((addr_t)chunk < (addr_t)ptr)
				chunk->size += size;
			else {
				free_chunk *newChunk = (free_chunk *)ptr;
				newChunk->next = chunk->next;
				newChunk->size = size + chunk->size;
				chunk = newChunk;
			}

			insert_chunk(chunk);
			hoardUnlock(sHeapLock);
			return;
		}

		last = chunk;

		if (chunk->size < (size_t)size)
			smaller = chunk;
	}

	// we didn't find an adjacent chunk, so insert the new chunk into the list

	free_chunk *newChunk = (free_chunk *)ptr;
	newChunk->size = size;
	if (smaller) {
		newChunk->next = smaller->next;
		smaller->next = newChunk;
	} else {
		newChunk->next = sFreeChunks;
		sFreeChunks = newChunk;
	}

	hoardUnlock(sHeapLock);
}


void
hoardLockInit(hoardLockType &lock, const char *name)
{
	lock = UNLOCKED;
}


void
hoardLock(hoardLockType &lock)
{
	// A yielding lock (with an initial spin).
	while (true) {
		int32 i = 0;
		while (i < SPIN_LIMIT) {
			if (atomic_test_and_set(&lock, LOCKED, UNLOCKED) == UNLOCKED) {
				// We got the lock.
				return;
			}
			i++;
		}

		// The lock is still being held by someone else.
		// Give up our quantum.
		hoardYield();
	}
}


void
hoardUnlock(hoardLockType &lock)
{
	atomic_set(&lock, UNLOCKED);
}


void
hoardYield(void)
{
	_kern_thread_yield();
}

}	// namespace BPrivate
