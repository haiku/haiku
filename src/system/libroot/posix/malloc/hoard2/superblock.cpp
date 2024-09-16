///-*-C++-*-//////////////////////////////////////////////////////////////////
//
// The Hoard Multiprocessor Memory Allocator
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
  superblock.cpp
  ------------------------------------------------------------------------
  The superblock class controls a number of blocks (which are
  allocatable units of memory).
  ------------------------------------------------------------------------
  Emery Berger                    | <http://www.cs.utexas.edu/users/emery>
  Department of Computer Sciences |             <http://www.cs.utexas.edu>
  University of Texas at Austin   |                <http://www.utexas.edu>
  ========================================================================
*/

#include <string.h>

#include "arch-specific.h"
#include "config.h"
#include "heap.h"
#include "processheap.h"
#include "superblock.h"

using namespace BPrivate;


superblock::superblock(int numBlocks,	// The number of blocks in the sb.
                       int szclass,		// The size class of the blocks.
                       hoardHeap * o)	// The heap that "owns" this sb.
	:
#if HEAP_DEBUG
	_magic(SUPERBLOCK_MAGIC),
#endif
	_sizeClass(szclass),
	_numBlocks(numBlocks),
	_numAvailable(0),
	_fullness(0), _freeList(NULL), _owner(o), _next(NULL), _prev(NULL)
{
	assert(_numBlocks >= 1);

	// Determine the size of each block.
	const int blksize = hoardHeap::align(sizeof(block)
		+ hoardHeap::sizeFromClass(_sizeClass));

	// Make sure this size is in fact aligned.
	assert((blksize & hoardHeap::ALIGNMENT_MASK) == 0);

	// Set the first block to just past this superblock header.
	block *b = (block *) hoardHeap::align((unsigned long)(this + 1));

	// Initialize all the blocks,
	// and insert the block pointers into the linked list.
	for (int i = 0; i < _numBlocks; i++) {
		// Make sure the block is on a double-word boundary.
		assert(((unsigned long)b & hoardHeap::ALIGNMENT_MASK) == 0);
		new(b) block(this);
		assert(b->getSuperblock() == this);
		b->setNext(_freeList);
		_freeList = b;
		b = (block *)((char *)b + blksize);
	}

	_numAvailable = _numBlocks;
	computeFullness();
	assert((unsigned long)b <= hoardHeap::align(sizeof(superblock) + blksize * _numBlocks)
		+ (unsigned long)this);

	hoardLockInit(_upLock, "hoard superblock");
}


superblock *
superblock::makeSuperblock(int sizeclass, processHeap *pHeap)
{
	// We need to get more memory.

	char *buf;
	int numBlocks = hoardHeap::numBlocks(sizeclass);

	// Compute how much memory we need.
	unsigned long moreMemory;
	if (numBlocks > 1) {
		moreMemory = hoardHeap::SUPERBLOCK_SIZE;
		assert(moreMemory >= hoardHeap::align(sizeof(superblock)
			+ (hoardHeap::align(sizeof(block)
			+ hoardHeap::sizeFromClass(sizeclass))) * numBlocks));

		// Get some memory from the process heap.
		buf = (char *)pHeap->getSuperblockBuffer();
	} else {
		// One object.
		assert(numBlocks == 1);

		size_t blksize = hoardHeap::align(sizeof(block)
			+ hoardHeap::sizeFromClass(sizeclass));
		moreMemory = hoardHeap::align(sizeof(superblock) + blksize);

		// Get space from the system.
		buf = (char *)hoardSbrk(moreMemory);
	}

	// Make sure that we actually got the memory.
	if (buf == NULL)
		return 0;

	buf = (char *)hoardHeap::align((unsigned long)buf);

	// Make sure this buffer is double-word aligned.
	assert(buf == (char *)hoardHeap::align((unsigned long)buf));
	assert((((unsigned long)buf) & hoardHeap::ALIGNMENT_MASK) == 0);

	// Instantiate the new superblock in the buffer.
	return new(buf) superblock(numBlocks, sizeclass, NULL);
}
