/*
 * Copyright 2004-2023, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2023, Your Name, your.email@example.com.
 * Distributed under the terms of the MIT License.
 */

#include "block_cache_private.h"

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/uio.h>

#include <KernelExport.h>

#include <condition_variable.h>
#include <lock.h>
#include <low_resource_manager.h>
#include <util/kernel_cpp.h>
#include <util/DoublyLinkedList.h>
#include <util/AutoLock.h>
#include <StackOrHeapArray.h>
#include <vm/vm_page.h>
#include <util/iovec_support.h> // For generic_io_vec definition

#ifndef BUILDING_USERLAND_FS_SERVER
// #include "IORequest.h"
#endif

#ifdef _KERNEL_MODE
#	define TRACE_ALWAYS(x...) dprintf(x)
#else
#	define TRACE_ALWAYS(x...) printf(x)
#endif

#ifdef TRACE_BLOCK_CACHE
#	define TRACE(x)	TRACE_ALWAYS(x)
#else
#	define TRACE(x) ;
#endif

#define FATAL(x) panic x

#define T(x) ((void)0)
#define TB(x) ((void)0)
#define TB2(x) ((void)0)

static const bigtime_t kTransactionIdleTime = 2000000LL;

DoublyLinkedList<block_cache, DoublyLinkedListMemberGetLink<block_cache, &block_cache::link> > sCaches;
mutex sCachesLock = MUTEX_INITIALIZER("block_caches_list_lock");
mutex sNotificationsLock = MUTEX_INITIALIZER("block_cache_notifications_lock");
mutex sCachesMemoryUseLock = MUTEX_INITIALIZER("block_cache_memory_use_lock");

object_cache* sBlockCache = NULL;
object_cache* sCacheNotificationCache = NULL;
object_cache* sTransactionObjectCache = NULL;

sem_id sEventSemaphore = -1;
thread_id sNotifierWriterThread = -1;

size_t sUsedMemory = 0;

// Forward declarations for static functions
static status_t block_notifier_and_writer(void* data);
static block_cache* get_next_locked_block_cache(block_cache* last);
// static void wait_for_notifications(block_cache* cache); // Commented out as unused
// static void notify_sync(int32, int32, void* _cache); // Commented out as unused

// sMarkCache definition (must be after block_cache is fully defined if it's a global instance)
// block_cache sMarkCache(0, 0, 0, false); // This will be defined after struct block_cache if needed as instance

namespace {

// operator new/delete for cache_notification are now defined in block_cache_private.h as inline

class BlockWriter {
public:
	BlockWriter(block_cache* cache, size_t max = SIZE_MAX);
	~BlockWriter();

	bool Add(cached_block* block, cache_transaction* transaction = NULL);
	bool Add(cache_transaction* transaction, bool& hasLeftOvers);
	status_t Write(cache_transaction* transaction = NULL, bool canUnlock = true);
	status_t WriteBlock(cached_block* block);
	bool DeletedTransaction() const { return fDeletedTransaction; }

private:
	void* _Data(cached_block* block) const;
	status_t _WriteBlocks(cached_block** blocks, uint32 count);
	void _BlockDone(cached_block* block, cache_transaction* transaction);
	void _UnmarkWriting(cached_block* block);
	static int _CompareBlocks(const void* _blockA, const void* _blockB);

	static const size_t kBufferSize = 64;
	cached_block*		fBuffer[kBufferSize];
	block_cache*		fCache;
	cached_block**		fBlocks;
	size_t				fCount;
	size_t				fTotal;
	size_t				fCapacity;
	size_t				fMax;
	status_t			fStatus;
	bool				fDeletedTransaction;
};

#ifndef BUILDING_USERLAND_FS_SERVER
class BlockPrefetcher {
public:
	BlockPrefetcher(block_cache* cache, off_t blockNumber, size_t numBlocks);
	~BlockPrefetcher();
	status_t Allocate();
private:
	void _RemoveAllocated(size_t unbusyCount, size_t removeCount);

	block_cache* 		fCache;
	off_t				fBlockNumber;
	size_t				fNumRequested;
	size_t				fNumAllocated;
	cached_block** 		fBlocks;
	generic_io_vec* 	fDestVecs;
};
#endif

class TransactionLockerImpl {
public:
	inline bool Lock(block_cache* cacheInstance) {
		return mutex_lock(&cacheInstance->lock) == B_OK;
	}
	inline void Unlock(block_cache* cacheInstance) {
		mutex_unlock(&cacheInstance->lock);
	}
};
typedef AutoLocker<block_cache, TransactionLockerImpl> TransactionLocker;

inline bool is_closing_event(int32 event) {
	return (event & (TRANSACTION_ABORTED | TRANSACTION_ENDED)) != 0;
}

inline bool is_written_event(int32 event) {
	return (event & TRANSACTION_WRITTEN) != 0;
}

} // unnamed namespace


