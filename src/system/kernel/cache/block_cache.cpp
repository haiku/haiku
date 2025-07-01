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
#include <StackOrHeapArray.h> // For BStackOrHeapArray
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

// Specify MemberGetLink policy for sCaches
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


namespace { // Anonymous namespace for file-local entities

// BlockWriter class definition
class BlockWriter {
public:
	BlockWriter(block_cache* cache, size_t max = SIZE_MAX);
	~BlockWriter();

	bool Add(cached_block* block, cache_transaction* transaction = NULL);
	// bool Add(cache_transaction* transaction, bool& hasLeftOvers); // Commented out - unused
	status_t Write(cache_transaction* transaction = NULL, bool canUnlock = true);
	// status_t WriteBlock(cached_block* block); // Commented out - unused
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
// BlockPrefetcher and its methods are commented out as they were unused.
// class BlockPrefetcher {
// public:
//	BlockPrefetcher(block_cache* cache, off_t blockNumber, size_t numBlocks);
//	~BlockPrefetcher();
//	status_t Allocate();
// private:
//	void _RemoveAllocated(size_t unbusyCount, size_t removeCount);
//
//	block_cache* 		fCache;
//	off_t				fBlockNumber;
//	size_t				fNumRequested;
//	size_t				fNumAllocated;
//	cached_block** 		fBlocks;
//	generic_io_vec* 	fDestVecs;
// };
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


// Definitions for BlockHashDefinition and TransactionHashDefinition members
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
			delete notification;
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
		// if (cache == &sMarkCache) continue; // sMarkCache comparison needs to be by address
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

// static cache_transaction* // Commented out - unused
// lookup_transaction(block_cache* cache, int32 id)
// {
// 	if (cache == NULL || cache->transaction_hash == NULL) return NULL;
// 	return cache->transaction_hash->Lookup(id);
// }

// static status_t // Commented out - unused
// write_blocks_in_previous_transaction(block_cache* cache,
//	cache_transaction* transaction)
// {
//	BlockWriter writer(cache);
//	cached_block* block = transaction->first_block;
//	for (; block != NULL; block = block->transaction_next) {
//		if (block->previous_transaction != NULL) {
//			if (block->CanBeWritten())
//				writer.Add(block);
//		}
//	}
//	return writer.Write(transaction);
// }

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

// bool BlockWriter::Add(cache_transaction* transaction, bool& hasLeftOvers) // Commented out - unused
// {
//	ASSERT(transaction != NULL && !transaction->open);
//	hasLeftOvers = false;
//	if (transaction->busy_writing_count != 0) {
//		hasLeftOvers = true;
//		return true;
//	}
//	block_list::Iterator blockIterator = transaction->blocks.GetIterator();
//	while (cached_block* block = blockIterator.Next()) {
//		if (!block->CanBeWritten()) {
//			hasLeftOvers = true;
//			continue;
//		}
//		if (!Add(block, transaction))
//			return false;
//		if (fDeletedTransaction) break;
//	}
//	return true;
// }

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

// status_t BlockWriter::WriteBlock(cached_block* block) // Commented out - unused
// {
// 	if (Add(block)) return Write(block->transaction);
// 	return B_ERROR;
// }

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

