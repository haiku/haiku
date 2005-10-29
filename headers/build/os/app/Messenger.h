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
				
	void SetTo(team_id team, port_id port, int32 token, bool preferred);

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
