/*
 * Copyright 2002-2003, Thomas Kurschel. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __LOCKED_POOL_H__
#define __LOCKED_POOL_H__


/*!	Paging-safe allocation of locked memory.

	Library for managing temporary, locked memory with the condition of 
	not calling any function during allocation that can lead to paging.
	Such memory is needed by drivers that are used to access the page file
	but still need locked memory to execute requests.
	
	Basically, a background thread manages a memory pool where blocks
	are allocated from. If the pool is empty, allocation is delayed until 
	either a blocks is freed or the pool is enlarged by the background 
	thread.
	
	All memory blocks must have same size and can be pre-initialized when
	added to memory pool (and cleaned-up when removed from pool). The
	free list is stored within free memory blocks, so you have to specify
	a block offset where the manager can store the list pointers without
	interfering with pre-initialization.
	
	You can also specify an alignment, e.g. if the blocks are used for
	DMA access, a minimum pool size (in blocks), a maximum pool size
	(in blocks) and the size of memory chunks to be added if the entire 
	pool is allocated.
*/


#include <device_manager.h>


typedef struct locked_pool *locked_pool_cookie;

typedef status_t (*locked_pool_add_hook)(void *block, void *arg);
typedef void (*locked_pool_remove_hook)(void *block, void *arg);

typedef struct {
	module_info minfo;

	// allocate block	
	void *(*alloc)(locked_pool_cookie pool);
	// free block
	void (*free)(locked_pool_cookie pool, void *block);

	// create new pool
	// block_size	- size of one memory block
	// alignment	- set address bits here that must be zero for block addresses
	// next_ofs		- offset in block where internal next-pointer can be stored
	// chunk_size	- how much system memory is to be allocated at once
	// max_blocks	- maximum number of blocks
	// min_free_block - minimum number of free blocks
	// name			- name of pool
	// lock_flags	- flags to be passed to lock_memory()
	// alloc_hook	- hook to be called when new block is added to pool (can be NULL )
	// free_hook	- hook to be called when block is removed from pool (can be NULL )
	// hook_arg		- value to be passed to hooks as arg
	locked_pool_cookie (*create)(int block_size, int alignment, int next_ofs,
		int chunk_size, int max_blocks, int min_free_blocks, const char *name,
		uint32 lock_flags, locked_pool_add_hook add_hook, 
		locked_pool_remove_hook remove_hook, void *hook_arg);
	void (*destroy)(locked_pool_cookie pool);
} locked_pool_interface;

#define LOCKED_POOL_MODULE_NAME "generic/locked_pool/v1"

#endif
