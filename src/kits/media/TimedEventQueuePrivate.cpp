/*
 * Copyright (c) 2002, 2003 Marcus Overhagen <Marcus@Overhagen.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files or portions
 * thereof (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice
 *    in the  binary, as well as this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided with
 *    the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

/* Implements _event_queue_imp used by BTimedEventQueue, not thread save!
 */
#include <string.h>

#include <Autolock.h>
#include <Buffer.h>
#include <InterfaceDefs.h> //defines B_DELETE
#include <TimedEventQueue.h>
#include <TimeSource.h>

#include "TimedEventQueuePrivate.h"

#include "Debug.h"
#include "debug.h"

_event_queue_imp::_event_queue_imp() :
	fLock(new BLocker("BTimedEventQueue locker")),
	fEventCount(0),
	fFirstEntry(NULL),
	fLastEntry(NULL),
	fCleanupHookContext(NULL),
	fCleanupHook(0)
{
}


_event_queue_imp::~_event_queue_imp()
{
	event_queue_entry *entry;
	entry = fFirstEntry;
	while (entry) {
		event_queue_entry *deleteme;
		deleteme = entry;
		entry = entry->next;
		delete deleteme;
	}
	delete fLock;
}

	
status_t
_event_queue_imp::AddEvent(const media_timed_event &event)
{
	BAutolock lock(fLock);

//	printf("        adding      %12Ld  at %12Ld\n", event.event_time, system_time());
	
	if (event.type <= 0) {
		 return B_BAD_VALUE;
	}
	
	*(bigtime_t *)&event.queued_time = BTimeSource::RealTime();

	//create a new queue
	if (fFirstEntry == NULL) {
		ASSERT(fEventCount == 0);
		ASSERT(fLastEntry == NULL);
		fFirstEntry = fLastEntry = new event_queue_entry;
		fFirstEntry->event = event;
		fFirstEntry->prev = NULL;
		fFirstEntry->next = NULL;
		fEventCount++;
		return B_OK;
	}
	
	//insert at queue begin
	if (fFirstEntry->event.event_time >= event.event_time) {
		event_queue_entry *newentry = new event_queue_entry;
		newentry->event = event;
		newentry->prev = NULL;
		newentry->next = fFirstEntry;
		fFirstEntry->prev = newentry;
		fFirstEntry = newentry;
		fEventCount++;
		return B_OK;
	}

	//insert at queue end
	if (fLastEntry->event.event_time <= event.event_time) {
		event_queue_entry *newentry = new event_queue_entry;
		newentry->event = event;
		newentry->prev = fLastEntry;
		newentry->next = NULL;
		fLastEntry->next = newentry;
		fLastEntry = newentry;
		fEventCount++;
		return B_OK;
	}
	
	//insert into the queue
	for (event_queue_entry *entry = fLastEntry; entry; entry = entry->prev) {
		if (entry->event.event_time <= event.event_time) {
			//insert after entry
			event_queue_entry *newentry = new event_queue_entry;
			newentry->event = event;
			newentry->prev = entry;
			newentry->next = entry->next;
			(entry->next)->prev = newentry;
			entry->next = newentry;
			fEventCount++;
			return B_OK;
		}
	}
	
	debugger("_event_queue_imp::AddEvent failed, should not be here\n");
	return B_OK;
}


status_t
_event_queue_imp::RemoveEvent(const media_timed_event *event)
{
	BAutolock lock(fLock);

	for (event_queue_entry *entry = fFirstEntry; entry; entry = entry->next) {
		if (entry->event == *event) {
			// No cleanup here
			RemoveEntry(entry);
			return B_OK;
		}
	}

	return B_ERROR;
}


status_t
_event_queue_imp::RemoveFirstEvent(media_timed_event * outEvent)
{
	BAutolock lock(fLock);
	
	if (fFirstEntry == 0)
		return B_ERROR;
	
	if (outEvent != 0) {
		// No cleanup here
		*outEvent = fFirstEntry->event;
	} else {
		CleanupEvent(&fFirstEntry->event);
	}

	RemoveEntry(fFirstEntry);

	return B_OK;
}

		
bool
_event_queue_imp::HasEvents() const
{
	return fEventCount != 0;
}


int32
_event_queue_imp::EventCount() const
{
	#if DEBUG > 1
		Dump();
	#endif
	return fEventCount;
}


const media_timed_event *	
_event_queue_imp::FirstEvent() const
{ 
	return fFirstEntry ? &fFirstEntry->event : NULL; 
}
			

bigtime_t
_event_queue_imp::FirstEventTime() const
{ 
	return fFirstEntry ? fFirstEntry->event.event_time : B_INFINITE_TIMEOUT; 
}


