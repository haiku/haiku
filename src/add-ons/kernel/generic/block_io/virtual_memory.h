/*
 * Copyright 2002/03, Thomas Kurschel. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _VIRTUAL_MEMORY_H
#define _VIRTUAL_MEMORY_H


#include <KernelExport.h>
#include <iovec.h>


// get memory map of iovec
extern status_t get_iovec_memory_map(iovec *vec, size_t vecCount,
					size_t vecOffset, size_t length, physical_entry *map,
					size_t maxMapEntries, size_t *_numEntries,
					size_t *_mappedLength);

#endif	/* _VIRTUAL_MEMORY_H */
