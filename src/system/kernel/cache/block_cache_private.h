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
#include <kernel.h>
#include <lock.h>
#include <util/DoublyLinkedList.h>
#include <util/OpenHashTable.h>


// DoublyLinkedListLink
template<typename Element>
struct DoublyLinkedListLink {
	Element*	previous;
	Element*	next;
};


// Forward declarations
struct cached_block;
struct cache_transaction;
struct block_cache; // Full definition will be below

// Structure for cache notifications (listeners)
struct cache_listener {
	DoublyLinkedListLink<cache_listener> link;
	void*				data;
	transaction_notification_hook hook;
	uint32				events; // Events this listener is interested in
	cache_transaction*	transaction; // The transaction this listener is attached to
};

typedef DoublyLinkedList<cache_listener> ListenerList;


// Hash table definition for cached_block
struct BlockHashDefinition {
	typedef off_t			KeyType;
	typedef	cached_block	ValueType;

	size_t HashKey(off_t key) const
	{
		return (size_t)key;
	}

	size_t Hash(cached_block* value) const; // Implemented in block_cache.cpp
	bool Compare(off_t key, cached_block* value) const; // Implemented in block_cache.cpp
	cached_block*& GetLink(cached_block* value) const; // Implemented in block_cache.cpp
};

typedef OpenHashTable<BlockHashDefinition, true> BlockTable;


// Hash table definition for cache_transaction
struct TransactionHashDefinition {
	typedef int32			KeyType;
	typedef cache_transaction ValueType;

	size_t HashKey(int32 key) const
	{
		return (size_t)key;
	}

	size_t Hash(cache_transaction* value) const; // Implemented in block_cache.cpp
	bool Compare(int32 key, cache_transaction* value) const; // Implemented in block_cache.cpp
	cache_transaction*& GetLink(cache_transaction* value) const; // Implemented in block_cache.cpp
};

typedef OpenHashTable<TransactionHashDefinition, true> TransactionTable;


// cached_block structure
struct cached_block : DoublyLinkedListLink<cached_block> {
	off_t				block_number;
	void*				data;			// points to the data in the cache
	bool				busy_writing;
	bool				busy_reading;
	bool				is_dirty;
	bool				is_writing;		// used by the BlockWriter
	cache_transaction*	transaction;
	cache_transaction*	previous_transaction;
										// links all blocks of a transaction
	cached_block*		transaction_next;
	cached_block*		transaction_prev;

	cached_block*		hash_link;		// used by BlockTable

	// Default constructor
	cached_block()
		: block_number(0), data(NULL), busy_writing(false), busy_reading(false),
		  is_dirty(false), is_writing(false), transaction(NULL),
		  previous_transaction(NULL), transaction_next(NULL), transaction_prev(NULL),
		  hash_link(NULL)
	{
		// DoublyLinkedListLink members (previous, next) are implicitly default-initialized
	}

	bool CanBeWritten() const; // Implemented in block_cache.cpp
};

typedef DoublyLinkedList<cached_block> block_list;


// cache_transaction structure
struct cache_transaction {
	int32				id;
	uint32				num_blocks;		// number of blocks in this transaction
	uint32				pending_notifications;
	int32				nesting_level;	// you can nest transactions via sub transactions
	bool				has_sub_transactions;
	bool				is_sub_transaction;
	cache_transaction*	parent_transaction;
	block_list			blocks;			// list of all blocks in this transaction
	cached_block*		first_block;	// shortcut
	ListenerList		listeners;
	cache_transaction*	next_block_transaction;
										// used by the block writer
	cache_transaction*	hash_link;		// used by TransactionTable
	int32				dependency_count; // for sub-transactions

	cache_transaction(int32 _id)
		: id(_id), num_blocks(0), pending_notifications(0), nesting_level(0),
		  has_sub_transactions(false), is_sub_transaction(false),
		  parent_transaction(NULL), first_block(NULL),
		  next_block_transaction(NULL), hash_link(NULL), dependency_count(0)
	{
	}

	// Removed default constructor if it's not needed or causes issues
	// cache_transaction() = default;
};


// block_cache structure (main cache structure)
struct block_cache {
	mutex				lock;
	int					fd;				// descriptor of the block device
	off_t				num_blocks;
	size_t				block_size;
	bool				read_only;

	BlockTable*			hash;			// hash table for all blocks in the cache
	block_list			unused_blocks;	// list of all unused blocks
	uint32				unused_block_count;
	TransactionTable*	transaction_hash; // hash table for all transactions

	ConditionVariable	busy_reading_condition;
	ConditionVariable	busy_writing_condition;
	ConditionVariable	transaction_condition; // General condition for transactions

	struct object_cache* buffer_cache; // Slab cache for block buffers

	// Link for the global list of caches
	DoublyLinkedListLink<block_cache> link;

	block_cache(int _fd, off_t _numBlocks, size_t _blockSize, bool _readOnly);
	~block_cache();

	status_t Init();
	void Free(void* buffer);
	void* Allocate();

	void FreeBlock(cached_block* block);
	void FreeBlockParentData(cached_block* block); // If block has parent data to free
	void RemoveUnusedBlocks(int32 count, int32 minSecondsOld = 0);
	void RemoveBlock(cached_block* block);
	void DiscardBlock(cached_block* block);

	static void _LowMemoryHandler(void* data, uint32 resources, int32 level);
};


// Global variables related to block_cache that were previously in block_cache.cpp
// and caused "not declared in this scope" errors.
// These should be declared (extern if needed) or defined appropriately.
// For now, as they are used within block_cache.cpp, defining them here might
// work if block_cache.cpp includes this header early.
// However, the proper place for definitions is a .cpp file.
// For now, I'll add declarations. The actual definitions will remain in block_cache.cpp

extern DoublyLinkedList<block_cache> sCaches;
extern mutex sCachesLock;
extern mutex sNotificationsLock; // Used for cache_notification list
extern mutex sCachesMemoryUseLock;
extern object_cache* sBlockCache; // Object cache for cached_block structs
extern object_cache* sCacheNotificationCache; // Object cache for cache_notification
extern sem_id sEventSemaphore; // For block_notifier_and_writer
extern thread_id sNotifierWriterThread; // block_notifier_and_writer thread
extern block_cache sMarkCache; // A special marker cache instance
extern size_t sUsedMemory; // Total memory used by block caches


#endif	// BLOCK_CACHE_PRIVATE_H
