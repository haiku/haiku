/*
 * Copyright 2006-2010, Ingo Weinhold <ingo_weinhold@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_GENERIC_VM_PHYSICAL_PAGE_MAPPER_H
#define _KERNEL_GENERIC_VM_PHYSICAL_PAGE_MAPPER_H


#include <boot/kernel_args.h>


// flags for generic_get_physical_page()
enum {
	PHYSICAL_PAGE_DONT_WAIT		= 0x01
};

#ifdef __cplusplus
extern "C" {
#endif

typedef status_t (*generic_map_iospace_chunk_func)(addr_t virtualAddress,
	phys_addr_t physicalAddress, uint32 flags);

status_t generic_get_physical_page(phys_addr_t pa, addr_t *va, uint32 flags);
status_t generic_put_physical_page(addr_t va);
status_t generic_vm_physical_page_mapper_init(kernel_args *args,
	generic_map_iospace_chunk_func mapIOSpaceChunk, addr_t *ioSpaceBase,
	size_t ioSpaceSize, size_t ioSpaceChunkSize);
status_t generic_vm_physical_page_mapper_init_post_area(kernel_args *args);
status_t generic_vm_physical_page_mapper_init_post_sem(kernel_args *args);

#ifdef __cplusplus
}
#endif


#endif	// _KERNEL_GENERIC_VM_PHYSICAL_PAGE_MAPPER_H
