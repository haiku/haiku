/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <boot/platform.h>
#include <stdlib.h>

#define HEAP_SIZE 32768


extern "C" void
platform_release_heap(void *base)
{
	free(base);
}


extern "C" status_t
platform_init_heap(struct stage2_args *args, void **_base, void **_top)
{
	*_base = malloc(HEAP_SIZE);
	*_top = (void *)((uint8 *)*_base + HEAP_SIZE);

	return B_OK;
}


