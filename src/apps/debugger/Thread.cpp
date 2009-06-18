/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "Thread.h"


Thread::Thread(Team* team, thread_id threadID)
	:
	fTeam(team),
	fID(threadID),
	fState(THREAD_STATE_UNKNOWN)
{
}


Thread::~Thread()
{
}


status_t
Thread::Init()
{
	return B_OK;
}


void
Thread::SetName(const BString& name)
{
	fName = name;
}


void
Thread::SetState(uint32 state)
{
	fState = state;
}
