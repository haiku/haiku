/*
 * Copyright 2004-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "block_cache_priv.h"

#include <new>

#include <stdlib.h>

#include "fssh_atomic.h"
#include "fssh_errno.h"
#include "fssh_fs_cache.h"
#include "fssh_kernel_export.h"
#include "fssh_string.h"
#include "fssh_unistd.h"
#include "hash.h"
#include "vfs.h"

// TODO: this is a naive but growing implementation to test the API:
//	1) block reading/writing is not at all optimized for speed, it will
//	   just read and write single blocks.
//	2) the locking could be improved; getting a block should not need to
//	   wait for blocks to be written
//	3) dirty blocks are only written back if asked for
// TODO: the retrieval/copy of the original data could be delayed until the
//		new data must be written, ie. in low memory situations.

//#define TRACE_BLOCK_CACHE
#ifdef TRACE_BLOCK_CACHE
#	define TRACE(x)	fssh_dprintf x
#else
#	define TRACE(x) ;
#endif

using std::nothrow;

// This macro is used for fatal situations that are acceptable in a running
// system, like out of memory situations - should only panic for debugging.
#define FATAL(x) fssh_panic x


namespace FSShell {

static const int32_t kMaxBlockCount = 1024;

struct cache_hook : DoublyLinkedListLinkImpl<cache_hook> {
	fssh_transaction_notification_hook	hook;
	void								*data;
};

typedef DoublyLinkedList<cache_hook> HookList;

struct cache_transaction {
	cache_transaction();

	cache_transaction *next;
	int32_t			id;
	int32_t			num_blocks;
	int32_t			main_num_blocks;
	int32_t			sub_num_blocks;
	cached_block	*first_block;
	block_list		blocks;
	fssh_transaction_notification_hook notification_hook;
	void			*notification_data;
	HookList		listeners;
	bool			open;
	bool			has_sub_transaction;
};

static fssh_status_t write_cached_block(block_cache *cache, cached_block *block,
	bool deleteTransaction = true);


//	#pragma mark - private transaction


cache_transaction::cache_transaction()
{
	num_blocks = 0;
	main_num_blocks = 0;
	sub_num_blocks = 0;
	first_block = NULL;
	notification_hook = NULL;
	notification_data = NULL;
	open = true;
}


static int
transaction_compare(void *_transaction, const void *_id)
{
	cache_transaction *transaction = (cache_transaction *)_transaction;
	const int32_t *id = (const int32_t *)_id;

	return transaction->id - *id;
}


static uint32_t
transaction_hash(void *_transaction, const void *_id, uint32_t range)
{
	cache_transaction *transaction = (cache_transaction *)_transaction;
	const int32_t *id = (const int32_t *)_id;

	if (transaction != NULL)
		return transaction->id % range;

	return (uint32_t)*id % range;
}


/*!	Notifies all listeners of this transaction, and removes them
	afterwards.
*/
static void
notify_transaction_listeners(cache_transaction *transaction, int32_t event)
{
	HookList::Iterator iterator = transaction->listeners.GetIterator();
	while (iterator.HasNext()) {
		cache_hook *hook = iterator.Next();

		hook->hook(transaction->id, event, hook->data);

		iterator.Remove();
		delete hook;
	}
}


static void
delete_transaction(block_cache *cache, cache_transaction *transaction)
{
	if (cache->last_transaction == transaction)
		cache->last_transaction = NULL;

	delete transaction;
}


static cache_transaction *
lookup_transaction(block_cache *cache, int32_t id)
{
	return (cache_transaction *)hash_lookup(cache->transaction_hash, &id);
}


//	#pragma mark - cached_block


/* static */
int
cached_block::Compare(void *_cacheEntry, const void *_block)
{
	cached_block *cacheEntry = (cached_block *)_cacheEntry;
	const fssh_off_t *block = (const fssh_off_t *)_block;

	return cacheEntry->block_number - *block;
}



/* static */
uint32_t
cached_block::Hash(void *_cacheEntry, const void *_block, uint32_t range)
{
	cached_block *cacheEntry = (cached_block *)_cacheEntry;
	const fssh_off_t *block = (const fssh_off_t *)_block;

	if (cacheEntry != NULL)
		return cacheEntry->block_number % range;

	return (uint64_t)*block % range;
}


//	#pragma mark - block_cache


block_cache::block_cache(int _fd, fssh_off_t numBlocks, fssh_size_t blockSize,
	bool readOnly)
	:
	hash(NULL),
	fd(_fd),
	max_blocks(numBlocks),
	block_size(blockSize),
	allocated_block_count(0),
	next_transaction_id(1),
	last_transaction(NULL),
	transaction_hash(NULL),
	read_only(readOnly)
{
	hash = hash_init(32, 0, &cached_block::Compare, &cached_block::Hash);
	if (hash == NULL)
		return;

	transaction_hash = hash_init(16, 0, &transaction_compare,
		&FSShell::transaction_hash);
	if (transaction_hash == NULL)
		return;

	if (benaphore_init(&lock, "block cache") < FSSH_B_OK)
		return;
}