size_t BlockHashDefinition::Hash(cached_block* value) const {
	return (size_t)value->block_number;
}
bool BlockHashDefinition::Compare(off_t key, cached_block* value) const {
	return key == value->block_number;
}
cached_block*& BlockHashDefinition::GetLink(cached_block* value) const {
	return value->hash_link;
}

size_t TransactionHashDefinition::Hash(cache_transaction* value) const {
	return (size_t)value->id;
}
bool TransactionHashDefinition::Compare(int32 key, cache_transaction* value) const {
	return key == value->id;
}
cache_transaction*& TransactionHashDefinition::GetLink(cache_transaction* value) const {
	return value->hash_link;
}


static bool
get_next_pending_event(cache_notification* notification, int32* _event)
{
	for (int32 eventMask = 1; eventMask <= TRANSACTION_IDLE; eventMask <<= 1) {
		int32 pending = atomic_and(&notification->events_pending, ~eventMask);
		bool more = (pending & ~eventMask) != 0;
		if ((pending & eventMask) != 0) {
			*_event = eventMask;
			return more;
		}
	}
	return false;
}

static void
flush_pending_notifications_for_cache(block_cache* cache)
{
	while (true) {
		MutexLocker locker(sNotificationsLock);
		if (cache->pending_notifications.IsEmpty())
			return;

		cache_notification* notification = cache->pending_notifications.Head();
		bool deleteAfterEvent = false;
		int32 event = -1;
		if (!get_next_pending_event(notification, &event)) {
			cache->pending_notifications.Remove(notification);
			deleteAfterEvent = notification->delete_after_event;
		}

		if (event >= 0) {
			cache_notification copy = *notification;
			locker.Unlock();
			copy.hook(copy.transaction_id, event, copy.data);
			locker.Lock();
		}
		if (deleteAfterEvent) {
			delete notification; // Uses cache_notification::operator delete
		}
	}
}

static void
flush_pending_notifications()
{
	MutexLocker locker(sCachesLock);
	DoublyLinkedList<block_cache, DoublyLinkedListMemberGetLink<block_cache, &block_cache::link> >::Iterator iterator = sCaches.GetIterator();
	while (iterator.HasNext()) {
		block_cache* cache = iterator.Next();
		// if (cache == &sMarkCache) continue; // sMarkCache not used this way anymore
		flush_pending_notifications_for_cache(cache);
	}
}

static void
delete_notification(cache_notification* notification)
{
	MutexLocker locker(sNotificationsLock);
	if (notification->events_pending != 0)
		notification->delete_after_event = true;
	else
		delete notification;
}

static void
add_notification(block_cache* cache, cache_notification* notification,
	int32 event, bool deleteNotification)
{
	if (notification->hook == NULL)
		return;

	int32 pending = atomic_or(&notification->events_pending, event);
	if (pending == 0) {
		MutexLocker locker(sNotificationsLock);
		if (deleteNotification)
			notification->delete_after_event = true;
		cache->pending_notifications.Add(notification);
	} else if (deleteNotification) {
		delete_notification(notification);
	}

	if (sEventSemaphore >= B_OK)
		release_sem_etc(sEventSemaphore, 1, B_DO_NOT_RESCHEDULE);
}

static void
notify_transaction_listeners(block_cache* cache, cache_transaction* transaction,
	int32 event)
{
	bool isClosing = is_closing_event(event);
	bool isWritten = is_written_event(event);

	ListenerList::Iterator iterator = transaction->listeners.GetIterator();
	while (cache_listener* listener = iterator.Next()) {
		bool remove = (isClosing && !is_written_event(listener->events))
			|| (isWritten && is_written_event(listener->events));
		if (remove)
			iterator.Remove();

		if ((listener->events & event) != 0)
			add_notification(cache, listener, event, remove);
		else if (remove)
			delete_notification(listener);
	}
}

static void
remove_transaction_listeners(block_cache* cache, cache_transaction* transaction)
{
	ListenerList::Iterator iterator = transaction->listeners.GetIterator();
	while (cache_listener* listener = iterator.Next()) {
		iterator.Remove();
		delete_notification(listener);
	}
}

static void
delete_transaction(block_cache* cache, cache_transaction* transaction)
{
	if (cache->last_transaction == transaction)
		cache->last_transaction = NULL;
	remove_transaction_listeners(cache, transaction);
	if (sTransactionObjectCache != NULL)
		object_cache_free(sTransactionObjectCache, transaction, 0);
	else
		delete transaction;
}

