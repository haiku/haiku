/*
 * Copyright 2004-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FSSH_BLOCK_CACHE_PRIVATE_H
#define _FSSH_BLOCK_CACHE_PRIVATE_H

#include "DoublyLinkedList.h"
#include "lock.h"


namespace FSShell {

struct hash_table;
struct vm_page;


//#define DEBUG_CHANGED
#undef DEBUG_CHANGED


struct cache_transaction;
struct cached_block;
struct block_cache;
typedef DoublyLinkedListLink<cached_block> block_link;


struct cached_block {
	cached_block	*next;			// next in hash
	cached_block	*transaction_next;
	block_link		link;
	fssh_off_t		block_number;
	void			*current_data;
	void			*original_data;
	void			*parent_data;
#ifdef DEBUG_CHANGED
	void			*compare;
#endif
	int32_t			ref_count;
	int32_t			accessed;
	bool			busy : 1;
	bool			is_writing : 1;
	bool			is_dirty : 1;
	bool			unused : 1;
	bool			unmapped : 1;
	cache_transaction *transaction;
	cache_transaction *previous_transaction;

	static int Compare(void *_cacheEntry, const void *_block);
	static uint32_t Hash(void *_cacheEntry, const void *_block, uint32_t range);
};

typedef DoublyLinkedList<cached_block,
	DoublyLinkedListMemberGetLink<cached_block,
		&cached_block::link> > block_list;

struct block_cache {
	hash_table	*hash;
	benaphore	lock;
	int			fd;
	fssh_off_t	max_blocks;
	fssh_size_t	block_size;
	int32_t		allocated_block_count;
	int32_t		next_transaction_id;
	cache_transaction *last_transaction;
	hash_table	*transaction_hash;

	block_list	unused_blocks;

	bool		read_only;

	block_cache(int fd, fssh_off_t numBlocks, fssh_size_t blockSize, bool readOnly);
	~block_cache();

	fssh_status_t InitCheck();

	void RemoveUnusedBlocks(int32_t maxAccessed = LONG_MAX, int32_t count = LONG_MAX);
	void FreeBlock(cached_block *block);
	cached_block *NewBlock(fssh_off_t blockNumber);
	void Free(void *address);
	void *Allocate();

	static void LowMemoryHandler(void *data, int32_t level);
};


}	// namespace FSShell


#endif	/* _FSSH_BLOCK_CACHE_PRIVATE_H */
