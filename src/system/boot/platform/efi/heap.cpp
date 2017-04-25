/*
 * Copyright 2016 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <boot/platform.h>
#include <boot/stage2.h>

#include "efi_platform.h"


#define STAGE_PAGES	0x2000 /* 32 MB */


static EFI_PHYSICAL_ADDRESS staging;


extern "C" void
platform_release_heap(struct stage2_args *args, void *base)
{
	if ((void*)staging != base)
		panic("Attempt to release heap with wrong base address!");

	kBootServices->FreePages(staging, STAGE_PAGES);
}


extern "C" status_t
platform_init_heap(struct stage2_args *args, void **_base, void **_top)
{
	if (kBootServices->AllocatePages(AllocateAnyPages, EfiLoaderData,
			STAGE_PAGES, &staging) != EFI_SUCCESS)
		return B_NO_MEMORY;

	*_base = (void*)staging;
	*_top = (void*)((int8*)staging + STAGE_PAGES * B_PAGE_SIZE);

	return B_OK;
}
