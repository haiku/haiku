/*
 * Copyright 2001-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold (bonefish@users.sf.net)
 */


#include <algorithm>
#include <new>

#include <Autolock.h>
#include <Message.h>
#include <MessagePrivate.h>
#include <Messenger.h>
#include <OS.h>
#include <RegistrarDefs.h>

#include "Debug.h"
#include "Event.h"
#include "EventQueue.h"
#include "MessageDeliverer.h"
#include "MessageRunnerManager.h"

using std::max;
using std::nothrow;

/*!	\class MessageRunnerManager
	\brief Manages the registrar side "shadows" of BMessageRunners.

	The class features four methods to which the registrar application
	dispatches the message runner specific request messages.

	Each active message runner (i.e. one that still has messages to be sent)
	is represented by a RunnerInfo that comprises all necessary information,
	among these a RunnerEvent added to the event queue. When the event is
	executed, it calls the _DoEvent() method, which in turn sends the message
	runner message to the respective target and schedules the event for the
	next time the message has to be sent (_ScheduleEvent()).

	A couple of helper methods provide convenient access to the RunnerInfo
	list (\a fRunnerInfos). A BLocker (\a fLock) and respective locking
	methods are used to serialize the access to the member variables.
*/

/*! \var BList MessageRunnerManager::fRunnerInfos
	\brief The list of RunnerInfos.
*/

/*! \var BLocker MessageRunnerManager::fLock
	\brief A locker used to serialize the access to the object's variable
		   members.
*/

/*! \var EventQueue *MessageRunnerManager::fEventQueue
	\brief Event queue used by the manager.
*/

/*! \var int32 MessageRunnerManager::fNextToken
	\brief Next unused token for message runners.
*/


using namespace BPrivate;

//! The minimal time interval for message runners (50 ms).
static const bigtime_t kMininalTimeInterval = 50000LL;


static bigtime_t
add_time(bigtime_t a, bigtime_t b)
{
	// avoid a bigtime_t overflow
	if (LONGLONG_MAX - b < a)
		return LONGLONG_MAX;
	else
		return a + b;
}


// RunnerEvent
/*!	\brief Event class used to by the message runner manager.

	For each active message runner such an event is used. It invokes
	MessageRunnerManager::_DoEvent() on execution.
*/
class MessageRunnerManager::RunnerEvent : public Event {
public:
	/*!	\brief Creates a new RunnerEvent.
		\param manager The message runner manager.
		\param info The RunnerInfo for the message runner.
	*/
	RunnerEvent(MessageRunnerManager *manager, RunnerInfo *info)
		: Event(false),
		  fManager(manager),
		  fInfo(info)
	{
	}

	/*!	\brief Hook method invoked when the event is executed.

		Implements Event. Calls MessageRunnerManager::_DoEvent().
		
		\param queue The event queue executing the event.
		\return \c true, if the object shall be deleted, \c false otherwise.
	*/
	virtual bool Do(EventQueue *queue)
	{
		return fManager->_DoEvent(fInfo);
	}

private:
	MessageRunnerManager	*fManager;	//!< The message runner manager.
	RunnerInfo				*fInfo;		//!< The message runner info.
};


// RunnerInfo
/*!	\brief Contains all needed information about an active message runner.
*/
struct MessageRunnerManager::RunnerInfo {
	/*!	\brief Creates a new RunnerInfo.
		\param team The team owning the message runner.
		\param token The unique token associated with the message runner.
		\param target The target the message shall be sent to.
		\param message The message to be sent to the target.
		\param interval The message runner's time interval.
		\param count The number of times the message shall be sent.
		\param replyTarget The reply target for the delivered message.
	*/
	RunnerInfo(team_id team, int32 token, BMessenger target, BMessage *message,
			   bigtime_t interval, int32 count, BMessenger replyTarget)
		: team(team),
		  token(token),
		  target(target),
		  message(message),
		  interval(interval),
		  count(count),
		  replyTarget(replyTarget),
		  time(0),
		  event(NULL),
		  rescheduled(false)
	{
	}

	/*!	\brief Frees all resources associated with the object.

		The message and the event are delete.
	*/
	~RunnerInfo()
	{
		delete message;
		delete event;
	}

