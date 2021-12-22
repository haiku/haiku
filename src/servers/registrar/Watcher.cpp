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
//	File Name:		Watcher.cpp
//	Author:			Ingo Weinhold (bonefish@users.sf.net)
//	Description:	A Watcher represents a target of a watching service.
//					A WatcherFilter represents a predicate on Watchers.
//------------------------------------------------------------------------------

#include <Message.h>

#include "MessageDeliverer.h"
#include "Watcher.h"

// Watcher

/*!	\class Watcher
	\brief A Watcher represents a target of a watching service.

	The Watcher base class only has one attribute, a BMessenger which
	specifies the target to which notification messages shall be sent.
	SendMessage() actually sends the message to the target. It can be
	overridden in case of special needs.
*/

/*!	\var Watcher::fTarget
	\brief The watcher's message target.
*/

// constructor
/*!	\brief Creates a new watcher with a specified target.

	The supplied BMessenger is copied, that is the caller can delete the
	object when the constructor returns.

	\param target The watcher's message target.
*/
Watcher::Watcher(const BMessenger &target)
	: fTarget(target)
{
}

// destructor
/*!	\brief Frees all resources associated with the object.
*/
Watcher::~Watcher()
{
}

// Target
/*!	\brief Returns the watcher's message target.
	\return The watcher's message target.
*/
const BMessenger&
Watcher::Target() const
{
	return fTarget;
}

// SendMessage
/*!	\brief Sends the supplied message to the watcher's message target.

	The method can be overridden by a derived class to e.g. add additional
	fields to the message. Note, that in this case the message must not be
	modified directly, but a copy has to be made.

	\param message The message to be sent.
	\return \c B_OK, if everything went fine, another error code, if an error
			occured.
*/
status_t
Watcher::SendMessage(BMessage *message)
{
	return MessageDeliverer::Default()->DeliverMessage(message, fTarget);
}


// WatcherFilter

/*!	\class WatcherFilter
	\brief A WatcherFilter represents a predicate on Watchers.

	It's only method Filter() returns whether a given Watcher and a BMessage
	satisfy the predicate. This class' Filter() implementation always returns
	\c true. Derived classes override it.
*/

// constructor
/*!	\brief Creates a new WatchingFilter.
*/
WatcherFilter::WatcherFilter()
{
}

// destructor
/*!	\brief Frees all resources associated with the object.
*/
WatcherFilter::~WatcherFilter()
{
}

// Filter
/*!	\brief Returns whether the watcher-message pair satisfies the predicate
		   represented by this object.

	Derived classes override this method. This version always returns \c true.

	\param watcher The watcher in question.
	\param message The message in question.
	\return \c true.
*/
bool
WatcherFilter::Filter(Watcher *watcher, BMessage *message)
{
	return true;
}

