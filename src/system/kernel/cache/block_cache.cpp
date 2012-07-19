/*
 * Copyright 2004-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <block_cache.h>

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

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
#include <util/khash.h>
#include <vm/vm_page.h>

#include "kernel_debug_config.h"


// TODO: this is a naive but growing implementation to test the API:
//	block reading/writing is not at all optimized for speed, it will
//	just read and write single blocks.
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

static const bigtime_t kTransactionIdleTime = 2000000LL;
	// a transaction is considered idle after 2 seconds of inactivity


struct cache_transaction;
struct cached_block;
struct block_cache;
typedef DoublyLinkedListLink<cached_block> block_link;

struct cached_block {
	cached_block*	next;			// next in hash
	cached_block*	transaction_next;
	block_link		link;
	off_t			block_number;
	void*			current_data;
		// The data that is seen by everyone using the API; this one is always
		// present.
	void*			original_data;
		// When in a transaction, this contains the original data from before
		// the transaction.
	void*			parent_data;
		// This is a lazily alloced buffer that represents the contents of the
		// block in the parent transaction. It may point to current_data if the
		// contents have been changed only in the parent transaction, or, if the
		// block has been changed in the current sub transaction already, to a
		// new block containing the contents changed in the parent transaction.
		// If this is NULL, the block has not been changed in the parent
		// transaction at all.
#if BLOCK_CACHE_DEBUG_CHANGED
	void*			compare;
#endif
	int32			ref_count;
	int32			last_accessed;
	bool			busy_reading : 1;
	bool			busy_writing : 1;
	bool			is_writing : 1;
		// Block has been checked out for writing without transactions, and
		// cannot be written back if set
	bool			is_dirty : 1;
	bool			unused : 1;
	bool			discard : 1;
	bool			busy_reading_waiters : 1;
	bool			busy_writing_waiters : 1;
	cache_transaction* transaction;
	cache_transaction* previous_transaction;

	bool CanBeWritten() const;
	int32 LastAccess() const
		{ return system_time() / 1000000L - last_accessed; }

	static int Compare(void* _cacheEntry, const void* _block);
	static uint32 Hash(void* _cacheEntry, const void* _block, uint32 range);
};

typedef DoublyLinkedList<cached_block,
	DoublyLinkedListMemberGetLink<cached_block,
		&cached_block::link> > block_list;

struct cache_notification : DoublyLinkedListLinkImpl<cache_notification> {
	int32			transaction_id;
	int32			events_pending;
	int32			events;
	transaction_notification_hook hook;
	void*			data;
	bool			delete_after_event;
};

typedef DoublyLinkedList<cache_notification> NotificationList;

struct block_cache : DoublyLinkedListLinkImpl<block_cache> {
	hash_table*		hash;
	mutex			lock;
	int				fd;
	off_t			max_blocks;
	size_t			block_size;
	int32			next_transaction_id;
	cache_transaction* last_transaction;
	hash_table*		transaction_hash;

	object_cache*	buffer_cache;
	block_list		unused_blocks;
	uint32			unused_block_count;

	ConditionVariable busy_reading_condition;
	uint32			busy_reading_count;
	bool			busy_reading_waiters;

	ConditionVariable busy_writing_condition;
	uint32			busy_writing_count;
	bool			busy_writing_waiters;

	uint32			num_dirty_blocks;
	bool			read_only;

	NotificationList pending_notifications;
	ConditionVariable condition_variable;

					block_cache(int fd, off_t numBlocks, size_t blockSize,
						bool readOnly);
					~block_cache();

	status_t		Init();

	void			Free(void* buffer);
	void*			Allocate();
	void			RemoveUnusedBlocks(int32 count, int32 minSecondsOld = 0);
	void			RemoveBlock(cached_block* block);
	void			DiscardBlock(cached_block* block);
	void			FreeBlock(cached_block* block);
	cached_block*	NewBlock(off_t blockNumber);


private:
	static void		_LowMemoryHandler(void* data, uint32 resources,
						int32 level);
	cached_block*	_GetUnusedBlock();
};

struct cache_listener;
typedef DoublyLinkedListLink<cache_listener> listener_link;

struct cache_listener : cache_notification {
	listener_link	link;
};

typedef DoublyLinkedList<cache_listener,
	DoublyLinkedListMemberGetLink<cache_listener,
		&cache_listener::link> > ListenerList;


struct cache_transaction {
	cache_transaction();

	cache_transaction* next;
	int32			id;
	int32			num_blocks;
	int32			main_num_blocks;
	int32			sub_num_blocks;
	cached_block*	first_block;
	block_list		blocks;
	ListenerList	listeners;
	bool			open;
	bool			has_sub_transaction;
	bigtime_t		last_used;
	int32			busy_writing_count;
};


class BlockWriter {
public:
								BlockWriter(block_cache* cache,
									size_t max = SIZE_MAX);
								~BlockWriter();

			bool				Add(cached_block* block,
									hash_iterator* iterator = NULL);
			bool				Add(cache_transaction* transaction,
									hash_iterator* iterator,
									bool& hasLeftOvers);

			status_t			Write(hash_iterator* iterator = NULL,
									bool canUnlock = true);

			bool				DeletedTransaction() const
									{ return fDeletedTransaction; }

	static	status_t			WriteBlock(block_cache* cache,
									cached_block* block);

private:
			void*				_Data(cached_block* block) const;
			status_t			_WriteBlock(cached_block* block);
			void				_BlockDone(cached_block* block,
									hash_iterator* iterator);
			void				_UnmarkWriting(cached_block* block);

private:
	static	const size_t		kBufferSize = 64;

			block_cache*		fCache;
			cached_block*		fBuffer[kBufferSize];
			cached_block**		fBlocks;
			size_t				fCount;
			size_t				fTotal;
			size_t				fCapacity;
			size_t				fMax;
			status_t			fStatus;
			bool				fDeletedTransaction;
};


class TransactionLocking {
public:
	inline bool Lock(block_cache* cache)
	{
		mutex_lock(&cache->lock);

		while (cache->busy_writing_count != 0) {
			// wait for all blocks to be written
			ConditionVariableEntry entry;
			cache->busy_writing_condition.Add(&entry);
			cache->busy_writing_waiters = true;

			mutex_unlock(&cache->lock);

			entry.Wait();

			mutex_lock(&cache->lock);
		}

		return true;
	}

	inline void Unlock(block_cache* cache)
	{
		mutex_unlock(&cache->lock);
	}
};

typedef AutoLocker<block_cache, TransactionLocking> TransactionLocker;


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
		out.Print("block cache %p, %s %Ld, %c%c%c transaction %ld "
			"(previous id %ld)\n", fCache, _Action(), fBlockNumber,
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
		out.Print("block cache %p, error %Ld, %s%s%s",
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
		out.Print("block cache %p, block %Ld, data %c%c%c: %s",
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
			kprintf("invalid offset (block size %lu)\n", fSize);
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

		kprintf("%s: offset %lu, %lu bytes\n", label, offset, size);

		static const uint32 kBlockSize = 16;
		data += offset;

		for (uint32 i = 0; i < size;) {
			int start = i;

			kprintf("  %04lx ", i);
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
		out.Print("block cache %p, %s transaction %p (id %ld)%s"
			", %ld/%ld blocks", fCache, fLabel, fTransaction, fID,
			fSub ? " sub" : "", fNumBlocks, fSubNumBlocks);
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
		out.Print("block cache %p, detach transaction %p (id %ld)"
			"from transaction %p (id %ld)%s",
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
			"%p (id %ld), blocks", fCache, fTransaction, fID);
		for (int32 i = 0; i < fNumBlocks && !out.IsFull(); i++)
			out.Print(" %Ld", fBlocks[i]);
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


static DoublyLinkedList<block_cache> sCaches;
static mutex sCachesLock = MUTEX_INITIALIZER("block caches");
static mutex sCachesMemoryUseLock
	= MUTEX_INITIALIZER("block caches memory use");
static size_t sUsedMemory;
static sem_id sEventSemaphore;
static mutex sNotificationsLock
	= MUTEX_INITIALIZER("block cache notifications");
static thread_id sNotifierWriterThread;
static DoublyLinkedListLink<block_cache> sMarkCache;
	// TODO: this only works if the link is the first entry of block_cache
static object_cache* sBlockCache;


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

	cache_listener* listener = new(std::nothrow) cache_listener;
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


static int
transaction_compare(void* _transaction, const void* _id)
{
	cache_transaction* transaction = (cache_transaction*)_transaction;
	const int32* id = (const int32*)_id;

	return transaction->id - *id;
}


static uint32
transaction_hash(void* _transaction, const void* _id, uint32 range)
{
	cache_transaction* transaction = (cache_transaction*)_transaction;
	const int32* id = (const int32*)_id;

	if (transaction != NULL)
		return transaction->id % range;

	return (uint32)*id % range;
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
	return (cache_transaction*)hash_lookup(cache->transaction_hash, &id);
}


//	#pragma mark - cached_block


int
compare_blocks(const void* _blockA, const void* _blockB)
{
	cached_block* blockA = *(cached_block**)_blockA;
	cached_block* blockB = *(cached_block**)_blockB;

	off_t diff = blockA->block_number - blockB->block_number;
	if (diff > 0)
		return 1;

	return diff < 0 ? -1 : 0;
}


bool
cached_block::CanBeWritten() const
{
	return !busy_writing && !busy_reading
		&& (previous_transaction != NULL
			|| (transaction == NULL && is_dirty && !is_writing));
}


/*static*/ int
cached_block::Compare(void* _cacheEntry, const void* _block)
{
	cached_block* cacheEntry = (cached_block*)_cacheEntry;
	const off_t* block = (const off_t*)_block;

	off_t diff = cacheEntry->block_number - *block;
	if (diff > 0)
		return 1;

	return diff < 0 ? -1 : 0;
}


