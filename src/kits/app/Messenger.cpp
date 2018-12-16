/*
 * Copyright 2001-2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold (bonefish@users.sf.net)
 */


#include <Messenger.h>

#include <new>
#include <stdio.h>
#include <strings.h>

#include <Application.h>
#include <AutoLocker.h>
#include <Handler.h>
#include <Looper.h>
#include <Message.h>
#include <OS.h>
#include <Roster.h>

#include <AppMisc.h>
#include <LaunchRoster.h>
#include <LooperList.h>
#include <MessagePrivate.h>
#include <MessageUtils.h>
#include <TokenSpace.h>


// debugging
//#define DBG(x) x
#define DBG(x)
#define OUT	printf

using BPrivate::gDefaultTokens;
using BPrivate::gLooperList;
using BPrivate::BLooperList;


BMessenger::BMessenger()
	:
	fPort(-1),
	fHandlerToken(B_NULL_TOKEN),
	fTeam(-1)
{
}


BMessenger::BMessenger(const char* signature, team_id team, status_t* result)
	:
	fPort(-1),
	fHandlerToken(B_NULL_TOKEN),
	fTeam(-1)
{
	_InitData(signature, team, result);
}


BMessenger::BMessenger(const BHandler* handler, const BLooper* looper,
	status_t* _result)
	:
	fPort(-1),
	fHandlerToken(B_NULL_TOKEN),
	fTeam(-1)
{
	_InitData(handler, looper, _result);
}


BMessenger::BMessenger(const BMessenger& other)
	:
	fPort(other.fPort),
	fHandlerToken(other.fHandlerToken),
	fTeam(other.fTeam)
{
}


BMessenger::~BMessenger()
{
}


//	#pragma mark - Target


bool
BMessenger::IsTargetLocal() const
{
	return BPrivate::current_team() == fTeam;
}


BHandler*
BMessenger::Target(BLooper** _looper) const
{
	BHandler* handler = NULL;
	if (IsTargetLocal()
		&& (fHandlerToken > B_NULL_TOKEN
			|| fHandlerToken == B_PREFERRED_TOKEN)) {
		gDefaultTokens.GetToken(fHandlerToken, B_HANDLER_TOKEN,
			(void**)&handler);
		if (_looper)
			*_looper = BPrivate::gLooperList.LooperForPort(fPort);
	} else if (_looper)
		*_looper = NULL;

	return handler;
}


bool
BMessenger::LockTarget() const
{
	BLooper* looper = NULL;
	Target(&looper);
	if (looper != NULL && looper->Lock()) {
		if (looper->fMsgPort == fPort)
			return true;

		looper->Unlock();
		return false;
	}

	return false;
}


status_t
BMessenger::LockTargetWithTimeout(bigtime_t timeout) const
{
	BLooper* looper = NULL;
	Target(&looper);
	if (looper == NULL)
		return B_BAD_VALUE;

	status_t result = looper->LockWithTimeout(timeout);

	if (result == B_OK && looper->fMsgPort != fPort) {
		looper->Unlock();
		return B_BAD_PORT_ID;
	}

	return result;
}


//	#pragma mark - Message sending


status_t
BMessenger::SendMessage(uint32 command, BHandler* replyTo) const
{
	BMessage message(command);
	return SendMessage(&message, replyTo);
}


status_t
BMessenger::SendMessage(BMessage* message, BHandler* replyTo,
	bigtime_t timeout) const
{
	DBG(OUT("BMessenger::SendMessage2(%.4s)\n", (char*)&message->what));

	status_t result = message != NULL ? B_OK : B_BAD_VALUE;
	if (result == B_OK) {
		BMessenger replyMessenger(replyTo);
		result = SendMessage(message, replyMessenger, timeout);
	}

	DBG(OUT("BMessenger::SendMessage2() done: %lx\n", result));

	return result;
}


status_t
BMessenger::SendMessage(BMessage* message, BMessenger replyTo,
	bigtime_t timeout) const
{
	if (message == NULL)
		return B_BAD_VALUE;

	return BMessage::Private(message).SendMessage(fPort, fTeam, fHandlerToken,
		timeout, false, replyTo);
}


status_t
BMessenger::SendMessage(uint32 command, BMessage* reply) const
{
	BMessage message(command);

	return SendMessage(&message, reply);
}


status_t
BMessenger::SendMessage(BMessage* message, BMessage* reply,
	bigtime_t deliveryTimeout, bigtime_t replyTimeout) const
{
	if (message == NULL || reply == NULL)
		return B_BAD_VALUE;

	status_t result = BMessage::Private(message).SendMessage(fPort, fTeam,
		fHandlerToken, reply, deliveryTimeout, replyTimeout);

	// map this result for now
	if (result == B_BAD_TEAM_ID)
		result = B_BAD_PORT_ID;

	return result;
}


//	#pragma mark - Operators and misc


status_t
BMessenger::SetTo(const char* signature, team_id team)
{
	status_t result = B_OK;
	_InitData(signature, team, &result);

	return result;
}


status_t
BMessenger::SetTo(const BHandler* handler, const BLooper* looper)
{
	status_t result = B_OK;
	_InitData(handler, looper, &result);

	return result;
}


BMessenger&
BMessenger::operator=(const BMessenger& other)
{
	if (this != &other) {
		fPort = other.fPort;
		fHandlerToken = other.fHandlerToken;
		fTeam = other.fTeam;
	}

	return *this;
}


