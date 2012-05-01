/*
 * Copyright 2009 Jonas Sundström, jonas@kirilla.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "mmu.h"

#include <boot/platform.h>
#include <boot/stdio.h>
#include <boot/kernel_args.h>
#include <boot/stage2.h>
#include <arch/cpu.h>
#include <arch_kernel.h>
#include <kernel.h>

#include <OS.h>

#include <string.h>


//	#pragma mark -


extern "C" addr_t
mmu_map_physical_memory(addr_t physicalAddress, size_t size, uint32 flags)
{
#warning IMPLEMENT mmu_map_physical_memory
	return 0;
}


extern "C" void*
mmu_allocate(void* virtualAddress, size_t size)
{
#warning IMPLEMENT mmu_allocate
	return 0;
}


extern "C" void
mmu_free(void* virtualAddress, size_t size)
{
#warning IMPLEMENT mmu_free
}


extern "C" void
mmu_init_for_kernel(void)
{
#warning IMPLEMENT mmu_init_for_kernel
}


extern "C" void
mmu_init(void)
{
#warning IMPLEMENT mmu_init
}


//	#pragma mark -


extern "C" status_t
platform_allocate_region(void** _address, size_t size, uint8 protection,
	bool /*exactAddress*/)
{
#warning IMPLEMENT platform_allocate_region
	return B_ERROR;
}


extern "C" status_t
platform_free_region(void* address, size_t size)
{
#warning IMPLEMENT platform_free_region
	return B_ERROR;
}


void
platform_release_heap(struct stage2_args* args, void* base)
{
#warning IMPLEMENT platform_release_heap
}


status_t
platform_init_heap(struct stage2_args* args, void** _base, void** _top)
{
#warning IMPLEMENT platform_init_heap
	return B_ERROR;
}
