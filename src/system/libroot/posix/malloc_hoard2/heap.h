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

/* hoardHeap, the base class for threadHeap and processHeap. */

#ifndef _HEAP_H_
#define _HEAP_H_

#include <OS.h>

#include "config.h"

#include "arch-specific.h"
#include "superblock.h"
#include "heapstats.h"


namespace BPrivate {

class processHeap;

class hoardHeap {
	public:
		hoardHeap(void);

		// A superblock that holds more than one object must hold at least
		// this many bytes.
		enum { SUPERBLOCK_SIZE = 8192 };

		// A thread heap must be at least 1/EMPTY_FRACTION empty before we
		// start returning superblocks to the process heap.
		enum { EMPTY_FRACTION = SUPERBLOCK_FULLNESS_GROUP - 1 };

		// Reset value for the least-empty bin.  The last bin
		// (SUPERBLOCK_FULLNESS_GROUP-1) is for completely full superblocks,
		// so we use the next-to-last bin.
		enum { RESET_LEAST_EMPTY_BIN = SUPERBLOCK_FULLNESS_GROUP - 2 };

		// The number of empty superblocks that we allow any thread heap to
		// hold once the thread heap has fallen below 1/EMPTY_FRACTION
		// empty.
		enum { MAX_EMPTY_SUPERBLOCKS = EMPTY_FRACTION };

		//
		// The number of size classes.  This combined with the
		// SIZE_CLASS_BASE determine the maximum size of an object.
		//
		// NB: Once this is changed, you must execute maketable.cpp and put
		// the generated values into heap.cpp.

#if MAX_INTERNAL_FRAGMENTATION == 2
		enum { SIZE_CLASSES = 115 };
#elif MAX_INTERNAL_FRAGMENTATION == 6
		enum { SIZE_CLASSES = 46 };
#elif MAX_INTERNAL_FRAGMENTATION == 10
		enum { SIZE_CLASSES = 32 };
#else
#	error "Undefined size class base."
#endif

		// Every object is aligned so that it can always hold any type.
#ifdef __x86_64__
		enum { ALIGNMENT = 16 };
#else		
		enum { ALIGNMENT = sizeof(double) };
#endif

		// ANDing with this rounds to ALIGNMENT.
		enum { ALIGNMENT_MASK = ALIGNMENT - 1 };

		// Used for sanity checking.
		enum { HEAP_MAGIC = 0x0badcafe };

		// Get the usage and allocated statistics.
		inline void getStats(int sizeclass, int &U, int &A);


#if HEAP_STATS
		// How much is the maximum ever in use for this size class?
		inline int maxInUse(int sizeclass);

		// How much is the maximum memory allocated for this size class?
		inline int maxAllocated(int sizeclass);
#endif

		// Insert a superblock into our list.
		void insertSuperblock(int sizeclass, superblock *sb, processHeap *pHeap);

		// Remove the superblock with the most free space.
		superblock *removeMaxSuperblock(int sizeclass);

		// Find an available superblock (i.e., with some space in it).
		inline superblock *findAvailableSuperblock(int sizeclass,
								block * &b, processHeap * pHeap);

		// Lock this heap.
		inline void lock(void);

		// Unlock this heap.
		inline void unlock(void);

		// Init this heap lock.
		inline void initLock(void);

		// Set our index number (which heap we are).
		inline void setIndex(int i);

		// Get our index number (which heap we are).
		inline int getIndex(void);

		// Free a block into a superblock.
		// This is used by processHeap::free().
		// Returns 1 iff the superblock was munmapped.
		int freeBlock(block * &b, superblock * &sb, int sizeclass,
				processHeap * pHeap);

		//// Utility functions ////

		// Return the size class for a given size.
		inline static int sizeClass(const size_t sz);

		// Return the size corresponding to a given size class.
		inline static size_t sizeFromClass(const int sizeclass);

		// Return the release threshold corresponding to a given size class.
		inline static int getReleaseThreshold(const int sizeclass);

		// Return how many blocks of a given size class fit into a superblock.
		inline static int numBlocks(const int sizeclass);

		// Align a value.
		inline static size_t align(const size_t sz);

	private:
		// Disable copying and assignment.

		hoardHeap(const hoardHeap &);
		const hoardHeap & operator=(const hoardHeap &);

		// Recycle a superblock.
		inline void recycle(superblock *);

		// Reuse a superblock (if one is available).
		inline superblock *reuse(int sizeclass);

		// Remove a particular superblock.
		void removeSuperblock(superblock *, int sizeclass);

		// Move a particular superblock from one bin to another.
		void moveSuperblock(superblock *,
							int sizeclass, int fromBin, int toBin);

		// Update memory in-use and allocated statistics.
		// (*UStats = just update U.)
		inline void incStats(int sizeclass, int updateU, int updateA);
		inline void incUStats(int sizeclass);

		inline void decStats(int sizeclass, int updateU, int updateA);
		inline void decUStats(int sizeclass);

