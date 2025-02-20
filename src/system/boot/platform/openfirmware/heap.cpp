/*
 * Copyright 2003, Axel Dörfler, axeld@pinc-software.de.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <OS.h>

#include <boot/platform.h>
#include <boot/heap.h>
#include <boot/stdio.h>
#include <platform/openfirmware/openfirmware.h>


//#define TRACE_HEAP 1
#if TRACE_HEAP
#	define TRACE(x) printf x
#else
#	define TRACE(x) ;
#endif


ssize_t
platform_allocate_heap_region(size_t size, void **_base)
{
	TRACE(("platform_allocate_heap_region()\n"));

	*_base = NULL;
	status_t error = platform_allocate_region(_base, size,
		B_READ_AREA | B_WRITE_AREA);
	if (error != B_OK)
		return error;

	printf("heap base = %p\n", *_base);
	return size;
}


void
platform_free_heap_region(void *_base, size_t size)
{
	if (_base != NULL)
		platform_free_region(_base, size);
}
