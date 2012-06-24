/*
 * Copyright 2003, Axel Dörfler, axeld@pinc-software.de.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <platform_arch.h>
#include <boot/platform.h>
#include <boot/stdio.h>
#include <boot/platform/cfe/cfe.h>
#include <stdarg.h>


status_t
platform_allocate_region(void **_address, size_t size, uint8 protection,
	bool exactAddress)
{
	if (size == 0)
		return B_BAD_VALUE;

	void *address = arch_mmu_allocate(*_address, size, protection,
		exactAddress);
	if (address == NULL)
		return B_NO_MEMORY;

	*_address = address;
	return B_OK;
}


status_t
platform_free_region(void *address, size_t size)
{
	return arch_mmu_free(address, size);
}


status_t
platform_allocate_elf_region(uint32 *_address, uint32 size, uint8 protection,
	void **_mappedAddress)
{
	if (size == 0)
		return B_BAD_VALUE;

	void *address = arch_mmu_allocate((void *)*_address, size, protection,
		exactAddress);
	if (address == NULL)
		return B_NO_MEMORY;

	*_address = (uint32)address;
	*_mappedAddress = address;
	return B_OK;
}

