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


status_t
platform_init_heap(stage2_args *args, void **_base, void **_top)
{
	TRACE(("platform_init_heap()\n"));

	*_base = NULL;
	status_t error = platform_allocate_region(_base, args->heap_size,
		B_READ_AREA | B_WRITE_AREA, false);
	if (error != B_OK)
		return error;

	printf("heap base = %p\n", *_base);
	*_top = (void *)((int8 *)*_base + args->heap_size);
	printf("heap top = %p\n", *_top);

	return B_OK;
}


void
platform_release_heap(stage2_args *args, void *base)
{
	if (base != NULL)
		platform_free_region(base, args->heap_size);
}

