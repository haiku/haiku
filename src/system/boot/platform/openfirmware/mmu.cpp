/*
 * Copyright 2003, Axel Dörfler, axeld@pinc-software.de.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <platform_arch.h>
#include <boot/platform.h>
#include <boot/stdio.h>
#include <platform/openfirmware/openfirmware.h>
#include <stdarg.h>


status_t
platform_allocate_region(void **_address, size_t size, uint8 protection)
{
	if (size == 0)
		return B_BAD_VALUE;

	void *address = arch_mmu_allocate(*_address, size, protection,
		false);
	if (address == NULL)
		return B_NO_MEMORY;

	*_address = address;
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


status_t
platform_free_region(void *address, size_t size)
{
	return arch_mmu_free(address, size);
}