	/*!	\brief Delivers the message to the respective target.
		\return \c B_OK, if the message has successfully been delivered or
				the target does still exist and its message port is full,
				an error code otherwise.
	*/
	status_t DeliverMessage()
	{
		if (count > 0)
			count--;

		// set the reply target
		BMessage::Private(message).SetReply(replyTarget);

		// deliver the message: We use the MessageDeliverer to allow the
		// message to be delivered, even if the target port is temporarily
		// full. For periodic message runners, that have to deliver further
		// messages, we restrict the delivery timeout to the message interval.
		status_t error;
		if (count > 0) {
			error = MessageDeliverer::Default()->DeliverMessage(message, target,
				interval);
		} else {
			error = MessageDeliverer::Default()->DeliverMessage(message,
				target);
		}

		// B_WOULD_BLOCK is as good as B_OK. We return an error only, if
		// there are serious problems with the target, i.e. if it doesn't
		// exist anymore for instance. A full message port is harmless.
		if (error == B_WOULD_BLOCK)
			error = B_OK;
		return error;
	}

	team_id		team;			//!< The team owning the message runner.
	int32		token;			/*!< The unique token associated with the
									 message runner. */
	BMessenger	target;			//!< The target the message shall be sent to.
	BMessage	*message;		//!< The message to be sent to the target.
	bigtime_t	interval;		//!< The message runner's time interval.
	int32		count;			/*!< The number of times the message shall be
									 sent. */
	BMessenger	replyTarget;	/*!< The reply target for the delivered
									 message. */
	bigtime_t	time;			/*!< Time at which the next message will be
									 sent. */
	RunnerEvent	*event;			//!< Runner event for the message runner.
	bool		rescheduled;	/*!< Set to \c true when the event has been
									 started to be executed while it was
									 rescheduled. */
};


// constructor
/*!	\brief Creates a new MessageRunnerManager.
	\param eventQueue The EventQueue the manager shall use.
*/
MessageRunnerManager::MessageRunnerManager(EventQueue *eventQueue)
	: fRunnerInfos(),
	  fLock(),
	  fEventQueue(eventQueue),
	  fNextToken(0)
{
}

// destructor
/*!	\brief Frees all resources associated with the object.

	The manager's event queue must already have been stopped
	(EventQueue::Die()).
*/
MessageRunnerManager::~MessageRunnerManager()
{
	// The event queue should already be stopped, but must still exist.
	// If it is still running and an event gets executed after we've locked
	// ourselves, then it will access an already deleted manager.
	BAutolock _lock(fLock);
	for (int32 i = 0; RunnerInfo *info = _InfoAt(i); i++) {
		if (!fEventQueue->RemoveEvent(info->event))
			info->event = NULL;
		delete info;
	}
	fRunnerInfos.MakeEmpty();
}

// HandleRegisterRunner
/*!	\brief Handles a registration request (BMessageRunner::InitData()).
	\param request The request message.
*/
void
MessageRunnerManager::HandleRegisterRunner(BMessage *request)
{
	FUNCTION_START();

	BAutolock _lock(fLock);
	status_t error = B_OK;
	// get the parameters
	team_id team;
	BMessenger target;
	// TODO: This should be a "new (nothrow)", but R5's BMessage doesn't
	// define that version.
	BMessage *message = new BMessage;
	bigtime_t interval;
	int32 count;
	BMessenger replyTarget;
	if (error == B_OK && message == NULL)
		error = B_NO_MEMORY;
	if (error == B_OK && request->FindInt32("team", &team) != B_OK)
		error = B_BAD_VALUE;
	if (error == B_OK && request->FindMessenger("target", &target) != B_OK)
		error = B_BAD_VALUE;
	if (error == B_OK && request->FindMessage("message", message) != B_OK)
		error = B_BAD_VALUE;
	if (error == B_OK && request->FindInt64("interval", &interval) != B_OK)
		error = B_BAD_VALUE;
	if (error == B_OK && request->FindInt32("count", &count) != B_OK)
		error = B_BAD_VALUE;
	if (error == B_OK
		&& request->FindMessenger("reply_target", &replyTarget) != B_OK) {
		error = B_BAD_VALUE;
	}

	// check the parameters
	if (error == B_OK && count == 0)
		error = B_BAD_VALUE;

	// add a new runner info
	RunnerInfo *info = NULL;
	if (error == B_OK) {
		interval = max(interval, kMininalTimeInterval);
		info = new(nothrow) RunnerInfo(team, _NextToken(), target, message,
									   interval, count, replyTarget);
		if (info) {
			info->time = system_time();
			if (!_AddInfo(info))
				error = B_NO_MEMORY;
		} else
			error = B_NO_MEMORY;
	}

	// create a new event
	RunnerEvent *event = NULL;
	if (error == B_OK) {
		event = new(nothrow) RunnerEvent(this, info);
		if (event) {
			info->event = event;
			if (!_ScheduleEvent(info))
				error = B_NO_MEMORY;	// TODO: The only possible reason?
		} else
			error = B_NO_MEMORY;
	}

	// cleanup on error
	if (error != B_OK) {
		if (info) {
			_RemoveInfo(info);
			delete info;
		}
		delete message;
	}

	// reply to the request
	if (error == B_OK) {
		BMessage reply(B_REG_SUCCESS);
		reply.AddInt32("token", info->token);
		request->SendReply(&reply);
	} else {
		BMessage reply(B_REG_ERROR);
		reply.AddInt32("error", error);
		request->SendReply(&reply);
	}

	FUNCTION_END();
}

