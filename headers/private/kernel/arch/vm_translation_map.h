/*
** Copyright 2002-2004, The Haiku Team. All rights reserved.
** Distributed under the terms of the Haiku License.
**
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef KERNEL_ARCH_VM_TRANSLATION_MAP_H
#define KERNEL_ARCH_VM_TRANSLATION_MAP_H


#include <vm_translation_map.h>


#ifdef __cplusplus
extern "C" {
#endif

status_t arch_vm_translation_map_init_map(vm_translation_map *map, bool kernel);
status_t arch_vm_translation_map_init_kernel_map_post_sem(vm_translation_map *map);

status_t arch_vm_translation_map_init(struct kernel_args *args);
status_t arch_vm_translation_map_init_post_area(struct kernel_args *args);
status_t arch_vm_translation_map_init_post_sem(struct kernel_args *args);

// quick function to map a page in regardless of map context. Used in VM initialization,
// before most vm data structures exist
status_t arch_vm_translation_map_early_map(struct kernel_args *args, addr_t va, addr_t pa,
	uint8 attributes, addr_t (*get_free_page)(struct kernel_args *));

#ifdef __cplusplus
}
#endif

#include <arch_vm_translation_map.h>

#endif	/* KERNEL_ARCH_VM_TRANSLATION_MAP_H */

