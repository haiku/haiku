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

/* Implementations of malloc(), free(), etc. in terms of hoard.
 * This lets us link in hoard in place of the stock malloc
 * (useful to test fragmentation).
 */

#include <string.h>

#include "config.h"
#include "threadheap.h"
#include "processheap.h"
#include "arch-specific.h"

using namespace BPrivate;


inline static processHeap *
getAllocator(void)
{
	static char *buffer = (char *)hoardSbrk(sizeof(processHeap));
	static processHeap *theAllocator = new (buffer) processHeap;

	return theAllocator;
}

#if 0
void * operator new (size_t size)
{
  return HOARD_MALLOC (size);
}
/*
void * operator new (size_t size, const std::nothrow_t&) throw() {
  return HOARD_MALLOC (size);
}
*/
void * operator new[] (size_t size)
{
  return HOARD_MALLOC (size);
}
/*
void * operator new[] (size_t size, const std::nothrow_t&) throw() {
  return HOARD_MALLOC (size);
}
*/
void operator delete (void * ptr)
{
  HOARD_FREE (ptr);
}

void operator delete[] (void * ptr)
{
  HOARD_FREE (ptr);
}
#endif


extern "C" void *
malloc(size_t size)
{
	static processHeap *pHeap = getAllocator();

	void *addr = pHeap->getHeap(pHeap->getHeapIndex()).malloc(size);
	return addr;
}


extern "C" void *
calloc(size_t nelem, size_t elsize)
{
	static processHeap *pHeap = getAllocator();
	void *ptr = pHeap->getHeap(pHeap->getHeapIndex()).malloc(nelem * elsize);

	// Zero out the malloc'd block.
	memset(ptr, 0, nelem * elsize);
	return ptr;
}


extern "C" void
free(void *ptr)
{
	static processHeap *pHeap = getAllocator();
	pHeap->free(ptr);
}


extern "C" void *
memalign(size_t alignment, size_t size)
{
	static processHeap *pHeap = getAllocator();
	void *addr = pHeap->getHeap(pHeap->getHeapIndex()).memalign (alignment, size);

	return addr;
}


extern "C" void *
valloc(size_t size)
{
  return memalign(hoardGetPageSize(), size);
}


extern "C" void *
realloc(void *ptr, size_t sz)
{
	if (ptr == NULL)
		return malloc(sz);

	if (sz == 0) {
		free(ptr);
		return NULL;
	}

	// If the existing object can hold the new size,
	// just return it.

	size_t objSize = threadHeap::objectSize(ptr);
	if (objSize >= sz)
		return ptr;

	// Allocate a new block of size sz.
	void *buffer = malloc(sz);

	// Copy the contents of the original object
	// up to the size of the new block.

	size_t minSize = (objSize < sz) ? objSize : sz;
	memcpy(buffer, ptr, minSize);

	// Free the old block.
	free(ptr);

	// Return a pointer to the new one.
	return buffer;
}

