/*
 * Copyright 2008 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SYSTEM_INFO_H
#define _SYSTEM_INFO_H


#include <OS.h>


#define B_MEMORY_INFO	'memo'

struct system_memory_info {
	uint64		max_memory;
	uint64		free_memory;
	uint64		needed_memory;
	uint64		max_swap_space;
	uint64		free_swap_space;
	uint64		block_cache_memory;
	uint32		page_faults;

	// TODO: add active/inactive page counts, swap in/out, ...
};


#ifdef __cplusplus
extern "C" {
#endif

extern status_t get_system_info_etc(int32 id, void *buffer, size_t bufferSize);

#ifdef __cplusplus
}
#endif

#endif	/* _SYSTEM_INFO_H */
