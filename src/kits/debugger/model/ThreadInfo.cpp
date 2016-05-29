/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "ThreadInfo.h"


ThreadInfo::ThreadInfo()
	:
	fTeam(-1),
	fThread(-1),
	fName()
{
}


ThreadInfo::ThreadInfo(const ThreadInfo& other)
	:
	fTeam(other.fTeam),
	fThread(other.fThread),
	fName(other.fName)
{
}


ThreadInfo::ThreadInfo(team_id team, thread_id thread, const BString& name)
	:
	fTeam(team),
	fThread(thread),
	fName(name)
{
}


void
ThreadInfo::SetTo(team_id team, thread_id thread, const BString& name)
{
	fTeam = team;
	fThread = thread;
	fName = name;
}
