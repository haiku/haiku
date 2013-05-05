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


GetCpuStateJob::GetCpuStateJob(DebuggerInterface* debuggerInterface,
	Thread* thread)
	:
	fKey(thread, JOB_TYPE_GET_CPU_STATE),
	fDebuggerInterface(debuggerInterface),
	fThread(thread)
{
	fThread->AcquireReference();
}


GetCpuStateJob::~GetCpuStateJob()
{
	fThread->ReleaseReference();
}


const JobKey&
GetCpuStateJob::Key() const
{
	return fKey;
}


status_t
GetCpuStateJob::Do()
{
	CpuState* state;
	status_t error = fDebuggerInterface->GetCpuState(fThread->ID(), state);
	if (error != B_OK)
		return error;
	BReference<CpuState> reference(state, true);

	AutoLocker<Team> locker(fThread->GetTeam());

	if (fThread->State() == THREAD_STATE_STOPPED)
		fThread->SetCpuState(state);

	return B_OK;
}
