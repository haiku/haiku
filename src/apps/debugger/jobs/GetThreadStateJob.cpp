/*
 * Copyright 2012, Rene Gollent, rene@gollent.com.
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "Jobs.h"

#include <AutoLocker.h>

#include "CpuState.h"
#include "DebuggerInterface.h"
#include "Team.h"
#include "Thread.h"


GetThreadStateJob::GetThreadStateJob(DebuggerInterface* debuggerInterface,
	Thread* thread)
	:
	fKey(thread, JOB_TYPE_GET_THREAD_STATE),
	fDebuggerInterface(debuggerInterface),
	fThread(thread)
{
	fThread->AcquireReference();
}


GetThreadStateJob::~GetThreadStateJob()
{
	fThread->ReleaseReference();
}


const JobKey&
GetThreadStateJob::Key() const
{
	return fKey;
}


status_t
GetThreadStateJob::Do()
{
	CpuState* state = NULL;
	status_t error = fDebuggerInterface->GetCpuState(fThread->ID(), state);
	BReference<CpuState> reference(state, true);

	AutoLocker<Team> locker(fThread->GetTeam());

	if (fThread->State() != THREAD_STATE_UNKNOWN)
		return B_OK;

	if (error == B_OK) {
		fThread->SetState(THREAD_STATE_STOPPED);
		fThread->SetCpuState(state);
	} else if (error == B_BAD_THREAD_STATE) {
		fThread->SetState(THREAD_STATE_RUNNING);
	} else
		return error;

	return B_OK;
}
