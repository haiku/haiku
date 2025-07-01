/*
 * Copyright 2004-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef BLOCK_CACHE_PRIVATE_H
#define BLOCK_CACHE_PRIVATE_H

#include <slab/Slab.h> // For object_cache typedef
#include <fs_cache.h> // For transaction_notification_hook

#include <block_cache.h> // Public API
#include <condition_variable.h>
#include <kernel.h>
#include <lock.h>
#include <util/DoublyLinkedList.h>
#include <util/OpenHashTable.h>

// Forward declarations
struct cached_block;
struct cache_transaction;
struct block_cache;
extern object_cache* sCacheNotificationCache;


struct cache_notification {
	DoublyLinkedListLink<cache_notification> link; // For pending_notifications list

	int32			transaction_id;
	int32			events_pending;
	int32			events;
	transaction_notification_hook hook;
	void*			data;
	bool			delete_after_event;

	// Inline new/delete to ensure definition is available and to handle -Werror=return-type
	static inline void* operator new(size_t size) {
		if (sCacheNotificationCache == NULL)
			panic("cache_notification::new: sCacheNotificationCache is not initialized!");
		void* block = object_cache_alloc(sCacheNotificationCache, 0);
		if (block == NULL)
			panic("cache_notification::new: object_cache_alloc failed!");
		return block;
	}

	static inline void operator delete(void* block) {
		if (sCacheNotificationCache == NULL) {
			// This might happen during shutdown if caches are cleaned up late.
			// It's not ideal to panic here if sCacheNotificationCache is gone.
			// Consider how critical this is or if a TRACE_ALWAYS is better.
			// For now, to avoid panic during potential shutdown races:
			if (block != NULL) {
				// Can't free via object_cache, potential leak or use after free if called late.
				// This indicates a system design issue if sCacheNotificationCache is gone before all instances.
				// panic("cache_notification::delete: sCacheNotificationCache is NULL but block is not!");
				TRACE_ALWAYS("cache_notification::delete: sCacheNotificationCache is NULL! Potential leak for block %p\n", block);
			}
			return;
		}
		if (block != NULL)
			object_cache_free(sCacheNotificationCache, block, 0);
	}

	cache_notification()
		: transaction_id(0), events_pending(0), events(0), hook(NULL), data(NULL),
		  delete_after_event(false)
	{
	}
};

struct cache_listener : public cache_notification {
	DoublyLinkedListLink<cache_listener> listener_list_link;
};

typedef DoublyLinkedList<cache_listener,
	DoublyLinkedListMemberGetLink<cache_listener,
		&cache_listener::listener_list_link> > ListenerList;


struct BlockHashDefinition {
	typedef off_t			KeyType;
	typedef	cached_block	ValueType;

	size_t HashKey(off_t key) const { return (size_t)key; }
	size_t Hash(cached_block* value) const;
	bool Compare(off_t key, cached_block* value) const;
	cached_block*& GetLink(cached_block* value) const;
};

typedef BOpenHashTable<BlockHashDefinition> BlockTable;


struct TransactionHashDefinition {
	typedef int32			KeyType;
	typedef cache_transaction ValueType;

	size_t HashKey(int32 key) const { return (size_t)key; }
	size_t Hash(cache_transaction* value) const;
	bool Compare(int32 key, cache_transaction* value) const;
	cache_transaction*& GetLink(cache_transaction* value) const;
};

typedef BOpenHashTable<TransactionHashDefinition> TransactionTable;


struct cached_block {
	DoublyLinkedListLink<cached_block> link;

	off_t				block_number;
	void*				data;
	bool				busy_writing;
	bool				busy_reading;
	bool				is_dirty;
	bool				is_writing;
	cache_transaction*	transaction;
	cache_transaction*	previous_transaction;
	cached_block*		transaction_next;
	cached_block*		transaction_prev;
	cached_block*		hash_link;

	int32				ref_count;
	bigtime_t			last_accessed;
	bool				unused;
	bool				discard;
	void*				original_data;
	void*				parent_data;
	bool				busy_reading_waiters;
	bool				busy_writing_waiters;

	cached_block()
		: block_number(0), data(NULL), busy_writing(false), busy_reading(false),
		  is_dirty(false), is_writing(false), transaction(NULL),
		  previous_transaction(NULL), transaction_next(NULL), transaction_prev(NULL),
		  hash_link(NULL), ref_count(0), last_accessed(0), unused(false), discard(false),
		  original_data(NULL), parent_data(NULL), busy_reading_waiters(false),
		  busy_writing_waiters(false)
	{
	}

	bool CanBeWritten() const;
};

typedef DoublyLinkedList<cached_block,
	DoublyLinkedListMemberGetLink<cached_block,
		&cached_block::link> > block_list;


struct cache_transaction {
	DoublyLinkedListLink<cache_transaction> link;

	int32				id;
	uint32				num_blocks;
	int32				nesting_level;
	bool				has_sub_transactions;
	bool				is_sub_transaction;
	cache_transaction*	parent_transaction;
	block_list			blocks;
	cached_block*		first_block;
	ListenerList		listeners;
	cache_transaction*	next_block_transaction;
	cache_transaction*	hash_link;
	int32				dependency_count;
	bool				open;
	int32				busy_writing_count;
	bigtime_t			last_used;
	uint32				main_num_blocks;
	uint32				sub_num_blocks;

	cache_transaction(int32 _id)
		: id(_id), num_blocks(0), nesting_level(0),
		  has_sub_transactions(false), is_sub_transaction(false),
		  parent_transaction(NULL), first_block(NULL),
		  next_block_transaction(NULL), hash_link(NULL), dependency_count(0),
		  open(true), busy_writing_count(0), last_used(0),
		  main_num_blocks(0), sub_num_blocks(0)
	{
	}
};


struct block_cache {
	DoublyLinkedListLink<block_cache> link;

	mutex				lock;
	int					fd;
	off_t				num_blocks;
	size_t				block_size;
	bool				read_only;

	BlockTable*			hash;
	block_list			unused_blocks;
	TransactionTable*	transaction_hash;
	cache_transaction*	last_transaction;

	ConditionVariable	busy_reading_condition;
	ConditionVariable	busy_writing_condition;
	ConditionVariable	transaction_condition;

	object_cache*		buffer_cache;

	DoublyLinkedList<cache_notification,
		DoublyLinkedListMemberGetLink<cache_notification,
			&cache_notification::link> > pending_notifications;

	int32				next_transaction_id;
	uint32				unused_block_count;
	uint32				busy_reading_count;
	bool				busy_reading_waiters;
	uint32				busy_writing_count;
	bool				busy_writing_waiters;
	bigtime_t			last_block_write;
	bigtime_t			last_block_write_duration;
	uint32				num_dirty_blocks;

	block_cache(int _fd, off_t _numBlocks, size_t _blockSize, bool _readOnly);
	~block_cache();

	status_t Init();
	void Free(void* buffer);
	void* Allocate();

	void FreeBlock(cached_block* block);
	void FreeBlockParentData(cached_block* block);
	void RemoveUnusedBlocks(int32 count, int32 minSecondsOld = 0);
	void RemoveBlock(cached_block* block);
	void DiscardBlock(cached_block* block);
	cached_block* NewBlock(off_t blockNumber);
	cached_block* _GetUnusedBlock();

	static void _LowMemoryHandler(void* data, uint32 resources, int32 level);
};


// Extern declarations for global variables
extern DoublyLinkedList<block_cache, DoublyLinkedListMemberGetLink<block_cache, &block_cache::link> > sCaches;
extern mutex sCachesLock;
extern mutex sNotificationsLock;
extern mutex sCachesMemoryUseLock;
extern object_cache* sBlockCache;
// sCacheNotificationCache is forward declared at the top, defined in .cpp
extern object_cache* sTransactionObjectCache;
extern sem_id sEventSemaphore;
extern thread_id sNotifierWriterThread;
extern block_cache sMarkCache;
extern size_t sUsedMemory;


#endif	// BLOCK_CACHE_PRIVATE_H
