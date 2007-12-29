/* 
 * Copyright 2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		François Revol <revol@free.fr>
 */

#include <arch/cpu.h>

#include <arch_030_mmu.h>

#define ARCH_M68K_MMU_TYPE MMU_68030

#include "arch_vm_translation_map_impl.cpp"

struct m68k_vm_ops m68030_vm_ops = {
	m68k_translation_map_get_pgdir,
	arch_vm_translation_map_init_map,
	arch_vm_translation_map_init_kernel_map_post_sem,
	arch_vm_translation_map_init,
	arch_vm_translation_map_init_post_area,
	arch_vm_translation_map_init_post_sem,
	arch_vm_translation_map_early_map,
	arch_vm_translation_map_early_query,
#if 0
	m68k_map_address_range,
	m68k_unmap_address_range,
	m68k_remap_address_range
#endif
};
