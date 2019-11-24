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

/*
  superblock.h
  ------------------------------------------------------------------------
  The superblock class controls a number of blocks (which are
  allocatable units of memory).
  ------------------------------------------------------------------------
  Emery Berger                    | <http://www.cs.utexas.edu/users/emery>
  Department of Computer Sciences |             <http://www.cs.utexas.edu>
  University of Texas at Austin   |                <http://www.utexas.edu>
  ========================================================================
*/

#ifndef _SUPERBLOCK_H_
#define _SUPERBLOCK_H_

#include "config.h"
#include "arch-specific.h"
#include "block.h"


namespace BPrivate {

class hoardHeap;				// forward declaration
class processHeap;				// forward declaration

class superblock {
	public:
		// Construct a superblock for a given size class and set the heap
		// owner.
		superblock(int numblocks, int sizeclass, hoardHeap *owner);
		~superblock(void) {}

		// Make (allocate or re-use) a superblock for a given size class.
		static superblock *makeSuperblock(int sizeclass, processHeap *pHeap);

		// Find out who allocated this superblock.
		inline hoardHeap *getOwner(void);

		// Set the superblock's owner.
		inline void setOwner(hoardHeap *o);

		// Get a block from the superblock.
		inline block *getBlock(void);

		// Put a block back in the superblock.
		inline void putBlock(block *b);

		// How many blocks are available?
		inline int getNumAvailable(void);

		// How many blocks are there, in total?
		inline int getNumBlocks(void);

		// What size class are blocks in this superblock?
		inline int getBlockSizeClass(void);

		// Insert this superblock before the next one.
		inline void insertBefore(superblock *nextSb);

		// Return the next pointer (to the next superblock in the list).
		inline superblock *const getNext(void);

		// Return the prev pointer (to the previous superblock in the list).
		inline superblock *const getPrev(void);

		// Compute the 'fullness' of this superblock.
		inline void computeFullness(void);

		// Return the 'fullness' of this superblock.
		inline int getFullness(void);

#if HEAP_FRAG_STATS
		// Return the amount of waste in every allocated block.
		int getMaxInternalFragmentation(void);
#endif

		// Remove this superblock from its linked list.
		inline void remove(void);

		// Is this superblock valid? (i.e.,
		// does it have the right magic number?)
		inline int isValid(void);

		void
		upLock(void)
		{
			hoardLock(_upLock);
		}

		void
		upUnlock(void)
		{
			hoardUnlock(_upLock);
		}

	private:
		// Disable copying and assignment.

		superblock(const superblock &);
		const superblock & operator=(const superblock &);

		// Used for sanity checking.
		enum { SUPERBLOCK_MAGIC = 0xCAFEBABE };

#if HEAP_DEBUG
		unsigned long _magic;
#endif

		const int _sizeClass;		// The size class of blocks in the superblock.
		const int _numBlocks;		// The number of blocks in the superblock.
		int _numAvailable;			// The number of blocks available.
		int _fullness;				// How full is this superblock?
		// (which SUPERBLOCK_FULLNESS group is it in)
		block *_freeList;			// A pointer to the first free block.
		hoardHeap *_owner;			// The heap who owns this superblock.
		superblock *_next;			// The next superblock in the list.
		superblock *_prev;			// The previous superblock in the list.

		hoardLockType _upLock;		// Lock this when moving a superblock to the global (process) heap.

		// We insert a cache pad here to prevent false sharing with the
		// first block (which immediately follows the superblock).
		double _pad[CACHE_LINE / sizeof(double)];
};


hoardHeap *
superblock::getOwner(void)
{
	assert(isValid());
	hoardHeap *o = _owner;
	return o;
}


void
superblock::setOwner(hoardHeap *o)
{
	assert(isValid());
	_owner = o;
}


block *
superblock::getBlock(void)
{
	assert(isValid());
	// Pop off a block from this superblock's freelist,
	// if there is one available.
	if (_freeList == NULL) {
		// The freelist is empty.
		assert(getNumAvailable() == 0);
		return NULL;
	}

	assert(getNumAvailable() > 0);
	block *b = _freeList;
	_freeList = _freeList->getNext();
	_numAvailable--;

	b->setNext(NULL);

	computeFullness();
	return b;
}


void
superblock::putBlock(block *b)
{
	assert(isValid());
	// Push a block onto the superblock's freelist.
	assert(b->isValid());
	assert(b->getSuperblock() == this);
	assert(getNumAvailable() < getNumBlocks());
	b->setNext(_freeList);
	_freeList = b;
	_numAvailable++;
	computeFullness();
}


int
superblock::getNumAvailable(void)
{
	assert(isValid());
	return _numAvailable;
}


int
superblock::getNumBlocks(void)
{
	assert(isValid());
	return _numBlocks;
}


int
superblock::getBlockSizeClass(void)
{
	assert(isValid());
	return _sizeClass;
}


superblock * const
superblock::getNext(void)
{
	assert(isValid());
	return _next;
}

superblock * const
superblock::getPrev(void)
{
	assert(isValid());
	return _prev;
}


void
superblock::insertBefore(superblock * nextSb)
{
	assert(isValid());
	// Insert this superblock before the next one (nextSb).
	assert(nextSb != this);
	_next = nextSb;
	if (nextSb) {
		_prev = nextSb->_prev;
		nextSb->_prev = this;
	}
}


void
superblock::remove(void)
{
	// Remove this superblock from a doubly-linked list.
	if (_next)
		_next->_prev = _prev;
	if (_prev)
		_prev->_next = _next;

	_prev = NULL;
	_next = NULL;
}


int
superblock::isValid(void)
{
	assert(_numBlocks > 0);
	assert(_numAvailable <= _numBlocks);
	assert(_sizeClass >= 0);
	return 1;
}


void
superblock::computeFullness(void)
{
	assert(isValid());
	_fullness = (((SUPERBLOCK_FULLNESS_GROUP - 1)
		* (getNumBlocks() - getNumAvailable())) / getNumBlocks());
}


int
superblock::getFullness(void)
{
	assert(isValid());
	return _fullness;
}

}	// namespace BPrivate

#endif // _SUPERBLOCK_H_