block_cache::~block_cache()
{
	benaphore_destroy(&lock);

	hash_uninit(transaction_hash);
	hash_uninit(hash);
}


fssh_status_t
block_cache::InitCheck()
{
	if (lock.sem < FSSH_B_OK)
		return lock.sem;

	if (hash == NULL || transaction_hash == NULL)
		return FSSH_B_NO_MEMORY;

	return FSSH_B_OK;
}


void
block_cache::Free(void *address)
{
	if (address == NULL)
		return;

	free(address);
}


void *
block_cache::Allocate()
{
	return malloc(block_size);
}


void
block_cache::FreeBlock(cached_block *block)
{
	Free(block->current_data);

	if (block->original_data != NULL || block->parent_data != NULL) {
		fssh_panic("block_cache::FreeBlock(): %Ld, original %p, parent %p\n",
			block->block_number, block->original_data, block->parent_data);
	}

#ifdef DEBUG_CHANGED
	Free(block->compare);
#endif

	delete block;
}


/*! Allocates a new block for \a blockNumber, ready for use */
cached_block *
block_cache::NewBlock(fssh_off_t blockNumber)
{
	cached_block *block = new(nothrow) cached_block;
	if (block == NULL) {
		FATAL(("could not allocate block!\n"));
		return NULL;
	}

	// if we hit the limit of blocks to cache¸ try to free one or more
	if (allocated_block_count >= kMaxBlockCount) {
		RemoveUnusedBlocks(LONG_MAX,
			allocated_block_count - kMaxBlockCount + 1);
	}

	block->current_data = Allocate();
	if (!block->current_data) {
		FATAL(("could not allocate block data!\n"));
		delete block;
		return NULL;
	}

	block->block_number = blockNumber;
	block->ref_count = 0;
	block->accessed = 0;
	block->transaction_next = NULL;
	block->transaction = block->previous_transaction = NULL;
	block->original_data = NULL;
	block->parent_data = NULL;
	block->is_dirty = false;
	block->unused = false;
#ifdef DEBUG_CHANGED
	block->compare = NULL;
#endif

	allocated_block_count++;

	return block;
}


void
block_cache::RemoveUnusedBlocks(int32_t maxAccessed, int32_t count)
{
	TRACE(("block_cache: remove up to %ld unused blocks\n", count));

	for (block_list::Iterator it = unused_blocks.GetIterator();
		 cached_block *block = it.Next();) {

		if (maxAccessed < block->accessed)
			continue;

		TRACE(("  remove block %Ld, accessed %ld times\n",
			block->block_number, block->accessed));

		// this can only happen if no transactions are used
		if (block->is_dirty)
			write_cached_block(this, block, false);

		// remove block from lists
		it.Remove();
		hash_remove(hash, block);

		FreeBlock(block);

		if (--count <= 0)
			break;
	}
}


//	#pragma mark - private block functions


static void
put_cached_block(block_cache *cache, cached_block *block)
{
#ifdef DEBUG_CHANGED
	if (!block->is_dirty && block->compare != NULL
		&& memcmp(block->current_data, block->compare, cache->block_size)) {
		fssh_dprintf("new block:\n");
		fssh_dump_block((const char *)block->current_data, 256, "  ");
		fssh_dprintf("unchanged block:\n");
		fssh_dump_block((const char *)block->compare, 256, "  ");
		write_cached_block(cache, block);
		fssh_panic("block_cache: supposed to be clean block was changed!\n");

		cache->Free(block->compare);
		block->compare = NULL;
	}
#endif

	if (--block->ref_count == 0
		&& block->transaction == NULL
		&& block->previous_transaction == NULL) {
		// put this block in the list of unused blocks
		block->unused = true;
		cache->unused_blocks.Add(block);
	}

	if (cache->allocated_block_count > kMaxBlockCount) {
		cache->RemoveUnusedBlocks(LONG_MAX,
			cache->allocated_block_count - kMaxBlockCount);
	}
}


static void
put_cached_block(block_cache *cache, fssh_off_t blockNumber)
{
	if (blockNumber < 0 || blockNumber >= cache->max_blocks) {
		fssh_panic("put_cached_block: invalid block number %lld (max %lld)",
			blockNumber, cache->max_blocks - 1);
	}

	cached_block *block = (cached_block *)hash_lookup(cache->hash, &blockNumber);
	if (block != NULL)
		put_cached_block(cache, block);
}


