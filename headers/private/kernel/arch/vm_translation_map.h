/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _NEWOS_KERNEL_ARCH_VM_TRANSLATION_MAP_H
#define _NEWOS_KERNEL_ARCH_VM_TRANSLATION_MAP_H

#include <kernel.h>
#include <stage2.h>
#include <lock.h>

typedef struct vm_translation_map_struct {
	struct vm_translation_map_struct *next;
	struct vm_translation_map_ops_struct *ops;
	recursive_lock lock;
	int map_count;
	struct vm_translation_map_arch_info_struct *arch_data;
} vm_translation_map;

// table of operations the vm may want to do to this mapping
typedef struct vm_translation_map_ops_struct {
	void (*destroy)(vm_translation_map *);
	int (*lock)(vm_translation_map*);
	int (*unlock)(vm_translation_map*);
	int (*map)(vm_translation_map *map, addr va, addr pa, unsigned int attributes);
	int (*unmap)(vm_translation_map *map, addr start, addr end);
	int (*query)(vm_translation_map *map, addr va, addr *out_physical, unsigned int *out_flags);
	addr (*get_mapped_size)(vm_translation_map*);
	int (*protect)(vm_translation_map *map, addr base, addr top, unsigned int attributes);
	int (*clear_flags)(vm_translation_map *map, addr va, unsigned int flags);
	void (*flush)(vm_translation_map *map);
	int (*get_physical_page)(addr physical_address, addr *out_virtual_address, int flags);
	int (*put_physical_page)(addr virtual_address);
} vm_translation_map_ops;

int vm_translation_map_create(vm_translation_map *new_map, bool kernel);
int vm_translation_map_module_init(kernel_args *ka);
int vm_translation_map_module_init2(kernel_args *ka);
void vm_translation_map_module_init_post_sem(kernel_args *ka);
// quick function to map a page in regardless of map context. Used in VM initialization,
// before most vm data structures exist
int vm_translation_map_quick_map(kernel_args *ka, addr va, addr pa, unsigned int attributes, addr (*get_free_page)(kernel_args *));

// quick function to return the physical pgdir of a mapping, needed for a context switch
addr vm_translation_map_get_pgdir(vm_translation_map *map);

#endif

