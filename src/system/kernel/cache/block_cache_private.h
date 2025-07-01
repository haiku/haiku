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

// Forward declarations from the old block_cache.cpp (if parts of transaction logic are kept)
// These are illustrative and will be refined.
struct cache_transaction_block_data; // If we keep transaction logic separate

// This structure will hold the per-instance data for a "block cache"
// which now primarily acts as a frontend to the unified_cache for block data,
// and manages block-specific logic like transactions if they aren't fully
// pushed into the unified_cache.
struct block_cache_instance_data {
    unified_cache*  master_cache;       // Pointer to the global unified cache instance
    int             fd;                 // File descriptor for the block device
    off_t           num_blocks;         // Total number of blocks on the device
    size_t          block_size;         // Size of each block
    bool            read_only;          // Is the cache read-only?

    // Transaction-related fields (if kept separate from unified_cache_entry)
    // This part needs significant redesign. For now, let's assume transactions
    // are still managed here, operating on data obtained from the unified cache.
    mutex                   transaction_lock; // Separate lock for transaction metadata
    int32                   next_transaction_id;
    struct cache_transaction* last_transaction; // old struct cache_transaction might be reused/adapted

    // We might need a way to map transaction IDs to transaction metadata.
    // The old TransactionTable might be adapted.
    // TransactionTable*       transaction_hash;


    // List of active transactions specific to this block device instance.
    // This is highly dependent on how transaction logic is refactored.
    // For now, keeping it conceptual.
    // DoublyLinkedList<adapted_cache_transaction> active_transactions;

    // TODO: Other fields needed to maintain the block cache API,
    // especially for transaction management.
    // The original `block_cache` struct had listener lists, notification lists, etc.
    // These will need to be re-evaluated.

    block_cache_instance_data(int _fd, off_t _num_blocks, size_t _block_size, bool _read_only)
        : master_cache(NULL), fd(_fd), num_blocks(_num_blocks), block_size(_block_size),
          read_only(_read_only), next_transaction_id(1), last_transaction(NULL)
          /* transaction_hash(NULL) */ {
        mutex_init(&transaction_lock, "block cache transaction lock");
    }

    ~block_cache_instance_data() {
        mutex_destroy(&transaction_lock);
        // unified_cache_destroy(master_cache); // This should be managed globally or by who created it
        // delete transaction_hash;
    }
};

// If transaction logic is heavily refactored, the old cache_transaction might look different.
// For now, let's assume we might need a similar structure.
struct cache_transaction {
    cache_transaction* next_hash_link; // For a potential hash table of transactions
    int32			id;
    // bool			open;
    // bigtime_t		last_used;
    // ListenerList	listeners; // From old block_cache.cpp

    // Which blocks are part of this transaction?
    // This needs a new way to be tracked, as 'cached_block' is gone.
    // Perhaps a list of block numbers or unified_cache_entry pointers (referenced).
    // DoublyLinkedList<uint64> dirty_block_ids_in_transaction;

    // Minimal for now, to be expanded
    cache_transaction(int32 _id) : id(_id) {}
};

// TODO: Define TransactionHash if needed for a hash table of cache_transactions.

#endif // _KERNEL_BLOCK_CACHE_PRIVATE_H
