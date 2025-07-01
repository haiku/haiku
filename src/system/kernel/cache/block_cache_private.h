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

#include <slab/Slab.h> // For object_cache typedef (MUST be before users)
#include <fs_cache.h> // For transaction_notification_hook (MUST be before users)

#include <block_cache.h> // Public API
#include <condition_variable.h>
#include <kernel.h>
#include <lock.h>
#include <util/DoublyLinkedList.h>
#include <util/OpenHashTable.h> // For BOpenHashTable

// Forward declarations
struct cached_block;
struct cache_transaction;
struct block_cache;


// cache_notification and cache_listener definitions
// Moved here from block_cache.cpp to resolve incomplete type errors.

struct cache_notification : DoublyLinkedListLink<cache_notification> {
	int32			transaction_id;
	int32			events_pending;
	int32			events;
	transaction_notification_hook hook;
	void*			data;
	bool			delete_after_event;

	// It's crucial that sCacheNotificationCache is initialized before this is ever used.
	// These operators should ideally be in the .cpp file if they rely on globals from there,
	// but defining them inline helps with some compiler visibility issues for templates.
	// However, this can lead to multiple definition errors if not careful.
	// For now, keeping them here to solve incomplete type errors during member access.
	// The proper fix is to ensure full definition is seen before use,
	// which might mean moving these operator definitions to the cpp and ensuring
	// all users of cache_notification only work with pointers/references until
	// the cpp is linked. Given the current errors, inline definition is a temporary fix.

	// static inline void* operator new(size_t size) {
	// 	if (sCacheNotificationCache == NULL) panic("sCacheNotificationCache not init!");
	// 	void* block = object_cache_alloc(sCacheNotificationCache, 0);
	// 	if (block == NULL) panic("cache_notification new failed!");
	// 	return block;
	// }
	// static inline void operator delete(void* block) {
	// 	if (sCacheNotificationCache == NULL) panic("sCacheNotificationCache not init for delete!");
	// 	if (block != NULL) object_cache_free(sCacheNotificationCache, block, 0);
	// }
	// NOTE: Removing inline new/delete from here. They will be in the .cpp file.
	// The "invalid use of incomplete type" errors should be resolved by having the
	// full struct definition available. The operator definition errors are separate.


	cache_notification()
		: transaction_id(0), events_pending(0), events(0), hook(NULL), data(NULL),
		  delete_after_event(false)
	{
	}
};

struct cache_listener : cache_notification {
	// This struct inherits `link` from `cache_notification` if ListenerList is
	// `DoublyLinkedList<cache_notification>`.
	// If ListenerList is `DoublyLinkedList<cache_listener>`, then cache_listener
	// itself needs to correctly provide the link, which it does by inheriting
	// from cache_notification which inherits from DoublyLinkedListLink<cache_notification>.
	// This is a bit tangled. A cleaner way:
	// struct cache_listener : DoublyLinkedListLink<cache_listener> { ... members of cache_notification ... };
	// For now, assume current structure and ListenerList<cache_listener> implies it uses
	// the link from its base `cache_notification` which is `DoublyLinkedListLink<cache_notification>`.
	// This will likely fail if `ListenerList` is `DoublyLinkedList<cache_listener>`.
	// Let's assume `ListenerList` is `DoublyLinkedList<cache_listener>`.
	// The `cache_listener` should inherit `DoublyLinkedListLink<cache_listener>`.
	// However, it also inherits `DoublyLinkedListLink<cache_notification>`.
	// This needs untangling. For now, stick to what `block_cache_private.h` previously had:
	// cache_listener inherits cache_notification, and ListenerList is DoublyLinkedList<cache_listener>.
	// This means cache_listener must provide the link for DoublyLinkedList<cache_listener>.
	// The easiest way is:
	// struct cache_listener : public cache_notification, public DoublyLinkedListLink<cache_listener> {};
	// But that is multiple inheritance of link types.

	// Simpler: Make cache_listener have its own link and copy relevant fields
	// Or, ensure ListenerList uses a GetLink policy for cache_notification's link.

	// For now, let's assume ListenerList is DoublyLinkedList<cache_listener>,
	// and cache_listener provides its own link for that list type.
	// This means it should NOT inherit DoublyLinkedListLink from cache_notification if that's
	// intended for a different list.
	// Given the errors, it seems `ListenerList` is `DoublyLinkedList<cache_listener>` and
	// it's looking for `cache_listener::link` or `cache_listener::GetDoublyLinkedListLink()`.
	// The original `cache_listener` in `block_cache.cpp` had `listener_link link;`
	// where `listener_link` was `typedef DoublyLinkedListLink<cache_listener> listener_link;`
	// This is the correct pattern.
	DoublyLinkedListLink<cache_listener> listener_list_link; // Specific link for ListenerList
};

typedef DoublyLinkedList<cache_listener, DoublyLinkedListMemberGetLink<cache_listener, &cache_listener::listener_list_link> > ListenerList;


// Hash table definition for cached_block
struct BlockHashDefinition {
	typedef off_t			KeyType;
	typedef	cached_block	ValueType;

	size_t HashKey(off_t key) const { return (size_t)key; }
	size_t Hash(cached_block* value) const;
	bool Compare(off_t key, cached_block* value) const;
	cached_block*& GetLink(cached_block* value) const;
};

typedef BOpenHashTable<BlockHashDefinition> BlockTable; // Removed ", true" - default is non-intrusive via GetLink


// Hash table definition for cache_transaction
struct TransactionHashDefinition {
	typedef int32			KeyType;
	typedef cache_transaction ValueType;

	size_t HashKey(int32 key) const { return (size_t)key; }
	size_t Hash(cache_transaction* value) const;
	bool Compare(int32 key, cache_transaction* value) const;
	cache_transaction*& GetLink(cache_transaction* value) const;
};

typedef BOpenHashTable<TransactionHashDefinition> TransactionTable; // Removed ", true"


// cached_block structure
struct cached_block {
	DoublyLinkedListLink<cached_block> link; // For block_list

	off_t				block_number;
	void*				data;
	bool				busy_writing;
	bool				busy_reading;
	bool				is_dirty;
	bool				is_writing;
	cache_transaction*	transaction;
	cache_transaction*	previous_transaction;
	cached_block*		transaction_next; // For linking blocks within a transaction
	cached_block*		transaction_prev; // For linking blocks within a transaction
	cached_block*		hash_link;      // For BlockTable's hash linkage

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
struct cache_transaction {
	DoublyLinkedListLink<cache_transaction> link; // For a global list of transactions, if any

	int32				id;
	uint32				num_blocks;
	int32				nesting_level;
	bool				has_sub_transactions;
	bool				is_sub_transaction;
	cache_transaction*	parent_transaction;
	block_list			blocks; // List of cached_block in this transaction
	cached_block*		first_block; // Head of a direct list of blocks in transaction
	ListenerList		listeners;
	cache_transaction*	next_block_transaction; // For BlockWriter's list of transactions to write
	cache_transaction*	hash_link; // For TransactionTable's hash linkage
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


// block_cache structure (main cache structure)
struct block_cache {
	DoublyLinkedListLink<block_cache> link; // For sCaches list

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
extern block_cache sMarkCache; // Ensure this is compatible with constructor
extern size_t sUsedMemory;


#endif	// BLOCK_CACHE_PRIVATE_H
