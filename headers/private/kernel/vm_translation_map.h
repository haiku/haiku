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
	size_t (*map_max_pages_need)(vm_translation_map *map, addr_t start, addr_t end);
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

	// get/put virtual address for physical page -- will be usuable on all CPUs
	// (usually more expensive than the *_current_cpu() versions)
	status_t (*get_physical_page)(addr_t physicalAddress,
				addr_t *_virtualAddress, void **handle);
	status_t (*put_physical_page)(addr_t virtualAddress, void *handle);

	// get/put virtual address for physical page -- thread must be pinned the
	// whole time
	status_t (*get_physical_page_current_cpu)(addr_t physicalAddress,
				addr_t *_virtualAddress, void **handle);
	status_t (*put_physical_page_current_cpu)(addr_t virtualAddress,
				void *handle);

	// get/put virtual address for physical in KDL
	status_t (*get_physical_page_debug)(addr_t physicalAddress,
				addr_t *_virtualAddress, void **handle);
	status_t (*put_physical_page_debug)(addr_t virtualAddress, void *handle);

	// memory operations on pages
	status_t (*memset_physical)(addr_t address, int value, size_t length);
	status_t (*memcpy_from_physical)(void* to, addr_t from, size_t length,
				bool user);
	status_t (*memcpy_to_physical)(addr_t to, const void* from, size_t length,
				bool user);
	void (*memcpy_physical_page)(addr_t to, addr_t from);
} vm_translation_map_ops;

#include <arch/vm_translation_map.h>

#endif	/* KERNEL_VM_TRANSLATION_MAP_H */
