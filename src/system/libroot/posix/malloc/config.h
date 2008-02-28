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
#ifndef _CONFIG_H_
#define _CONFIG_H_

#ifndef _REENTRANT
#	define _REENTRANT		// If defined, generate a multithreaded-capable version.
#endif

#ifndef USER_LOCKS
#	define USER_LOCKS 1		// Use our own user-level locks if they're available for the current architecture.
#endif

#define HEAP_LOG 0		// If non-zero, keep a log of heap accesses.


///// You should not change anything below here. /////


// The base of the exponential used for size classes.
// An object is in size class i if
//        base^(b+i-1) * ALIGNMENT < size <= base^(b+i) * ALIGNMENT,
// where b = log_base(ALIGNMENT).
// Note that this puts an upper-limit on internal fragmentation:
//   if SIZE_CLASS_BASE is 1.2, then we will never see more than
//   20% internal fragmentation (for aligned requests).

#define SIZE_CLASS_BASE 1.2
#define MAX_INTERNAL_FRAGMENTATION 2

// The number of groups of superblocks we maintain based on what
// fraction of the superblock is empty. NB: This number must be at
// least 2, and is 1 greater than the EMPTY_FRACTION in heap.h.

enum { SUPERBLOCK_FULLNESS_GROUP = 9 };


// DO NOT CHANGE THESE.  They require running of maketable to replace
// the values in heap.cpp for the _numBlocks array.

#define HEAP_DEBUG 0		// If non-zero, keeps extra info for sanity checking.
#define HEAP_STATS 0		// If non-zero, maintain blowup statistics.
#define HEAP_FRAG_STATS 0	// If non-zero, maintain fragmentation statistics.

// A simple (and slow) leak checker
#define HEAP_LEAK_CHECK 0
#define HEAP_CALL_STACK_SIZE 8

// A simple wall checker
#define HEAP_WALL 0
#define HEAP_WALL_SIZE 32

// CACHE_LINE = The number of bytes in a cache line.

#if defined(i386) || defined(WIN32)
#	define CACHE_LINE 32
#endif

#ifdef sparc
#	define CACHE_LINE 64
#endif

#ifdef __sgi
#	define CACHE_LINE 128
#endif

#ifndef CACHE_LINE
// We don't know what the architecture is,
// so go for the gusto.
#define CACHE_LINE 64
#endif

#ifdef __GNUG__
// Use the max operator, an extension to C++ found in GNU C++.
#	define MAX(a,b) ((a) >? (b))
#else
#	define MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif


#endif // _CONFIG_H_