// HandleUnregisterRunner
/*!	\brief Handles an unregistration request (BMessageRunner destructor).
	\param request The request message.
*/
void
MessageRunnerManager::HandleUnregisterRunner(BMessage *request)
{
	FUNCTION_START();

	BAutolock _lock(fLock);
	status_t error = B_OK;
	// get the parameters
	int32 token;
	if (error == B_OK && request->FindInt32("token", &token) != B_OK)
		error = B_BAD_VALUE;
	// find and delete the runner info
	if (error == B_OK) {
		if (RunnerInfo *info = _InfoForToken(token))
			_DeleteInfo(info, false);
		else
			error = B_BAD_VALUE;
	}
	// reply to the request
	if (error == B_OK) {
		BMessage reply(B_REG_SUCCESS);
		request->SendReply(&reply);
	} else {
		BMessage reply(B_REG_ERROR);
		reply.AddInt32("error", error);
		request->SendReply(&reply);
	}

	FUNCTION_END();
}

// HandleSetRunnerParams
/*!	\brief Handles an parameter change request (BMessageRunner::SetParams()).
	\param request The request message.
*/
void
MessageRunnerManager::HandleSetRunnerParams(BMessage *request)
{
	FUNCTION_START();

	BAutolock _lock(fLock);
	status_t error = B_OK;
	// get the parameters
	int32 token;
	bigtime_t interval;
	int32 count;
	bool setInterval = false;
	bool setCount = false;
	if (error == B_OK && request->FindInt32("token", &token) != B_OK)
		error = B_BAD_VALUE;
	if (error == B_OK && request->FindInt64("interval", &interval) == B_OK)
		setInterval = true;
	if (error == B_OK && request->FindInt32("count", &count) == B_OK)
		setCount = true;

	// find the runner info
	RunnerInfo *info = NULL;
	if (error == B_OK) {
		info = _InfoForToken(token);
		if (!info) {
			// TODO: At this point, the runner could have been deleted already.
			//	Since setting its parameters at this point should still be
			//	valid, we'd have to recreate it.
			//	(Even though the documentation in *our* BMessageRunner
			//	implementation specifically denies the possibility of setting
			//	the runner's parameters at this point, it would still be nice
			//	to allow this.)
			error = B_BAD_VALUE;
		}
	}

	// set the new values
	if (error == B_OK) {
		bool eventRemoved = false;
		bool deleteInfo = false;
		// count
		if (setCount) {
			if (count == 0)
				deleteInfo = true;
			else
				info->count = count;
		}
		// interval
		if (setInterval) {
			eventRemoved = fEventQueue->RemoveEvent(info->event);
			if (!eventRemoved)
				info->rescheduled = true;
			interval = max(interval, kMininalTimeInterval);
			info->interval = interval;
			info->time = system_time();
			if (!_ScheduleEvent(info))
				error = B_NO_MEMORY;	// TODO: The only possible reason?
		}
		// remove and delete the info on error
		if (error != B_OK || deleteInfo)
			_DeleteInfo(info, eventRemoved);
	}

	// reply to the request
	if (error == B_OK) {
		BMessage reply(B_REG_SUCCESS);
		request->SendReply(&reply);
	} else {
		BMessage reply(B_REG_ERROR);
		reply.AddInt32("error", error);
		request->SendReply(&reply);
	}

	FUNCTION_END();
}

