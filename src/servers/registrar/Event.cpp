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
//	File Name:		Event.cpp
//	Author:			Ingo Weinhold (bonefish@users.sf.net)
//					YellowBites (http://www.yellowbites.com)
//	Description:	Base class for events as handled by EventQueue.
//------------------------------------------------------------------------------

#include "Event.h"

/*!	\class Event
	\brief Base class for events as handled by EventQueue.

	Event features methods to set and get the event time and the "auto delete"
	flag, and a Do() method invoked, when the event is executed.

	If the "auto delete" flag is set to \c true, the event is deleted by the
	event queue after it has been executed. The same happens, if Do() returns
	\c true.
*/

/*!	\var bigtime_t Event::fTime
	\brief The event time.
*/

/*!	\var bool Event::fAutoDelete
	\brief The "auto delete" flag.
*/

// constructor
/*!	\brief Creates a new event.

	The event time is initialized to 0. That is, it should be set before
	pushing the event into an event queue.

	\param autoDelete Specifies whether the object shall automatically be
		   deleted by the event queue after being executed.
*/
Event::Event(bool autoDelete)
	: fTime(0),
	  fAutoDelete(autoDelete)
{
}

// constructor
/*!	\brief Creates a new event.
	\param time Time when the event shall be executed.
	\param autoDelete Specifies whether the object shall automatically be
		   deleted by the event queue after being executed.
*/
Event::Event(bigtime_t time, bool autoDelete)
	: fTime(time),
	  fAutoDelete(autoDelete)
{
}

// destructor
/*!	\brief Frees all resources associated with the object.

	Does nothing.
*/
Event::~Event()
{
}

// SetTime
/*!	\brief Sets a new event time.

	\note You must not call this method, when the event is in an event queue.
		  Use EventQueue::ModifyEvent() instead.

	\param time The new event time.
*/
void
Event::SetTime(bigtime_t time)
{
	fTime = time;
}

// Time
/*!	\brief Returns the time of the event.
	\return Returns the time of the event.
*/
bigtime_t
Event::Time() const
{
	return fTime;
}

// SetAutoDelete
/*!	\brief Sets whether the event shall be deleted after execution.
	\param autoDelete Specifies whether the object shall automatically be
		   deleted by the event queue after being executed.
*/
void
Event::SetAutoDelete(bool autoDelete)
{
	fAutoDelete = autoDelete;
}

// IsAutoDelete
/*!	\brief Returns whether the event shall be deleted after execution.
	\return Returns whether the object shall automatically be
			deleted by the event queue after being executed.
*/
bool
Event::IsAutoDelete() const
{
	return fAutoDelete;
}

// Do
/*!	\brief Hook method invoked when the event time has arrived.

	To be overridden by derived classes. As the method is executed in the
	event queue's timer thread, the execution of the method should take
	as little time as possible to keep the event queue precise.

	The return value of this method indicates whether the event queue shall
	delete the object. This does not override the IsAutoDelete() value. If
	IsAutoDelete() is \c true, then the object is deleted regardless of this
	method's return value, but if IsAutoDelete() is \c false, this method's
	return value is taken into consideration. To be precise the logical OR
	of IsAutoDelete() and the return value of Do() specifies whether the
	object shall be deleted. The reason for this handling is that there are
	usally two kind of events: "one-shot" events and those that are reused
	periodically or from time to time. The first kind can simply be
	constructed with "auto delete" set to \c true and doesn't need to care
	about Do()'s return value. The second kind shall usually not be deleted,
	but during the execution of Do() it might turn out, that it the would be
	a good idea to let it be deleted by the event queue.
	BTW, the event may as well delete itself in Do(); that is not a very
	nice practice though.

	\note IsAutoDelete() is checked by the event queue before Do() is invoked.
		  Thus changing it in Do() won't have any effect.

	If it is not deleted, the event can re-push itself into the queue
	within Do().

	\note The event queue is not locked when this method is invoked and it
		  doesn't contain the event anymore.

	\param queue The event queue executing the event.
	\return \c true, if the event shall be deleted by the event queue,
			\c false, if IsAutoDelete() shall be checked for this decision.
*/
bool
Event::Do(EventQueue *queue)
{
	return fAutoDelete;
}

