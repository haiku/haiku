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
	BMessenger(const BMessenger &from);
	~BMessenger();

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

private:
	port_id	fPort;
	int32	fHandlerToken;
	team_id	fTeam;

	int32	_reserved[3];
};

bool operator<(const BMessenger &a, const BMessenger &b);
bool operator!=(const BMessenger &a, const BMessenger &b);

#endif	// _MESSENGER_H
