/*
 * Copyright 2001-2010, Haiku, Inc.
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


/*!	\brief Creates and initializes a new BMessageRunner.

	The target for replies to the delivered message(s) is \c be_app_messenger.

	The success of the initialization can (and should) be asked for via
	InitCheck(). This object will not take ownership of the \a message, you
	may freely change or delete it after creation.

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
BMessageRunner::BMessageRunner(BMessenger target, const BMessage* message,
	bigtime_t interval, int32 count)
	:
	fToken(-1)
{
	_InitData(target, message, interval, count, be_app_messenger);
}


/*!	\brief Creates and initializes a new BMessageRunner.

	The target for replies to the delivered message(s) is \c be_app_messenger.

	The success of the initialization can (and should) be asked for via
	InitCheck(). This object will not take ownership of the \a message, you
	may freely change or delete it after creation.

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
BMessageRunner::BMessageRunner(BMessenger target, const BMessage& message,
	bigtime_t interval, int32 count)
	:
	fToken(-1)
{
	_InitData(target, &message, interval, count, be_app_messenger);
}


/*!	\brief Creates and initializes a new BMessageRunner.

	This constructor version additionally allows to specify the target for
	replies to the delivered message(s).

	The success of the initialization can (and should) be asked for via
	InitCheck(). This object will not take ownership of the \a message, you
	may freely change or delete it after creation.

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
BMessageRunner::BMessageRunner(BMessenger target, const BMessage* message,
	bigtime_t interval, int32 count, BMessenger replyTo)
	:
	fToken(-1)
{
	_InitData(target, message, interval, count, replyTo);
}


/*!	\brief Creates and initializes a new BMessageRunner.

	This constructor version additionally allows to specify the target for
	replies to the delivered message(s).

	The success of the initialization can (and should) be asked for via
	InitCheck(). This object will not take ownership of the \a message, you
	may freely change or delete it after creation.

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
BMessageRunner::BMessageRunner(BMessenger target, const BMessage& message,
	bigtime_t interval, int32 count, BMessenger replyTo)
	:
	fToken(-1)
{
	_InitData(target, &message, interval, count, replyTo);
}


/*!	\brief Frees all resources associated with the object.
*/
BMessageRunner::~BMessageRunner()
{
	if (fToken < B_OK)
		return;

	// compose the request message
	BMessage request(B_REG_UNREGISTER_MESSAGE_RUNNER);
	status_t error = request.AddInt32("token", fToken);

	// send the request
	BMessage reply;
	if (error == B_OK)
		error = BRoster::Private().SendTo(&request, &reply, false);

	// ignore the reply, we can't do anything anyway
}


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
	return fToken >= 0 ? B_OK : fToken;
}


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
	return _SetParams(true, interval, false, 0);
}


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
	return _SetParams(false, 0, true, count);
}


/*!	\brief Returns the time interval between two messages and the number of
		   times the message has still to be sent.

	Both parameters (\a interval and \a count) may be \c NULL.

	\param interval Pointer to a pre-allocated bigtime_t variable to be set
		   to the time interval. May be \c NULL.
	\param count Pointer to a pre-allocated int32 variable to be set
		   to the number of times the message has still to be sent.
		   May be \c NULL.
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: The message runner is not longer valid. All the
	  messages that had to be sent have already been sent.
*/
status_t
BMessageRunner::GetInfo(bigtime_t* interval, int32* count) const
{
	status_t error =  (fToken >= 0 ? B_OK : B_BAD_VALUE);

	// compose the request message
	BMessage request(B_REG_GET_MESSAGE_RUNNER_INFO);
	if (error == B_OK)
		error = request.AddInt32("token", fToken);

	// send the request
	BMessage reply;
	if (error == B_OK)
		error = BRoster::Private().SendTo(&request, &reply, false);

	// evaluate the reply
	if (error == B_OK) {
		if (reply.what == B_REG_SUCCESS) {
			// count
			int32 _count;
			if (reply.FindInt32("count", &_count) == B_OK) {
				if (count)
					*count = _count;
			} else
				error = B_ERROR;

			// interval
			bigtime_t _interval;
			if (reply.FindInt64("interval", &_interval) == B_OK) {
				if (interval)
					*interval = _interval;
			} else
				error = B_ERROR;
		} else {
			if (reply.FindInt32("error", &error) != B_OK)
				error = B_ERROR;
		}
	}
	return error;
}