/*!
	Retrieves the block \a blockNumber from the hash table, if it's already
	there, or reads it from the disk.

	\param _allocated tells you wether or not a new block has been allocated
		to satisfy your request.
	\param readBlock if \c false, the block will not be read in case it was
		not already in the cache. The block you retrieve may contain random
		data.
*/
static cached_block *
get_cached_block(block_cache *cache, fssh_off_t blockNumber, bool *_allocated,
	bool readBlock = true)
{
	if (blockNumber < 0 || blockNumber >= cache->max_blocks) {
		fssh_panic("get_cached_block: invalid block number %lld (max %lld)",
			blockNumber, cache->max_blocks - 1);
		return NULL;
	}

	cached_block *block = (cached_block *)hash_lookup(cache->hash,
		&blockNumber);
	*_allocated = false;

	if (block == NULL) {
		// read block into cache
		block = cache->NewBlock(blockNumber);
		if (block == NULL)
			return NULL;

		hash_insert(cache->hash, block);
		*_allocated = true;
	}

	if (*_allocated && readBlock) {
		int32_t blockSize = cache->block_size;

		if (fssh_read_pos(cache->fd, blockNumber * blockSize, block->current_data,
				blockSize) < blockSize) {
			hash_remove(cache->hash, block);
			cache->FreeBlock(block);
			FATAL(("could not read block %Ld\n", blockNumber));
			return NULL;
		}
	}

	if (block->unused) {
		//TRACE(("remove block %Ld from unused\n", blockNumber));
		block->unused = false;
		cache->unused_blocks.Remove(block);
	}

	block->ref_count++;
	block->accessed++;

	return block;
}


/*!
	Returns the writable block data for the requested blockNumber.
	If \a cleared is true, the block is not read from disk; an empty block
	is returned.

	This is the only method to insert a block into a transaction. It makes
	sure that the previous block contents are preserved in that case.
*/
static void *
get_writable_cached_block(block_cache *cache, fssh_off_t blockNumber, fssh_off_t base,
	fssh_off_t length, int32_t transactionID, bool cleared)
{
	TRACE(("get_writable_cached_block(blockNumber = %Ld, transaction = %d)\n",
		blockNumber, transactionID));

	if (blockNumber < 0 || blockNumber >= cache->max_blocks) {
		fssh_panic("get_writable_cached_block: invalid block number %lld (max %lld)",
			blockNumber, cache->max_blocks - 1);
	}

	bool allocated;
	cached_block *block = get_cached_block(cache, blockNumber, &allocated,
		!cleared);
	if (block == NULL)
		return NULL;

	// if there is no transaction support, we just return the current block
	if (transactionID == -1) {
		if (cleared)
			fssh_memset(block->current_data, 0, cache->block_size);

		block->is_dirty = true;
			// mark the block as dirty

		return block->current_data;
	}

	cache_transaction *transaction = block->transaction;

	if (transaction != NULL && transaction->id != transactionID) {
		// ToDo: we have to wait here until the other transaction is done.
		//	Maybe we should even panic, since we can't prevent any deadlocks.
		fssh_panic("get_writable_cached_block(): asked to get busy writable block (transaction %d)\n", (int)transaction->id);
		put_cached_block(cache, block);
		return NULL;
	}
	if (transaction == NULL && transactionID != -1) {
		// get new transaction
		transaction = lookup_transaction(cache, transactionID);
		if (transaction == NULL) {
			fssh_panic("get_writable_cached_block(): invalid transaction %d!\n",
				(int)transactionID);
			put_cached_block(cache, block);
			return NULL;
		}
		if (!transaction->open) {
			fssh_panic("get_writable_cached_block(): transaction already done!\n");
			put_cached_block(cache, block);
			return NULL;
		}

		block->transaction = transaction;

		// attach the block to the transaction block list
		block->transaction_next = transaction->first_block;
		transaction->first_block = block;
		transaction->num_blocks++;
	}

	bool wasUnchanged = block->original_data == NULL
		|| block->previous_transaction != NULL;

	if (!(allocated && cleared) && block->original_data == NULL) {
		// we already have data, so we need to preserve it
		block->original_data = cache->Allocate();
		if (block->original_data == NULL) {
			FATAL(("could not allocate original_data\n"));
			put_cached_block(cache, block);
			return NULL;
		}

		fssh_memcpy(block->original_data, block->current_data, cache->block_size);
	}
	if (block->parent_data == block->current_data) {
		// remember any previous contents for the parent transaction
		block->parent_data = cache->Allocate();
		if (block->parent_data == NULL) {
			// TODO: maybe we should just continue the current transaction in this case...
			FATAL(("could not allocate parent\n"));
			put_cached_block(cache, block);
			return NULL;
		}

		fssh_memcpy(block->parent_data, block->current_data, cache->block_size);
		transaction->sub_num_blocks++;
	} else if (transaction != NULL && transaction->has_sub_transaction
		&& block->parent_data == NULL && wasUnchanged)
		transaction->sub_num_blocks++;

	if (cleared)
		fssh_memset(block->current_data, 0, cache->block_size);

	block->is_dirty = true;

	return block->current_data;
}


