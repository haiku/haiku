/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <boot/platform.h>
#include <stdlib.h>


status_t
platform_allocate_region(void **_address, size_t size, uint8 protection)
{
	void *address = malloc(size);
	if (address == NULL)
		return B_NO_MEMORY;

	*_address = address;
	return B_OK;
}


status_t
platform_free_region(void *address, size_t size)
{
	free(address);
	return B_OK;
}

