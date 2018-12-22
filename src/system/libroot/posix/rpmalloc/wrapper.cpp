/*
 * Copyright 2018, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Augustin Cavalier <waddlesplash>
 */

#include <errno.h>
#include <string.h>

#include <errno_private.h>
#include <user_thread.h>

#include "rpmalloc.h"

using namespace BPrivate::rpmalloc;

#if USER_MALLOC_TRACING
#	define KTRACE(format...)	ktrace_printf(format)
#else
#	define KTRACE(format...)	do {} while (false)
#endif


//	#pragma mark - internal functions


extern "C" void
__init_heap()
{
	rpmalloc_initialize();
}


extern "C" void
__heap_terminate_after()
{
	// nothing to do
}


extern "C" void
__heap_before_fork()
{
	// rpmalloc is lock-free; nothing to do
}


extern "C" void
__heap_after_fork_child()
{
	// rpmalloc is lock-free; nothing to do
}


extern "C" void
__heap_after_fork_parent()
{
	// rpmalloc is lock-free; nothing to do
}


extern "C" void
__heap_thread_init()
{
	rpmalloc_thread_initialize();
}


extern "C" void
__heap_thread_exit()
{
	rpmalloc_thread_finalize();
}


//	#pragma mark - public functions


extern "C" void *
malloc(size_t size)
{
	void* addr = rpmalloc(size);
	if (addr == NULL) {
		__set_errno(B_NO_MEMORY);
		KTRACE("malloc(%lu) -> NULL", size);
		return NULL;
	}

	KTRACE("malloc(%lu) -> %p", size, addr);

	return addr;
}


extern "C" void *
calloc(size_t nelem, size_t elsize)
{
	void* addr = rpcalloc(nelem, elsize);
	if (addr == NULL) {
		__set_errno(B_NO_MEMORY);
		KTRACE("calloc(%lu, %lu) -> NULL", nelem, elsize);
		return NULL;
	}

	KTRACE("calloc(%lu, %lu) -> %p", nelem, elsize, addr);

	return addr;
}


extern "C" void
free(void *ptr)
{
	KTRACE("free(%p)", ptr);
	rpfree(ptr);
}


extern "C" void*
memalign(size_t alignment, size_t size)
{
	void* addr = rpmemalign(alignment, size);
	if (addr == NULL) {
		__set_errno(B_NO_MEMORY);
		KTRACE("memalign(%lu, %lu) -> NULL", alignment, size);
		return NULL;
	}

	KTRACE("memalign(%lu, %lu) -> %p", alignment, size, addr);

	return addr;
}


extern "C" int
posix_memalign(void** _pointer, size_t alignment, size_t size)
{
	if (_pointer == NULL)
		return B_BAD_VALUE;

	status_t status = rpposix_memalign(_pointer, alignment, size);
	KTRACE("posix_memalign(%p, %lu, %lu) -> %s, %p", _pointer, alignment,
		size, strerror(status), *_pointer);
	return status;
}


extern "C" void*
valloc(size_t size)
{
	return memalign(B_PAGE_SIZE, size);
}


extern "C" void*
realloc(void* ptr, size_t size)
{
	if (ptr == NULL)
		return malloc(size);
	if (size == 0) {
		free(ptr);
		return NULL;
	}

	void* addr = rprealloc(ptr, size);
	if (addr == NULL) {
		__set_errno(B_NO_MEMORY);
		KTRACE("realloc(%p, %lu) -> NULL", ptr, size);
		return NULL;
	}

	KTRACE("rprealloc(%p, %lu) -> %p", ptr, size, addr);

	return addr;
}


extern "C" size_t
malloc_usable_size(void *ptr)
{
	if (ptr == NULL)
		return 0;

	return rpmalloc_usable_size(ptr);
}


//	#pragma mark - BeOS specific extensions


struct mstats {
	size_t bytes_total;
	size_t chunks_used;
	size_t bytes_used;
	size_t chunks_free;
	size_t bytes_free;
};


extern "C" struct mstats mstats(void);

extern "C" struct mstats
mstats(void)
{
	// TODO
	static struct mstats stats = {0};
	return stats;
}
