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
//	File Name:		MessageRunnerManager.cpp
//	Author:			Ingo Weinhold (bonefish@users.sf.net)
//	Description:	Manages the registrar side "shadows" of BMessageRunners.
//------------------------------------------------------------------------------

#include <algobase.h>
#include <new.h>

#include <Autolock.h>
#include <Message.h>
#include <Messenger.h>
#include <OS.h>
#include <RegistrarDefs.h>

#include "Debug.h"
#include "Event.h"
#include "EventQueue.h"
#include "MessageRunnerManager.h"

// the minimal time interval for message runners
static const bigtime_t kMininalTimeInterval = 50000LL;

// RunnerEvent
class MessageRunnerManager::RunnerEvent : public Event {
public:
	RunnerEvent(MessageRunnerManager *manager, RunnerInfo *info)
		: Event(false),
		  fManager(manager),
		  fInfo(info)
	{
	}

	virtual bool Do()
	{
		return fManager->_DoEvent(fInfo);
	}

private:
	MessageRunnerManager	*fManager;
	RunnerInfo				*fInfo;
};


// RunnerInfo
struct MessageRunnerManager::RunnerInfo {
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

	~RunnerInfo()
	{
		delete message;
		delete event;
	}

	status_t DeliverMessage()
	{
		if (count > 0)
			count--;
		status_t error = target.SendMessage(message, replyTarget, 0);
		// B_WOULD_BLOCK is as good as B_OK. We return an error only, if
		// there are serious problems with the target, i.e. if it doesn't
		// exist anymore for instance. A full message port is harmless.
		if (error == B_WOULD_BLOCK)
			error = B_OK;
		return error;
	}

	team_id		team;
	int32		token;
	BMessenger	target;
	BMessage	*message;
	bigtime_t	interval;
	int32		count;
	BMessenger	replyTarget;
	bigtime_t	time;
	RunnerEvent	*event;
	bool		rescheduled;
};


// constructor
MessageRunnerManager::MessageRunnerManager(EventQueue *eventQueue)
	: fRunnerInfos(),
	  fLock(),
	  fEventQueue(eventQueue),
	  fNextToken(0)
{
}

// destructor
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
		if (!info)
			error = B_BAD_VALUE;
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
bool
MessageRunnerManager::Lock()
{
	return fLock.Lock();
}

// Unlock
void
MessageRunnerManager::Unlock()
{
	fLock.Unlock();
}

// _AddInfo
bool
MessageRunnerManager::_AddInfo(RunnerInfo *info)
{
	return fRunnerInfos.AddItem(info);
}

// _RemoveInfo
bool
MessageRunnerManager::_RemoveInfo(RunnerInfo *info)
{
	return fRunnerInfos.RemoveItem(info);
}

// _RemoveInfo
MessageRunnerManager::RunnerInfo*
MessageRunnerManager::_RemoveInfo(int32 index)
{
	return (RunnerInfo*)fRunnerInfos.RemoveItem(index);
}

// _RemoveInfoWithToken
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
int32
MessageRunnerManager::_CountInfos() const
{
	return fRunnerInfos.CountItems();
}

// _InfoAt
MessageRunnerManager::RunnerInfo*
MessageRunnerManager::_InfoAt(int32 index) const
{
	return (RunnerInfo*)fRunnerInfos.ItemAt(index);
}

// _InfoForToken
MessageRunnerManager::RunnerInfo*
MessageRunnerManager::_InfoForToken(int32 token) const
{
	return _InfoAt(_IndexOfToken(token));
}

// _IndexOf
int32
MessageRunnerManager::_IndexOf(RunnerInfo *info) const
{
	return fRunnerInfos.IndexOf(info);
}

// _IndexOfToken
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
bool
MessageRunnerManager::_ScheduleEvent(RunnerInfo *info)
{
	bool scheduled = false;
	// calculate next event time
	if (info->count != 0) {
		// avoid a bigtime_t overflow
		if (LONGLONG_MAX - info->interval < info->time)
			info->time = LONGLONG_MAX;
		else
			info->time += info->interval;
		info->event->SetTime(info->time);
		scheduled = fEventQueue->AddEvent(info->event);
PRINT(("runner %ld (%lld, %ld) rescheduled: %d, time: %lld, now: %lld\n",
info->token, info->interval, info->count, scheduled, info->time, system_time()));
	}
	return scheduled;
}

// _NextToken
int32
MessageRunnerManager::_NextToken()
{
	return fNextToken++;
}

