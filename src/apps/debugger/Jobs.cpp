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
#include "Function.h"
#include "Image.h"
#include "ImageDebugInfo.h"
#include "SourceCode.h"
#include "SpecificImageDebugInfo.h"
#include "StackTrace.h"
#include "Team.h"
#include "TeamDebugInfo.h"
#include "Thread.h"


// #pragma mark - GetThreadStateJob


GetThreadStateJob::GetThreadStateJob(DebuggerInterface* debuggerInterface,
	Thread* thread)
	:
	fDebuggerInterface(debuggerInterface),
	fThread(thread)
{
	fThread->AddReference();
}


GetThreadStateJob::~GetThreadStateJob()
{
	fThread->RemoveReference();
}


JobKey
GetThreadStateJob::Key() const
{
	return JobKey(fThread, JOB_TYPE_GET_THREAD_STATE);
}


status_t
GetThreadStateJob::Do()
{
	CpuState* state = NULL;
	status_t error = fDebuggerInterface->GetCpuState(fThread->ID(), state);
	Reference<CpuState> reference(state, true);

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
		switch (WaitFor(JobKey(image, JOB_TYPE_LOAD_IMAGE_DEBUG_INFO))) {
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


// #pragma mark - LoadImageDebugInfoJob


LoadImageDebugInfoJob::LoadImageDebugInfoJob(Image* image)
	:
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
	ImageDebugInfo* debugInfo;
	status_t error = fImage->GetTeam()->DebugInfo()->LoadImageDebugInfo(
		imageInfo, fImage->ImageFile(), debugInfo);

	// set the result
	locker.Lock();
	if (error == B_OK) {
		error = fImage->SetImageDebugInfo(debugInfo, IMAGE_DEBUG_INFO_LOADED);
		debugInfo->RemoveReference();
	} else {
		fImage->SetImageDebugInfo(NULL, IMAGE_DEBUG_INFO_UNAVAILABLE);
		delete debugInfo;
	}

	return error;
}


/*static*/ status_t
LoadImageDebugInfoJob::ScheduleIfNecessary(Worker* worker, Image* image,
	ImageDebugInfo** _imageDebugInfo)
{
	AutoLocker<Team> teamLocker(image->GetTeam());

	// If already loaded, we're done.
	if (image->GetImageDebugInfo() != NULL) {
		if (_imageDebugInfo != NULL) {
			*_imageDebugInfo = image->GetImageDebugInfo();
			(*_imageDebugInfo)->AddReference();
		}
		return B_OK;
	}

	// If already loading, the caller has to wait, if desired.
	if (image->ImageDebugInfoState() == IMAGE_DEBUG_INFO_LOADING) {
		if (_imageDebugInfo != NULL)
			*_imageDebugInfo = NULL;
		return B_OK;
	}

	// If an earlier load attempt failed, bail out.
	if (image->ImageDebugInfoState() != IMAGE_DEBUG_INFO_NOT_LOADED)
		return B_ERROR;

	// schedule a job
	LoadImageDebugInfoJob* job = new(std::nothrow) LoadImageDebugInfoJob(image);
	if (job == NULL)
		return B_NO_MEMORY;

	status_t error = worker->ScheduleJob(job);
	if (error != B_OK) {
		image->SetImageDebugInfo(NULL, IMAGE_DEBUG_INFO_UNAVAILABLE);
		return error;
	}

	image->SetImageDebugInfo(NULL, IMAGE_DEBUG_INFO_LOADING);

	if (_imageDebugInfo != NULL)
		*_imageDebugInfo = NULL;
	return B_OK;
}


// #pragma mark - LoadSourceCodeJob


LoadSourceCodeJob::LoadSourceCodeJob(
	DebuggerInterface* debuggerInterface, Architecture* architecture,
	Team* team, Function* function)
	:
	fDebuggerInterface(debuggerInterface),
	fArchitecture(architecture),
	fTeam(team),
	fFunction(function)
{
	fFunction->AddReference();
}


LoadSourceCodeJob::~LoadSourceCodeJob()
{
	fFunction->RemoveReference();
}


JobKey
LoadSourceCodeJob::Key() const
{
	return JobKey(fFunction, JOB_TYPE_LOAD_SOURCE_CODE);
}


status_t
LoadSourceCodeJob::Do()
{
	// Get the function debug info for an instance which we can use to load the
	// source code.
	AutoLocker<Team> locker(fTeam);

	status_t error = B_OK;
	FunctionDebugInfo* functionDebugInfo = NULL;
	if (FunctionInstance* instance = fFunction->FirstInstance()) {
		functionDebugInfo = instance->GetFunctionDebugInfo();
	} else
		error = B_ENTRY_NOT_FOUND;
	Reference<FunctionDebugInfo> functionDebugInfoReference(functionDebugInfo);

	locker.Unlock();

	// load the source code, if we can
	SourceCode* sourceCode = NULL;
	if (error == B_OK) {
		error = functionDebugInfo->GetSpecificImageDebugInfo()->LoadSourceCode(
			functionDebugInfo, sourceCode);
	}

	// set the result
	locker.Lock();
	if (error == B_OK) {
		fFunction->SetSourceCode(sourceCode, FUNCTION_SOURCE_LOADED);
		sourceCode->RemoveReference();
	} else
		fFunction->SetSourceCode(NULL, FUNCTION_SOURCE_UNAVAILABLE);

	return error;
}