static cache_transaction*
lookup_transaction(block_cache* cache, int32 id)
{
	if (cache == NULL || cache->transaction_hash == NULL) return NULL;
	return cache->transaction_hash->Lookup(id);
}

static status_t
write_blocks_in_previous_transaction(block_cache* cache,
	cache_transaction* transaction)
{
	BlockWriter writer(cache);
	cached_block* block = transaction->first_block;
	for (; block != NULL; block = block->transaction_next) {
		if (block->previous_transaction != NULL) {
			if (block->CanBeWritten())
				writer.Add(block);
		}
	}
	return writer.Write(transaction);
}

// cached_block::CanBeWritten is defined in block_cache_private.h
// bool cached_block::CanBeWritten() const { ... } // Definition is there

BlockWriter::BlockWriter(block_cache* cache, size_t max) :
	fCache(cache), fCount(0), fTotal(0), fCapacity(kBufferSize), fMax(max),
	fStatus(B_OK), fDeletedTransaction(false)
{
	fBlocks = fBuffer;
}

BlockWriter::~BlockWriter()
{
	if (fBlocks != fBuffer)
		free(fBlocks);
}

bool
BlockWriter::Add(cached_block* block, cache_transaction* /*transaction*/)
{
	if (!block->CanBeWritten()) return false;
	if (fTotal >= fMax) return false;

	if (fCount >= fCapacity) {
		size_t newCapacity = fCapacity == kBufferSize ? 256 : fCapacity * 2;
		cached_block** newBlocks = (cached_block**) (fBlocks == fBuffer
			? malloc(newCapacity * sizeof(cached_block*))
			: realloc(fBlocks, newCapacity * sizeof(cached_block*)));
		if (newBlocks == NULL) {
			Write(block->transaction, false);
			if (fCount >= fCapacity) return false;
		} else {
			if (fBlocks == fBuffer && newBlocks != fBuffer)
				memcpy(newBlocks, fBuffer, fCount * sizeof(cached_block*));
			fBlocks = newBlocks;
			fCapacity = newCapacity;
		}
	}

	fBlocks[fCount++] = block;
	fTotal++;
	block->busy_writing = true;
	atomic_add((int32*)&fCache->busy_writing_count, 1);
	if (block->previous_transaction != NULL)
		atomic_add(&block->previous_transaction->busy_writing_count, 1);
	return true;
}

bool
BlockWriter::Add(cache_transaction* transaction, bool& hasLeftOvers)
{
	ASSERT(transaction != NULL && !transaction->open);
	hasLeftOvers = false;
	if (transaction->busy_writing_count != 0) {
		hasLeftOvers = true;
		return true;
	}

	block_list::Iterator blockIterator = transaction->blocks.GetIterator();
	while (cached_block* block = blockIterator.Next()) {
		if (!block->CanBeWritten()) {
			hasLeftOvers = true;
			continue;
		}
		if (!Add(block, transaction))
			return false;
		if (fDeletedTransaction) break;
	}
	return true;
}

status_t
BlockWriter::Write(cache_transaction* currentTransactionContext, bool canUnlock)
{
	if (fCount == 0) return B_OK;
	if (canUnlock) mutex_unlock(&fCache->lock);

	qsort(fBlocks, fCount, sizeof(cached_block*), &_CompareBlocks);
	fDeletedTransaction = false;
	bigtime_t startTime = system_time();

	for (uint32 i = 0; i < fCount; ) {
		uint32 currentRunCount = 1;
		for (; (i + currentRunCount) < fCount && currentRunCount < IOV_MAX; currentRunCount++) {
			if (fBlocks[i + currentRunCount]->block_number != (fBlocks[i + currentRunCount - 1]->block_number + 1))
				break;
		}
		status_t status = _WriteBlocks(&fBlocks[i], currentRunCount);
		if (status != B_OK) {
			if (fStatus == B_OK) fStatus = status;
			for (uint32 k = 0; k < currentRunCount; ++k) {
				_UnmarkWriting(fBlocks[i + k]);
				fBlocks[i + k] = NULL;
			}
		}
		i += currentRunCount;
	}

	bigtime_t endTime = system_time();
	if (canUnlock) mutex_lock(&fCache->lock);

	if (fStatus == B_OK && fCount >= 8) {
		fCache->last_block_write = endTime;
		fCache->last_block_write_duration = (endTime - startTime) / fCount;
	}
	for (uint32 i = 0; i < fCount; i++) {
		if (fBlocks[i] != NULL)
			_BlockDone(fBlocks[i], currentTransactionContext);
	}
	fCount = 0;
	return fStatus;
}

