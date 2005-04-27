/*
 * Copyright 2002-2005, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <KernelExport.h>
#include <vm.h>
#include <vm_page.h>
#include <vm_priv.h>

#include <arch/vm.h>
#include <arch/int.h>
#include <arch/cpu.h>

#include <arch/x86/bios.h>


//#define TRACE_ARCH_VM
#ifdef TRACE_ARCH_VM
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


status_t
arch_vm_init(kernel_args *args)
{
	TRACE(("arch_vm_init: entry\n"));
	return 0;
}


status_t
arch_vm_init_post_area(kernel_args *args)
{
	void *dmaAddress;
	area_id id;

	TRACE(("arch_vm_init_post_area: entry\n"));

	// account for DMA area and mark the pages unusable
	vm_mark_page_range_inuse(0x0, 0xa0000 / B_PAGE_SIZE);

	// map 0 - 0xa0000 directly
	id = map_physical_memory("dma_region", (void *)0x0, 0xa0000,
		B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, &dmaAddress);
	if (id < 0) {
		panic("arch_vm_init_post_area: unable to map dma region\n");
		return B_NO_MEMORY;
	}

	return bios_init();
}


status_t
arch_vm_init_end(kernel_args *args)
{
	TRACE(("arch_vm_init_endvm: entry\n"));

	// throw away anything in the kernel_args.pgtable[] that's not yet mapped
	vm_free_unused_boot_loader_range(KERNEL_BASE, 0x400000 * args->arch_args.num_pgtables);

	return B_OK;
}


void
arch_vm_aspace_swap(vm_address_space *aspace)
{
	i386_swap_pgdir((addr_t)i386_translation_map_get_pgdir(&aspace->translation_map));
}


bool
arch_vm_supports_protection(uint32 protection)
{
	// x86 always has the same read/write properties for userland and the kernel.
	// That's why we do not support user-read/kernel-write access. While the
	// other way around is not supported either, we don't care in this case
	// and give the kernel full access.
	if ((protection & (B_READ_AREA | B_WRITE_AREA)) == B_READ_AREA
		&& protection & B_KERNEL_WRITE_AREA)
		return false;

	return true;
}

