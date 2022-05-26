//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, Haiku
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
//	File Name:		EventMaskWatcher.cpp
//	Author:			Ingo Weinhold (bonefish@users.sf.net)
//	Description:	EventMaskWatcher is a Watcher extended by an event mask.
//					EventMaskWatcherFilter filters EventMaskWatchers with
//					respect to their event mask and a specific event.
//------------------------------------------------------------------------------

#include "EventMaskWatcher.h"

// EventMaskWatcher

/*!	\class EventMaskWatcher
	\brief EventMaskWatcher is a Watcher extended by an event mask.

	Each set bit in the watcher's event mask specifies an event the watcher
	wants to get notified about.

	This class is intended to be used together with EventMaskWatcherFilter.
	Such a filter object is created with a certain event and its
	EventMaskWatcherFilter::Filter() method only selects those
	EventMaskWatchers with an event mask that contains the event.
*/

/*!	\var uint32 EventMaskWatcher::fEventMask
	\brief The watcher's event mask.
*/

// constructor
/*!	\brief Creates a new EventMaskWatcher with a given target and event mask.
	\param target The watcher's message target.
	\param eventMask the watcher's event mask.
*/
EventMaskWatcher::EventMaskWatcher(const BMessenger &target, uint32 eventMask)
	: Watcher(target),
	  fEventMask(eventMask)
{
}

// EventMask
/*!	\brief Returns the watcher's event mask.
	\return The watcher's event mask.
*/
uint32
EventMaskWatcher::EventMask() const
{
	return fEventMask;
}


// EventMaskWatcherFilter

/*!	\class EventMaskWatcherFilter
	\brief EventMaskWatcherFilter filters EventMaskWatchers with respect to
		   their event mask and a specific event.

	This class is intended to be used together with EventMaskWatcher.
	A filter object is created with a certain event and its Filter() method
	only selects those EventMaskWatchers with an event mask that contains
	the event.
*/

/*!	\var uint32 EventMaskWatcherFilter::fEvent
	\brief The filter's event.

	Filter only returns \c true for watchers whose event mask contains the
	event.
*/

// constructor
/*!	\brief Creates a new EventMaskWatcherFilter with a specified event.

	Actually \a event may specify more than one event. Usually each set bit
	represents an event.

	\param event The event.
*/
EventMaskWatcherFilter::EventMaskWatcherFilter(uint32 event)
	: WatcherFilter(),
	  fEvent(event)
{
}

// Filter
/*!	\brief Returns whether the watcher-message pair satisfies the predicate
		   represented by this object.

	Returns \c true, if the supplied watcher is an EventMaskWatcher and its
	event mask contains this filter's event, or more precise the bitwise AND
	on event mask and event is not 0.

	\param watcher The watcher in question.
	\param message The message in question.
	\return \c true, if the watcher's event mask contains the filter's event.
*/
bool
EventMaskWatcherFilter::Filter(Watcher *watcher, BMessage *message)
{
	EventMaskWatcher *emWatcher = dynamic_cast<EventMaskWatcher *>(watcher);
	return (emWatcher && (emWatcher->EventMask() & fEvent));
}

