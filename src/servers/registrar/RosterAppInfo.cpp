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
//	File Name:		RosterAppInfo.cpp
//	Author:			Ingo Weinhold (bonefish@users.sf.net)
//	Description:	An extended app_info.
//------------------------------------------------------------------------------

#include <new>

#include <string.h>

#include "RosterAppInfo.h"

using std::nothrow;

// constructor
RosterAppInfo::RosterAppInfo()
			 : app_info(),
			   state(APP_STATE_UNREGISTERED),
			   token(0),
			   registration_time(0)
{
}

// Init
void
RosterAppInfo::Init(thread_id thread, team_id team, port_id port, uint32 flags,
					const entry_ref *ref, const char *signature)
{
	this->thread = thread;
	this->team = team;
	this->port = port;
	this->flags = flags;
	this->ref = *ref;
	if (signature) {
		strncpy(this->signature, signature, B_MIME_TYPE_LENGTH - 1);
		this->signature[B_MIME_TYPE_LENGTH - 1] = '\0';
	} else
		this->signature[0] = '\0';
}

// Clone
RosterAppInfo *
RosterAppInfo::Clone() const
{
	RosterAppInfo *clone = new(nothrow) RosterAppInfo;
	if (!clone)
		return NULL;

	clone->Init(thread, team, port, flags, &ref, signature);
	clone->registration_time = registration_time;
	return clone;
}