static fssh_status_t
write_cached_block(block_cache *cache, cached_block *block,
	bool deleteTransaction)
{
	cache_transaction *previous = block->previous_transaction;
	int32_t blockSize = cache->block_size;

	void *data = previous && block->original_data
		? block->original_data : block->current_data;
		// we first need to write back changes from previous transactions

	TRACE(("write_cached_block(block %Ld)\n", block->block_number));

	fssh_ssize_t written = fssh_write_pos(cache->fd, block->block_number * blockSize,
		data, blockSize);

	if (written < blockSize) {
		FATAL(("could not write back block %Ld (%s)\n", block->block_number,
			fssh_strerror(fssh_get_errno())));
		return FSSH_B_IO_ERROR;
	}

	if (data == block->current_data)
		block->is_dirty = false;

	if (previous != NULL) {
		previous->blocks.Remove(block);
		block->previous_transaction = NULL;

		if (block->original_data != NULL && block->transaction == NULL) {
			// This block is not part of a transaction, so it does not need
			// its original pointer anymore.
			cache->Free(block->original_data);
			block->original_data = NULL;
		}

		// Has the previous transation been finished with that write?
		if (--previous->num_blocks == 0) {
			TRACE(("cache transaction %ld finished!\n", previous->id));

			if (previous->notification_hook != NULL) {
				previous->notification_hook(previous->id,
					FSSH_TRANSACTION_WRITTEN, previous->notification_data);
			}

			if (deleteTransaction) {
				hash_remove(cache->transaction_hash, previous);
				delete_transaction(cache, previous);
			}
		}
	}

	return FSSH_B_OK;
}


fssh_status_t
block_cache_init()
{
	return FSSH_B_OK;
}


}	// namespace FSShell


//	#pragma mark - public transaction API


using namespace FSShell;


int32_t
fssh_cache_start_transaction(void *_cache)
{
	block_cache *cache = (block_cache *)_cache;
	BenaphoreLocker locker(&cache->lock);

	if (cache->last_transaction && cache->last_transaction->open) {
		fssh_panic("last transaction (%d) still open!\n",
			(int)cache->last_transaction->id);
	}

	cache_transaction *transaction = new(nothrow) cache_transaction;
	if (transaction == NULL)
		return FSSH_B_NO_MEMORY;

	transaction->id = fssh_atomic_add(&cache->next_transaction_id, 1);
	cache->last_transaction = transaction;

	TRACE(("cache_start_transaction(): id %d started\n", transaction->id));

	hash_insert(cache->transaction_hash, transaction);

	return transaction->id;
}


fssh_status_t
fssh_cache_sync_transaction(void *_cache, int32_t id)
{
	block_cache *cache = (block_cache *)_cache;
	BenaphoreLocker locker(&cache->lock);
	fssh_status_t status = FSSH_B_ENTRY_NOT_FOUND;

	TRACE(("cache_sync_transaction(id %d)\n", id));

	hash_iterator iterator;
	hash_open(cache->transaction_hash, &iterator);

	cache_transaction *transaction;
	while ((transaction = (cache_transaction *)hash_next(
			cache->transaction_hash, &iterator)) != NULL) {
		// close all earlier transactions which haven't been closed yet

		if (transaction->id <= id && !transaction->open) {
			// write back all of their remaining dirty blocks
			while (transaction->num_blocks > 0) {
				status = write_cached_block(cache, transaction->blocks.Head(),
					false);
				if (status != FSSH_B_OK)
					return status;
			}

			hash_remove_current(cache->transaction_hash, &iterator);
			delete_transaction(cache, transaction);
		}
	}

	hash_close(cache->transaction_hash, &iterator, false);
	return FSSH_B_OK;
}


