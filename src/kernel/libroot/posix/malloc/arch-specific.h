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

#ifndef _ARCH_SPECIFIC_H_
#define _ARCH_SPECIFIC_H_

#include "config.h"

#include <OS.h>
#include <assert.h>


typedef struct {
	int32	ben;
	sem_id	sem;
} hoardLockType;

inline void *
operator new(size_t, void *_P)
{
	return _P;
}

typedef thread_id hoardThreadType;

namespace BPrivate {
///// Thread-related wrappers.

void hoardCreateThread(hoardThreadType &t,
		void *(*function)(void *), void *arg);
void hoardJoinThread(hoardThreadType &t);
void hoardSetConcurrency(int n);

// Return a thread identifier appropriate for hashing:
// if the system doesn't produce consecutive thread id's,
// some hackery may be necessary.
int	hoardGetThreadID(void);

///// Lock-related wrappers.

void hoardLockInit(hoardLockType &lock, const char *name);
void hoardLock(hoardLockType &lock);
void hoardUnlock(hoardLockType &lock);

///// Memory-related wrapper.

int	hoardGetPageSize(void);
void *hoardSbrk(long size);
void hoardUnsbrk(void *ptr, long size);

///// Other.

void hoardYield(void);
int	hoardGetNumProcessors(void);
unsigned long hoardInterlockedExchange(unsigned long *oldval,
		unsigned long newval);

}	// namespace BPrivate

#endif // _ARCH_SPECIFIC_H_
