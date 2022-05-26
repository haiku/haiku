/*
 * Copyright 2002-2007, Haiku Inc.
 * Distributed under the terms of the MIT License.
 */

/* Hoard: A Fast, Scalable, and Memory-Efficient Allocator
 * 		for Shared-Memory Multiprocessors
 * Contact author: Emery Berger, http://www.cs.utexas.edu/users/emery
 *
 * Copyright (c) 1998-2000, The University of Texas at Austin.
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation, http://www.fsf.org.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 */

#include "config.h"
#include "threadheap.h"
#include "processheap.h"
#include "arch-specific.h"

#include <image.h>

#include <errno.h>
#include <string.h>

#include <errno_private.h>
#include <user_thread.h>

#include "tracing_config.h"

using namespace BPrivate;


#if USER_MALLOC_TRACING
#	define KTRACE(format...)	ktrace_printf(format)
#else
#	define KTRACE(format...)	do {} while (false)
#endif


#if HEAP_LEAK_CHECK
static block* sUsedList = NULL;
static hoardLockType sUsedLock = MUTEX_INITIALIZER("");


/*!
	Finds the closest symbol that comes before the given address.
*/
static status_t
get_symbol_for_address(void* address, char *imageBuffer, size_t imageBufferSize,
	char* buffer, size_t bufferSize, int32& offset)
{
	offset = -1;

	image_info info;
	int32 cookie = 0;
	while (get_next_image_info(0, &cookie, &info) == B_OK) {
		if (((addr_t)info.text > (addr_t)address
				|| (addr_t)info.text + info.text_size < (addr_t)address)
			&& ((addr_t)info.data > (addr_t)address
				|| (addr_t)info.data + info.data_size < (addr_t)address))
			continue;

		char name[256];
		int32 index = 0;
		int32 nameLength = sizeof(name);
		int32 symbolType;
		void* location;
		while (get_nth_image_symbol(info.id, index, name, &nameLength,
				&symbolType, &location) == B_OK) {
			if ((addr_t)address >= (addr_t)location) {
				// see if this is better than what we have
				int32 newOffset = (addr_t)address - (addr_t)location;

				if (offset == -1 || offset > newOffset) {
					const char* imageName = strrchr(info.name, '/');
					if (imageName != NULL)
						strlcpy(imageBuffer, imageName + 1, imageBufferSize);
					else
						strlcpy(imageBuffer, info.name, imageBufferSize);

					strlcpy(buffer, name, bufferSize);
					offset = newOffset;
				}
			}

			nameLength = sizeof(name);
			index++;
		}
	}

	return offset != -1 ? B_OK : B_ENTRY_NOT_FOUND;
}


static void
dump_block(block* b)
{
	printf("  %p, %ld bytes: call stack", b + 1, b->getAllocatedSize());

	for (int i = 0; i < HEAP_CALL_STACK_SIZE; i++) {
		if (b->getCallStack(i) != NULL) {
			char image[256];
			char name[256];
			int32 offset;
			if (get_symbol_for_address(b->getCallStack(i), image, sizeof(image),
					name, sizeof(name), offset) != B_OK) {
				strcpy(name, "???");
				offset = 0;
			}

			printf(": %p (%s:%s+0x%lx)", b->getCallStack(i), image, name, offset);
		}
	}
	putchar('\n');
}


extern "C" void __dump_allocated(void);

extern "C" void
__dump_allocated(void)
{
	hoardLock(sUsedLock);

	puts("allocated:\n");

	block* b = sUsedList;
	while (b != NULL) {
		dump_block(b);

		b = b->getNext();
	}

	hoardUnlock(sUsedLock);
}


static void
add_address(void* address, size_t size)
{
	block *b = (block *)address - 1;

#ifdef __i386__
	// set call stack
	struct stack_frame {
		struct stack_frame*	previous;
		void*				return_address;
	};

	stack_frame* frame = (stack_frame*)get_stack_frame();

	for (int i = 0; i < HEAP_CALL_STACK_SIZE; i++) {
		if (frame != NULL) {
			b->setCallStack(i, frame->return_address);
			frame = frame->previous;
		} else
			b->setCallStack(i, NULL);
	}

	b->setAllocatedSize(size);
#endif

	hoardLock(sUsedLock);

	b->setNext(sUsedList);
	sUsedList = b;

	hoardUnlock(sUsedLock);
}


static void
remove_address(void* address)
{
	block* b = (block *)address - 1;
	hoardLock(sUsedLock);

	if (sUsedList == b) {
		// we're lucky, it's the first block in the list
		sUsedList = b->getNext();
	} else {
		// search for block in the used list (very slow!)
		block* last = sUsedList;
		while (last != NULL && last->getNext() != b) {
			last = last->getNext();
		}

		if (last == NULL) {
			printf("freed block not in used list!\n");
			dump_block(b);
		} else
			last->setNext(b->getNext());
	}

	hoardUnlock(sUsedLock);
}