fssh_status_t
fssh_cache_end_transaction(void *_cache, int32_t id,
	fssh_transaction_notification_hook hook, void *data)
{
	block_cache *cache = (block_cache *)_cache;
	BenaphoreLocker locker(&cache->lock);

	TRACE(("cache_end_transaction(id = %d)\n", id));

	cache_transaction *transaction = lookup_transaction(cache, id);
	if (transaction == NULL) {
		fssh_panic("cache_end_transaction(): invalid transaction ID\n");
		return FSSH_B_BAD_VALUE;
	}

	transaction->notification_hook = hook;
	transaction->notification_data = data;

	notify_transaction_listeners(transaction, FSSH_TRANSACTION_ENDED);

	// iterate through all blocks and free the unchanged original contents

	cached_block *block = transaction->first_block, *next;
	for (; block != NULL; block = next) {
		next = block->transaction_next;

		if (block->previous_transaction != NULL) {
			// need to write back pending changes
			write_cached_block(cache, block);
		}

		if (block->original_data != NULL) {
			cache->Free(block->original_data);
			block->original_data = NULL;
		}
		if (transaction->has_sub_transaction) {
			if (block->parent_data != block->current_data)
				cache->Free(block->parent_data);
			block->parent_data = NULL;
		}

		// move the block to the previous transaction list
		transaction->blocks.Add(block);

		block->previous_transaction = transaction;
		block->transaction_next = NULL;
		block->transaction = NULL;
	}

	transaction->open = false;

	return FSSH_B_OK;
}


fssh_status_t
fssh_cache_abort_transaction(void *_cache, int32_t id)
{
	block_cache *cache = (block_cache *)_cache;
	BenaphoreLocker locker(&cache->lock);

	TRACE(("cache_abort_transaction(id = %ld)\n", id));

	cache_transaction *transaction = lookup_transaction(cache, id);
	if (transaction == NULL) {
		fssh_panic("cache_abort_transaction(): invalid transaction ID\n");
		return FSSH_B_BAD_VALUE;
	}

	notify_transaction_listeners(transaction, FSSH_TRANSACTION_ABORTED);

	// iterate through all blocks and restore their original contents

	cached_block *block = transaction->first_block, *next;
	for (; block != NULL; block = next) {
		next = block->transaction_next;

		if (block->original_data != NULL) {
			TRACE(("cache_abort_transaction(id = %ld): restored contents of block %Ld\n",
				transaction->id, block->block_number));
			fssh_memcpy(block->current_data, block->original_data, cache->block_size);
			cache->Free(block->original_data);
			block->original_data = NULL;
		}
		if (transaction->has_sub_transaction) {
			if (block->parent_data != block->current_data)
				cache->Free(block->parent_data);
			block->parent_data = NULL;
		}

		block->transaction_next = NULL;
		block->transaction = NULL;
	}

	hash_remove(cache->transaction_hash, transaction);
	delete_transaction(cache, transaction);
	return FSSH_B_OK;
}


/*!	Acknowledges the current parent transaction, and starts a new transaction
	from its sub transaction.
	The new transaction also gets a new transaction ID.
*/
int32_t
fssh_cache_detach_sub_transaction(void *_cache, int32_t id,
	fssh_transaction_notification_hook hook, void *data)
{
	block_cache *cache = (block_cache *)_cache;
	BenaphoreLocker locker(&cache->lock);

	TRACE(("cache_detach_sub_transaction(id = %d)\n", id));

	cache_transaction *transaction = lookup_transaction(cache, id);
	if (transaction == NULL) {
		fssh_panic("cache_detach_sub_transaction(): invalid transaction ID\n");
		return FSSH_B_BAD_VALUE;
	}
	if (!transaction->has_sub_transaction)
		return FSSH_B_BAD_VALUE;

	// create a new transaction for the sub transaction
	cache_transaction *newTransaction = new(nothrow) cache_transaction;
	if (transaction == NULL)
		return FSSH_B_NO_MEMORY;

	newTransaction->id = fssh_atomic_add(&cache->next_transaction_id, 1);

	transaction->notification_hook = hook;
	transaction->notification_data = data;

	notify_transaction_listeners(transaction, FSSH_TRANSACTION_ENDED);

	// iterate through all blocks and free the unchanged original contents

	cached_block *block = transaction->first_block, *next, *last = NULL;
	for (; block != NULL; block = next) {
		next = block->transaction_next;

		if (block->previous_transaction != NULL) {
			// need to write back pending changes
			write_cached_block(cache, block);
		}

		if (block->original_data != NULL && block->parent_data != NULL
			&& block->parent_data != block->current_data) {
			// free the original data if the parent data of the transaction
			// will be made current - but keep them otherwise
			cache->Free(block->original_data);
			block->original_data = NULL;
		}
		if (block->parent_data == NULL
			|| block->parent_data != block->current_data) {
			// we need to move this block over to the new transaction
			block->original_data = block->parent_data;
			if (last == NULL)
				newTransaction->first_block = block;
			else
				last->transaction_next = block;

			block->transaction = newTransaction;
			last = block;
		} else
			block->transaction = NULL;

		if (block->parent_data != NULL) {
			// move the block to the previous transaction list
			transaction->blocks.Add(block);
			block->parent_data = NULL;
		}

		block->previous_transaction = transaction;
		block->transaction_next = NULL;
	}

	newTransaction->num_blocks = transaction->sub_num_blocks;

	transaction->open = false;
	transaction->has_sub_transaction = false;
	transaction->num_blocks = transaction->main_num_blocks;
	transaction->sub_num_blocks = 0;

	hash_insert(cache->transaction_hash, newTransaction);
	cache->last_transaction = newTransaction;

	return newTransaction->id;
}


