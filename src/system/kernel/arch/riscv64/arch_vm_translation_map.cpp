/*
 * Copyright 2007-2010, François Revol, revol@free.fr.
 * Copyright 2008-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de. All rights
 *   reserved.
 * Copyright 2019, Adrien Destugues, pulkomandy@pulkomandy.tk.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <arch_cpu_defs.h>
#include <boot/kernel_args.h>
#include <KernelExport.h>
#include <kernel.h>
#include <vm/vm.h>
#include <vm/vm_priv.h>
#include <vm/VMAddressSpace.h>
#include <Clint.h>
#include <Htif.h>
#include <Plic.h>

#include "RISCV64VMTranslationMap.h"


#define TRACE_VM_TMAP
#ifdef TRACE_VM_TMAP
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...) ;
#endif


ssize_t gVirtFromPhysOffset = 0;

phys_addr_t sPageTable = 0;
char sPhysicalPageMapperData[sizeof(RISCV64VMPhysicalPageMapper)];


// TODO: Consolidate function with RISCV64VMTranslationMap

static Pte*
LookupPte(addr_t virtAdr, bool alloc, kernel_args* args,
	phys_addr_t (*get_free_page)(kernel_args *))
{
	Pte *pte = (Pte*)VirtFromPhys(sPageTable);
	for (int level = 2; level > 0; level --) {
		pte += VirtAdrPte(virtAdr, level);
		if (!((1 << pteValid) & pte->flags)) {
			if (!alloc)
				return NULL;
			pte->ppn = get_free_page(args);
			if (pte->ppn == 0)
				return NULL;
			memset((Pte*)VirtFromPhys(B_PAGE_SIZE * pte->ppn), 0, B_PAGE_SIZE);
			pte->flags |= (1 << pteValid);
		}
		pte = (Pte*)VirtFromPhys(B_PAGE_SIZE * pte->ppn);
	}
	pte += VirtAdrPte(virtAdr, 0);
	return pte;
}


static void
Map(addr_t virtAdr, phys_addr_t physAdr, uint64 flags, kernel_args* args,
	phys_addr_t (*get_free_page)(kernel_args *))
{
	// dprintf("Map(0x%" B_PRIxADDR ", 0x%" B_PRIxADDR ")\n", virtAdr, physAdr);
	Pte* pte = LookupPte(virtAdr, true, args, get_free_page);
	if (pte == NULL) panic("can't allocate page table");

	pte->ppn = physAdr / B_PAGE_SIZE;
	pte->flags = (1 << pteValid) | (1 << pteAccessed) | (1 << pteDirty) | flags;

	FlushTlbPage(virtAdr);
}


//#pragma mark -

status_t
arch_vm_translation_map_init(kernel_args *args,
	VMPhysicalPageMapper** _physicalPageMapper)
{
	TRACE("vm_translation_map_init: entry\n");

#ifdef TRACE_VM_TMAP
	TRACE("physical memory ranges:\n");
	for (uint32 i = 0; i < args->num_physical_memory_ranges; i++) {
		phys_addr_t start = args->physical_memory_range[i].start;
		phys_addr_t end = start + args->physical_memory_range[i].size;
		TRACE("  %" B_PRIxPHYSADDR " - %" B_PRIxPHYSADDR "\n", start, end);
	}

	TRACE("allocated physical ranges:\n");
	for (uint32 i = 0; i < args->num_physical_allocated_ranges; i++) {
		phys_addr_t start = args->physical_allocated_range[i].start;
		phys_addr_t end = start + args->physical_allocated_range[i].size;
		TRACE("  %" B_PRIxPHYSADDR " - %" B_PRIxPHYSADDR "\n", start, end);
	}

	TRACE("allocated virtual ranges:\n");
	for (uint32 i = 0; i < args->num_virtual_allocated_ranges; i++) {
		addr_t start = args->virtual_allocated_range[i].start;
		addr_t end = start + args->virtual_allocated_range[i].size;
		TRACE("  %" B_PRIxADDR " - %" B_PRIxADDR "\n", start, end);
	}

	TRACE("kernel args ranges:\n");
	for (uint32 i = 0; i < args->num_kernel_args_ranges; i++) {
		phys_addr_t start = args->kernel_args_range[i].start;
		phys_addr_t end = start + args->kernel_args_range[i].size;
		TRACE("  %" B_PRIxPHYSADDR " - %" B_PRIxPHYSADDR "\n", start, end);
	}
#endif
	
	{
		SatpReg satp(Satp());
		sPageTable = satp.ppn * B_PAGE_SIZE;
	}
	
	dprintf("physMapBase: %#" B_PRIxADDR "\n", args->arch_args.physMap.start);
	dprintf("physMemBase: %#" B_PRIxADDR "\n", args->physical_memory_range[0].start);
	gVirtFromPhysOffset = args->arch_args.physMap.start - args->physical_memory_range[0].start;

	clear_ac();

	*_physicalPageMapper = new(&sPhysicalPageMapperData)
		RISCV64VMPhysicalPageMapper();

	return B_OK;
}


status_t
arch_vm_translation_map_init_post_sem(kernel_args *args)
{
	return B_OK;
}


status_t
arch_vm_translation_map_init_post_area(kernel_args *args)
{
	TRACE("vm_translation_map_init_post_area: entry\n");
	return B_OK;
}


status_t
arch_vm_translation_map_early_map(kernel_args *args,
	addr_t virtAdr, phys_addr_t physAdr, uint8 attributes,
	phys_addr_t (*get_free_page)(kernel_args *))
{
	//dprintf("early_map(%#" B_PRIxADDR ", %#" B_PRIxADDR ")\n", virtAdr, physAdr);
	uint64 flags = 0;
	if ((attributes & B_KERNEL_READ_AREA) != 0)
		flags |= (1 << pteRead);
	if ((attributes & B_KERNEL_WRITE_AREA) != 0)
		flags |= (1 << pteWrite);
	if ((attributes & B_KERNEL_EXECUTE_AREA) != 0)
		flags |= (1 << pteExec);
	Map(virtAdr, physAdr, flags, args, get_free_page);
	return B_OK;
}


status_t
arch_vm_translation_map_create_map(bool kernel, VMTranslationMap** _map)
{
	*_map = new(std::nothrow) RISCV64VMTranslationMap(kernel,
		(kernel) ? sPageTable : 0);

	if (*_map == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


bool
arch_vm_translation_map_is_kernel_page_accessible(addr_t virtualAddress,
	uint32 protection)
{
	return virtualAddress != 0;
}
