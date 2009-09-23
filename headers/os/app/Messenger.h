/*
 * Copyright 2001-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold (bonefish@users.sf.net)
 */
#ifndef _MESSENGER_H
#define _MESSENGER_H


#include <OS.h>
#include <ByteOrder.h>
#include <Message.h>

class BHandler;
class BLooper;


class BMessenger {
public:	
	BMessenger();
	BMessenger(const char *signature, team_id team = -1,
			   status_t *result = NULL);
	BMessenger(const BHandler *handler, const BLooper *looper = NULL,
			   status_t *result = NULL);
	BMessenger(const BMessenger &from);
	~BMessenger();

	// Target

	bool IsTargetLocal() const;
	BHandler *Target(BLooper **looper) const;
	bool LockTarget() const;
	status_t LockTargetWithTimeout(bigtime_t timeout) const;

	// Message sending

	status_t SendMessage(uint32 command, BHandler *replyTo = NULL) const;
	status_t SendMessage(BMessage *message, BHandler *replyTo = NULL,
						 bigtime_t timeout = B_INFINITE_TIMEOUT) const;
	status_t SendMessage(BMessage *message, BMessenger replyTo,
						 bigtime_t timeout = B_INFINITE_TIMEOUT) const;
	status_t SendMessage(uint32 command, BMessage *reply) const;
	status_t SendMessage(BMessage *message, BMessage *reply,
						 bigtime_t deliveryTimeout = B_INFINITE_TIMEOUT,
						 bigtime_t replyTimeout = B_INFINITE_TIMEOUT) const;
	
	// Operators and misc

	BMessenger &operator=(const BMessenger &from);
	bool operator==(const BMessenger &other) const;

	bool IsValid() const;
	team_id Team() const;

	//----- Private or reserved -----------------------------------------

	class Private;

private:
	friend class Private;

	void _SetTo(team_id team, port_id port, int32 token);
	void _InitData(const char *signature, team_id team, status_t *result);

private:
	port_id	fPort;
	int32	fHandlerToken;
	team_id	fTeam;

	int32	_reserved[3];
};

bool operator<(const BMessenger &a, const BMessenger &b);
bool operator!=(const BMessenger &a, const BMessenger &b);

#endif	// _MESSENGER_H