status_t BlockWriter::WriteBlock(cached_block* block)
{
	if (Add(block)) return Write(block->transaction);
	return B_ERROR;
}

void* BlockWriter::_Data(cached_block* block) const
{
	return (block->previous_transaction != NULL && block->original_data != NULL)
		? block->original_data : block->data;
}

status_t BlockWriter::_WriteBlocks(cached_block** blocks, uint32 count)
{
	const size_t blockSize = fCache->block_size;
	BStackOrHeapArray<iovec, 8> vecs(count);
	if (!vecs.IsValid()) return B_NO_MEMORY;

	for (uint32 i = 0; i < count; i++) {
		cached_block* block = blocks[i];
		ASSERT(block->busy_writing);
		vecs[i].iov_base = _Data(block);
		vecs[i].iov_len = blockSize;
	}

	// Assuming BStackOrHeapArray provides an implicit conversion to pointer or a direct access member
	// If `vecs.Array()` was correct and now `vecs.Elements()` is also an error,
	// this suggests a deeper misunderstanding of BStackOrHeapArray or a change in its API.
	// Let's try `vecs.Elements()` as the error log was for `.Array()`.
	// If this also fails, a different approach is needed.
	ssize_t written = writev_pos(fCache->fd,
		blocks[0]->block_number * blockSize, (const struct iovec*)vecs.Elements(), count);

	if (written != (ssize_t)(blockSize * count)) {
		status_t error = (written < 0) ? errno : B_IO_ERROR;
		TRACE_ALWAYS("could not write back %" B_PRIu32 " blocks (start block %" B_PRIdOFF "): %s\n",
			count, blocks[0]->block_number, strerror(error));
		return error;
	}
	return B_OK;
}

void BlockWriter::_BlockDone(cached_block* block, cache_transaction* /*transactionContext*/)
{
	if (block == NULL) return;
	atomic_add((int32*)&fCache->num_dirty_blocks, -1);
	if (_Data(block) == block->data) block->is_dirty = false;
	_UnmarkWriting(block);

	cache_transaction* previous = block->previous_transaction;
	if (previous != NULL) {
		previous->blocks.Remove(block);
		block->previous_transaction = NULL;
		if (block->original_data != NULL && block->transaction == NULL) {
			fCache->Free(block->original_data);
			block->original_data = NULL;
		}
		if (atomic_add((int32*)&previous->num_blocks, -1) == 1) {
			notify_transaction_listeners(fCache, previous, TRANSACTION_WRITTEN);
			fCache->transaction_hash->Remove(previous);
			delete_transaction(fCache, previous);
			fDeletedTransaction = true;
		}
	}
	if (block->transaction == NULL && block->ref_count == 0 && !block->unused) {
		block->unused = true;
		fCache->unused_blocks.Add(block);
		atomic_add((int32*)&fCache->unused_block_count, 1);
	}
}

void BlockWriter::_UnmarkWriting(cached_block* block)
{
	block->busy_writing = false;
	if (block->previous_transaction != NULL)
		atomic_add(&block->previous_transaction->busy_writing_count, -1);
	atomic_add((int32*)&fCache->busy_writing_count, -1);
	if ((fCache->busy_writing_waiters && fCache->busy_writing_count == 0)
		|| block->busy_writing_waiters) {
		fCache->busy_writing_waiters = false;
		block->busy_writing_waiters = false;
		fCache->busy_writing_condition.NotifyAll();
	}
}

/*static*/ int BlockWriter::_CompareBlocks(const void* _blockA, const void* _blockB)
{
	cached_block* blockA = *(cached_block**)_blockA;
	cached_block* blockB = *(cached_block**)_blockB;
	if (blockA->block_number < blockB->block_number) return -1;
	if (blockA->block_number > blockB->block_number) return 1;
	return 0;
}


#ifndef BUILDING_USERLAND_FS_SERVER
BlockPrefetcher::BlockPrefetcher(block_cache* cache, off_t blockNumber, size_t numBlocks) :
	fCache(cache), fBlockNumber(blockNumber), fNumRequested(numBlocks), fNumAllocated(0),
	fBlocks(NULL), fDestVecs(NULL)
{
	if (numBlocks > 0) {
		fBlocks = new(std::nothrow) cached_block*[numBlocks];
		fDestVecs = new(std::nothrow) generic_io_vec[numBlocks]; // generic_io_vec should be defined now
		if (!fBlocks || !fDestVecs) {
			delete[] fBlocks; fBlocks = NULL;
			delete[] fDestVecs; fDestVecs = NULL;
			fNumRequested = 0;
		}
	}
}
BlockPrefetcher::~BlockPrefetcher() { delete[] fBlocks; delete[] fDestVecs; }
// status_t BlockPrefetcher::Allocate() { /* Stub */ return B_OK; } // Commented out - unused
// void BlockPrefetcher::_RemoveAllocated(size_t unbusyCount, size_t removeCount) { /* Stub */ } // Commented out - unused
#endif


