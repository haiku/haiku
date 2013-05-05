/*
 * Copyright 2001-2009, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold (bonefish@users.sf.net)
 */


#include "EventQueue.h"

#include <stdio.h>

#include <String.h>

#include "Event.h"


static const char *kDefaultEventQueueName = "event looper";


/*!	\class EventQueue
	\brief A class providing a mechanism for executing events at specified
		   times.

	The class' interface is quite small. It basically features methods to
	add or remove an event or to modify an event's time. It is derived from
	BLocker to inherit a locking mechanism needed to serialize the access to
	its member variables (especially the event list).

	The queue runs an own thread (in _EventLooper()), which executes the
	events at the right times. The execution of an event consists of invoking
	its Event::Do() method. If the event's Event::IsAutoDelete() or its Do()
	method return \c true, the event object is deleted after execution. In
	any case the event is removed from the list before it is executed. The
	queue is not locked while an event is executed.

	The event list (\a fEvents) is ordered ascendingly by time. The thread
	uses a semaphore (\a fLooperControl) to wait (time out) for the next
	event. This semaphore is released to indicate changes to the event list.
*/

/*!	\var BList EventQueue::fEvents
	\brief List of events (Event*) in ascending order (by time).
*/

/*!	\var thread_id EventQueue::fEventLooper
	\brief Thread ID of the queue's thread.
*/

/*!	\var sem_id EventQueue::fLooperControl
	\brief Queue thread control semaphore.

	The semaphore is initialized with count \c 0 and used by the queue's
	thread for timing out at the time when the next event has to be executed.
	If the event list is changed by another thread and that changed the time
	of the next event the semaphore is released (_Reschedule()).
*/

/*!	\var volatile bigtime_t EventQueue::fNextEventTime
	\brief Time at which the queue's thread will wake up next time.
*/

/*!	\var status_t EventQueue::fStatus
	\brief Initialization status.
*/

/*!	\var volatile bool EventQueue::fTerminating
	\brief Set to \c true by Die() to signal the queue's thread not to
		   execute any more events.
*/


/*!	\brief Creates a new event queue.

	The status of the initialization can and should be check with InitCheck().

	\param name The name used for the queue's thread and semaphore. If \c NULL,
		   a default name is used.
*/
EventQueue::EventQueue(const char *name)
	:
	fEvents(100),
	fEventLooper(-1),
	fLooperControl(-1),
	fNextEventTime(0),
	fStatus(B_ERROR),
	fTerminating(false)
{
	if (!name)
		name = kDefaultEventQueueName;
	fLooperControl = create_sem(0, (BString(name) += " control").String());
	if (fLooperControl >= B_OK)
		fStatus = B_OK;
	else
		fStatus = fLooperControl;
	if (fStatus == B_OK) {
		fEventLooper = spawn_thread(_EventLooperEntry, name,
			B_DISPLAY_PRIORITY + 1, this);
		if (fEventLooper >= B_OK) {
			fStatus = B_OK;
			resume_thread(fEventLooper);
		} else
			fStatus = fEventLooper;
	}
}


/*!	\brief Frees all resources associated by this object.

	Die() is called to terminate the queue's thread and all events whose
	IsAutoDelete() returns \c true are deleted.
*/
EventQueue::~EventQueue()
{
	Die();
	while (Event *event = (Event*)fEvents.RemoveItem((int32)0)) {
		if (event->IsAutoDelete())
			delete event;
	}
}


/*!	\brief Returns the initialization status of the event queue.
	\return \c B_OK, if everything went fine, an error code otherwise.
*/
status_t
EventQueue::InitCheck()
{
	return fStatus;
}


/*!	\brief Terminates the queue's thread.

	If an event is currently executed, it is allowed to finish its task
	normally, but no more events will be executed.

	Afterwards events can still be added to and removed from the queue.
*/
void
EventQueue::Die()
{
	fTerminating = true;
	if (delete_sem(fLooperControl) == B_OK) {
		int32 dummy;
		wait_for_thread(fEventLooper, &dummy);
	}
}


/*!	\brief Adds a new event to the queue.

	The event's time must be set, before adding it. Afterwards ModifyEvent()
	must be used to change an event's time.

	If the event's time is already passed, it is executed as soon as possible.

	\param event The event to be added.
	\return \c true, if the event has been added successfully, \c false, if
			an error occured.
*/
bool
EventQueue::AddEvent(Event *event)
{
	Lock();
	bool result = (event && _AddEvent(event));
	if (result)
		_Reschedule();
	Unlock();
	return result;
}


/*!	\brief Removes an event from the queue.
	\param event The event to be removed.
	\return \c true, if the event has been removed successfully, \c false, if
			the supplied event wasn't in the queue before.
*/
bool
EventQueue::RemoveEvent(Event *event)
{
	bool result = false;
	Lock();
	if (event && ((result = _RemoveEvent(event))))
		_Reschedule();
	Unlock();
	return result;
}


