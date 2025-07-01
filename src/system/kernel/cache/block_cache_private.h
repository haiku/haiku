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

#include <block_cache.h> // Public API
#include <condition_variable.h>
#include <kernel.h>
#include <lock.h>
#include <fs_cache.h> // For transaction_notification_hook
#include <util/DoublyLinkedList.h>
#include <util/OpenHashTable.h> // For BOpenHashTable
#include <slab/Slab.h> // For object_cache typedef

// Forward declarations
struct cached_block;
struct cache_transaction;
struct block_cache;

// Moved cache_notification and cache_listener definitions here from block_cache.cpp
// to resolve incomplete type errors.

struct cache_notification : DoublyLinkedListLink<cache_notification> {
	// Note: DoublyLinkedListLink<cache_notification> implies 'link' member is inherited.
	// If a different link member name is used, DoublyLinkedList needs a custom policy.
	// For now, assume default 'link' from inheritance.

	// static inline void* operator new(size_t size); // Definition will be in .cpp
	// static inline void operator delete(void* block); // Definition will be in .cpp

	int32			transaction_id;
	int32			events_pending;
	int32			events;
	transaction_notification_hook hook;
	void*			data;
	bool			delete_after_event;

	// Default constructor might be needed if objects are created without explicit initialization
	cache_notification()
		: transaction_id(0), events_pending(0), events(0), hook(NULL), data(NULL),
		  delete_after_event(false)
	{
	}
};

struct cache_listener : cache_notification {
	// Inherits members from cache_notification and DoublyLinkedListLink<cache_notification>
	// If cache_listener needs its own distinct link for ListenerList, it should be:
	// DoublyLinkedListLink<cache_listener> specific_listener_link;
	// For now, assumes it uses the inherited link from cache_notification,
	// which means ListenerList would be DoublyLinkedList<cache_notification>
	// or ListenerList needs a custom GetLink policy for cache_listener.
	// Let's assume ListenerList is of cache_listener and uses its inherited link.
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

typedef BOpenHashTable<BlockHashDefinition, true> BlockTable;


// Hash table definition for cache_transaction
struct TransactionHashDefinition {
	typedef int32			KeyType;
	typedef cache_transaction ValueType;

	size_t HashKey(int32 key) const { return (size_t)key; }
	size_t Hash(cache_transaction* value) const;
	bool Compare(int32 key, cache_transaction* value) const;
	cache_transaction*& GetLink(cache_transaction* value) const;
};

typedef BOpenHashTable<TransactionHashDefinition, true> TransactionTable;


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
	cached_block*		hash_link; // For BlockTable

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

typedef DoublyLinkedList<cached_block> block_list;


// cache_transaction structure
struct cache_transaction : DoublyLinkedListLink<cache_transaction> { // Assuming it can be in a list
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
	cache_transaction*	hash_link; // For TransactionTable
	int32				dependency_count;
	bool				open;
	int32				busy_writing_count;
	bigtime_t			last_used;
	uint32				main_num_blocks; // Added based on usage
	uint32				sub_num_blocks;  // Added based on usage

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


// block_cache structure (main cache structure)
struct block_cache : DoublyLinkedListLink<block_cache> {
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

	object_cache*		buffer_cache; // Use typedef from Slab.h

	DoublyLinkedList<cache_notification> pending_notifications;

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


// Extern declarations for global variables defined in block_cache.cpp
extern DoublyLinkedList<block_cache> sCaches;
extern mutex sCachesLock;
extern mutex sNotificationsLock;
extern mutex sCachesMemoryUseLock;
extern object_cache* sBlockCache;
extern object_cache* sCacheNotificationCache;
extern object_cache* sTransactionObjectCache;
extern sem_id sEventSemaphore;
extern thread_id sNotifierWriterThread;
extern block_cache sMarkCache;
extern size_t sUsedMemory;


#endif	// BLOCK_CACHE_PRIVATE_H
