/*
** Copyright 2002/03, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

/*
	Functions that are missing in kernel.
*/

#ifndef _KERNEL_EXPORT_EXT_H
#define _KERNEL_EXPORT_EXT_H

#include <KernelExport.h>
#include <iovec.h>


// get memory map of iovec
status_t get_iovec_memory_map( 
	iovec *vec, 				// iovec to analyze
	size_t vec_count, 			// number of entries in vec
	size_t vec_offset, 			// number of bytes to skip at beginning of vec
	size_t len, 				// number of bytes to analyze
	physical_entry *map, 		// resulting memory map
	size_t max_entries, 		// max number of entries in map
	size_t *num_entries, 		// actual number of map entries used
	size_t *mapped_len 			// actual number of bytes described by map
);

// map main memory into virtual address space
status_t map_mainmemory( 
	addr_t physical_addr, 		// physical address to map
	void **virtual_addr 		// receives corresponding virtual address
);

// unmap main memory from virtual address space
status_t unmap_mainmemory( 
	void *virtual_addr 			// virtual address to release
);


// this should be moved to SupportDefs.h
int32	atomic_exchange(vint32 *value, int32 newValue);	


#endif
