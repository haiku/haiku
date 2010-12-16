/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "Thread.h"

#include <stdio.h>

#include "CpuState.h"
#include "StackTrace.h"
#include "Team.h"


Thread::Thread(Team* team, thread_id threadID)
	:
	fTeam(team),
	fID(threadID),
	fState(THREAD_STATE_UNKNOWN),
	fStoppedReason(THREAD_STOPPED_UNKNOWN),
	fCpuState(NULL),
	fStackTrace(NULL)
{
}


Thread::~Thread()
{
	if (fCpuState != NULL)
		fCpuState->ReleaseReference();
	if (fStackTrace != NULL)
		fStackTrace->ReleaseReference();
}


status_t
Thread::Init()
{
	return B_OK;
}


bool
Thread::IsMainThread() const
{
	return fID == fTeam->ID();
}


void
Thread::SetName(const BString& name)
{
	fName = name;
}


void
Thread::SetState(uint32 state, uint32 reason, const BString& info)
{
	if (state == fState)
		return;

	fState = state;
	fStoppedReason = reason;
	fStoppedReasonInfo = info;

	// unset CPU state and stack trace, if the thread isn't stopped
	if (fState != THREAD_STATE_STOPPED) {
		SetCpuState(NULL);
		SetStackTrace(NULL);
	}

	fTeam->NotifyThreadStateChanged(this);
}


void
Thread::SetCpuState(CpuState* state)
{
	if (state == fCpuState)
		return;

	if (fCpuState != NULL)
		fCpuState->ReleaseReference();

	fCpuState = state;

	if (fCpuState != NULL)
		fCpuState->AcquireReference();

	fTeam->NotifyThreadCpuStateChanged(this);
}


void
Thread::SetStackTrace(StackTrace* trace)
{
	if (trace == fStackTrace)
		return;

	if (fStackTrace != NULL)
		fStackTrace->ReleaseReference();

	fStackTrace = trace;

	if (fStackTrace != NULL)
		fStackTrace->AcquireReference();

	fTeam->NotifyThreadStackTraceChanged(this);
}
