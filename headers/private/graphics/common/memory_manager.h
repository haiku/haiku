/*
 * Copyright 2006, Haiku Inc.
 * Copyright 2002, Thomas Kurschel.
 *
 * Distributed under the terms of the MIT license.
 */
#ifndef _MEMORY_MANAGER_H
#define _MEMORY_MANAGER_H

/**	Memory manager used for graphics mem */

#include <OS.h>


typedef struct mem_info mem_info;

#ifdef __cplusplus
extern "C" {
#endif

mem_info *mem_init(const char *name, uint32 start, uint32 length, uint32 blockSize,
				uint32 heapEntries);
void mem_destroy(mem_info *mem);
status_t mem_alloc(mem_info *mem, uint32 size, void *tag, uint32 *blockID, uint32 *offset);
status_t mem_free(mem_info *mem, uint32 blockID, void *tag);
status_t mem_freetag(mem_info *mem, void *tag);

#ifdef __cplusplus
}
#endif

#endif	/* _MEMORY_MANAGER_H */
