/*
 * Copyright 2007, François Revol, revol@free.fr.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2003-2007, Axel Dörfler, axeld@pinc-software.de.
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

/*
 * Each mmu of the m68k family has its own tricks, registers and opcodes...
 * so we use a function array to switch to the one we want.
 */

#warning M68K: 060: must *not* have pgtables in copyback cachable mem!!!

//extern struct m68k_vm_ops m68851_vm_ops;
extern struct m68k_vm_ops m68030_vm_ops;
extern struct m68k_vm_ops m68040_vm_ops;
// 060 should be identical to 040 except for copyback issue
//extern struct m68k_vm_ops m68060_vm_ops;

#warning M68K: use a static!
m68k_vm_ops *get_vm_ops()
{
	int mmu = arch_mmu_type;

	switch (mmu) {
		case 68551:
			panic("Unimplemented yet (mmu)");
			//return &m68851_vm_ops;
			return NULL;
		case 68030:
			return &m68030_vm_ops;
		case 68040:
			return &m68040_vm_ops;
		case 68060:
			//return &m68060_vm_ops;
			panic("Unimplemented yet (mmu)");
			return NULL;
		default:
			panic("Invalid mmu type!");
			return NULL;
	}
}

void *
m68k_translation_map_get_pgdir(VMTranslationMap *map)
{
	return get_vm_ops()->m68k_translation_map_get_pgdir(map);
}

//  #pragma mark -
//  VM API


status_t
arch_vm_translation_map_init_map(VMTranslationMap *map, bool kernel)
{
	return get_vm_ops()->arch_vm_translation_map_init_map(map, kernel);
}


status_t
arch_vm_translation_map_init_kernel_map_post_sem(VMTranslationMap *map)
{
	return get_vm_ops()->arch_vm_translation_map_init_kernel_map_post_sem(map);
}


status_t
arch_vm_translation_map_init(kernel_args *args)
{
	return get_vm_ops()->arch_vm_translation_map_init(args);
}


status_t
arch_vm_translation_map_init_post_area(kernel_args *args)
{
	return get_vm_ops()->arch_vm_translation_map_init_post_area(args);
}


status_t
arch_vm_translation_map_init_post_sem(kernel_args *args)
{
	return get_vm_ops()->arch_vm_translation_map_init_post_sem(args);
}


/**	Directly maps a page without having knowledge of any kernel structures.
 *	Used only during VM setup.
 *	It currently ignores the "attributes" parameter and sets all pages
 *	read/write.
 */

status_t
arch_vm_translation_map_early_map(kernel_args *ka, addr_t virtualAddress, addr_t physicalAddress,
	uint8 attributes, addr_t (*get_free_page)(kernel_args *))
{
	return get_vm_ops()->arch_vm_translation_map_early_map(ka, virtualAddress, physicalAddress,
		attributes, get_free_page);
}


// XXX currently assumes this translation map is active

status_t
arch_vm_translation_map_early_query(addr_t va, addr_t *out_physical)
{
	return get_vm_ops()->arch_vm_translation_map_early_query(va, out_physical);
}


// #pragma mark -
void
m68k_set_pgdir(void *rt)
{
	return get_vm_ops()->m68k_set_pgdir(rt);
}
#if 0

status_t
m68k_map_address_range(addr_t virtualAddress, addr_t physicalAddress,
	size_t size)
{
	return get_vm_ops()->m68k_map_address_range(virtualAddress, physicalAddress, size);
}


void
m68k_unmap_address_range(addr_t virtualAddress, size_t size)
{
	get_vm_ops()->m68k_unmap_address_range(virtualAddress, size);
}


status_t
m68k_remap_address_range(addr_t *_virtualAddress, size_t size, bool unmap)
{
	return get_vm_ops()->m68k_remap_address_range(_virtualAddress, size, unmap);
}

#endif

bool
arch_vm_translation_map_is_kernel_page_accessible(addr_t virtualAddress,
	uint32 protection)
{
	return get_vm_ops()-arch_vm_translation_map_is_kernel_page_accessible(virtualAddress,
		protection);
}


