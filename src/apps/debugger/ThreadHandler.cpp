/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "ThreadHandler.h"

#include <stdio.h>

#include <new>

#include <AutoLocker.h>

#include "CpuState.h"
#include "DebuggerInterface.h"
#include "Jobs.h"
#include "MessageCodes.h"
#include "Team.h"
#include "TeamDebugModel.h"
#include "Worker.h"


ThreadHandler::ThreadHandler(TeamDebugModel* debugModel, Thread* thread,
	Worker* worker, DebuggerInterface* debuggerInterface)
	:
	fDebugModel(debugModel),
	fThread(thread),
	fWorker(worker),
	fDebuggerInterface(debuggerInterface)
{
}


ThreadHandler::~ThreadHandler()
{
}


void
ThreadHandler::Init()
{
	fWorker->ScheduleJob(new(std::nothrow) GetThreadStateJob(fDebuggerInterface,
		fThread));
}


bool
ThreadHandler::HandleThreadDebugged(ThreadDebuggedEvent* event)
{
	return _HandleThreadStopped(NULL);
}


bool
ThreadHandler::HandleDebuggerCall(DebuggerCallEvent* event)
{
	return _HandleThreadStopped(NULL);
}


bool
ThreadHandler::HandleBreakpointHit(BreakpointHitEvent* event)
{
	return _HandleThreadStopped(event->GetCpuState());
}


bool
ThreadHandler::HandleWatchpointHit(WatchpointHitEvent* event)
{
	return _HandleThreadStopped(event->GetCpuState());
}


bool
ThreadHandler::HandleSingleStep(SingleStepEvent* event)
{
	return _HandleThreadStopped(event->GetCpuState());
}


bool
ThreadHandler::HandleExceptionOccurred(ExceptionOccurredEvent* event)
{
	return _HandleThreadStopped(NULL);
}


void
ThreadHandler::HandleThreadAction(uint32 action)
{
	AutoLocker<TeamDebugModel> locker(fDebugModel);

	if (fThread->State() == THREAD_STATE_UNKNOWN)
		return;

	// When stop is requested, thread must be running, otherwise stopped.
	if (action == MSG_THREAD_STOP
			? fThread->State() != THREAD_STATE_RUNNING
			: fThread->State() != THREAD_STATE_STOPPED) {
		return;
	}

	// When continuing the thread update thread state before actually issuing
	// the command, since we need to unlock.
	if (action != MSG_THREAD_STOP)
		_SetThreadState(THREAD_STATE_RUNNING, NULL);

	locker.Unlock();

	switch (action) {
		case MSG_THREAD_RUN:
printf("MSG_THREAD_RUN\n");
			fDebuggerInterface->ContinueThread(ThreadID());
			break;
		case MSG_THREAD_STOP:
printf("MSG_THREAD_STOP\n");
			fDebuggerInterface->StopThread(ThreadID());
			break;
		case MSG_THREAD_STEP_OVER:
printf("MSG_THREAD_STEP_OVER\n");
			fDebuggerInterface->SingleStepThread(ThreadID());
			break;
		case MSG_THREAD_STEP_INTO:
printf("MSG_THREAD_STEP_INTO\n");
			fDebuggerInterface->SingleStepThread(ThreadID());
			break;
		case MSG_THREAD_STEP_OUT:
printf("MSG_THREAD_STEP_OUT\n");
			fDebuggerInterface->SingleStepThread(ThreadID());
			break;

// TODO: Handle stepping correctly!
	}
}


void
ThreadHandler::HandleThreadStateChanged()
{
	AutoLocker<TeamDebugModel> locker(fDebugModel);

	// cancel jobs for this thread
	fWorker->AbortJob(JobKey(fThread, JOB_TYPE_GET_CPU_STATE));
	fWorker->AbortJob(JobKey(fThread, JOB_TYPE_GET_STACK_TRACE));

	// If the thread is stopped and has no CPU state yet, schedule a job.
	if (fThread->State() == THREAD_STATE_STOPPED
			&& fThread->GetCpuState() == NULL) {
		fWorker->ScheduleJob(
			new(std::nothrow) GetCpuStateJob(fDebuggerInterface, fThread));
	}
}


void
ThreadHandler::HandleCpuStateChanged()
{
	AutoLocker<TeamDebugModel> locker(fDebugModel);

	// cancel stack trace job for this thread
	fWorker->AbortJob(JobKey(fThread, JOB_TYPE_GET_STACK_TRACE));

	// If the thread has a CPU state, but no stack trace yet, schedule a job.
	if (fThread->GetCpuState() != NULL && fThread->GetStackTrace() == NULL) {
		fWorker->ScheduleJob(
			new(std::nothrow) GetStackTraceJob(fDebuggerInterface,
				fDebuggerInterface->GetArchitecture(), fThread));
	}
}


void
ThreadHandler::HandleStackTraceChanged()
{
printf("ThreadHandler::_HandleStackTraceChanged()\n");
}


bool
ThreadHandler::_HandleThreadStopped(CpuState* cpuState)
{
	AutoLocker<TeamDebugModel> locker(fDebugModel);

	_SetThreadState(THREAD_STATE_STOPPED, cpuState);

	return true;
}


void
ThreadHandler::_SetThreadState(uint32 state, CpuState* cpuState)
{
	fThread->SetState(state);
	fThread->SetCpuState(cpuState);
}
