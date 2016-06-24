/*
 * Copyright 2016 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <boot/platform.h>
#include <boot/stage2.h>

#include "efi_platform.h"


extern "C" status_t
platform_allocate_region(void **_virtualAddress, size_t size, uint8 protection,
	bool exactAddress)
{
	if (kBootServices->AllocatePool(EfiLoaderData, size, _virtualAddress) != EFI_SUCCESS)
		return B_NO_MEMORY;

	return B_OK;
}


extern "C" status_t
platform_free_region(void *address, size_t size)
{
	if (kBootServices->FreePool(address) != EFI_SUCCESS)
		return B_ERROR;

	return B_OK;
}
