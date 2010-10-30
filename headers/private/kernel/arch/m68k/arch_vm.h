/*
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef ARCH_M68K_VM_H
#define ARCH_M68K_VM_H

#include <vm/VMTranslationMap.h>

/* This many pages will be read/written on I/O if possible */

#define NUM_IO_PAGES	4
	/* 16 kB */

#define PAGE_SHIFT 12


struct m68k_vm_ops {
	void *(*m68k_translation_map_get_pgdir)(VMTranslationMap *map);
	status_t (*arch_vm_translation_map_init_map)(VMTranslationMap *map, bool kernel);
	status_t (*arch_vm_translation_map_init_kernel_map_post_sem)(VMTranslationMap *map);
	status_t (*arch_vm_translation_map_init)(kernel_args *args);
	status_t (*arch_vm_translation_map_init_post_area)(kernel_args *args);
	status_t (*arch_vm_translation_map_init_post_sem)(kernel_args *args);
	status_t (*arch_vm_translation_map_early_map)(kernel_args *ka, addr_t virtualAddress, addr_t physicalAddress,
		uint8 attributes, addr_t (*get_free_page)(kernel_args *));
	status_t (*arch_vm_translation_map_early_query)(addr_t va, addr_t *out_physical);
	void (*m68k_set_pgdir)(void *);
#if 0 /* ppc stuff only ? */
	status_t (*m68k_map_address_range)(addr_t virtualAddress, addr_t physicalAddress,
		size_t size);
	void (*m68k_unmap_address_range)(addr_t virtualAddress, size_t size);
	status_t (*m68k_remap_address_range)(addr_t *_virtualAddress, size_t size, bool unmap);
#endif
	bool (*arch_vm_translation_map_is_kernel_page_accessible)(addr_t virtualAddress, uint32 protection);

};

#ifdef __cplusplus
extern "C" {
#endif

extern struct m68k_vm_ops *get_vm_ops();

extern void *m68k_translation_map_get_pgdir(VMTranslationMap *map);
extern void m68k_set_pgdir(void *rt);

#ifdef __cplusplus
}
#endif

#endif	/* ARCH_M68K_VM_H */
