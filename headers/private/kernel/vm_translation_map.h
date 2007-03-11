/*
 * Copyright 2002-2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef KERNEL_VM_TRANSLATION_MAP_H
#define KERNEL_VM_TRANSLATION_MAP_H


#include <kernel.h>
#include <lock.h>


struct kernel_args;


typedef struct vm_translation_map {
	struct vm_translation_map		*next;
	struct vm_translation_map_ops	*ops;
	recursive_lock					lock;
	int32							map_count;
	struct vm_translation_map_arch_info *arch_data;
} vm_translation_map;


// table of operations the vm may want to do to this mapping
typedef struct vm_translation_map_ops {
	void (*destroy)(vm_translation_map *map);
	status_t (*lock)(vm_translation_map *map);
	status_t (*unlock)(vm_translation_map *map);
	status_t (*map)(vm_translation_map *map, addr_t va, addr_t pa,
				uint32 attributes);
	status_t (*unmap)(vm_translation_map *map, addr_t start, addr_t end);
	status_t (*query)(vm_translation_map *map, addr_t va, addr_t *_outPhysical,
				uint32 *_outFlags);
	status_t (*query_interrupt)(vm_translation_map *map, addr_t va,
				addr_t *_outPhysical, uint32 *_outFlags);
	addr_t (*get_mapped_size)(vm_translation_map*);
	status_t (*protect)(vm_translation_map *map, addr_t base, addr_t top,
				uint32 attributes);
	status_t (*clear_flags)(vm_translation_map *map, addr_t va, uint32 flags);
	void (*flush)(vm_translation_map *map);
	status_t (*get_physical_page)(addr_t physicalAddress,
				addr_t *_virtualAddress, uint32 flags);
	status_t (*put_physical_page)(addr_t virtualAddress);
} vm_translation_map_ops;

#include <arch/vm_translation_map.h>

#endif	/* KERNEL_VM_TRANSLATION_MAP_H */