const media_timed_event *	
_event_queue_imp::LastEvent() const
{ 
	return fLastEntry ? &fLastEntry->event : NULL; 
}


bigtime_t	
_event_queue_imp::LastEventTime() const
{
	return fLastEntry ? fLastEntry->event.event_time : B_INFINITE_TIMEOUT; 
}


const media_timed_event *
_event_queue_imp::FindFirstMatch(bigtime_t eventTime,
								 BTimedEventQueue::time_direction direction,
								 bool inclusive,
								 int32 eventType)
{
	event_queue_entry *end;
	event_queue_entry *entry;

	BAutolock lock(fLock);

	switch (direction) {
		case BTimedEventQueue::B_ALWAYS:
			for (entry = fFirstEntry; entry; entry = entry->next) {
				if (eventType == BTimedEventQueue::B_ANY_EVENT || eventType == entry->event.type)
					return &entry->event;
			}
			return NULL;

		case BTimedEventQueue::B_BEFORE_TIME:
			end = GetEnd_BeforeTime(eventTime, inclusive);
			if (end == NULL)
				return NULL;
			end = end->next;
			for (entry = fFirstEntry; entry != end; entry = entry->next) {
				if (eventType == BTimedEventQueue::B_ANY_EVENT || eventType == entry->event.type) {
					return &entry->event;
				}
			}
			return NULL;

		case BTimedEventQueue::B_AT_TIME:
			{
				bool found_time = false;
				for (entry = fFirstEntry; entry; entry = entry->next) {
					if (eventTime == entry->event.event_time) {
						found_time = true;
						if (eventType == BTimedEventQueue::B_ANY_EVENT || eventType == entry->event.type)
							return &entry->event;
					} else if (found_time)
						return NULL;
				}
				return NULL;
			}

		case BTimedEventQueue::B_AFTER_TIME:
			for (entry = GetStart_AfterTime(eventTime, inclusive); entry; entry = entry->next) {
				if (eventType == BTimedEventQueue::B_ANY_EVENT || eventType == entry->event.type) {
					return &entry->event;
				}
			}
			return NULL;
	}

	return NULL;
}