// HandleGetRunnerInfo
/*!	\brief Handles an get info request (BMessageRunner::GetInfo()).
	\param request The request message.
*/
void
MessageRunnerManager::HandleGetRunnerInfo(BMessage *request)
{
	FUNCTION_START();

	BAutolock _lock(fLock);
	status_t error = B_OK;
	// get the parameters
	int32 token;
	if (error == B_OK && request->FindInt32("token", &token) != B_OK)
		error = B_BAD_VALUE;
	// find the runner info
	RunnerInfo *info = NULL;
	if (error == B_OK) {
		info = _InfoForToken(token);
		if (!info)
			error = B_BAD_VALUE;
	}
	// reply to the request
	if (error == B_OK) {
		BMessage reply(B_REG_SUCCESS);
		reply.AddInt64("interval", info->interval);
		reply.AddInt32("count", info->count);
		request->SendReply(&reply);
	} else {
		BMessage reply(B_REG_ERROR);
		reply.AddInt32("error", error);
		request->SendReply(&reply);
	}

	FUNCTION_END();
}

// Lock
/*!	\brief Locks the manager.
	\return \c true, if locked successfully, \c false otherwise.
*/
bool
MessageRunnerManager::Lock()
{
	return fLock.Lock();
}

// Unlock
/*!	\brief Unlocks the manager.
*/
void
MessageRunnerManager::Unlock()
{
	fLock.Unlock();
}

// _AddInfo
/*!	\brief Adds a RunnerInfo to the list of RunnerInfos.

	\note The manager must be locked.

	\param info The RunnerInfo to be added.
	\return \c true, if added successfully, \c false otherwise.
*/
bool
MessageRunnerManager::_AddInfo(RunnerInfo *info)
{
	return fRunnerInfos.AddItem(info);
}

// _RemoveInfo
/*!	\brief Removes a RunnerInfo from the list of RunnerInfos.

	\note The manager must be locked.

	\param info The RunnerInfo to be removed.
	\return \c true, if removed successfully, \c false, if the list doesn't
			contain the supplied info.
*/
bool
MessageRunnerManager::_RemoveInfo(RunnerInfo *info)
{
	return fRunnerInfos.RemoveItem(info);
}

// _RemoveInfo
/*!	\brief Removes a RunnerInfo at a given index from the list of RunnerInfos.

	\note The manager must be locked.

	\param index The index of the RunnerInfo to be removed.
	\return \c true, if removed successfully, \c false, if the supplied index
			is out of range.
*/
MessageRunnerManager::RunnerInfo*
MessageRunnerManager::_RemoveInfo(int32 index)
{
	return (RunnerInfo*)fRunnerInfos.RemoveItem(index);
}

// _RemoveInfoWithToken
/*!	\brief Removes a RunnerInfo with a specified token from the list of
		   RunnerInfos.

	\note The manager must be locked.

	\param token The token identifying the RunnerInfo to be removed.
	\return \c true, if removed successfully, \c false, if the list doesn't
			contain an info with the supplied token.
*/
MessageRunnerManager::RunnerInfo*
MessageRunnerManager::_RemoveInfoWithToken(int32 token)
{
	RunnerInfo *info = NULL;
	int32 index = _IndexOfToken(token);
	if (index >= 0)
		info = _RemoveInfo(index);
	return info;
}

// _DeleteInfo
/*!	\brief Removes a RunnerInfo from the list of RunnerInfos and deletes it.

	\note The manager must be locked.

	\param index The index of the RunnerInfo to be deleted.
	\return \c true, if removed and deleted successfully, \c false, if the
			list doesn't contain the supplied info.
*/
bool
MessageRunnerManager::_DeleteInfo(RunnerInfo *info, bool eventRemoved)
{
	bool result = _RemoveInfo(info);
	if (result) {
		// If the event is not in the event queue and has not been removed
		// just before, then it is in progress. It will delete itself.
		if (!eventRemoved && !fEventQueue->RemoveEvent(info->event))
			info->event = NULL;
		delete info;
	}
	return result;
}

// _CountInfos
/*!	\brief Returns the number of RunnerInfos in the list of RunnerInfos.

	\note The manager must be locked.

	\return Returns the number of RunnerInfos in the list of RunnerInfos.
*/
int32
MessageRunnerManager::_CountInfos() const
{
	return fRunnerInfos.CountItems();
}

// _InfoAt
/*!	\brief Returns the RunnerInfo at the specified index in the list of
		   RunnerInfos.

	\note The manager must be locked.

	\param index The index of the RunnerInfo to be returned.
	\return The runner info at the specified index, or \c NULL, if the index
			is out of range.
*/
MessageRunnerManager::RunnerInfo*
MessageRunnerManager::_InfoAt(int32 index) const
{
	return (RunnerInfo*)fRunnerInfos.ItemAt(index);
}

