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

// #include "arch_vm_translation_map_impl.cpp"
// Let's see what breaks if this is not included.
// The functions it provides might be needed for m68040_vm_ops (now commented out)
// or for early boot. M68KVMTranslationMap040 should provide the main map ops.

// Forward declarations for functions that were in arch_vm_translation_map_impl.cpp
// and might be used by m68040_vm_ops or other early boot code if that ops struct were active.
// Since m68040_vm_ops is commented out, these might not be strictly necessary anymore
// unless called directly from other parts of arch_040_mmu.cpp or related boot code.
struct VMTranslationMap;
struct kernel_args;
static void *_m68k_translation_map_get_pgdir(VMTranslationMap *map);
static status_t m68k_vm_translation_map_init_map(VMTranslationMap *map, bool kernel);
static status_t m68k_vm_translation_map_init_kernel_map_post_sem(VMTranslationMap *map);
static status_t m68k_vm_translation_map_init(kernel_args *args);
static status_t m68k_vm_translation_map_init_post_area(kernel_args *args);
static status_t m68k_vm_translation_map_init_post_sem(kernel_args *args);
static status_t m68k_vm_translation_map_early_map(kernel_args *args, addr_t va, addr_t pa, uint8 attributes);
static status_t early_query(addr_t va, addr_t *_physicalAddress);
static bool m68k_vm_translation_map_is_kernel_page_accessible(addr_t virtualAddress, uint32 protection);
// End forward declarations

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

/*
struct m68k_vm_ops m68040_vm_ops = {
	_m68k_translation_map_get_pgdir,
	m68k_vm_translation_map_init_map,
	m68k_vm_translation_map_init_kernel_map_post_sem,
	m68k_vm_translation_map_init,
	m68k_vm_translation_map_init_post_area,
	m68k_vm_translation_map_init_post_sem,
	m68k_vm_translation_map_early_map,
	early_query,
	set_pgdir,
#if 0
	m68k_map_address_range,
	m68k_unmap_address_range,
	m68k_remap_address_range
#endif
	m68k_vm_translation_map_is_kernel_page_accessible
};
*/