status_t
_event_queue_imp::DoForEach(BTimedEventQueue::for_each_hook hook,
							void *context,
							bigtime_t eventTime,
							BTimedEventQueue::time_direction direction,
							bool inclusive,
							int32 eventType)
{
	event_queue_entry *end;
	event_queue_entry *entry;
	event_queue_entry *remove;
	BTimedEventQueue::queue_action action;

	BAutolock lock(fLock);

	switch (direction) {
		case BTimedEventQueue::B_ALWAYS:
			for (entry = fFirstEntry; entry; ) {
				if (eventType == BTimedEventQueue::B_ANY_EVENT || eventType == entry->event.type) {
					action = (*hook)(&entry->event, context);
					switch (action) {
						case BTimedEventQueue::B_DONE: 
							return B_OK;
						case BTimedEventQueue::B_REMOVE_EVENT:
							CleanupEvent(&entry->event);
							remove = entry;
							entry = entry->next;
							RemoveEntry(remove);
							break;
						case BTimedEventQueue::B_NO_ACTION:
						case BTimedEventQueue::B_RESORT_QUEUE:
						default:
							entry = entry->next;
							break;
					}
				} else {
					entry = entry->next;
				}
			}
			return B_OK;

		case BTimedEventQueue::B_BEFORE_TIME:
			end = GetEnd_BeforeTime(eventTime, inclusive);
			if (end == NULL)
				return B_OK;
			end = end->next;
			for (entry = fFirstEntry; entry != end; ) {
				if (eventType == BTimedEventQueue::B_ANY_EVENT || eventType == entry->event.type) {
					action = (*hook)(&entry->event, context);
					switch (action) {
						case BTimedEventQueue::B_DONE: 
							return B_OK;
						case BTimedEventQueue::B_REMOVE_EVENT:
							CleanupEvent(&entry->event);
							remove = entry;
							entry = entry->next;
							RemoveEntry(remove);
							break;
						case BTimedEventQueue::B_NO_ACTION:
						case BTimedEventQueue::B_RESORT_QUEUE:
						default:
							entry = entry->next;
							break;
					}
				} else {
					entry = entry->next;
				}
			}
			return B_OK;

		case BTimedEventQueue::B_AT_TIME:
			{
				bool found_time = false;
				for (entry = fFirstEntry; entry; ) {
					if (eventTime == entry->event.event_time) {
						found_time = true;
						if (eventType == BTimedEventQueue::B_ANY_EVENT || eventType == entry->event.type) {
							action = (*hook)(&entry->event, context);
							switch (action) {
								case BTimedEventQueue::B_DONE: 
									return B_OK;
								case BTimedEventQueue::B_REMOVE_EVENT:
									CleanupEvent(&entry->event);
									remove = entry;
									entry = entry->next;
									RemoveEntry(remove);
									break;
								case BTimedEventQueue::B_NO_ACTION:
								case BTimedEventQueue::B_RESORT_QUEUE:
								default:
									entry = entry->next;
									break;
							}
						} else {
							entry = entry->next;
						}
					} else if (found_time) {
						break;
					} else {
						entry = entry->next;
					}
				}
				return B_OK;
			}

		case BTimedEventQueue::B_AFTER_TIME:
			for (entry = GetStart_AfterTime(eventTime, inclusive); entry; ) {
				if (eventType == BTimedEventQueue::B_ANY_EVENT || eventType == entry->event.type) {
					action = (*hook)(&entry->event, context);
					switch (action) {
						case BTimedEventQueue::B_DONE: 
							return B_OK;
						case BTimedEventQueue::B_REMOVE_EVENT:
							CleanupEvent(&entry->event);
							remove = entry;
							entry = entry->next;
							RemoveEntry(remove);
							break;
						case BTimedEventQueue::B_NO_ACTION:
						case BTimedEventQueue::B_RESORT_QUEUE:
						default:
							entry = entry->next;
							break;
					}
				} else {
					entry = entry->next;
				}
			}
			return B_OK;
	}

	return B_ERROR;
}

		
status_t
_event_queue_imp::FlushEvents(bigtime_t eventTime,
							  BTimedEventQueue::time_direction direction,
							  bool inclusive,
							  int32 eventType)
{
	event_queue_entry *end;
	event_queue_entry *entry;
	event_queue_entry *remove;

	BAutolock lock(fLock);

	switch (direction) {
		case BTimedEventQueue::B_ALWAYS:
			for (entry = fFirstEntry; entry; ) {
				if (eventType == BTimedEventQueue::B_ANY_EVENT || eventType == entry->event.type) {
					CleanupEvent(&entry->event);
					remove = entry;
					entry = entry->next;
					RemoveEntry(remove);
				} else {
					entry = entry->next;
				}
			}
			return B_OK;

		case BTimedEventQueue::B_BEFORE_TIME:
			end = GetEnd_BeforeTime(eventTime, inclusive);
			if (end == NULL)
				return B_OK;
			end = end->next;
			for (entry = fFirstEntry; entry != end; ) {
				if (eventType == BTimedEventQueue::B_ANY_EVENT || eventType == entry->event.type) {
					CleanupEvent(&entry->event);
					remove = entry;
					entry = entry->next;
					RemoveEntry(remove);
				} else {
					entry = entry->next;
				}
			}
			return B_OK;

		case BTimedEventQueue::B_AT_TIME:
			{
				bool found_time = false;
				for (entry = fFirstEntry; entry; ) {
					if (eventTime == entry->event.event_time) {
						found_time = true;
						if (eventType == BTimedEventQueue::B_ANY_EVENT || eventType == entry->event.type) {
							CleanupEvent(&entry->event);
							remove = entry;
							entry = entry->next;
							RemoveEntry(remove);
						} else {
							entry = entry->next;
						}
					} else if (found_time) {
						break;
					} else {
						entry = entry->next;
					}
				}
				return B_OK;
			}

		case BTimedEventQueue::B_AFTER_TIME:
			for (entry = GetStart_AfterTime(eventTime, inclusive); entry; ) {
				if (eventType == BTimedEventQueue::B_ANY_EVENT || eventType == entry->event.type) {
					CleanupEvent(&entry->event);
					remove = entry;
					entry = entry->next;
					RemoveEntry(remove);
				} else {
					entry = entry->next;
				}
			}
			return B_OK;
	}

	return B_ERROR;
}


void
_event_queue_imp::SetCleanupHook(BTimedEventQueue::cleanup_hook hook, void *context)
{
	BAutolock lock(fLock);
	fCleanupHookContext = context;
	fCleanupHook = hook;
}