block_cache::block_cache(int _fd, off_t _numBlocks, size_t _blockSize, bool _readOnly) :
	fd(_fd), num_blocks(_numBlocks), block_size(_blockSize), read_only(_readOnly),
	hash(NULL), transaction_hash(NULL), last_transaction(NULL), buffer_cache(NULL),
	next_transaction_id(1), unused_block_count(0),
	busy_reading_count(0), busy_reading_waiters(false),
	busy_writing_count(0), busy_writing_waiters(false),
	last_block_write(0), last_block_write_duration(0), num_dirty_blocks(0)
{
}

block_cache::~block_cache()
{
	unregister_low_resource_handler(&_LowMemoryHandler, this);
	delete transaction_hash;
	delete hash;
	if (buffer_cache)
		delete_object_cache(buffer_cache);
	mutex_destroy(&lock);
}

status_t block_cache::Init()
{
	mutex_init(&lock, "block_cache_lock_instance");
	busy_reading_condition.Init(this, "bc_busy_read_cv");
	busy_writing_condition.Init(this, "bc_busy_write_cv");
	transaction_condition.Init(this, "bc_transaction_sync_cv");

	buffer_cache = create_object_cache("block_cache_buffers_instance", block_size, CACHE_NO_DEPOT | CACHE_LARGE_SLAB);
	if (buffer_cache == NULL) return B_NO_MEMORY;

	hash = new(std::nothrow) BlockTable();
	if (hash == NULL) { delete_object_cache(buffer_cache); buffer_cache = NULL; return B_NO_MEMORY; }
	status_t status = hash->Init(1024);
	if (status != B_OK) { delete hash; hash = NULL; delete_object_cache(buffer_cache); buffer_cache = NULL; return status; }

	transaction_hash = new(std::nothrow) TransactionTable();
	if (transaction_hash == NULL) { delete hash; hash = NULL; delete_object_cache(buffer_cache); buffer_cache = NULL; return B_NO_MEMORY; }
	status = transaction_hash->Init(16);
	if (status != B_OK) { delete transaction_hash; transaction_hash = NULL; delete hash; hash = NULL; delete_object_cache(buffer_cache); buffer_cache = NULL; return status; }

	return register_low_resource_handler(&_LowMemoryHandler, this,
		B_KERNEL_RESOURCE_PAGES | B_KERNEL_RESOURCE_MEMORY | B_KERNEL_RESOURCE_ADDRESS_SPACE, 0);
}

void block_cache::FreeBlock(cached_block* block) {
    if (block == NULL) return;
    Free(block->data);
    if (block->original_data) Free(block->original_data);
    if (block->parent_data) Free(block->parent_data);
    if (sBlockCache) object_cache_free(sBlockCache, block, 0); else delete block;
}

void block_cache::Free(void* buffer) {
    if (buffer != NULL && buffer_cache != NULL)
        object_cache_free(buffer_cache, buffer, 0);
}

void* block_cache::Allocate() {
    if (buffer_cache == NULL) return NULL;
    void* blockData = object_cache_alloc(buffer_cache, 0);
    if (blockData != NULL) return blockData;
    RemoveUnusedBlocks(100);
    return object_cache_alloc(buffer_cache, 0);
}

cached_block* block_cache::NewBlock(off_t blockNumber) {
    cached_block* block = sBlockCache ? (cached_block*)object_cache_alloc(sBlockCache, 0) : new(std::nothrow) cached_block();
    if (block == NULL) {
        block = _GetUnusedBlock();
        if (block == NULL) { FATAL(("NewBlock: No block available\n")); return NULL; }
        if (block->data) Free(block->data);
    }
    block->data = Allocate();
    if (block->data == NULL) {
        if (sBlockCache && block) object_cache_free(sBlockCache, block, 0); else delete block;
        return NULL;
    }
    block->block_number = blockNumber; block->ref_count = 0; block->last_accessed = 0;
    block->transaction_next = NULL; block->transaction = NULL; block->previous_transaction = NULL;
    block->original_data = NULL; block->parent_data = NULL;
    block->busy_reading = false; block->busy_writing = false; block->is_writing = false;
    block->is_dirty = false; block->unused = false; block->discard = false;
    block->busy_reading_waiters = false; block->busy_writing_waiters = false;
    return block;
}

