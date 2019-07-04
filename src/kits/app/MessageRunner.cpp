/*
 * Copyright 2001-2010 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 */


#include <MessageRunner.h>

#include <Application.h>
#include <AppMisc.h>
#include <RegistrarDefs.h>
#include <Roster.h>
#include <RosterPrivate.h>


using namespace BPrivate;


BMessageRunner::BMessageRunner(BMessenger target, const BMessage* message,
	bigtime_t interval, int32 count)
	:
	fToken(-1)
{
	_InitData(target, message, interval, count, be_app_messenger);
}


BMessageRunner::BMessageRunner(BMessenger target, const BMessage& message,
	bigtime_t interval, int32 count)
	:
	fToken(-1)
{
	_InitData(target, &message, interval, count, be_app_messenger);
}


BMessageRunner::BMessageRunner(BMessenger target, const BMessage* message,
	bigtime_t interval, int32 count, BMessenger replyTo)
	:
	fToken(-1)
{
	_InitData(target, message, interval, count, replyTo);
}


BMessageRunner::BMessageRunner(BMessenger target, const BMessage& message,
	bigtime_t interval, int32 count, BMessenger replyTo)
	:
	fToken(-1)
{
	_InitData(target, &message, interval, count, replyTo);
}


BMessageRunner::~BMessageRunner()
{
	if (fToken < B_OK)
		return;

	// compose the request message
	BMessage request(B_REG_UNREGISTER_MESSAGE_RUNNER);
	status_t result = request.AddInt32("token", fToken);

	// send the request
	BMessage reply;
	if (result == B_OK)
		result = BRoster::Private().SendTo(&request, &reply, false);

	// ignore the reply, we can't do anything anyway
}


status_t
BMessageRunner::InitCheck() const
{
	return fToken >= 0 ? B_OK : fToken;
}


status_t
BMessageRunner::SetInterval(bigtime_t interval)
{
	return _SetParams(true, interval, false, 0);
}


status_t
BMessageRunner::SetCount(int32 count)
{
	return _SetParams(false, 0, true, count);
}


status_t
BMessageRunner::GetInfo(bigtime_t* interval, int32* count) const
{
	status_t result =  fToken >= 0 ? B_OK : B_BAD_VALUE;

	// compose the request message
	BMessage request(B_REG_GET_MESSAGE_RUNNER_INFO);
	if (result == B_OK)
		result = request.AddInt32("token", fToken);

	// send the request
	BMessage reply;
	if (result == B_OK)
		result = BRoster::Private().SendTo(&request, &reply, false);

	// evaluate the reply
	if (result == B_OK) {
		if (reply.what == B_REG_SUCCESS) {
			// count
			int32 _count;
			if (reply.FindInt32("count", &_count) == B_OK) {
				if (count != 0)
					*count = _count;
			} else
				result = B_ERROR;

			// interval
			bigtime_t _interval;
			if (reply.FindInt64("interval", &_interval) == B_OK) {
				if (interval != 0)
					*interval = _interval;
			} else
				result = B_ERROR;
		} else {
			if (reply.FindInt32("error", &result) != B_OK)
				result = B_ERROR;
		}
	}

	return result;
}


/*static*/ status_t
BMessageRunner::StartSending(BMessenger target, const BMessage* message,
	bigtime_t interval, int32 count)
{
	int32 token = _RegisterRunner(target, message, interval, count, true,
		be_app_messenger);

	return token >= B_OK ? B_OK : token;
}


/*static*/ status_t
BMessageRunner::StartSending(BMessenger target, const BMessage* message,
	bigtime_t interval, int32 count, BMessenger replyTo)
{
	int32 token = _RegisterRunner(target, message, interval, count, true,
		replyTo);

	return token >= B_OK ? B_OK : token;
}


// FBC
void BMessageRunner::_ReservedMessageRunner1() {}
void BMessageRunner::_ReservedMessageRunner2() {}
void BMessageRunner::_ReservedMessageRunner3() {}
void BMessageRunner::_ReservedMessageRunner4() {}
void BMessageRunner::_ReservedMessageRunner5() {}
void BMessageRunner::_ReservedMessageRunner6() {}


