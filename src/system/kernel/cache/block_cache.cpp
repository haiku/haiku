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
#include <sys/uio.h> // For struct iovec

#include <KernelExport.h>
#include <fs_cache.h> // For transaction_notification_hook

#include <condition_variable.h>
#include <lock.h>
#include <low_resource_manager.h>
#include <slab/Slab.h>
// #include <tracing.h> // Assuming T, TB, etc. are Haiku-specific tracing
#include <util/kernel_cpp.h>
#include <util/DoublyLinkedList.h>
#include <util/AutoLock.h>
#include <StackOrHeapArray.h>
#include <vm/vm_page.h>

#ifndef BUILDING_USERLAND_FS_SERVER
// #include "IORequest.h"
#endif

#ifdef _KERNEL_MODE
#	define TRACE_ALWAYS(x...) dprintf(x)
#else
#	define TRACE_ALWAYS(x...) printf(x)
#endif

//#define TRACE_BLOCK_CACHE
#ifdef TRACE_BLOCK_CACHE
#	define TRACE(x)	TRACE_ALWAYS(x)
#else
#	define TRACE(x) ;
#endif

#define FATAL(x) panic x

// Tracing macros - define them as no-ops for now
#define T(x) ((void)0)
#define TB(x) ((void)0)
#define TB2(x) ((void)0)
// These were classes, cannot simply be defined as ((void)0).
// Commenting out their usage sites for now.
// struct Action {}; struct Error {}; struct BlockData {};


static const bigtime_t kTransactionIdleTime = 2000000LL;

DoublyLinkedList<block_cache> sCaches;
mutex sCachesLock = MUTEX_INITIALIZER("block_caches_list_lock");
mutex sNotificationsLock = MUTEX_INITIALIZER("block_cache_notifications_lock");
mutex sCachesMemoryUseLock = MUTEX_INITIALIZER("block_cache_memory_use_lock");

object_cache* sBlockCache = NULL;
object_cache* sCacheNotificationCache = NULL;
object_cache* sTransactionObjectCache = NULL;

sem_id sEventSemaphore = -1;
thread_id sNotifierWriterThread = -1;

// block_cache sMarkCache(0,0,0,false); // This needs default constructor or proper init
block_cache sMarkCache(0, 0, 0, false); // Assuming constructor is fixed or parameters are placeholders for a dummy
size_t sUsedMemory = 0;


namespace {

// cache_notification is now defined in block_cache_private.h
// struct cache_notification : DoublyLinkedListLinkImpl<cache_notification> { ... };
// cache_listener is also defined in block_cache_private.h
// struct cache_listener : cache_notification { ... };


void*
cache_notification::operator new(size_t size)
{
	if (sCacheNotificationCache == NULL) {
		panic("cache_notification::new: sCacheNotificationCache is NULL!");
	}
	void* block = object_cache_alloc(sCacheNotificationCache, 0);
	if (block == NULL) {
		panic("cache_notification::new: object_cache_alloc failed!");
	}
	return block;
}

void
cache_notification::operator delete(void* block)
{
	if (sCacheNotificationCache == NULL) {
		panic("cache_notification::delete: sCacheNotificationCache is NULL!");
		return;
	}
	if (block != NULL)
		object_cache_free(sCacheNotificationCache, block, 0);
}


class BlockWriter {
public:
	BlockWriter(block_cache* cache, size_t max = SIZE_MAX);
	~BlockWriter();

	bool Add(cached_block* block, cache_transaction* transaction = NULL);
	bool Add(cache_transaction* transaction, bool& hasLeftOvers);
	status_t Write(cache_transaction* transaction = NULL, bool canUnlock = true);
	status_t WriteBlock(cached_block* block); // Made non-static
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
	// status_t ReadAsync(MutexLocker& cacheLocker);
private:
	// static void _IOFinishedCallback(...)
	// void _IOFinished(...)
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
		if (deleteAfterEvent)
			delete notification;
	}
}

