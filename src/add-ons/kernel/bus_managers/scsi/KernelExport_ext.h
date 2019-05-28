/*
 * Copyright 2004-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2002/03, Thomas Kurschel. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */

/*
	Functions that are missing in kernel.
*/

#ifndef _KERNEL_EXPORT_EXT_H
#define _KERNEL_EXPORT_EXT_H


#include <sys/cdefs.h>

#include <KernelExport.h>
#include <iovec.h>


__BEGIN_DECLS


// get memory map of iovec
status_t get_iovec_memory_map(
	iovec *vec, 				// iovec to analyze
	size_t vec_count, 			// number of entries in vec
	size_t vec_offset, 			// number of bytes to skip at beginning of vec
	size_t len, 				// number of bytes to analyze
	physical_entry *map, 		// resulting memory map
	uint32 max_entries, 		// max number of entries in map
	uint32 *num_entries, 		// actual number of map entries used
	size_t *mapped_len 			// actual number of bytes described by map
);


__END_DECLS


#endif
