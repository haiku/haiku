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

#include <new>

#include <OS.h>
#include <assert.h>

#include <locks.h>


// TODO: some kind of adaptive mutex (i.e. trying to spin for a while before
//  may be a better choice
typedef mutex hoardLockType;

namespace BPrivate {

///// Lock-related wrappers.

void hoardLockInit(hoardLockType &lock, const char *name);
void hoardLock(hoardLockType &lock);
void hoardUnlock(hoardLockType &lock);

///// Memory-related wrapper.

void *hoardSbrk(long size);
void hoardUnsbrk(void *ptr, long size);

///// Other.

void hoardYield(void);

}	// namespace BPrivate

#endif // _ARCH_SPECIFIC_H_