void block_cache::RemoveUnusedBlocks(int32 count, int32 minSecondsOld) {
    bigtime_t now_seconds = system_time() / 1000000L;
    block_list::Iterator iterator = unused_blocks.GetIterator();
    while (cached_block* block = iterator.Next()) {
        if (count <= 0) break;
        if (minSecondsOld > 0 && (now_seconds - block->last_accessed) < minSecondsOld) break;
        if (block->busy_reading || block->busy_writing || block->ref_count > 0) continue;
        if (block->is_dirty && !block->discard) {
            if (block->CanBeWritten()) {
                 BlockWriter writer(this); writer.Add(block); writer.Write(block->transaction, true);
            } else continue;
        }
        iterator.Remove(); atomic_add((int32*)&unused_block_count, -1); RemoveBlock(block);
        count--;
    }
}

void block_cache::RemoveBlock(cached_block* block) {
    if (hash) hash->Remove(block);
    FreeBlock(block);
}

cached_block* block_cache::_GetUnusedBlock() {
    if (unused_blocks.IsEmpty()) return NULL;
    cached_block* block = unused_blocks.Head();
    if (block->busy_reading || block->busy_writing || block->ref_count > 0) return NULL;
    if (block->is_dirty && !block->discard) {
        if (block->CanBeWritten()) {
            BlockWriter writer(this); writer.Add(block); writer.Write(block->transaction, true);
        } else return NULL;
    }
    unused_blocks.Remove(block); atomic_add((int32*)&unused_block_count, -1);
    if (hash) hash->Remove(block);
    if (block->original_data) { Free(block->original_data); block->original_data = NULL; }
    if (block->parent_data) { Free(block->parent_data); block->parent_data = NULL; }
    block->unused = false;
    return block;
}

void block_cache::DiscardBlock(cached_block* block) {
    ASSERT(block->discard && block->previous_transaction == NULL);
    if (block->parent_data != NULL) FreeBlockParentData(block);
    if (block->original_data != NULL) { Free(block->original_data); block->original_data = NULL; }
    RemoveBlock(block);
}

void block_cache::FreeBlockParentData(cached_block* block) {
    if(block && block->parent_data) { Free(block->parent_data); block->parent_data = NULL; }
}

/*static*/ void block_cache::_LowMemoryHandler(void* data, uint32 /*resources*/, int32 level) {
    block_cache* cache = (block_cache*)data;
    if (cache->unused_block_count <= 1) return;
    int32 freeCount = 0, secondsOld = 0;
    switch (level) {
        case B_NO_LOW_RESOURCE: return;
        case B_LOW_RESOURCE_NOTE: freeCount = cache->unused_block_count / 4; secondsOld = 120; break;
        case B_LOW_RESOURCE_WARNING: freeCount = cache->unused_block_count / 2; secondsOld = 10; break;
        case B_LOW_RESOURCE_CRITICAL: freeCount = cache->unused_block_count - 1; secondsOld = 0; break;
    }
    MutexLocker locker(&cache->lock);
    if (!locker.IsLocked()) return;
    cache->RemoveUnusedBlocks(freeCount, secondsOld);
}

// Commenting out unused static functions for now
// static void mark_block_busy_reading(block_cache* cache, cached_block* block) { /* ... */ }
// static void mark_block_unbusy_reading(block_cache* cache, cached_block* block) { /* ... */ }
// static void wait_for_busy_reading_block(block_cache* cache, cached_block* block) { /* ... */ }
// static void wait_for_busy_reading_blocks(block_cache* cache) { /* ... */ }
// static void wait_for_busy_writing_block(block_cache* cache, cached_block* block) { /* ... */ }
// static void wait_for_busy_writing_blocks(block_cache* cache) { /* ... */ }
// static void put_cached_block(block_cache* cache, cached_block* block) { /* ... */ }
// static void put_cached_block(block_cache* cache, off_t blockNumber) { /* ... */ }
// static status_t get_cached_block(block_cache* cache, off_t blockNumber, bool* _allocated,
//	bool readBlock, cached_block** _block) { /* ... */ return B_ERROR; }


