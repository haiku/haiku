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

//#include <assert.h>

#include "arch-specific.h"
#include "heap.h"

// How many iterations we spin waiting for a lock.
enum { SPIN_LIMIT = 100 };

// The values of a user-level lock.
enum { UNLOCKED = 0, LOCKED = 1 };

extern "C" {

#include <OS.h>
#include <unistd.h>

using namespace BPrivate;

static area_id heap_region = -1;
static addr_t brk;

int
__heap_init()
{
	// XXX do something better here
	if (heap_region < 0) {
		heap_region = create_area("heap", (void **)&brk,
			B_ANY_ADDRESS, 4*1024*1024, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
	}
	return 0;
}

class heap_init_hack_t {
public:
	heap_init_hack_t(void)
	{
		__heap_init();
	}
} heap_init_hack;


void
hoardCreateThread(hoardThreadType &thread,
	void *(*function)(void *), void *arg)
{
	thread = spawn_thread((int32 (*)(void*))function, "some thread",
			B_NORMAL_PRIORITY, arg);
	if (thread >= B_OK)
		resume_thread(thread);
	else
		debugger("spawn_thread() failed!");
}


void
hoardJoinThread(hoardThreadType &thread)
{
	wait_for_thread(thread, NULL);
}


void
hoardSetConcurrency(int)
{
}


int
hoardGetThreadID(void)
{
	return find_thread(NULL);
}


void
hoardLockInit(hoardLockType &lock, const char *name)
{
	lock.ben = 0;
	lock.sem = create_sem(0, name);
}


void
hoardLock(hoardLockType &lock)
{
	if (atomic_add(&(lock.ben), 1) >= 1)
		acquire_sem(lock.sem);
}


void
hoardUnlock(hoardLockType &lock)
{
	if (atomic_add(&(lock.ben), -1) > 1)
		release_sem(lock.sem);
}


int
hoardGetPageSize(void)
{
	return B_PAGE_SIZE;
}


int
hoardGetNumProcessors(void)
{
	system_info info;
	if (get_system_info(&info) != B_OK)
		return 1;

	return info.cpu_count;
}


void *
hoardSbrk(long size)
{
	void *ret = (void *)brk;
	brk += size + hoardHeap::ALIGNMENT - 1;
	return ret;

//	return sbrk(size + hoardHeap::ALIGNMENT - 1);
}


void
hoardUnsbrk(void * ptr, long size)
{
	// NOT CURRENTLY IMPLEMENTED!
}


void
hoardYield(void)
{
}


unsigned long
hoardInterlockedExchange(unsigned long *oldval,
	unsigned long newval)
{
	// This *should* be made atomic.  It's never used in the BeOS
	// version, so this is included strictly for completeness.
	unsigned long o = *oldval;
	*oldval = newval;
	return o;
}

}
