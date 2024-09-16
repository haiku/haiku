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

#include <string.h>
#include <stdio.h>

#include "config.h"

#if USE_PRIVATE_HEAPS
#	include "privateheap.h"
#	define HEAPTYPE privateHeap
#else
#	define HEAPTYPE threadHeap
#	include "threadheap.h"
#endif

#include "processheap.h"

using namespace BPrivate;


processHeap::processHeap()
	:
	theap((HEAPTYPE*)hoardSbrk(sizeof(HEAPTYPE) * fMaxThreadHeaps)),
#if HEAP_FRAG_STATS
	_currentAllocated(0),
	_currentRequested(0),
	_maxAllocated(0),
	_inUseAtMaxAllocated(0),
	_maxRequested(0),
#endif
#if HEAP_LOG
	_log((Log<MemoryRequest>*)
		hoardSbrk(sizeof(Log<MemoryRequest>) * (fMaxThreadHeaps + 1))),
#endif
	_buffer(NULL),
	_bufferCount(0)
{
	if (theap == NULL)
		return;
	new(theap) HEAPTYPE[fMaxThreadHeaps];

#if HEAP_LOG
	if (_log == NULL)
		return;
	new(_log) Log<MemoryRequest>[fMaxThreadHeaps + 1];
#endif

	int i;
	// The process heap is heap 0.
	setIndex(0);
	for (i = 0; i < fMaxThreadHeaps; i++) {
		// Set every thread's process heap to this one.
		theap[i].setpHeap(this);
		// Set every thread heap's index.
		theap[i].setIndex(i + 1);
	}
#if HEAP_LOG
	for (i = 0; i < fMaxThreadHeaps + 1; i++) {
		char fname[255];
		sprintf(fname, "log%d", i);
		unlink(fname);
		_log[i].open(fname);
	}
#endif
#if HEAP_FRAG_STATS
	hoardLockInit(_statsLock, "hoard stats");
#endif
	hoardLockInit(_bufferLock, "hoard buffer");
}


// Print out statistics information.
void
processHeap::stats(void)
{
#if HEAP_STATS
	int umax = 0;
	int amax = 0;
	for (int j = 0; j < fMaxThreadHeaps; j++) {
		for (int i = 0; i < SIZE_CLASSES; i++) {
			amax += theap[j].maxAllocated(i) * sizeFromClass(i);
			umax += theap[j].maxInUse(i) * sizeFromClass(i);
		}
	}
	printf("Amax <= %d, Umax <= %d\n", amax, umax);

#if HEAP_FRAG_STATS
	amax = getMaxAllocated();
	umax = getMaxRequested();
	printf
	("Maximum allocated = %d\nMaximum in use = %d\nIn use at max allocated = %d\n",
	 amax, umax, getInUseAtMaxAllocated());
	printf("Still in use = %d\n", _currentRequested);
	printf("Fragmentation (3) = %f\n",
		   (float)amax / (float)getInUseAtMaxAllocated());
	printf("Fragmentation (4) = %f\n", (float)amax / (float)umax);
#endif
#endif // HEAP_STATS

#if HEAP_LOG
	printf("closing logs.\n");
	fflush(stdout);
	for (int i = 0; i < fMaxThreadHeaps + 1; i++) {
		_log[i].close();
	}
#endif
}


#if HEAP_FRAG_STATS
void
processHeap::setAllocated(int requestedSize, int actualSize)
{
	hoardLock(_statsLock);
	_currentRequested += requestedSize;
	_currentAllocated += actualSize;
	if (_currentRequested > _maxRequested) {
		_maxRequested = _currentRequested;
	}
	if (_currentAllocated > _maxAllocated) {
		_maxAllocated = _currentAllocated;
		_inUseAtMaxAllocated = _currentRequested;
	}
	hoardUnlock(_statsLock);
}


void
processHeap::setDeallocated(int requestedSize, int actualSize)
{
	hoardLock(_statsLock);
	_currentRequested -= requestedSize;
	_currentAllocated -= actualSize;
	hoardUnlock(_statsLock);
}
#endif	// HEAP_FRAG_STATS


// free (ptr, pheap):
//   inputs: a pointer to an object allocated by malloc().
//   side effects: returns the block to the object's superblock;
//                 updates the thread heap's statistics;
//                 may release the superblock to the process heap.

void
processHeap::free(void *ptr)
{
	// Return if ptr is 0.
	// This is the behavior prescribed by the standard.
	if (ptr == 0)
		return;

	// Find the block and superblock corresponding to this ptr.

	block *b = (block *) ptr - 1;
	assert(b->isValid());

	// Check to see if this block came from a memalign() call.
	if (((unsigned long)b->getNext() & 1) == 1) {
		// It did. Set the block to the actual block header.
		b = (block *) ((unsigned long)b->getNext() & ~1);
		assert(b->isValid());
	}

	b->markFree();

	superblock *sb = b->getSuperblock();
	assert(sb);
	assert(sb->isValid());

	const int sizeclass = sb->getBlockSizeClass();

	//
	// Return the block to the superblock,
	// find the heap that owns this superblock
	// and update its statistics.
	//

	hoardHeap *owner;

	// By acquiring the up lock on the superblock,
	// we prevent it from moving to the global heap.
	// This eventually pins it down in one heap,
	// so this loop is guaranteed to terminate.
	// (It should generally take no more than two iterations.)
	sb->upLock();
	while (1) {
		owner = sb->getOwner();
		owner->lock();
		if (owner == sb->getOwner()) {
			break;
		} else {
			owner->unlock();
		}
		// Suspend to allow ownership to quiesce.
		hoardYield();
	}

#if HEAP_LOG
	MemoryRequest m;
	m.free(ptr);
	getLog(owner->getIndex()).append(m);
#endif
#if HEAP_FRAG_STATS
	setDeallocated(b->getRequestedSize(), 0);
#endif

	int sbUnmapped = owner->freeBlock(b, sb, sizeclass, this);

	owner->unlock();
	if (!sbUnmapped)
		sb->upUnlock();
}
