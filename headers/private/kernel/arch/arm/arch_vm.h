/*
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef ARCH_ARM_VM_H
#define ARCH_ARM_VM_H

//#include <vm/vm_translation_map.h>

/* This many pages will be read/written on I/O if possible */

#define NUM_IO_PAGES	4
	/* 16 kB */

#define PAGE_SHIFT 12


struct arm_vm_ops {
//	void *(*arm_translation_map_get_pgdir)(vm_translation_map *map);
/*	status_t (*arch_vm_translation_map_init_map)(vm_translation_map *map, bool kernel);
	status_t (*arch_vm_translation_map_init_kernel_map_post_sem)(vm_translation_map *map);
	status_t (*arch_vm_translation_map_init)(kernel_args *args);
	status_t (*arch_vm_translation_map_init_post_area)(kernel_args *args);
	status_t (*arch_vm_translation_map_init_post_sem)(kernel_args *args);
	status_t (*arch_vm_translation_map_early_map)(kernel_args *ka, addr_t virtualAddress, addr_t physicalAddress,
		uint8 attributes, addr_t (*get_free_page)(kernel_args *));
	status_t (*arch_vm_translation_map_early_query)(addr_t va, addr_t *out_physical);*/
//	void (*m68k_set_pgdir)(void *);
};

#ifdef __cplusplus
extern "C" {
#endif

extern struct arm_vm_ops *get_vm_ops();

//extern void *arm_translation_map_get_pgdir(vm_translation_map *map);
//extern void arm_set_pgdir(void *rt);

#ifdef __cplusplus
}
#endif

#endif	/* ARCH_M68K_VM_H */
