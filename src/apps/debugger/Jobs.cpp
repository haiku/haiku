/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "Jobs.h"

#include <new>

#include <AutoLocker.h>

#include "Architecture.h"
#include "CpuState.h"
#include "DebuggerInterface.h"
#include "Image.h"
#include "ImageDebugInfo.h"
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
	status_t error = fArchitecture->CreateStackTrace(fThread->GetTeam(), this,
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


status_t
GetStackTraceJob::GetImageDebugInfo(Image* image, ImageDebugInfo*& _info)
{
	AutoLocker<Team> teamLocker(fThread->GetTeam());

	while (image->GetImageDebugInfo() == NULL) {
		// The info has not yet been loaded -- check whether a job has already
		// been scheduled.
		AutoLocker<Worker> workerLocker(GetWorker());
		JobKey key(image, JOB_TYPE_LOAD_IMAGE_DEBUG_INFO);
		Job* loadImageDebugInfoJob = GetWorker()->GetJob(key);
		if (loadImageDebugInfoJob == NULL) {
			// no job yet -- schedule one
			loadImageDebugInfoJob = new(std::nothrow) LoadImageDebugInfoJob(
				fDebuggerInterface, fArchitecture, image);
			if (loadImageDebugInfoJob == NULL)
				return B_NO_MEMORY;

			status_t error = GetWorker()->ScheduleJob(loadImageDebugInfoJob);
			if (error != B_OK)
				return error;
		}

		workerLocker.Unlock();
		teamLocker.Unlock();

		// wait for the job to finish
		switch (WaitFor(key)) {
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
	_info->AddReference();

	return B_OK;
}


// #pragma mark - GetStackTraceJob


LoadImageDebugInfoJob::LoadImageDebugInfoJob(
	DebuggerInterface* debuggerInterface, Architecture* architecture,
	Image* image)
	:
	fDebuggerInterface(debuggerInterface),
	fArchitecture(architecture),
	fImage(image)
{
	fImage->AddReference();
}


LoadImageDebugInfoJob::~LoadImageDebugInfoJob()
{
	fImage->RemoveReference();
}


JobKey
LoadImageDebugInfoJob::Key() const
{
	return JobKey(fImage, JOB_TYPE_LOAD_IMAGE_DEBUG_INFO);
}


status_t
LoadImageDebugInfoJob::Do()
{
	// get an image info for the image
	AutoLocker<Team> locker(fImage->GetTeam());
	ImageInfo imageInfo(fImage->Info());
	locker.Unlock();

	// create the debug info
	ImageDebugInfo* debugInfo = new(std::nothrow) ImageDebugInfo(imageInfo,
		fDebuggerInterface, fArchitecture);
	if (debugInfo == NULL)
		return B_NO_MEMORY;

	status_t error = debugInfo->Init();
	if (error != B_OK) {
		delete debugInfo;
		return error;
	}

	// set the info
	locker.Lock();
	fImage->SetImageDebugInfo(debugInfo);

	return B_OK;
}
