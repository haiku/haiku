/*
 * Copyright 2010, Ithamar R. Adema, ithamar.adema@team-embedded.nl
 * Copyright 2008-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <arch/vm_translation_map.h>

#include <boot/kernel_args.h>

#include "paging/32bit/ARMPagingMethod32Bit.h"
//#include "paging/pae/ARMPagingMethodPAE.h"


//#define TRACE_VM_TMAP
#ifdef TRACE_VM_TMAP
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...) ;
#endif


static union {
	uint64	align;
	char	thirty_two[sizeof(ARMPagingMethod32Bit)];
	#if B_HAIKU_PHYSICAL_BITS == 64
		char	pae[sizeof(ARMPagingMethodPAE)];
	#endif
} sPagingMethodBuffer;


// #pragma mark - VM API


status_t
arch_vm_translation_map_create_map(bool kernel, VMTranslationMap** _map)
{
	return gARMPagingMethod->CreateTranslationMap(kernel, _map);
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

	#if B_HAIKU_PHYSICAL_BITS == 64 //IRA: Check 64 bit code and adjust for ARM
	bool paeAvailable = x86_check_feature(IA32_FEATURE_PAE, FEATURE_COMMON);
	bool paeNeeded = false;
	for (uint32 i = 0; i < args->num_physical_memory_ranges; i++) {
		phys_addr_t end = args->physical_memory_range[i].start
			+ args->physical_memory_range[i].size;
		if (end > 0x100000000LL) {
			paeNeeded = true;
			break;
		}
	}

	if (paeAvailable && paeNeeded) {
		dprintf("using PAE paging\n");
		gARMPagingMethod = new(&sPagingMethodBuffer) ARMPagingMethodPAE;
	} else {
		dprintf("using 32 bit paging (PAE not %s)\n",
			paeNeeded ? "available" : "needed");
		gARMPagingMethod = new(&sPagingMethodBuffer) ARMPagingMethod32Bit;
	}
	#else
	gARMPagingMethod = new(&sPagingMethodBuffer) ARMPagingMethod32Bit;
	#endif

	return gARMPagingMethod->Init(args, _physicalPageMapper);
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

	return gARMPagingMethod->InitPostArea(args);
}


status_t
arch_vm_translation_map_early_map(kernel_args *args, addr_t va, phys_addr_t pa,
	uint8 attributes, phys_addr_t (*get_free_page)(kernel_args *))
{
	TRACE("early_tmap: entry pa 0x%lx va 0x%lx\n", pa, va);

	return gARMPagingMethod->MapEarly(args, va, pa, attributes, get_free_page);
}


/*!	Verifies that the page at the given virtual address can be accessed in the
	current context.

	This function is invoked in the kernel debugger. Paranoid checking is in
	order.

	\param virtualAddress The virtual address to be checked.
	\param protection The area protection for which to check. Valid is a bitwise
		or of one or more of \c B_KERNEL_READ_AREA or \c B_KERNEL_WRITE_AREA.
	\return \c true, if the address can be accessed in all ways specified by
		\a protection, \c false otherwise.
*/
bool
arch_vm_translation_map_is_kernel_page_accessible(addr_t virtualAddress,
	uint32 protection)
{
	if (!gARMPagingMethod)
		return true;

	return gARMPagingMethod->IsKernelPageAccessible(virtualAddress, protection);
}