#ifdef _BEOS_R5_COMPATIBLE_
//! Privatized copy constructor to prevent usage.
BMessageRunner::BMessageRunner(const BMessageRunner &)
	:
	fToken(-1)
{
}


//! Privatized assignment operator to prevent usage.
BMessageRunner&
BMessageRunner::operator=(const BMessageRunner&)
{
	return* this;
}
#endif


/*!	Initializes the BMessageRunner.

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
void
BMessageRunner::_InitData(BMessenger target, const BMessage* message,
	bigtime_t interval, int32 count, BMessenger replyTo)
{
	fToken = _RegisterRunner(target, message, interval, count, false, replyTo);
}


/*!	Registers the BMessageRunner in the registrar.

	\param target Target of the message(s).
	\param message The message to be sent to the target.
	\param interval Period of time before the first message is sent and
	       between messages (if more than one shall be sent) in microseconds.
	\param count Specifies how many times the message shall be sent.
	       A value less than \c 0 for an unlimited number of repetitions.
	\param replyTo Target replies to the delivered message(s) shall be sent to.

	\return The token the message runner is registered with, or the error code
	        while trying to register it.
*/
/*static*/ int32
BMessageRunner::_RegisterRunner(BMessenger target, const BMessage* message,
	bigtime_t interval, int32 count, bool detach, BMessenger replyTo)
{
	status_t result = B_OK;
	if (message == NULL || count == 0 || (count < 0 && detach))
		result = B_BAD_VALUE;

	// compose the request message
	BMessage request(B_REG_REGISTER_MESSAGE_RUNNER);
	if (result == B_OK)
		result = request.AddInt32("team", BPrivate::current_team());

	if (result == B_OK)
		result = request.AddMessenger("target", target);

	if (result == B_OK)
		result = request.AddMessage("message", message);

	if (result == B_OK)
		result = request.AddInt64("interval", interval);

	if (result == B_OK)
		result = request.AddInt32("count", count);

	if (result == B_OK)
		result = request.AddMessenger("reply_target", replyTo);

	// send the request
	BMessage reply;
	if (result == B_OK)
		result = BRoster::Private().SendTo(&request, &reply, false);

	int32 token;

	// evaluate the reply
	if (result == B_OK) {
		if (reply.what == B_REG_SUCCESS) {
			if (reply.FindInt32("token", &token) != B_OK)
				result = B_ERROR;
		} else {
			if (reply.FindInt32("error", &result) != B_OK)
				result = B_ERROR;
		}
	}

	if (result == B_OK)
		return token;

	return result;
}


/*!	Sets the message runner's interval and count parameters.

	The parameters \a resetInterval and \a resetCount specify whether
	the interval or the count parameter respectively shall be reset.

	At least one parameter must be set, otherwise the methods returns
	\c B_BAD_VALUE.

	\param resetInterval \c true, if the interval shall be reset, \c false
	       otherwise -- then \a interval is ignored.
	\param interval The new interval in microseconds.
	\param resetCount \c true, if the count shall be reset, \c false
	       otherwise -- then \a count is ignored.
	\param count Specifies how many times the message shall be sent.
	       A value less than \c 0 for an unlimited number of repetitions.

	\return A status code.
	\retval B_OK Everything went fine.
	\retval B_BAD_VALUE The message runner is not longer valid. All the
	        messages that had to be sent have already been sent. Or both
	        \a resetInterval and \a resetCount are \c false.
*/
status_t
BMessageRunner::_SetParams(bool resetInterval, bigtime_t interval,
	bool resetCount, int32 count)
{
	if ((!resetInterval && !resetCount) || fToken < 0)
		return B_BAD_VALUE;

	// compose the request message
	BMessage request(B_REG_SET_MESSAGE_RUNNER_PARAMS);
	status_t result = request.AddInt32("token", fToken);
	if (result == B_OK && resetInterval)
		result = request.AddInt64("interval", interval);

	if (result == B_OK && resetCount)
		result = request.AddInt32("count", count);

	// send the request
	BMessage reply;
	if (result == B_OK)
		result = BRoster::Private().SendTo(&request, &reply, false);

	// evaluate the reply
	if (result == B_OK) {
		if (reply.what != B_REG_SUCCESS) {
			if (reply.FindInt32("error", &result) != B_OK)
				result = B_ERROR;
		}
	}

	return result;
}