fssh_status_t
fssh_cache_abort_sub_transaction(void *_cache, int32_t id)
{
	block_cache *cache = (block_cache *)_cache;
	BenaphoreLocker locker(&cache->lock);

	TRACE(("cache_abort_sub_transaction(id = %ld)\n", id));

	cache_transaction *transaction = lookup_transaction(cache, id);
	if (transaction == NULL) {
		fssh_panic("cache_abort_sub_transaction(): invalid transaction ID\n");
		return FSSH_B_BAD_VALUE;
	}
	if (!transaction->has_sub_transaction)
		return FSSH_B_BAD_VALUE;

	notify_transaction_listeners(transaction, FSSH_TRANSACTION_ABORTED);

	// revert all changes back to the version of the parent

	cached_block *block = transaction->first_block, *next;
	for (; block != NULL; block = next) {
		next = block->transaction_next;

		if (block->parent_data == NULL) {
			if (block->original_data != NULL) {
				// the parent transaction didn't change the block, but the sub
				// transaction did - we need to revert from the original data
				fssh_memcpy(block->current_data, block->original_data,
					cache->block_size);
			}
		} else if (block->parent_data != block->current_data) {
			// the block has been changed and must be restored
			TRACE(("cache_abort_sub_transaction(id = %ld): restored contents of block %Ld\n",
				transaction->id, block->block_number));
			fssh_memcpy(block->current_data, block->parent_data,
				cache->block_size);
			cache->Free(block->parent_data);
		}

		block->parent_data = NULL;
	}

	// all subsequent changes will go into the main transaction
	transaction->has_sub_transaction = false;
	transaction->sub_num_blocks = 0;

	return FSSH_B_OK;
}


fssh_status_t
fssh_cache_start_sub_transaction(void *_cache, int32_t id)
{
	block_cache *cache = (block_cache *)_cache;
	BenaphoreLocker locker(&cache->lock);

	TRACE(("cache_start_sub_transaction(id = %d)\n", id));

	cache_transaction *transaction = lookup_transaction(cache, id);
	if (transaction == NULL) {
		fssh_panic("cache_start_sub_transaction(): invalid transaction ID %d\n", (int)id);
		return FSSH_B_BAD_VALUE;
	}

	notify_transaction_listeners(transaction, FSSH_TRANSACTION_ENDED);

	// move all changed blocks up to the parent

	cached_block *block = transaction->first_block, *next;
	for (; block != NULL; block = next) {
		next = block->transaction_next;

		if (transaction->has_sub_transaction
			&& block->parent_data != NULL
			&& block->parent_data != block->current_data) {
			// there already is an older sub transaction - we acknowledge
			// its changes and move its blocks up to the parent
			cache->Free(block->parent_data);
		}

		// we "allocate" the parent data lazily, that means, we don't copy
		// the data (and allocate memory for it) until we need to
		block->parent_data = block->current_data;
	}

	// all subsequent changes will go into the sub transaction
	transaction->has_sub_transaction = true;
	transaction->main_num_blocks = transaction->num_blocks;
	transaction->sub_num_blocks = 0;

	return FSSH_B_OK;
}


/*!	Adds a transaction listener that gets notified when the transaction
	is ended or aborted.
	The listener gets automatically removed in this case.
*/
fssh_status_t
fssh_cache_add_transaction_listener(void *_cache, int32_t id, int32_t events,
	fssh_transaction_notification_hook hookFunction, void *data)
{
// TODO: this is currently not used in a critical context in BFS
#if 0
	block_cache *cache = (block_cache *)_cache;

	cache_hook *hook = new(std::nothrow) cache_hook;
	if (hook == NULL)
		return FSSH_B_NO_MEMORY;

	BenaphoreLocker locker(&cache->lock);

	cache_transaction *transaction = lookup_transaction(cache, id);
	if (transaction == NULL) {
		delete hook;
		return FSSH_B_BAD_VALUE;
	}

	hook->hook = hookFunction;
	hook->data = data;

	transaction->listeners.Add(hook);
#endif
	return FSSH_B_OK;
}


fssh_status_t
fssh_cache_remove_transaction_listener(void *_cache, int32_t id,
	fssh_transaction_notification_hook hookFunction, void *data)
{
	block_cache *cache = (block_cache *)_cache;

	BenaphoreLocker locker(&cache->lock);

	cache_transaction *transaction = lookup_transaction(cache, id);
	if (transaction == NULL)
		return FSSH_B_BAD_VALUE;

	HookList::Iterator iterator = transaction->listeners.GetIterator();
	while (iterator.HasNext()) {
		cache_hook *hook = iterator.Next();
		if (hook->data == data && hook->hook == hookFunction) {
			iterator.Remove();
			delete hook;
			return FSSH_B_OK;
		}
	}

	return FSSH_B_ENTRY_NOT_FOUND;
}


