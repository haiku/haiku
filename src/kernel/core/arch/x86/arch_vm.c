/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/


#include <KernelExport.h>
#include <kernel.h>
#include <console.h>
#include <vm.h>
#include <vm_page.h>
#include <vm_priv.h>
#include <debug.h>

#include <arch/vm.h>
#include <arch/int.h>
#include <arch/cpu.h>

#include <arch/x86/interrupts.h>

#define TRACE_ARCH_VM 0
#if TRACE_ARCH_VM
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


int
arch_vm_init(kernel_args *ka)
{
	TRACE(("arch_vm_init: entry\n"));
	return 0;
}


int
arch_vm_init2(kernel_args *ka)
{
	TRACE(("arch_vm_init2: entry\n"));

	// account for DMA area and mark the pages unusable
	vm_mark_page_range_inuse(0x0, 0xa0000 / PAGE_SIZE);

	return 0;
}


int
arch_vm_init_endvm(kernel_args *ka)
{
	region_id id;
	void *ptr;

	TRACE(("arch_vm_init_endvm: entry\n"));

	// map 0 - 0xa0000 directly
	id = map_physical_memory("dma_region", (void *)0x0, 0xa0000,
		B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, &ptr);
	if (id < 0) {
		panic("arch_vm_init_endvm: unable to map dma region\n");
		return B_NO_MEMORY;
	}

	return 0;
}


void
arch_vm_aspace_swap(vm_address_space *aspace)
{
	i386_swap_pgdir((addr_t)i386_translation_map_get_pgdir(&aspace->translation_map));
}

