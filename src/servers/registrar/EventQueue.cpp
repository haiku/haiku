//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		EventQueue.cpp
//	Author:			Ingo Weinhold (bonefish@users.sf.net)
//					YellowBites (http://www.yellowbites.com)
//	Description:	A class featuring providing a mechanism to do events at
//					specified times.
//------------------------------------------------------------------------------

#include <stdio.h>

#include <String.h>

#include "Event.h"

#include "EventQueue.h"

static const char *kDefaultEventQueueName = "event looper";

// constructor
EventQueue::EventQueue(const char *name)
	: fEvents(100),
	  fEventLooper(-1),
	  fLooperControl(-1),
	  fNextEventTime(0),
	  fStatus(B_ERROR)
	  
{
	if (!name)
		name = kDefaultEventQueueName;
	fLooperControl = create_sem(0, (BString(name) += " control").String());
	if (fLooperControl >= B_OK)
		fStatus = B_OK;
	else
		fStatus = fLooperControl;
	if (fStatus == B_OK) {
		fEventLooper = spawn_thread(_EventLooperEntry, name, B_NORMAL_PRIORITY,
									this);
		if (fEventLooper >= B_OK) {
			fStatus = B_OK;
			resume_thread(fEventLooper);
		} else
			fStatus = fEventLooper;
	}
}

// destructor
EventQueue::~EventQueue()
{
	Die();
	while (Event *event = (Event*)fEvents.RemoveItem(0L)) {
		if (event->IsAutoDelete())
			delete event;
	}
}

// InitCheck
status_t
EventQueue::InitCheck()
{
	return fStatus;
}

// Die
void
EventQueue::Die()
{
	if (delete_sem(fLooperControl) == B_OK) {
		int32 dummy;
		wait_for_thread(fEventLooper, &dummy);
	}
}

// AddEvent
bool
EventQueue::AddEvent(Event *event)
{
	Lock();
	bool result = _AddEvent(event);
	if (result)
		_Reschedule();
	Unlock();
	return result;
}

// RemoveEvent
bool
EventQueue::RemoveEvent(Event *event)
{
	bool result = false;
	Lock();
	if ((result = fEvents.RemoveItem(event)))
		_Reschedule();
	Unlock();
	return result;
}

// ModifyEvent
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

// _AddEvent
//
// PRE: The object must be locked.
bool
EventQueue::_AddEvent(Event *event)
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
	return fEvents.AddItem(event, lower);
}

// _EventAt
Event*
EventQueue::_EventAt(int32 index) const
{
	return (Event*)fEvents.ItemAt(index);
}

// _EventLooperEntry
int32
EventQueue::_EventLooperEntry(void *data)
{
	return ((EventQueue*)data)->_EventLooper();
}

// _EventLooper
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
				while (Lock() && !fEvents.IsEmpty()
					   && system_time() >= _EventAt(0)->Time()) {
					Event *event = (Event*)fEvents.RemoveItem(0L);
					Unlock();
					bool autoDeleteEvent = event->IsAutoDelete();
					bool deleteEvent = event->Do() || autoDeleteEvent;
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

// _Reschedule
//
// PRE: The object must be locked.
void
EventQueue::_Reschedule()
{
	if (fStatus == B_OK) {
		if (!fEvents.IsEmpty() && _EventAt(0)->Time() < fNextEventTime)
			release_sem(fLooperControl);
	}
}