fssh_status_t
fssh_cache_next_block_in_transaction(void *_cache, int32_t id, bool mainOnly,
	long *_cookie, fssh_off_t *_blockNumber, void **_data,
	void **_unchangedData)
{
	cached_block *block = (cached_block *)*_cookie;
	block_cache *cache = (block_cache *)_cache;

	BenaphoreLocker locker(&cache->lock);

	cache_transaction *transaction = lookup_transaction(cache, id);
	if (transaction == NULL || !transaction->open)
		return FSSH_B_BAD_VALUE;

	if (block == NULL)
		block = transaction->first_block;
	else
		block = block->transaction_next;

	if (mainOnly && transaction->has_sub_transaction) {
		// find next block that the parent changed
		while (block != NULL && block->parent_data == NULL)
			block = block->transaction_next;
	}

	if (block == NULL)
		return FSSH_B_ENTRY_NOT_FOUND;

	if (_blockNumber)
		*_blockNumber = block->block_number;
	if (_data)
		*_data = mainOnly ? block->parent_data : block->current_data;
	if (_unchangedData)
		*_unchangedData = block->original_data;

	*_cookie = (fssh_addr_t)block;
	return FSSH_B_OK;
}


int32_t
fssh_cache_blocks_in_transaction(void *_cache, int32_t id)
{
	block_cache *cache = (block_cache *)_cache;
	BenaphoreLocker locker(&cache->lock);

	cache_transaction *transaction = lookup_transaction(cache, id);
	if (transaction == NULL)
		return FSSH_B_BAD_VALUE;

	return transaction->num_blocks;
}


int32_t
fssh_cache_blocks_in_main_transaction(void *_cache, int32_t id)
{
	block_cache *cache = (block_cache *)_cache;
	BenaphoreLocker locker(&cache->lock);

	cache_transaction *transaction = lookup_transaction(cache, id);
	if (transaction == NULL)
		return FSSH_B_BAD_VALUE;

	return transaction->main_num_blocks;
}


int32_t
fssh_cache_blocks_in_sub_transaction(void *_cache, int32_t id)
{
	block_cache *cache = (block_cache *)_cache;
	BenaphoreLocker locker(&cache->lock);

	cache_transaction *transaction = lookup_transaction(cache, id);
	if (transaction == NULL)
		return FSSH_B_BAD_VALUE;

	return transaction->sub_num_blocks;
}


//	#pragma mark - public block cache API
//	public interface


void
fssh_block_cache_delete(void *_cache, bool allowWrites)
{
	block_cache *cache = (block_cache *)_cache;

	if (allowWrites)
		fssh_block_cache_sync(cache);

	BenaphoreLocker locker(&cache->lock);

	// free all blocks

	uint32_t cookie = 0;
	cached_block *block;
	while ((block = (cached_block *)hash_remove_first(cache->hash,
			&cookie)) != NULL) {
		cache->FreeBlock(block);
	}

	// free all transactions (they will all be aborted)

	cookie = 0;
	cache_transaction *transaction;
	while ((transaction = (cache_transaction *)hash_remove_first(
			cache->transaction_hash, &cookie)) != NULL) {
		delete transaction;
	}

	delete cache;
}


void *
fssh_block_cache_create(int fd, fssh_off_t numBlocks, fssh_size_t blockSize, bool readOnly)
{
	block_cache *cache = new(nothrow) block_cache(fd, numBlocks, blockSize,
		readOnly);
	if (cache == NULL)
		return NULL;

	if (cache->InitCheck() != FSSH_B_OK) {
		delete cache;
		return NULL;
	}

	return cache;
}


fssh_status_t
fssh_block_cache_sync(void *_cache)
{
	block_cache *cache = (block_cache *)_cache;

	// we will sync all dirty blocks to disk that have a completed
	// transaction or no transaction only

	BenaphoreLocker locker(&cache->lock);
	hash_iterator iterator;
	hash_open(cache->hash, &iterator);

	cached_block *block;
	while ((block = (cached_block *)hash_next(cache->hash, &iterator)) != NULL) {
		if (block->previous_transaction != NULL
			|| (block->transaction == NULL && block->is_dirty)) {
			fssh_status_t status = write_cached_block(cache, block);
			if (status != FSSH_B_OK)
				return status;
		}
	}

	hash_close(cache->hash, &iterator, false);
	return FSSH_B_OK;
}


