/*
 * Copyright 2004-2023, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2023, Your Name, your.email@example.com.
 * Distributed under the terms of the MIT License.
 */

#include "block_cache_private.h"
#include "unified_cache.h"

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/uio.h>

#include <KernelExport.h>
#include <fs_cache.h>

#include <condition_variable.h>
#include <lock.h>
#include <low_resource_manager.h>
#include <slab/Slab.h>
#include <tracing.h>
#include <util/kernel_cpp.h>
#include <util/DoublyLinkedList.h>
#include <util/AutoLock.h>
#include <StackOrHeapArray.h>
#include <vm/vm_page.h> // For read_pos, writev_pos if used directly

#ifndef BUILDING_USERLAND_FS_SERVER
// #include "IORequest.h" // This might be replaced or adapted depending on how I/O is handled
#endif // !BUILDING_USERLAND_FS_SERVER
#include "kernel_debug_config.h"


// TODO: this is a naive but growing implementation to test the API:
//	block reading/writing is not at all optimized for speed, it will
//	just read and write single blocks.
// TODO: the retrieval/copy of the original data could be delayed until the
//		new data must be written, ie. in low memory situations.

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


// This macro is used for fatal situations that are acceptable in a running
// system, like out of memory situations - should only panic for debugging.
#define FATAL(x) panic x

static const bigtime_t kTransactionIdleTime = 2000000LL;
	// a transaction is considered idle after 2 seconds of inactivity

// Global unified cache for all block devices.
// This should be initialized in block_cache_init().
// A more sophisticated system might use a cache manager or allow multiple
// named unified_cache instances. For now, a single one simplifies things.
static unified_cache* gBlockUnifiedCache = NULL;

// Object cache for transaction structures if they are kept similar
static object_cache* sTransactionObjectCache = NULL;
static object_cache* sCacheNotificationCache = NULL; // Used by cache_notification new/delete

// List of all block_cache_instance_data structures
static DoublyLinkedList<block_cache_instance_data> sBlockCacheInstances;
static mutex sBlockCacheInstancesLock = MUTEX_INITIALIZER("block_cache_instances_list_lock");

// For transaction event notifications
static sem_id sTransactionEventSemaphore;
// sTransactionNotificationsLock is already defined below if needed, or should be here. Let's assume it's correct.
static thread_id sTransactionNotifierWriterThread;

// If sMarkInstance is used for safe iteration over sBlockCacheInstances
// static DoublyLinkedListLink<block_cache_instance_data> sMarkInstance;

// Global memory usage tracking, if not handled per-instance or by unified_cache
// static mutex sBlockCacheMemoryUseLock = MUTEX_INITIALIZER("block_cache_memory_use_lock");
// static size_t sCurrentBlockDataMemoryUsed = 0;


// The old cached_block and block_cache structs are removed or significantly changed.
// The new block_cache_instance_data (from block_cache_private.h) will hold
// instance-specific data (fd, block_size, etc.) and a pointer to gBlockUnifiedCache.

// Forward declarations for static functions that might be kept or adapted
// For example, transaction helper functions.
// Many of the old static functions related to direct cache management (hashing, LRU)
// will be removed.

// The old BlockHash, TransactionHash, block_list, etc., are removed as
// the unified cache handles its own hashing and eviction lists.

// The old 'namespace {' anonymous namespace might still be used for static helpers.
namespace {

// Adapted cache_transaction might still be needed if transaction logic is complex
// and not fully embedded in unified_cache_entry.
// The definition from block_cache_private.h will be used.

// TODO: Review if cache_notification needs to be redefined or if fs_cache.h version is enough
// For now, assume the local definition from old block_cache.cpp is what's used for listeners.
struct cache_notification : DoublyLinkedListLinkImpl<cache_notification> {
	static inline void* operator new(size_t size);
	static inline void operator delete(void* block);

	int32			transaction_id;
	int32			events_pending;
	int32			events;
	transaction_notification_hook hook; // From fs_cache.h
	void*			data;
	bool			delete_after_event;
};

struct cache_listener; // Forward declare for cache_notification::new
typedef DoublyLinkedListLink<cache_listener> listener_link;

// ListenerList and related notification structures might be kept if the
// transaction notification API is preserved.
typedef DoublyLinkedList<cache_notification> NotificationList;

struct cache_listener : cache_notification {
	listener_link	link;
};
typedef DoublyLinkedList<cache_listener,
	DoublyLinkedListMemberGetLink<cache_listener,
		&cache_listener::link> > ListenerList;

void*
cache_notification::operator new(size_t size)
{
	ASSERT(size <= sizeof(cache_listener));
	return object_cache_alloc(sCacheNotificationCache, 0);
}

void
cache_notification::operator delete(void* block)
{
	object_cache_free(sCacheNotificationCache, block, 0);
}


// BlockWriter might be simplified or adapted. It primarily writes data to disk.
// It will now get data from unified_cache_entry.
class BlockWriter {
public:
								BlockWriter(block_cache_instance_data* cacheInstance,
									size_t max = SIZE_MAX);
								~BlockWriter();

			bool				Add(unified_cache_entry* entry,
									cache_transaction* transaction = NULL);
			// bool				Add(cache_transaction* transaction,
			// 						bool& hasLeftOvers); // This needs redesign for transactions

			status_t			Write(/*cache_transaction* transaction = NULL,*/
									bool canUnlock = true); // transaction might not be needed if BlockWriter just writes entries

			// bool				DeletedTransaction() const
			// 						{ return fDeletedTransaction; } // If transactions are managed differently

	// static	status_t			WriteBlock(block_cache_instance_data* cacheInstance,
	// 								unified_cache_entry* entry); // Static helper

private:
			// void*				_Data(unified_cache_entry* entry) const; // Gets data from entry
			status_t			_WriteBlocks(unified_cache_entry** entries, uint32 count, size_t blockSize, int fd);
			// void				_BlockDone(unified_cache_entry* entry,
			// 						cache_transaction* transaction);
			// void				_UnmarkWriting(unified_cache_entry* entry);

	static	int					_CompareBlocks(const void* _entryA,
									const void* _entryB); // Compares based on entry->id (block_number)

private:
	static	const size_t		kBufferSize = 64; // Max entries to write in one go

			block_cache_instance_data*		fCacheInstance;
			unified_cache_entry*		fBuffer[kBufferSize];
			unified_cache_entry**		fEntries; // Points to fBuffer or allocated array
			size_t				fCount;
			size_t				fTotal; // Total entries added for writing in this batch
			size_t				fCapacity;
			size_t				fMax; // Max entries to write in this BlockWriter lifetime
			status_t			fStatus;
			// bool				fDeletedTransaction;
};


// BlockPrefetcher will also need to be adapted to use unified_cache
#ifndef BUILDING_USERLAND_FS_SERVER
class BlockPrefetcher {
public:
								BlockPrefetcher(block_cache_instance_data* cacheInstance,
									off_t blockNumber, size_t numBlocks);
								~BlockPrefetcher();

			status_t			AllocateAndFetch(); // Simplified

private:
	// static	void			_IOFinishedCallback(void* cookie, io_request* request,
	// 								status_t status, bool partialTransfer,
	// 								generic_size_t bytesTransferred);
	//		void			_IOFinished(status_t status, generic_size_t bytesTransferred);
	//		void				_RemoveAllocated(size_t unbusyCount, size_t removeCount);

private:
			block_cache_instance_data* 	fCacheInstance;
			off_t				fStartBlockNumber;
			size_t				fNumBlocksRequested;
			// unified_cache_entry** 		fEntries; // Store entries if needed during async op
			// generic_io_vec* 	fDestVecs;
};
#endif // !BUILDING_USERLAND_FS_SERVER


// TransactionLocking might not be needed if unified_cache handles its own locking.
// Or it might be for the transaction_lock in block_cache_instance_data.
// For now, let's assume it's for the instance's transaction_lock.
class TransactionInstanceLocking {
public:
	inline bool Lock(block_cache_instance_data* cacheInstance)
	{
		return mutex_lock(&cacheInstance->transaction_lock) == B_OK;
	}

	inline void Unlock(block_cache_instance_data* cacheInstance)
	{
		mutex_unlock(&cacheInstance->transaction_lock);
	}
};

typedef AutoLocker<block_cache_instance_data, TransactionInstanceLocking> TransactionInstanceLocker;

} // unnamed namespace


#if BLOCK_CACHE_BLOCK_TRACING && !defined(BUILDING_USERLAND_FS_SERVER)
namespace BlockTracing {

class Action : public AbstractTraceEntry {
public:
	Action(block_cache* cache, cached_block* block)
		:
		fCache(cache),
		fBlockNumber(block->block_number),
		fIsDirty(block->is_dirty),
		fHasOriginal(block->original_data != NULL),
		fHasParent(block->parent_data != NULL),
		fTransactionID(-1),
		fPreviousID(-1)
	{
		if (block->transaction != NULL)
			fTransactionID = block->transaction->id;
		if (block->previous_transaction != NULL)
			fPreviousID = block->previous_transaction->id;
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("block cache %p, %s %" B_PRIu64 ", %c%c%c transaction %" B_PRId32
			" (previous id %" B_PRId32 ")\n", fCache, _Action(), fBlockNumber,
			fIsDirty ? 'd' : '-', fHasOriginal ? 'o' : '-',
			fHasParent ? 'p' : '-', fTransactionID, fPreviousID);
	}

	virtual const char* _Action() const = 0;

private:
	block_cache*		fCache;
	uint64				fBlockNumber;
	bool				fIsDirty;
	bool				fHasOriginal;
	bool				fHasParent;
	int32				fTransactionID;
	int32				fPreviousID;
};

class Get : public Action {
public:
	Get(block_cache* cache, cached_block* block)
		:
		Action(cache, block)
	{
		Initialized();
	}

	virtual const char* _Action() const { return "get"; }
};

class Put : public Action {
public:
	Put(block_cache* cache, cached_block* block)
		:
		Action(cache, block)
	{
		Initialized();
	}

	virtual const char* _Action() const { return "put"; }
};

class Read : public Action {
public:
	Read(block_cache* cache, cached_block* block)
		:
		Action(cache, block)
	{
		Initialized();
	}

	virtual const char* _Action() const { return "read"; }
};

class Write : public Action {
public:
	Write(block_cache* cache, cached_block* block)
		:
		Action(cache, block)
	{
		Initialized();
	}

	virtual const char* _Action() const { return "write"; }
};

class Flush : public Action {
public:
	Flush(block_cache* cache, cached_block* block, bool getUnused = false)
		:
		Action(cache, block),
		fGetUnused(getUnused)
	{
		Initialized();
	}

	virtual const char* _Action() const
		{ return fGetUnused ? "get-unused" : "flush"; }

private:
	bool	fGetUnused;
};

class Error : public AbstractTraceEntry {
public:
	Error(block_cache* cache, uint64 blockNumber, const char* message,
			status_t status = B_OK)
		:
		fCache(cache),
		fBlockNumber(blockNumber),
		fMessage(message),
		fStatus(status)
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("block cache %p, error %" B_PRIu64 ", %s%s%s",
			fCache, fBlockNumber, fMessage, fStatus != B_OK ? ": " : "",
			fStatus != B_OK ? strerror(fStatus) : "");
	}

private:
	block_cache*	fCache;
	uint64			fBlockNumber;
	const char*		fMessage;
	status_t		fStatus;
};

#if BLOCK_CACHE_BLOCK_TRACING >= 2
class BlockData : public AbstractTraceEntry {
public:
	enum {
		kCurrent	= 0x01,
		kParent		= 0x02,
		kOriginal	= 0x04
	};

	BlockData(block_cache* cache, cached_block* block, const char* message)
		:
		fCache(cache),
		fSize(cache->block_size),
		fBlockNumber(block->block_number),
		fMessage(message)
	{
		_Allocate(fCurrent, block->current_data);
		_Allocate(fParent, block->parent_data);
		_Allocate(fOriginal, block->original_data);

#if KTRACE_PRINTF_STACK_TRACE
		fStackTrace = capture_tracing_stack_trace(KTRACE_PRINTF_STACK_TRACE, 1,
			false);
#endif

		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("block cache %p, block %" B_PRIu64 ", data %c%c%c: %s",
			fCache, fBlockNumber, fCurrent != NULL ? 'c' : '-',
			fParent != NULL ? 'p' : '-', fOriginal != NULL ? 'o' : '-',
			fMessage);
	}

#if KTRACE_PRINTF_STACK_TRACE
	virtual void DumpStackTrace(TraceOutput& out)
	{
		out.PrintStackTrace(fStackTrace);
	}
#endif

	void DumpBlocks(uint32 which, uint32 offset, uint32 size)
	{
		if ((which & kCurrent) != 0)
			DumpBlock(kCurrent, offset, size);
		if ((which & kParent) != 0)
			DumpBlock(kParent, offset, size);
		if ((which & kOriginal) != 0)
			DumpBlock(kOriginal, offset, size);
	}

	void DumpBlock(uint32 which, uint32 offset, uint32 size)
	{
		if (offset > fSize) {
			kprintf("invalid offset (block size %" B_PRIu32 ")\n", fSize);
			return;
		}
		if (offset + size > fSize)
			size = fSize - offset;

		const char* label;
		uint8* data;

		if ((which & kCurrent) != 0) {
			label = "current";
			data = fCurrent;
		} else if ((which & kParent) != 0) {
			label = "parent";
			data = fParent;
		} else if ((which & kOriginal) != 0) {
			label = "original";
			data = fOriginal;
		} else
			return;

		kprintf("%s: offset %" B_PRIu32 ", %" B_PRIu32 " bytes\n", label, offset, size);

		static const uint32 kBlockSize = 16;
		data += offset;

		for (uint32 i = 0; i < size;) {
			int start = i;

			kprintf("  %04" B_PRIx32 " ", i);
			for (; i < start + kBlockSize; i++) {
				if (!(i % 4))
					kprintf(" ");

				if (i >= size)
					kprintf("  ");
				else
					kprintf("%02x", data[i]);
			}

			kprintf("\n");
		}
	}

private:
	void _Allocate(uint8*& target, void* source)
	{
		if (source == NULL) {
			target = NULL;
			return;
		}

		target = alloc_tracing_buffer_memcpy(source, fSize, false);
	}

	block_cache*	fCache;
	uint32			fSize;
	uint64			fBlockNumber;
	const char*		fMessage;
	uint8*			fCurrent;
	uint8*			fParent;
	uint8*			fOriginal;
#if KTRACE_PRINTF_STACK_TRACE
	tracing_stack_trace* fStackTrace;
#endif
};
#endif	// BLOCK_CACHE_BLOCK_TRACING >= 2

}	// namespace BlockTracing

#	define TB(x) new(std::nothrow) BlockTracing::x;
#else
#	define TB(x) ;
#endif