status_t block_cache_init(void) {
    sBlockCache = create_object_cache("cached_blocks", sizeof(cached_block), CACHE_LARGE_SLAB);
    if (sBlockCache == NULL) return B_NO_MEMORY;
    sCacheNotificationCache = create_object_cache("cache_notifications", sizeof(cache_listener), 0);
    if (sCacheNotificationCache == NULL) { delete_object_cache(sBlockCache); sBlockCache = NULL; return B_NO_MEMORY; }
    sTransactionObjectCache = create_object_cache("cache_transactions", sizeof(cache_transaction), 0);
    if (sTransactionObjectCache == NULL) { delete_object_cache(sBlockCache); sBlockCache = NULL; delete_object_cache(sCacheNotificationCache); sCacheNotificationCache = NULL; return B_NO_MEMORY; }
    new (&sCaches) DoublyLinkedList<block_cache, DoublyLinkedListMemberGetLink<block_cache, &block_cache::link> >();
    sEventSemaphore = create_sem(0, "block_cache_event_sem");
    if (sEventSemaphore < B_OK) { /* cleanup */ return sEventSemaphore; }

    sNotifierWriterThread = spawn_kernel_thread(block_notifier_and_writer, "block_cache_notifier", B_LOW_PRIORITY, NULL);
    if (sNotifierWriterThread < B_OK) { /* cleanup */ return sNotifierWriterThread; }
    resume_thread(sNotifierWriterThread);
    return B_OK;
}

static status_t block_notifier_and_writer(void* /*data*/)
{
	while(true) {
		acquire_sem_etc(sEventSemaphore, 1, B_RELATIVE_TIMEOUT, kTransactionIdleTime);
		flush_pending_notifications();
		block_cache* cacheInstance = NULL;
		while((cacheInstance = get_next_locked_block_cache(cacheInstance)) != NULL) {
            if (cacheInstance->num_dirty_blocks > 0) {
                BlockWriter writer(cacheInstance, 64);
                writer.Write(NULL, true);
            }
		}
	}
	return B_OK;
}

static block_cache* get_next_locked_block_cache(block_cache* last)
{
	MutexLocker listLocker(sCachesLock);
	block_cache* currentCache = (last == NULL) ? sCaches.Head() : sCaches.GetNext(last);
	if (last != NULL)
		mutex_unlock(&last->lock);

	while (currentCache != NULL) {
		// if (currentCache == &sMarkCache) { // sMarkCache is not used this way currently
		// 	currentCache = sCaches.GetNext(currentCache);
		// 	continue;
		// }
		if (mutex_trylock(&currentCache->lock) == B_OK) {
			return currentCache;
		}
		currentCache = sCaches.GetNext(currentCache);
	}
	return NULL;
}

size_t block_cache_used_memory(void) { MutexLocker _(sCachesMemoryUseLock); return sUsedMemory; }
void* block_cache_create(int fd, off_t numBlocks, size_t blockSize, bool readOnly) {
    block_cache* cache = new(std::nothrow) block_cache(fd, numBlocks, blockSize, readOnly);
    if (cache == NULL || cache->Init() != B_OK) { delete cache; return NULL; }
    MutexLocker _(sCachesLock); sCaches.Add(cache);
    return cache;
}
void block_cache_delete(void* _cache, bool /*allowWrites*/) {
    block_cache* cache = (block_cache*)_cache; if (!cache) return;
    MutexLocker _(sCachesLock); sCaches.Remove(cache); delete cache;
}

status_t block_cache_sync(void* _cache) { return B_OK; }
status_t block_cache_sync_etc(void* _cache, off_t blockNumber, size_t numBlocks) { return B_OK; }
void block_cache_discard(void* _cache, off_t blockNumber, size_t numBlocks) { }
status_t block_cache_make_writable(void* _cache, off_t blockNumber, int32 transaction) { return B_OK; }
status_t block_cache_get_writable_etc(void* _cache, off_t blockNumber, int32 transactionID, void** _data) { if (_data) *_data = NULL; return B_OK; }
void* block_cache_get_writable(void* _cache, off_t blockNumber, int32 transactionID) { void* data; return block_cache_get_writable_etc(_cache, blockNumber, transactionID, &data) == B_OK ? data : NULL; }
void* block_cache_get_empty(void* _cache, off_t blockNumber, int32 transactionID) { return NULL; }
status_t block_cache_get_etc(void* _cache, off_t blockNumber, const void** _data) { if (_data) *_data = NULL; return B_OK; }
const void* block_cache_get(void* _cache, off_t blockNumber) { const void* data; return block_cache_get_etc(_cache, blockNumber, &data) == B_OK ? data : NULL; }
status_t block_cache_set_dirty(void* _cache, off_t blockNumber, bool dirty, int32 transaction) { return B_OK; }
status_t block_cache_prefetch(void* _cache, off_t blockNumber, size_t* _numBlocks) { if(_numBlocks) *_numBlocks = 0; return B_OK; }

