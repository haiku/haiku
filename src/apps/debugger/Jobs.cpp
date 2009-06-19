/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "Jobs.h"

#include <AutoLocker.h>

#include "Architecture.h"
#include "CpuState.h"
#include "DebuggerInterface.h"
#include "StackTrace.h"
#include "Team.h"
#include "Thread.h"


// #pragma mark - GetCpuStateJob


GetCpuStateJob::GetCpuStateJob(DebuggerInterface* debuggerInterface,
	Thread* thread)
	:
	fDebuggerInterface(debuggerInterface),
	fThread(thread)
{
	fThread->AddReference();
}


GetCpuStateJob::~GetCpuStateJob()
{
	fThread->RemoveReference();
}


JobKey
GetCpuStateJob::Key() const
{
	return JobKey(fThread, JOB_TYPE_GET_CPU_STATE);
}


status_t
GetCpuStateJob::Do()
{
	CpuState* state;
	status_t error = fDebuggerInterface->GetCpuState(fThread->ID(), state);
	if (error != B_OK)
		return error;
	Reference<CpuState> reference(state, true);

	AutoLocker<Team> locker(fThread->GetTeam());

	if (fThread->State() == THREAD_STATE_STOPPED)
		fThread->SetCpuState(state);

	return B_OK;
}


// #pragma mark - GetStackTraceJob


GetStackTraceJob::GetStackTraceJob(DebuggerInterface* debuggerInterface,
	Architecture* architecture, Thread* thread)
	:
	fDebuggerInterface(debuggerInterface),
	fArchitecture(architecture),
	fThread(thread)
{
	fThread->AddReference();

	fCpuState = fThread->GetCpuState();
	if (fCpuState != NULL)
		fCpuState->AddReference();
}


GetStackTraceJob::~GetStackTraceJob()
{
	if (fCpuState != NULL)
		fCpuState->RemoveReference();

	fThread->RemoveReference();
}


JobKey
GetStackTraceJob::Key() const
{
	return JobKey(fThread, JOB_TYPE_GET_CPU_STATE);
}


status_t
GetStackTraceJob::Do()
{
	if (fCpuState == NULL)
		return B_BAD_VALUE;

	// get the stack trace
	StackTrace* stackTrace;
	status_t error = fArchitecture->CreateStackTrace(fThread->GetTeam(),
		fCpuState, stackTrace);
	if (error != B_OK)
		return error;
	Reference<StackTrace> stackTraceReference(stackTrace, true);

	// set the stack trace, unless something has changed
	AutoLocker<Team> locker(fThread->GetTeam());

	if (fThread->GetCpuState() == fCpuState)
		fThread->SetStackTrace(stackTrace);

	return B_OK;
}
