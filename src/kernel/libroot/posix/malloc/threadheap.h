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

#ifndef _THREADHEAP_H_
#define _THREADHEAP_H_

#include "config.h"

#include <string.h>

#include "heap.h"

namespace BPrivate {

class processHeap;		 // forward declaration

//
// We use one threadHeap for each thread (processor).
//

class threadHeap : public hoardHeap {
	public:
		threadHeap(void);

		// Memory allocation routines.
		void *malloc(const size_t sz);
		inline void *memalign(size_t alignment, size_t sz);

		// Find out how large an allocated object is.
		inline static size_t objectSize(void *ptr);

		// Set our process heap.
		inline void setpHeap(processHeap *p);

	private:
		// Prevent copying and assignment.
		threadHeap(const threadHeap &);
		const threadHeap &operator=(const threadHeap &);

		// Our process heap.
		processHeap *_pHeap;

		// We insert a cache pad here to avoid false sharing (the
		// processHeap holds an array of threadHeaps, and we don't want
		// these to share any cache lines).
		double _pad[CACHE_LINE / sizeof(double)];
};


void *
threadHeap::memalign(size_t alignment, size_t size)
{
	// Calculate the amount of space we need
	// to satisfy the alignment requirements.

	size_t newSize;

	// If the alignment is less than the required alignment,
	// just call malloc.
	if (alignment <= ALIGNMENT)
		return this->malloc(size);

	if (alignment < sizeof(block))
		alignment = sizeof(block);

	// Alignment must be a power of two!
	assert((alignment & (alignment - 1)) == 0);

	// Leave enough room to align the block within the malloced space.
	newSize = size + sizeof(block) + alignment;

	// Now malloc the space up with a little extra (we'll put the block
	// pointer in right behind the allocated space).

	void *ptr = this->malloc(newSize);
	if ((((unsigned long) ptr) & -((long) alignment)) == 0) {
		// ptr is already aligned, so return it.
		assert(((unsigned long) ptr % alignment) == 0);
		return ptr;
	} else {
		// Align ptr.
		char *newptr = (char *)(((unsigned long)ptr + alignment - 1) & -((long)alignment));

		// If there's not enough room for the block header, skip to the
		// next aligned space within the block..
		if ((unsigned long)newptr - (unsigned long)ptr < sizeof(block))
			newptr += alignment;

		assert(((unsigned long)newptr % alignment) == 0);

		// Copy the block from the start of the allocated memory.
		block *b = ((block *)ptr - 1);

		assert(b->isValid());
		assert(b->getSuperblock()->isValid());

		// Make sure there's enough room for the block header.
		assert(((unsigned long)newptr - (unsigned long)ptr) >=
		       sizeof(block));

		block *p = ((block *)newptr - 1);

		// Make sure there's enough room allocated for size bytes.
		assert(((unsigned long)p - sizeof(block)) >= (unsigned long)b);

		if (p != b) {
			assert((unsigned long)newptr > (unsigned long)ptr);
			// Copy the block header.
			*p = *b;
			assert(p->isValid());
			assert(p->getSuperblock()->isValid());

			// Set the next pointer to point to b with the 1 bit set.
			// When this block is freed, it will be treated specially.
			p->setNext((block *)((unsigned long)b | 1));
		} else
			assert(ptr != newptr);

		assert(((unsigned long)ptr + newSize) >=
		       ((unsigned long)newptr + size));
		return newptr;
	}
}


size_t
threadHeap::objectSize(void *ptr)
{
	// Find the superblock pointer.
	block *b = ((block *)ptr - 1);
	assert(b->isValid());
	superblock *sb = b->getSuperblock();
	assert(sb);

	// Return the size.
	return sizeFromClass(sb->getBlockSizeClass());
}


void threadHeap::setpHeap(processHeap *p)
{
	_pHeap = p;
}

}	// namespace BPrivate

#endif				 // _THREADHEAP_H_
