/* 
 * Copyright 2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		François Revol <revol@free.fr>
 */

#include <arch/cpu.h>

#include <arch_040_mmu.h>

#define ARCH_M68K_MMU_TYPE 68040

#include "arch_vm_translation_map_impl.cpp"

static void
set_pgdir(void *rt)
{
	uint32 rp;
	rp = (uint32)rt & ~((1 << 9) - 1);

	asm volatile(          \
		"movec %0,%%srp\n" \
		"movec %0,%%urp\n" \
		: : "d"(rp));
	
}


struct m68k_vm_ops m68040_vm_ops = {
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