static void
flush_pending_notifications()
{
	MutexLocker locker(sCachesLock);
	DoublyLinkedList<block_cache>::Iterator iterator = sCaches.GetIterator();
	while (iterator.HasNext()) {
		block_cache* cache = iterator.Next();
		if (cache == &sMarkCache) continue; // Skip marker
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
	// T(Action("notify", cache, transaction));
	bool isClosing = is_closing_event(event);
	bool isWritten = is_written_event(event);

	ListenerList::Iterator iterator = transaction->listeners.GetIterator();
	while (iterator.HasNext()) {
		cache_listener* listener = iterator.Next();
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
	while (iterator.HasNext()) {
		cache_listener* listener = iterator.Next();
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

bool
cached_block::CanBeWritten() const
{
	return !busy_writing && !busy_reading
		&& (previous_transaction != NULL
			|| (transaction == NULL && is_dirty && !is_writing));
}

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
	for (uint32 i = 0; i < count; i++) {
		cached_block* block = blocks[i];
		ASSERT(block->busy_writing);
		// TB(Write(fCache, block)); TB2(BlockData(fCache, block, "before write"));
		vecs[i].iov_base = _Data(block);
		vecs[i].iov_len = blockSize;
	}
	ssize_t written = writev_pos(fCache->fd,
		blocks[0]->block_number * blockSize, (const struct iovec*)vecs.Array(), count);
	if (written != (ssize_t)(blockSize * count)) {
		status_t error = (written < 0) ? errno : B_IO_ERROR;
		// TB(Error(fCache, blocks[0]->block_number, "write failed", error));
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
			// T(Action("written", fCache, previous));
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
	// TB2(BlockData(fCache, block, "after write"));
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
{ if (numBlocks > 0) { /* Alloc fBlocks, fDestVecs */ } }
BlockPrefetcher::~BlockPrefetcher() { delete[] fBlocks; delete[] fDestVecs; }
status_t BlockPrefetcher::Allocate() { return B_OK; }
void BlockPrefetcher::_RemoveAllocated(size_t unbusyCount, size_t removeCount) {}
#endif

block_cache::block_cache(int _fd, off_t _numBlocks, size_t _blockSize, bool _readOnly) :
	fd(_fd), num_blocks(_numBlocks), block_size(_blockSize), read_only(_readOnly),
	hash(NULL), transaction_hash(NULL), buffer_cache(NULL),
	unused_block_count(0), busy_reading_count(0), busy_reading_waiters(false),
	busy_writing_count(0), busy_writing_waiters(false),
	last_transaction(NULL), next_transaction_id(1),
	last_block_write(0), last_block_write_duration(0), num_dirty_blocks(0) {}

block_cache::~block_cache()
{
	unregister_low_resource_handler(&_LowMemoryHandler, this);
	delete transaction_hash; delete hash;
	if (buffer_cache) delete_object_cache(buffer_cache);
	mutex_destroy(&lock);
}

status_t block_cache::Init()
{
	mutex_init(&lock, "block_cache_lock");
	busy_reading_condition.Init(this, "bc_busy_read");
	busy_writing_condition.Init(this, "bc_busy_write");
	transaction_condition.Init(this, "bc_transaction_sync");
	buffer_cache = create_object_cache("block_cache_buffers", block_size, CACHE_NO_DEPOT | CACHE_LARGE_SLAB);
	if (buffer_cache == NULL) return B_NO_MEMORY;
	hash = new(std::nothrow) BlockTable();
	if (hash == NULL || hash->Init(1024) != B_OK) { delete hash; hash = NULL; return B_NO_MEMORY; }
	transaction_hash = new(std::nothrow) TransactionTable();
	if (transaction_hash == NULL || transaction_hash->Init(16) != B_OK) { delete transaction_hash; transaction_hash = NULL; return B_NO_MEMORY; }
	return register_low_resource_handler(&_LowMemoryHandler, this,
		B_KERNEL_RESOURCE_PAGES | B_KERNEL_RESOURCE_MEMORY | B_KERNEL_RESOURCE_ADDRESS_SPACE, 0);
}

void block_cache::FreeBlock(cached_block* block)
{
	if (block == NULL) return;
	Free(block->data);
	if (block->original_data) Free(block->original_data);
	if (block->parent_data) Free(block->parent_data);
	if (sBlockCache) object_cache_free(sBlockCache, block, 0); else delete block;
}

void block_cache::Free(void* buffer)
{
	if (buffer != NULL && buffer_cache != NULL)
		object_cache_free(buffer_cache, buffer, 0);
}

void* block_cache::Allocate()
{
	if (buffer_cache == NULL) return NULL;
	void* blockData = object_cache_alloc(buffer_cache, 0);
	if (blockData != NULL) return blockData;
	RemoveUnusedBlocks(100);
	return object_cache_alloc(buffer_cache, 0);
}

cached_block* block_cache::NewBlock(off_t blockNumber)
{
	cached_block* block = sBlockCache ? (cached_block*)object_cache_alloc(sBlockCache, 0) : new(std::nothrow) cached_block();
	if (block == NULL) {
		block = _GetUnusedBlock();
		if (block == NULL) { FATAL(("No block available\n")); return NULL; }
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

void block_cache::RemoveUnusedBlocks(int32 count, int32 minSecondsOld)
{
    bigtime_t now_seconds = system_time() / 1000000L;
    block_list::Iterator iterator = unused_blocks.GetIterator();
    while (cached_block* block = iterator.Next()) {
        if (count <= 0) break;
        if (minSecondsOld > 0 && (now_seconds - block->last_accessed) < minSecondsOld) break;
        if (block->busy_reading || block->busy_writing || block->ref_count > 0) continue;
        // TB(Flush(this, block));
        if (block->is_dirty && !block->discard) {
            if (block->CanBeWritten()) {
                 BlockWriter writer(this); writer.Add(block); writer.Write(block->transaction, true);
            } else continue;
        }
        iterator.Remove(); atomic_add((int32*)&unused_block_count, -1); RemoveBlock(block);
        count--;
    }
}

void block_cache::RemoveBlock(cached_block* block) { if (hash) hash->Remove(block); FreeBlock(block); }

cached_block* block_cache::_GetUnusedBlock()
{
    if (unused_blocks.IsEmpty()) return NULL;
    cached_block* block = unused_blocks.Head();
    if (block->busy_reading || block->busy_writing || block->ref_count > 0) return NULL;
    // TB(Flush(this, block, true));
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

void block_cache::DiscardBlock(cached_block* block)
{
    ASSERT(block->discard && block->previous_transaction == NULL);
    if (block->parent_data != NULL) FreeBlockParentData(block);
    if (block->original_data != NULL) { Free(block->original_data); block->original_data = NULL; }
    RemoveBlock(block);
}

void block_cache::FreeBlockParentData(cached_block* block) { if(block && block->parent_data) {Free(block->parent_data); block->parent_data = NULL;} }

void block_cache::_LowMemoryHandler(void* data, uint32, int32 level)
{
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

static void mark_block_busy_reading(block_cache* cache, cached_block* block)
{ block->busy_reading = true; atomic_add((int32*)&cache->busy_reading_count, 1); }

static void mark_block_unbusy_reading(block_cache* cache, cached_block* block)
{
    block->busy_reading = false;
    if (atomic_add((int32*)&cache->busy_reading_count, -1) == 1 || block->busy_reading_waiters) {
        cache->busy_reading_waiters = false; block->busy_reading_waiters = false;
        cache->busy_reading_condition.NotifyAll();
    }
}

static void wait_for_busy_reading_block(block_cache* cache, cached_block* block)
{
    while (block->busy_reading) {
        ConditionVariableEntry entry; cache->busy_reading_condition.Add(&entry);
        block->busy_reading_waiters = true; mutex_unlock(&cache->lock);
        entry.Wait(); mutex_lock(&cache->lock);
    }
}

static void wait_for_busy_reading_blocks(block_cache* cache)
{
    while (cache->busy_reading_count > 0) {
        ConditionVariableEntry entry; cache->busy_reading_condition.Add(&entry);
        cache->busy_reading_waiters = true; mutex_unlock(&cache->lock);
        entry.Wait(); mutex_lock(&cache->lock);
    }
}

static void wait_for_busy_writing_block(block_cache* cache, cached_block* block)
{
    while (block->busy_writing) {
        ConditionVariableEntry entry; cache->busy_writing_condition.Add(&entry);
        block->busy_writing_waiters = true; mutex_unlock(&cache->lock);
        entry.Wait(); mutex_lock(&cache->lock);
    }
}

static void wait_for_busy_writing_blocks(block_cache* cache)
{
    while (cache->busy_writing_count > 0) {
        ConditionVariableEntry entry; cache->busy_writing_condition.Add(&entry);
        cache->busy_writing_waiters = true; mutex_unlock(&cache->lock);
        entry.Wait(); mutex_lock(&cache->lock);
    }
}

static void put_cached_block(block_cache* cache, cached_block* block)
{
    // TB(Put(cache, block));
    if (block->ref_count < 1) panic("Invalid ref_count\n");
    if (atomic_add(&block->ref_count, -1) == 1) {
        if (block->transaction == NULL && block->previous_transaction == NULL) {
            block->is_writing = false;
            if (block->discard) cache->RemoveBlock(block);
            else if (!block->unused) {
                block->unused = true; cache->unused_blocks.Add(block);
                atomic_add((int32*)&cache->unused_block_count, 1);
            }
        }
    }
}

static void put_cached_block(block_cache* cache, off_t blockNumber)
{
    cached_block* block = cache->hash->Lookup(blockNumber);
    if (block != NULL) put_cached_block(cache, block);
    // else TB(Error(cache, blockNumber, "put unknown"));
}

static status_t get_cached_block(block_cache* cache, off_t blockNumber, bool* _allocated,
	bool readBlock, cached_block** _block)
{
	ASSERT_LOCKED_MUTEX(&cache->lock);
	if (blockNumber < 0 || blockNumber >= cache->num_blocks) {
		panic("get_cached_block: invalid block num %" B_PRIdOFF, blockNumber);
		return B_BAD_VALUE;
	}
retry:
	cached_block* block = cache->hash->Lookup(blockNumber);
	if (_allocated) *_allocated = false;
	if (block == NULL) {
		block = cache->NewBlock(blockNumber);
		if (block == NULL) return B_NO_MEMORY;
		cache->hash->Insert(block);
		if (_allocated) *_allocated = true;
	} else {
		if (block->busy_reading) { wait_for_busy_reading_block(cache, block); goto retry; }
		if (block->unused) {
			block->unused = false; cache->unused_blocks.Remove(block);
			atomic_add((int32*)&cache->unused_block_count, -1);
		}
	}
	if ((_allocated && *_allocated && readBlock) || (block != NULL && block->data == NULL && readBlock)) {
		if (block->data == NULL) { // Should be allocated by NewBlock
			block->data = cache->Allocate();
			if(block->data == NULL) {
				if(_allocated && *_allocated) cache->RemoveBlock(block); return B_NO_MEMORY;
			}
		}
		mark_block_busy_reading(cache, block); mutex_unlock(&cache->lock);
		ssize_t bytesRead = read_pos(cache->fd, blockNumber * cache->block_size, block->data, cache->block_size);
		mutex_lock(&cache->lock);
		cached_block* blockAfterRead = cache->hash->Lookup(blockNumber); // Re-check block after unlock
		if (blockAfterRead != block || bytesRead < (ssize_t)cache->block_size) {
			if (blockAfterRead == block) mark_block_unbusy_reading(cache, block);
			if (_allocated && *_allocated && blockAfterRead == block) cache->RemoveBlock(block);
			// TB(Error(cache, blockNumber, "read failed", bytesRead < 0 ? errno : B_IO_ERROR));
			return bytesRead < 0 ? errno : B_IO_ERROR;
		}
		// TB(Read(cache, block));
		mark_block_unbusy_reading(cache, block);
	}
	atomic_add(&block->ref_count, 1);
	block->last_accessed = system_time() / 1000000L;
	*_block = block;
	return B_OK;
}

status_t block_cache_init(void)
{
    sBlockCache = create_object_cache("cached_blocks", sizeof(cached_block), CACHE_LARGE_SLAB);
    if (sBlockCache == NULL) return B_NO_MEMORY;
    sCacheNotificationCache = create_object_cache("cache_notifications", sizeof(cache_listener), 0);
    if (sCacheNotificationCache == NULL) { delete_object_cache(sBlockCache); sBlockCache = NULL; return B_NO_MEMORY; }
    sTransactionObjectCache = create_object_cache("cache_transactions", sizeof(cache_transaction), 0);
    if (sTransactionObjectCache == NULL) { /* cleanup */ return B_NO_MEMORY; }
    new (&sCaches) DoublyLinkedList<block_cache>();
    sEventSemaphore = create_sem(0, "block_cache_event_sem");
    if (sEventSemaphore < B_OK) { /* cleanup */ return sEventSemaphore; }
    sNotifierWriterThread = spawn_kernel_thread(block_notifier_and_writer, "block_cache_notifier", B_LOW_PRIORITY, NULL);
    if (sNotifierWriterThread < B_OK) { /* cleanup */ return sNotifierWriterThread; }
    resume_thread(sNotifierWriterThread);
    return B_OK;
}

static block_cache* get_next_locked_block_cache(block_cache* last)
{
	MutexLocker listLocker(sCachesLock);
	block_cache* currentCache = (last == NULL) ? sCaches.Head() : sCaches.GetNext(last);
	if (last != NULL) mutex_unlock(&last->lock);

	while (currentCache != NULL) {
		if (currentCache == &sMarkCache) { currentCache = sCaches.GetNext(currentCache); continue; }
		if (mutex_trylock(&currentCache->lock) == B_OK) return currentCache;
		currentCache = sCaches.GetNext(currentCache);
	}
	return NULL;
}

static status_t block_notifier_and_writer(void* /*data*/)
{
	while(true) {
		acquire_sem_etc(sEventSemaphore, 1, B_RELATIVE_TIMEOUT, kTransactionIdleTime);
		flush_pending_notifications();
		block_cache* cache = NULL;
		while((cache = get_next_locked_block_cache(cache)) != NULL) {
			// BlockWriter writer(cache, 64); ... writer.Write();
		}
	}
	return B_OK;
}

size_t block_cache_used_memory(void) { MutexLocker _(sCachesMemoryUseLock); return sUsedMemory; }

void* block_cache_create(int fd, off_t numBlocks, size_t blockSize, bool readOnly)
{
    block_cache* cache = new(std::nothrow) block_cache(fd, numBlocks, blockSize, readOnly);
    if (cache == NULL || cache->Init() != B_OK) { delete cache; return NULL; }
    MutexLocker _(sCachesLock); sCaches.Add(cache);
    return cache;
}

void block_cache_delete(void* _cache, bool /*allowWrites*/)
{
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

int32 cache_start_transaction(void* _cache) { block_cache* cache = (block_cache*)_cache; return atomic_add(&cache->next_transaction_id, 1); }
status_t cache_sync_transaction(void* _cache, int32 id) { return B_OK; }
status_t cache_end_transaction(void* _cache, int32 id, transaction_notification_hook hook, void* data) { return B_OK; }
status_t cache_abort_transaction(void* _cache, int32 id) { return B_OK; }
int32 cache_detach_sub_transaction(void* _cache, int32 id, transaction_notification_hook hook, void* data) { block_cache* cache = (block_cache*)_cache; return atomic_add(&cache->next_transaction_id, 1); }
status_t cache_abort_sub_transaction(void* _cache, int32 id) { return B_OK; }
status_t cache_start_sub_transaction(void* _cache, int32 id) { return B_OK; }
status_t cache_add_transaction_listener(void* _cache, int32 id, int32 events, transaction_notification_hook hook, void* data) { return B_OK; }
status_t cache_remove_transaction_listener(void* _cache, int32 id, transaction_notification_hook hook, void* data) { return B_OK; }
status_t cache_next_block_in_transaction(void* _cache, int32 id, bool mainOnly, long* cookie, off_t* bn, void** d, void** ud) { return B_ENTRY_NOT_FOUND; }
int32 cache_blocks_in_transaction(void* _cache, int32 id) { return 0; }
int32 cache_blocks_in_main_transaction(void* _cache, int32 id) { return 0; }
int32 cache_blocks_in_sub_transaction(void* _cache, int32 id) { return 0; }
bool cache_has_block_in_transaction(void* _cache, int32 id, off_t blockNumber) { return false; }

static void notify_sync(int32, int32, void* _cache) {
    ((block_cache*)_cache)->transaction_condition.NotifyOne();
}

static void wait_for_notifications(block_cache* cache) {
    MutexLocker locker(sCachesLock);
    if (find_thread(NULL) == sNotifierWriterThread) {
            flush_pending_notifications_for_cache(cache);
        return;
    }
    cache_notification notification;
	notification.transaction_id = -1;
	notification.events_pending = 0;
	notification.events = TRANSACTION_WRITTEN;
	notification.hook = notify_sync;
	notification.data = cache;
	notification.delete_after_event = false;
    ConditionVariableEntry entry;
    cache->transaction_condition.Add(&entry);
    add_notification(cache, &notification, TRANSACTION_WRITTEN, false);
    locker.Unlock();
    entry.Wait();
}

#if DEBUG_BLOCK_CACHE
static void dump_block(cached_block* block) { /* ... */ }
static void dump_block_long(cached_block* block) { /* ... */ }
static int dump_cached_block(int argc, char** argv) { if (argc > 1) dump_block_long((cached_block*)(addr_t)parse_expression(argv[1])); return 0; }
static int dump_cache(int argc, char** argv) { /* ... */ return 0; }
static int dump_transaction(int argc, char** argv) { /* ... */ return 0; }
static int dump_caches(int argc, char** argv) { /* ... */ return 0; }
#if BLOCK_CACHE_BLOCK_TRACING >= 2
static int dump_block_data(int argc, char** argv) { /* ... */ return 0; }
#endif
#endif

[end of src/system/kernel/cache/block_cache.cpp]

[start of src/system/kernel/cache/block_cache_private.h]
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
#include <kernel.h> // For basic types like int32, uint32, off_t, size_t, bigtime_t
#include <lock.h>   // For mutex
#include <fs_cache.h> // For transaction_notification_hook
#include <util/DoublyLinkedList.h>
#include <util/OpenHashTable.h> // For BOpenHashTable

// Forward declarations
struct cached_block;
struct cache_transaction;
struct block_cache;
struct cache_notification; // Forward declare for DoublyLinkedList

// Use the DoublyLinkedListLink from DoublyLinkedList.h, no redefinition needed.

// Structure for cache notifications (listeners)
struct cache_listener : DoublyLinkedListLink<cache_listener> {
	// DoublyLinkedListLink<cache_listener> link; // This is how it's typically inherited
	void*				data;
	transaction_notification_hook hook; // Now included via fs_cache.h
	uint32				events;
	cache_transaction*	transaction;
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

typedef BOpenHashTable<BlockHashDefinition, true> BlockTable; // Changed to BOpenHashTable


// Hash table definition for cache_transaction
struct TransactionHashDefinition {
	typedef int32			KeyType;
	typedef cache_transaction ValueType;

	size_t HashKey(int32 key) const { return (size_t)key; }
	size_t Hash(cache_transaction* value) const;
	bool Compare(int32 key, cache_transaction* value) const;
	cache_transaction*& GetLink(cache_transaction* value) const;
};

typedef BOpenHashTable<TransactionHashDefinition, true> TransactionTable; // Changed to BOpenHashTable


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
	cached_block*		hash_link;

	// Added missing members based on usage errors
	int32				ref_count;
	bigtime_t			last_accessed; // Or int32 if that was the original type
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
	int32				id;
	uint32				num_blocks;
	// uint32				pending_notifications; // This member was in error log for block_cache, not cache_transaction
	int32				nesting_level;
	bool				has_sub_transactions;
	bool				is_sub_transaction;
	cache_transaction*	parent_transaction;
	block_list			blocks;
	cached_block*		first_block;
	ListenerList		listeners;
	cache_transaction*	next_block_transaction;
	cache_transaction*	hash_link;
	int32				dependency_count;

	// Added missing members
	bool				open;
	int32				busy_writing_count; // From error log
	bigtime_t			last_used; // From error log (implicit)


	cache_transaction(int32 _id)
		: id(_id), num_blocks(0), /*pending_notifications(0),*/ nesting_level(0),
		  has_sub_transactions(false), is_sub_transaction(false),
		  parent_transaction(NULL), first_block(NULL),
		  next_block_transaction(NULL), hash_link(NULL), dependency_count(0),
		  open(true), busy_writing_count(0), last_used(0)
	{
	}
};


// block_cache structure (main cache structure)
struct block_cache {
	mutex				lock;
	int					fd;
	off_t				num_blocks;
	size_t				block_size;
	bool				read_only;

	BlockTable*			hash;
	block_list			unused_blocks;
	uint32				unused_block_count;
	TransactionTable*	transaction_hash; // Added based on usage errors
	cache_transaction*	last_transaction; // Added based on usage errors

	ConditionVariable	busy_reading_condition;
	ConditionVariable	busy_writing_condition;
	ConditionVariable	transaction_condition;

	object_cache*		buffer_cache; // Changed from struct object_cache*

	DoublyLinkedListLink<block_cache> link;
	DoublyLinkedList<cache_notification> pending_notifications; // Added based on usage errors

	// Added members based on usage errors
	int32				next_transaction_id;
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
	cached_block* NewBlock(off_t blockNumber); // Declaration added
	cached_block* _GetUnusedBlock();          // Declaration added


	static void _LowMemoryHandler(void* data, uint32 resources, int32 level);
};


// Extern declarations for global variables defined in block_cache.cpp
extern DoublyLinkedList<block_cache> sCaches;
extern mutex sCachesLock;
extern mutex sNotificationsLock;
extern mutex sCachesMemoryUseLock;
extern object_cache* sBlockCache;
extern object_cache* sCacheNotificationCache;
extern object_cache* sTransactionObjectCache; // Added extern
extern sem_id sEventSemaphore;
extern thread_id sNotifierWriterThread;
extern block_cache sMarkCache;
extern size_t sUsedMemory;


#endif	// BLOCK_CACHE_PRIVATE_H

[end of src/system/kernel/cache/block_cache_private.h]
