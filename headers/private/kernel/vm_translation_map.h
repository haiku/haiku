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
	status_t (*lock)(vm_translation_map*);
	status_t (*unlock)(vm_translation_map*);
	status_t (*map)(vm_translation_map *map, addr_t va, addr_t pa, uint32 attributes);
	status_t (*unmap)(vm_translation_map *map, addr_t start, addr_t end);
	status_t (*query)(vm_translation_map *map, addr_t va, addr_t *_outPhysical, uint32 *_outFlags);
	addr_t (*get_mapped_size)(vm_translation_map*);
	status_t (*protect)(vm_translation_map *map, addr_t base, addr_t top, uint32 attributes);
	status_t (*clear_flags)(vm_translation_map *map, addr_t va, uint32 flags);
	void (*flush)(vm_translation_map *map);
	status_t (*get_physical_page)(addr_t physical_address, addr_t *out_virtual_address, uint32 flags);
	status_t (*put_physical_page)(addr_t virtual_address);
} vm_translation_map_ops;

// ToDo: fix this
// unlike the usual habit, the VM translation map functions are
// arch-specific but do not have the arch_ prefix.
#include <arch/vm_translation_map.h>

#endif	/* KERNEL_VM_TRANSLATION_MAP_H */