/*!	\brief Creates and initializes a detached BMessageRunner.

	You cannot alter the runner after the creation, and it will be deleted
	automatically once it is done.
	The target for replies to the delivered message(s) is \c be_app_messenger.

	\param target Target of the message(s).
	\param message The message to be sent to the target.
	\param interval Period of time before the first message is sent and
		   between messages (if more than one shall be sent) in microseconds.
	\param count Specifies how many times the message shall be sent.
		   A value less than \c 0 for an unlimited number of repetitions.
*/
/*static*/ status_t
BMessageRunner::StartSending(BMessenger target, const BMessage* message,
	bigtime_t interval, int32 count)
{
	int32 token = _RegisterRunner(target, message, interval, count, true,
		be_app_messenger);
	return token >= B_OK ? B_OK : token;
}


/*!	\brief Creates and initializes a detached BMessageRunner.

	You cannot alter the runner after the creation, and it will be deleted
	automatically once it is done.

	\param target Target of the message(s).
	\param message The message to be sent to the target.
	\param interval Period of time before the first message is sent and
		   between messages (if more than one shall be sent) in microseconds.
	\param count Specifies how many times the message shall be sent.
		   A value less than \c 0 for an unlimited number of repetitions.
	\param replyTo Target replies to the delivered message(s) shall be sent to.
*/
/*static*/ status_t
BMessageRunner::StartSending(BMessenger target, const BMessage* message,
	bigtime_t interval, int32 count, BMessenger replyTo)
{
	int32 token = _RegisterRunner(target, message, interval, count, true, replyTo);
	return token >= B_OK ? B_OK : token;
}


// FBC
void BMessageRunner::_ReservedMessageRunner1() {}
void BMessageRunner::_ReservedMessageRunner2() {}
void BMessageRunner::_ReservedMessageRunner3() {}
void BMessageRunner::_ReservedMessageRunner4() {}
void BMessageRunner::_ReservedMessageRunner5() {}
void BMessageRunner::_ReservedMessageRunner6() {}


/*!	\brief Privatized copy constructor to prevent usage.
*/
BMessageRunner::BMessageRunner(const BMessageRunner &)
	: fToken(-1)
{
}


/*!	\brief Privatized assignment operator to prevent usage.
*/
BMessageRunner &
BMessageRunner::operator=(const BMessageRunner &)
{
	return* this;
}


/*!	\brief Initializes the BMessageRunner.

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


/*!	\brief Registers the BMessageRunner in the registrar.

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
	status_t error = B_OK;
	if (message == NULL || count == 0 || (count < 0 && detach))
		error = B_BAD_VALUE;

	// compose the request message
	BMessage request(B_REG_REGISTER_MESSAGE_RUNNER);
	if (error == B_OK)
		error = request.AddInt32("team", BPrivate::current_team());
	if (error == B_OK)
		error = request.AddMessenger("target", target);
	if (error == B_OK)
		error = request.AddMessage("message", message);
	if (error == B_OK)
		error = request.AddInt64("interval", interval);
	if (error == B_OK)
		error = request.AddInt32("count", count);
	if (error == B_OK)
		error = request.AddMessenger("reply_target", replyTo);

	// send the request
	BMessage reply;
	if (error == B_OK)
		error = BRoster::Private().SendTo(&request, &reply, false);

	int32 token;

	// evaluate the reply
	if (error == B_OK) {
		if (reply.what == B_REG_SUCCESS) {
			if (reply.FindInt32("token", &token) != B_OK)
				error = B_ERROR;
		} else {
			if (reply.FindInt32("error", &error) != B_OK)
				error = B_ERROR;
		}
	}

	if (error == B_OK)
		return token;

	return error;
}


/*!	\brief Sets the message runner's interval and count parameters.

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
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: The message runner is not longer valid. All the
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
	status_t error = request.AddInt32("token", fToken);
	if (error == B_OK && resetInterval)
		error = request.AddInt64("interval", interval);
	if (error == B_OK && resetCount)
		error = request.AddInt32("count", count);

	// send the request
	BMessage reply;
	if (error == B_OK)
		error = BRoster::Private().SendTo(&request, &reply, false);

	// evaluate the reply
	if (error == B_OK) {
		if (reply.what != B_REG_SUCCESS) {
			if (reply.FindInt32("error", &error) != B_OK)
				error = B_ERROR;
		}
	}
	return error;
}
