/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef KERNEL_ARCH_VM_TRANSLATION_MAP_H
#define KERNEL_ARCH_VM_TRANSLATION_MAP_H


#include <vm_translation_map.h>


#ifdef __cplusplus
extern "C" {
#endif

int vm_translation_map_create(vm_translation_map *new_map, bool kernel);
int vm_translation_map_module_init(struct kernel_args *ka);
int vm_translation_map_module_init2(struct kernel_args *ka);
void vm_translation_map_module_init_post_sem(struct kernel_args *ka);
// quick function to map a page in regardless of map context. Used in VM initialization,
// before most vm data structures exist
status_t vm_translation_map_quick_map(struct kernel_args *ka, addr_t va, addr_t pa,
	uint8 attributes, addr_t (*get_free_page)(struct kernel_args *));

#ifdef __cplusplus
}
#endif

#include <arch_vm_translation_map.h>

#endif	/* KERNEL_ARCH_VM_TRANSLATION_MAP_H */

