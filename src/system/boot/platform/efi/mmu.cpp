/*
 * Copyright 2016 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <boot/platform.h>
#include <boot/stage2.h>


extern "C" status_t
platform_allocate_region(void **_virtualAddress, size_t size, uint8 protection,
	bool exactAddress)
{
	panic("platform_allocate_region not implemented");
	return B_ERROR;
}


extern "C" status_t
platform_free_region(void *address, size_t size)
{
	panic("platform_free_region not implemented");
	return B_ERROR;
}
