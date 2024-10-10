/*
 * Copyright 2016-2024, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <boot/platform.h>

#include "efi_platform.h"


extern "C" ssize_t
platform_allocate_heap_region(size_t _size, void** _base)
{
	size_t pages = (_size + (B_PAGE_SIZE - 1)) / B_PAGE_SIZE;
	efi_physical_addr base;
	if (kBootServices->AllocatePages(AllocateAnyPages, EfiLoaderData, pages, &base) != EFI_SUCCESS)
		return B_NO_MEMORY;

	*_base = (void*)base;
	return pages * B_PAGE_SIZE;
}


extern "C" void
platform_free_heap_region(void* base, size_t size)
{
	kBootServices->FreePages((efi_physical_addr)base, size / B_PAGE_SIZE);
}
