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
  wrapper.cpp
  ------------------------------------------------------------------------
  Implementations of malloc(), free(), etc. in terms of hoard.
  This lets us link in hoard in place of the stock malloc
  (useful to test fragmentation).
  ------------------------------------------------------------------------
  @(#) $Id: wrapper.cpp,v 1.1 2002/10/05 17:13:30 axeld Exp $
  ------------------------------------------------------------------------
  Emery Berger                    | <http://www.cs.utexas.edu/users/emery>
  Department of Computer Sciences |             <http://www.cs.utexas.edu>
  University of Texas at Austin   |                <http://www.utexas.edu>
  ========================================================================
*/

#include <string.h>
#include "config.h"

#if USE_PRIVATE_HEAPS
#include "privateheap.h"
#define HEAPTYPE privateHeap
#else
#include "threadheap.h"
#define HEAPTYPE threadHeap
#endif

#include "processheap.h"
#include "arch-specific.h"


inline static processHeap * getAllocator (void) {
  static char * buf = (char *) hoardSbrk (sizeof(processHeap));
  static processHeap * theAllocator = new (buf) processHeap;
  return theAllocator;
}

#define HOARD_MALLOC(x) malloc(x)
#define HOARD_FREE(x) free(x)
#define HOARD_REALLOC(x,y) realloc(x,y)
#define HOARD_CALLOC(x,y) calloc(x,y)
#define HOARD_MEMALIGN(x,y) memalign(x,y)
#define HOARD_VALLOC(x) valloc(x)

extern "C" void * HOARD_MALLOC(size_t);
extern "C" void HOARD_FREE(void *);
extern "C" void * HOARD_REALLOC(void *, size_t);
extern "C" void * HOARD_CALLOC(size_t, size_t);
extern "C" void * HOARD_MEMALIGN(size_t, size_t);
extern "C" void * HOARD_VALLOC(size_t);


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


extern "C" void * HOARD_MALLOC (size_t sz)
{
  static processHeap * pHeap = getAllocator();
  void * addr = pHeap->getHeap(pHeap->getHeapIndex()).malloc (sz);
  return addr;
}

extern "C" void * HOARD_CALLOC (size_t nelem, size_t elsize)
{
  static processHeap * pHeap = getAllocator();
  void * ptr = pHeap->getHeap(pHeap->getHeapIndex()).malloc (nelem * elsize);
  // Zero out the malloc'd block.
  memset (ptr, 0, nelem * elsize);
  return ptr;
}

extern "C" void HOARD_FREE (void * ptr)
{
  static processHeap * pHeap = getAllocator();
#if USE_PRIVATE_HEAPS
  pHeap->getHeap(pHeap->getHeapIndex()).free(ptr);
#else
  pHeap->free (ptr);
#endif
}


extern "C" void * HOARD_MEMALIGN (size_t alignment, size_t size)
{
  static processHeap * pHeap = getAllocator();
  void * addr = pHeap->getHeap(pHeap->getHeapIndex()).memalign (alignment, size);
  return addr;
}


extern "C" void * HOARD_VALLOC (size_t size)
{
  return HOARD_MEMALIGN (hoardGetPageSize(), size);
}


extern "C" void * HOARD_REALLOC (void * ptr, size_t sz)
{
  if (ptr == NULL) {
    return HOARD_MALLOC (sz);
  }
  if (sz == 0) {
    HOARD_FREE (ptr);
    return NULL;
  }

  // If the existing object can hold the new size,
  // just return it.

  size_t objSize = HEAPTYPE::objectSize (ptr);

  if (objSize >= sz) {
    return ptr;
  }

  // Allocate a new block of size sz.

  void * buf = HOARD_MALLOC (sz);

  // Copy the contents of the original object
  // up to the size of the new block.

  size_t minSize = (objSize < sz) ? objSize : sz;
  memcpy (buf, ptr, minSize);

  // Free the old block.

  HOARD_FREE (ptr);

  // Return a pointer to the new one.

  return buf;
}


#ifdef WIN32

// Replace the CRT debugging allocation routines, just to be on the safe side.
// This is not a complete solution, but should avoid inadvertent mixing of allocations.

extern "C" void * _malloc_dbg (size_t sz, int, const char *, int) {
	return HOARD_MALLOC (sz);
}

void * operator new(
        unsigned int cb,
        int nBlockUse,
        const char * szFileName,
        int nLine
        )
{
	return HOARD_MALLOC (cb);
}

extern "C" void * _calloc_dbg (size_t num, size_t size, int, const char *, int) {
	return HOARD_CALLOC (num, size);
}

extern "C" void * _realloc_dbg (void * ptr, size_t newSize, int, const char *, int) {
	return HOARD_REALLOC (ptr, newSize);
}

extern "C" void _free_dbg (void * ptr, int) {
	HOARD_FREE (ptr);
}

extern "C" size_t _msize_dbg (void * ptr) {
  // Find the block and superblock corresponding to this ptr.

  block * b = (block *) ptr - 1;
  assert (b->isValid());

  // Check to see if this block came from a memalign() call.
  if (((unsigned long) b->getNext() & 1) == 1) {
    // It did. Set the block to the actual block header.
    b = (block *) ((unsigned long) b->getNext() & ~1);
    assert (b->isValid());
  }

  superblock * sb = b->getSuperblock();
  assert (sb);
  assert (sb->isValid());

  const int sizeclass = sb->getBlockSizeClass();
  return hoardHeap::sizeFromClass (sizeclass);
}

extern "C" size_t _msize (void * ptr) {
  // Find the block and superblock corresponding to this ptr.

  block * b = (block *) ptr - 1;
  assert (b->isValid());

  // Check to see if this block came from a memalign() call.
  if (((unsigned long) b->getNext() & 1) == 1) {
    // It did. Set the block to the actual block header.
    b = (block *) ((unsigned long) b->getNext() & ~1);
    assert (b->isValid());
  }

  superblock * sb = b->getSuperblock();
  assert (sb);
  assert (sb->isValid());

  const int sizeclass = sb->getBlockSizeClass();
  return hoardHeap::sizeFromClass (sizeclass);
}

extern "C" void * _nh_malloc_base (size_t sz, int) {
	return HOARD_MALLOC (sz);
}

extern "C" void * _nh_malloc_dbg (size_t sz, size_t, int, int, const char *, int) {
	return HOARD_MALLOC (sz);
}

#endif // WIN32


#if 0
extern "C" void malloc_stats (void)
{
  TheWrapper.TheAllocator()->stats();
}
#endif