#if BLOCK_CACHE_BLOCK_TRACING >= 2
#	define TB2(x) new(std::nothrow) BlockTracing::x;
#else
#	define TB2(x) ;
#endif


#if BLOCK_CACHE_TRANSACTION_TRACING && !defined(BUILDING_USERLAND_FS_SERVER)
namespace TransactionTracing {

class Action : public AbstractTraceEntry {
public:
	Action(const char* label, block_cache* cache,
			cache_transaction* transaction)
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
		out.Print("block cache %p, %s transaction %p (id %" B_PRId32 ")%s"
			", %" B_PRId32 "/%" B_PRId32 " blocks", fCache, fLabel, fTransaction,
			fID, fSub ? " sub" : "", fNumBlocks, fSubNumBlocks);
	}

private:
	char				fLabel[12];
	block_cache*		fCache;
	cache_transaction*	fTransaction;
	int32				fID;
	bool				fSub;
	int32				fNumBlocks;
	int32				fSubNumBlocks;
};

class Detach : public AbstractTraceEntry {
public:
	Detach(block_cache* cache, cache_transaction* transaction,
			cache_transaction* newTransaction)
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
		out.Print("block cache %p, detach transaction %p (id %" B_PRId32 ")"
			"from transaction %p (id %" B_PRId32 ")%s",
			fCache, fNewTransaction, fNewID, fTransaction, fID,
			fSub ? " sub" : "");
	}

private:
	block_cache*		fCache;
	cache_transaction*	fTransaction;
	int32				fID;
	bool				fSub;
	cache_transaction*	fNewTransaction;
	int32				fNewID;
};

class Abort : public AbstractTraceEntry {
public:
	Abort(block_cache* cache, cache_transaction* transaction)
		:
		fCache(cache),
		fTransaction(transaction),
		fID(transaction->id),
		fNumBlocks(0)
	{
		bool isSub = transaction->has_sub_transaction;
		fNumBlocks = isSub ? transaction->sub_num_blocks
			: transaction->num_blocks;
		fBlocks = (off_t*)alloc_tracing_buffer(fNumBlocks * sizeof(off_t));
		if (fBlocks != NULL) {
			cached_block* block = transaction->first_block;
			for (int32 i = 0; block != NULL && i < fNumBlocks;
					block = block->transaction_next) {
				fBlocks[i++] = block->block_number;
			}
		} else
			fNumBlocks = 0;

#if KTRACE_PRINTF_STACK_TRACE
		fStackTrace = capture_tracing_stack_trace(KTRACE_PRINTF_STACK_TRACE, 1,
			false);
#endif

		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("block cache %p, abort transaction "
			"%p (id %" B_PRId32 "), blocks", fCache, fTransaction, fID);
		for (int32 i = 0; i < fNumBlocks && !out.IsFull(); i++)
			out.Print(" %" B_PRIdOFF, fBlocks[i]);
	}

#if KTRACE_PRINTF_STACK_TRACE
	virtual void DumpStackTrace(TraceOutput& out)
	{
		out.PrintStackTrace(fStackTrace);
	}
#endif

private:
	block_cache*		fCache;
	cache_transaction*	fTransaction;
	int32				fID;
	off_t*				fBlocks;
	int32				fNumBlocks;
#if KTRACE_PRINTF_STACK_TRACE
	tracing_stack_trace* fStackTrace;
#endif
};

}	// namespace TransactionTracing

#	define T(x) new(std::nothrow) TransactionTracing::x;
#else
#	define T(x) ;
#endif

// TODO: sCaches list might be re-introduced if block_cache_instance_data needs global tracking.
// static DoublyLinkedList<block_cache_instance_data> sBlockCacheInstances;
// static mutex sBlockCacheInstancesLock = MUTEX_INITIALIZER("block cache instances");

static mutex sBlockCacheMemoryUseLock = MUTEX_INITIALIZER("block cache memory use");
static size_t sCurrentBlockDataMemoryUsed = 0; // Approximate memory used by block data in unified cache

// The event semaphore and notifier thread for transactions might still be needed.
static sem_id sTransactionEventSemaphore;
static mutex sTransactionNotificationsLock = MUTEX_INITIALIZER("block cache transaction notifications");
static thread_id sTransactionNotifierWriterThread;
// static DoublyLinkedListLink<block_cache_instance_data> sMarkInstance; // If iterating instances


// These static functions operated on the old 'cached_block' and 'block_cache' structs.
// They will either be removed or heavily adapted.
// static void mark_block_busy_reading(block_cache* cache, cached_block* block);
// static void mark_block_unbusy_reading(block_cache* cache, cached_block* block);

// TODO: Placeholder for reading a block from disk into a provided buffer.
// This will be used when unified_cache_get_entry results in a miss.
static status_t
_read_block_from_disk(block_cache_instance_data* instance, off_t blockNumber, void* buffer)
{
    TRACE(("_read_block_from_disk: instance %p, block %lld, buffer %p\n", instance, blockNumber, buffer));
    ssize_t bytesRead = read_pos(instance->fd, blockNumber * instance->block_size,
        buffer, instance->block_size);

    if (bytesRead < 0)
        return errno;
    if ((size_t)bytesRead != instance->block_size)
        return B_IO_ERROR; // Partial read

    return B_OK;
}

// TODO: Placeholder for writing a block from a buffer to disk.
// This will be used for dirty blocks during sync or eviction.
static status_t
_write_block_to_disk(block_cache_instance_data* instance, off_t blockNumber, const void* buffer)
{
    TRACE(("_write_block_to_disk: instance %p, block %lld, buffer %p\n", instance, blockNumber, buffer));
    ssize_t bytesWritten = write_pos(instance->fd, blockNumber * instance->block_size,
        buffer, instance->block_size);

    if (bytesWritten < 0)
        return errno;
    if ((size_t)bytesWritten != instance->block_size)
        return B_IO_ERROR; // Partial write

    return B_OK;
}


//	#pragma mark - notifications/listener


/*!	Checks whether or not this is an event that closes a transaction. */
static inline bool
is_closing_event(int32 event)
{
	return (event & (TRANSACTION_ABORTED | TRANSACTION_ENDED)) != 0;
}


static inline bool
is_written_event(int32 event)
{
	return (event & TRANSACTION_WRITTEN) != 0;
}


/*!	From the specified \a notification, it will remove the lowest pending
	event, and return that one in \a _event.
	If there is no pending event anymore, it will return \c false.
*/
static bool
get_next_pending_event(cache_notification* notification, int32* _event)
{
	for (int32 eventMask = 1; eventMask <= TRANSACTION_IDLE; eventMask <<= 1) {
		int32 pending = atomic_and(&notification->events_pending,
			~eventMask);

		bool more = (pending & ~eventMask) != 0;

		if ((pending & eventMask) != 0) {
			*_event = eventMask;
			return more;
		}
	}

	return false;
}


static void
flush_pending_notifications(block_cache* cache)
{
	ASSERT_LOCKED_MUTEX(&sCachesLock);

	while (true) {
		MutexLocker locker(sNotificationsLock);

		cache_notification* notification = cache->pending_notifications.Head();
		if (notification == NULL)
			return;

		bool deleteAfterEvent = false;
		int32 event = -1;
		if (!get_next_pending_event(notification, &event)) {
			// remove the notification if this was the last pending event
			cache->pending_notifications.Remove(notification);
			deleteAfterEvent = notification->delete_after_event;
		}

		if (event >= 0) {
			// Notify listener, we need to copy the notification, as it might
			// be removed when we unlock the list.
			cache_notification copy = *notification;
			locker.Unlock();

			copy.hook(copy.transaction_id, event, copy.data);

			locker.Lock();
		}

		if (deleteAfterEvent)
			delete notification;
	}
}


/*!	Flushes all pending notifications by calling the appropriate hook
	functions.
	Must not be called with a cache lock held.
*/
static void
flush_pending_notifications()
{
	MutexLocker _(sCachesLock);

	DoublyLinkedList<block_cache>::Iterator iterator = sCaches.GetIterator();
	while (iterator.HasNext()) {
		block_cache* cache = iterator.Next();

		flush_pending_notifications(cache);
	}
}


/*!	Initializes the \a notification as specified. */
static void
set_notification(cache_transaction* transaction,
	cache_notification &notification, int32 events,
	transaction_notification_hook hook, void* data)
{
	notification.transaction_id = transaction != NULL ? transaction->id : -1;
	notification.events_pending = 0;
	notification.events = events;
	notification.hook = hook;
	notification.data = data;
	notification.delete_after_event = false;
}


/*!	Makes sure the notification is deleted. It either deletes it directly,
	when possible, or marks it for deletion if the notification is pending.
*/
static void
delete_notification(cache_notification* notification)
{
	MutexLocker locker(sNotificationsLock);

	if (notification->events_pending != 0)
		notification->delete_after_event = true;
	else
		delete notification;
}


/*!	Adds the notification to the pending notifications list, or, if it's
	already part of it, updates its events_pending field.
	Also marks the notification to be deleted if \a deleteNotification
	is \c true.
	Triggers the notifier thread to run.
*/
static void
add_notification(block_cache* cache, cache_notification* notification,
	int32 event, bool deleteNotification)
{
	if (notification->hook == NULL)
		return;

	int32 pending = atomic_or(&notification->events_pending, event);
	if (pending == 0) {
		// not yet part of the notification list
		MutexLocker locker(sNotificationsLock);
		if (deleteNotification)
			notification->delete_after_event = true;
		cache->pending_notifications.Add(notification);
	} else if (deleteNotification) {
		// we might need to delete it ourselves if we're late
		delete_notification(notification);
	}

	release_sem_etc(sEventSemaphore, 1, B_DO_NOT_RESCHEDULE);
		// We're probably still holding some locks that makes rescheduling
		// not a good idea at this point.
}