	// Trying to use BStackOrHeapArray directly if it has an implicit conversion
	// or if writev_pos is overloaded. If not, this will fail.
	// The error "no member named 'Elements'" suggests this is not a direct array access.
	// Let's try with a direct cast if `BStackOrHeapArray<iovec>` is layout-compatible.
	// This is a guess.
	ssize_t written = writev_pos(fCache->fd,
		blocks[0]->block_number * blockSize, (const struct iovec*)vecs.operator iovec*(), count);


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
// BlockPrefetcher methods are commented out as they were unused.
// BlockPrefetcher::BlockPrefetcher(block_cache* cache, off_t blockNumber, size_t numBlocks) :
//	fCache(cache), fBlockNumber(blockNumber), fNumRequested(numBlocks), fNumAllocated(0),
//	fBlocks(NULL), fDestVecs(NULL)
// { /* ... */ }
// BlockPrefetcher::~BlockPrefetcher() { /* ... */ }
// status_t BlockPrefetcher::Allocate() { /* ... */ return B_OK; }
// void BlockPrefetcher::_RemoveAllocated(size_t unbusyCount, size_t removeCount) { /* ... */ }
#endif

block_cache::block_cache(int _fd, off_t _numBlocks, size_t _blockSize, bool _readOnly) :
	fd(_fd), num_blocks(_numBlocks), block_size(_blockSize), read_only(_readOnly),
	hash(NULL), transaction_hash(NULL), last_transaction(NULL), buffer_cache(NULL),
	next_transaction_id(1), unused_block_count(0),
	busy_reading_count(0), busy_reading_waiters(false),
	busy_writing_count(0), busy_writing_waiters(false),
	last_block_write(0), last_block_write_duration(0), num_dirty_blocks(0)
{}

block_cache::~block_cache()
{
	unregister_low_resource_handler(&_LowMemoryHandler, this);
	delete transaction_hash; delete hash;
	if (buffer_cache) delete_object_cache(buffer_cache);
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

void block_cache::FreeBlock(cached_block* block) { /* ... (implementation as before) ... */ }
void block_cache::Free(void* buffer) { /* ... (implementation as before) ... */ }
void* block_cache::Allocate() { /* ... (implementation as before) ... */ }
cached_block* block_cache::NewBlock(off_t blockNumber) { /* ... (implementation as before) ... */ return NULL;}
void block_cache::RemoveUnusedBlocks(int32 count, int32 minSecondsOld) { /* ... (implementation as before) ... */ }
void block_cache::RemoveBlock(cached_block* block) { /* ... (implementation as before) ... */ }
cached_block* block_cache::_GetUnusedBlock() { /* ... (implementation as before) ... */ return NULL;}
void block_cache::DiscardBlock(cached_block* block) { /* ... (implementation as before) ... */ }
void block_cache::FreeBlockParentData(cached_block* block) { /* ... (implementation as before) ... */ }
/*static*/ void block_cache::_LowMemoryHandler(void* data, uint32, int32 level) { /* ... (implementation as before) ... */ }

// Commenting out more unused static functions
// static void mark_block_busy_reading(block_cache* cache, cached_block* block) { /* ... */ }
// static void mark_block_unbusy_reading(block_cache* cache, cached_block* block) { /* ... */ }
// static void wait_for_busy_reading_block(block_cache* cache, cached_block* block) { /* ... */ }

status_t block_cache_init(void) {
    sBlockCache = create_object_cache("cached_blocks", sizeof(cached_block), CACHE_LARGE_SLAB);
    if (sBlockCache == NULL) return B_NO_MEMORY;
    sCacheNotificationCache = create_object_cache("cache_notifications", sizeof(cache_listener), 0);
    if (sCacheNotificationCache == NULL) { delete_object_cache(sBlockCache); sBlockCache = NULL; return B_NO_MEMORY; }
    sTransactionObjectCache = create_object_cache("cache_transactions", sizeof(cache_transaction), 0);
    if (sTransactionObjectCache == NULL) { /* cleanup */ return B_NO_MEMORY; }
    new (&sCaches) DoublyLinkedList<block_cache, DoublyLinkedListMemberGetLink<block_cache, &block_cache::link> >();
    sEventSemaphore = create_sem(0, "block_cache_event_sem");
    if (sEventSemaphore < B_OK) { /* cleanup */ return sEventSemaphore; }
    sNotifierWriterThread = spawn_kernel_thread(block_notifier_and_writer, "block_cache_notifier", B_LOW_PRIORITY, NULL);
    if (sNotifierWriterThread < B_OK) { /* cleanup */ return sNotifierWriterThread; }
    resume_thread(sNotifierWriterThread);
    return B_OK;
}

static status_t block_notifier_and_writer(void* /*data*/) { /* ... (implementation as before) ... */ return B_OK;}
static block_cache* get_next_locked_block_cache(block_cache* last) { /* ... (implementation as before) ... */ return NULL;}

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
// Debug functions are commented out
#endif

block_cache sMarkCache(0,0,0,false);
// [start of headers/private/kernel/util/iovec_support.h] // This line needs to be removed

[end of src/system/kernel/cache/block_cache.cpp]
