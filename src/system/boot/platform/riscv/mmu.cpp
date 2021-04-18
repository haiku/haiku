/*
 * Copyright 2004-2007, Axel Dörfler, axeld@pinc-software.de.
 * Based on code written by Travis Geiselbrecht for NewOS.
 *
 * Distributed under the terms of the MIT License.
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

extern uint8 gStackEnd;

uint8* gMemBase = NULL;
size_t gTotalMem = 0;
uint8* gFreeMem = &gStackEnd;


//	#pragma mark -

extern "C" status_t
platform_allocate_region(void** _address, size_t size, uint8 protection,
	bool exactAddress)
{
	if (exactAddress)
		return B_ERROR;

	gFreeMem = (uint8*)(((addr_t)gFreeMem
		+ (B_PAGE_SIZE - 1)) / B_PAGE_SIZE * B_PAGE_SIZE);
	*_address = gFreeMem;
	gFreeMem += size;

	return B_OK;
}


extern "C" status_t
platform_free_region(void* address, size_t size)
{
	return B_OK;
}


void
platform_release_heap(struct stage2_args* args, void* base)
{
}


status_t
platform_init_heap(struct stage2_args* args, void** _base, void** _top)
{
	void* heap = (void*)gFreeMem;
	gFreeMem += args->heap_size;

	*_base = heap;
	*_top = (void*)((int8*)heap + args->heap_size);
	return B_OK;
}


status_t
platform_bootloader_address_to_kernel_address(void* address, addr_t* _result)
{
	*_result = (addr_t)address;
	return B_OK;
}


status_t
platform_kernel_address_to_bootloader_address(addr_t address, void** _result)
{
	*_result = (void*)address;
	return B_OK;
}


//	#pragma mark -

void
mmu_init(void)
{
}


void
mmu_init_for_kernel(void)
{
	// map in a kernel stack
	void* stack_address = NULL;
	if (platform_allocate_region(&stack_address,
		KERNEL_STACK_SIZE + KERNEL_STACK_GUARD_PAGES * B_PAGE_SIZE, 0, false)
		!= B_OK) {
		panic("Unabled to allocate a stack");
	}
	gKernelArgs.cpu_kstack[0].start = (addr_t)stack_address;
	gKernelArgs.cpu_kstack[0].size = KERNEL_STACK_SIZE
		+ KERNEL_STACK_GUARD_PAGES * B_PAGE_SIZE;
	dprintf("Kernel stack at %#lx\n", gKernelArgs.cpu_kstack[0].start);

	gKernelArgs.physical_memory_range[0].start = (addr_t)gMemBase;
	gKernelArgs.physical_memory_range[0].size = gTotalMem;
	gKernelArgs.num_physical_memory_ranges = 1;

	gKernelArgs.physical_allocated_range[0].start = (addr_t)gMemBase;
	gKernelArgs.physical_allocated_range[0].size = gFreeMem - gMemBase;
	gKernelArgs.num_physical_allocated_ranges = 1;

	gKernelArgs.virtual_allocated_range[0].start = (addr_t)gMemBase;
	gKernelArgs.virtual_allocated_range[0].size = gFreeMem - gMemBase;
	gKernelArgs.num_virtual_allocated_ranges = 1;
}