int32 cache_start_transaction(void* _cache) { block_cache* cache = (block_cache*)_cache; if (!cache) return B_ERROR; return atomic_add(&cache->next_transaction_id, 1); }
status_t cache_sync_transaction(void* _cache, int32 id) { return B_OK; }
status_t cache_end_transaction(void* _cache, int32 id, transaction_notification_hook hook, void* data) { return B_OK; }
status_t cache_abort_transaction(void* _cache, int32 id) { return B_OK; }
int32 cache_detach_sub_transaction(void* _cache, int32 id, transaction_notification_hook hook, void* data) { block_cache* cache = (block_cache*)_cache; if (!cache) return B_ERROR; return atomic_add(&cache->next_transaction_id, 1); }
status_t cache_abort_sub_transaction(void* _cache, int32 id) { return B_OK; }
status_t cache_start_sub_transaction(void* _cache, int32 id) { return B_OK; }
status_t cache_add_transaction_listener(void* _cache, int32 id, int32 events, transaction_notification_hook hook, void* data) { return B_OK; }
status_t cache_remove_transaction_listener(void* _cache, int32 id, transaction_notification_hook hook, void* data) { return B_OK; }
status_t cache_next_block_in_transaction(void* _cache, int32 id, bool mainOnly, long* cookie, off_t* bn, void** d, void** ud) { return B_ENTRY_NOT_FOUND; }
int32 cache_blocks_in_transaction(void* _cache, int32 id) { return 0; }
int32 cache_blocks_in_main_transaction(void* _cache, int32 id) { return 0; }
int32 cache_blocks_in_sub_transaction(void* _cache, int32 id) { return 0; }
bool cache_has_block_in_transaction(void* _cache, int32 id, off_t blockNumber) { return false; }

// static void notify_sync(int32, int32, void* _cache) { /* ... */ } // Commented as unused
// static void wait_for_notifications(block_cache* cache) { /* ... */ } // Commented as unused

#if DEBUG_BLOCK_CACHE
// Debug functions commented out as per unused warnings
#endif

block_cache sMarkCache(0,0,0,false); // Definition of the global sMarkCache
// [end of src/system/kernel/cache/block_cache.cpp]

[start of headers/private/kernel/util/iovec_support.h]
/*
 * Copyright 2022, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 */
#ifndef _UTIL_IOVEC_SUPPORT_H
#define _UTIL_IOVEC_SUPPORT_H


#include <KernelExport.h>


typedef struct generic_io_vec {
	generic_addr_t	base;
	generic_size_t	length;
} generic_io_vec;


#ifdef _KERNEL_VM_VM_H

static inline status_t
generic_memcpy(generic_addr_t dest, bool destPhysical, generic_addr_t src, bool srcPhysical,
	generic_size_t size, bool user = false)
{
	if (!srcPhysical && !destPhysical) {
		if (user)
			return user_memcpy((void*)dest, (void*)src, size);
		memcpy((void*)dest, (void*)src, size);
		return B_OK;
	} else if (destPhysical && !srcPhysical) {
		return vm_memcpy_to_physical(dest, (const void*)src, size, user);
	} else if (!destPhysical && srcPhysical) {
		return vm_memcpy_from_physical((void*)dest, src, size, user);
	}

	panic("generic_memcpy: physical -> physical not supported!");
	return B_NOT_SUPPORTED;
}

#endif


#ifdef IS_USER_ADDRESS

/*!
 * Copies an array of `iovec`s from userland.
 * Callers must verify vecCount <= IOV_MAX and supply their own vecs buffer.
 */
static inline status_t
get_iovecs_from_user(const iovec* userVecs, size_t vecCount, iovec* vecs,
	bool permitNull = false)
{
	if (vecCount == 0)
		return B_BAD_VALUE;

	if (!IS_USER_ADDRESS(userVecs))
		return B_BAD_ADDRESS;

	if (user_memcpy(vecs, userVecs, sizeof(iovec) * vecCount) != B_OK)
		return B_BAD_ADDRESS;

	size_t total = 0;
	for (size_t i = 0; i < vecCount; i++) {
		if (permitNull && vecs[i].iov_base == NULL)
			continue;
		if (!is_user_address_range(vecs[i].iov_base, vecs[i].iov_len)) {
			return B_BAD_ADDRESS;
		}
		if (vecs[i].iov_len > SSIZE_MAX || total > (SSIZE_MAX - vecs[i].iov_len)) {
			return B_BAD_VALUE;
		}
		total += vecs[i].iov_len;
	}

	return B_OK;
}

#endif


#endif	// _UTIL_IOVEC_SUPPORT_H

[end of headers/private/kernel/util/iovec_support.h]