#endif	// HEAP_LEAK_CHECK

#if HEAP_WALL

static void*
set_wall(void* addr, size_t size)
{
	size_t *start = (size_t*)addr;

	start[0] = size;
	memset(start + 1, 0x88, HEAP_WALL_SIZE - sizeof(size_t));
	memset((uint8*)addr + size - HEAP_WALL_SIZE, 0x66, HEAP_WALL_SIZE);

	return (uint8*)addr + HEAP_WALL_SIZE;
}


static void*
check_wall(uint8* buffer)
{
	buffer -= HEAP_WALL_SIZE;
	size_t size = *(size_t*)buffer;

	if (threadHeap::objectSize(buffer) < size)
		debugger("invalid size");

	for (size_t i = 0; i < HEAP_WALL_SIZE; i++) {
		if (i >= sizeof(size_t) && buffer[i] != 0x88) {
			debug_printf("allocation %p, size %ld front wall clobbered at byte %ld.\n",
				buffer + HEAP_WALL_SIZE, size - 2 * HEAP_WALL_SIZE, i);
			debugger("front wall clobbered");
		}
		if (buffer[i + size - HEAP_WALL_SIZE] != 0x66) {
			debug_printf("allocation %p, size %ld back wall clobbered at byte %ld.\n",
				buffer + HEAP_WALL_SIZE, size - 2 * HEAP_WALL_SIZE, i);
			debugger("back wall clobbered");
		}
	}

	return buffer;
}

#endif	// HEAP_WALL

inline static processHeap *
getAllocator(void)
{
	static char *buffer = (char *)hoardSbrk(sizeof(processHeap));
	static processHeap *theAllocator = new (buffer) processHeap();

	return theAllocator;
}


extern "C" void
__heap_before_fork(void)
{
	static processHeap *pHeap = getAllocator();
	for (int i = 0; i < pHeap->getMaxThreadHeaps(); i++)
		pHeap->getHeap(i).lock();
}

void __init_after_fork(void);

extern "C" void
__heap_after_fork_child(void)
{
	__init_after_fork();
	static processHeap *pHeap = getAllocator();
	for (int i = 0; i < pHeap->getMaxThreadHeaps(); i++)
		pHeap->getHeap(i).initLock();
}


extern "C" void
__heap_after_fork_parent(void)
{
	static processHeap *pHeap = getAllocator();
	for (int i = 0; i < pHeap->getMaxThreadHeaps(); i++)
		pHeap->getHeap(i).unlock();
}


extern "C" void
__heap_thread_init(void)
{
}


extern "C" void
__heap_thread_exit(void)
{
}


//	#pragma mark - public functions


extern "C" void *
malloc(size_t size)
{
	static processHeap *pHeap = getAllocator();

#if HEAP_WALL
	size += 2 * HEAP_WALL_SIZE;
#endif

	defer_signals();

	void *addr = pHeap->getHeap(pHeap->getHeapIndex()).malloc(size);
	if (addr == NULL) {
		undefer_signals();
		__set_errno(B_NO_MEMORY);
		KTRACE("malloc(%lu) -> NULL", size);
		return NULL;
	}

#if HEAP_LEAK_CHECK
	add_address(addr, size);
#endif

	undefer_signals();

#if HEAP_WALL
	addr = set_wall(addr, size);
#endif

	KTRACE("malloc(%lu) -> %p", size, addr);

	return addr;
}


extern "C" void *
calloc(size_t nelem, size_t elsize)
{
	static processHeap *pHeap = getAllocator();
	size_t size = nelem * elsize;
	void *ptr = NULL;

	if ((nelem > 0) && ((size/nelem) != elsize))
		goto nomem;

#if HEAP_WALL
	size += 2 * HEAP_WALL_SIZE;

	if (nelem == 0 || elsize == 0)
		goto ok;
	if (size < (nelem * size)&& size < (elsize * size))
		goto nomem;

ok:
#endif

	defer_signals();

	ptr = pHeap->getHeap(pHeap->getHeapIndex()).malloc(size);
	if (ptr == NULL) {
		undefer_signals();
	nomem:
		__set_errno(B_NO_MEMORY);
		KTRACE("calloc(%lu, %lu) -> NULL", nelem, elsize);
		return NULL;
	}

#if HEAP_LEAK_CHECK
	add_address(ptr, size);
#endif

	undefer_signals();

#if HEAP_WALL
	ptr = set_wall(ptr, size);
	size -= 2 * HEAP_WALL_SIZE;
#endif

	// Zero out the malloc'd block.
	memset(ptr, 0, size);
	KTRACE("calloc(%lu, %lu) -> %p", nelem, elsize, ptr);
	return ptr;
}


extern "C" void
free(void *ptr)
{
	static processHeap *pHeap = getAllocator();

#if HEAP_WALL
	if (ptr == NULL)
		return;
	KTRACE("free(%p)", ptr);
	ptr = check_wall((uint8*)ptr);
#else
	KTRACE("free(%p)", ptr);
#endif

	defer_signals();

#if HEAP_LEAK_CHECK
	if (ptr != NULL)
		remove_address(ptr);
#endif
	pHeap->free(ptr);

	undefer_signals();
}


