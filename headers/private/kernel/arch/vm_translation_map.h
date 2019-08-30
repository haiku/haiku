/*
** Copyright 2002-2010, The Haiku Team. All rights reserved.
** Distributed under the terms of the MIT License.
**
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef KERNEL_ARCH_VM_TRANSLATION_MAP_H
#define KERNEL_ARCH_VM_TRANSLATION_MAP_H


#include <vm/VMTranslationMap.h>


#ifdef __cplusplus
extern "C" {
#endif

status_t arch_vm_translation_map_create_map(bool kernel,
	VMTranslationMap** _map);

status_t arch_vm_translation_map_init(struct kernel_args *args,
	VMPhysicalPageMapper** _physicalPageMapper);
status_t arch_vm_translation_map_init_post_area(struct kernel_args *args);
status_t arch_vm_translation_map_init_post_sem(struct kernel_args *args);

// Quick function to map a page in regardless of map context. Used in VM
// initialization before most vm data structures exist.
status_t arch_vm_translation_map_early_map(struct kernel_args *args, addr_t va,
	phys_addr_t pa, uint8 attributes,
	phys_addr_t (*get_free_page)(struct kernel_args *));

bool arch_vm_translation_map_is_kernel_page_accessible(addr_t virtualAddress,
	uint32 protection);

#ifdef __cplusplus
}
#endif

#include <arch_vm_translation_map.h>

#endif	/* KERNEL_ARCH_VM_TRANSLATION_MAP_H */