		//// Members ////

		// Heap statistics.
		heapStats _stats[SIZE_CLASSES];

		// The per-heap lock.
		hoardLockType _lock;

		// Which heap this is (0 = the process (global) heap).
		int _index;

		// Reusable superblocks.
		superblock *_reusableSuperblocks;
		int _reusableSuperblocksCount;

		// Lists of superblocks.
		superblock *_superblocks[SUPERBLOCK_FULLNESS_GROUP][SIZE_CLASSES];

		// The current least-empty superblock bin.
		int _leastEmptyBin[SIZE_CLASSES];

#if HEAP_DEBUG
		// For sanity checking.
		const unsigned long _magic;
#else
#	define _magic HEAP_MAGIC
#endif

		// The lookup table for size classes.
		static size_t _sizeTable[SIZE_CLASSES];

		// The lookup table for release thresholds.
		static size_t _threshold[SIZE_CLASSES];

	public:
		static void initNumProcs(void);

	protected:
		// The maximum number of thread heaps we allow.  (NOT the maximum
		// number of threads -- Hoard imposes no such limit.)  This must be
		// a power of two! NB: This number is twice the maximum number of
		// PROCESSORS supported by Hoard.
		static int fMaxThreadHeaps;

		// number of CPUs, cached
		static int _numProcessors;
		static int _numProcessorsMask;
};



void
hoardHeap::incStats(int sizeclass, int updateU, int updateA)
{
	assert(_magic == HEAP_MAGIC);
	assert(updateU >= 0);
	assert(updateA >= 0);
	assert(sizeclass >= 0);
	assert(sizeclass < SIZE_CLASSES);
	_stats[sizeclass].incStats(updateU, updateA);
}


void
hoardHeap::incUStats(int sizeclass)
{
	assert(_magic == HEAP_MAGIC);
	assert(sizeclass >= 0);
	assert(sizeclass < SIZE_CLASSES);
	_stats[sizeclass].incUStats();
}


void
hoardHeap::decStats(int sizeclass, int updateU, int updateA)
{
	assert(_magic == HEAP_MAGIC);
	assert(updateU >= 0);
	assert(updateA >= 0);
	assert(sizeclass >= 0);
	assert(sizeclass < SIZE_CLASSES);
	_stats[sizeclass].decStats(updateU, updateA);
}


void
hoardHeap::decUStats(int sizeclass)
{
	assert(_magic == HEAP_MAGIC);
	assert(sizeclass >= 0);
	assert(sizeclass < SIZE_CLASSES);
	_stats[sizeclass].decUStats();
}


void
hoardHeap::getStats(int sizeclass, int &U, int &A)
{
	assert(_magic == HEAP_MAGIC);
	assert(sizeclass >= 0);
	assert(sizeclass < SIZE_CLASSES);
	_stats[sizeclass].getStats(U, A);
}


#if HEAP_STATS
int
hoardHeap::maxInUse(int sizeclass)
{
	assert(_magic == HEAP_MAGIC);
	return _stats[sizeclass].getUmax();
}


int
hoardHeap::maxAllocated(int sizeclass)
{
	assert(_magic == HEAP_MAGIC);
	return _stats[sizeclass].getAmax();
}
#endif	// HEAP_STATS


superblock *
hoardHeap::findAvailableSuperblock(int sizeclass,
	block * &b, processHeap * pHeap)
{
	assert(this);
	assert(_magic == HEAP_MAGIC);
	assert(sizeclass >= 0);
	assert(sizeclass < SIZE_CLASSES);

	superblock *sb = NULL;
	int reUsed = 0;

	// Look through the superblocks, starting with the almost-full ones
	// and going to the emptiest ones.  The Least Empty Bin for a
	// sizeclass is a conservative approximation (fixed after one
	// iteration) of the first bin that has superblocks in it, starting
	// with (surprise) the least-empty bin.

	for (int i = _leastEmptyBin[sizeclass]; i >= 0; i--) {
		sb = _superblocks[i][sizeclass];
		if (sb == NULL) {
			if (i == _leastEmptyBin[sizeclass]) {
				// There wasn't a superblock in this bin,
				// so we adjust the least empty bin.
				_leastEmptyBin[sizeclass]--;
			}
		} else if (sb->getNumAvailable() > 0) {
			assert(sb->getOwner() == this);
			break;
		}
		sb = NULL;
	}

#if 1
	if (sb == NULL) {
		// Try to reuse a superblock.
		sb = reuse(sizeclass);
		if (sb) {
			assert(sb->getOwner() == this);
			reUsed = 1;
		}
	}
#endif

	if (sb != NULL) {
		// Sanity checks:
		//   This superblock is 'valid'.
		assert(sb->isValid());
		//   This superblock has the right ownership.
		assert(sb->getOwner() == this);

		int oldFullness = sb->getFullness();

		// Now get a block from the superblock.
		// This superblock must have space available.
		b = sb->getBlock();
		assert(b != NULL);

		// Update the stats.
		incUStats(sizeclass);

		if (reUsed) {
			insertSuperblock(sizeclass, sb, pHeap);
			// Fix the stats (since insert will just have incremented them
			// by this amount).
			decStats(sizeclass,
					 sb->getNumBlocks() - sb->getNumAvailable(),
					 sb->getNumBlocks());
		} else {
			// If we've crossed a fullness group,
			// move the superblock.
			int fullness = sb->getFullness();

			if (fullness != oldFullness) {
				// Move the superblock.
				moveSuperblock(sb, sizeclass, oldFullness, fullness);
			}
		}
	}
	// Either we didn't find a superblock or we did and got a block.
	assert((sb == NULL) || (b != NULL));
	// Either we didn't get a block or we did and we also got a superblock.
	assert((b == NULL) || (sb != NULL));

	return sb;
}


int
hoardHeap::sizeClass(const size_t sz)
{
	// Find the size class for a given object size
	// (the smallest i such that _sizeTable[i] >= sz).
	int sizeclass = 0;
	while (_sizeTable[sizeclass] < sz) {
		sizeclass++;
		assert(sizeclass < SIZE_CLASSES);
	}
	return sizeclass;
}


size_t
hoardHeap::sizeFromClass(const int sizeclass)
{
	assert(sizeclass >= 0);
	assert(sizeclass < SIZE_CLASSES);
	return _sizeTable[sizeclass];
}


int
hoardHeap::getReleaseThreshold(const int sizeclass)
{
	assert(sizeclass >= 0);
	assert(sizeclass < SIZE_CLASSES);
	return _threshold[sizeclass];
}


int
hoardHeap::numBlocks(const int sizeclass)
{
	assert(sizeclass >= 0);
	assert(sizeclass < SIZE_CLASSES);
	const size_t s = sizeFromClass(sizeclass);
	assert(s > 0);
	const int blksize = align(sizeof(block) + s);
	// Compute the number of blocks that will go into this superblock.
	int nb = max_c(1, ((SUPERBLOCK_SIZE - sizeof(superblock)) / blksize));
	return nb;
}


void
hoardHeap::lock(void)
{
	assert(_magic == HEAP_MAGIC);
	hoardLock(_lock);
}


void
hoardHeap::unlock(void)
{
	assert(_magic == HEAP_MAGIC);
	hoardUnlock(_lock);
}


void
hoardHeap::initLock(void)
{
	// Initialize the per-heap lock.
	hoardLockInit(_lock, "hoard heap");
}


size_t
hoardHeap::align(const size_t sz)
{
	// Align sz up to the nearest multiple of ALIGNMENT.
	// This is much faster than using multiplication
	// and division.
	return (sz + ALIGNMENT_MASK) & ~ALIGNMENT_MASK;
}


void
hoardHeap::setIndex(int i)
{
	_index = i;
}


int
hoardHeap::getIndex(void)
{
	return _index;
}


void
hoardHeap::recycle(superblock *sb)
{
	assert(sb != NULL);
	assert(sb->getOwner() == this);
	assert(sb->getNumBlocks() > 1);
	assert(sb->getNext() == NULL);
	assert(sb->getPrev() == NULL);
	assert(hoardHeap::numBlocks(sb->getBlockSizeClass()) > 1);
	sb->insertBefore(_reusableSuperblocks);
	_reusableSuperblocks = sb;
	++_reusableSuperblocksCount;
	// printf ("count: %d => %d\n", getIndex(), _reusableSuperblocksCount);
}


superblock *
hoardHeap::reuse(int sizeclass)
{
	if (_reusableSuperblocks == NULL)
		return NULL;

	// Make sure that we aren't using a sizeclass
	// that is too big for a 'normal' superblock.
	if (hoardHeap::numBlocks(sizeclass) <= 1)
		return NULL;

	// Pop off a superblock from the reusable-superblock list.
	assert(_reusableSuperblocksCount > 0);
	superblock *sb = _reusableSuperblocks;
	_reusableSuperblocks = sb->getNext();
	sb->remove();
	assert(sb->getNumBlocks() > 1);
	--_reusableSuperblocksCount;

	// Reformat the superblock if necessary.
	if (sb->getBlockSizeClass() != sizeclass) {
		decStats(sb->getBlockSizeClass(),
			sb->getNumBlocks() - sb->getNumAvailable(),
			sb->getNumBlocks());

		sb = new((char *)sb) superblock(numBlocks(sizeclass),
			sizeclass, this);

		incStats(sizeclass,
			sb->getNumBlocks() - sb->getNumAvailable(),
			sb->getNumBlocks());
	}

	assert(sb->getOwner() == this);
	assert(sb->getBlockSizeClass() == sizeclass);
	return sb;
}

}	// namespace BPrivate

#endif // _HEAP_H_
