/*
 * Copyright 2023, Your Name, your.email@example.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_BLOCK_CACHE_PRIVATE_H
#define _KERNEL_BLOCK_CACHE_PRIVATE_H

#include "unified_cache.h"
#include <fs_cache.h> // For transaction_notification_hook
#include <lock.h>
#include <kernel.h> // For struct system_info, team_id etc.
#include <util/DoublyLinkedList.h>
#include <util/OpenHashTable.h> // For TransactionTable if transactions remain similar

// Forward declarations
struct cache_listener;
struct cache_transaction;
struct unified_cache_entry; // From unified_cache.h, but good to forward declare

typedef DoublyLinkedList<cache_listener> ListenerList;

// This structure will hold the per-instance data for a "block cache"
struct block_cache_instance_data {
	unified_cache*  master_cache;
	int             fd;
	off_t           num_blocks;
	size_t          block_size;
	bool            read_only;

	mutex           transaction_lock;
	int32           next_transaction_id;
	cache_transaction* last_transaction;

	struct TransactionHashDefinition {
		typedef int32               KeyType;
		typedef cache_transaction   ValueType;

		size_t HashKey(KeyType key) const { return key; }
		size_t Hash(ValueType* value) const; // Implemented in .cpp
		bool Compare(KeyType key, ValueType* value) const; // Implemented in .cpp
		ValueType*& GetLink(ValueType* value) const; // Implemented in .cpp
	};
	typedef BOpenHashTable<TransactionHashDefinition> TransactionTable;
	TransactionTable*       transaction_hash;

	// For managing pending notifications (if this logic is kept)
	mutex			notifications_lock; // Separate from main cache lock
	DoublyLinkedList<cache_listener> pending_notifications; // Listeners waiting for events

	// Condition variables from old block_cache, now per instance
	ConditionVariable busy_reading_condition;
	ConditionVariable busy_writing_condition;
	ConditionVariable transaction_sync_condition; // For cache_sync_transaction waits

	uint32			busy_reading_count;
	bool			busy_reading_waiters;
	uint32			busy_writing_count;
	bool			busy_writing_waiters; // If specific waiters on block, else use CV broadcast

	// Stats from old block_cache
	bigtime_t		last_block_write;
	bigtime_t		last_block_write_duration;
	uint32			num_dirty_blocks; // If tracking dirty blocks outside unified_cache for this instance

	block_cache_instance_data(int _fd, off_t _num_blocks, size_t _block_size, bool _read_only);
	~block_cache_instance_data();
	status_t Init(); // To initialize hash table, CVs etc.
};

// Represents a transaction
struct cache_transaction {
	cache_transaction* next_hash_link; // For TransactionTable
	int32			id;
	bool			open;
	bigtime_t		last_used;
	ListenerList	listeners; 		// List of listeners for this transaction

	// How to track blocks in a transaction without cached_block?
	// Option 1: List of referenced unified_cache_entry pointers.
	// Option 2: List of block_ids (uint64).
	// Let's try with unified_cache_entry pointers for now, means they must be kept referenced.
	DoublyLinkedList<unified_cache_entry,
		DoublyLinkedListMemberGetLink<unified_cache_entry,
			&unified_cache_entry::sieve_link> > blocks_in_transaction;
			// Reusing sieve_link for transaction list might be problematic if an entry
			// is in multiple lists. Better to have a dedicated transaction_link in unified_cache_entry
			// or manage this list by other means (e.g. separate link struct or list of IDs).
			// For now, this is a placeholder showing the need.

	// Let's use a simpler list of block IDs for now, assuming original_data is handled
	// by copying to a temporary buffer if a transaction modifies a block.
	struct TransactionBlockEntry : DoublyLinkedListLinkImpl<TransactionBlockEntry> {
		uint64 block_id;
		void* original_data; // if needed for rollback for this transaction
		void* parent_data;   // if part of a sub-transaction
		bool  discard_in_transaction;
		// Other transaction-specific flags for this block
		TransactionBlockEntry(uint64 id) : block_id(id), original_data(NULL), parent_data(NULL), discard_in_transaction(false) {}
	};
	typedef DoublyLinkedList<TransactionBlockEntry> TransactionBlockList;
	TransactionBlockList	blocks; // Blocks modified by this transaction specifically

	unified_cache_entry* first_block; // This was used for iteration, needs rethink.

	int32			num_blocks;
	int32			main_num_blocks; // For sub-transactions
	int32			sub_num_blocks;  // For sub-transactions
	bool			has_sub_transaction;
	int32			busy_writing_count; // Count of blocks from this transaction currently being written

	cache_transaction(int32 _id);
	// No default constructor if ID is mandatory
};

// cache_notification and cache_listener from block_cache.cpp anonymous namespace,
// possibly moved here or to a shared internal header if ListenerList is in cache_transaction.
struct cache_notification_data : DoublyLinkedListLinkImpl<cache_notification_data> {
	int32			transaction_id;
	int32			events_pending;
	int32			events;
	transaction_notification_hook hook;
	void*			data;
	bool			delete_after_event;
};

// This was cache_listener in the anonymous namespace
struct block_cache_listener : cache_notification_data {
	DoublyLinkedListLink<block_cache_listener> link; // For ListenerList
};

// Re-declare ListenerList using the correct type
typedef DoublyLinkedList<block_cache_listener,
	DoublyLinkedListMemberGetLink<block_cache_listener,
		&block_cache_listener::link> > ActualListenerList;
// And cache_transaction should use ActualListenerList listeners;


#endif // _KERNEL_BLOCK_CACHE_PRIVATE_H
