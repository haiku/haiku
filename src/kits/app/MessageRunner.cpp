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
//	File Name:		MessageRunner.cpp
//	Author:			Ingo Weinhold (bonefish@users.sf.net)
//	Description:	A BMessageRunner periodically sends a message to a
//                  specified target.
//------------------------------------------------------------------------------
#include <MessageRunner.h>

enum {
	NOT_IMPLEMENTED	= B_ERROR
};

// constructor
/*!	\brief Creates and initializes a new BMessageRunner.

	The target for replies to the delivered message(s) is \c be_app_messenger.

	The success of the initialization can (and should) be asked for via
	InitCheck().

	\note As soon as the last message has been sent, the message runner
		  becomes unusable. InitCheck() will still return \c B_OK, but
		  SetInterval(), SetCount() and GetInfo() will fail.

	\param target Target of the message(s).
	\param message The message to be sent to the target.
	\param interval Period of time before the first message is sent and
		   between messages (if more than one shall be sent) in microseconds.
	\param count Specifies how many times the message shall be sent.
		   A value less than \c 0 for an unlimited number of repetitions.
*/
BMessageRunner::BMessageRunner(BMessenger target, const BMessage *message,
							   bigtime_t interval, int32 count)
	: fToken(-1)
{
	// not implemented
}

// constructor
/*!	\brief Creates and initializes a new BMessageRunner.

	This constructor version additionally allows to specify the target for
	replies to the delivered message(s).

	The success of the initialization can (and should) be asked for via
	InitCheck().

	\note As soon as the last message has been sent, the message runner
		  becomes unusable. InitCheck() will still return \c B_OK, but
		  SetInterval(), SetCount() and GetInfo() will fail.

	\param target Target of the message(s).
	\param message The message to be sent to the target.
	\param interval Period of time before the first message is sent and
		   between messages (if more than one shall be sent) in microseconds.
	\param count Specifies how many times the message shall be sent.
		   A value less than \c 0 for an unlimited number of repetitions.
	\param replyTo Target replies to the delivered message(s) shall be sent to.
*/
BMessageRunner::BMessageRunner(BMessenger target, const BMessage *message,
							   bigtime_t interval, int32 count,
							   BMessenger replyTo)
	: fToken(-1)
{
	// not implemented
}

// destructor
/*!	\brief Frees all resources associated with the object.
*/
BMessageRunner::~BMessageRunner()
{
	// not implemented
}

// InitCheck
/*!	\brief Returns the status of the initialization.

	\note As soon as the last message has been sent, the message runner
		  becomes unusable. InitCheck() will still return \c B_OK, but
		  SetInterval(), SetCount() and GetInfo() will fail.

	\return \c B_OK, if the object is properly initialized, an error code
			otherwise.
*/
status_t
BMessageRunner::InitCheck() const
{
	return NOT_IMPLEMENTED;	// not implemented
}

// SetInterval
/*!	\brief Sets the interval of time between messages.
	\param interval The new interval in microseconds.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The message runner is not properly initialized.
	- \c B_BAD_VALUE: \a interval is \c 0 or negative, or the message runner
	  has already sent all messages to be sent and has become unusable.
*/
status_t
BMessageRunner::SetInterval(bigtime_t interval)
{
	return NOT_IMPLEMENTED;	// not implemented
}

// SetCount
/*!	\brief Sets the number of times message shall be sent.
	\param count Specifies how many times the message shall be sent.
		   A value less than \c 0 for an unlimited number of repetitions.
	- \c B_BAD_VALUE: The message runner has already sent all messages to be
	  sent and has become unusable.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The message runner is not properly initialized.
*/
status_t
BMessageRunner::SetCount(int32 count)
{
	return NOT_IMPLEMENTED;	// not implemented
}

// GetInfo
/*!	\brief Returns the time interval between two messages and the number of
		   times the message has still to be sent.
	\param interval Pointer to a pre-allocated bigtime_t variable to be set
		   to the time interval.
	\param count Pointer to a pre-allocated int32 variable to be set
		   to the number of times the message has still to be sent.
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: The message runner is not longer valid. All the
	  messages that had to be sent have already been sent.
*/
status_t
BMessageRunner::GetInfo(bigtime_t *interval, int32 *count) const
{
	return NOT_IMPLEMENTED;	// not implemented
}

// FBC
void BMessageRunner::_ReservedMessageRunner1() {}
void BMessageRunner::_ReservedMessageRunner2() {}
void BMessageRunner::_ReservedMessageRunner3() {}
void BMessageRunner::_ReservedMessageRunner4() {}
void BMessageRunner::_ReservedMessageRunner5() {}
void BMessageRunner::_ReservedMessageRunner6() {}

// copy constructor
/*!	\brief Privatized copy constructor to prevent usage.
*/
BMessageRunner::BMessageRunner(const BMessageRunner &)
	: fToken(-1)
{
}

// =
/*!	\brief Privatized assignment operator to prevent usage.
*/
BMessageRunner &
BMessageRunner::operator=(const BMessageRunner &)
{
	return *this;
}

// InitData
void
BMessageRunner::InitData(BMessenger target, const BMessage *message,
						 bigtime_t interval, int32 count, BMessenger replyTo)
{
	// not implemented
}

// SetParams
status_t
BMessageRunner::SetParams(bool resetInterval, bigtime_t interval,
						  bool resetCount, int32 count)
{
	return NOT_IMPLEMENTED;	// not implemented
}

