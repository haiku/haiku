/*
 * Copyright 2003-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
*/


#include <boot/platform.h>

#include <stdlib.h>
#include <stdio.h>


extern "C" status_t
platform_allocate_region(void **_address, size_t size, uint8 protection)
{
	printf("platform_allocate_region(address = %p, size = %lu, protection = %u, exactAdress = %d)\n",
		*_address, size, protection);

	void *address = malloc(size);
	if (address == NULL)
		return B_NO_MEMORY;

	*_address = address;
	return B_OK;
}


extern "C" status_t
platform_free_region(void *address, size_t size)
{
	free(address);
	return B_OK;
}


extern "C" status_t
platform_bootloader_address_to_kernel_address(void *address, addr_t *_result)
{
	*_result = (addr_t)address;
	return B_OK;
}


extern "C" status_t
platform_kernel_address_to_bootloader_address(addr_t address, void **_result)
{
	*_result = (void*)address;
	return B_OK;
}
