/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

#include "VMAnonymousNoSwapCache.h"

#include <stdlib.h>

#include <arch_config.h>
#include <heap.h>
#include <KernelExport.h>
#include <vm_priv.h>


//#define TRACE_STORE
#ifdef TRACE_STORE
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

// The stack functionality looks like a good candidate to put into its own
// store. I have not done this because once we have a swap file backing up
// the memory, it would probably not be a good idea to separate this
// anymore.


VMAnonymousNoSwapCache::~VMAnonymousNoSwapCache()
{
	vm_unreserve_memory(committed_size);
}


status_t
VMAnonymousNoSwapCache::Init(bool canOvercommit, int32 numPrecommittedPages,
	int32 numGuardPages)
{
	TRACE(("VMAnonymousNoSwapCache::Init(canOvercommit = %s, numGuardPages = %ld) "
		"at %p\n", canOvercommit ? "yes" : "no", numGuardPages, store));

	status_t error = VMCache::Init(CACHE_TYPE_RAM);
	if (error != B_OK)
		return error;

	fCanOvercommit = canOvercommit;
	fHasPrecommitted = false;
	fPrecommittedPages = min_c(numPrecommittedPages, 255);
	fGuardedSize = numGuardPages * B_PAGE_SIZE;

	return B_OK;
}


status_t
VMAnonymousNoSwapCache::Commit(off_t size)
{
	// if we can overcommit, we don't commit here, but in anonymous_fault()
	if (fCanOvercommit) {
		if (fHasPrecommitted)
			return B_OK;

		// pre-commit some pages to make a later failure less probable
		fHasPrecommitted = true;
		uint32 precommitted = fPrecommittedPages * B_PAGE_SIZE;
		if (size > precommitted)
			size = precommitted;
	}

	// Check to see how much we could commit - we need real memory

	if (size > committed_size) {
		// try to commit
		if (vm_try_reserve_memory(size - committed_size, 1000000) != B_OK)
			return B_NO_MEMORY;
	} else {
		// we can release some
		vm_unreserve_memory(committed_size - size);
	}

	committed_size = size;
	return B_OK;
}


bool
VMAnonymousNoSwapCache::HasPage(off_t offset)
{
	return false;
}


status_t
VMAnonymousNoSwapCache::Read(off_t offset, const iovec *vecs, size_t count,
	size_t *_numBytes)
{
	panic("anonymous_store: read called. Invalid!\n");
	return B_ERROR;
}


status_t
VMAnonymousNoSwapCache::Write(off_t offset, const iovec *vecs, size_t count,
	uint32 flags, size_t *_numBytes)
{
	// no place to write, this will cause the page daemon to skip this store
	return B_ERROR;
}


status_t
VMAnonymousNoSwapCache::Fault(struct vm_address_space *aspace, off_t offset)
{
	if (fCanOvercommit) {
		if (fGuardedSize > 0) {
			uint32 guardOffset;

#ifdef STACK_GROWS_DOWNWARDS
			guardOffset = 0;
#elif defined(STACK_GROWS_UPWARDS)
			guardOffset = virtual_size - fGuardedSize;
#else
#	error Stack direction has not been defined in arch_config.h
#endif

			// report stack fault, guard page hit!
			if (offset >= guardOffset && offset < guardOffset + fGuardedSize) {
				TRACE(("stack overflow!\n"));
				return B_BAD_ADDRESS;
			}
		}

		if (fPrecommittedPages == 0) {
			// try to commit additional memory
			if (vm_try_reserve_memory(B_PAGE_SIZE, 0) != B_OK)
				return B_NO_MEMORY;

			committed_size += B_PAGE_SIZE;
		} else
			fPrecommittedPages--;
	}

	// This will cause vm_soft_fault() to handle the fault
	return B_BAD_HANDLER;
}


void
VMAnonymousNoSwapCache::MergeStore(VMCache* _source)
{
	VMAnonymousNoSwapCache* source
		= dynamic_cast<VMAnonymousNoSwapCache*>(_source);
	if (source == NULL) {
		panic("VMAnonymousNoSwapCache::MergeStore(): merge with incompatible "
			"cache %p requested", _source);
		return;
	}

	// take over the source' committed size
	committed_size += source->committed_size;
	source->committed_size = 0;

	off_t actualSize = virtual_end - virtual_base;
	if (committed_size > actualSize) {
		vm_unreserve_memory(committed_size - actualSize);
		committed_size = actualSize;
	}
}
