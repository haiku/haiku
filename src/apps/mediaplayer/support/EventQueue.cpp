/*	
 * Copyright (c) 2000-2008, Ingo Weinhold <ingo_weinhold@gmx.de>,
 * Copyright (c) 2000-2008, Stephan Aßmus <superstippi@gmx.de>,
 * All Rights Reserved. Distributed under the terms of the MIT license.
 */
#include <new>
#include <stdio.h>

#include "Event.h"

#include "EventQueue.h"

using std::nothrow;


EventQueue::EventQueue()
	: fEvents(100),
	  fEventExecutor(-1),
	  fThreadControl(-1),
	  fNextEventTime(0),
	  fStatus(B_ERROR)
	  
{
	fThreadControl = create_sem(0, "event queue control");
	if (fThreadControl >= B_OK)
		fStatus = B_OK;
	else
		fStatus = fThreadControl;
	if (fStatus == B_OK) {
		fEventExecutor = spawn_thread(_execute_events_, "event queue runner",
									  B_NORMAL_PRIORITY, this);
		if (fEventExecutor >= B_OK) {
			fStatus = B_OK;
			resume_thread(fEventExecutor);
		} else
			fStatus = fEventExecutor;
	}
}


EventQueue::~EventQueue()
{
	if (delete_sem(fThreadControl) == B_OK)
		wait_for_thread(fEventExecutor, &fEventExecutor);
	while (Event *event = (Event*)fEvents.RemoveItem(0L)) {
		if (event->AutoDelete())
			delete event;
	}
}


status_t
EventQueue::InitCheck()
{
	return fStatus;
}


EventQueue*
EventQueue::CreateDefault()
{
	if (!fDefaultQueue) {
		fDefaultQueue = new(nothrow) EventQueue;
		if (fDefaultQueue && fDefaultQueue->InitCheck() != B_OK)
			DeleteDefault();
	}
	return fDefaultQueue;
}


void
EventQueue::DeleteDefault()
{
	if (fDefaultQueue) {
		delete fDefaultQueue;
		fDefaultQueue = NULL;
	}
}


EventQueue&
EventQueue::Default()
{
	return *fDefaultQueue;
}


void
EventQueue::AddEvent(Event* event)
{
	Lock();
	_AddEvent(event);
	_Reschedule();
	Unlock();
}


bool
EventQueue::RemoveEvent(Event* event)
{
	bool result = false;
	Lock();
	if ((result = fEvents.RemoveItem(event)))
		_Reschedule();
	Unlock();
	return result;
}


void
EventQueue::ChangeEvent(Event* event, bigtime_t newTime)
{
	Lock();
	if (fEvents.RemoveItem(event)) {
		event->SetTime(newTime);
		_AddEvent(event);
		_Reschedule();
	}
	Unlock();
}


// PRE: The object must be locked.
void
EventQueue::_AddEvent(Event* event)
{
	// find the insertion index
	int32 lower = 0;
	int32 upper = fEvents.CountItems();
	while (lower < upper) {
		int32 mid = (lower + upper) / 2;
		Event* midEvent = _EventAt(mid);
		if (event->Time() < midEvent->Time())
			upper = mid;
		else
			lower = mid + 1;
	}
	fEvents.AddItem(event, lower);
}


Event*
EventQueue::_EventAt(int32 index) const
{
	return (Event*)fEvents.ItemAtFast(index);
}


int32
EventQueue::_execute_events_(void* cookie)
{
	EventQueue *gc = (EventQueue*)cookie;
	return gc->_ExecuteEvents();
}


int32
EventQueue::_ExecuteEvents()
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
		status_t err = acquire_sem_etc(fThreadControl, 1, B_ABSOLUTE_TIMEOUT,
									   waitUntil);
		switch (err) {
			case B_TIMED_OUT:
				// execute events, that are supposed to go off
				if (Lock()) {
					while (!fEvents.IsEmpty()
						   && system_time() >= _EventAt(0)->Time()) {
						Event* event = (Event*)fEvents.RemoveItem(0L);
						bool deleteEvent = event->AutoDelete();
						event->Execute();
						if (deleteEvent)
							delete event;
					}
					Unlock();
				}
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


// PRE: The object must be locked.
void
EventQueue::_Reschedule()
{
	if (fStatus == B_OK) {
		if (!fEvents.IsEmpty() && _EventAt(0)->Time() < fNextEventTime)
			release_sem(fThreadControl);
	}
}


// static variables

EventQueue*	EventQueue::fDefaultQueue = NULL;

