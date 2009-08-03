/* 
 * Copyright 2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		François Revol <revol@free.fr>
 */

#include <arch/cpu.h>

#include <arch_030_mmu.h>

#define ARCH_M68K_MMU_TYPE 68030

#include "arch_vm_translation_map_impl.cpp"

static void
set_pgdir(void *rt)
{
	long_page_directory_entry entry;
	*(uint64 *)&entry = DFL_PAGEENT_VAL;
	entry.type = DT_ROOT;
	entry.addr = TA_TO_PREA(((addr_t)rt));

	asm volatile( \
		"pmove (%0),%%srp\n" \
		"pmove (%0),%%crp\n" \
		: : "a"((uint64 *)&entry));
	
}


struct m68k_vm_ops m68030_vm_ops = {
	_m68k_translation_map_get_pgdir,
	m68k_vm_translation_map_init_map,
	m68k_vm_translation_map_init_kernel_map_post_sem,
	m68k_vm_translation_map_init,
	m68k_vm_translation_map_init_post_area,
	m68k_vm_translation_map_init_post_sem,
	m68k_vm_translation_map_early_map,
	/*m68k_vm_translation_map_*/early_query,
	set_pgdir,
#if 0
	m68k_map_address_range,
	m68k_unmap_address_range,
	m68k_remap_address_range
#endif
	m68k_vm_translation_map_is_kernel_page_accessible
};