extern "C" void *
memalign(size_t alignment, size_t size)
{
	static processHeap *pHeap = getAllocator();

#if HEAP_WALL
	debug_printf("memalign() is not yet supported by the wall code.\n");
	return NULL;
#endif

	defer_signals();

	void *addr = pHeap->getHeap(pHeap->getHeapIndex()).memalign(alignment,
		size);
	if (addr == NULL) {
		undefer_signals();
		__set_errno(B_NO_MEMORY);
		KTRACE("memalign(%lu, %lu) -> NULL", alignment, size);
		return NULL;
	}

#if HEAP_LEAK_CHECK
	add_address(addr, size);
#endif

	undefer_signals();

	KTRACE("memalign(%lu, %lu) -> %p", alignment, size, addr);
	return addr;
}


extern "C" void *
aligned_alloc(size_t alignment, size_t size)
{
	if (size % alignment != 0) {
		__set_errno(B_BAD_VALUE);
		return NULL;
	}
	return memalign(alignment, size);
}


extern "C" int
posix_memalign(void **_pointer, size_t alignment, size_t size)
{
	if ((alignment & (sizeof(void *) - 1)) != 0 || _pointer == NULL)
		return B_BAD_VALUE;

#if HEAP_WALL
	debug_printf("posix_memalign() is not yet supported by the wall code.\n");
	return -1;
#endif
	static processHeap *pHeap = getAllocator();
	defer_signals();
	void *pointer = pHeap->getHeap(pHeap->getHeapIndex()).memalign(alignment,
		size);
	if (pointer == NULL) {
		undefer_signals();
		KTRACE("posix_memalign(%p, %lu, %lu) -> NULL", _pointer, alignment,
			size);
		return B_NO_MEMORY;
	}

#if HEAP_LEAK_CHECK
	add_address(pointer, size);
#endif

	undefer_signals();

	*_pointer = pointer;
	KTRACE("posix_memalign(%p, %lu, %lu) -> %p", _pointer, alignment, size,
		pointer);
	return 0;
}


extern "C" void *
valloc(size_t size)
{
	return memalign(B_PAGE_SIZE, size);
}


extern "C" void *
realloc(void *ptr, size_t size)
{
	if (ptr == NULL)
		return malloc(size);

	if (size == 0) {
		free(ptr);
		return NULL;
	}

	// If the existing object can hold the new size,
	// just return it.

#if HEAP_WALL
	size += 2 * HEAP_WALL_SIZE;
	ptr = (uint8*)ptr - HEAP_WALL_SIZE;
#endif

	size_t objSize = threadHeap::objectSize(ptr);
	if (objSize >= size) {
#if HEAP_WALL
		check_wall((uint8*)ptr + HEAP_WALL_SIZE);
		ptr = set_wall(ptr, size);
#endif
		KTRACE("realloc(%p, %lu) -> %p", ptr, size, ptr);
		return ptr;
	}

#if HEAP_WALL
	size -= 2 * HEAP_WALL_SIZE;
	objSize -= 2 * HEAP_WALL_SIZE;
	ptr = (uint8*)ptr + HEAP_WALL_SIZE;
#endif

	// Allocate a new block of size sz.
	void *buffer = malloc(size);
	if (buffer == NULL) {
		// Allocation failed, leave old block and return
		__set_errno(B_NO_MEMORY);
		KTRACE("realloc(%p, %lu) -> NULL", ptr, size);
		return NULL;
	}

	// Copy the contents of the original object
	// up to the size of the new block.

	size_t minSize = (objSize < size) ? objSize : size;
	memcpy(buffer, ptr, minSize);

	// Free the old block.
	free(ptr);

	// Return a pointer to the new one.
	KTRACE("realloc(%p, %lu) -> %p", ptr, size, buffer);
	return buffer;
}


extern "C" size_t
malloc_usable_size(void *ptr)
{
	if (ptr == NULL)
		return 0;
	return threadHeap::objectSize(ptr);
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
	// Note, the stats structure is not thread-safe, but it doesn't
	// matter that much either
	processHeap *heap = getAllocator();
	static struct mstats stats;

	int allocated = 0;
	int used = 0;
	int chunks = 0;

	for (int i = 0; i < hoardHeap::SIZE_CLASSES; i++) {
		int classUsed, classAllocated;
		heap->getStats(i, classUsed, classAllocated);

		if (classUsed > 0)
			chunks++;

		allocated += classAllocated;
		used += classUsed;
	}

	stats.bytes_total = allocated;
	stats.chunks_used = chunks;
	stats.bytes_used = used;
	stats.chunks_free = hoardHeap::SIZE_CLASSES - chunks;
	stats.bytes_free = allocated - used;

	return stats;
}

