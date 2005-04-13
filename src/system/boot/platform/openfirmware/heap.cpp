/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <boot/platform.h>
#include <boot/heap.h>
#include <boot/stdio.h>
#include "openfirmware.h"


#define TRACE_HEAP 1
#if TRACE_HEAP
#	define TRACE(x) printf x
#else
#	define TRACE(x) ;
#endif


status_t
platform_init_heap(stage2_args *args, void **_base, void **_top)
{
	TRACE(("platform_init_heap()\n"));

	int memory;
	if (of_getprop(gChosen, "memory", &memory, sizeof(int)) == OF_FAILED)
		return B_ERROR;

	printf("memory = %d\n", memory);
	struct of_region available;
	int memPackage = of_instance_to_package(memory);
	printf("memPackage = %d\n", memPackage);
	of_getprop(memPackage, "available", &available, sizeof(struct of_region));
	printf("available: base = %p, size = %ld\n", available.base, available.size);

	*_base = of_claim(available.base, args->heap_size, 64);
	printf("heap base = %p\n", *_base);
	*_top = (void *)((int8 *)*_base + args->heap_size);
	printf("heap top = %p\n", *_top);

	return B_OK;
}


void
platform_release_heap(stage2_args *args, void *base)
{
	if (base != NULL)
		of_release(base, args->heap_size);
}