/*static*/ uint32
cached_block::Hash(void* _cacheEntry, const void* _block, uint32 range)
{
	cached_block* cacheEntry = (cached_block*)_cacheEntry;
	const off_t* block = (const off_t*)_block;

	if (cacheEntry != NULL)
		return cacheEntry->block_number % range;

	return (uint64)*block % range;
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
BlockWriter::Add(cached_block* block, hash_iterator* iterator)
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
			Write(iterator, false);
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
BlockWriter::Add(cache_transaction* transaction, hash_iterator* iterator,
	bool& hasLeftOvers)
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
		if (!Add(block, iterator))
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
BlockWriter::Write(hash_iterator* iterator, bool canUnlock)
{
	if (fCount == 0)
		return B_OK;

	if (canUnlock)
		mutex_unlock(&fCache->lock);

	// Sort blocks in their on-disk order
	// TODO: ideally, this should be handled by the I/O scheduler

	qsort(fBlocks, fCount, sizeof(void*), &compare_blocks);
	fDeletedTransaction = false;

	for (uint32 i = 0; i < fCount; i++) {
		status_t status = _WriteBlock(fBlocks[i]);
		if (status != B_OK) {
			// propagate to global error handling
			if (fStatus == B_OK)
				fStatus = status;

			_UnmarkWriting(fBlocks[i]);
			fBlocks[i] = NULL;
				// This block will not be marked clean
		}
	}

	if (canUnlock)
		mutex_lock(&fCache->lock);

	for (uint32 i = 0; i < fCount; i++)
		_BlockDone(fBlocks[i], iterator);

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
	return block->previous_transaction && block->original_data
		? block->original_data : block->current_data;
		// We first need to write back changes from previous transactions
}


status_t
BlockWriter::_WriteBlock(cached_block* block)
{
	ASSERT(block->busy_writing);

	TRACE(("BlockWriter::_WriteBlock(block %Ld)\n", block->block_number));
	TB(Write(fCache, block));
	TB2(BlockData(fCache, block, "before write"));

	size_t blockSize = fCache->block_size;

	ssize_t written = write_pos(fCache->fd,
		block->block_number * blockSize, _Data(block), blockSize);

	if (written != (ssize_t)blockSize) {
		TB(Error(fCache, block->block_number, "write failed", written));
		FATAL(("could not write back block %Ld (%s)\n", block->block_number,
			strerror(errno)));
		if (written < 0)
			return errno;

		return B_IO_ERROR;
	}

	return B_OK;
}


void
BlockWriter::_BlockDone(cached_block* block, hash_iterator* iterator)
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

		// Has the previous transation been finished with that write?
		if (--previous->num_blocks == 0) {
			TRACE(("cache transaction %ld finished!\n", previous->id));
			T(Action("written", fCache, previous));

			notify_transaction_listeners(fCache, previous,
				TRANSACTION_WRITTEN);

			if (iterator != NULL)
				hash_remove_current(fCache->transaction_hash, iterator);
			else
				hash_remove(fCache->transaction_hash, previous);

			delete_transaction(fCache, previous);
			fDeletedTransaction = true;
		}
	}
	if (block->transaction == NULL && block->ref_count == 0 && !block->unused) {
		// the block is no longer used
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
	num_dirty_blocks(0),
	read_only(readOnly)
{
}


/*! Should be called with the cache's lock held. */
block_cache::~block_cache()
{
	unregister_low_resource_handler(&_LowMemoryHandler, this);

	hash_uninit(transaction_hash);
	hash_uninit(hash);

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

	buffer_cache = create_object_cache_etc("block cache buffers", block_size,
		8, 0, 0, 0, CACHE_LARGE_SLAB, NULL, NULL, NULL, NULL);
	if (buffer_cache == NULL)
		return B_NO_MEMORY;

	cached_block dummyBlock;
	hash = hash_init(1024, offset_of_member(dummyBlock, next),
		&cached_block::Compare, &cached_block::Hash);
	if (hash == NULL)
		return B_NO_MEMORY;

	cache_transaction dummyTransaction;
	transaction_hash = hash_init(16, offset_of_member(dummyTransaction, next),
		&transaction_compare, &::transaction_hash);
	if (transaction_hash == NULL)
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
		panic("block_cache::FreeBlock(): %Ld, original %p, parent %p\n",
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
			dprintf("block allocation failed, unused list is %sempty.\n",
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
		TRACE(("  remove block %Ld, last accessed %" B_PRId32 "\n",
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
	hash_remove(hash, block);
	FreeBlock(block);
}


/*!	Discards the block from a transaction (this method must not be called
	for blocks not part of a transaction).
*/
void
block_cache::DiscardBlock(cached_block* block)
{
	ASSERT(block->discard);

	if (block->parent_data != NULL && block->parent_data != block->current_data)
		Free(block->parent_data);

	block->parent_data = NULL;

	if (block->original_data != NULL) {
		Free(block->original_data);
		block->original_data = NULL;
	}

	RemoveBlock(block);
}


void
block_cache::_LowMemoryHandler(void* data, uint32 resources, int32 level)
{
	TRACE(("block_cache: low memory handler called with level %ld\n", level));

	// free some blocks according to the low memory state
	// (if there is enough memory left, we don't free any)

	block_cache* cache = (block_cache*)data;
	int32 free = 0;
	int32 secondsOld = 0;
	switch (level) {
		case B_NO_LOW_RESOURCE:
			return;
		case B_LOW_RESOURCE_NOTE:
			free = cache->unused_block_count / 8;
			secondsOld = 120;
			break;
		case B_LOW_RESOURCE_WARNING:
			free = cache->unused_block_count / 4;
			secondsOld = 10;
			break;
		case B_LOW_RESOURCE_CRITICAL:
			free = cache->unused_block_count / 2;
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

	TRACE(("block_cache::_LowMemoryHandler(): %p: unused: %lu -> %lu\n", cache,
		oldUnused, cache->unused_block_count));
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
		hash_remove(hash, block);

		// TODO: see if parent/compare data is handled correctly here!
		if (block->parent_data != NULL
			&& block->parent_data != block->original_data)
			Free(block->parent_data);
		if (block->original_data != NULL)
			Free(block->original_data);

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
		dprintf("new block:\n");
		dump_block((const char*)block->current_data, 256, "  ");
		dprintf("unchanged block:\n");
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

			ASSERT(block->original_data == NULL
				&& block->parent_data == NULL);
			cache->unused_blocks.Add(block);
			cache->unused_block_count++;
		}
	}
}


static void
put_cached_block(block_cache* cache, off_t blockNumber)
{
	if (blockNumber < 0 || blockNumber >= cache->max_blocks) {
		panic("put_cached_block: invalid block number %lld (max %lld)",
			blockNumber, cache->max_blocks - 1);
	}

	cached_block* block = (cached_block*)hash_lookup(cache->hash, &blockNumber);
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
static cached_block*
get_cached_block(block_cache* cache, off_t blockNumber, bool* _allocated,
	bool readBlock = true)
{
	ASSERT_LOCKED_MUTEX(&cache->lock);

	if (blockNumber < 0 || blockNumber >= cache->max_blocks) {
		panic("get_cached_block: invalid block number %lld (max %lld)",
			blockNumber, cache->max_blocks - 1);
		return NULL;
	}

retry:
	cached_block* block = (cached_block*)hash_lookup(cache->hash,
		&blockNumber);
	*_allocated = false;

	if (block == NULL) {
		// put block into cache
		block = cache->NewBlock(blockNumber);
		if (block == NULL)
			return NULL;

		hash_insert_grow(cache->hash, block);
		*_allocated = true;
	} else if (block->busy_reading) {
		// The block is currently busy_reading - wait and try again later
		wait_for_busy_reading_block(cache, block);
		goto retry;
	}

	if (block->unused) {
		//TRACE(("remove block %Ld from unused\n", blockNumber));
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

			FATAL(("could not read block %Ld: bytesRead: %ld, error: %s\n",
				blockNumber, bytesRead, strerror(errno)));
			return NULL;
		}
		TB(Read(cache, block));

		mark_block_unbusy_reading(cache, block);
	}

	block->ref_count++;
	block->last_accessed = system_time() / 1000000L;

	return block;
}


/*!	Returns the writable block data for the requested blockNumber.
	If \a cleared is true, the block is not read from disk; an empty block
	is returned.

	This is the only method to insert a block into a transaction. It makes
	sure that the previous block contents are preserved in that case.
*/
static void*
get_writable_cached_block(block_cache* cache, off_t blockNumber, off_t base,
	off_t length, int32 transactionID, bool cleared)
{
	TRACE(("get_writable_cached_block(blockNumber = %Ld, transaction = %ld)\n",
		blockNumber, transactionID));

	if (blockNumber < 0 || blockNumber >= cache->max_blocks) {
		panic("get_writable_cached_block: invalid block number %lld (max %lld)",
			blockNumber, cache->max_blocks - 1);
	}

	bool allocated;
	cached_block* block = get_cached_block(cache, blockNumber, &allocated,
		!cleared);
	if (block == NULL)
		return NULL;

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
		return block->current_data;
	}

	cache_transaction* transaction = block->transaction;

	if (transaction != NULL && transaction->id != transactionID) {
		// TODO: we have to wait here until the other transaction is done.
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
			TB(Error(cache, blockNumber, "allocate original failed"));
			FATAL(("could not allocate original_data\n"));
			put_cached_block(cache, block);
			return NULL;
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
			// TODO: maybe we should just continue the current transaction in this case...
			TB(Error(cache, blockNumber, "allocate parent failed"));
			FATAL(("could not allocate parent\n"));
			put_cached_block(cache, block);
			return NULL;
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

	return block->current_data;
}


#if DEBUG_BLOCK_CACHE


static void
dump_block(cached_block* block)
{
	kprintf("%08lx %9Ld %08lx %08lx %08lx %5ld %6ld %c%c%c%c%c%c %08lx %08lx\n",
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
	kprintf(" ref_count:     %ld\n", block->ref_count);
	kprintf(" accessed:      %ld\n", block->LastAccess());
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
		cached_block* block = (cached_block*)hash_lookup(cache->hash,
			&blockNumber);
		if (block != NULL)
			dump_block_long(block);
		else
			kprintf("block %Ld not found\n", blockNumber);
		return 0;
	}

	kprintf("BLOCK CACHE: %p\n", cache);

	kprintf(" fd:           %d\n", cache->fd);
	kprintf(" max_blocks:   %Ld\n", cache->max_blocks);
	kprintf(" block_size:   %lu\n", cache->block_size);
	kprintf(" next_transaction_id: %ld\n", cache->next_transaction_id);
	kprintf(" buffer_cache: %p\n", cache->buffer_cache);
	kprintf(" busy_reading: %lu, %s waiters\n", cache->busy_reading_count,
		cache->busy_reading_waiters ? "has" : "no");
	kprintf(" busy_writing: %lu, %s waiters\n", cache->busy_writing_count,
		cache->busy_writing_waiters ? "has" : "no");

	if (!cache->pending_notifications.IsEmpty()) {
		kprintf(" pending notifications:\n");

		NotificationList::Iterator iterator
			= cache->pending_notifications.GetIterator();
		while (iterator.HasNext()) {
			cache_notification* notification = iterator.Next();

			kprintf("  %p %5lx %p - %p\n", notification,
				notification->events_pending, notification->hook,
				notification->data);
		}
	}

	if (showTransactions) {
		kprintf(" transactions:\n");
		kprintf("address       id state  blocks  main   sub\n");

		hash_iterator iterator;
		hash_open(cache->transaction_hash, &iterator);

		cache_transaction* transaction;
		while ((transaction = (cache_transaction*)hash_next(
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
	uint32 discarded = 0;
	hash_iterator iterator;
	hash_open(cache->hash, &iterator);
	cached_block* block;
	while ((block = (cached_block*)hash_next(cache->hash, &iterator)) != NULL) {
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

	kprintf(" %ld blocks total, %ld dirty, %ld discarded, %ld referenced, %ld "
		"busy, %" B_PRIu32 " in unused.\n", count, dirty, discarded, referenced,
		cache->busy_reading_count, cache->unused_block_count);

	hash_close(cache->hash, &iterator, false);
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
	kprintf(" idle:           %Ld secs\n",
		(system_time() - transaction->last_used) / 1000000);

	kprintf(" listeners:\n");

	ListenerList::Iterator iterator = transaction->listeners.GetIterator();
	while (iterator.HasNext()) {
		cache_listener* listener = iterator.Next();

		kprintf("  %p %5lx %p - %p\n", listener, listener->events_pending,
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

		kprintf("%5ld. %.*s\n", index, length, dump);

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
		sCaches.Insert(sCaches.GetNext(cache), (block_cache*)&sMarkCache);
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
	const bigtime_t kTimeout = 2000000LL;
	bigtime_t timeout = kTimeout;

	while (true) {
		bigtime_t start = system_time();

		status_t status = acquire_sem_etc(sEventSemaphore, 1,
			B_RELATIVE_TIMEOUT, timeout);
		if (status == B_OK) {
			flush_pending_notifications();
			timeout -= system_time() - start;
			continue;
		}

		// write 64 blocks of each block_cache every two seconds
		// TODO: change this once we have an I/O scheduler
		timeout = kTimeout;
		size_t usedMemory;
		object_cache_get_usage(sBlockCache, &usedMemory);

		block_cache* cache = NULL;
		while ((cache = get_next_locked_block_cache(cache)) != NULL) {
			BlockWriter writer(cache, 64);

			size_t cacheUsedMemory;
			object_cache_get_usage(cache->buffer_cache, &cacheUsedMemory);
			usedMemory += cacheUsedMemory;

			if (cache->num_dirty_blocks) {
				// This cache is not using transactions, we'll scan the blocks
				// directly
				hash_iterator iterator;
				hash_open(cache->hash, &iterator);

				cached_block* block;
				while ((block = (cached_block*)hash_next(cache->hash, &iterator))
						!= NULL) {
					if (block->CanBeWritten() && !writer.Add(block))
						break;
				}

				hash_close(cache->hash, &iterator, false);
			} else {
				hash_iterator iterator;
				hash_open(cache->transaction_hash, &iterator);

				cache_transaction* transaction;
				while ((transaction = (cache_transaction*)hash_next(
						cache->transaction_hash, &iterator)) != NULL) {
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
					if (!writer.Add(transaction, &iterator, hasLeftOvers))
						break;
				}

				hash_close(cache->transaction_hash, &iterator, false);
			}

			writer.Write();

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
	sBlockCache = create_object_cache_etc("cached blocks", sizeof(cached_block),
		8, 0, 0, 0, CACHE_LARGE_SLAB, NULL, NULL, NULL, NULL);
	if (sBlockCache == NULL)
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
		panic("last transaction (%ld) still open!\n",
			cache->last_transaction->id);
	}

	cache_transaction* transaction = new(std::nothrow) cache_transaction;
	if (transaction == NULL)
		return B_NO_MEMORY;

	transaction->id = atomic_add(&cache->next_transaction_id, 1);
	cache->last_transaction = transaction;

	TRACE(("cache_start_transaction(): id %ld started\n", transaction->id));
	T(Action("start", cache, transaction));

	hash_insert_grow(cache->transaction_hash, transaction);

	return transaction->id;
}


status_t
cache_sync_transaction(void* _cache, int32 id)
{
	block_cache* cache = (block_cache*)_cache;
	bool hadBusy;

	TRACE(("cache_sync_transaction(id %ld)\n", id));

	do {
		TransactionLocker locker(cache);
		hadBusy = false;

		BlockWriter writer(cache);
		hash_iterator iterator;
		hash_open(cache->transaction_hash, &iterator);

		cache_transaction* transaction;
		while ((transaction = (cache_transaction*)hash_next(
				cache->transaction_hash, &iterator)) != NULL) {
			// close all earlier transactions which haven't been closed yet

			if (transaction->busy_writing_count != 0) {
				hadBusy = true;
				continue;
			}
			if (transaction->id <= id && !transaction->open) {
				// write back all of their remaining dirty blocks
				T(Action("sync", cache, transaction));

				bool hasLeftOvers;
				writer.Add(transaction, &iterator, hasLeftOvers);

				if (hasLeftOvers) {
					// This transaction contains blocks that a previous
					// transaction is trying to write back in this write run
					hadBusy = true;
				}
			}
		}

		hash_close(cache->transaction_hash, &iterator, false);

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

	TRACE(("cache_end_transaction(id = %ld)\n", id));

	cache_transaction* transaction = lookup_transaction(cache, id);
	if (transaction == NULL) {
		panic("cache_end_transaction(): invalid transaction ID\n");
		return B_BAD_VALUE;
	}

	// Write back all pending transaction blocks

	BlockWriter writer(cache);
	cached_block* block = transaction->first_block;
	for (; block != NULL; block = block->transaction_next) {
		if (block->previous_transaction != NULL) {
			// need to write back pending changes
			writer.Add(block);
		}
	}

	status_t status = writer.Write();
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
	for (block = transaction->first_block; block != NULL; block = next) {
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
cache_abort_transaction(void* _cache, int32 id)
{
	block_cache* cache = (block_cache*)_cache;
	TransactionLocker locker(cache);

	TRACE(("cache_abort_transaction(id = %ld)\n", id));

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
			TRACE(("cache_abort_transaction(id = %ld): restored contents of "
				"block %Ld\n", transaction->id, block->block_number));
			memcpy(block->current_data, block->original_data,
				cache->block_size);
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
		block->discard = false;
	}

	hash_remove(cache->transaction_hash, transaction);
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

	TRACE(("cache_detach_sub_transaction(id = %ld)\n", id));

	cache_transaction* transaction = lookup_transaction(cache, id);
	if (transaction == NULL) {
		panic("cache_detach_sub_transaction(): invalid transaction ID\n");
		return B_BAD_VALUE;
	}
	if (!transaction->has_sub_transaction)
		return B_BAD_VALUE;

	// iterate through all blocks and free the unchanged original contents

	BlockWriter writer(cache);
	cached_block* block = transaction->first_block;
	for (; block != NULL; block = block->transaction_next) {
		if (block->previous_transaction != NULL) {
			// need to write back pending changes
			writer.Add(block);
		}
	}

	status_t status = writer.Write();
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
	for (block = transaction->first_block; block != NULL; block = next) {
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

			// move the block to the previous transaction list
			transaction->blocks.Add(block);
			block->previous_transaction = transaction;
			block->parent_data = NULL;
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

	hash_insert_grow(cache->transaction_hash, newTransaction);
	cache->last_transaction = newTransaction;

	return newTransaction->id;
}


status_t
cache_abort_sub_transaction(void* _cache, int32 id)
{
	block_cache* cache = (block_cache*)_cache;
	TransactionLocker locker(cache);

	TRACE(("cache_abort_sub_transaction(id = %ld)\n", id));

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
	cached_block* next;
	for (; block != NULL; block = next) {
		next = block->transaction_next;

		if (block->parent_data == NULL) {
			// the parent transaction didn't change the block, but the sub
			// transaction did - we need to revert from the original data
			ASSERT(block->original_data != NULL);
			memcpy(block->current_data, block->original_data,
				cache->block_size);
		} else if (block->parent_data != block->current_data) {
			// the block has been changed and must be restored
			TRACE(("cache_abort_sub_transaction(id = %ld): restored contents "
				"of block %Ld\n", transaction->id, block->block_number));
			memcpy(block->current_data, block->parent_data, cache->block_size);
			cache->Free(block->parent_data);
		}

		block->parent_data = NULL;
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

	TRACE(("cache_start_sub_transaction(id = %ld)\n", id));

	cache_transaction* transaction = lookup_transaction(cache, id);
	if (transaction == NULL) {
		panic("cache_start_sub_transaction(): invalid transaction ID %ld\n",
			id);
		return B_BAD_VALUE;
	}

	notify_transaction_listeners(cache, transaction, TRANSACTION_ENDED);

	// move all changed blocks up to the parent

	cached_block* block = transaction->first_block;
	cached_block* last = NULL;
	cached_block* next;
	for (; block != NULL; block = next) {
		next = block->transaction_next;

		if (block->discard) {
			// This block has been discarded in the parent transaction
			// TODO: this is wrong: since the parent transaction is not
			// finished here (just extended), this block cannot be reverted
			// anymore in case the parent transaction is aborted!!!
			if (last != NULL)
				last->transaction_next = next;
			else
				transaction->first_block = next;

			cache->DiscardBlock(block);
			transaction->num_blocks--;
			continue;
		}

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
		last = block;
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

	uint32 cookie = 0;
	cached_block* block;
	while ((block = (cached_block*)hash_remove_first(cache->hash,
			&cookie)) != NULL) {
		cache->FreeBlock(block);
	}

	// free all transactions (they will all be aborted)

	cookie = 0;
	cache_transaction* transaction;
	while ((transaction = (cache_transaction*)hash_remove_first(
			cache->transaction_hash, &cookie)) != NULL) {
		delete transaction;
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
	hash_iterator iterator;
	hash_open(cache->hash, &iterator);

	cached_block* block;
	while ((block = (cached_block*)hash_next(cache->hash, &iterator)) != NULL) {
		if (block->CanBeWritten())
			writer.Add(block);
	}

	hash_close(cache->hash, &iterator, false);

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
		panic("block_cache_sync_etc: invalid block number %Ld (max %Ld)",
			blockNumber, cache->max_blocks - 1);
		return B_BAD_VALUE;
	}

	MutexLocker locker(&cache->lock);
	BlockWriter writer(cache);

	for (; numBlocks > 0; numBlocks--, blockNumber++) {
		cached_block* block = (cached_block*)hash_lookup(cache->hash,
			&blockNumber);
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
		cached_block* block = (cached_block*)hash_lookup(cache->hash,
			&blockNumber);
		if (block != NULL && block->previous_transaction != NULL)
			writer.Add(block);
	}

	writer.Write();
		// TODO: this can fail, too!

	blockNumber -= numBlocks;
		// reset blockNumber to its original value

	for (size_t i = 0; i < numBlocks; i++, blockNumber++) {
		cached_block* block = (cached_block*)hash_lookup(cache->hash,
			&blockNumber);
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
				panic("Discarded block %Ld has already been changed in this "
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
	block_cache* cache = (block_cache*)_cache;
	MutexLocker locker(&cache->lock);

	if (cache->read_only) {
		panic("tried to make block writable on a read-only cache!");
		return B_ERROR;
	}

	// TODO: this can be done better!
	void* block = get_writable_cached_block(cache, blockNumber,
		blockNumber, 1, transaction, false);
	if (block != NULL) {
		put_cached_block((block_cache*)_cache, blockNumber);
		return B_OK;
	}

	return B_ERROR;
}


void*
block_cache_get_writable_etc(void* _cache, off_t blockNumber, off_t base,
	off_t length, int32 transaction)
{
	block_cache* cache = (block_cache*)_cache;
	MutexLocker locker(&cache->lock);

	TRACE(("block_cache_get_writable_etc(block = %Ld, transaction = %ld)\n",
		blockNumber, transaction));
	if (cache->read_only)
		panic("tried to get writable block on a read-only cache!");

	return get_writable_cached_block(cache, blockNumber, base, length,
		transaction, false);
}


void*
block_cache_get_writable(void* _cache, off_t blockNumber, int32 transaction)
{
	return block_cache_get_writable_etc(_cache, blockNumber,
		blockNumber, 1, transaction);
}


void*
block_cache_get_empty(void* _cache, off_t blockNumber, int32 transaction)
{
	block_cache* cache = (block_cache*)_cache;
	MutexLocker locker(&cache->lock);

	TRACE(("block_cache_get_empty(block = %Ld, transaction = %ld)\n",
		blockNumber, transaction));
	if (cache->read_only)
		panic("tried to get empty writable block on a read-only cache!");

	return get_writable_cached_block((block_cache*)_cache, blockNumber,
		blockNumber, 1, transaction, true);
}


const void*
block_cache_get_etc(void* _cache, off_t blockNumber, off_t base, off_t length)
{
	block_cache* cache = (block_cache*)_cache;
	MutexLocker locker(&cache->lock);
	bool allocated;

	cached_block* block = get_cached_block(cache, blockNumber, &allocated);
	if (block == NULL)
		return NULL;

#if BLOCK_CACHE_DEBUG_CHANGED
	if (block->compare == NULL)
		block->compare = cache->Allocate();
	if (block->compare != NULL)
		memcpy(block->compare, block->current_data, cache->block_size);
#endif
	TB(Get(cache, block));

	return block->current_data;
}


const void*
block_cache_get(void* _cache, off_t blockNumber)
{
	return block_cache_get_etc(_cache, blockNumber, blockNumber, 1);
}


/*!	Changes the internal status of a writable block to \a dirty. This can be
	helpful in case you realize you don't need to change that block anymore
	for whatever reason.

	Note, you must only use this function on blocks that were acquired
	writable!
*/
status_t
block_cache_set_dirty(void* _cache, off_t blockNumber, bool dirty,
	int32 transaction)
{
	block_cache* cache = (block_cache*)_cache;
	MutexLocker locker(&cache->lock);

	cached_block* block = (cached_block*)hash_lookup(cache->hash,
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


void
block_cache_put(void* _cache, off_t blockNumber)
{
	block_cache* cache = (block_cache*)_cache;
	MutexLocker locker(&cache->lock);

	put_cached_block(cache, blockNumber);
}

