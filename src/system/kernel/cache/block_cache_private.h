/*
 * Copyright 2004-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef BLOCK_CACHE_PRIVATE_H
#define BLOCK_CACHE_PRIVATE_H


#include <lock.h>
#include <slab/Slab.h>
#include <util/DoublyLinkedList.h>

struct hash_table;
struct vm_page;


#define DEBUG_CHANGED


struct cache_transaction;
struct cached_block;
struct block_cache;
typedef DoublyLinkedListLink<cached_block> block_link;


struct cached_block {
	cached_block	*next;			// next in hash
	cached_block	*transaction_next;
	block_link		link;
	off_t			block_number;
	void			*current_data;
	void			*original_data;
	void			*parent_data;
#ifdef DEBUG_CHANGED
	void			*compare;
#endif
	int32			ref_count;
	int32			accessed;
	bool			busy : 1;
	bool			is_writing : 1;
	bool			is_dirty : 1;
	bool			unused : 1;
	cache_transaction *transaction;
	cache_transaction *previous_transaction;

	static int Compare(void *_cacheEntry, const void *_block);
	static uint32 Hash(void *_cacheEntry, const void *_block, uint32 range);
};

typedef DoublyLinkedList<cached_block,
	DoublyLinkedListMemberGetLink<cached_block,
		&cached_block::link> > block_list;

struct block_cache : DoublyLinkedListLinkImpl<block_cache> {
	hash_table	*hash;
	benaphore	lock;
	int			fd;
	off_t		max_blocks;
	size_t		block_size;
	int32		next_transaction_id;
	cache_transaction *last_transaction;
	hash_table	*transaction_hash;

	object_cache *buffer_cache;
	block_list	unused_blocks;

	uint32		num_dirty_blocks;
	bool		read_only;

	block_cache(int fd, off_t numBlocks, size_t blockSize, bool readOnly);
	~block_cache();

	status_t InitCheck();

	void RemoveUnusedBlocks(int32 maxAccessed = LONG_MAX,
		int32 count = LONG_MAX);
	void FreeBlock(cached_block *block);
	cached_block *NewBlock(off_t blockNumber);
	void Free(void *buffer);
	void *Allocate();

	static void LowMemoryHandler(void *data, int32 level);

private:
	cached_block *_GetUnusedBlock();
};

#endif	/* BLOCK_CACHE_PRIVATE_H */