/*!	\brief Modifies an event's time.

	The event must be in the queue.

	If the event's new time is already passed, it is executed as soon as
	possible.

	\param event The event in question.
	\param newTime The new event time.
*/
void
EventQueue::ModifyEvent(Event *event, bigtime_t newTime)
{
	Lock();
	if (fEvents.RemoveItem(event)) {
		event->SetTime(newTime);
		_AddEvent(event);
		_Reschedule();
	}
	Unlock();
}


/*!	\brief Adds an event to the event list.

	\note The object must be locked when this method is invoked.

	\param event The event to be added.
	\return \c true, if the event has been added successfully, \c false, if
			an error occured.
*/
bool
EventQueue::_AddEvent(Event *event)
{
	int32 index = _FindInsertionIndex(event->Time());
	return fEvents.AddItem(event, index);
}


/*!	\brief Removes an event from the event list.

	\note The object must be locked when this method is invoked.

	\param event The event to be removed.
	\return \c true, if the event has been removed successfully, \c false, if
			the supplied event wasn't in the queue before.
*/
bool
EventQueue::_RemoveEvent(Event *event)
{
	int32 index = _IndexOfEvent(event);
	return (index >= 0 && fEvents.RemoveItem(index));
}


/*!	\brief Returns an event from the event list.

	\note The object must be locked when this method is invoked.

	\param index The list index of the event to be returned.
	\return The event, or \c NULL, if the index is out of range.
*/
Event*
EventQueue::_EventAt(int32 index) const
{
	return (Event*)fEvents.ItemAt(index);
}


/*!	\brief Returns the event list index of the supplied event.

	\note The object must be locked when this method is invoked.

	\param event The event to be found.
	\return The event list index of the supplied event or \c -1, if the event
			is not in the event list.
*/
int32
EventQueue::_IndexOfEvent(Event *event) const
{
	bigtime_t time = event->Time();
	int32 index = _FindInsertionIndex(time);
	// The found index is the index succeeding the one of the last event with
	// the same time.
	// search backwards for the event
	Event *listEvent = NULL;
	while (((listEvent = _EventAt(--index))) && listEvent->Time() == time) {
		if (listEvent == event)
			return index;
	}
	return -1;
}


/*!	\brief Finds the event list index at which an event with the supplied
		   has to be added.

	The returned index is the greatest possible index, that is if there are
	events with the same time in the list, the index is the successor of the
	index of the last of these events.

	\note The object must be locked when this method is invoked.

	\param time The event time.
	\return The index at which an event with the supplied time should be added.
*/
int32
EventQueue::_FindInsertionIndex(bigtime_t time) const
{
	// binary search
	int32 lower = 0;
	int32 upper = fEvents.CountItems();
	// invariant: lower-1:time <= time < upper:time (ignoring invalid indices)
	while (lower < upper) {
		int32 mid = (lower + upper) / 2;
		Event* midEvent = _EventAt(mid);
		if (time < midEvent->Time())
			upper = mid;		// time < mid:time  =>  time < upper:time
		else
			lower = mid + 1;	// mid:time <= time  =>  lower-1:time <= time
	}
	return lower;
}


/*!	\brief Entry point from the queue's thread.
	\param data The queue's \c this pointer.
	\return The thread's result. Of no relevance in this case.
*/
int32
EventQueue::_EventLooperEntry(void *data)
{
	return ((EventQueue*)data)->_EventLooper();
}


/*!	\brief Method with the main loop of the queue's thread.
	\return The thread's result. Of no relevance in this case.
*/
int32
EventQueue::_EventLooper()
{
	bool running = true;
	while (running) {
		bigtime_t waitUntil = B_INFINITE_TIMEOUT;
		if (Lock()) {
			if (!fEvents.IsEmpty())
				waitUntil = _EventAt(0)->Time();
			fNextEventTime = waitUntil;
			Unlock();
		}
		status_t err = acquire_sem_etc(fLooperControl, 1, B_ABSOLUTE_TIMEOUT,
									   waitUntil);
		switch (err) {
			case B_TIMED_OUT:
				// do events, that are supposed to go off
				while (!fTerminating && Lock() && !fEvents.IsEmpty()
					   && system_time() >= _EventAt(0)->Time()) {
					Event *event = (Event*)fEvents.RemoveItem((int32)0);
					Unlock();
					bool autoDeleteEvent = event->IsAutoDelete();
					bool deleteEvent = event->Do(this) || autoDeleteEvent;
					if (deleteEvent)
						delete event;
				}
				if (IsLocked())
					Unlock();
				break;
			case B_BAD_SEM_ID:
				running = false;
				break;
			case B_OK:
			default:
				break;
		}
	}
	return 0;
}


/*!	\brief To be called, when an event has been added or removed.

	Checks whether the queue's thread has to recalculate the time when it
	needs to wake up to execute the next event, and signals the thread to
	do so, if necessary.

	\note The object must be locked when this method is invoked.
*/
void
EventQueue::_Reschedule()
{
	if (fStatus == B_OK) {
		if (!fEvents.IsEmpty() && _EventAt(0)->Time() < fNextEventTime)
			release_sem(fLooperControl);
	}
}
