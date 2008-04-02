/*
 * Copyright 2004-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "block_cache_private.h"

#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <KernelExport.h>
#include <fs_cache.h>

#include <block_cache.h>
#include <lock.h>
#include <vm_low_memory.h>
#include <tracing.h>
#include <util/kernel_cpp.h>
#include <util/DoublyLinkedList.h>
#include <util/AutoLock.h>
#include <util/khash.h>


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
#	define TRACE(x)	dprintf x
#else
#	define TRACE(x) ;
#endif

#define DEBUG_BLOCK_CACHE
//#define DEBUG_CHANGED

// This macro is used for fatal situations that are acceptable in a running
// system, like out of memory situations - should only panic for debugging.
#define FATAL(x) panic x


static const bigtime_t kTransactionIdleTime = 2000000LL;
	// a transaction is considered idle after 2 seconds of inactivity

struct cache_hook : DoublyLinkedListLinkImpl<cache_hook> {
	transaction_notification_hook	hook;
	void							*data;
};

typedef DoublyLinkedList<cache_hook> HookList;

struct cache_transaction {
	cache_transaction();

	cache_transaction *next;
	int32			id;
	int32			num_blocks;
	int32			main_num_blocks;
	int32			sub_num_blocks;
	cached_block	*first_block;
	block_list		blocks;
	transaction_notification_hook notification_hook;
	void			*notification_data;
	HookList		listeners;
	bool			open;
	bool			has_sub_transaction;
	bigtime_t		last_used;
};

#ifdef BLOCK_CACHE_TRANSACTION_TRACING
namespace TransactionTracing {

class Action : public AbstractTraceEntry {
	public:
		Action(const char *label, block_cache *cache,
				cache_transaction *transaction)
			:
			fCache(cache),
			fTransaction(transaction),
			fID(transaction->id),
			fSub(transaction->has_sub_transaction),
			fNumBlocks(transaction->num_blocks),
			fSubNumBlocks(transaction->sub_num_blocks)
		{
			strlcpy(fLabel, label, sizeof(fLabel));
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("cache %p, %s transaction %p (id %ld)%s"
				", %ld/%ld blocks", fCache, fLabel, fTransaction, fID,
				fSub ? " sub" : "", fNumBlocks, fSubNumBlocks);
		}

	private:
		char				fLabel[12];
		block_cache			*fCache;
		cache_transaction	*fTransaction;
		int32				fID;
		bool				fSub;
		int32				fNumBlocks;
		int32				fSubNumBlocks;
};

class Detach : public AbstractTraceEntry {
	public:
		Detach(block_cache *cache, cache_transaction *transaction,
				cache_transaction *newTransaction)
			:
			fCache(cache),
			fTransaction(transaction),
			fID(transaction->id),
			fSub(transaction->has_sub_transaction),
			fNewTransaction(newTransaction),
			fNewID(newTransaction->id)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("cache %p, detach transaction %p (id %ld)"
				"from transaction %p (id %ld)%s",
				fCache, fNewTransaction, fNewID, fTransaction, fID,
				fSub ? " sub" : "");
		}

	private:
		block_cache			*fCache;
		cache_transaction	*fTransaction;
		int32				fID;
		bool				fSub;
		cache_transaction	*fNewTransaction;
		int32				fNewID;
};

class Abort : public AbstractTraceEntry {
	public:
		Abort(block_cache *cache, cache_transaction *transaction)
			:
			fCache(cache),
			fTransaction(transaction),
			fID(transaction->id),
			fNumBlocks(0)
		{
			bool isSub = transaction->has_sub_transaction;
			fNumBlocks = isSub ? transaction->sub_num_blocks
				: transaction->num_blocks;
			fBlocks = (off_t *)alloc_tracing_buffer(fNumBlocks * sizeof(off_t));
			if (fBlocks != NULL) {
				cached_block *block = transaction->first_block;
				for (int32 i = 0; block != NULL && i < fNumBlocks;
						block = block->transaction_next) {
					fBlocks[i++] = block->block_number;
				}
			} else
				fNumBlocks = 0;
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("cache %p, abort transaction "
				"%p (id %ld), blocks", fCache, fTransaction, fID);
			for (int32 i = 0; i < fNumBlocks && !out.IsFull(); i++)
				out.Print(" %Ld", fBlocks[i]);
		}

	private:
		block_cache			*fCache;
		cache_transaction	*fTransaction;
		int32				fID;
		off_t				*fBlocks;
		int32				fNumBlocks;
};

}	// namespace TransactionTracing

#	define T(x) new(std::nothrow) TransactionTracing::x;
#else
#	define T(x) ;
#endif


static status_t write_cached_block(block_cache *cache, cached_block *block,
	bool deleteTransaction = true);


static DoublyLinkedList<block_cache> sCaches;
static mutex sCachesLock;
static DoublyLinkedListLink<block_cache> sMarkCache;
	// TODO: this only works if the link is the first entry of block_cache
static object_cache *sBlockCache;


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
	last_used = system_time();
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


/*!	Notifies all listeners of this transaction, and removes them
	afterwards if requested via \a removeListener.
*/
static void
notify_transaction_listeners(cache_transaction *transaction, int32 event,
	bool removeListener)
{
	HookList::Iterator iterator = transaction->listeners.GetIterator();
	while (iterator.HasNext()) {
		cache_hook *hook = iterator.Next();

		hook->hook(transaction->id, event, hook->data);

		if (removeListener) {
			iterator.Remove();
			delete hook;
		}
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
lookup_transaction(block_cache *cache, int32 id)
{
	return (cache_transaction *)hash_lookup(cache->transaction_hash, &id);
}


//	#pragma mark - cached_block


int
compare_blocks(const void *_blockA, const void *_blockB)
{
	cached_block *blockA = *(cached_block **)_blockA;
	cached_block *blockB = *(cached_block **)_blockB;

	off_t diff = blockA->block_number - blockB->block_number;
	if (diff > 0)
		return 1;

	return diff < 0 ? -1 : 0;
}


/*static*/ int
cached_block::Compare(void *_cacheEntry, const void *_block)
{
	cached_block *cacheEntry = (cached_block *)_cacheEntry;
	const off_t *block = (const off_t *)_block;

	off_t diff = cacheEntry->block_number - *block;
	if (diff > 0)
		return 1;

	return diff < 0 ? -1 : 0;
}



/*static*/ uint32
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
	num_dirty_blocks(0),
	read_only(readOnly)
{
	mutex_lock(&sCachesLock);
	sCaches.Add(this);
	mutex_unlock(&sCachesLock);

	buffer_cache = create_object_cache_etc("block cache buffers", blockSize,
		8, 0, CACHE_LARGE_SLAB, NULL, NULL, NULL, NULL);
	if (buffer_cache == NULL)
		return;

	hash = hash_init(1024, offsetof(cached_block, next), &cached_block::Compare,
		&cached_block::Hash);
	if (hash == NULL)
		return;

	transaction_hash = hash_init(16, offsetof(cache_transaction, next),
		&transaction_compare, &::transaction_hash);
	if (transaction_hash == NULL)
		return;

	if (recursive_lock_init(&lock, "block cache") < B_OK)
		return;

	register_low_memory_handler(&block_cache::LowMemoryHandler, this, 0);
}


block_cache::~block_cache()
{
	mutex_lock(&sCachesLock);
	sCaches.Remove(this);
	mutex_unlock(&sCachesLock);

	unregister_low_memory_handler(&block_cache::LowMemoryHandler, this);

	recursive_lock_destroy(&lock);

	hash_uninit(transaction_hash);
	hash_uninit(hash);

	delete_object_cache(buffer_cache);
}


status_t
block_cache::InitCheck()
{
	if (lock.sem < B_OK)
		return lock.sem;

	if (buffer_cache == NULL || hash == NULL || transaction_hash == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


void
block_cache::Free(void *buffer)
{
	if (buffer != NULL)
		object_cache_free(buffer_cache, buffer);
}


void *
block_cache::Allocate()
{
	return object_cache_alloc(buffer_cache, 0);
}


void
block_cache::FreeBlock(cached_block *block)
{
	Free(block->current_data);

	if (block->original_data != NULL || block->parent_data != NULL) {
		panic("block_cache::FreeBlock(): %Ld, original %p, parent %p\n",
			block->block_number, block->original_data, block->parent_data);
	}

#ifdef DEBUG_CHANGED
	Free(block->compare);
#endif

	object_cache_free(sBlockCache, block);
}


cached_block *
block_cache::_GetUnusedBlock()
{
	TRACE(("block_cache: get unused block\n"));

	for (block_list::Iterator iterator = unused_blocks.GetIterator();
			cached_block *block = iterator.Next();) {
		// this can only happen if no transactions are used
		if (block->is_dirty)
			write_cached_block(this, block, false);

		// remove block from lists
		iterator.Remove();
		hash_remove(hash, block);

		// TODO: see if parent/compare data is handled correctly here!
		if (block->parent_data != NULL
			&& block->parent_data != block->original_data)
			Free(block->parent_data);
		if (block->original_data != NULL)
			Free(block->original_data);

		return block;
	}

	return NULL;
}


/*! Allocates a new block for \a blockNumber, ready for use */
cached_block *
block_cache::NewBlock(off_t blockNumber)
{
	cached_block *block = (cached_block *)object_cache_alloc(sBlockCache, 0);
	if (block == NULL) {
		dprintf("block allocation failed, unused list is %sempty.\n",
			unused_blocks.IsEmpty() ? "" : "not ");

		// allocation failed, try to reuse an unused block
		block = _GetUnusedBlock();
		if (block == NULL) {
			FATAL(("could not allocate block!\n"));
			return NULL;
		}
	}

	block->current_data = Allocate();
	if (block->current_data == NULL) {
		object_cache_free(sBlockCache, block);
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

	return block;
}


void
block_cache::RemoveUnusedBlocks(int32 maxAccessed, int32 count)
{
	TRACE(("block_cache: remove up to %ld unused blocks\n", count));

	for (block_list::Iterator iterator = unused_blocks.GetIterator();
			cached_block *block = iterator.Next();) {
		if (maxAccessed < block->accessed)
			continue;

		TRACE(("  remove block %Ld, accessed %ld times\n",
			block->block_number, block->accessed));

		// this can only happen if no transactions are used
		if (block->is_dirty)
			write_cached_block(this, block, false);

		// remove block from lists
		iterator.Remove();
		hash_remove(hash, block);

		FreeBlock(block);

		if (--count <= 0)
			break;
	}
}


void
block_cache::LowMemoryHandler(void *data, int32 level)
{
	block_cache *cache = (block_cache *)data;
	RecursiveLocker locker(&cache->lock);

	if (!locker.IsLocked()) {
		// If our block_cache were deleted, it could be that we had
		// been called before that deletion went through, therefore,
		// acquiring its lock might fail.
		return;
	}

	TRACE(("block_cache: low memory handler called with level %ld\n", level));

	// free some blocks according to the low memory state
	// (if there is enough memory left, we don't free any)

	int32 free = 1;
	int32 accessed = 1;
	switch (vm_low_memory_state()) {
		case B_NO_LOW_MEMORY:
			return;
		case B_LOW_MEMORY_NOTE:
			free = 50;
			accessed = 2;
			break;
		case B_LOW_MEMORY_WARNING:
			free = 200;
			accessed = 10;
			break;
		case B_LOW_MEMORY_CRITICAL:
			free = LONG_MAX;
			accessed = LONG_MAX;
			break;
	}

	cache->RemoveUnusedBlocks(accessed, free);
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

	if (block->ref_count < 1) {
		panic("Invalid ref_count for block %p, cache %p\n", block, cache);
		return;
	}

	if (--block->ref_count == 0
		&& block->transaction == NULL
		&& block->previous_transaction == NULL) {
		// put this block in the list of unused blocks
		block->unused = true;
if (block->original_data != NULL || block->parent_data != NULL)
	panic("put_cached_block(): %p (%Ld): %p, %p\n", block, block->block_number, block->original_data, block->parent_data);
		cache->unused_blocks.Add(block);
//		block->current_data = cache->allocator->Release(block->current_data);
	}

	// free some blocks according to the low memory state
	// (if there is enough memory left, we don't free any)

	int32 free = 1;
	switch (vm_low_memory_state()) {
		case B_NO_LOW_MEMORY:
			return;
		case B_LOW_MEMORY_NOTE:
			free = 1;
			break;
		case B_LOW_MEMORY_WARNING:
			free = 5;
			break;
		case B_LOW_MEMORY_CRITICAL:
			free = 20;
			break;
	}

	cache->RemoveUnusedBlocks(LONG_MAX, free);
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

		hash_insert_grow(cache->hash, block);
		*_allocated = true;
	}

	if (*_allocated && readBlock) {
		int32 blockSize = cache->block_size;

		ssize_t bytesRead = read_pos(cache->fd, blockNumber * blockSize,
			block->current_data, blockSize);
		if (bytesRead < blockSize) {
			hash_remove(cache->hash, block);
			cache->FreeBlock(block);
			FATAL(("could not read block %Ld: bytesRead: %ld, error: %s\n",
				blockNumber, bytesRead, strerror(errno)));
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

		if (!block->is_dirty)
			cache->num_dirty_blocks++;

		block->is_dirty = true;
			// mark the block as dirty

		return block->current_data;
	}

	cache_transaction *transaction = block->transaction;

	if (transaction != NULL && transaction->id != transactionID) {
		// ToDo: we have to wait here until the other transaction is done.
		//	Maybe we should even panic, since we can't prevent any deadlocks.
		panic("get_writable_cached_block(): asked to get busy writable block (transaction %ld)\n", block->transaction->id);
		put_cached_block(cache, block);
		return NULL;
	}
	if (transaction == NULL && transactionID != -1) {
		// get new transaction
		transaction = lookup_transaction(cache, transactionID);
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
	if (transaction != NULL)
		transaction->last_used = system_time();

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
		transaction->sub_num_blocks++;
	} else if (transaction != NULL && transaction->has_sub_transaction
		&& block->parent_data == NULL && wasUnchanged)
		transaction->sub_num_blocks++;

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

	if (cache->num_dirty_blocks > 0)
		cache->num_dirty_blocks--;

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
				previous->notification_hook(previous->id, TRANSACTION_WRITTEN,
					previous->notification_data);
			}
			notify_transaction_listeners(previous, TRANSACTION_WRITTEN, false);

			if (deleteTransaction) {
				hash_remove(cache->transaction_hash, previous);
				delete_transaction(cache, previous);
			}
		}
	}
	if (block->transaction == NULL && block->ref_count == 0) {
		// the block is no longer used
		block->unused = true;
		cache->unused_blocks.Add(block);
	}

	return B_OK;
}


#ifdef DEBUG_BLOCK_CACHE
static void
dump_block(cached_block *block)
{
	kprintf("%08lx %9Ld %08lx %08lx %08lx %5ld %6ld %c%c%c%c  %08lx "
		"%08lx\n", (addr_t)block, block->block_number,
		(addr_t)block->current_data, (addr_t)block->original_data,
		(addr_t)block->parent_data, block->ref_count, block->accessed,
		block->busy ? 'B' : '-', block->is_writing ? 'W' : '-',
		block->is_dirty ? 'B' : '-', block->unused ? 'U' : '-',
		(addr_t)block->transaction,
		(addr_t)block->previous_transaction);
}


static int
dump_cache(int argc, char **argv)
{
	bool showTransactions = false;
	bool showBlocks = false;
	int32 i = 1;
	while (argv[i] != NULL && argv[i][0] == '-') {
		for (char *arg = &argv[i][1]; arg[0]; arg++) {
			switch (arg[0]) {
				case 'b':
					showBlocks = true;
					break;
				case 't':
					showTransactions = true;
					break;
				default:
					print_debugger_command_usage(argv[0]);
					return 0;
			}
		}
		i++;
	}

	if (i >= argc) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	block_cache *cache = (struct block_cache *)parse_expression(argv[i]);
	if (cache == NULL) {
		kprintf("invalid cache address\n");
		return 0;
	}

	off_t blockNumber = -1;
	if (i + 1 < argc) {
		blockNumber = parse_expression(argv[i + 1]);
		cached_block *block = (cached_block *)hash_lookup(cache->hash,
			&blockNumber);
		if (block != NULL) {
			kprintf("BLOCK %p\n", block);
			kprintf(" current data:  %p\n", block->current_data);
			kprintf(" original data: %p\n", block->original_data);
			kprintf(" parent data:   %p\n", block->parent_data);
			kprintf(" ref_count:     %ld\n", block->ref_count);
			kprintf(" accessed:      %ld\n", block->accessed);
			kprintf(" flags:        ");
			if (block->is_writing)
				kprintf(" is-writing");
			if (block->is_dirty)
				kprintf(" is-dirty");
			if (block->unused)
				kprintf(" unused");
			kprintf("\n");
			if (block->transaction != NULL) {
				kprintf(" transaction:   %p (%ld)\n", block->transaction,
					block->transaction->id);
				if (block->transaction_next != NULL) {
					kprintf(" next in transaction: %Ld\n",
						block->transaction_next->block_number);
				}
			}
			if (block->previous_transaction != NULL) {
				kprintf(" previous transaction: %p (%ld)\n",
					block->previous_transaction,
					block->previous_transaction->id);
			}

			set_debug_variable("_current", (addr_t)block->current_data);
			set_debug_variable("_original", (addr_t)block->original_data);
			set_debug_variable("_parent", (addr_t)block->parent_data);
		} else
			kprintf("block %Ld not found\n", blockNumber);
		return 0;
	}

	kprintf("BLOCK CACHE: %p\n", cache);

	kprintf(" fd:         %d\n", cache->fd);
	kprintf(" max_blocks: %Ld\n", cache->max_blocks);
	kprintf(" block_size: %lu\n", cache->block_size);
	kprintf(" next_transaction_id: %ld\n", cache->next_transaction_id);

	if (showTransactions) {
		kprintf(" transactions:\n");
		kprintf("address       id state  blocks  main   sub\n");

		hash_iterator iterator;
		hash_open(cache->transaction_hash, &iterator);

		cache_transaction *transaction;
		while ((transaction = (cache_transaction *)hash_next(
				cache->transaction_hash, &iterator)) != NULL) {
			kprintf("%p %5ld %-7s %5ld %5ld %5ld\n", transaction,
				transaction->id, transaction->open ? "open" : "closed",
				transaction->num_blocks, transaction->main_num_blocks,
				transaction->sub_num_blocks);
		}
	}

	if (showBlocks) {
		kprintf(" blocks:\n");
		kprintf("address  block no. current  original parent    refs access "
			"flags transact prev. trans\n");
	}

	uint32 referenced = 0;
	uint32 count = 0;
	uint32 dirty = 0;
	hash_iterator iterator;
	hash_open(cache->hash, &iterator);
	cached_block *block;
	while ((block = (cached_block *)hash_next(cache->hash, &iterator)) != NULL) {
		if (showBlocks)
			dump_block(block);

		if (block->is_dirty)
			dirty++;
		if (block->ref_count)
			referenced++;
		count++;
	}

	kprintf(" %ld blocks total, %ld dirty, %ld referenced, %ld in unused.\n", count, dirty,
		referenced, cache->unused_blocks.Size());

	hash_close(cache->hash, &iterator, false);
	return 0;
}


static int
dump_transaction(int argc, char **argv)
{
	bool showBlocks = false;
	int i = 1;
	if (argc > 1 && !strcmp(argv[1], "-b")) {
		showBlocks = true;
		i++;
	}

	if (argc - i < 1 || argc - i > 2) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	cache_transaction *transaction = NULL;

	if (argc - i == 1) {
		transaction = (cache_transaction *)parse_expression(argv[i]);
	} else {
		block_cache *cache = (block_cache *)parse_expression(argv[i]);
		int32 id = parse_expression(argv[i + 1]);
		transaction = lookup_transaction(cache, id);
		if (transaction == NULL) {
			kprintf("No transaction with ID %ld found.\n", id);
			return 0;
		}
	}

	kprintf("TRANSACTION %p\n", transaction);

	kprintf(" id:             %ld\n", transaction->id);
	kprintf(" num block:      %ld\n", transaction->num_blocks);
	kprintf(" main num block: %ld\n", transaction->main_num_blocks);
	kprintf(" sub num block:  %ld\n", transaction->sub_num_blocks);
	kprintf(" has sub:        %d\n", transaction->has_sub_transaction);
	kprintf(" state:          %s\n", transaction->open ? "open" : "closed");
	kprintf(" idle:           %ld secs\n",
		(system_time() - transaction->last_used) / 1000000);

	if (!showBlocks)
		return 0;

	kprintf(" blocks:\n");
	kprintf("address  block no. current  original parent    refs access "
		"flags transact prev. trans\n");

	cached_block *block = transaction->first_block;
	while (block != NULL) {
		dump_block(block);
		block = block->transaction_next;
	}

	kprintf("--\n");

	block_list::Iterator iterator = transaction->blocks.GetIterator();
	while (iterator.HasNext()) {
		block = iterator.Next();
		dump_block(block);
	}

	return 0;
}


static int
dump_caches(int argc, char **argv)
{
	kprintf("Block caches:\n");
	DoublyLinkedList<block_cache>::Iterator i = sCaches.GetIterator();
	while (i.HasNext()) {
		block_cache *cache = i.Next();
		if (cache == (block_cache *)&sMarkCache)
			continue;

		kprintf("  %p\n", cache);
	}

	return 0;
}
#endif	// DEBUG_BLOCK_CACHE


static block_cache *
get_next_block_cache(block_cache *last)
{
	MutexLocker _(sCachesLock);
	block_cache *cache;
	if (last != NULL) {
		cache = sCaches.GetNext((block_cache *)&sMarkCache);
		sCaches.Remove((block_cache *)&sMarkCache);
	} else
		cache = sCaches.Head();

	if (cache != NULL)
		sCaches.Insert(sCaches.GetNext(cache), (block_cache *)&sMarkCache);

	return cache;
}


static status_t
block_writer(void *)
{
	while (true) {
		// write 64 blocks of each block_cache every two seconds
		// TODO: change this once we have an I/O scheduler
		snooze(2000000LL);

		block_cache *cache = NULL;
		while ((cache = get_next_block_cache(cache)) != NULL) {
			RecursiveLocker locker(&cache->lock);
			const uint32 kMaxCount = 64;
			cached_block *blocks[kMaxCount];
			uint32 count = 0;

			if (cache->num_dirty_blocks) {
				// This cache is not using transactions, we'll scan the blocks
				// directly
				hash_iterator iterator;
				hash_open(cache->hash, &iterator);

				cached_block *block;
				while (count < kMaxCount
					&& (block = (cached_block *)hash_next(cache->hash,
							&iterator)) != NULL) {
					if (block->is_dirty)
						blocks[count++] = block;
				}

				hash_close(cache->hash, &iterator, false);
			} else {
				hash_iterator iterator;
				hash_open(cache->transaction_hash, &iterator);

				cache_transaction *transaction;
				while ((transaction = (cache_transaction *)hash_next(
						cache->transaction_hash, &iterator)) != NULL
					&& count < kMaxCount) {
					if (transaction->open) {
						if (system_time() > transaction->last_used
								+ kTransactionIdleTime) {
							// transaction is open but idle
							notify_transaction_listeners(transaction,
								TRANSACTION_IDLE, false);
						}
						continue;
					}

					// sort blocks to speed up writing them back
					// TODO: ideally, this should be handled by the I/O scheduler
					block_list::Iterator iterator
						= transaction->blocks.GetIterator();

					for (; count < kMaxCount && iterator.HasNext(); count++) {
						blocks[count] = iterator.Next();
					}
				}

				hash_close(cache->transaction_hash, &iterator, false);
			}

			qsort(blocks, count, sizeof(void *), &compare_blocks);

			for (uint32 i = 0; i < count; i++) {
				if (write_cached_block(cache, blocks[i], true) != B_OK)
					break;
			}
		}
	}
}


extern "C" status_t
block_cache_init(void)
{
	sBlockCache = create_object_cache_etc("cached blocks", sizeof(cached_block),
		8, 0, CACHE_LARGE_SLAB, NULL, NULL, NULL, NULL);
	if (sBlockCache == NULL)
		return B_ERROR;

	mutex_init(&sCachesLock, "block caches");
	new (&sCaches) DoublyLinkedList<block_cache>;
		// manually call constructor

	thread_id thread = spawn_kernel_thread(&block_writer, "block writer",
		B_LOW_PRIORITY, NULL);
	if (thread >= B_OK)
		send_signal_etc(thread, SIGCONT, B_DO_NOT_RESCHEDULE);

#ifdef DEBUG_BLOCK_CACHE
	add_debugger_command_etc("block_caches", &dump_caches,
		"dumps all block caches", "\n", 0);
	add_debugger_command_etc("block_cache", &dump_cache,
		"dumps a specific block cache",
		"[-bt] <cache-address> [block-number]\n"
		"  -t lists the transactions\n"
		"  -b lists all blocks\n", 0);
	add_debugger_command_etc("transaction", &dump_transaction,
		"dumps a specific transaction", "[-b] ((<cache> <id>) | <transaction>)\n"
		"Either use a block cache pointer and an ID or a pointer to the transaction.\n"
		"  -b lists all blocks that are part of this transaction\n", 0);
#endif

	return B_OK;
}


//	#pragma mark - public transaction API


extern "C" int32
cache_start_transaction(void *_cache)
{
	block_cache *cache = (block_cache *)_cache;
	RecursiveLocker locker(&cache->lock);

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
	T(Action("start", cache, transaction));

	hash_insert_grow(cache->transaction_hash, transaction);

	return transaction->id;
}


extern "C" status_t
cache_sync_transaction(void *_cache, int32 id)
{
	block_cache *cache = (block_cache *)_cache;
	RecursiveLocker locker(&cache->lock);
	status_t status = B_ENTRY_NOT_FOUND;

	TRACE(("cache_sync_transaction(id %ld)\n", id));

	hash_iterator iterator;
	hash_open(cache->transaction_hash, &iterator);

	cache_transaction *transaction;
	while ((transaction = (cache_transaction *)hash_next(
			cache->transaction_hash, &iterator)) != NULL) {
		// close all earlier transactions which haven't been closed yet

		if (transaction->id <= id && !transaction->open) {
			// write back all of their remaining dirty blocks
			T(Action("sync", cache, transaction));
			while (transaction->num_blocks > 0) {
				// sort blocks to speed up writing them back
				// TODO: ideally, this should be handled by the I/O scheduler
				block_list::Iterator iterator = transaction->blocks.GetIterator();
				uint32 maxCount = transaction->num_blocks;
				cached_block *buffer[16];
				cached_block **blocks = (cached_block **)malloc(maxCount
					* sizeof(void *));
				if (blocks == NULL) {
					maxCount = 16;
					blocks = buffer;
				}

				uint32 count = 0;
				for (; count < maxCount && iterator.HasNext(); count++) {
					blocks[count] = iterator.Next();
				}
				qsort(blocks, count, sizeof(void *), &compare_blocks);

				for (uint32 i = 0; i < count; i++) {
					status = write_cached_block(cache, blocks[i], false);
					if (status != B_OK)
						break;
				}

				if (blocks != buffer)
					free(blocks);

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


extern "C" status_t
cache_end_transaction(void *_cache, int32 id,
	transaction_notification_hook hook, void *data)
{
	block_cache *cache = (block_cache *)_cache;
	RecursiveLocker locker(&cache->lock);

	TRACE(("cache_end_transaction(id = %ld)\n", id));

	cache_transaction *transaction = lookup_transaction(cache, id);
	if (transaction == NULL) {
		panic("cache_end_transaction(): invalid transaction ID\n");
		return B_BAD_VALUE;
	}

	T(Action("end", cache, transaction));

	transaction->notification_hook = hook;
	transaction->notification_data = data;

	notify_transaction_listeners(transaction, TRANSACTION_ENDED, true);

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


extern "C" status_t
cache_abort_transaction(void *_cache, int32 id)
{
	block_cache *cache = (block_cache *)_cache;
	RecursiveLocker locker(&cache->lock);

	TRACE(("cache_abort_transaction(id = %ld)\n", id));

	cache_transaction *transaction = lookup_transaction(cache, id);
	if (transaction == NULL) {
		panic("cache_abort_transaction(): invalid transaction ID\n");
		return B_BAD_VALUE;
	}

	T(Abort(cache, transaction));
	notify_transaction_listeners(transaction, TRANSACTION_ABORTED, true);

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


/*!	Acknowledges the current parent transaction, and starts a new transaction
	from its sub transaction.
	The new transaction also gets a new transaction ID.
*/
extern "C" int32
cache_detach_sub_transaction(void *_cache, int32 id,
	transaction_notification_hook hook, void *data)
{
	block_cache *cache = (block_cache *)_cache;
	RecursiveLocker locker(&cache->lock);

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
	T(Detach(cache, transaction, newTransaction));

	transaction->notification_hook = hook;
	transaction->notification_data = data;

	notify_transaction_listeners(transaction, TRANSACTION_ENDED, true);

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

	hash_insert_grow(cache->transaction_hash, newTransaction);
	cache->last_transaction = newTransaction;

	return newTransaction->id;
}


extern "C" status_t
cache_abort_sub_transaction(void *_cache, int32 id)
{
	block_cache *cache = (block_cache *)_cache;
	RecursiveLocker locker(&cache->lock);

	TRACE(("cache_abort_sub_transaction(id = %ld)\n", id));

	cache_transaction *transaction = lookup_transaction(cache, id);
	if (transaction == NULL) {
		panic("cache_abort_sub_transaction(): invalid transaction ID\n");
		return B_BAD_VALUE;
	}
	if (!transaction->has_sub_transaction)
		return B_BAD_VALUE;

	T(Abort(cache, transaction));
	notify_transaction_listeners(transaction, TRANSACTION_ABORTED, true);

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
	transaction->sub_num_blocks = 0;

	return B_OK;
}


extern "C" status_t
cache_start_sub_transaction(void *_cache, int32 id)
{
	block_cache *cache = (block_cache *)_cache;
	RecursiveLocker locker(&cache->lock);

	TRACE(("cache_start_sub_transaction(id = %ld)\n", id));

	cache_transaction *transaction = lookup_transaction(cache, id);
	if (transaction == NULL) {
		panic("cache_start_sub_transaction(): invalid transaction ID %ld\n", id);
		return B_BAD_VALUE;
	}

	notify_transaction_listeners(transaction, TRANSACTION_ENDED, true);

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
	T(Action("start-sub", cache, transaction));

	return B_OK;
}


/*!	Adds a transaction listener that gets notified when the transaction
	is ended or aborted.
	The listener gets automatically removed in this case.
*/
status_t
cache_add_transaction_listener(void *_cache, int32 id,
	transaction_notification_hook hookFunction, void *data)
{
	block_cache *cache = (block_cache *)_cache;

	cache_hook *hook = new(std::nothrow) cache_hook;
	if (hook == NULL)
		return B_NO_MEMORY;

	RecursiveLocker locker(&cache->lock);

	cache_transaction *transaction = lookup_transaction(cache, id);
	if (transaction == NULL) {
		delete hook;
		return B_BAD_VALUE;
	}

	hook->hook = hookFunction;
	hook->data = data;

	transaction->listeners.Add(hook);
	return B_OK;
}


status_t
cache_remove_transaction_listener(void *_cache, int32 id,
	transaction_notification_hook hookFunction, void *data)
{
	block_cache *cache = (block_cache *)_cache;

	RecursiveLocker locker(&cache->lock);

	cache_transaction *transaction = lookup_transaction(cache, id);
	if (transaction == NULL)
		return B_BAD_VALUE;

	HookList::Iterator iterator = transaction->listeners.GetIterator();
	while (iterator.HasNext()) {
		cache_hook *hook = iterator.Next();
		if (hook->data == data && hook->hook == hookFunction) {
			iterator.Remove();
			delete hook;
			return B_OK;
		}
	}

	return B_ENTRY_NOT_FOUND;
}


extern "C" status_t
cache_next_block_in_transaction(void *_cache, int32 id, bool mainOnly,
	long *_cookie, off_t *_blockNumber, void **_data, void **_unchangedData)
{
	cached_block *block = (cached_block *)*_cookie;
	block_cache *cache = (block_cache *)_cache;

	RecursiveLocker locker(&cache->lock);

	cache_transaction *transaction = lookup_transaction(cache, id);
	if (transaction == NULL || !transaction->open)
		return B_BAD_VALUE;

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
		return B_ENTRY_NOT_FOUND;

	if (_blockNumber)
		*_blockNumber = block->block_number;
	if (_data)
		*_data = mainOnly ? block->parent_data : block->current_data;
	if (_unchangedData)
		*_unchangedData = block->original_data;

	*_cookie = (addr_t)block;
	return B_OK;
}


extern "C" int32
cache_blocks_in_transaction(void *_cache, int32 id)
{
	block_cache *cache = (block_cache *)_cache;
	RecursiveLocker locker(&cache->lock);

	cache_transaction *transaction = lookup_transaction(cache, id);
	if (transaction == NULL)
		return B_BAD_VALUE;

	return transaction->num_blocks;
}


extern "C" int32
cache_blocks_in_main_transaction(void *_cache, int32 id)
{
	block_cache *cache = (block_cache *)_cache;
	RecursiveLocker locker(&cache->lock);

	cache_transaction *transaction = lookup_transaction(cache, id);
	if (transaction == NULL)
		return B_BAD_VALUE;

	return transaction->main_num_blocks;
}


extern "C" int32
cache_blocks_in_sub_transaction(void *_cache, int32 id)
{
	block_cache *cache = (block_cache *)_cache;
	RecursiveLocker locker(&cache->lock);

	cache_transaction *transaction = lookup_transaction(cache, id);
	if (transaction == NULL)
		return B_BAD_VALUE;

	return transaction->sub_num_blocks;
}


//	#pragma mark - public block cache API


extern "C" void
block_cache_delete(void *_cache, bool allowWrites)
{
	block_cache *cache = (block_cache *)_cache;

	if (allowWrites)
		block_cache_sync(cache);

	RecursiveLocker locker(&cache->lock);

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


extern "C" void *
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


extern "C" status_t
block_cache_sync(void *_cache)
{
	block_cache *cache = (block_cache *)_cache;

	// we will sync all dirty blocks to disk that have a completed
	// transaction or no transaction only

	RecursiveLocker locker(&cache->lock);
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
block_cache_sync_etc(void *_cache, off_t blockNumber, size_t numBlocks)
{
	block_cache *cache = (block_cache *)_cache;

	// we will sync all dirty blocks to disk that have a completed
	// transaction or no transaction only

	if (blockNumber < 0 || blockNumber >= cache->max_blocks) {
		panic("block_cache_sync_etc: invalid block number %Ld (max %Ld)",
			blockNumber, cache->max_blocks - 1);
		return B_BAD_VALUE;
	}

	RecursiveLocker locker(&cache->lock);

	for (; numBlocks > 0; numBlocks--, blockNumber++) {
		cached_block *block = (cached_block *)hash_lookup(cache->hash,
			&blockNumber);
		if (block == NULL)
			continue;

		// TODO: sort blocks!

		if (block->previous_transaction != NULL
			|| (block->transaction == NULL && block->is_dirty)) {
			status_t status = write_cached_block(cache, block);
			if (status != B_OK)
				return status;
		}
	}

	return B_OK;
}


extern "C" status_t
block_cache_make_writable(void *_cache, off_t blockNumber, int32 transaction)
{
	block_cache *cache = (block_cache *)_cache;
	RecursiveLocker locker(&cache->lock);

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


extern "C" void *
block_cache_get_writable_etc(void *_cache, off_t blockNumber, off_t base,
	off_t length, int32 transaction)
{
	block_cache *cache = (block_cache *)_cache;
	RecursiveLocker locker(&cache->lock);

	TRACE(("block_cache_get_writable_etc(block = %Ld, transaction = %ld)\n",
		blockNumber, transaction));
	if (cache->read_only)
		panic("tried to get writable block on a read-only cache!");

	return get_writable_cached_block(cache, blockNumber, base, length,
		transaction, false);
}


extern "C" void *
block_cache_get_writable(void *_cache, off_t blockNumber, int32 transaction)
{
	return block_cache_get_writable_etc(_cache, blockNumber,
		blockNumber, 1, transaction);
}


extern "C" void *
block_cache_get_empty(void *_cache, off_t blockNumber, int32 transaction)
{
	block_cache *cache = (block_cache *)_cache;
	RecursiveLocker locker(&cache->lock);

	TRACE(("block_cache_get_empty(block = %Ld, transaction = %ld)\n",
		blockNumber, transaction));
	if (cache->read_only)
		panic("tried to get empty writable block on a read-only cache!");

	return get_writable_cached_block((block_cache *)_cache, blockNumber,
		blockNumber, 1, transaction, true);
}


extern "C" const void *
block_cache_get_etc(void *_cache, off_t blockNumber, off_t base, off_t length)
{
	block_cache *cache = (block_cache *)_cache;
	RecursiveLocker locker(&cache->lock);
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


extern "C" const void *
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
extern "C" status_t
block_cache_set_dirty(void *_cache, off_t blockNumber, bool dirty,
	int32 transaction)
{
	block_cache *cache = (block_cache *)_cache;
	RecursiveLocker locker(&cache->lock);

	cached_block *block = (cached_block *)hash_lookup(cache->hash,
		&blockNumber);
	if (block == NULL)
		return B_BAD_VALUE;
	if (block->is_dirty == dirty) {
		// there is nothing to do for us
		return B_OK;
	}

	// TODO: not yet implemented
	if (dirty)
		panic("block_cache_set_dirty(): not yet implemented that way!\n");

	return B_OK;
}


extern "C" void
block_cache_put(void *_cache, off_t blockNumber)
{
	block_cache *cache = (block_cache *)_cache;
	RecursiveLocker locker(&cache->lock);

	put_cached_block(cache, blockNumber);
}

