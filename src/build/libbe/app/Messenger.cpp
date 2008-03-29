/*
 * Copyright 2001-2008, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold, bonefish@users.sf.net
 */

/*!
	\class BMessenger
	Delivers messages to local or remote targets.
*/

#include <Messenger.h>

#include <new>
#include <stdio.h>
#include <string.h>

#include <Application.h>
#include <Handler.h>
#include <Looper.h>
#include <Message.h>
#include <OS.h>
#include <Roster.h>

#include <AppMisc.h>
#include <LooperList.h>
#include <MessageUtils.h>
#include <MessengerPrivate.h>
#include <ObjectLocker.h>
#include <TokenSpace.h>

#define DBG(x)
#define OUT	printf

using BPrivate::gDefaultTokens;
using BPrivate::gLooperList;
using BPrivate::BLooperList;
using BPrivate::BObjectLocker;


//	#pragma mark -


/*!	\brief Creates an unitialized BMessenger.
*/
BMessenger::BMessenger()
	:
	fPort(-1),
	fHandlerToken(B_NULL_TOKEN),
	fTeam(-1),
	fPreferredTarget(false)
{
}


/*!	\brief Creates a BMessenger and initializes it to have the same target
	as the supplied messemger.

	\param from The messenger to be copied.
*/
BMessenger::BMessenger(const BMessenger &from)
	:
	fPort(from.fPort),
	fHandlerToken(from.fHandlerToken),
	fTeam(from.fTeam),
	fPreferredTarget(from.fPreferredTarget)
{
}


/*!	\brief Frees all resources associated with this object.
*/
BMessenger::~BMessenger()
{
}


/*!	\brief Makes this BMessenger a copy of the supplied one.

	\param from the messenger to be copied.
	\return A reference to this object.
*/
BMessenger &
BMessenger::operator=(const BMessenger &from)
{
	if (this != &from) {
		fPort = from.fPort;
		fHandlerToken = from.fHandlerToken;
		fTeam = from.fTeam;
		fPreferredTarget = from.fPreferredTarget;
	}
	return *this;
}


/*!	\brief Returns whether this and the supplied messenger have the same
	target.

	\param other The other messenger.
	\return \c true, if the messengers have the same target or if both aren't
			properly initialzed, \c false otherwise.
*/
bool
BMessenger::operator==(const BMessenger &other) const
{
	// Note: The fTeam fields are not compared.
	return fPort == other.fPort
		&& fHandlerToken == other.fHandlerToken
		&& fPreferredTarget == other.fPreferredTarget;
}


/*!	\brief Returns whether the messenger's target looper does still exist.

	It is not checked whether the target handler is also still existing.

	\return \c true, if the messenger's target looper does still exist,
			\c false otherwise.
*/
bool
BMessenger::IsValid() const
{
	return fPort >= 0;
		// for the build version, we don't actually check the port
}


/*!	\brief Returns the ID of the team the messenger's target lives in.

	\return The team of the messenger's target.
*/
team_id
BMessenger::Team() const
{
	return fTeam;
}


/*!	\brief Sets the messenger's team, target looper port and handler token.

	If \a preferred is \c true, \a token is ignored.

	\param team The target's team.
	\param port The target looper port.
	\param token The target handler token.
	\param preferred \c true to rather use the looper's preferred handler
		   instead of the one specified by \a token.
*/
void
BMessenger::SetTo(team_id team, port_id port, int32 token, bool preferred)
{
	fTeam = team;
	fPort = port;
	fHandlerToken = preferred ? B_PREFERRED_TOKEN : token;
	fPreferredTarget = preferred;
}


/*!	\brief Returns whether the first one of two BMessengers is less than the
	second one.

	This method defines an order on BMessengers based on their member
	variables \c fPort, \c fHandlerToken and \c fPreferredTarget.

	\param a The first messenger.
	\param b The second messenger.
	\return \c true, if \a a is less than \a b, \c false otherwise.
*/
bool
operator<(const BMessenger &_a, const BMessenger &_b)
{
	BMessenger::Private a(const_cast<BMessenger&>(_a));
	BMessenger::Private b(const_cast<BMessenger&>(_b));

	// significance:
	// 1. fPort
	// 2. fHandlerToken
	// 3. fPreferredTarget
	// fTeam is insignificant
	return a.Port() < b.Port()
		|| (a.Port() == b.Port()
			&& (a.Token() < b.Token()
				|| (a.Token() == b.Token()
					&& !a.IsPreferredTarget()
					&& b.IsPreferredTarget())));
}


/*!	\brief Returns whether two BMessengers have not the same target.

	\param a The first messenger.
	\param b The second messenger.
	\return \c false, if \a a and \a b have the same targets or are both not
			properly initialized, \c true otherwise.
*/
bool
operator!=(const BMessenger &a, const BMessenger &b)
{
	return !(a == b);
}
