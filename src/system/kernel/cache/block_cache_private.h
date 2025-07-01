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

#include <block_cache.h>
#include <condition_variable.h>
#include <kernel.h> // For basic types like int32, uint32, off_t, size_t, bigtime_t
#include <lock.h>   // For mutex
#include <fs_cache.h> // For transaction_notification_hook
#include <util/DoublyLinkedList.h>
#include <util/OpenHashTable.h> // For BOpenHashTable

// Forward declarations
struct cached_block;
struct cache_transaction;
struct block_cache;
struct cache_notification; // Forward declare for DoublyLinkedList

// Use the DoublyLinkedListLink from DoublyLinkedList.h, no redefinition needed.

// Structure for cache notifications (listeners)
struct cache_listener : DoublyLinkedListLink<cache_listener> {
	// DoublyLinkedListLink<cache_listener> link; // This is how it's typically inherited
	void*				data;
	transaction_notification_hook hook; // Now included via fs_cache.h
	uint32				events;
	cache_transaction*	transaction;
};

typedef DoublyLinkedList<cache_listener> ListenerList;


// Hash table definition for cached_block
struct BlockHashDefinition {
	typedef off_t			KeyType;
	typedef	cached_block	ValueType;

	size_t HashKey(off_t key) const { return (size_t)key; }
	size_t Hash(cached_block* value) const;
	bool Compare(off_t key, cached_block* value) const;
	cached_block*& GetLink(cached_block* value) const;
};

typedef BOpenHashTable<BlockHashDefinition, true> BlockTable; // Changed to BOpenHashTable


// Hash table definition for cache_transaction
struct TransactionHashDefinition {
	typedef int32			KeyType;
	typedef cache_transaction ValueType;

	size_t HashKey(int32 key) const { return (size_t)key; }
	size_t Hash(cache_transaction* value) const;
	bool Compare(int32 key, cache_transaction* value) const;
	cache_transaction*& GetLink(cache_transaction* value) const;
};

typedef BOpenHashTable<TransactionHashDefinition, true> TransactionTable; // Changed to BOpenHashTable


// cached_block structure
struct cached_block : DoublyLinkedListLink<cached_block> {
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

	// Added missing members based on usage errors
	int32				ref_count;
	bigtime_t			last_accessed; // Or int32 if that was the original type
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

typedef DoublyLinkedList<cached_block> block_list;


// cache_transaction structure
struct cache_transaction {
	int32				id;
	uint32				num_blocks;
	// uint32				pending_notifications; // This member was in error log for block_cache, not cache_transaction
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

	// Added missing members
	bool				open;
	int32				busy_writing_count; // From error log
	bigtime_t			last_used; // From error log (implicit)


	cache_transaction(int32 _id)
		: id(_id), num_blocks(0), /*pending_notifications(0),*/ nesting_level(0),
		  has_sub_transactions(false), is_sub_transaction(false),
		  parent_transaction(NULL), first_block(NULL),
		  next_block_transaction(NULL), hash_link(NULL), dependency_count(0),
		  open(true), busy_writing_count(0), last_used(0)
	{
	}
};


// block_cache structure (main cache structure)
struct block_cache {
	mutex				lock;
	int					fd;
	off_t				num_blocks;
	size_t				block_size;
	bool				read_only;

	BlockTable*			hash;
	block_list			unused_blocks;
	uint32				unused_block_count;
	TransactionTable*	transaction_hash; // Added based on usage errors
	cache_transaction*	last_transaction; // Added based on usage errors

	ConditionVariable	busy_reading_condition;
	ConditionVariable	busy_writing_condition;
	ConditionVariable	transaction_condition;

	object_cache*		buffer_cache; // Changed from struct object_cache*

	DoublyLinkedListLink<block_cache> link;
	DoublyLinkedList<cache_notification> pending_notifications; // Added based on usage errors

	// Added members based on usage errors
	int32				next_transaction_id;
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
	cached_block* NewBlock(off_t blockNumber); // Declaration added
	cached_block* _GetUnusedBlock();          // Declaration added


	static void _LowMemoryHandler(void* data, uint32 resources, int32 level);
};


// Extern declarations for global variables defined in block_cache.cpp
extern DoublyLinkedList<block_cache> sCaches;
extern mutex sCachesLock;
extern mutex sNotificationsLock;
extern mutex sCachesMemoryUseLock;
extern object_cache* sBlockCache;
extern object_cache* sCacheNotificationCache;
extern object_cache* sTransactionObjectCache; // Added extern
extern sem_id sEventSemaphore;
extern thread_id sNotifierWriterThread;
extern block_cache sMarkCache;
extern size_t sUsedMemory;


#endif	// BLOCK_CACHE_PRIVATE_H
