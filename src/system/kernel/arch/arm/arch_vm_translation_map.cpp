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
//#include <arch_mmu.h>
#include <stdlib.h>

#include "generic_vm_physical_page_mapper.h"


void *
m68k_translation_map_get_pgdir(VMTranslationMap *map)
{
	return NULL;
#warning ARM:WRITEME
//get_vm_ops()->m68k_translation_map_get_pgdir(map);
}

//  #pragma mark -
//  VM API


status_t
arch_vm_translation_map_init(kernel_args *args,
        VMPhysicalPageMapper** _physicalPageMapper)
{
	return NULL;
#warning ARM:WRITEME

//get_vm_ops()->arch_vm_translation_map_init_map(map, kernel);
}

status_t
arch_vm_translation_map_create_map(bool kernel, VMTranslationMap** _map)
{
	return NULL;
#warning ARM:WRITEME
}

status_t
arch_vm_translation_map_init_kernel_map_post_sem(VMTranslationMap *map)
{
	return NULL;
#warning ARM:WRITEME

//get_vm_ops()->arch_vm_translation_map_init_kernel_map_post_sem(map);
}


status_t
arch_vm_translation_map_init(kernel_args *args)
{
	return NULL;
#warning ARM:WRITEME

//get_vm_ops()->arch_vm_translation_map_init(args);
}


status_t
arch_vm_translation_map_init_post_area(kernel_args *args)
{
	return NULL;
#warning ARM:WRITEME

//get_vm_ops()->arch_vm_translation_map_init_post_area(args);
}


status_t
arch_vm_translation_map_init_post_sem(kernel_args *args)
{
	return NULL;
#warning ARM:WRITEME

//get_vm_ops()->arch_vm_translation_map_init_post_sem(args);
}


/**	Directly maps a page without having knowledge of any kernel structures.
 *	Used only during VM setup.
 *	It currently ignores the "attributes" parameter and sets all pages
 *	read/write.
 */

status_t
arch_vm_translation_map_early_map(kernel_args *ka, addr_t virtualAddress,
	phys_addr_t physicalAddress, uint8 attributes,
	phys_addr_t (*get_free_page)(kernel_args *))
{
	return NULL;
#warning ARM:WRITEME

//get_vm_ops()->arch_vm_translation_map_early_map(ka, virtualAddress, physicalAddress,
//		attributes, get_free_page);
}


// XXX currently assumes this translation map is active

status_t
arch_vm_translation_map_early_query(addr_t va, phys_addr_t *out_physical)
{
	return NULL;
#warning ARM:WRITEME

//get_vm_ops()->arch_vm_translation_map_early_query(va, out_physical);
}


bool
arch_vm_translation_map_is_kernel_page_accessible(addr_t virtualAddress,
        uint32 protection)
{
#warning ARM:WRITEME
        return TRUE;
//get_vm_ops()-arch_vm_translation_map_is_kernel_page_accessible(virtualAddress,
  //              protection);
}