void 
_event_queue_imp::RemoveEntry(event_queue_entry *entry)
{
	//remove the entry from double-linked list
	//and delete it
	if (entry->prev != NULL)
		(entry->prev)->next = entry->next;
	if (entry->next != NULL)
		(entry->next)->prev = entry->prev;
	if (entry == fFirstEntry) {
		fFirstEntry = entry->next;
		if (fFirstEntry != NULL)
			fFirstEntry->prev = NULL;
	}
	if (entry == fLastEntry) {
		fLastEntry = entry->prev;
		if (fLastEntry != NULL)
			fLastEntry->next = NULL;
	}
	delete entry;
	fEventCount--;
	ASSERT(fEventCount >= 0);
	ASSERT(fEventCount != 0 || ((fFirstEntry == NULL) && (fLastEntry == NULL)));
}


void 
_event_queue_imp::CleanupEvent(media_timed_event *event)
{
	//perform the cleanup action required
	//when deleting an event from the queue

	//BeBook says:
	//	Each event has a cleanup flag associated with it that indicates 
	//  what sort of special action needs to be performed when the event is 
	//  removed from the queue. If this value is B_NO_CLEANUP, nothing is done. 
	//  If it's B_RECYCLE, and the event is a B_HANDLE_BUFFER event, BTimedEventQueue
	//  will automatically recycle the buffer associated with the event. 
	//  If the cleanup flag is B_DELETE or is B_USER_CLEANUP or greater, 
	//  the cleanup hook function will be called. 
	//and:
	//  cleanup hook function specified by hook to be called for events
	//  as they're removed from the queue. The hook will be called only 
	//  for events with cleanup_flag values of B_DELETE or B_USER_CLEANUP or greater. 
	//and:
	//  These values define how BTimedEventQueue should handle removing 
	//  events from the queue. If the flag is B_USER_CLEANUP or greater, 
	//  the cleanup hook function is called when the event is removed. 
	//Problems:
	//  B_DELETE is a keyboard code! (seems to have existed in early 
	//  sample code as a cleanup flag)
	//
	//  exiting cleanup flags are:
	//		B_NO_CLEANUP = 0,
	//		B_RECYCLE_BUFFER,		// recycle buffers handled by BTimedEventQueue
	//		B_EXPIRE_TIMER,			// call TimerExpired() on the event->data
	//		B_USER_CLEANUP = 0x4000	// others go to the cleanup func
	//	
	
	if (event->cleanup == BTimedEventQueue::B_NO_CLEANUP) {
		// do nothing
	} else if (event->type == BTimedEventQueue::B_HANDLE_BUFFER && event->cleanup == BTimedEventQueue::B_RECYCLE_BUFFER) {
		((BBuffer *)event->pointer)->Recycle();
		DEBUG_ONLY(*const_cast<void **>(&event->pointer) = NULL);
	} else if (event->cleanup == BTimedEventQueue::B_EXPIRE_TIMER) {
		// call TimerExpired() on the event->data
		debugger("BTimedEventQueue cleanup: calling TimerExpired() should be implemented here\n");
	} else if (event->cleanup == B_DELETE || event->cleanup >= BTimedEventQueue::B_USER_CLEANUP) {
		if (fCleanupHook)
			(*fCleanupHook)(event,fCleanupHookContext);
	} else {
		ERROR("BTimedEventQueue cleanup unhandled! type = %ld, cleanup = %ld\n", event->type, event->cleanup);
	}
}


_event_queue_imp::event_queue_entry *
_event_queue_imp::GetEnd_BeforeTime(bigtime_t eventTime, bool inclusive)
{
	event_queue_entry *entry;
	
	entry = fLastEntry;
	while (entry) {
		if ((entry->event.event_time > eventTime) || (!inclusive && entry->event.event_time == eventTime))
			entry = entry->prev;
		else
			break;
	}
	return entry;
}


_event_queue_imp::event_queue_entry *
_event_queue_imp::GetStart_AfterTime(bigtime_t eventTime, bool inclusive)
{
	event_queue_entry *entry;
	
	entry = fFirstEntry;
	while (entry) {
		if ((entry->event.event_time < eventTime) || (!inclusive && entry->event.event_time == eventTime))
			entry = entry->next;
		else
			break;
	}
	return entry;
}


#if DEBUG > 1
void
_event_queue_imp::Dump() const
{
	TRACE("fEventCount = 0x%x\n",(int)fEventCount);
	TRACE("fFirstEntry = 0x%x\n",(int)fFirstEntry);
	TRACE("fLastEntry  = 0x%x\n",(int)fLastEntry);
	for (event_queue_entry *entry = fFirstEntry; entry; entry = entry->next) {
		TRACE("entry = 0x%x\n",(int)entry);
		TRACE("  entry.prev = 0x%x\n",(int)entry->prev);
		TRACE("  entry.next = 0x%x\n",(int)entry->next);
		TRACE("  entry.event.event_time = 0x%x\n",(int)entry->event.event_time);
	}
}
#endif