bool
BMessenger::operator==(const BMessenger& other) const
{
	// Note: The fTeam fields are not compared.
	return fPort == other.fPort && fHandlerToken == other.fHandlerToken;
}


bool
BMessenger::IsValid() const
{
	port_info info;
	return fPort >= 0 && get_port_info(fPort, &info) == B_OK;
}


team_id
BMessenger::Team() const
{
	return fTeam;
}


uint32
BMessenger::HashValue() const
{
	return fPort * 19 + fHandlerToken;
}


//	#pragma mark - Private or reserved


/*!	Sets the messenger's team, target looper port and handler token.

	To target the preferred handler, use \c B_PREFERRED_TOKEN as token.

	\param team The target's team.
	\param port The target looper port.
	\param token The target handler token.
*/
void
BMessenger::_SetTo(team_id team, port_id port, int32 token)
{
	fTeam = team;
	fPort = port;
	fHandlerToken = token;
}


/*!	Initializes the BMessenger object's data given the signature and/or
	team ID of a target.

	When only a signature is given, and multiple instances of the application
	are running it is undeterminate which one is chosen as the target. In case
	only a team ID is passed, the target application is identified uniquely.
	If both are supplied, the application identified by the team ID must have
	a matching signature, otherwise the initilization fails.

	\param signature The target application's signature. May be \c NULL.
	\param team The target application's team ID. May be < 0.
	\param result An optional pointer to a pre-allocated status_t into which
		   the result of the initialization is written.
*/
void
BMessenger::_InitData(const char* signature, team_id team, status_t* _result)
{
	status_t result = B_OK;

	// get an app_info
	app_info info;
	if (team < 0) {
		// no team ID given
		if (signature != NULL) {
			// Try existing launch communication data first
			BMessage data;
			if (BLaunchRoster().GetData(signature, data) == B_OK) {
				info.port = data.GetInt32("port", -1);
				team = data.GetInt32("team", -5);
			}
			if (info.port < 0) {
				result = be_roster->GetAppInfo(signature, &info);
				team = info.team;
				// B_ERROR means that no application with the given signature
				// is running. But we are supposed to return B_BAD_VALUE.
				if (result == B_ERROR)
					result = B_BAD_VALUE;
			} else
				info.flags = 0;
		} else
			result = B_BAD_TYPE;
	} else {
		// a team ID is given
		result = be_roster->GetRunningAppInfo(team, &info);
		// Compare the returned signature with the supplied one.
		if (result == B_OK && signature != NULL
			&& strcasecmp(signature, info.signature) != 0) {
			result = B_MISMATCHED_VALUES;
		}
	}
	// check whether the app flags say B_ARGV_ONLY
	if (result == B_OK && (info.flags & B_ARGV_ONLY) != 0) {
		result = B_BAD_TYPE;
		// Set the team ID nevertheless -- that's what Be's implementation
		// does. Don't know, if that is a bug, but at least it doesn't harm.
		fTeam = team;
	}
	// init our members
	if (result == B_OK) {
		fTeam = team;
		fPort = info.port;
		fHandlerToken = B_PREFERRED_TOKEN;
	}

	// return the result
	if (_result != NULL)
		*_result = result;
}


/*!	Initializes the BMessenger to target the local BHandler and/or BLooper.

	When a \c NULL handler is supplied, the preferred handler in the given
	looper is targeted. If no looper is supplied the looper the given handler
	belongs to is used -- that means in particular, that the handler must
	already belong to a looper. If both are supplied the handler must actually
	belong to looper.

	\param handler The target handler. May be \c NULL.
	\param looper The target looper. May be \c NULL.
	\param result An optional pointer to a pre-allocated status_t into which
	       the result of the initialization is written.
*/
void
BMessenger::_InitData(const BHandler* handler, const BLooper* looper,
	status_t* _result)
{
	status_t result = handler || looper != NULL ? B_OK : B_BAD_VALUE;
	if (result == B_OK) {
		if (handler != NULL) {
			// BHandler is given, check/retrieve the looper.
			if (looper != NULL) {
				if (handler->Looper() != looper)
					result = B_MISMATCHED_VALUES;
			} else {
				looper = handler->Looper();
				if (looper == NULL)
					result = B_MISMATCHED_VALUES;
			}
		}

		// set port, token,...
		if (result == B_OK) {
			AutoLocker<BLooperList> locker(gLooperList);
			if (locker.IsLocked() && gLooperList.IsLooperValid(looper)) {
				fPort = looper->fMsgPort;
				fHandlerToken = handler != NULL
					? _get_object_token_(handler)
					: B_PREFERRED_TOKEN;
				fTeam = looper->Team();
			} else
				result = B_BAD_VALUE;
		}
	}

	if (_result != NULL)
		*_result = result;
}


//	#pragma mark - Operator functions


bool
operator<(const BMessenger& _a, const BMessenger& _b)
{
	BMessenger::Private a(const_cast<BMessenger&>(_a));
	BMessenger::Private b(const_cast<BMessenger&>(_b));

	// significance:
	// 1. fPort
	// 2. fHandlerToken
	// 3. fPreferredTarget
	// fTeam is insignificant
	return (a.Port() < b.Port()
			|| (a.Port() == b.Port()
				&& (a.Token() < b.Token()
					|| (a.Token() == b.Token()
						&& !a.IsPreferredTarget()
						&& b.IsPreferredTarget()))));
}


bool
operator!=(const BMessenger& a, const BMessenger& b)
{
	return !(a == b);
}
