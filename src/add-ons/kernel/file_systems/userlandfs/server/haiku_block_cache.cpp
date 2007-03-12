/*
 * Copyright 2004-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "haiku_block_cache_priv.h"

#include <new>

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "haiku_fs_cache.h"
#include "haiku_hash.h"
#include "kernel_emu.h"

// ToDo: this is a naive but growing implementation to test the API:
//	1) block reading/writing is not at all optimized for speed, it will
//	   just read and write single blocks.
//	2) the locking could be improved; getting a block should not need to
//	   wait for blocks to be written
//	3) dirty blocks are only written back if asked for
// TODO: the retrieval/copy of the original data could be delayed until the
//		new data must be written, ie. in low memory situations.

//#define TRACE_BLOCK_CACHE
#ifdef TRACE_BLOCK_CACHE
#	define TRACE(x)	dprintf x
#else
#	define TRACE(x) ;
#endif

// This macro is used for fatal situations that are acceptable in a running
// system, like out of memory situations - should only panic for debugging.
#define FATAL(x) panic x

using UserlandFS::KernelEmu::dprintf;
using UserlandFS::KernelEmu::dump_block;
using UserlandFS::KernelEmu::panic;

namespace UserlandFS {
namespace HaikuKernelEmu {

static const int32 kMaxBlockCount = 1024;


struct cache_transaction {
	cache_transaction();

	cache_transaction *next;
	int32			id;
	int32			num_blocks;
	int32			sub_num_blocks;
	cached_block	*first_block;
	block_list		blocks;
	transaction_notification_hook notification_hook;
	void			*notification_data;
	bool			open;
	bool			has_sub_transaction;
};

static status_t write_cached_block(block_cache *cache, cached_block *block,
	bool deleteTransaction = true);


//	#pragma mark - private transaction


cache_transaction::cache_transaction()
{
	num_blocks = 0;
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
	const int32 *id = (const int32 *)_id;

	return transaction->id - *id;
}


static uint32
transaction_hash(void *_transaction, const void *_id, uint32 range)
{
	cache_transaction *transaction = (cache_transaction *)_transaction;
	const int32 *id = (const int32 *)_id;

	if (transaction != NULL)
		return transaction->id % range;

	return (uint32)*id % range;
}


static void
delete_transaction(block_cache *cache, cache_transaction *transaction)
{
	if (cache->last_transaction == transaction)
		cache->last_transaction = NULL;

	delete transaction;
}


static cache_transaction *
lookup_transaction(block_cache *cache, int32 id)
{
	return (cache_transaction *)hash_lookup(cache->transaction_hash, &id);
}


//	#pragma mark - cached_block


/* static */
int
cached_block::Compare(void *_cacheEntry, const void *_block)
{
	cached_block *cacheEntry = (cached_block *)_cacheEntry;
	const off_t *block = (const off_t *)_block;

	return cacheEntry->block_number - *block;
}



/* static */
uint32
cached_block::Hash(void *_cacheEntry, const void *_block, uint32 range)
{
	cached_block *cacheEntry = (cached_block *)_cacheEntry;
	const off_t *block = (const off_t *)_block;

	if (cacheEntry != NULL)
		return cacheEntry->block_number % range;

	return (uint64)*block % range;
}


//	#pragma mark - block_cache


block_cache::block_cache(int _fd, off_t numBlocks, size_t blockSize,
	bool readOnly)
	:
	hash(NULL),
	fd(_fd),
	max_blocks(numBlocks),
	block_size(blockSize),
	next_transaction_id(1),
	last_transaction(NULL),
	transaction_hash(NULL),
	read_only(readOnly)
{
	hash = hash_init(32, 0, &cached_block::Compare, &cached_block::Hash);
	if (hash == NULL)
		return;

	transaction_hash = hash_init(16, 0, &transaction_compare,
		&UserlandFS::HaikuKernelEmu::transaction_hash);
	if (transaction_hash == NULL)
		return;

	if (benaphore_init(&lock, "block cache") < B_OK)
		return;
}


block_cache::~block_cache()
{
	benaphore_destroy(&lock);

	hash_uninit(transaction_hash);
	hash_uninit(hash);
}


status_t
block_cache::InitCheck()
{
	if (lock.sem < B_OK)
		return lock.sem;

	if (hash == NULL || transaction_hash == NULL)
		return B_NO_MEMORY;

	return B_OK;
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
	block->current_data = NULL;

	if (block->original_data != NULL || block->parent_data != NULL) {
		panic("block_cache::FreeBlock(): %p, %p\n", block->original_data,
			block->parent_data);
	}

#ifdef DEBUG_CHANGED
	Free(block->compare);
#endif

	delete block;
}


