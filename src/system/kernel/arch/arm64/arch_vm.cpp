/*
 * Copyright 2019-2022 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#include <KernelExport.h>

#include <boot/kernel_args.h>
#include <kernel.h>

#include <arch/vm.h>
#include <vm/vm.h>
#include <vm/VMAddressSpace.h>
#include <vm/vm_types.h>

#include "VMSAv8TranslationMap.h"

//#define TRACE_ARCH_VM
#ifdef TRACE_ARCH_VM
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...) ;
#endif


status_t
arch_vm_init(kernel_args* args)
{
	TRACE("arch_vm_init\n");
	return B_OK;
}


status_t
arch_vm_init2(kernel_args* args)
{
	TRACE("arch_vm_init2\n");
	return B_OK;
}


status_t
arch_vm_init_post_area(kernel_args* args)
{
	TRACE("arch_vm_init_post_area\n");
	return B_OK;
}


status_t
arch_vm_init_end(kernel_args* args)
{
	TRACE("arch_vm_init_end(): %" B_PRIu32 " virtual ranges to keep:\n",
		args->arch_args.num_virtual_ranges_to_keep);

	for (int i = 0; i < (int)args->arch_args.num_virtual_ranges_to_keep; i++) {
		addr_range &range = args->arch_args.virtual_ranges_to_keep[i];

		TRACE("  start: %p, size: %#" B_PRIxSIZE "\n", (void*)range.start, range.size);

		// skip ranges outside the kernel address space
		if (!IS_KERNEL_ADDRESS(range.start)) {
			TRACE("    no kernel address, skipping...\n");
			continue;
		}

		phys_addr_t physicalAddress;
		void *address = (void*)range.start;
		if (vm_get_page_mapping(VMAddressSpace::KernelID(), range.start,
			&physicalAddress) != B_OK)
			panic("arch_vm_init_end(): No page mapping for %p\n", address);

		area_id area = vm_map_physical_memory(VMAddressSpace::KernelID(),
			"boot loader reserved area", &address,
			B_EXACT_ADDRESS, range.size,
			B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
			physicalAddress, true);

		if (area < 0) {
			panic("arch_vm_init_end(): Failed to create area for boot loader "
				"reserved area: %p - %p\n", (void*)range.start,
				(void*)(range.start + range.size));
		}
	}

	return B_OK;
}


status_t
arch_vm_init_post_modules(kernel_args* args)
{
	TRACE("arch_vm_init_post_modules\n");
	return B_OK;
}


void
arch_vm_aspace_swap(struct VMAddressSpace* from, struct VMAddressSpace* to)
{
	VMSAv8TranslationMap* fromMap = (VMSAv8TranslationMap*)from->TranslationMap();
	VMSAv8TranslationMap* toMap = (VMSAv8TranslationMap*)to->TranslationMap();
	if (fromMap != toMap)
		VMSAv8TranslationMap::SwitchUserMap(fromMap, toMap);
}


bool
arch_vm_supports_protection(uint32 protection)
{
	// User-RO/Kernel-RW is not possible
	if ((protection & B_READ_AREA) != 0 && (protection & B_WRITE_AREA) == 0
		&& (protection & B_KERNEL_WRITE_AREA) != 0) {
		return false;
	}

	// User-Execute implies User-Read, because it would break PAN otherwise
	if ((protection & B_EXECUTE_AREA) != 0
	    && (protection & B_READ_AREA) == 0) {
		return false;
	}

	return true;
}


void
arch_vm_unset_memory_type(VMArea* area)
{
}


status_t
arch_vm_set_memory_type(VMArea* area, phys_addr_t physicalBase, uint32 type,
	uint32 *effectiveType)
{
	// Memory type is set in page tables during mapping,
	// no need to do anything more here.
	return B_OK;
}
