/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <SupportDefs.h>
#include <boot/platform.h>
#include <boot/heap.h>

#include <string.h>


void _start(void);


// dummy implementations


void
panic(const char *format, ...)
{
}


void
dprintf(const char *format, ...)
{
}


status_t
platform_get_boot_devices(struct stage2_args *args, struct list *devicesList)
{
	return B_OK;
}


void
platform_release_heap(void *base)
{
}


status_t
platform_init_heap(struct stage2_args *args, void **_base, void **_top)
{
}


void
_start(void)
{
}

