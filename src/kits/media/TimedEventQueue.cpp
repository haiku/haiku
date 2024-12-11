/*
 * Copyright 2024, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <TimedEventQueue.h>

#include <Autolock.h>
#include <Buffer.h>
#include <MediaDebug.h>
#include <InterfaceDefs.h>
#include <util/DoublyLinkedList.h>
#include <locks.h>


//	#pragma mark - media_timed_event


media_timed_event::media_timed_event()
{
	CALLED();
	memset(this, 0, sizeof(*this));
}


media_timed_event::media_timed_event(bigtime_t inTime, int32 inType)
{
	CALLED();
	memset(this, 0, sizeof(*this));

	event_time = inTime;
	type = inType;
}


media_timed_event::media_timed_event(bigtime_t inTime, int32 inType,
	void* inPointer, uint32 inCleanup)
{
	CALLED();
	memset(this, 0, sizeof(*this));

	event_time = inTime;
	type = inType;
	pointer = inPointer;
	cleanup = inCleanup;
}


media_timed_event::media_timed_event(bigtime_t inTime, int32 inType,
	void* inPointer, uint32 inCleanup,
	int32 inData, int64 inBigdata,
	const char* inUserData,  size_t dataSize)
{
	CALLED();
	memset(this, 0, sizeof(*this));

	event_time = inTime;
	type = inType;
	pointer = inPointer;
	cleanup = inCleanup;
	data = inData;
	bigdata = inBigdata;
	memcpy(user_data, inUserData,
		min_c(sizeof(media_timed_event::user_data), dataSize));
}


media_timed_event::media_timed_event(const media_timed_event& clone)
{
	CALLED();
	*this = clone;
}


void
media_timed_event::operator=(const media_timed_event& clone)
{
	CALLED();
	memcpy(this, &clone, sizeof(*this));
}


media_timed_event::~media_timed_event()
{
	CALLED();
}


//	#pragma mark - media_timed_event global operators


bool
operator==(const media_timed_event& a, const media_timed_event& b)
{
	CALLED();
	return (memcmp(&a, &b, sizeof(media_timed_event)) == 0);
}


bool
operator!=(const media_timed_event& a, const media_timed_event& b)
{
	CALLED();
	return (memcmp(&a, &b, sizeof(media_timed_event)) != 0);
}


bool
operator<(const media_timed_event& a, const media_timed_event& b)
{
	CALLED();
	return a.event_time < b.event_time;
}


bool
operator>(const media_timed_event& a, const media_timed_event& b)
{
	CALLED();
	return a.event_time > b.event_time;
}


//	#pragma mark - BTimedEventQueue


namespace {

struct queue_entry : public DoublyLinkedListLinkImpl<queue_entry> {
	media_timed_event	event;
};
typedef DoublyLinkedList<queue_entry> QueueEntryList;

} // namespace


class BPrivate::TimedEventQueueData {
public:
	TimedEventQueueData()
		:
		fLock("BTimedEventQueue")
	{
		mutex_init(&fEntryAllocationLock, "BTimedEventQueue entry allocation");

		fEventCount = 0;
		fCleanupHook = NULL;
		fCleanupHookContext = NULL;

		for (size_t i = 0; i < B_COUNT_OF(fInlineEntries); i++)
			fFreeEntries.Add(&fInlineEntries[i]);
	}
	~TimedEventQueueData()
	{
		while (queue_entry* chunk = fChunkHeads.RemoveHead())
			free(chunk);
	}

	queue_entry* AllocateEntry()
	{
		MutexLocker locker(fEntryAllocationLock);
		if (fFreeEntries.Head() != NULL)
			return fFreeEntries.RemoveHead();

		// We need a new chunk.
		const size_t chunkSize = B_PAGE_SIZE;
		queue_entry* newEntries = (queue_entry*)malloc(chunkSize);
		fChunkHeads.Add(&newEntries[0]);
		for (size_t i = 1; i < (chunkSize / sizeof(queue_entry)); i++)
			fFreeEntries.Add(&newEntries[i]);

		return fFreeEntries.RemoveHead();
	}

	void FreeEntry(queue_entry* entry)
	{
		MutexLocker locker(fEntryAllocationLock);
		fFreeEntries.Add(entry);

		// TODO: Chunks are currently only freed in the destructor.
		// (Is that a problem? They're probably rarely used, anyway.)
	}

	status_t	AddEntry(queue_entry* newEntry);
	void		RemoveEntry(queue_entry* newEntry);
	void		CleanupAndFree(queue_entry* entry);

public:
	BLocker				fLock;
	QueueEntryList		fEvents;
	int32				fEventCount;

	BTimedEventQueue::cleanup_hook 	fCleanupHook;
	void* 				fCleanupHookContext;

private:
	mutex				fEntryAllocationLock;
	QueueEntryList		fFreeEntries;
	QueueEntryList		fChunkHeads;
	queue_entry			fInlineEntries[8];
};

using BPrivate::TimedEventQueueData;


BTimedEventQueue::BTimedEventQueue()
	: fData(new TimedEventQueueData)
{
	CALLED();
}


BTimedEventQueue::~BTimedEventQueue()
{
	CALLED();

	FlushEvents(0, B_ALWAYS);
	delete fData;
}


status_t
BTimedEventQueue::AddEvent(const media_timed_event& event)
{
	CALLED();

	if (event.type < B_START)
		return B_BAD_VALUE;

	queue_entry* newEntry = fData->AllocateEntry();
	if (newEntry == NULL)
		return B_NO_MEMORY;

	newEntry->event = event;

	BAutolock locker(fData->fLock);
	return fData->AddEntry(newEntry);
}


status_t
BTimedEventQueue::RemoveEvent(const media_timed_event* event)
{
	CALLED();
	BAutolock locker(fData->fLock);

	QueueEntryList::Iterator it = fData->fEvents.GetIterator();
	while (queue_entry* entry = it.Next()) {
		if (entry->event != *event)
			continue;

		fData->RemoveEntry(entry);

		locker.Unlock();

		// No cleanup.
		fData->FreeEntry(entry);
		return B_OK;
	}

	return B_ERROR;
}


status_t
BTimedEventQueue::RemoveFirstEvent(media_timed_event* _event)
{
	CALLED();
	BAutolock locker(fData->fLock);

	if (fData->fEventCount == 0)
		return B_ERROR;

	queue_entry* entry = fData->fEvents.Head();
	fData->RemoveEntry(entry);

	locker.Unlock();

	if (_event != NULL) {
		// No cleanup.
		*_event = entry->event;
		fData->FreeEntry(entry);
	} else {
		fData->CleanupAndFree(entry);
	}
	return B_OK;
}


status_t
TimedEventQueueData::AddEntry(queue_entry* newEntry)
{
	ASSERT(fLock.IsLocked());

	if (fEvents.IsEmpty()) {
		fEvents.Add(newEntry);
		fEventCount++;
		return B_OK;
	}
	if (fEvents.First()->event.event_time > newEntry->event.event_time) {
		fEvents.Add(newEntry, false);
		fEventCount++;
		return B_OK;
	}

	QueueEntryList::ReverseIterator it = fEvents.GetReverseIterator();
	while (queue_entry* entry = it.Next()) {
		if (newEntry->event.event_time < entry->event.event_time)
			continue;

		// Insert the new event after this entry.
		fEvents.InsertAfter(entry, newEntry);
		fEventCount++;
		return B_OK;
	}

	debugger("BTimedEventQueue::AddEvent: Invalid queue!");
	return B_ERROR;
}


void
TimedEventQueueData::RemoveEntry(queue_entry* entry)
{
	ASSERT(fLock.IsLocked());

	fEvents.Remove(entry);
	fEventCount--;
}


void
TimedEventQueueData::CleanupAndFree(queue_entry* entry)
{
	uint32 cleanup = entry->event.cleanup;
	if (cleanup == B_DELETE) {
		// B_DELETE is a keyboard code, but the Be Book indicates it's a valid
		// cleanup value. (Early sample code may have used it too.)
		cleanup = BTimedEventQueue::B_USER_CLEANUP;
	}

	if (cleanup == BTimedEventQueue::B_NO_CLEANUP) {
		// Nothing to do.
	} else if (entry->event.type == BTimedEventQueue::B_HANDLE_BUFFER
			&& cleanup == BTimedEventQueue::B_RECYCLE_BUFFER) {
		(reinterpret_cast<BBuffer*>(entry->event.pointer))->Recycle();
	} else if (cleanup == BTimedEventQueue::B_EXPIRE_TIMER) {
		// TimerExpired() is invoked in BMediaEventLooper::DispatchEvent; nothing to do.
	} else if (cleanup >= BTimedEventQueue::B_USER_CLEANUP) {
		if (fCleanupHook != NULL)
			(*fCleanupHook)(&entry->event, fCleanupHookContext);
	} else {
		ERROR("BTimedEventQueue: Unhandled cleanup! (type %" B_PRId32 ", "
			"cleanup %" B_PRId32 ")\n", entry->event.type, entry->event.cleanup);
	}

	FreeEntry(entry);
}


void
BTimedEventQueue::SetCleanupHook(cleanup_hook hook, void* context)
{
	CALLED();

	BAutolock lock(fData->fLock);
	fData->fCleanupHook = hook;
	fData->fCleanupHookContext = context;
}


bool
BTimedEventQueue::HasEvents() const
{
	CALLED();

	BAutolock locker(fData->fLock);
	return fData->fEventCount != 0;
}


int32
BTimedEventQueue::EventCount() const
{
	CALLED();

	BAutolock locker(fData->fLock);
	return fData->fEventCount;
}


const media_timed_event*
BTimedEventQueue::FirstEvent() const
{
	CALLED();
	BAutolock locker(fData->fLock);

	queue_entry* entry = fData->fEvents.First();
	if (entry == NULL)
		return NULL;
	return &entry->event;
}


bigtime_t
BTimedEventQueue::FirstEventTime() const
{
	CALLED();
	BAutolock locker(fData->fLock);

	queue_entry* entry = fData->fEvents.First();
	if (entry == NULL)
		return B_INFINITE_TIMEOUT;
	return entry->event.event_time;
}


const media_timed_event*
BTimedEventQueue::LastEvent() const
{
	CALLED();
	BAutolock locker(fData->fLock);

	queue_entry* entry = fData->fEvents.Last();
	if (entry == NULL)
		return NULL;
	return &entry->event;
}


bigtime_t
BTimedEventQueue::LastEventTime() const
{
	CALLED();
	BAutolock locker(fData->fLock);

	queue_entry* entry = fData->fEvents.Last();
	if (entry == NULL)
		return B_INFINITE_TIMEOUT;
	return entry->event.event_time;
}


const media_timed_event*
BTimedEventQueue::FindFirstMatch(bigtime_t eventTime,
	time_direction direction, bool inclusive, int32 eventType)
{
	CALLED();
	BAutolock locker(fData->fLock);

	QueueEntryList::Iterator it = fData->fEvents.GetIterator();
	while (queue_entry* entry = it.Next()) {
		int match = _Match(entry->event, eventTime, direction, inclusive, eventType);
		if (match == B_DONE)
			break;
		if (match == B_NO_ACTION)
			continue;

		return &entry->event;
	}

	return NULL;
}


status_t
BTimedEventQueue::DoForEach(for_each_hook hook, void* context,
	bigtime_t eventTime, time_direction direction,
	bool inclusive, int32 eventType)
{
	CALLED();
	BAutolock locker(fData->fLock);

	bool resort = false;

	QueueEntryList::Iterator it = fData->fEvents.GetIterator();
	while (queue_entry* entry = it.Next()) {
		int match = _Match(entry->event, eventTime, direction, inclusive, eventType);
		if (match == B_DONE)
			break;
		if (match == B_NO_ACTION)
			continue;

		queue_action action = hook(&entry->event, context);
		if (action == B_DONE)
			break;

		switch (action) {
			case B_REMOVE_EVENT:
				fData->RemoveEntry(entry);
				fData->CleanupAndFree(entry);
				break;

			case B_RESORT_QUEUE:
				resort = true;
				break;

			case B_NO_ACTION:
			default:
				break;
		}
	}

	if (resort) {
		QueueEntryList entries;
		entries.TakeFrom(&fData->fEvents);
		fData->fEventCount = 0;

		while (queue_entry* entry = entries.RemoveHead())
			fData->AddEntry(entry);
	}

	return B_OK;
}


status_t
BTimedEventQueue::FlushEvents(bigtime_t eventTime, time_direction direction,
	bool inclusive, int32 eventType)
{
	CALLED();
	BAutolock locker(fData->fLock);

	QueueEntryList::Iterator it = fData->fEvents.GetIterator();
	while (queue_entry* entry = it.Next()) {
		int match = _Match(entry->event, eventTime, direction, inclusive, eventType);
		if (match == B_DONE)
			break;
		if (match == B_NO_ACTION)
			continue;

		fData->RemoveEntry(entry);
		fData->CleanupAndFree(entry);
	}

	return B_OK;
}


int
BTimedEventQueue::_Match(const media_timed_event& event,
	bigtime_t eventTime, time_direction direction,
	bool inclusive, int32 eventType)
{
	if (direction == B_ALWAYS) {
		// Nothing to check.
	} else if (direction == B_BEFORE_TIME) {
		if (event.event_time > eventTime)
			return B_DONE;
		if (event.event_time == eventTime && !inclusive)
			return B_DONE;
	} else if (direction == B_AT_TIME) {
		if (event.event_time > eventTime)
			return B_DONE;
		if (event.event_time != eventTime)
			return B_NO_ACTION;
	} else if (direction == B_AFTER_TIME) {
		if (event.event_time < eventTime)
			return B_NO_ACTION;
		if (event.event_time == eventTime && !inclusive)
			return B_NO_ACTION;
	}

	if (eventType != B_ANY_EVENT && eventType != event.type)
		return B_NO_ACTION;

	return 1;
}


//	#pragma mark - C++ binary compatibility


void*
BTimedEventQueue::operator new(size_t size)
{
	CALLED();
	return ::operator new(size);
}


void
BTimedEventQueue::operator delete(void* ptr, size_t s)
{
	CALLED();
	return ::operator delete(ptr);
}


void BTimedEventQueue::_ReservedTimedEventQueue0() {}
void BTimedEventQueue::_ReservedTimedEventQueue1() {}
void BTimedEventQueue::_ReservedTimedEventQueue2() {}
void BTimedEventQueue::_ReservedTimedEventQueue3() {}
void BTimedEventQueue::_ReservedTimedEventQueue4() {}
void BTimedEventQueue::_ReservedTimedEventQueue5() {}
void BTimedEventQueue::_ReservedTimedEventQueue6() {}
void BTimedEventQueue::_ReservedTimedEventQueue7() {}
void BTimedEventQueue::_ReservedTimedEventQueue8() {}
void BTimedEventQueue::_ReservedTimedEventQueue9() {}
void BTimedEventQueue::_ReservedTimedEventQueue10() {}
void BTimedEventQueue::_ReservedTimedEventQueue11() {}
void BTimedEventQueue::_ReservedTimedEventQueue12() {}
void BTimedEventQueue::_ReservedTimedEventQueue13() {}
void BTimedEventQueue::_ReservedTimedEventQueue14() {}
void BTimedEventQueue::_ReservedTimedEventQueue15() {}
void BTimedEventQueue::_ReservedTimedEventQueue16() {}
void BTimedEventQueue::_ReservedTimedEventQueue17() {}
void BTimedEventQueue::_ReservedTimedEventQueue18() {}
void BTimedEventQueue::_ReservedTimedEventQueue19() {}
void BTimedEventQueue::_ReservedTimedEventQueue20() {}
void BTimedEventQueue::_ReservedTimedEventQueue21() {}
void BTimedEventQueue::_ReservedTimedEventQueue22() {}
void BTimedEventQueue::_ReservedTimedEventQueue23() {}
