/*
 * Copyright 2009 Jonas Sundström, jonas@kirilla.com
 * Copyright 2007 François Revol, revol@free.fr
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
#include <stdlib.h>

#include "generic_vm_physical_page_mapper.h"


//  #pragma mark -
//  VM API


status_t
arch_vm_translation_map_init_map(vm_translation_map* map, bool kernel)
{
#warning IMPLEMENT arch_vm_translation_map_init_map
	return NULL;
}


status_t
arch_vm_translation_map_init_kernel_map_post_sem(vm_translation_map* map)
{
#warning IMPLEMENT arch_vm_translation_map_init_kernel_map_post_sem
	return NULL;
}


status_t
arch_vm_translation_map_init(kernel_args* args)
{
#warning IMPLEMENT arch_vm_translation_map_init
	return NULL;
}


status_t
arch_vm_translation_map_init_post_area(kernel_args* args)
{
#warning IMPLEMENT arch_vm_translation_map_init_post_area
	return NULL;
}


status_t
arch_vm_translation_map_init_post_sem(kernel_args* args)
{
#warning IMPLEMENT arch_vm_translation_map_init_post_sem
	return NULL;
}


status_t
arch_vm_translation_map_early_map(kernel_args* ka, addr_t virtualAddress,
	phys_addr_t physicalAddress, uint8 attributes,
	phys_addr_t (*get_free_page)(kernel_args* ))
{
#warning IMPLEMENT arch_vm_translation_map_early_map
	return NULL;
}


status_t
arch_vm_translation_map_early_query(addr_t va, phys_addr_t* out_physical)
{
#warning IMPLEMENT arch_vm_translation_map_early_query
	return NULL;
}


bool
arch_vm_translation_map_is_kernel_page_accessible(addr_t virtualAddress,
	uint32 protection)
{
#warning IMPLEMENT arch_vm_translation_map_is_kernel_page_accessible
	return TRUE;
}


// #pragma mark -


status_t
mipsel_map_address_range(addr_t virtualAddress, phys_addr_t physicalAddress,
	size_t size)
{
#warning IMPLEMENT mipsel_map_address_range
	return B_OK;
}


void
mipsel_unmap_address_range(addr_t virtualAddress, size_t size)
{
#warning IMPLEMENT mipsel_unmap_address_range
}


status_t
mipsel_remap_address_range(addr_t *_virtualAddress, size_t size, bool unmap)
{
#warning IMPLEMENT mipsel_remap_address_range
	return B_OK;
}

