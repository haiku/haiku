/*
 * Copyright 2006-2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "accelerant.h"
#include "intel_extreme.h"

#include <errno.h>
#include <unistd.h>


//#define TRACE_MEMORY
#ifdef TRACE_MEMORY
extern "C" void _sPrintf(const char* format, ...);
#	define TRACE(x) _sPrintf x
#else
#	define TRACE(x) ;
#endif


void
intel_free_memory(uint32 base)
{
	if (base == 0)
		return;

	intel_free_graphics_memory freeMemory;
	freeMemory.magic = INTEL_PRIVATE_DATA_MAGIC;
	freeMemory.buffer_base = base;

	ioctl(gInfo->device, INTEL_FREE_GRAPHICS_MEMORY, &freeMemory,
		sizeof(freeMemory));
}


status_t
intel_allocate_memory(size_t size, uint32 flags, uint32 &base)
{
	intel_allocate_graphics_memory allocMemory;
	allocMemory.magic = INTEL_PRIVATE_DATA_MAGIC;
	allocMemory.size = size;
	allocMemory.alignment = 0;
	allocMemory.flags = flags;

	if (ioctl(gInfo->device, INTEL_ALLOCATE_GRAPHICS_MEMORY, &allocMemory,
			sizeof(allocMemory)) < 0)
		return errno;

	base = allocMemory.buffer_base;
	return B_OK;
}

