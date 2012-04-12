/*
 * Copyright 2007-2010, François Revol, revol@free.fr.
 * Copyright 2008-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

#include <KernelExport.h>
#include <kernel.h>
#include <vm/vm.h>
#include <vm/vm_priv.h>
#include <vm/VMAddressSpace.h>
#include <int.h>
#include <boot/kernel_args.h>
#include <arch/vm_translation_map.h>
#include <arch/cpu.h>
#include <arch_mmu.h>
#include <stdlib.h>

#include "generic_vm_physical_page_mapper.h"
//#include "paging/030/M68KPagingMethod030.h"
#include "paging/040/M68KPagingMethod040.h"
//#include "paging/060/M68KPagingMethod060.h"


#define TRACE_VM_TMAP
#ifdef TRACE_VM_TMAP
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...) ;
#endif


/*
 * Each mmu of the m68k family has its own tricks, registers and opcodes...
 * so they all have a specific paging method class.
 */

#warning M68K: 060: must *not* have pgtables in copyback cachable mem!!!

static union {
	uint64	align;
	//char	m68851[sizeof(M68KPagingMethod851)];
	//char	m68030[sizeof(M68KPagingMethod030)];
	char	m68040[sizeof(M68KPagingMethod040)];
// 060 should be identical to 040 except for copyback issue
	//char	m68060[sizeof(M68KPagingMethod060)];
} sPagingMethodBuffer;


#if 0
void *
m68k_translation_map_get_pgdir(VMTranslationMap *map)
{
	return get_vm_ops()->m68k_translation_map_get_pgdir(map);
}
#endif

//  #pragma mark -
//  VM API


status_t
arch_vm_translation_map_create_map(bool kernel, VMTranslationMap** _map)
{
	return gM68KPagingMethod->CreateTranslationMap(kernel, _map);
}


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
		TRACE("  %#10" B_PRIxPHYSADDR " - %#10" B_PRIxPHYSADDR "\n", start,
			end);
	}

	TRACE("allocated physical ranges:\n");
	for (uint32 i = 0; i < args->num_physical_allocated_ranges; i++) {
		phys_addr_t start = args->physical_allocated_range[i].start;
		phys_addr_t end = start + args->physical_allocated_range[i].size;
		TRACE("  %#10" B_PRIxPHYSADDR " - %#10" B_PRIxPHYSADDR "\n", start,
			end);
	}

	TRACE("allocated virtual ranges:\n");
	for (uint32 i = 0; i < args->num_virtual_allocated_ranges; i++) {
		addr_t start = args->virtual_allocated_range[i].start;
		addr_t end = start + args->virtual_allocated_range[i].size;
		TRACE("  %#10" B_PRIxADDR " - %#10" B_PRIxADDR "\n", start, end);
	}
#endif
	switch (arch_mmu_type) {
		/*
		case 68030:
			gM68KPagingMethod = new(&sPagingMethodBuffer) M68KPagingMethod030;
			break;
		*/
		case 68040:
			gM68KPagingMethod = new(&sPagingMethodBuffer) M68KPagingMethod040;
			break;
		/*
		case 68060:
			gM68KPagingMethod = new(&sPagingMethodBuffer) M68KPagingMethod060;
			break;
		*/
		default:
			break;
	}
	return gM68KPagingMethod->Init(args, _physicalPageMapper);
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

	return gM68KPagingMethod->InitPostArea(args);
}


/**	Directly maps a page without having knowledge of any kernel structures.
 *	Used only during VM setup.
 *	It currently ignores the "attributes" parameter and sets all pages
 *	read/write.
 */
status_t
arch_vm_translation_map_early_map(kernel_args *args, addr_t va, phys_addr_t pa,
	uint8 attributes, phys_addr_t (*get_free_page)(kernel_args *))
{
	TRACE("early_tmap: entry pa 0x%lx va 0x%lx\n", pa, va);

	return gM68KPagingMethod->MapEarly(args, va, pa, attributes, get_free_page);
}


// XXX currently assumes this translation map is active
/*
status_t
arch_vm_translation_map_early_query(addr_t va, addr_t *out_physical)
{
	return get_vm_ops()->arch_vm_translation_map_early_query(va, out_physical);
}
*/


// #pragma mark -


bool
arch_vm_translation_map_is_kernel_page_accessible(addr_t virtualAddress,
	uint32 protection)
{
	return gM68KPagingMethod->IsKernelPageAccessible(virtualAddress, protection);
}

