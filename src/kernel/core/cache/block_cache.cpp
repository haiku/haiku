/* 
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <KernelExport.h>
#include <fs_cache.h>

#include <util/kernel_cpp.h>
#include <util/DoublyLinkedList.h>
#include <cache.h>
#include <khash.h>
#include <lock.h>

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


// ToDo: this is a naive implementation to test the API:
//	1) it does not have any useful memory management (just uses malloc/free)
//	2) block reading/writing is not at all optimized for speed, it will
//	   just read and write single blocks.
//	3) the locking could be improved; getting a block should not need to
//	   wait for blocks to be written
//	4) dirty blocks are only written back if asked for
//	5) blocks are never removed yet

#define TRACE_BLOCK_CACHE
#ifdef TRACE_BLOCK_CACHE
#	define TRACE(x)	dprintf x
#else
#	define TRACE(x) ;
#endif

#define DEBUG_CHANGED

struct cache_transaction;
typedef DoublyLinked::Link block_link;

struct cached_block {
	cached_block	*next;			// next in hash
	cached_block	*transaction_next;
	block_link		previous_transaction_link;
	off_t			block_number;
	void			*data;
	void			*original;
#ifdef DEBUG_CHANGED
	void			*compare;
#endif
	int32			ref_count;
	int32			lock;
	bool			is_dirty;
	cache_transaction *transaction;
	cache_transaction *previous_transaction;
};

struct block_cache {
	hash_table	*hash;
	benaphore	lock;
	int			fd;
	off_t		max_blocks;
	size_t		block_size;
	int32		next_transaction_id;
	hash_table	*transaction_hash;
};

typedef DoublyLinked::List<cached_block, &cached_block::previous_transaction_link> block_list;

struct cache_transaction {
	cache_transaction *next;
	int32			id;
	int32			num_blocks;
	cached_block	*first_block;
	block_list		blocks;
	transaction_notification_hook notification_hook;
	void			*notification_data;
	bool			open;
};

struct cache {
	hash_table	*hash;
	benaphore	lock;
	off_t		max_blocks;
	size_t		block_size;
};

static const int32 kNumCaches = 16;
struct cache sCaches[kNumCaches];
	// we can cache the first 16 fds (I said we were dumb, right?)


class BenaphoreLocker {
	public:
		BenaphoreLocker(int fd)
			: fBenaphore(NULL)
		{
			if (fd < 0 || fd >= kNumCaches)
				return;

			fBenaphore = &sCaches[fd].lock;
			benaphore_lock(fBenaphore);
		}

		BenaphoreLocker(block_cache *cache)
			: fBenaphore(&cache->lock)
		{
			benaphore_lock(fBenaphore);
		}

		~BenaphoreLocker()
		{
			if (fBenaphore != NULL)
				benaphore_unlock(fBenaphore);
		}

		status_t InitCheck()
		{
			return fBenaphore != NULL ? B_OK : B_ERROR;
		}

	private:
		benaphore	*fBenaphore;
};


//	private transaction functions


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

	return *id % range;
}


static void
delete_transaction(block_cache *cache, cache_transaction *transaction)
{
	hash_remove(cache->transaction_hash, transaction);
	delete transaction;
}


static cache_transaction *
lookup_transaction(block_cache *cache, int32 id)
{
	return (cache_transaction *)hash_lookup(cache->transaction_hash, &id);
}


//	#pragma mark -
//	private cached block functions


static int
cached_block_compare(void *_cacheEntry, const void *_block)
{
	cached_block *cacheEntry = (cached_block *)_cacheEntry;
	const off_t *block = (const off_t *)_block;

	return cacheEntry->block_number - *block;
}


static uint32
cached_block_hash(void *_cacheEntry, const void *_block, uint32 range)
{
	cached_block *cacheEntry = (cached_block *)_cacheEntry;
	const off_t *block = (const off_t *)_block;

	if (cacheEntry != NULL)
		return cacheEntry->block_number % range;

	return *block % range;
}


static void
free_cached_block(cached_block *block)
{
	free(block->data);
	free(block->original);
	free(block);
}


static cached_block *
new_cached_block(block_cache *cache, off_t blockNumber, bool cleared = false)
{
	cached_block *block = (cached_block *)malloc(sizeof(cached_block));
	if (block == NULL)
		return NULL;

	if (!cleared) {
		block->data = malloc(cache->block_size);
		if (block->data == NULL) {
			free(block);
			return NULL;
		}
	} else
		block->data = NULL;

	block->block_number = blockNumber;
	block->lock = 0;
	block->transaction_next = NULL;
	block->transaction = block->previous_transaction = NULL;
	block->original = NULL;
#ifdef DEBUG_CHANGED
	block->compare = NULL;
#endif

	hash_insert(cache->hash, block);

	return block;
}


static void
put_cached_block(block_cache *cache, cached_block *block)
{
#ifdef DEBUG_CHANGED
	if (!block->is_dirty && block->compare != NULL && memcmp(block->data, block->compare, cache->block_size))
		panic("block_cache: supposed to be clean block was changed!\n");
#endif

	block->lock--;
}


static void
put_cached_block(block_cache *cache, off_t blockNumber)
{
	cached_block *block = (cached_block *)hash_lookup(cache->hash, &blockNumber);
	if (block != NULL)
		put_cached_block(cache, block);
}


static cached_block *
get_cached_block(block_cache *cache, off_t blockNumber, bool cleared = false)
{
	cached_block *block = (cached_block *)hash_lookup(cache->hash, &blockNumber);
	bool allocated = false;

	if (block == NULL) {
		// read block into cache
		block = new_cached_block(cache, blockNumber, cleared);
		if (block == NULL)
			return NULL;

		allocated = true;
	}
	
	if (!allocated && block->data == NULL && !cleared) {
		// there is no block yet, but we need one
		block->data = malloc(cache->block_size);
		if (block->data == NULL)
			return NULL;

		allocated = true;
	}

	if (allocated && !cleared) {
		int32 blockSize = cache->block_size;

		if (read_pos(cache->fd, blockNumber * blockSize, block->data, blockSize) < blockSize) {
			free_cached_block(block);
			return NULL;
		}
	}

	block->lock++;
	return block;
}


static void *
get_writable_cached_block(block_cache *cache, off_t blockNumber, off_t base, off_t length,
	int32 transactionID, bool cleared)
{
	BenaphoreLocker locker(cache);

	cached_block *block = get_cached_block(cache, blockNumber, cleared);
	if (block == NULL)
		return NULL;

	// if there is no transaction support, we just return the current block
	if (transactionID == -1) {
		if (cleared && block->data == NULL) {
			block->data = malloc(cache->block_size);
			if (block->data == NULL) {
				put_cached_block(cache, block);
				return NULL;
			}
		}
		if (cleared)
			memset(block->data, 0, cache->block_size);

		block->is_dirty = true;
			// mark the block as dirty

		return block->data;
	}

	// ToDo: note, even if we panic, we should probably put the cached block
	//	back before we return

	if (block->transaction != NULL && block->transaction->id != transactionID) {
		// ToDo: we have to wait here until the other transaction is done.
		//	Maybe we should even panic, since we can't prevent any deadlocks.
		panic("block_cache_get_writable(): asked to get busy writable block\n");
		return NULL;
	}
	if (block->transaction == NULL && transactionID != -1) {
		// get new transaction
		cache_transaction *transaction = lookup_transaction(cache, transactionID);
		if (transaction == NULL) {
			panic("block_cache_get_writable(): invalid transaction %ld!\n", transactionID);
			return NULL;
		}
		if (!transaction->open) {
			panic("block_cache_get_writable(): transaction already done!\n");
			return NULL;
		}

		block->transaction = transaction;

		// attach the block to the transaction block list
		block->transaction_next = transaction->first_block;
		transaction->first_block = block;
		transaction->num_blocks++;
	}

	if (block->data != NULL && block->original == NULL) {
		// we already have data, so we need to save it
		block->original = malloc(cache->block_size);
		if (block->original == NULL) {
			put_cached_block(cache, block);
			return NULL;
		}

		memcpy(block->original, block->data, cache->block_size);
	}

	if (block->data == NULL && cleared) {
		// there is no data yet, we need a clean new block
		block->data = malloc(cache->block_size);
		if (block->data == NULL) {
			put_cached_block(cache, block);
			return NULL;
		}
		memset(block->data, 0, cache->block_size);
	}
	block->is_dirty = true;

	return block->data;
}


status_t
write_cached_block(block_cache *cache, cached_block *block, bool deleteTransaction = true)
{
	cache_transaction *previous = block->previous_transaction;
	int32 blockSize = cache->block_size;

	TRACE(("write_cached_block(block %Ld)\n", block->block_number));

	ssize_t written = write_pos(cache->fd, block->block_number * blockSize, block->data, blockSize);

	if (written < blockSize) {
		dprintf("could not write back block %Ld (errno = %d (%s))\n", block->block_number, errno, strerror(errno));
		return B_IO_ERROR;
	}

	block->is_dirty = false;

	if (previous != NULL) {
		previous->blocks.Remove(block);
		block->previous_transaction = NULL;

		if (--previous->num_blocks == 0) {
			if (previous->notification_hook != NULL)
				previous->notification_hook(previous->id, previous->notification_data);

			if (deleteTransaction)
				delete_transaction(cache, previous);
		}
	}

	return B_OK;
}


//	#pragma mark -
//	Transactions


extern "C" int32
cache_transaction_start(void *_cache, transaction_notification_hook hook, void *data)
{
	block_cache *cache = (block_cache *)_cache;

	cache_transaction *transaction = new cache_transaction;
	if (transaction == NULL)
		return B_NO_MEMORY;

	transaction->id = atomic_add(&cache->next_transaction_id, 1);
	transaction->num_blocks = 0;
	transaction->first_block = NULL;
	transaction->notification_hook = hook;
	transaction->notification_data = data;
	transaction->open = true;

	TRACE(("cache_transaction_start(): id %ld started\n", transaction->id));

	BenaphoreLocker locker(cache);
	hash_insert(cache->transaction_hash, transaction);

	return transaction->id;
}


extern "C" status_t
cache_transaction_sync(void *_cache, int32 id)
{
	block_cache *cache = (block_cache *)_cache;
	BenaphoreLocker locker(cache);
	status_t status = B_ENTRY_NOT_FOUND;

	hash_iterator iterator;
	hash_open(cache->transaction_hash, &iterator);

	cache_transaction *transaction;
	while ((transaction = (cache_transaction *)hash_next(cache->transaction_hash, &iterator)) != NULL) {
		// ToDo: fix hash interface to make this easier

		if (transaction->id <= id && !transaction->open) {
			while (transaction->num_blocks > 0) {
				status = write_cached_block(cache, block_list::GetItem(transaction->blocks.Head()), false);
				if (status != B_OK)
					return status;
			}
			delete_transaction(cache, transaction);
			hash_rewind(cache->transaction_hash, &iterator);
		}
	}

	hash_close(cache->transaction_hash, &iterator, false);
	return B_OK;
}


extern "C" status_t
cache_transaction_end(void *_cache, int32 id)
{
	block_cache *cache = (block_cache *)_cache;
	BenaphoreLocker locker(cache);

	TRACE(("cache_transaction_end(id = %ld)\n", id));

	cache_transaction *transaction = lookup_transaction(cache, id);
	if (transaction == NULL) {
		panic("cache_transaction_end(): invalid transaction ID\n");
		return B_BAD_VALUE;
	}

	// iterate through all blocks and free the unchanged original contents

	cached_block *block = transaction->first_block, *next;
	for (; block != NULL; block = next) {
		next = block->transaction_next;

		if (block->previous_transaction != NULL) {
			// need to write back pending changes
			write_cached_block(cache, block);
		}

		if (block->original != NULL) {
			free(block->original);
			block->original = NULL;
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


extern "C" status_t
cache_transaction_abort(void *_cache, int32 id)
{
	block_cache *cache = (block_cache *)_cache;
	BenaphoreLocker locker(cache);

	return B_OK;
}


extern "C" status_t
cache_transaction_next_block(void *_cache, int32 id, uint32 *_cookie, off_t *_blockNumber,
	void **_data, void **_unchangedData)
{
	cached_block *block = (cached_block *)*_cookie;
	block_cache *cache = (block_cache *)_cache;

	BenaphoreLocker locker(cache);

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
		*_data = block->data;
	if (_unchangedData)
		*_unchangedData = block->original;

	*_cookie = (uint32)block;
	return B_OK;	
}


//	#pragma mark -
//	public interface


extern "C" void
block_cache_delete(void *_cache, bool allowWrites)
{
	block_cache *cache = (block_cache *)_cache;

	if (allowWrites)
		block_cache_sync(cache);

	// free all blocks

	uint32 cookie = 0;
	cached_block *block;
	while ((block = (cached_block *)hash_remove_first(cache->hash, &cookie)) != NULL) {
		free_cached_block(block);
	}
	
	// free all transactions (they will all be aborted)	

	cookie = 0;
	cache_transaction *transaction;
	while ((transaction = (cache_transaction *)hash_remove_first(cache->transaction_hash, &cookie)) != NULL) {
		delete transaction;
	}

	hash_uninit(cache->hash);
	hash_uninit(cache->transaction_hash);
	benaphore_destroy(&cache->lock);

	delete cache;
}


extern "C" void *
block_cache_create(int fd, off_t numBlocks, size_t blockSize)
{
	block_cache *cache = new block_cache;
	if (cache == NULL)
		return NULL;

	cache->hash = hash_init(32, 0, &cached_block_compare, &cached_block_hash);
	if (cache->hash == NULL)
		goto err1;

	cache->transaction_hash = hash_init(16, 0, &transaction_compare, &transaction_hash);
	if (cache->transaction_hash == NULL)
		goto err2;

	if (benaphore_init(&cache->lock, "block cache") < B_OK)
		goto err3;

	cache->fd = fd;
	cache->max_blocks = numBlocks;
	cache->block_size = blockSize;
	cache->next_transaction_id = 1;

	return cache;

err3:
	hash_uninit(cache->transaction_hash);
err2:
	hash_uninit(cache->hash);
err1:
	delete cache;
	return NULL;
}


extern "C" status_t
block_cache_sync(void *_cache)
{
	block_cache *cache = (block_cache *)_cache;

	// we will sync all dirty blocks to disk that have a completed
	// transaction or no transaction only

	BenaphoreLocker locker(cache);
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


extern "C" status_t
block_cache_make_writable(void *_cache, off_t blockNumber, int32 transaction)
{
	// ToDo: this can be done better!
	void *block = block_cache_get_writable_etc(_cache, blockNumber, blockNumber, 1, transaction);
	if (block != NULL) {
		put_cached_block((block_cache *)_cache, blockNumber);
		return B_OK;
	}

	return B_ERROR;
}


extern "C" void *
block_cache_get_writable_etc(void *_cache, off_t blockNumber, off_t base, off_t length,
	int32 transaction)
{
	TRACE(("block_cache_get_writable_etc(block = %Ld, transaction = %ld)\n", blockNumber, transaction));

	return get_writable_cached_block((block_cache *)_cache, blockNumber,
				base, length, transaction, false);
}


extern "C" void *
block_cache_get_writable(void *_cache, off_t blockNumber, int32 transaction)
{
	return block_cache_get_writable_etc(_cache, blockNumber, blockNumber, 1, transaction);
}


extern "C" void *
block_cache_get_empty(void *_cache, off_t blockNumber, int32 transaction)
{
	TRACE(("block_cache_get_empty(block = %Ld, transaction = %ld)\n", blockNumber, transaction));

	return get_writable_cached_block((block_cache *)_cache, blockNumber,
				blockNumber, 1, transaction, true);
}


extern "C" const void *
block_cache_get_etc(void *_cache, off_t blockNumber, off_t base, off_t length)
{
	block_cache *cache = (block_cache *)_cache;
	BenaphoreLocker locker(cache);

	cached_block *block = get_cached_block(cache, blockNumber);
	if (block == NULL)
		return NULL;

#ifdef DEBUG_CHANGED
	if (block->compare == NULL) {
		block->compare = malloc(cache->block_size);
		if (block->compare != NULL)
			memcpy(block->compare, block->data, cache->block_size);
	}
#endif
	return block->data;
}


extern "C" const void *
block_cache_get(void *_cache, off_t blockNumber)
{
	return block_cache_get_etc(_cache, blockNumber, blockNumber, 1);
}


extern "C" status_t
block_cache_set_dirty(void *_cache, off_t blockNumber, bool isDirty, int32 transaction)
{
	// not yet implemented
	// Note, you must only use this function on blocks that were acquired writable!
	if (isDirty)
		panic("block_cache_set_dirty(): not yet implemented that way!\n");

	return B_OK;
}


extern "C" void
block_cache_put(void *_cache, off_t blockNumber)
{
	block_cache *cache = (block_cache *)_cache;
	BenaphoreLocker locker(cache);

	put_cached_block(cache, blockNumber);
}


//	#pragma mark -
//	private BeOS compatible interface (to be removed)

#if 0

void
put_cache_entry(int fd, cached_block *entry)
{
	if (entry == NULL)
		return;

	if (--entry->ref_count > 0)
		return;

	hash_remove(sCaches[fd].hash, entry);
	free(entry->data);
	free(entry);
}


cached_block *
new_cache_entry(int fd, off_t blockNumber, size_t blockSize)
{
	cached_block *entry = (cached_block *)malloc(sizeof(cached_block));
	if (entry == NULL)
		return NULL;

	entry->data = malloc(blockSize);
	if (entry->data == NULL) {
		free(entry);
		return NULL;
	}

	sCaches[fd].block_size = blockSize;
		// store block size (once would be sufficient, though)

	entry->block_number = blockNumber;
	entry->ref_count = 1;
	entry->lock = 0;

	hash_insert(sCaches[fd].hash, entry);

	return entry;
}


static cached_block *
lookup_cache_entry(int fd, off_t blockNumber)
{
	cached_block *entry = (cached_block *)hash_lookup(sCaches[fd].hash, &blockNumber);
	if (entry == NULL)
		return NULL;

	entry->ref_count++;
	return entry;
}


//	#pragma mark -
//	public BeOS compatible read-only interface (to be removed)


extern "C" void
force_cache_flush(int dev, int prefer_log_blocks)
{
}


extern "C" int
flush_blocks(int dev, off_t bnum, int nblocks)
{
	return 0;
}


extern "C" int
flush_device(int dev, int warn_locked)
{
	return 0;
}


extern "C" int
init_cache_for_device(int fd, off_t max_blocks)
{
	if (fd < 0 || fd >= kNumCaches)
		return B_ERROR;
	if (sCaches[fd].hash != NULL)
		return B_BUSY;

	sCaches[fd].hash = hash_init(32, 0, &cached_block_compare, &cached_block_hash);
	return benaphore_init(&sCaches[fd].lock, "block cache");
}


extern "C" int
remove_cached_device_blocks(int fd, int allowWrite)
{
	if (fd < 0 || fd >= kNumCaches || sCaches[fd].hash == NULL)
		return B_ERROR;

	hash_table *hash = sCaches[fd].hash;

	uint32 cookie = 0;
	cached_block *block;
	while ((block = (cached_block *)hash_remove_first(hash, &cookie)) != NULL) {
		free_cached_block(block);
	}

	hash_uninit(hash);
	benaphore_destroy(&sCaches[fd].lock);

	return B_OK;
}


extern "C" void *
get_block(int fd, off_t blockNumber, int blockSize)
{
	BenaphoreLocker locker(fd);
	if (locker.InitCheck() != B_OK)
		return NULL;

	cached_block *entry = lookup_cache_entry(fd, blockNumber);
	if (entry == NULL) {
		// read entry into cache
		entry = new_cache_entry(fd, blockNumber, blockSize);
		if (entry == NULL)
			return NULL;

		if (read_pos(fd, blockNumber * blockSize, entry->data, blockSize) < blockSize) {
			free(entry->data);
			free(entry);
			return NULL;
		}
	}

	entry->lock++;

	return entry->data;
}


extern "C" void *
get_empty_block(int fd, off_t blockNumber, int blockSize)
{
	BenaphoreLocker locker(fd);
	if (locker.InitCheck() != B_OK)
		return NULL;

	cached_block *entry = lookup_cache_entry(fd, blockNumber);
	if (entry == NULL) {
		// create new cache entry
		entry = new_cache_entry(fd, blockNumber, blockSize);
		if (entry == NULL)
			return NULL;
	}
	memset(entry->data, 0, blockSize);

	entry->lock++;

	return entry->data;
}


extern "C" int
release_block(int fd, off_t blockNumber)
{
	BenaphoreLocker locker(fd);
	if (locker.InitCheck() != B_OK)
		return B_ERROR;

	cached_block *entry = lookup_cache_entry(fd, blockNumber);
	if (entry == NULL)
		panic("release_block() called on block %Ld that was not cached\n", blockNumber);

	entry->lock--;
	put_cache_entry(fd, entry);
	return 0;
}


extern "C" int
mark_blocks_dirty(int fd, off_t blockNumber, int numBlocks)
{
	return -1;
}


extern "C" int
cached_read(int fd, off_t blockNumber, void *data, off_t numBlocks, int blockSize)
{
	BenaphoreLocker locker(fd);
	if (locker.InitCheck() != B_OK)
		return B_ERROR;

	ssize_t bytes = numBlocks * blockSize;
	if (read_pos(fd, blockNumber * blockSize, data, bytes) < bytes)
		return -1;

	return 0;
}


extern "C" int
cached_write(int fd, off_t blockNumber, const void *data, off_t numBlocks, int blockSize)
{
	return -1;
}


extern "C" int
cached_write_locked(int fd, off_t blockNumber, const void *data, off_t numBlocks, int blockSize)
{
	return -1;
}


extern "C" int
set_blocks_info(int fd, off_t *blocks, int numBlocks,
	void (*func)(off_t bnum, size_t nblocks, void *arg),
	void *arg)
{
	return -1;
}

#endif
