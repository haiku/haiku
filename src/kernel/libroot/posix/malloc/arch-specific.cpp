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

#include <unistd.h>

using namespace BPrivate;


struct free_chunk {
	free_chunk	*next;
	size_t		size;
};

static const size_t kInitialHeapSize = 50 * B_PAGE_SIZE;
	// that's about what hoard allocates anyway

static area_id sHeapArea;
static hoardLockType sHeapLock;
static addr_t sHeapBase;
static size_t sHeapSize, sHeapAreaSize;
static free_chunk *sFreeChunks;


// ToDo: add real fork() support!

extern "C" status_t
__init_heap(void)
{
	sHeapAreaSize = kInitialHeapSize;
	sHeapBase = 0x10000000;
		// let the heap start at 256 MB for now

	// ToDo: add a VM call that instructs other areas to avoid the space after the heap when possible
	//	(and if not, create it at the end of that range, so that the heap can grow as much as possible)
	sHeapArea = create_area("heap", (void **)&sHeapBase, B_BASE_ADDRESS,
		sHeapAreaSize, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);

	hoardLockInit(sHeapLock, "heap");
	return sHeapArea >= B_OK ? B_OK : sHeapArea;
}

namespace BPrivate {

void *
hoardSbrk(long size)
{
	assert(size > 0);

	// align size request
	size = (size + hoardHeap::ALIGNMENT - 1) & ~(hoardHeap::ALIGNMENT - 1);

	hoardLock(sHeapLock);

	// find chunk in free list
	
	free_chunk *chunk = sFreeChunks, *last = NULL;
	for (; chunk != NULL; chunk = chunk->next, last = chunk) {
		if (chunk->size < (size_t)size)
			continue;

		// this chunk is large enough to satisfy the request

		SERIAL_PRINT(("HEAP-%ld: found free chunk to hold %ld bytes\n",
			find_thread(NULL), size));

		void *address = (void *)chunk;

		if (chunk->size + sizeof(free_chunk) > (size_t)size)
			chunk = (free_chunk *)((addr_t)chunk + size);
		else
			chunk = chunk->next;
		
		if (last != NULL)
			last->next = chunk;
		else
			sFreeChunks = chunk;

		hoardUnlock(sHeapLock);
		return address;
	}

	// There was no chunk, let's see if the area is large enough

	size_t oldHeapSize = sHeapSize;
	sHeapSize += size;

	// round to next page size
	size_t pageSize = (sHeapSize + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);

	if (pageSize < sHeapAreaSize) {
		SERIAL_PRINT(("HEAP-%ld: heap area large enough for %ld\n", find_thread(NULL), size));
		// the area is large enough already
		hoardUnlock(sHeapLock);
		return (void *)(sHeapBase + oldHeapSize);
	}

	// We need to grow the area

	SERIAL_PRINT(("HEAP-%ld: need to resize heap area to %ld (%ld requested)\n",
		find_thread(NULL), pageSize, size));

	if (resize_area(sHeapArea, pageSize) < B_OK) {
		// out of memory - ToDo: as a fall back, we could try to allocate another area
		hoardUnlock(sHeapLock);
		return NULL;
	}

	sHeapAreaSize = pageSize;

	hoardUnlock(sHeapLock);
	return (void *)(sHeapBase + oldHeapSize);
}


void
hoardUnsbrk(void *ptr, long size)
{
	// NOT CURRENTLY IMPLEMENTED!
}


void
hoardCreateThread(hoardThreadType &thread,
	void *(*function)(void *), void *arg)
{
	thread = spawn_thread((int32 (*)(void *))function, "hoard thread",
		B_NORMAL_PRIORITY, arg);
	if (thread < B_OK)
		debugger("spawn_thread() failed!");

	resume_thread(thread);
}


void
hoardJoinThread(hoardThreadType &thread)
{
	wait_for_thread(thread, NULL);
}


void
hoardSetConcurrency(int)
{
}


int
hoardGetThreadID(void)
{
	return find_thread(NULL);
}


void
hoardLockInit(hoardLockType &lock, const char *name)
{
	lock.ben = 0;
	lock.sem = create_sem(0, name);
}


void
hoardLock(hoardLockType &lock)
{
	if (atomic_add(&(lock.ben), 1) >= 1)
		acquire_sem(lock.sem);
}


void
hoardUnlock(hoardLockType &lock)
{
	if (atomic_add(&(lock.ben), -1) > 1)
		release_sem(lock.sem);
}


int
hoardGetPageSize(void)
{
	return B_PAGE_SIZE;
}


int
hoardGetNumProcessors(void)
{
	system_info info;
	if (get_system_info(&info) != B_OK)
		return 1;

	return info.cpu_count;
}


void
hoardYield(void)
{
}


unsigned long
hoardInterlockedExchange(unsigned long *oldval,
	unsigned long newval)
{
	// This *should* be made atomic.  It's never used in the BeOS
	// version, so this is included strictly for completeness.
	unsigned long o = *oldval;
	*oldval = newval;
	return o;
}

}	// namespace BPrivate
