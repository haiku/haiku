/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef KERNEL_VM_TRANSLATION_MAP_H
#define KERNEL_VM_TRANSLATION_MAP_H


#include <kernel.h>
#include <lock.h>


struct kernel_args;


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

// ToDo: fix this
// unlike the usual habit, the VM translation map functions are
// arch-specific but do not have the arch_ prefix.
#include <arch/vm_translation_map.h>

#endif	/* KERNEL_VM_TRANSLATION_MAP_H */