// _InfoForToken
/*!	\brief Returns the RunnerInfo with the specified index.

	\note The manager must be locked.

	\param token The token identifying the RunnerInfo to be returned.
	\return The runner info at the specified index, or \c NULL, if the list
			doesn't contain an info with the specified token.
*/
MessageRunnerManager::RunnerInfo*
MessageRunnerManager::_InfoForToken(int32 token) const
{
	return _InfoAt(_IndexOfToken(token));
}

// _IndexOf
/*!	\brief Returns the index of the supplied RunnerInfo in the list of
		   RunnerInfos.

	\note The manager must be locked.

	\param info The RunnerInfo whose index shall be returned.
	\return The index of the supplied RunnerInfo, or -1, if the list doesn't
			contain the supplied info.
*/
int32
MessageRunnerManager::_IndexOf(RunnerInfo *info) const
{
	return fRunnerInfos.IndexOf(info);
}

// _IndexOfToken
/*!	\brief Returns the index of the RunnerInfo identified by the supplied
		   token in the list of RunnerInfos.

	\note The manager must be locked.

	\param token The token identifying the RunnerInfo whose index shall be
		   returned.
	\return The index of the requested RunnerInfo, or -1, if the list doesn't
			contain an info with the supplied token.
*/
int32
MessageRunnerManager::_IndexOfToken(int32 token) const
{
	for (int32 i = 0; RunnerInfo *info = _InfoAt(i); i++) {
		if (info->token == token)
			return i;
	}
	return -1;
}

// _DoEvent
/*!	\brief Invoked when a message runner's event is executed.

	If the message runner info is still valid and the event was not just
	rescheduled, the message is delivered to the message runner's target
	and the event is rescheduled.

	\param info The message runner's info.
	\return \c true, if the event object shall be deleted, \c false otherwise.
*/
bool
MessageRunnerManager::_DoEvent(RunnerInfo *info)
{
	FUNCTION_START();

	BAutolock _lock(fLock);
	bool deleteEvent = false;
	// first check whether the info does still exist
	if (_lock.IsLocked() && _IndexOf(info) >= 0) {
		// If the event has been rescheduled after being removed from the
		// queue for execution, it needs to be ignored. This may happen, when
		// the interval is modified.
		if (info->rescheduled)
			info->rescheduled = false;
		else {
			// send the message
			bool success = (info->DeliverMessage() == B_OK);
			// reschedule the event
			if (success)
				success = _ScheduleEvent(info);

			// clean up, if the message delivery of the rescheduling failed
			// (or the runner had already fulfilled its job)
			if (!success) {
				deleteEvent = true;
				info->event = NULL;
				_RemoveInfo(info);
				delete info;
			}
		}
	} else {
		// The info is no more. That means it had been removed after the
		// event was removed from the event queue, but before we could acquire
		// the lock. Simply delete the event.
		deleteEvent = true;
	}

	FUNCTION_END();

	return deleteEvent;
}

// _ScheduleEvent
/*!	\brief Schedules the event for a message runner for the next time a
		   message has to be sent.

	\note The manager must be locked.

	\param info The message runner's info.
	\return \c true, if the event successfully been rescheduled, \c false,
			if either all messages have already been sent or the event queue
			doesn't allow adding the event (e.g. due to insufficient memory).
*/
bool
MessageRunnerManager::_ScheduleEvent(RunnerInfo *info)
{
	bool scheduled = false;
	// calculate next event time
	if (info->count != 0) {
		info->time = add_time(info->time, info->interval);

		// For runners without a count limit, we skip messages, if we're already
		// late.
		bigtime_t now = system_time();
		if (info->time < now && info->count < 0) {
			// keep the remainder modulo interval
			info->time = add_time(now,
				info->interval - (now - info->time) % info->interval);
		}

		info->event->SetTime(info->time);
		scheduled = fEventQueue->AddEvent(info->event);

PRINT("runner %ld (%lld, %ld) rescheduled: %d, time: %lld, now: %lld\n",
info->token, info->interval, info->count, scheduled, info->time, system_time());
	}
	return scheduled;
}

// _NextToken
/*!	\brief Returns a new unused message runner token.

	\note The manager must be locked.

	\return A new unused message runner token.
*/
int32
MessageRunnerManager::_NextToken()
{
	return fNextToken++;
}

