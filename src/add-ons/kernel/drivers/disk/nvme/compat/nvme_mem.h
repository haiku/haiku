/*
 * Copyright 2019, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Augustin Cavalier <waddlesplash>
 */
#ifndef __NVME_MEMORY_H_
#define __NVME_MEMORY_H_

#include <OS.h>


int nvme_mem_init();
void nvme_mem_cleanup();

void* nvme_mem_alloc_node(size_t size, size_t align,
	unsigned int node_id, phys_addr_t* paddr);
void* nvme_malloc_node(size_t size, size_t align,
	unsigned int node_id);

phys_addr_t nvme_mem_vtophys(void* vaddr);


#define NVME_VTOPHYS_ERROR	(~0ULL)

#define nvme_node_max()		(1)
#define NVME_NODE_MAX		(1)
#define nvme_node_id()		(0)


#endif /* __NVME_MEMORY_H_ */
