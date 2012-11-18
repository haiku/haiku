/*
 * Copyright 2012, Rene Gollent, rene@gollent.com.
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "Jobs.h"

#include <AutoLocker.h>

#include "Architecture.h"
#include "CpuState.h"
#include "DebuggerInterface.h"
#include "ImageDebugInfo.h"
#include "StackTrace.h"
#include "Thread.h"
#include "Team.h"


GetStackTraceJob::GetStackTraceJob(DebuggerInterface* debuggerInterface,
	Architecture* architecture, Thread* thread)
	:
	fKey(thread, JOB_TYPE_GET_STACK_TRACE),
	fDebuggerInterface(debuggerInterface),
	fArchitecture(architecture),
	fThread(thread)
{
	fThread->AcquireReference();

	fCpuState = fThread->GetCpuState();
	if (fCpuState != NULL)
		fCpuState->AcquireReference();
}


GetStackTraceJob::~GetStackTraceJob()
{
	if (fCpuState != NULL)
		fCpuState->ReleaseReference();

	fThread->ReleaseReference();
}


const JobKey&
GetStackTraceJob::Key() const
{
	return fKey;
}


status_t
GetStackTraceJob::Do()
{
	if (fCpuState == NULL)
		return B_BAD_VALUE;

	// get the stack trace
	StackTrace* stackTrace;
	status_t error = fArchitecture->CreateStackTrace(fThread->GetTeam(), this,
		fCpuState, stackTrace);
	if (error != B_OK)
		return error;
	BReference<StackTrace> stackTraceReference(stackTrace, true);

	// set the stack trace, unless something has changed
	AutoLocker<Team> locker(fThread->GetTeam());

	if (fThread->GetCpuState() == fCpuState)
		fThread->SetStackTrace(stackTrace);

	return B_OK;
}


status_t
GetStackTraceJob::GetImageDebugInfo(Image* image, ImageDebugInfo*& _info)
{
	AutoLocker<Team> teamLocker(fThread->GetTeam());

	while (image->GetImageDebugInfo() == NULL) {
		// schedule a job, if not loaded
		ImageDebugInfo* info;
		status_t error = LoadImageDebugInfoJob::ScheduleIfNecessary(GetWorker(),
			image, &info);
		if (error != B_OK)
			return error;

		if (info != NULL) {
			_info = info;
			return B_OK;
		}

		teamLocker.Unlock();

		// wait for the job to finish
		switch (WaitFor(SimpleJobKey(image, JOB_TYPE_LOAD_IMAGE_DEBUG_INFO))) {
			case JOB_DEPENDENCY_SUCCEEDED:
			case JOB_DEPENDENCY_NOT_FOUND:
				// "Not found" can happen due to a race condition between
				// unlocking the worker and starting to wait.
				break;
			case JOB_DEPENDENCY_FAILED:
			case JOB_DEPENDENCY_ABORTED:
			default:
				return B_ERROR;
		}

		teamLocker.Lock();
	}

	_info = image->GetImageDebugInfo();
	_info->AcquireReference();

	return B_OK;
}
