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
//	File Name:		MessageEvent.cpp
//	Author:			Ingo Weinhold (bonefish@users.sf.net)
//					YellowBites (http://www.yellowbites.com)
//	Description:	A special event that sends a message when executed.
//------------------------------------------------------------------------------

#include <Handler.h>
#include <Looper.h>

#include "MessageEvent.h"

/*!	\class MessageEvent
	\brief A special event that sends a message when executed.

	The constructors set the "auto delete" flag to \c true by default. It can
	be changed by SetAutoDelete(), but note, that the object's creator is
	responsible for its destruction then.
*/

/*!	\var BMessage MessageEvent::fMessage
	\brief Message to be sent.
*/

/*!	\var BMessenger MessageEvent::fMessenger
	\brief Message target.

	Valid only, if \a fHandler is \c NULL.
*/

/*!	\var BHandler *MessageEvent::fHandler
	\brief Message target.

	May be \c NULL, then \a fMessenger specifies the message target.
*/


// constructor
/*!	\brief Creates a new MessageEvent with a target BHandler and a message
		   command.

	\note The supplied BHandler must be valid the whole life time of the
		  MessageEvent.

	\param time The time at which the message shall be sent.
	\param handler The BHandler to which the message shall be delivered.
	\param command The "what" field of the message to be sent.
*/
MessageEvent::MessageEvent(bigtime_t time, BHandler* handler, uint32 command)
	: Event(time, true),
	  fMessage(command),
	  fMessenger(),
	  fHandler(handler)
{
}

// constructor
/*!	\brief Creates a new MessageEvent with a target BHandler and a message.

	The caller retains ownership of the supplied message. It can savely be
	deleted after the constructor returns.

	\note The supplied BHandler must be valid the whole life time of the
		  MessageEvent.

	\param time The time at which the message shall be sent.
	\param handler The BHandler to which the message shall be delivered.
	\param message The message to be sent.
*/
MessageEvent::MessageEvent(bigtime_t time, BHandler* handler,
						   const BMessage *message)
	: Event(time, true),
	  fMessage(*message),
	  fMessenger(),
	  fHandler(handler)
{
}

// constructor
/*!	\brief Creates a new MessageEvent with a target BMessenger and a message
		   command.
	\param time The time at which the message shall be sent.
	\param messenger The BMessenger specifying the target to which the message
		   shall be delivered.
	\param command The "what" field of the message to be sent.
*/
MessageEvent::MessageEvent(bigtime_t time, const BMessenger& messenger,
						   uint32 command)
	: Event(time, true),
	  fMessage(command),
	  fMessenger(messenger),
	  fHandler(NULL)
{
}

// constructor
/*!	\brief Creates a new MessageEvent with a target BMessenger and a message.

	The caller retains ownership of the supplied message. It can savely be
	deleted after the constructor returns.

	\param time The time at which the message shall be sent.
	\param messenger The BMessenger specifying the target to which the message
		   shall be delivered.
	\param message The message to be sent.
*/
MessageEvent::MessageEvent(bigtime_t time, const BMessenger& messenger,
						   const BMessage *message)
	: Event(time, true),
	  fMessage(*message),
	  fMessenger(messenger),
	  fHandler(NULL)
{
}

// destructor
/*!	\brief Frees all resources associated with the object.
*/
MessageEvent::~MessageEvent()
{
}

// Do
/*!	\brief Hook method invoked when the event is executed.

	Implements Event. Delivers the message to the target.
	
	\param queue The event queue executing the event.
	\return \c true, if the object shall be deleted, \c false otherwise.
*/
bool
MessageEvent::Do(EventQueue *queue)
{
	if (fHandler) {
		if (BLooper* looper = fHandler->Looper())
			looper->PostMessage(&fMessage, fHandler);
	} else
		fMessenger.SendMessage(&fMessage);
	return false;
}