fssh_status_t
fssh_block_cache_sync_etc(void *_cache, fssh_off_t blockNumber,
	fssh_size_t numBlocks)
{
	block_cache *cache = (block_cache *)_cache;

	// we will sync all dirty blocks to disk that have a completed
	// transaction or no transaction only

	if (blockNumber < 0 || blockNumber >= cache->max_blocks) {
		fssh_panic("block_cache_sync_etc: invalid block number %Ld (max %Ld)",
			blockNumber, cache->max_blocks - 1);
		return FSSH_B_BAD_VALUE;
	}

	BenaphoreLocker locker(&cache->lock);

	for (; numBlocks > 0; numBlocks--, blockNumber++) {
		cached_block *block = (cached_block *)hash_lookup(cache->hash,
			&blockNumber);
		if (block == NULL)
			continue;
		if (block->previous_transaction != NULL
			|| (block->transaction == NULL && block->is_dirty)) {
			fssh_status_t status = write_cached_block(cache, block);
			if (status != FSSH_B_OK)
				return status;
		}
	}

	return FSSH_B_OK;
}


fssh_status_t
fssh_block_cache_make_writable(void *_cache, fssh_off_t blockNumber, int32_t transaction)
{
	block_cache *cache = (block_cache *)_cache;
	BenaphoreLocker locker(&cache->lock);

	if (cache->read_only)
		fssh_panic("tried to make block writable on a read-only cache!");

	// ToDo: this can be done better!
	void *block = get_writable_cached_block(cache, blockNumber,
		blockNumber, 1, transaction, false);
	if (block != NULL) {
		put_cached_block((block_cache *)_cache, blockNumber);
		return FSSH_B_OK;
	}

	return FSSH_B_ERROR;
}


void *
fssh_block_cache_get_writable_etc(void *_cache, fssh_off_t blockNumber, fssh_off_t base,
	fssh_off_t length, int32_t transaction)
{
	block_cache *cache = (block_cache *)_cache;
	BenaphoreLocker locker(&cache->lock);

	TRACE(("block_cache_get_writable_etc(block = %Ld, transaction = %ld)\n",
		blockNumber, transaction));
	if (cache->read_only)
		fssh_panic("tried to get writable block on a read-only cache!");

	return get_writable_cached_block(cache, blockNumber, base, length,
		transaction, false);
}


void *
fssh_block_cache_get_writable(void *_cache, fssh_off_t blockNumber,
	int32_t transaction)
{
	return fssh_block_cache_get_writable_etc(_cache, blockNumber,
		blockNumber, 1, transaction);
}


void *
fssh_block_cache_get_empty(void *_cache, fssh_off_t blockNumber,
	int32_t transaction)
{
	block_cache *cache = (block_cache *)_cache;
	BenaphoreLocker locker(&cache->lock);

	TRACE(("block_cache_get_empty(block = %Ld, transaction = %ld)\n",
		blockNumber, transaction));
	if (cache->read_only)
		fssh_panic("tried to get empty writable block on a read-only cache!");

	return get_writable_cached_block((block_cache *)_cache, blockNumber,
		blockNumber, 1, transaction, true);
}


const void *
fssh_block_cache_get_etc(void *_cache, fssh_off_t blockNumber, fssh_off_t base,
	fssh_off_t length)
{
	block_cache *cache = (block_cache *)_cache;
	BenaphoreLocker locker(&cache->lock);
	bool allocated;

	cached_block *block = get_cached_block(cache, blockNumber, &allocated);
	if (block == NULL)
		return NULL;

#ifdef DEBUG_CHANGED
	if (block->compare == NULL)
		block->compare = cache->Allocate();
	if (block->compare != NULL)
		memcpy(block->compare, block->current_data, cache->block_size);
#endif
	return block->current_data;
}


const void *
fssh_block_cache_get(void *_cache, fssh_off_t blockNumber)
{
	return fssh_block_cache_get_etc(_cache, blockNumber, blockNumber, 1);
}


/*!
	Changes the internal status of a writable block to \a dirty. This can be
	helpful in case you realize you don't need to change that block anymore
	for whatever reason.

	Note, you must only use this function on blocks that were acquired
	writable!
*/
fssh_status_t
fssh_block_cache_set_dirty(void *_cache, fssh_off_t blockNumber, bool dirty,
	int32_t transaction)
{
	// TODO: not yet implemented
	if (dirty)
		fssh_panic("block_cache_set_dirty(): not yet implemented that way!\n");

	return FSSH_B_OK;
}


void
fssh_block_cache_put(void *_cache, fssh_off_t blockNumber)
{
	block_cache *cache = (block_cache *)_cache;
	BenaphoreLocker locker(&cache->lock);

	put_cached_block(cache, blockNumber);
}