/*! Allocates a new block for \a blockNumber, ready for use */
cached_block *
block_cache::NewBlock(off_t blockNumber)
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
block_cache::RemoveUnusedBlocks(int32 maxAccessed, int32 count)
{
	TRACE(("block_cache: remove up to %ld unused blocks\n", count));

	cached_block *next = NULL;
	for (cached_block *block = unused_blocks.First(); block != NULL;
			block = next) {
		next = block->next;

		if (maxAccessed < block->accessed)
			continue;

		TRACE(("  remove block %Ld, accessed %ld times\n",
			block->block_number, block->accessed));

		// this can only happen if no transactions are used
		if (block->is_dirty)
			write_cached_block(this, block, false);

		// remove block from lists
		unused_blocks.Remove(block);
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
		dprintf("new block:\n");
		dump_block((const char *)block->current_data, 256, "  ");
		dprintf("unchanged block:\n");
		dump_block((const char *)block->compare, 256, "  ");
		write_cached_block(cache, block);
		panic("block_cache: supposed to be clean block was changed!\n");

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
put_cached_block(block_cache *cache, off_t blockNumber)
{
	if (blockNumber < 0 || blockNumber >= cache->max_blocks) {
		panic("put_cached_block: invalid block number %lld (max %lld)",
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
get_cached_block(block_cache *cache, off_t blockNumber, bool *_allocated,
	bool readBlock = true)
{
	if (blockNumber < 0 || blockNumber >= cache->max_blocks) {
		panic("get_cached_block: invalid block number %lld (max %lld)",
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
	} else {
		// TODO: currently, the data is always mapped in
/*
		if (block->ref_count == 0 && block->current_data != NULL) {
			// see if the old block can be resurrected
			block->current_data = cache->allocator->Acquire(block->current_data);
		}

		if (block->current_data == NULL) {
			// there is no block yet, but we need one
			block->current_data = cache->allocator->Get();
			if (block->current_data == NULL)
				return NULL;

			*_allocated = true;
		}
*/
	}

	if (*_allocated && readBlock) {
		int32 blockSize = cache->block_size;

		if (read_pos(cache->fd, blockNumber * blockSize, block->current_data,
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
get_writable_cached_block(block_cache *cache, off_t blockNumber, off_t base,
	off_t length, int32 transactionID, bool cleared)
{
	TRACE(("get_writable_cached_block(blockNumber = %Ld, transaction = %ld)\n",
		blockNumber, transactionID));

	if (blockNumber < 0 || blockNumber >= cache->max_blocks) {
		panic("get_writable_cached_block: invalid block number %lld (max %lld)",
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
			memset(block->current_data, 0, cache->block_size);

		block->is_dirty = true;
			// mark the block as dirty

		return block->current_data;
	}

	if (block->transaction != NULL && block->transaction->id != transactionID) {
		// ToDo: we have to wait here until the other transaction is done.
		//	Maybe we should even panic, since we can't prevent any deadlocks.
		panic("get_writable_cached_block(): asked to get busy writable block (transaction %ld)\n", block->transaction->id);
		put_cached_block(cache, block);
		return NULL;
	}
	if (block->transaction == NULL && transactionID != -1) {
		// get new transaction
		cache_transaction *transaction = lookup_transaction(cache, transactionID);
		if (transaction == NULL) {
			panic("get_writable_cached_block(): invalid transaction %ld!\n",
				transactionID);
			put_cached_block(cache, block);
			return NULL;
		}
		if (!transaction->open) {
			panic("get_writable_cached_block(): transaction already done!\n");
			put_cached_block(cache, block);
			return NULL;
		}

		block->transaction = transaction;

		// attach the block to the transaction block list
		block->transaction_next = transaction->first_block;
		transaction->first_block = block;
		transaction->num_blocks++;
	}

	if (!(allocated && cleared) && block->original_data == NULL) {
		// we already have data, so we need to preserve it
		block->original_data = cache->Allocate();
		if (block->original_data == NULL) {
			FATAL(("could not allocate original_data\n"));
			put_cached_block(cache, block);
			return NULL;
		}

		memcpy(block->original_data, block->current_data, cache->block_size);
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

		memcpy(block->parent_data, block->current_data, cache->block_size);
		block->transaction->sub_num_blocks++;
	}

	if (cleared)
		memset(block->current_data, 0, cache->block_size);

	block->is_dirty = true;

	return block->current_data;
}


static status_t
write_cached_block(block_cache *cache, cached_block *block,
	bool deleteTransaction)
{
	cache_transaction *previous = block->previous_transaction;
	int32 blockSize = cache->block_size;

	void *data = previous && block->original_data
		? block->original_data : block->current_data;
		// we first need to write back changes from previous transactions

	TRACE(("write_cached_block(block %Ld)\n", block->block_number));

	ssize_t written = write_pos(cache->fd, block->block_number * blockSize,
		data, blockSize);

	if (written < blockSize) {
		FATAL(("could not write back block %Ld (%s)\n", block->block_number,
			strerror(errno)));
		return B_IO_ERROR;
	}

	if (data == block->current_data)
		block->is_dirty = false;

	if (previous != NULL) {
		previous->blocks.Remove(block);
		block->previous_transaction = NULL;

		// Has the previous transation been finished with that write?
		if (--previous->num_blocks == 0) {
			TRACE(("cache transaction %ld finished!\n", previous->id));

			if (previous->notification_hook != NULL) {
				previous->notification_hook(previous->id,
					previous->notification_data);
			}

			if (deleteTransaction) {
				hash_remove(cache->transaction_hash, previous);
				delete_transaction(cache, previous);
			}
		}
	}

	return B_OK;
}


status_t
block_cache_init()
{
	return B_OK;
}


//	#pragma mark - public transaction API


int32
cache_start_transaction(void *_cache)
{
	block_cache *cache = (block_cache *)_cache;
	BenaphoreLocker locker(&cache->lock);

	if (cache->last_transaction && cache->last_transaction->open) {
		panic("last transaction (%ld) still open!\n",
			cache->last_transaction->id);
	}

	cache_transaction *transaction = new(nothrow) cache_transaction;
	if (transaction == NULL)
		return B_NO_MEMORY;

	transaction->id = atomic_add(&cache->next_transaction_id, 1);
	cache->last_transaction = transaction;

	TRACE(("cache_start_transaction(): id %ld started\n", transaction->id));

	hash_insert(cache->transaction_hash, transaction);

	return transaction->id;
}


status_t
cache_sync_transaction(void *_cache, int32 id)
{
	block_cache *cache = (block_cache *)_cache;
	BenaphoreLocker locker(&cache->lock);
	status_t status = B_ENTRY_NOT_FOUND;

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
				if (status != B_OK)
					return status;
			}

			hash_remove_current(cache->transaction_hash, &iterator);
			delete_transaction(cache, transaction);
		}
	}

	hash_close(cache->transaction_hash, &iterator, false);
	return B_OK;
}


status_t
cache_end_transaction(void *_cache, int32 id,
	transaction_notification_hook hook, void *data)
{
	block_cache *cache = (block_cache *)_cache;
	BenaphoreLocker locker(&cache->lock);

	TRACE(("cache_end_transaction(id = %ld)\n", id));

	cache_transaction *transaction = lookup_transaction(cache, id);
	if (transaction == NULL) {
		panic("cache_end_transaction(): invalid transaction ID\n");
		return B_BAD_VALUE;
	}

	transaction->notification_hook = hook;
	transaction->notification_data = data;

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

	return B_OK;
}


status_t
cache_abort_transaction(void *_cache, int32 id)
{
	block_cache *cache = (block_cache *)_cache;
	BenaphoreLocker locker(&cache->lock);

	TRACE(("cache_abort_transaction(id = %ld)\n", id));

	cache_transaction *transaction = lookup_transaction(cache, id);
	if (transaction == NULL) {
		panic("cache_abort_transaction(): invalid transaction ID\n");
		return B_BAD_VALUE;
	}

	// iterate through all blocks and restore their original contents

	cached_block *block = transaction->first_block, *next;
	for (; block != NULL; block = next) {
		next = block->transaction_next;

		if (block->original_data != NULL) {
			TRACE(("cache_abort_transaction(id = %ld): restored contents of block %Ld\n",
				transaction->id, block->block_number));
			memcpy(block->current_data, block->original_data, cache->block_size);
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
	return B_OK;
}


/*!
	Acknowledges the current parent transaction, and starts a new transaction
	from its sub transaction.
	The new transaction also gets a new transaction ID.
*/
int32
cache_detach_sub_transaction(void *_cache, int32 id,
	transaction_notification_hook hook, void *data)
{
	block_cache *cache = (block_cache *)_cache;
	BenaphoreLocker locker(&cache->lock);

	TRACE(("cache_detach_sub_transaction(id = %ld)\n", id));

	cache_transaction *transaction = lookup_transaction(cache, id);
	if (transaction == NULL) {
		panic("cache_detach_sub_transaction(): invalid transaction ID\n");
		return B_BAD_VALUE;
	}
	if (!transaction->has_sub_transaction)
		return B_BAD_VALUE;

	// create a new transaction for the sub transaction
	cache_transaction *newTransaction = new(nothrow) cache_transaction;
	if (transaction == NULL)
		return B_NO_MEMORY;

	newTransaction->id = atomic_add(&cache->next_transaction_id, 1);

	transaction->notification_hook = hook;
	transaction->notification_data = data;

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
		if (block->parent_data != NULL
			&& block->parent_data != block->current_data) {
			// we need to move this block over to the new transaction
			block->original_data = block->parent_data;
			if (last == NULL)
				newTransaction->first_block = block;
			else
				last->transaction_next = block;

			last = block;
		}
		block->parent_data = NULL;

		// move the block to the previous transaction list
		transaction->blocks.Add(block);

		block->previous_transaction = transaction;
		block->transaction_next = NULL;
		block->transaction = newTransaction;
	}

	transaction->open = false;

	hash_insert(cache->transaction_hash, newTransaction);
	cache->last_transaction = newTransaction;

	return B_OK;
}


status_t
cache_abort_sub_transaction(void *_cache, int32 id)
{
	block_cache *cache = (block_cache *)_cache;
	BenaphoreLocker locker(&cache->lock);

	TRACE(("cache_abort_sub_transaction(id = %ld)\n", id));

	cache_transaction *transaction = lookup_transaction(cache, id);
	if (transaction == NULL) {
		panic("cache_abort_sub_transaction(): invalid transaction ID\n");
		return B_BAD_VALUE;
	}
	if (!transaction->has_sub_transaction)
		return B_BAD_VALUE;

	// revert all changes back to the version of the parent

	cached_block *block = transaction->first_block, *next;
	for (; block != NULL; block = next) {
		next = block->transaction_next;

		if (block->parent_data == NULL) {
			if (block->original_data != NULL) {
				// the parent transaction didn't change the block, but the sub
				// transaction did - we need to revert from the original data
				memcpy(block->current_data, block->original_data,
					cache->block_size);
			}
		} else if (block->parent_data != block->current_data) {
			// the block has been changed and must be restored
			TRACE(("cache_abort_sub_transaction(id = %ld): restored contents of block %Ld\n",
				transaction->id, block->block_number));
			memcpy(block->current_data, block->parent_data, cache->block_size);
			cache->Free(block->parent_data);
		}

		block->parent_data = NULL;
	}

	// all subsequent changes will go into the main transaction
	transaction->has_sub_transaction = false;
	return B_OK;
}


status_t
cache_start_sub_transaction(void *_cache, int32 id)
{
	block_cache *cache = (block_cache *)_cache;
	BenaphoreLocker locker(&cache->lock);

	TRACE(("cache_start_sub_transaction(id = %ld)\n", id));

	cache_transaction *transaction = lookup_transaction(cache, id);
	if (transaction == NULL) {
		panic("cache_start_sub_transaction(): invalid transaction ID %ld\n", id);
		return B_BAD_VALUE;
	}

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
	transaction->sub_num_blocks = 0;

	return B_OK;
}


status_t
cache_next_block_in_transaction(void *_cache, int32 id, uint32 *_cookie,
	off_t *_blockNumber, void **_data, void **_unchangedData)
{
	cached_block *block = (cached_block *)*_cookie;
	block_cache *cache = (block_cache *)_cache;

	BenaphoreLocker locker(&cache->lock);

	cache_transaction *transaction = lookup_transaction(cache, id);
	if (transaction == NULL)
		return B_BAD_VALUE;

	if (block == NULL)
		block = transaction->first_block;
	else
		block = block->transaction_next;

	if (block == NULL)
		return B_ENTRY_NOT_FOUND;

	if (_blockNumber)
		*_blockNumber = block->block_number;
	if (_data)
		*_data = block->current_data;
	if (_unchangedData)
		*_unchangedData = block->original_data;

	*_cookie = (uint32)block;
	return B_OK;	
}


int32
cache_blocks_in_transaction(void *_cache, int32 id)
{
	block_cache *cache = (block_cache *)_cache;
	BenaphoreLocker locker(&cache->lock);

	cache_transaction *transaction = lookup_transaction(cache, id);
	if (transaction == NULL)
		return B_BAD_VALUE;

	return transaction->num_blocks;
}


int32
cache_blocks_in_sub_transaction(void *_cache, int32 id)
{
	block_cache *cache = (block_cache *)_cache;
	BenaphoreLocker locker(&cache->lock);

	cache_transaction *transaction = lookup_transaction(cache, id);
	if (transaction == NULL)
		return B_BAD_VALUE;

	return transaction->sub_num_blocks;
}


//	#pragma mark - public block cache API
//	public interface


void
block_cache_delete(void *_cache, bool allowWrites)
{
	block_cache *cache = (block_cache *)_cache;

	if (allowWrites)
		block_cache_sync(cache);

	BenaphoreLocker locker(&cache->lock);

	// free all blocks

	uint32 cookie = 0;
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
block_cache_create(int fd, off_t numBlocks, size_t blockSize, bool readOnly)
{
	block_cache *cache = new(nothrow) block_cache(fd, numBlocks, blockSize,
		readOnly);
	if (cache == NULL)
		return NULL;

	if (cache->InitCheck() != B_OK) {
		delete cache;
		return NULL;
	}

	return cache;
}


status_t
block_cache_sync(void *_cache)
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
			status_t status = write_cached_block(cache, block);
			if (status != B_OK)
				return status;
		}
	}

	hash_close(cache->hash, &iterator, false);
	return B_OK;
}


status_t
block_cache_make_writable(void *_cache, off_t blockNumber, int32 transaction)
{
	block_cache *cache = (block_cache *)_cache;
	BenaphoreLocker locker(&cache->lock);

	if (cache->read_only)
		panic("tried to make block writable on a read-only cache!");

	// ToDo: this can be done better!
	void *block = get_writable_cached_block(cache, blockNumber,
		blockNumber, 1, transaction, false);
	if (block != NULL) {
		put_cached_block((block_cache *)_cache, blockNumber);
		return B_OK;
	}

	return B_ERROR;
}


void *
block_cache_get_writable_etc(void *_cache, off_t blockNumber, off_t base,
	off_t length, int32 transaction)
{
	block_cache *cache = (block_cache *)_cache;
	BenaphoreLocker locker(&cache->lock);

	TRACE(("block_cache_get_writable_etc(block = %Ld, transaction = %ld)\n",
		blockNumber, transaction));
	if (cache->read_only)
		panic("tried to get writable block on a read-only cache!");

	return get_writable_cached_block(cache, blockNumber, base, length,
		transaction, false);
}


void *
block_cache_get_writable(void *_cache, off_t blockNumber, int32 transaction)
{
	return block_cache_get_writable_etc(_cache, blockNumber,
		blockNumber, 1, transaction);
}


void *
block_cache_get_empty(void *_cache, off_t blockNumber, int32 transaction)
{
	block_cache *cache = (block_cache *)_cache;
	BenaphoreLocker locker(&cache->lock);

	TRACE(("block_cache_get_empty(block = %Ld, transaction = %ld)\n",
		blockNumber, transaction));
	if (cache->read_only)
		panic("tried to get empty writable block on a read-only cache!");

	return get_writable_cached_block((block_cache *)_cache, blockNumber,
				blockNumber, 1, transaction, true);
}


const void *
block_cache_get_etc(void *_cache, off_t blockNumber, off_t base, off_t length)
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
block_cache_get(void *_cache, off_t blockNumber)
{
	return block_cache_get_etc(_cache, blockNumber, blockNumber, 1);
}


/*!
	Changes the internal status of a writable block to \a dirty. This can be
	helpful in case you realize you don't need to change that block anymore
	for whatever reason.

	Note, you must only use this function on blocks that were acquired
	writable!
*/
status_t
block_cache_set_dirty(void *_cache, off_t blockNumber, bool dirty,
	int32 transaction)
{
	// TODO: not yet implemented
	if (dirty)
		panic("block_cache_set_dirty(): not yet implemented that way!\n");

	return B_OK;
}


void
block_cache_put(void *_cache, off_t blockNumber)
{
	block_cache *cache = (block_cache *)_cache;
	BenaphoreLocker locker(&cache->lock);

	put_cached_block(cache, blockNumber);
}

}	// namespace HaikuKernelEmu
}	// namespace UserlandFS