/*!	Notifies all interested listeners of this transaction about the \a event.
	If \a event is a closing event (ie. TRANSACTION_ENDED, and
	TRANSACTION_ABORTED), all listeners except those listening to
	TRANSACTION_WRITTEN will be removed.
*/
static void
notify_transaction_listeners(block_cache* cache, cache_transaction* transaction,
	int32 event)
{
	T(Action("notify", cache, transaction));

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


/*!	Removes and deletes all listeners that are still monitoring this
	transaction.
*/
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


static status_t
add_transaction_listener(block_cache* cache, cache_transaction* transaction,
	int32 events, transaction_notification_hook hookFunction, void* data)
{
	ListenerList::Iterator iterator = transaction->listeners.GetIterator();
	while (iterator.HasNext()) {
		cache_listener* listener = iterator.Next();

		if (listener->data == data && listener->hook == hookFunction) {
			// this listener already exists, just update it
			listener->events |= events;
			return B_OK;
		}
	}

	cache_listener* listener = new cache_listener;
	if (listener == NULL)
		return B_NO_MEMORY;

	set_notification(transaction, *listener, events, hookFunction, data);
	transaction->listeners.Add(listener);
	return B_OK;
}


//	#pragma mark - private transaction


cache_transaction::cache_transaction()
{
	num_blocks = 0;
	main_num_blocks = 0;
	sub_num_blocks = 0;
	first_block = NULL;
	open = true;
	has_sub_transaction = false;
	last_used = system_time();
	busy_writing_count = 0;
}


static void
delete_transaction(block_cache* cache, cache_transaction* transaction)
{
	if (cache->last_transaction == transaction)
		cache->last_transaction = NULL;

	remove_transaction_listeners(cache, transaction);
	delete transaction;
}


static cache_transaction*
lookup_transaction(block_cache* cache, int32 id)
{
	return cache->transaction_hash->Lookup(id);
}


size_t TransactionHash::Hash(cache_transaction* transaction) const
{
	return transaction->id;
}


bool TransactionHash::Compare(int32 key, cache_transaction* transaction) const
{
	return transaction->id == key;
}


cache_transaction*& TransactionHash::GetLink(cache_transaction* value) const
{
	return value->next;
}


/*!	Writes back any changes made to blocks in \a transaction that are still
	part of a previous transacton.
*/
static status_t
write_blocks_in_previous_transaction(block_cache* cache,
	cache_transaction* transaction)
{
	BlockWriter writer(cache);

	cached_block* block = transaction->first_block;
	for (; block != NULL; block = block->transaction_next) {
		if (block->previous_transaction != NULL) {
			// need to write back pending changes
			writer.Add(block);
		}
	}

	return writer.Write();
}


//	#pragma mark - cached_block


bool
cached_block::CanBeWritten() const
{
	return !busy_writing && !busy_reading
		&& (previous_transaction != NULL
			|| (transaction == NULL && is_dirty && !is_writing));
}


//	#pragma mark - BlockWriter


BlockWriter::BlockWriter(block_cache* cache, size_t max)
	:
	fCache(cache),
	fBlocks(fBuffer),
	fCount(0),
	fTotal(0),
	fCapacity(kBufferSize),
	fMax(max),
	fStatus(B_OK),
	fDeletedTransaction(false)
{
}


BlockWriter::~BlockWriter()
{
	if (fBlocks != fBuffer)
		free(fBlocks);
}


/*!	Adds the specified block to the to be written array. If no more blocks can
	be added, false is returned, otherwise true.
*/
bool
BlockWriter::Add(cached_block* block, cache_transaction* transaction)
{
	ASSERT(block->CanBeWritten());

	if (fTotal == fMax)
		return false;

	if (fCount >= fCapacity) {
		// Enlarge array if necessary
		cached_block** newBlocks;
		size_t newCapacity = max_c(256, fCapacity * 2);
		if (fBlocks == fBuffer)
			newBlocks = (cached_block**)malloc(newCapacity * sizeof(void*));
		else {
			newBlocks = (cached_block**)realloc(fBlocks,
				newCapacity * sizeof(void*));
		}

		if (newBlocks == NULL) {
			// Allocating a larger array failed - we need to write back what
			// we have synchronously now (this will also clear the array)
			Write(transaction, false);
		} else {
			if (fBlocks == fBuffer)
				memcpy(newBlocks, fBuffer, kBufferSize * sizeof(void*));

			fBlocks = newBlocks;
			fCapacity = newCapacity;
		}
	}

	fBlocks[fCount++] = block;
	fTotal++;
	block->busy_writing = true;
	fCache->busy_writing_count++;
	if (block->previous_transaction != NULL)
		block->previous_transaction->busy_writing_count++;

	return true;
}


/*!	Adds all blocks of the specified transaction to the to be written array.
	If no more blocks can be added, false is returned, otherwise true.
*/
bool
BlockWriter::Add(cache_transaction* transaction, bool& hasLeftOvers)
{
	ASSERT(!transaction->open);

	if (transaction->busy_writing_count != 0) {
		hasLeftOvers = true;
		return true;
	}

	hasLeftOvers = false;

	block_list::Iterator blockIterator = transaction->blocks.GetIterator();
	while (cached_block* block = blockIterator.Next()) {
		if (!block->CanBeWritten()) {
			// This block was already part of a previous transaction within this
			// writer
			hasLeftOvers = true;
			continue;
		}
		if (!Add(block, transaction))
			return false;

		if (DeletedTransaction())
			break;
	}

	return true;
}


/*! Cache must be locked when calling this method, but it will be unlocked
	while the blocks are written back.
*/
status_t
BlockWriter::Write(cache_transaction* transaction, bool canUnlock)
{
	if (fCount == 0)
		return B_OK;

	if (canUnlock)
		mutex_unlock(&fCache->lock);

	// Sort blocks in their on-disk order, so we can merge consecutive writes.
	qsort(fBlocks, fCount, sizeof(void*), &_CompareBlocks);
	fDeletedTransaction = false;

	bigtime_t start = system_time();

	for (uint32 i = 0; i < fCount; i++) {
		uint32 blocks = 1;
		for (; (i + blocks) < fCount && blocks < IOV_MAX; blocks++) {
			const uint32 j = i + blocks;
			if (fBlocks[j]->block_number != (fBlocks[j - 1]->block_number + 1))
				break;
		}

		status_t status = _WriteBlocks(fBlocks + i, blocks);
		if (status != B_OK) {
			// propagate to global error handling
			if (fStatus == B_OK)
				fStatus = status;

			for (uint32 j = i; j < (i + blocks); j++) {
				_UnmarkWriting(fBlocks[j]);
				fBlocks[j] = NULL;
					// This block will not be marked clean
			}
		}

		i += (blocks - 1);
	}

	bigtime_t finish = system_time();

	if (canUnlock)
		mutex_lock(&fCache->lock);

	if (fStatus == B_OK && fCount >= 8) {
		fCache->last_block_write = finish;
		fCache->last_block_write_duration = (fCache->last_block_write - start)
			/ fCount;
	}

	for (uint32 i = 0; i < fCount; i++)
		_BlockDone(fBlocks[i], transaction);

	fCount = 0;
	return fStatus;
}


/*!	Writes the specified \a block back to disk. It will always only write back
	the oldest change of the block if it is part of more than one transaction.
	It will automatically send out TRANSACTION_WRITTEN notices, as well as
	delete transactions when they are no longer used, and \a deleteTransaction
	is \c true.
*/
/*static*/ status_t
BlockWriter::WriteBlock(block_cache* cache, cached_block* block)
{
	BlockWriter writer(cache);

	writer.Add(block);
	return writer.Write();
}


void*
BlockWriter::_Data(cached_block* block) const
{
	return block->previous_transaction != NULL && block->original_data != NULL
		? block->original_data : block->current_data;
		// We first need to write back changes from previous transactions
}


status_t
BlockWriter::_WriteBlocks(cached_block** blocks, uint32 count)
{
	const size_t blockSize = fCache->block_size;

	BStackOrHeapArray<iovec, 8> vecs(count);
	for (uint32 i = 0; i < count; i++) {
		cached_block* block = blocks[i];
		ASSERT(block->busy_writing);
		ASSERT(i == 0 || block->block_number == (blocks[i - 1]->block_number + 1));

		TRACE(("BlockWriter::_WriteBlocks(block %" B_PRIdOFF ", count %" B_PRIu32 ")\n",
			block->block_number, count));
		TB(Write(fCache, block));
		TB2(BlockData(fCache, block, "before write"));

		vecs[i].iov_base = _Data(block);
		vecs[i].iov_len = blockSize;
	}

	ssize_t written = writev_pos(fCache->fd,
		blocks[0]->block_number * blockSize, vecs, count);

	if (written != (ssize_t)(blockSize * count)) {
		TB(Error(fCache, block->block_number, "write failed", written));
		status_t error = errno;
		TRACE_ALWAYS("could not write back %" B_PRIu32 " blocks (start block %" B_PRIdOFF
			"): %s\n", count, blocks[0]->block_number, strerror(error));
		if (written < 0 && error != 0)
			return error;
		return B_IO_ERROR;
	}

	return B_OK;
}


void
BlockWriter::_BlockDone(cached_block* block,
	cache_transaction* transaction)
{
	if (block == NULL) {
		// An error occured when trying to write this block
		return;
	}

	if (fCache->num_dirty_blocks > 0)
		fCache->num_dirty_blocks--;

	if (_Data(block) == block->current_data)
		block->is_dirty = false;

	_UnmarkWriting(block);

	cache_transaction* previous = block->previous_transaction;
	if (previous != NULL) {
		previous->blocks.Remove(block);
		block->previous_transaction = NULL;

		if (block->original_data != NULL && block->transaction == NULL) {
			// This block is not part of a transaction, so it does not need
			// its original pointer anymore.
			fCache->Free(block->original_data);
			block->original_data = NULL;
		}

		// Has the previous transaction been finished with that write?
		if (--previous->num_blocks == 0) {
			TRACE(("cache transaction %" B_PRId32 " finished!\n", previous->id));
			T(Action("written", fCache, previous));

			notify_transaction_listeners(fCache, previous,
				TRANSACTION_WRITTEN);

			if (transaction != NULL) {
				// This function is called while iterating transaction_hash. We
				// use RemoveUnchecked so the iterator is still valid. A regular
				// Remove can trigger a resize of the hash table which would
				// result in the linked items in the table changing order.
				fCache->transaction_hash->RemoveUnchecked(transaction);
			} else
				fCache->transaction_hash->Remove(previous);

			delete_transaction(fCache, previous);
			fDeletedTransaction = true;
		}
	}
	if (block->transaction == NULL && block->ref_count == 0 && !block->unused) {
		// the block is no longer used
		ASSERT(block->original_data == NULL && block->parent_data == NULL);
		block->unused = true;
		fCache->unused_blocks.Add(block);
		fCache->unused_block_count++;
	}

	TB2(BlockData(fCache, block, "after write"));
}


void
BlockWriter::_UnmarkWriting(cached_block* block)
{
	block->busy_writing = false;
	if (block->previous_transaction != NULL)
		block->previous_transaction->busy_writing_count--;
	fCache->busy_writing_count--;

	if ((fCache->busy_writing_waiters && fCache->busy_writing_count == 0)
		|| block->busy_writing_waiters) {
		fCache->busy_writing_waiters = false;
		block->busy_writing_waiters = false;
		fCache->busy_writing_condition.NotifyAll();
	}
}


/*static*/ int
BlockWriter::_CompareBlocks(const void* _blockA, const void* _blockB)
{
	cached_block* blockA = *(cached_block**)_blockA;
	cached_block* blockB = *(cached_block**)_blockB;

	off_t diff = blockA->block_number - blockB->block_number;
	if (diff > 0)
		return 1;

	return diff < 0 ? -1 : 0;
}


#ifndef BUILDING_USERLAND_FS_SERVER
//	#pragma mark - BlockPrefetcher


BlockPrefetcher::BlockPrefetcher(block_cache* cache, off_t blockNumber, size_t numBlocks)
	:
	fCache(cache),
	fBlockNumber(blockNumber),
	fNumRequested(numBlocks),
	fNumAllocated(0)
{
	fBlocks = new cached_block*[numBlocks];
	fDestVecs = new generic_io_vec[numBlocks];
}


BlockPrefetcher::~BlockPrefetcher()
{
	delete[] fBlocks;
	delete[] fDestVecs;
}


/*!	Allocates cached_block objects in preparation for prefetching.
	@return If an error is returned, then no blocks have been allocated.
	@post Blocks have been constructed (including allocating the current_data member)
	but current_data is uninitialized.
*/
status_t
BlockPrefetcher::Allocate()
{
	TRACE(("BlockPrefetcher::Allocate: looking up %" B_PRIuSIZE " blocks, starting with %"
		B_PRIdOFF "\n", fNumBlocks, fBlockNumber));

	ASSERT_LOCKED_MUTEX(&fCache->lock);

	size_t finalNumBlocks = fNumRequested;

	// determine whether any requested blocks are already cached
	for (size_t i = 0; i < fNumRequested; ++i) {
		off_t blockNumIter = fBlockNumber + i;
		if (blockNumIter < 0 || blockNumIter >= fCache->max_blocks) {
			panic("BlockPrefetcher::Allocate: invalid block number %" B_PRIdOFF " (max %"
				B_PRIdOFF ")", blockNumIter, fCache->max_blocks - 1);
			return B_BAD_VALUE;
		}
		cached_block* block = fCache->hash->Lookup(blockNumIter);
		if (block != NULL) {
			// truncate the request
			TRACE(("BlockPrefetcher::Allocate: found an existing block (%" B_PRIdOFF ")\n",
				blockNumIter));
			fBlocks[i] = NULL;
			finalNumBlocks = i;
			break;
		}
	}

	// allocate the blocks
	for (size_t i = 0; i < finalNumBlocks; ++i) {
		cached_block* block = fCache->NewBlock(fBlockNumber + i);
		if (block == NULL) {
			_RemoveAllocated(0, i);
			return B_NO_MEMORY;
		}
		fCache->hash->Insert(block);

		block->unused = true;
		fCache->unused_blocks.Add(block);
		fCache->unused_block_count++;

		fBlocks[i] = block;
	}

	fNumAllocated = finalNumBlocks;

	return B_OK;
}


/*!	Schedules reads from disk to cache.
	\post The calling object will eventually be deleted by IOFinishedCallback.
*/
status_t
BlockPrefetcher::ReadAsync(MutexLocker& cacheLocker)
{
	TRACE(("BlockPrefetcher::Read: reading %" B_PRIuSIZE " blocks\n", fNumAllocated));

	size_t blockSize = fCache->block_size;
	generic_io_vec* vecs = fDestVecs;
	for (size_t i = 0; i < fNumAllocated; ++i) {
		vecs[i].base = reinterpret_cast<generic_addr_t>(fBlocks[i]->current_data);
		vecs[i].length = blockSize;
		mark_block_busy_reading(fCache, fBlocks[i]);
	}

	IORequest* request = new IORequest;
	status_t status = request->Init(fBlockNumber * blockSize, vecs, fNumAllocated,
		fNumAllocated * blockSize, false, B_DELETE_IO_REQUEST);
	if (status != B_OK) {
		TB(Error(fCache, fBlockNumber, "IORequest::Init starting here failed", status));
		TRACE_ALWAYS("BlockPrefetcher::Read: failed to initialize IO request for %" B_PRIuSIZE
			" blocks starting with %" B_PRIdOFF ": %s\n",
			fNumAllocated, fBlockNumber, strerror(status));

		_RemoveAllocated(fNumAllocated, fNumAllocated);
		delete request;
		return status;
	}

	request->SetFinishedCallback(_IOFinishedCallback, this);

	// do_fd_io() may invoke callbacks directly, so we need to have unlocked the cache.
	cacheLocker.Unlock();

	return do_fd_io(fCache->fd, request);
}


/*static*/ void
BlockPrefetcher::_IOFinishedCallback(void* cookie, io_request* request, status_t status,
	bool partialTransfer, generic_size_t bytesTransferred)
{
	TRACE(("BlockPrefetcher::_IOFinishedCallback: status %s, partial %d\n",
		strerror(status), partialTransfer));
	((BlockPrefetcher*)cookie)->_IOFinished(status, bytesTransferred);
}


void
BlockPrefetcher::_IOFinished(status_t status, generic_size_t bytesTransferred)
{
	MutexLocker locker(&fCache->lock);

	if (bytesTransferred < (fNumAllocated * fCache->block_size)) {
		_RemoveAllocated(fNumAllocated, fNumAllocated);

		TB(Error(cache, fBlockNumber, "prefetch starting here failed", status));
		TRACE_ALWAYS("BlockPrefetcher::_IOFinished: transferred only %" B_PRIuGENADDR
			" bytes in attempt to read %" B_PRIuSIZE " blocks (start block %" B_PRIdOFF "): %s\n",
			bytesTransferred, fNumAllocated, fBlockNumber, strerror(status));
	} else {
		for (size_t i = 0; i < fNumAllocated; i++) {
			TB(Read(cache, fBlockNumber + i));
			mark_block_unbusy_reading(fCache, fBlocks[i]);
			fBlocks[i]->last_accessed = system_time() / 1000000L;
		}
	}

	delete this;
}


/*!	Cleans up blocks that were allocated for prefetching when an in-progress prefetch
	is cancelled.
*/
void
BlockPrefetcher::_RemoveAllocated(size_t unbusyCount, size_t removeCount)
{
	TRACE(("BlockPrefetcher::_RemoveAllocated:  unbusy %" B_PRIuSIZE " and remove %" B_PRIuSIZE
		" starting with %" B_PRIdOFF "\n", unbusyCount, removeCount, (*fBlocks)->block_number));

	ASSERT_LOCKED_MUTEX(&fCache->lock);

	for (size_t i = 0; i < unbusyCount; ++i)
		mark_block_unbusy_reading(fCache, fBlocks[i]);

	for (size_t i = 0; i < removeCount; ++i) {
		ASSERT(fBlocks[i]->is_dirty == false && fBlocks[i]->unused == true);

		fCache->unused_blocks.Remove(fBlocks[i]);
		fCache->unused_block_count--;

		fCache->RemoveBlock(fBlocks[i]);
		fBlocks[i] = NULL;
	}

	fNumAllocated = 0;

	return;
}
#endif // !BUILDING_USERLAND_FS_SERVER


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
	buffer_cache(NULL),
	unused_block_count(0),
	busy_reading_count(0),
	busy_reading_waiters(false),
	busy_writing_count(0),
	busy_writing_waiters(0),
	last_block_write(0),
	last_block_write_duration(0),
	num_dirty_blocks(0),
	read_only(readOnly)
{
}


/*! Should be called with the cache's lock held. */
block_cache::~block_cache()
{
	unregister_low_resource_handler(&_LowMemoryHandler, this);

	delete transaction_hash;
	delete hash;

	delete_object_cache(buffer_cache);

	mutex_destroy(&lock);
}


status_t
block_cache::Init()
{
	busy_reading_condition.Init(this, "cache block busy_reading");
	busy_writing_condition.Init(this, "cache block busy writing");
	condition_variable.Init(this, "cache transaction sync");
	mutex_init(&lock, "block cache");

	buffer_cache = create_object_cache("block cache buffers", block_size,
		CACHE_NO_DEPOT | CACHE_LARGE_SLAB);
	if (buffer_cache == NULL)
		return B_NO_MEMORY;

	hash = new BlockTable();
	if (hash == NULL || hash->Init(1024) != B_OK)
		return B_NO_MEMORY;

	transaction_hash = new(std::nothrow) TransactionTable();
	if (transaction_hash == NULL || transaction_hash->Init(16) != B_OK)
		return B_NO_MEMORY;

	return register_low_resource_handler(&_LowMemoryHandler, this,
		B_KERNEL_RESOURCE_PAGES | B_KERNEL_RESOURCE_MEMORY
			| B_KERNEL_RESOURCE_ADDRESS_SPACE, 0);
}


void
block_cache::Free(void* buffer)
{
	if (buffer != NULL)
		object_cache_free(buffer_cache, buffer, 0);
}


void*
block_cache::Allocate()
{
	void* block = object_cache_alloc(buffer_cache, 0);
	if (block != NULL)
		return block;

	// recycle existing before allocating a new one
	RemoveUnusedBlocks(100);

	return object_cache_alloc(buffer_cache, 0);
}


void
block_cache::FreeBlock(cached_block* block)
{
	Free(block->current_data);

	if (block->original_data != NULL || block->parent_data != NULL) {
		panic("block_cache::FreeBlock(): %" B_PRIdOFF ", original %p, parent %p\n",
			block->block_number, block->original_data, block->parent_data);
	}

#if BLOCK_CACHE_DEBUG_CHANGED
	Free(block->compare);
#endif

	object_cache_free(sBlockCache, block, 0);
}


/*! Allocates a new block for \a blockNumber, ready for use */
cached_block*
block_cache::NewBlock(off_t blockNumber)
{
	cached_block* block = NULL;

	if (low_resource_state(B_KERNEL_RESOURCE_PAGES | B_KERNEL_RESOURCE_MEMORY
			| B_KERNEL_RESOURCE_ADDRESS_SPACE) != B_NO_LOW_RESOURCE) {
		// recycle existing instead of allocating a new one
		block = _GetUnusedBlock();
	}
	if (block == NULL) {
		block = (cached_block*)object_cache_alloc(sBlockCache, 0);
		if (block != NULL) {
			block->current_data = Allocate();
			if (block->current_data == NULL) {
				object_cache_free(sBlockCache, block, 0);
				return NULL;
			}
		} else {
			TB(Error(this, blockNumber, "allocation failed"));
			TRACE_ALWAYS("block allocation failed, unused list is %sempty.\n",
				unused_blocks.IsEmpty() ? "" : "not ");

			// allocation failed, try to reuse an unused block
			block = _GetUnusedBlock();
			if (block == NULL) {
				TB(Error(this, blockNumber, "get unused failed"));
				FATAL(("could not allocate block!\n"));
				return NULL;
			}
		}
	}

	block->block_number = blockNumber;
	block->ref_count = 0;
	block->last_accessed = 0;
	block->transaction_next = NULL;
	block->transaction = block->previous_transaction = NULL;
	block->original_data = NULL;
	block->parent_data = NULL;
	block->busy_reading = false;
	block->busy_writing = false;
	block->is_writing = false;
	block->is_dirty = false;
	block->unused = false;
	block->discard = false;
	block->busy_reading_waiters = false;
	block->busy_writing_waiters = false;
#if BLOCK_CACHE_DEBUG_CHANGED
	block->compare = NULL;
#endif

	return block;
}


void
block_cache::FreeBlockParentData(cached_block* block)
{
	ASSERT(block->parent_data != NULL);
	if (block->parent_data != block->current_data)
		Free(block->parent_data);
	block->parent_data = NULL;
}


void
block_cache::RemoveUnusedBlocks(int32 count, int32 minSecondsOld)
{
	TRACE(("block_cache: remove up to %" B_PRId32 " unused blocks\n", count));

	for (block_list::Iterator iterator = unused_blocks.GetIterator();
			cached_block* block = iterator.Next();) {
		if (minSecondsOld >= block->LastAccess()) {
			// The list is sorted by last access
			break;
		}
		if (block->busy_reading || block->busy_writing)
			continue;

		TB(Flush(this, block));
		TRACE(("  remove block %" B_PRIdOFF ", last accessed %" B_PRId32 "\n",
			block->block_number, block->last_accessed));

		// this can only happen if no transactions are used
		if (block->is_dirty && !block->discard) {
			if (block->busy_writing)
				continue;

			BlockWriter::WriteBlock(this, block);
		}

		// remove block from lists
		iterator.Remove();
		unused_block_count--;
		RemoveBlock(block);

		if (--count <= 0)
			break;
	}
}


void
block_cache::RemoveBlock(cached_block* block)
{
	hash->Remove(block);
	FreeBlock(block);
}


/*!	Discards the block from a transaction (this method must not be called
	for blocks not part of a transaction).
*/
void
block_cache::DiscardBlock(cached_block* block)
{
	ASSERT(block->discard);
	ASSERT(block->previous_transaction == NULL);

	if (block->parent_data != NULL)
		FreeBlockParentData(block);

	if (block->original_data != NULL) {
		Free(block->original_data);
		block->original_data = NULL;
	}

	RemoveBlock(block);
}


void
block_cache::_LowMemoryHandler(void* data, uint32 resources, int32 level)
{
	TRACE(("block_cache: low memory handler called with level %" B_PRId32 "\n", level));

	// free some blocks according to the low memory state
	// (if there is enough memory left, we don't free any)

	block_cache* cache = (block_cache*)data;
	if (cache->unused_block_count <= 1)
		return;

	int32 free = 0;
	int32 secondsOld = 0;
	switch (level) {
		case B_NO_LOW_RESOURCE:
			return;
		case B_LOW_RESOURCE_NOTE:
			free = cache->unused_block_count / 4;
			secondsOld = 120;
			break;
		case B_LOW_RESOURCE_WARNING:
			free = cache->unused_block_count / 2;
			secondsOld = 10;
			break;
		case B_LOW_RESOURCE_CRITICAL:
			free = cache->unused_block_count - 1;
			secondsOld = 0;
			break;
	}

	MutexLocker locker(&cache->lock);

	if (!locker.IsLocked()) {
		// If our block_cache were deleted, it could be that we had
		// been called before that deletion went through, therefore,
		// acquiring its lock might fail.
		return;
	}

#ifdef TRACE_BLOCK_CACHE
	uint32 oldUnused = cache->unused_block_count;
#endif

	cache->RemoveUnusedBlocks(free, secondsOld);

	TRACE(("block_cache::_LowMemoryHandler(): %p: unused: %" B_PRIu32 " -> %" B_PRIu32 "\n",
		cache, oldUnused, cache->unused_block_count));
}


cached_block*
block_cache::_GetUnusedBlock()
{
	TRACE(("block_cache: get unused block\n"));

	for (block_list::Iterator iterator = unused_blocks.GetIterator();
			cached_block* block = iterator.Next();) {
		TB(Flush(this, block, true));
		// this can only happen if no transactions are used
		if (block->is_dirty && !block->busy_writing && !block->discard)
			BlockWriter::WriteBlock(this, block);

		// remove block from lists
		iterator.Remove();
		unused_block_count--;
		hash->Remove(block);

		ASSERT(block->original_data == NULL && block->parent_data == NULL);
		block->unused = false;

		// TODO: see if compare data is handled correctly here!
#if BLOCK_CACHE_DEBUG_CHANGED
		if (block->compare != NULL)
			Free(block->compare);
#endif
		return block;
	}

	return NULL;
}


//	#pragma mark - private block functions


/*!	Cache must be locked.
*/
static void
mark_block_busy_reading(block_cache* cache, cached_block* block)
{
	block->busy_reading = true;
	cache->busy_reading_count++;
}


/*!	Cache must be locked.
*/
static void
mark_block_unbusy_reading(block_cache* cache, cached_block* block)
{
	block->busy_reading = false;
	cache->busy_reading_count--;

	if ((cache->busy_reading_waiters && cache->busy_reading_count == 0)
			|| block->busy_reading_waiters) {
		cache->busy_reading_waiters = false;
		block->busy_reading_waiters = false;
		cache->busy_reading_condition.NotifyAll();
	}
}


/*!	Cache must be locked.
*/
static void
wait_for_busy_reading_block(block_cache* cache, cached_block* block)
{
	while (block->busy_reading) {
		// wait for at least the specified block to be read in
		ConditionVariableEntry entry;
		cache->busy_reading_condition.Add(&entry);
		block->busy_reading_waiters = true;

		mutex_unlock(&cache->lock);
		entry.Wait();
		mutex_lock(&cache->lock);
	}
}


/*!	Cache must be locked.
*/
static void
wait_for_busy_reading_blocks(block_cache* cache)
{
	while (cache->busy_reading_count != 0) {
		// wait for all blocks to be read in
		ConditionVariableEntry entry;
		cache->busy_reading_condition.Add(&entry);
		cache->busy_reading_waiters = true;

		mutex_unlock(&cache->lock);
		entry.Wait();
		mutex_lock(&cache->lock);
	}
}


/*!	Cache must be locked.
*/
static void
wait_for_busy_writing_block(block_cache* cache, cached_block* block)
{
	while (block->busy_writing) {
		// wait for all blocks to be written back
		ConditionVariableEntry entry;
		cache->busy_writing_condition.Add(&entry);
		block->busy_writing_waiters = true;

		mutex_unlock(&cache->lock);
		entry.Wait();
		mutex_lock(&cache->lock);
	}
}


/*!	Cache must be locked.
*/
static void
wait_for_busy_writing_blocks(block_cache* cache)
{
	while (cache->busy_writing_count != 0) {
		// wait for all blocks to be written back
		ConditionVariableEntry entry;
		cache->busy_writing_condition.Add(&entry);
		cache->busy_writing_waiters = true;

		mutex_unlock(&cache->lock);
		entry.Wait();
		mutex_lock(&cache->lock);
	}
}


/*!	Removes a reference from the specified \a block. If this was the last
	reference, the block is moved into the unused list.
	In low memory situations, it will also free some blocks from that list,
	but not necessarily the \a block it just released.
*/
static void
put_cached_block(block_cache* cache, cached_block* block)
{
#if BLOCK_CACHE_DEBUG_CHANGED
	if (!block->is_dirty && block->compare != NULL
		&& memcmp(block->current_data, block->compare, cache->block_size)) {
		TRACE_ALWAYS("new block:\n");
		dump_block((const char*)block->current_data, 256, "  ");
		TRACE_ALWAYS("unchanged block:\n");
		dump_block((const char*)block->compare, 256, "  ");
		BlockWriter::WriteBlock(cache, block);
		panic("block_cache: supposed to be clean block was changed!\n");

		cache->Free(block->compare);
		block->compare = NULL;
	}
#endif
	TB(Put(cache, block));

	if (block->ref_count < 1) {
		panic("Invalid ref_count for block %p, cache %p\n", block, cache);
		return;
	}

	if (--block->ref_count == 0
		&& block->transaction == NULL && block->previous_transaction == NULL) {
		// This block is not used anymore, and not part of any transaction
		block->is_writing = false;

		if (block->discard) {
			cache->RemoveBlock(block);
		} else {
			// put this block in the list of unused blocks
			ASSERT(!block->unused);
			block->unused = true;

			ASSERT(block->original_data == NULL && block->parent_data == NULL);
			cache->unused_blocks.Add(block);
			cache->unused_block_count++;
		}
	}
}


static void
put_cached_block(block_cache* cache, off_t blockNumber)
{
	if (blockNumber < 0 || blockNumber >= cache->max_blocks) {
		panic("put_cached_block: invalid block number %" B_PRIdOFF " (max %" B_PRIdOFF ")",
			blockNumber, cache->max_blocks - 1);
	}

	cached_block* block = cache->hash->Lookup(blockNumber);
	if (block != NULL)
		put_cached_block(cache, block);
	else {
		TB(Error(cache, blockNumber, "put unknown"));
	}
}


/*!	Retrieves the block \a blockNumber from the hash table, if it's already
	there, or reads it from the disk.
	You need to have the cache locked when calling this function.

	\param _allocated tells you whether or not a new block has been allocated
		to satisfy your request.
	\param readBlock if \c false, the block will not be read in case it was
		not already in the cache. The block you retrieve may contain random
		data. If \c true, the cache will be temporarily unlocked while the
		block is read in.
*/
static status_t
get_cached_block(block_cache* cache, off_t blockNumber, bool* _allocated,
	bool readBlock, cached_block** _block)
{
	ASSERT_LOCKED_MUTEX(&cache->lock);

	if (blockNumber < 0 || blockNumber >= cache->max_blocks) {
		panic("get_cached_block: invalid block number %" B_PRIdOFF " (max %" B_PRIdOFF ")",
			blockNumber, cache->max_blocks - 1);
		return B_BAD_VALUE;
	}

retry:
	cached_block* block = cache->hash->Lookup(blockNumber);
	*_allocated = false;

	if (block == NULL) {
		// put block into cache
		block = cache->NewBlock(blockNumber);
		if (block == NULL)
			return B_NO_MEMORY;

		cache->hash->Insert(block);
		*_allocated = true;
	} else if (block->busy_reading) {
		// The block is currently busy_reading - wait and try again later
		wait_for_busy_reading_block(cache, block);

		// The block may have been deleted or replaced in the meantime,
		// so we must look it up in the hash again after waiting.
		goto retry;
	}

	if (block->unused) {
		//TRACE(("remove block %" B_PRIdOFF " from unused\n", blockNumber));
		block->unused = false;
		cache->unused_blocks.Remove(block);
		cache->unused_block_count--;
	}

	if (*_allocated && readBlock) {
		// read block into cache
		int32 blockSize = cache->block_size;

		mark_block_busy_reading(cache, block);
		mutex_unlock(&cache->lock);

		ssize_t bytesRead = read_pos(cache->fd, blockNumber * blockSize,
			block->current_data, blockSize);

		mutex_lock(&cache->lock);
		if (bytesRead < blockSize) {
			cache->RemoveBlock(block);
			TB(Error(cache, blockNumber, "read failed", bytesRead));

			status_t error = errno;
			TRACE_ALWAYS("could not read block %" B_PRIdOFF ": bytesRead: %zd,"
				" error: %s\n", blockNumber, bytesRead, strerror(error));
			if (error == B_OK)
				return B_IO_ERROR;
			return error;
		}
		TB(Read(cache, block));

		mark_block_unbusy_reading(cache, block);
	}

	block->ref_count++;
	block->last_accessed = system_time() / 1000000L;

	*_block = block;
	return B_OK;
}


/*!	Returns the writable block data for the requested blockNumber.
	If \a cleared is true, the block is not read from disk; an empty block
	is returned.

	This is the only method to insert a block into a transaction. It makes
	sure that the previous block contents are preserved in that case.
*/
static status_t
get_writable_cached_block(block_cache* cache, off_t blockNumber,
	int32 transactionID, bool cleared, void** _block)
{
	TRACE(("get_writable_cached_block(blockNumber = %" B_PRIdOFF ", transaction = %" B_PRId32 ")\n",
		blockNumber, transactionID));

	if (blockNumber < 0 || blockNumber >= cache->max_blocks) {
		panic("get_writable_cached_block: invalid block number %" B_PRIdOFF " (max %" B_PRIdOFF ")",
			blockNumber, cache->max_blocks - 1);
		return B_BAD_VALUE;
	}

	bool allocated;
	cached_block* block;
	status_t status = get_cached_block(cache, blockNumber, &allocated,
		!cleared, &block);
	if (status != B_OK)
		return status;

	if (block->busy_writing)
		wait_for_busy_writing_block(cache, block);

	block->discard = false;

	// if there is no transaction support, we just return the current block
	if (transactionID == -1) {
		if (cleared) {
			mark_block_busy_reading(cache, block);
			mutex_unlock(&cache->lock);

			memset(block->current_data, 0, cache->block_size);

			mutex_lock(&cache->lock);
			mark_block_unbusy_reading(cache, block);
		}

		block->is_writing = true;

		if (!block->is_dirty) {
			cache->num_dirty_blocks++;
			block->is_dirty = true;
				// mark the block as dirty
		}

		TB(Get(cache, block));
		*_block = block->current_data;
		return B_OK;
	}

	cache_transaction* transaction = block->transaction;

	if (transaction != NULL && transaction->id != transactionID) {
		// TODO: we have to wait here until the other transaction is done.
		//	Maybe we should even panic, since we can't prevent any deadlocks.
		panic("get_writable_cached_block(): asked to get busy writable block "
			"(transaction %" B_PRId32 ")\n", block->transaction->id);
		put_cached_block(cache, block);
		return B_BAD_VALUE;
	}
	if (transaction == NULL && transactionID != -1) {
		// get new transaction
		transaction = lookup_transaction(cache, transactionID);
		if (transaction == NULL) {
			panic("get_writable_cached_block(): invalid transaction %" B_PRId32 "!\n",
				transactionID);
			put_cached_block(cache, block);
			return B_BAD_VALUE;
		}
		if (!transaction->open) {
			panic("get_writable_cached_block(): transaction already done!\n");
			put_cached_block(cache, block);
			return B_BAD_VALUE;
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
			TB(Error(cache, blockNumber, "allocate original failed"));
			FATAL(("could not allocate original_data\n"));
			put_cached_block(cache, block);
			return B_NO_MEMORY;
		}

		mark_block_busy_reading(cache, block);
		mutex_unlock(&cache->lock);

		memcpy(block->original_data, block->current_data, cache->block_size);

		mutex_lock(&cache->lock);
		mark_block_unbusy_reading(cache, block);
	}
	if (block->parent_data == block->current_data) {
		// remember any previous contents for the parent transaction
		block->parent_data = cache->Allocate();
		if (block->parent_data == NULL) {
			// TODO: maybe we should just continue the current transaction in
			// this case...
			TB(Error(cache, blockNumber, "allocate parent failed"));
			FATAL(("could not allocate parent\n"));
			put_cached_block(cache, block);
			return B_NO_MEMORY;
		}

		mark_block_busy_reading(cache, block);
		mutex_unlock(&cache->lock);

		memcpy(block->parent_data, block->current_data, cache->block_size);

		mutex_lock(&cache->lock);
		mark_block_unbusy_reading(cache, block);

		transaction->sub_num_blocks++;
	} else if (transaction != NULL && transaction->has_sub_transaction
		&& block->parent_data == NULL && wasUnchanged)
		transaction->sub_num_blocks++;

	if (cleared) {
		mark_block_busy_reading(cache, block);
		mutex_unlock(&cache->lock);

		memset(block->current_data, 0, cache->block_size);

		mutex_lock(&cache->lock);
		mark_block_unbusy_reading(cache, block);
	}

	block->is_dirty = true;
	TB(Get(cache, block));
	TB2(BlockData(cache, block, "get writable"));

	*_block = block->current_data;
	return B_OK;
}


#if DEBUG_BLOCK_CACHE


static void
dump_block(cached_block* block)
{
	kprintf("%08lx %9" B_PRIdOFF " %08lx %08lx %08lx %5" B_PRId32 " %6" B_PRId32
		" %c%c%c%c%c%c %08lx %08lx\n",
		(addr_t)block, block->block_number,
		(addr_t)block->current_data, (addr_t)block->original_data,
		(addr_t)block->parent_data, block->ref_count, block->LastAccess(),
		block->busy_reading ? 'r' : '-', block->busy_writing ? 'w' : '-',
		block->is_writing ? 'W' : '-', block->is_dirty ? 'D' : '-',
		block->unused ? 'U' : '-', block->discard ? 'D' : '-',
		(addr_t)block->transaction,
		(addr_t)block->previous_transaction);
}


static void
dump_block_long(cached_block* block)
{
	kprintf("BLOCK %p\n", block);
	kprintf(" current data:  %p\n", block->current_data);
	kprintf(" original data: %p\n", block->original_data);
	kprintf(" parent data:   %p\n", block->parent_data);
#if BLOCK_CACHE_DEBUG_CHANGED
	kprintf(" compare data:  %p\n", block->compare);
#endif
	kprintf(" ref_count:     %" B_PRId32 "\n", block->ref_count);
	kprintf(" accessed:      %" B_PRId32 "\n", block->LastAccess());
	kprintf(" flags:        ");
	if (block->busy_reading)
		kprintf(" busy_reading");
	if (block->busy_writing)
		kprintf(" busy_writing");
	if (block->is_writing)
		kprintf(" is-writing");
	if (block->is_dirty)
		kprintf(" is-dirty");
	if (block->unused)
		kprintf(" unused");
	if (block->discard)
		kprintf(" discard");
	kprintf("\n");
	if (block->transaction != NULL) {
		kprintf(" transaction:   %p (%" B_PRId32 ")\n", block->transaction,
			block->transaction->id);
		if (block->transaction_next != NULL) {
			kprintf(" next in transaction: %" B_PRIdOFF "\n",
				block->transaction_next->block_number);
		}
	}
	if (block->previous_transaction != NULL) {
		kprintf(" previous transaction: %p (%" B_PRId32 ")\n",
			block->previous_transaction,
			block->previous_transaction->id);
	}

	set_debug_variable("_current", (addr_t)block->current_data);
	set_debug_variable("_original", (addr_t)block->original_data);
	set_debug_variable("_parent", (addr_t)block->parent_data);
}


static int
dump_cached_block(int argc, char** argv)
{
	if (argc != 2) {
		kprintf("usage: %s <block-address>\n", argv[0]);
		return 0;
	}

	dump_block_long((struct cached_block*)(addr_t)parse_expression(argv[1]));
	return 0;
}


static int
dump_cache(int argc, char** argv)
{
	bool showTransactions = false;
	bool showBlocks = false;
	int32 i = 1;
	while (argv[i] != NULL && argv[i][0] == '-') {
		for (char* arg = &argv[i][1]; arg[0]; arg++) {
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

	block_cache* cache = (struct block_cache*)(addr_t)parse_expression(argv[i]);
	if (cache == NULL) {
		kprintf("invalid cache address\n");
		return 0;
	}

	off_t blockNumber = -1;
	if (i + 1 < argc) {
		blockNumber = parse_expression(argv[i + 1]);
		cached_block* block = cache->hash->Lookup(blockNumber);
		if (block != NULL)
			dump_block_long(block);
		else
			kprintf("block %" B_PRIdOFF " not found\n", blockNumber);
		return 0;
	}

	kprintf("BLOCK CACHE: %p\n", cache);

	kprintf(" fd:           %d\n", cache->fd);
	kprintf(" max_blocks:   %" B_PRIdOFF "\n", cache->max_blocks);
	kprintf(" block_size:   %zu\n", cache->block_size);
	kprintf(" next_transaction_id: %" B_PRId32 "\n", cache->next_transaction_id);
	kprintf(" buffer_cache: %p\n", cache->buffer_cache);
	kprintf(" busy_reading: %" B_PRIu32 ", %s waiters\n", cache->busy_reading_count,
		cache->busy_reading_waiters ? "has" : "no");
	kprintf(" busy_writing: %" B_PRIu32 ", %s waiters\n", cache->busy_writing_count,
		cache->busy_writing_waiters ? "has" : "no");

	if (!cache->pending_notifications.IsEmpty()) {
		kprintf(" pending notifications:\n");

		NotificationList::Iterator iterator
			= cache->pending_notifications.GetIterator();
		while (iterator.HasNext()) {
			cache_notification* notification = iterator.Next();

			kprintf("  %p %5" B_PRIx32 " %p - %p\n", notification,
				notification->events_pending, notification->hook,
				notification->data);
		}
	}

	if (showTransactions) {
		kprintf(" transactions:\n");
		kprintf("address       id state  blocks  main   sub\n");

		TransactionTable::Iterator iterator(cache->transaction_hash);

		while (iterator.HasNext()) {
			cache_transaction* transaction = iterator.Next();
			kprintf("%p %5" B_PRId32 " %-7s %5" B_PRId32 " %5" B_PRId32 " %5"
				B_PRId32 "\n", transaction, transaction->id,
				transaction->open ? "open" : "closed",
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
	uint32 discarded = 0;
	BlockTable::Iterator iterator(cache->hash);
	while (iterator.HasNext()) {
		cached_block* block = iterator.Next();
		if (showBlocks)
			dump_block(block);

		if (block->is_dirty)
			dirty++;
		if (block->discard)
			discarded++;
		if (block->ref_count)
			referenced++;
		count++;
	}

	kprintf(" %" B_PRIu32 " blocks total, %" B_PRIu32 " dirty, %" B_PRIu32
		" discarded, %" B_PRIu32 " referenced, %" B_PRIu32 " busy, %" B_PRIu32
		" in unused.\n",
		count, dirty, discarded, referenced, cache->busy_reading_count,
		cache->unused_block_count);
	return 0;
}


static int
dump_transaction(int argc, char** argv)
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

	cache_transaction* transaction = NULL;

	if (argc - i == 1) {
		transaction = (cache_transaction*)(addr_t)parse_expression(argv[i]);
	} else {
		block_cache* cache = (block_cache*)(addr_t)parse_expression(argv[i]);
		int32 id = parse_expression(argv[i + 1]);
		transaction = lookup_transaction(cache, id);
		if (transaction == NULL) {
			kprintf("No transaction with ID %" B_PRId32 " found.\n", id);
			return 0;
		}
	}

	kprintf("TRANSACTION %p\n", transaction);

	kprintf(" id:             %" B_PRId32 "\n", transaction->id);
	kprintf(" num block:      %" B_PRId32 "\n", transaction->num_blocks);
	kprintf(" main num block: %" B_PRId32 "\n", transaction->main_num_blocks);
	kprintf(" sub num block:  %" B_PRId32 "\n", transaction->sub_num_blocks);
	kprintf(" has sub:        %d\n", transaction->has_sub_transaction);
	kprintf(" state:          %s\n", transaction->open ? "open" : "closed");
	kprintf(" idle:           %" B_PRId64 " secs\n",
		(system_time() - transaction->last_used) / 1000000);

	kprintf(" listeners:\n");

	ListenerList::Iterator iterator = transaction->listeners.GetIterator();
	while (iterator.HasNext()) {
		cache_listener* listener = iterator.Next();

		kprintf("  %p %5" B_PRIx32 " %p - %p\n", listener, listener->events_pending,
			listener->hook, listener->data);
	}

	if (!showBlocks)
		return 0;

	kprintf(" blocks:\n");
	kprintf("address  block no. current  original parent    refs access "
		"flags transact prev. trans\n");

	cached_block* block = transaction->first_block;
	while (block != NULL) {
		dump_block(block);
		block = block->transaction_next;
	}

	kprintf("--\n");

	block_list::Iterator blockIterator = transaction->blocks.GetIterator();
	while (blockIterator.HasNext()) {
		block = blockIterator.Next();
		dump_block(block);
	}

	return 0;
}


static int
dump_caches(int argc, char** argv)
{
	kprintf("Block caches:\n");
	DoublyLinkedList<block_cache>::Iterator i = sCaches.GetIterator();
	while (i.HasNext()) {
		block_cache* cache = i.Next();
		if (cache == (block_cache*)&sMarkCache)
			continue;

		kprintf("  %p\n", cache);
	}

	return 0;
}


#if BLOCK_CACHE_BLOCK_TRACING >= 2
static int
dump_block_data(int argc, char** argv)
{
	using namespace BlockTracing;

	// Determine which blocks to show

	bool printStackTrace = true;
	uint32 which = 0;
	int32 i = 1;
	while (i < argc && argv[i][0] == '-') {
		char* arg = &argv[i][1];
		while (arg[0]) {
			switch (arg[0]) {
				case 'c':
					which |= BlockData::kCurrent;
					break;
				case 'p':
					which |= BlockData::kParent;
					break;
				case 'o':
					which |= BlockData::kOriginal;
					break;

				default:
					kprintf("invalid block specifier (only o/c/p are "
						"allowed).\n");
					return 0;
			}
			arg++;
		}

		i++;
	}
	if (which == 0)
		which = BlockData::kCurrent | BlockData::kParent | BlockData::kOriginal;

	if (i == argc) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	// Get the range of blocks to print

	int64 from = parse_expression(argv[i]);
	int64 to = from;
	if (argc > i + 1)
		to = parse_expression(argv[i + 1]);
	if (to < from)
		to = from;

	uint32 offset = 0;
	uint32 size = LONG_MAX;
	if (argc > i + 2)
		offset = parse_expression(argv[i + 2]);
	if (argc > i + 3)
		size = parse_expression(argv[i + 3]);

	TraceEntryIterator iterator;
	iterator.MoveTo(from - 1);

	static char sBuffer[1024];
	LazyTraceOutput out(sBuffer, sizeof(sBuffer), TRACE_OUTPUT_TEAM_ID);

	while (TraceEntry* entry = iterator.Next()) {
		int32 index = iterator.Index();
		if (index > to)
			break;

		Action* action = dynamic_cast<Action*>(entry);
		if (action != NULL) {
			out.Clear();
			out.DumpEntry(action);
			continue;
		}

		BlockData* blockData = dynamic_cast<BlockData*>(entry);
		if (blockData == NULL)
			continue;

		out.Clear();

		const char* dump = out.DumpEntry(entry);
		int length = strlen(dump);
		if (length > 0 && dump[length - 1] == '\n')
			length--;

		kprintf("%5" B_PRId32 ". %.*s\n", index, length, dump);

		if (printStackTrace) {
			out.Clear();
			entry->DumpStackTrace(out);
			if (out.Size() > 0)
				kputs(out.Buffer());
		}

		blockData->DumpBlocks(which, offset, size);
	}

	return 0;
}
#endif	// BLOCK_CACHE_BLOCK_TRACING >= 2


#endif	// DEBUG_BLOCK_CACHE


/*!	Traverses through the block_cache list, and returns one cache after the
	other. The cache returned is automatically locked when you get it, and
	unlocked with the next call to this function. Ignores caches that are in
	deletion state.
	Returns \c NULL when the end of the list is reached.
*/
static block_cache*
get_next_locked_block_cache(block_cache* last)
{
	MutexLocker _(sCachesLock);

	block_cache* cache;
	if (last != NULL) {
		mutex_unlock(&last->lock);

		cache = sCaches.GetNext((block_cache*)&sMarkCache);
		sCaches.Remove((block_cache*)&sMarkCache);
	} else
		cache = sCaches.Head();

	if (cache != NULL) {
		mutex_lock(&cache->lock);
		sCaches.InsertBefore(sCaches.GetNext(cache), (block_cache*)&sMarkCache);
	}

	return cache;
}


/*!	Background thread that continuously checks for pending notifications of
	all caches.
	Every two seconds, it will also write back up to 64 blocks per cache.
*/
static status_t
block_notifier_and_writer(void* /*data*/)
{
	const bigtime_t kDefaultTimeout = 2000000LL;
	bigtime_t timeout = kDefaultTimeout;

	while (true) {
		bigtime_t start = system_time();

		status_t status = acquire_sem_etc(sEventSemaphore, 1,
			B_RELATIVE_TIMEOUT, timeout);
		if (status == B_OK) {
			flush_pending_notifications();
			timeout -= system_time() - start;
			continue;
		}

		// Write 64 blocks of each block_cache roughly every 2 seconds,
		// potentially more or less depending on congestion and drive speeds
		// (usually much less.) We do not want to queue everything at once
		// because a future transaction might then get held up waiting for
		// a specific block to be written.
		timeout = kDefaultTimeout;
		size_t usedMemory;
		object_cache_get_usage(sBlockCache, &usedMemory);

		block_cache* cache = NULL;
		while ((cache = get_next_locked_block_cache(cache)) != NULL) {
			// Give some breathing room: wait 2x the length of the potential
			// maximum block count-sized write between writes, and also skip
			// if there are more than 16 blocks currently being written.
			const bigtime_t next = cache->last_block_write
					+ cache->last_block_write_duration * 2 * 64;
			if (cache->busy_writing_count > 16 || system_time() < next) {
				if (cache->last_block_write_duration > 0) {
					timeout = min_c(timeout,
						cache->last_block_write_duration * 2 * 64);
				}
				continue;
			}

			BlockWriter writer(cache, 64);
			bool hasMoreBlocks = false;

			size_t cacheUsedMemory;
			object_cache_get_usage(cache->buffer_cache, &cacheUsedMemory);
			usedMemory += cacheUsedMemory;

			if (cache->num_dirty_blocks) {
				// This cache is not using transactions, we'll scan the blocks
				// directly
				BlockTable::Iterator iterator(cache->hash);

				while (iterator.HasNext()) {
					cached_block* block = iterator.Next();
					if (block->CanBeWritten() && !writer.Add(block)) {
						hasMoreBlocks = true;
						break;
					}
				}
			} else {
				TransactionTable::Iterator iterator(cache->transaction_hash);

				while (iterator.HasNext()) {
					cache_transaction* transaction = iterator.Next();
					if (transaction->open) {
						if (system_time() > transaction->last_used
								+ kTransactionIdleTime) {
							// Transaction is open but idle
							notify_transaction_listeners(cache, transaction,
								TRANSACTION_IDLE);
						}
						continue;
					}

					bool hasLeftOvers;
						// we ignore this one
					if (!writer.Add(transaction, hasLeftOvers)) {
						hasMoreBlocks = true;
						break;
					}
				}
			}

			writer.Write();

			if (hasMoreBlocks && cache->last_block_write_duration > 0) {
				// There are probably still more blocks that we could write, so
				// see if we can decrease the timeout.
				timeout = min_c(timeout,
					cache->last_block_write_duration * 2 * 64);
			}

			if ((block_cache_used_memory() / B_PAGE_SIZE)
					> vm_page_num_pages() / 2) {
				// Try to reduce memory usage to half of the available
				// RAM at maximum
				cache->RemoveUnusedBlocks(1000, 10);
			}
		}

		MutexLocker _(sCachesMemoryUseLock);
		sUsedMemory = usedMemory;
	}

	// never can get here
	return B_OK;
}


/*!	Notify function for wait_for_notifications(). */
static void
notify_sync(int32 transactionID, int32 event, void* _cache)
{
	block_cache* cache = (block_cache*)_cache;

	cache->condition_variable.NotifyOne();
}


/*!	Must be called with the sCachesLock held. */
static bool
is_valid_cache(block_cache* cache)
{
	ASSERT_LOCKED_MUTEX(&sCachesLock);

	DoublyLinkedList<block_cache>::Iterator iterator = sCaches.GetIterator();
	while (iterator.HasNext()) {
		if (cache == iterator.Next())
			return true;
	}

	return false;
}


/*!	Waits until all pending notifications are carried out.
	Safe to be called from the block writer/notifier thread.
	You must not hold the \a cache lock when calling this function.
*/
static void
wait_for_notifications(block_cache* cache)
{
	MutexLocker locker(sCachesLock);

	if (find_thread(NULL) == sNotifierWriterThread) {
		// We're the notifier thread, don't wait, but flush all pending
		// notifications directly.
		if (is_valid_cache(cache))
			flush_pending_notifications(cache);
		return;
	}

	// add sync notification
	cache_notification notification;
	set_notification(NULL, notification, TRANSACTION_WRITTEN, notify_sync,
		cache);

	ConditionVariableEntry entry;
	cache->condition_variable.Add(&entry);

	add_notification(cache, &notification, TRANSACTION_WRITTEN, false);
	locker.Unlock();

	// wait for notification hook to be called
	entry.Wait();
}


status_t
block_cache_init(void)
{
	sBlockCache = create_object_cache("cached blocks", sizeof(cached_block),
		CACHE_LARGE_SLAB);
	if (sBlockCache == NULL)
		return B_NO_MEMORY;

	sCacheNotificationCache = create_object_cache("cache notifications",
		sizeof(cache_listener), 0);
	if (sCacheNotificationCache == NULL)
		return B_NO_MEMORY;

	new (&sCaches) DoublyLinkedList<block_cache>;
		// manually call constructor

	sEventSemaphore = create_sem(0, "block cache event");
	if (sEventSemaphore < B_OK)
		return sEventSemaphore;

	sNotifierWriterThread = spawn_kernel_thread(&block_notifier_and_writer,
		"block notifier/writer", B_LOW_PRIORITY, NULL);
	if (sNotifierWriterThread >= B_OK)
		resume_thread(sNotifierWriterThread);

#if DEBUG_BLOCK_CACHE
	add_debugger_command_etc("block_caches", &dump_caches,
		"dumps all block caches", "\n", 0);
	add_debugger_command_etc("block_cache", &dump_cache,
		"dumps a specific block cache",
		"[-bt] <cache-address> [block-number]\n"
		"  -t lists the transactions\n"
		"  -b lists all blocks\n", 0);
	add_debugger_command("cached_block", &dump_cached_block,
		"dumps the specified cached block");
	add_debugger_command_etc("transaction", &dump_transaction,
		"dumps a specific transaction", "[-b] ((<cache> <id>) | <transaction>)\n"
		"Either use a block cache pointer and an ID or a pointer to the transaction.\n"
		"  -b lists all blocks that are part of this transaction\n", 0);
#	if BLOCK_CACHE_BLOCK_TRACING >= 2
	add_debugger_command_etc("block_cache_data", &dump_block_data,
		"dumps the data blocks logged for the actions",
		"[-cpo] <from> [<to> [<offset> [<size>]]]\n"
		"If no data specifier is used, all blocks are shown by default.\n"
		" -c       the current data is shown, if available.\n"
		" -p       the parent data is shown, if available.\n"
		" -o       the original data is shown, if available.\n"
		" <from>   first index of tracing entries to show.\n"
		" <to>     if given, the last entry. If not, only <from> is shown.\n"
		" <offset> the offset of the block data.\n"
		" <from>   the size of the block data that is dumped\n", 0);
#	endif
#endif	// DEBUG_BLOCK_CACHE

	return B_OK;
}


size_t
block_cache_used_memory(void)
{
	MutexLocker _(sCachesMemoryUseLock);
	return sUsedMemory;
}


//	#pragma mark - public transaction API


int32
cache_start_transaction(void* _cache)
{
	block_cache* cache = (block_cache*)_cache;
	TransactionLocker locker(cache);

	if (cache->last_transaction && cache->last_transaction->open) {
		panic("last transaction (%" B_PRId32 ") still open!\n",
			cache->last_transaction->id);
	}

	cache_transaction* transaction = new(std::nothrow) cache_transaction;
	if (transaction == NULL)
		return B_NO_MEMORY;

	transaction->id = atomic_add(&cache->next_transaction_id, 1);
	cache->last_transaction = transaction;

	TRACE(("cache_start_transaction(): id %" B_PRId32 " started\n", transaction->id));
	T(Action("start", cache, transaction));

	cache->transaction_hash->Insert(transaction);

	return transaction->id;
}


status_t
cache_sync_transaction(void* _cache, int32 id)
{
	block_cache* cache = (block_cache*)_cache;
	bool hadBusy;

	TRACE(("cache_sync_transaction(id %" B_PRId32 ")\n", id));

	do {
		TransactionLocker locker(cache);
		hadBusy = false;

		BlockWriter writer(cache);
		TransactionTable::Iterator iterator(cache->transaction_hash);

		while (iterator.HasNext()) {
			// close all earlier transactions which haven't been closed yet
			cache_transaction* transaction = iterator.Next();

			if (transaction->busy_writing_count != 0) {
				hadBusy = true;
				continue;
			}
			if (transaction->id <= id && !transaction->open) {
				// write back all of their remaining dirty blocks
				T(Action("sync", cache, transaction));

				bool hasLeftOvers;
				writer.Add(transaction, hasLeftOvers);

				if (hasLeftOvers) {
					// This transaction contains blocks that a previous
					// transaction is trying to write back in this write run
					hadBusy = true;
				}
			}
		}

		status_t status = writer.Write();
		if (status != B_OK)
			return status;
	} while (hadBusy);

	wait_for_notifications(cache);
		// make sure that all pending TRANSACTION_WRITTEN notifications
		// are handled after we return
	return B_OK;
}


status_t
cache_end_transaction(void* _cache, int32 id,
	transaction_notification_hook hook, void* data)
{
	block_cache* cache = (block_cache*)_cache;
	TransactionLocker locker(cache);

	TRACE(("cache_end_transaction(id = %" B_PRId32 ")\n", id));

	cache_transaction* transaction = lookup_transaction(cache, id);
	if (transaction == NULL) {
		panic("cache_end_transaction(): invalid transaction ID\n");
		return B_BAD_VALUE;
	}

	// Write back all pending transaction blocks
	status_t status = write_blocks_in_previous_transaction(cache, transaction);
	if (status != B_OK)
		return status;

	notify_transaction_listeners(cache, transaction, TRANSACTION_ENDED);

	if (hook != NULL
		&& add_transaction_listener(cache, transaction, TRANSACTION_WRITTEN,
			hook, data) != B_OK) {
		return B_NO_MEMORY;
	}

	T(Action("end", cache, transaction));

	// iterate through all blocks and free the unchanged original contents

	cached_block* next;
	for (cached_block* block = transaction->first_block; block != NULL;
			block = next) {
		next = block->transaction_next;
		ASSERT(block->previous_transaction == NULL);

		if (block->discard) {
			// This block has been discarded in the transaction
			cache->DiscardBlock(block);
			transaction->num_blocks--;
			continue;
		}

		if (block->original_data != NULL) {
			cache->Free(block->original_data);
			block->original_data = NULL;
		}
		if (block->parent_data != NULL) {
			ASSERT(transaction->has_sub_transaction);
			cache->FreeBlockParentData(block);
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
cache_abort_transaction(void* _cache, int32 id)
{
	block_cache* cache = (block_cache*)_cache;
	TransactionLocker locker(cache);

	TRACE(("cache_abort_transaction(id = %" B_PRId32 ")\n", id));

	cache_transaction* transaction = lookup_transaction(cache, id);
	if (transaction == NULL) {
		panic("cache_abort_transaction(): invalid transaction ID\n");
		return B_BAD_VALUE;
	}

	T(Abort(cache, transaction));
	notify_transaction_listeners(cache, transaction, TRANSACTION_ABORTED);

	// iterate through all blocks and restore their original contents

	cached_block* block = transaction->first_block;
	cached_block* next;
	for (; block != NULL; block = next) {
		next = block->transaction_next;

		if (block->original_data != NULL) {
			TRACE(("cache_abort_transaction(id = %" B_PRId32 "): restored contents of "
				"block %" B_PRIdOFF "\n", transaction->id, block->block_number));
			memcpy(block->current_data, block->original_data,
				cache->block_size);
			cache->Free(block->original_data);
			block->original_data = NULL;
		}
		if (transaction->has_sub_transaction && block->parent_data != NULL)
			cache->FreeBlockParentData(block);

		block->transaction_next = NULL;
		block->transaction = NULL;
		block->discard = false;
		if (block->previous_transaction == NULL)
			block->is_dirty = false;
	}

	cache->transaction_hash->Remove(transaction);
	delete_transaction(cache, transaction);
	return B_OK;
}


/*!	Acknowledges the current parent transaction, and starts a new transaction
	from its sub transaction.
	The new transaction also gets a new transaction ID.
*/
int32
cache_detach_sub_transaction(void* _cache, int32 id,
	transaction_notification_hook hook, void* data)
{
	block_cache* cache = (block_cache*)_cache;
	TransactionLocker locker(cache);

	TRACE(("cache_detach_sub_transaction(id = %" B_PRId32 ")\n", id));

	cache_transaction* transaction = lookup_transaction(cache, id);
	if (transaction == NULL) {
		panic("cache_detach_sub_transaction(): invalid transaction ID\n");
		return B_BAD_VALUE;
	}
	if (!transaction->has_sub_transaction)
		return B_BAD_VALUE;

	// iterate through all blocks and free the unchanged original contents

	status_t status = write_blocks_in_previous_transaction(cache, transaction);
	if (status != B_OK)
		return status;

	// create a new transaction for the sub transaction
	cache_transaction* newTransaction = new(std::nothrow) cache_transaction;
	if (newTransaction == NULL)
		return B_NO_MEMORY;

	newTransaction->id = atomic_add(&cache->next_transaction_id, 1);
	T(Detach(cache, transaction, newTransaction));

	notify_transaction_listeners(cache, transaction, TRANSACTION_ENDED);

	if (add_transaction_listener(cache, transaction, TRANSACTION_WRITTEN, hook,
			data) != B_OK) {
		delete newTransaction;
		return B_NO_MEMORY;
	}

	cached_block* last = NULL;
	cached_block* next;
	for (cached_block* block = transaction->first_block; block != NULL;
			block = next) {
		next = block->transaction_next;
		ASSERT(block->previous_transaction == NULL);

		if (block->discard) {
			cache->DiscardBlock(block);
			transaction->main_num_blocks--;
			continue;
		}

		if (block->parent_data != NULL) {
			// The block changed in the parent - free the original data, since
			// they will be replaced by what is in current.
			ASSERT(block->original_data != NULL);
			cache->Free(block->original_data);

			if (block->parent_data != block->current_data) {
				// The block had been changed in both transactions
				block->original_data = block->parent_data;
			} else {
				// The block has only been changed in the parent
				block->original_data = NULL;
			}
			block->parent_data = NULL;

			// move the block to the previous transaction list
			transaction->blocks.Add(block);
			block->previous_transaction = transaction;
		}

		if (block->original_data != NULL) {
			// This block had been changed in the current sub transaction,
			// we need to move this block over to the new transaction.
			ASSERT(block->parent_data == NULL);

			if (last == NULL)
				newTransaction->first_block = block;
			else
				last->transaction_next = block;

			block->transaction = newTransaction;
			last = block;
		} else
			block->transaction = NULL;

		block->transaction_next = NULL;
	}

	newTransaction->num_blocks = transaction->sub_num_blocks;

	transaction->open = false;
	transaction->has_sub_transaction = false;
	transaction->num_blocks = transaction->main_num_blocks;
	transaction->sub_num_blocks = 0;

	cache->transaction_hash->Insert(newTransaction);
	cache->last_transaction = newTransaction;

	return newTransaction->id;
}


status_t
cache_abort_sub_transaction(void* _cache, int32 id)
{
	block_cache* cache = (block_cache*)_cache;
	TransactionLocker locker(cache);

	TRACE(("cache_abort_sub_transaction(id = %" B_PRId32 ")\n", id));

	cache_transaction* transaction = lookup_transaction(cache, id);
	if (transaction == NULL) {
		panic("cache_abort_sub_transaction(): invalid transaction ID\n");
		return B_BAD_VALUE;
	}
	if (!transaction->has_sub_transaction)
		return B_BAD_VALUE;

	T(Abort(cache, transaction));
	notify_transaction_listeners(cache, transaction, TRANSACTION_ABORTED);

	// revert all changes back to the version of the parent

	cached_block* block = transaction->first_block;
	cached_block* last = NULL;
	cached_block* next;
	for (; block != NULL; block = next) {
		next = block->transaction_next;

		if (block->parent_data == NULL) {
			// The parent transaction didn't change the block, but the sub
			// transaction did - we need to revert to the original data.
			// The block is no longer part of the transaction
			if (block->original_data != NULL) {
				// The block might not have original data if was empty
				memcpy(block->current_data, block->original_data,
					cache->block_size);
			}

			if (last != NULL)
				last->transaction_next = next;
			else
				transaction->first_block = next;

			block->transaction_next = NULL;
			block->transaction = NULL;
			transaction->num_blocks--;

			if (block->previous_transaction == NULL) {
				cache->Free(block->original_data);
				block->original_data = NULL;
				block->is_dirty = false;

				if (block->ref_count == 0) {
					// Move the block into the unused list if possible
					block->unused = true;
					cache->unused_blocks.Add(block);
					cache->unused_block_count++;
				}
			}
		} else {
			if (block->parent_data != block->current_data) {
				// The block has been changed and must be restored - the block
				// is still dirty and part of the transaction
				TRACE(("cache_abort_sub_transaction(id = %" B_PRId32 "): "
					"restored contents of block %" B_PRIdOFF "\n",
					transaction->id, block->block_number));
				memcpy(block->current_data, block->parent_data,
					cache->block_size);
				cache->Free(block->parent_data);
				// The block stays dirty
			}
			block->parent_data = NULL;
			last = block;
		}

		block->discard = false;
	}

	// all subsequent changes will go into the main transaction
	transaction->has_sub_transaction = false;
	transaction->sub_num_blocks = 0;

	return B_OK;
}


status_t
cache_start_sub_transaction(void* _cache, int32 id)
{
	block_cache* cache = (block_cache*)_cache;
	TransactionLocker locker(cache);

	TRACE(("cache_start_sub_transaction(id = %" B_PRId32 ")\n", id));

	cache_transaction* transaction = lookup_transaction(cache, id);
	if (transaction == NULL) {
		panic("cache_start_sub_transaction(): invalid transaction ID %" B_PRId32 "\n",
			id);
		return B_BAD_VALUE;
	}

	notify_transaction_listeners(cache, transaction, TRANSACTION_ENDED);

	// move all changed blocks up to the parent

	cached_block* block = transaction->first_block;
	cached_block* next;
	for (; block != NULL; block = next) {
		next = block->transaction_next;

		if (block->parent_data != NULL) {
			// There already is an older sub transaction - we acknowledge
			// its changes and move its blocks up to the parent
			ASSERT(transaction->has_sub_transaction);
			cache->FreeBlockParentData(block);
		}
		if (block->discard) {
			// This block has been discarded in the parent transaction.
			// Just throw away any changes made in this transaction, so that
			// it can still be reverted to its original contents if needed
			ASSERT(block->previous_transaction == NULL);
			if (block->original_data != NULL) {
				memcpy(block->current_data, block->original_data,
					cache->block_size);

				cache->Free(block->original_data);
				block->original_data = NULL;
			}
			continue;
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
	is ended, aborted, written, or idle as specified by \a events.
	The listener gets automatically removed when the transaction ends.
*/
status_t
cache_add_transaction_listener(void* _cache, int32 id, int32 events,
	transaction_notification_hook hook, void* data)
{
	block_cache* cache = (block_cache*)_cache;
	TransactionLocker locker(cache);

	cache_transaction* transaction = lookup_transaction(cache, id);
	if (transaction == NULL)
		return B_BAD_VALUE;

	return add_transaction_listener(cache, transaction, events, hook, data);
}


status_t
cache_remove_transaction_listener(void* _cache, int32 id,
	transaction_notification_hook hookFunction, void* data)
{
	block_cache* cache = (block_cache*)_cache;
	TransactionLocker locker(cache);

	cache_transaction* transaction = lookup_transaction(cache, id);
	if (transaction == NULL)
		return B_BAD_VALUE;

	ListenerList::Iterator iterator = transaction->listeners.GetIterator();
	while (iterator.HasNext()) {
		cache_listener* listener = iterator.Next();
		if (listener->data == data && listener->hook == hookFunction) {
			iterator.Remove();

			if (listener->events_pending != 0) {
				MutexLocker _(sNotificationsLock);
				if (listener->events_pending != 0)
					cache->pending_notifications.Remove(listener);
			}
			delete listener;
			return B_OK;
		}
	}

	return B_ENTRY_NOT_FOUND;
}


status_t
cache_next_block_in_transaction(void* _cache, int32 id, bool mainOnly,
	long* _cookie, off_t* _blockNumber, void** _data, void** _unchangedData)
{
	cached_block* block = (cached_block*)*_cookie;
	block_cache* cache = (block_cache*)_cache;
	TransactionLocker locker(cache);

	cache_transaction* transaction = lookup_transaction(cache, id);
	if (transaction == NULL || !transaction->open)
		return B_BAD_VALUE;

	if (block == NULL)
		block = transaction->first_block;
	else
		block = block->transaction_next;

	if (transaction->has_sub_transaction) {
		if (mainOnly) {
			// find next block that the parent changed
			while (block != NULL && block->parent_data == NULL)
				block = block->transaction_next;
		} else {
			// find next non-discarded block
			while (block != NULL && block->discard)
				block = block->transaction_next;
		}
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


int32
cache_blocks_in_transaction(void* _cache, int32 id)
{
	block_cache* cache = (block_cache*)_cache;
	TransactionLocker locker(cache);

	cache_transaction* transaction = lookup_transaction(cache, id);
	if (transaction == NULL)
		return B_BAD_VALUE;

	return transaction->num_blocks;
}


/*!	Returns the number of blocks that are part of the main transaction. If this
	transaction does not have a sub transaction yet, this is the same value as
	cache_blocks_in_transaction() would return.
*/
int32
cache_blocks_in_main_transaction(void* _cache, int32 id)
{
	block_cache* cache = (block_cache*)_cache;
	TransactionLocker locker(cache);

	cache_transaction* transaction = lookup_transaction(cache, id);
	if (transaction == NULL)
		return B_BAD_VALUE;

	if (transaction->has_sub_transaction)
		return transaction->main_num_blocks;

	return transaction->num_blocks;
}


int32
cache_blocks_in_sub_transaction(void* _cache, int32 id)
{
	block_cache* cache = (block_cache*)_cache;
	TransactionLocker locker(cache);

	cache_transaction* transaction = lookup_transaction(cache, id);
	if (transaction == NULL)
		return B_BAD_VALUE;

	return transaction->sub_num_blocks;
}


/*!	Check if block is in transaction
*/
bool
cache_has_block_in_transaction(void* _cache, int32 id, off_t blockNumber)
{
	block_cache* cache = (block_cache*)_cache;
	TransactionLocker locker(cache);

	cached_block* block = cache->hash->Lookup(blockNumber);

	return (block != NULL && block->transaction != NULL
		&& block->transaction->id == id);
}


//	#pragma mark - public block cache API


void
block_cache_delete(void* _cache, bool allowWrites)
{
	block_cache* cache = (block_cache*)_cache;

	if (allowWrites)
		block_cache_sync(cache);

	mutex_lock(&sCachesLock);
	sCaches.Remove(cache);
	mutex_unlock(&sCachesLock);

	mutex_lock(&cache->lock);

	// wait for all blocks to become unbusy
	wait_for_busy_reading_blocks(cache);
	wait_for_busy_writing_blocks(cache);

	// free all blocks

	cached_block* block = cache->hash->Clear(true);
	while (block != NULL) {
		cached_block* next = block->next;
		cache->FreeBlock(block);
		block = next;
	}

	// free all transactions (they will all be aborted)

	cache_transaction* transaction = cache->transaction_hash->Clear(true);
	while (transaction != NULL) {
		cache_transaction* next = transaction->next;
		delete transaction;
		transaction = next;
	}

	delete cache;
}


void*
block_cache_create(int fd, off_t numBlocks, size_t blockSize, bool readOnly)
{
	block_cache* cache = new(std::nothrow) block_cache(fd, numBlocks, blockSize,
		readOnly);
	if (cache == NULL)
		return NULL;

	if (cache->Init() != B_OK) {
		delete cache;
		return NULL;
	}

	MutexLocker _(sCachesLock);
	sCaches.Add(cache);

	return cache;
}


status_t
block_cache_sync(void* _cache)
{
	block_cache* cache = (block_cache*)_cache;

	// We will sync all dirty blocks to disk that have a completed
	// transaction or no transaction only

	MutexLocker locker(&cache->lock);

	BlockWriter writer(cache);
	BlockTable::Iterator iterator(cache->hash);

	while (iterator.HasNext()) {
		cached_block* block = iterator.Next();
		if (block->CanBeWritten())
			writer.Add(block);
	}

	status_t status = writer.Write();

	locker.Unlock();

	wait_for_notifications(cache);
		// make sure that all pending TRANSACTION_WRITTEN notifications
		// are handled after we return
	return status;
}


status_t
block_cache_sync_etc(void* _cache, off_t blockNumber, size_t numBlocks)
{
	block_cache* cache = (block_cache*)_cache;

	// We will sync all dirty blocks to disk that have a completed
	// transaction or no transaction only

	if (blockNumber < 0 || blockNumber >= cache->max_blocks) {
		panic("block_cache_sync_etc: invalid block number %" B_PRIdOFF
			" (max %" B_PRIdOFF ")",
			blockNumber, cache->max_blocks - 1);
		return B_BAD_VALUE;
	}

	MutexLocker locker(&cache->lock);
	BlockWriter writer(cache);

	for (; numBlocks > 0; numBlocks--, blockNumber++) {
		cached_block* block = cache->hash->Lookup(blockNumber);
		if (block == NULL)
			continue;

		if (block->CanBeWritten())
			writer.Add(block);
	}

	status_t status = writer.Write();

	locker.Unlock();

	wait_for_notifications(cache);
		// make sure that all pending TRANSACTION_WRITTEN notifications
		// are handled after we return
	return status;
}


/*!	Discards a block from the current transaction or from the cache.
	You have to call this function when you no longer use a block, ie. when it
	might be reclaimed by the file cache in order to make sure they won't
	interfere.
*/
void
block_cache_discard(void* _cache, off_t blockNumber, size_t numBlocks)
{
	// TODO: this could be a nice place to issue the ATA trim command
	block_cache* cache = (block_cache*)_cache;
	TransactionLocker locker(cache);

	BlockWriter writer(cache);

	for (size_t i = 0; i < numBlocks; i++, blockNumber++) {
		cached_block* block = cache->hash->Lookup(blockNumber);
		if (block != NULL && block->previous_transaction != NULL)
			writer.Add(block);
	}

	writer.Write();
		// TODO: this can fail, too!

	blockNumber -= numBlocks;
		// reset blockNumber to its original value

	for (size_t i = 0; i < numBlocks; i++, blockNumber++) {
		cached_block* block = cache->hash->Lookup(blockNumber);
		if (block == NULL)
			continue;

		ASSERT(block->previous_transaction == NULL);

		if (block->unused) {
			cache->unused_blocks.Remove(block);
			cache->unused_block_count--;
			cache->RemoveBlock(block);
		} else {
			if (block->transaction != NULL && block->parent_data != NULL
				&& block->parent_data != block->current_data) {
				panic("Discarded block %" B_PRIdOFF " has already been changed in this "
					"transaction!", blockNumber);
			}

			// mark it as discarded (in the current transaction only, if any)
			block->discard = true;
		}
	}
}


status_t
block_cache_make_writable(void* _cache, off_t blockNumber, int32 transaction)
{
	block_cache_instance_data* instance = (block_cache_instance_data*)_cache;
	// TransactionLocker locker(instance); // Uses instance->transaction_lock

	if (instance->read_only) {
		panic("tried to make block writable on a read-only cache!");
		return B_ERROR;
	}

	unified_cache_entry* entry = unified_cache_get_entry(instance->master_cache,
		blockNumber, BLOCK_DATA);

	if (entry == NULL) {
		// Block not in cache, try to read it first
		void* dataBuffer = malloc(instance->block_size);
		if (dataBuffer == NULL)
			return B_NO_MEMORY;

		status_t status = _read_block_from_disk(instance, blockNumber, dataBuffer);
		if (status != B_OK) {
			free(dataBuffer);
			return status;
		}
		// Now put it into the cache (not dirty yet, just read)
		status = unified_cache_put_entry(instance->master_cache, blockNumber, BLOCK_DATA,
			dataBuffer, instance->block_size, false, &entry);
		if (status != B_OK) {
			free(dataBuffer); // unified_cache_put_entry failed, free buffer
			return status;
		}
		// unified_cache_put_entry returns a referenced entry
	}

	// Mark as writable (and dirty by default)
	status_t status = unified_cache_make_writable(instance->master_cache, entry, true);
	// TODO: Transaction logic would go here.
	// If in a transaction, original data might need to be saved if not already.
	// The 'transaction' parameter is currently unused in this simplified version.

	unified_cache_release_entry(instance->master_cache, entry);
		// release the reference from get_entry/put_entry
	return status;
}


status_t
block_cache_get_writable_etc(void* _cache, off_t blockNumber,
	int32 transactionId, void** _block)
{
	block_cache_instance_data* instance = (block_cache_instance_data*)_cache;
	if (instance->read_only) {
		TRACE_ALWAYS("block_cache_get_writable_etc: Attempt to get writable block on read-only cache instance %p\n", instance);
		return B_PERMISSION_DENIED;
	}

	TRACE(("block_cache_get_writable_etc: block %lld, transaction %d\n", blockNumber, transactionId));

	unified_cache_entry* entry = unified_cache_get_entry(instance->master_cache,
		blockNumber, BLOCK_DATA);

	if (entry == NULL) {
		// Block not in cache, read it from disk
		void* dataBuffer = malloc(instance->block_size);
		if (dataBuffer == NULL) {
			TRACE_ALWAYS("block_cache_get_writable_etc: No memory for dataBuffer for block %lld\n", blockNumber);
			return B_NO_MEMORY;
		}
		status_t status = _read_block_from_disk(instance, blockNumber, dataBuffer);
		if (status != B_OK) {
			TRACE_ALWAYS("block_cache_get_writable_etc: Failed to read block %lld from disk: %s\n", blockNumber, strerror(status));
			free(dataBuffer);
			return status;
		}
		// Put into cache, not dirty yet. The subsequent make_writable will mark it.
		status = unified_cache_put_entry(instance->master_cache, blockNumber, BLOCK_DATA,
			dataBuffer, instance->block_size, false, &entry);
		if (status != B_OK) {
			TRACE_ALWAYS("block_cache_get_writable_etc: Failed to put block %lld into cache: %s\n", blockNumber, strerror(status));
			free(dataBuffer); // unified_cache takes ownership on success
			return status;
		}
		// entry is now referenced
	}

	// Mark the block as writable. This also handles the dirty flag.
	status_t status = unified_cache_make_writable(instance->master_cache, entry, true);
	if (status != B_OK) {
		TRACE_ALWAYS("block_cache_get_writable_etc: Failed to make block %lld writable: %s\n", blockNumber, strerror(status));
		unified_cache_release_entry(instance->master_cache, entry); // Release ref from get/put
		return status;
	}

	// TODO: Transaction logic.
	// The old code had complex handling for original_data, parent_data here.
	// This needs to be reimplemented, possibly by storing transaction-specific copies
	// outside the unified_cache_entry, or by extending unified_cache_entry.
	// For now, we just return the current data pointer.

	*_block = entry->data;
	// The entry remains referenced. Caller must call block_cache_put().
	return B_OK;
}


void*
block_cache_get_writable(void* _cache, off_t blockNumber, int32 transactionId)
{
	void* blockData;
	status_t status = block_cache_get_writable_etc(_cache, blockNumber, transactionId, &blockData);
	if (status == B_OK)
		return blockData;

	TRACE_ALWAYS("block_cache_get_writable: Failed for block %lld, transaction %d: %s\n", blockNumber, transactionId, strerror(status));
	return NULL;
}


void*
block_cache_get_empty(void* _cache, off_t blockNumber, int32 transactionId)
{
	block_cache_instance_data* instance = (block_cache_instance_data*)_cache;
	if (instance->read_only) {
		TRACE_ALWAYS("block_cache_get_empty: Attempt on read-only cache instance %p\n", instance);
		return NULL;
	}

	TRACE(("block_cache_get_empty: block %lld, transaction %d\n", blockNumber, transactionId));

	unified_cache_entry* entry = unified_cache_get_entry(instance->master_cache,
		blockNumber, BLOCK_DATA);
	status_t status;

	if (entry == NULL) {
		// Block not in cache, create a new zeroed buffer for it
		void* dataBuffer = malloc(instance->block_size);
		if (dataBuffer == NULL) {
			TRACE_ALWAYS("block_cache_get_empty: No memory for dataBuffer for block %lld\n", blockNumber);
			return NULL;
		}
		memset(dataBuffer, 0, instance->block_size);

		// Put into cache, mark as dirty since it's new and "empty" (zeroed) content.
		status = unified_cache_put_entry(instance->master_cache, blockNumber, BLOCK_DATA,
			dataBuffer, instance->block_size, true, &entry);
		if (status != B_OK) {
			TRACE_ALWAYS("block_cache_get_empty: Failed to put new empty block %lld into cache: %s\n", blockNumber, strerror(status));
			free(dataBuffer);
			return NULL;
		}
		// entry is now referenced
	} else {
		// Entry exists, mark it writable and zero its contents.
		status = unified_cache_make_writable(instance->master_cache, entry, true);
		if (status != B_OK) {
			TRACE_ALWAYS("block_cache_get_empty: Failed to make block %lld writable: %s\n", blockNumber, strerror(status));
			unified_cache_release_entry(instance->master_cache, entry); // Release ref from get
			return NULL;
		}
		// Zero the existing buffer
		memset(entry->data, 0, instance->block_size);
	}

	// TODO: Transaction logic (as in get_writable_etc)

	// Caller must call block_cache_put().
	return entry->data;
}


status_t
block_cache_get_etc(void* _cache, off_t blockNumber, const void** _block)
{
	block_cache_instance_data* instance = (block_cache_instance_data*)_cache;
	TRACE(("block_cache_get_etc: block %lld\n", blockNumber));

	unified_cache_entry* entry = unified_cache_get_entry(instance->master_cache,
		blockNumber, BLOCK_DATA);

	if (entry == NULL) {
		// Block not in cache, read from disk
		TRACE(("block_cache_get_etc: Miss for block %lld. Reading from disk.\n", blockNumber));
		void* dataBuffer = malloc(instance->block_size);
		if (dataBuffer == NULL) {
			TRACE_ALWAYS("block_cache_get_etc: No memory for dataBuffer for block %lld\n", blockNumber);
			return B_NO_MEMORY;
		}

		status_t status = _read_block_from_disk(instance, blockNumber, dataBuffer);
		if (status != B_OK) {
			TRACE_ALWAYS("block_cache_get_etc: Failed to read block %lld from disk: %s\n", blockNumber, strerror(status));
			free(dataBuffer);
			return status;
		}

		// Put into cache (not dirty)
		status = unified_cache_put_entry(instance->master_cache, blockNumber, BLOCK_DATA,
			dataBuffer, instance->block_size, false, &entry);
		if (status != B_OK) {
			TRACE_ALWAYS("block_cache_get_etc: Failed to put block %lld into cache: %s\n", blockNumber, strerror(status));
			free(dataBuffer); // unified_cache takes ownership on success
			return status;
		}
		// entry is now referenced
	}

	*_block = entry->data;
	// The entry remains referenced. Caller must call block_cache_put().
	return B_OK;
}


const void*
block_cache_get(void* _cache, off_t blockNumber)
{
	const void* blockData;
	status_t status = block_cache_get_etc(_cache, blockNumber, &blockData);
	if (status == B_OK)
		return blockData;

	TRACE_ALWAYS("block_cache_get: Failed for block %lld: %s\n", blockNumber, strerror(status));
	return NULL;
}


/*!	Changes the internal status of a writable block to \a dirty. This can be
	helpful in case you realize you don't need to change that block anymore
	for whatever reason.

	Note, you must only use this function on blocks that were acquired
	writable!
*/
status_t
block_cache_set_dirty(void* _cache, off_t blockNumber, bool isDirty,
	int32 transactionId)
{
	block_cache_instance_data* instance = (block_cache_instance_data*)_cache;
	TRACE(("block_cache_set_dirty: block %lld, dirty %d, transaction %d\n", blockNumber, isDirty, transactionId));

	// We need a reference to the entry to modify it.
	// Since this function doesn't return the block, we get and release here.
	unified_cache_entry* entry = unified_cache_get_entry(instance->master_cache,
		blockNumber, BLOCK_DATA);

	if (entry == NULL) {
		// This is problematic. If the block isn't in cache, what does setting dirty mean?
		// The old cache would likely have it already if it was made writable.
		// For now, assume it must be in cache (e.g., after a get_writable).
		TRACE_ALWAYS("block_cache_set_dirty: Block %lld not found in cache.\n", blockNumber);
		return B_ENTRY_NOT_FOUND;
	}

	// TODO: Transaction logic. If part of a transaction, this interacts with it.

	status_t status = unified_cache_make_writable(instance->master_cache, entry, isDirty);
	unified_cache_release_entry(instance->master_cache, entry);

	if (status != B_OK) {
		TRACE_ALWAYS("block_cache_set_dirty: Failed to set dirty status for block %lld: %s\n", blockNumber, strerror(status));
	}
	return status;
}


void
block_cache_put(void* _cache, off_t blockNumber)
{
	block_cache_instance_data* instance = (block_cache_instance_data*)_cache;
	TRACE(("block_cache_put: block %lld\n", blockNumber));

	// This function releases a reference to a block previously obtained via get.
	// The unified_cache_entry pointer itself is not passed here, only the blockNumber.
	// This requires looking up the entry again just to release it, which is inefficient.
	// A better API would pass the entry pointer.
	// For now, we have to do this:
	// NOTE: This is a temporary, potentially problematic implementation.
	// The caller of block_cache_put expects to release one reference.
	// If the block is not in cache (e.g. get failed, but put is still called),
	// get_entry here will return NULL.
	// A robust solution would involve the get functions returning an opaque cookie
	// or the unified_cache_entry* itself, which is then passed to put.
	// For now, assume the block *should* be in cache if put is called.

	MutexLocker locker(instance->master_cache->lock); // Need to lock for direct lookup and ref_count change
	unified_cache_entry* entry = instance->master_cache->LookupEntry(blockNumber);
	if (entry != NULL && entry->data_type == BLOCK_DATA) {
		// Manually decrease ref count. This is simplified.
		// Proper release should go through unified_cache_release_entry.
		// This is a placeholder for a more robust ref_counting release mechanism.
		// For now, directly calling unified_cache_release_entry without getting a new ref.
		// This assumes the caller of block_cache_put() is releasing a ref they hold.
		// The entry *must* have been previously acquired.
		// unified_cache_release_entry already handles the locking.
		locker.Unlock(); // unified_cache_release_entry will relock
		unified_cache_release_entry(instance->master_cache, entry);

	} else {
		// This case should ideally not happen if get/put are paired correctly.
		// Or it means the block was evicted between get and put, which is also a problem
		// if the caller still holds a pointer to the data.
		TRACE_ALWAYS("block_cache_put: Block %lld not found or type mismatch during put. This might indicate an issue.\n", blockNumber);
	}
}


/*! Allocates blocks and schedules them to be read from disk, but does not get references to the
	blocks.
	@param blockNumber The index of the first requested block.
	@param _numBlocks As input, the number of blocks requested. As output, the number of
	blocks actually scheduled.  Prefetching will stop short if the requested range includes a
	block that is already cached.
*/
status_t
block_cache_prefetch(void* _cache, off_t startBlockNumber, size_t* _numBlocks)
{
#ifndef BUILDING_USERLAND_FS_SERVER
	block_cache_instance_data* instance = (block_cache_instance_data*)_cache;
	size_t requestedBlocks = *_numBlocks;
	size_t actualBlocks = 0;

	TRACE(("block_cache_prefetch: instance %p, start %lld, count %lu\n", instance, startBlockNumber, requestedBlocks));

	if (requestedBlocks == 0) {
		*_numBlocks = 0;
		return B_OK;
	}

	// The BlockPrefetcher class needs significant rework.
	// For now, a simplified synchronous prefetch loop.
	// A true prefetcher would be asynchronous.
	for (size_t i = 0; i < requestedBlocks; ++i) {
		off_t currentBlockNumber = startBlockNumber + i;
		if (currentBlockNumber >= instance->num_blocks)
			break; // Past end of device

		// Check if already in cache; if so, prefetching this one is redundant
		// but SIEVE might benefit from the access.
		// For true prefetch, we'd only load if not present.
		unified_cache_entry* entry = unified_cache_get_entry(instance->master_cache,
			currentBlockNumber, BLOCK_DATA);

		if (entry != NULL) {
			// Already cached. Release our reference.
			unified_cache_release_entry(instance->master_cache, entry);
			// Some prefetch strategies might stop if a block is found.
			// For simplicity, we'll continue for now, but this could be changed.
			// actualBlocks++;
			// continue;
            // Let's follow the original comment: "stop short if the requested range includes a block that is already cached."
            TRACE(("block_cache_prefetch: Block %lld already in cache. Stopping prefetch at this point.\n", currentBlockNumber));
            break;
		}

		// Not in cache, load it
		void* dataBuffer = malloc(instance->block_size);
		if (dataBuffer == NULL) {
			// Ran out of memory during prefetch
			TRACE_ALWAYS("block_cache_prefetch: Out of memory for block %lld\n", currentBlockNumber);
			break;
		}

		status_t readStatus = _read_block_from_disk(instance, currentBlockNumber, dataBuffer);
		if (readStatus != B_OK) {
			TRACE_ALWAYS("block_cache_prefetch: Failed to read block %lld: %s\n", currentBlockNumber, strerror(readStatus));
			free(dataBuffer);
			break; // Stop prefetching on I/O error
		}

		// Put into cache (not dirty, no specific entry pointer needed back)
		status_t putStatus = unified_cache_put_entry(instance->master_cache, currentBlockNumber,
			BLOCK_DATA, dataBuffer, instance->block_size, false, NULL);
		if (putStatus != B_OK) {
			TRACE_ALWAYS("block_cache_prefetch: Failed to put prefetched block %lld into cache: %s\n", currentBlockNumber, strerror(putStatus));
			free(dataBuffer); // Unified cache did not take ownership
			break; // Stop if cache insertion fails
		}
		// unified_cache_put_entry takes ownership of dataBuffer on success
		actualBlocks++;
	}

	*_numBlocks = actualBlocks;
	return B_OK;

#else // BUILDING_USERLAND_FS_SERVER
	*_numBlocks = 0;
	return B_UNSUPPORTED;
#endif // !BUILDING_USERLAND_FS_SERVER
}
