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
//	File Name:		Messenger.h
//	Author:			Ingo Weinhold (bonefish@users.sf.net)
//	Description:	BMessenger delivers messages to local or remote targets.
//------------------------------------------------------------------------------

#ifndef _MESSENGER_H
#define _MESSENGER_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <BeBuild.h>
#include <OS.h>
#include <ByteOrder.h>
#include <Message.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class BHandler;
class BLooper;

// BMessenger class ------------------------------------------------------------
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
private:
	friend class BRoster;
	friend class _TRoster_;
	friend class BMessage;
	friend inline void _set_message_reply_(BMessage *, BMessenger);
	friend status_t swap_data(type_code, void *, size_t, swap_action);
	friend bool operator<(const BMessenger &a, const BMessenger &b);
	friend bool operator!=(const BMessenger &a, const BMessenger &b);
				
	BMessenger(team_id team, port_id port, int32 token, bool preferred);

	void InitData(const char *signature, team_id team, status_t *result);

private:
	port_id	fPort;
	int32	fHandlerToken;
	team_id	fTeam;
	int32	extra0;
	int32	extra1;
	bool	fPreferredTarget;
	bool	extra2;
	bool	extra3;
	bool	extra4;
};

_IMPEXP_BE bool operator<(const BMessenger &a, const BMessenger &b);
_IMPEXP_BE bool operator!=(const BMessenger &a, const BMessenger &b);

#endif	// _MESSENGER_H
