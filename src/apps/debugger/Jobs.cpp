/*
 * Copyright 2012, Rene Gollent, rene@gollent.com.
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "Jobs.h"

#include <AutoLocker.h>
#include <memory_private.h>

#include "Architecture.h"
#include "BitBuffer.h"
#include "CpuState.h"
#include "DebuggerInterface.h"
#include "DisassembledCode.h"
#include "FileSourceCode.h"
#include "Function.h"
#include "Image.h"
#include "ImageDebugInfo.h"
#include "Register.h"
#include "SourceCode.h"
#include "SpecificImageDebugInfo.h"
#include "StackFrameDebugInfo.h"
#include "StackFrameValueInfos.h"
#include "StackFrameValues.h"
#include "StackTrace.h"
#include "Team.h"
#include "TeamDebugInfo.h"
#include "TeamMemory.h"
#include "TeamMemoryBlock.h"
#include "TeamTypeInformation.h"
#include "Thread.h"
#include "Tracing.h"
#include "Type.h"
#include "TypeComponentPath.h"
#include "Value.h"
#include "ValueLoader.h"
#include "ValueLocation.h"
#include "ValueNode.h"
#include "ValueNodeContainer.h"
#include "Variable.h"


// #pragma mark - GetThreadStateJob


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


// #pragma mark - GetCpuStateJob


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


// #pragma mark - GetStackTraceJob


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


// #pragma mark - LoadImageDebugInfoJob


LoadImageDebugInfoJob::LoadImageDebugInfoJob(Image* image)
	:
	fKey(image, JOB_TYPE_LOAD_IMAGE_DEBUG_INFO),
	fImage(image)
{
	fImage->AcquireReference();
}


LoadImageDebugInfoJob::~LoadImageDebugInfoJob()
{
	fImage->ReleaseReference();
}


const JobKey&
LoadImageDebugInfoJob::Key() const
{
	return fKey;
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
		debugInfo->ReleaseReference();
	} else
		fImage->SetImageDebugInfo(NULL, IMAGE_DEBUG_INFO_UNAVAILABLE);

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
			(*_imageDebugInfo)->AcquireReference();
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
	Team* team, FunctionInstance* functionInstance, bool loadForFunction)
	:
	fKey(functionInstance, JOB_TYPE_LOAD_SOURCE_CODE),
	fDebuggerInterface(debuggerInterface),
	fArchitecture(architecture),
	fTeam(team),
	fFunctionInstance(functionInstance),
	fLoadForFunction(loadForFunction)
{
	fFunctionInstance->AcquireReference();
}


LoadSourceCodeJob::~LoadSourceCodeJob()
{
	fFunctionInstance->ReleaseReference();
}


const JobKey&
LoadSourceCodeJob::Key() const
{
	return fKey;
}


status_t
LoadSourceCodeJob::Do()
{
	// if requested, try loading the source code for the function
	Function* function = fFunctionInstance->GetFunction();
	if (fLoadForFunction) {
		FileSourceCode* sourceCode;
		status_t error = fTeam->DebugInfo()->LoadSourceCode(
			function->SourceFile(), sourceCode);

		AutoLocker<Team> locker(fTeam);

		if (error == B_OK) {
			function->SetSourceCode(sourceCode, FUNCTION_SOURCE_LOADED);
			sourceCode->ReleaseReference();
			return B_OK;
		}

		function->SetSourceCode(NULL, FUNCTION_SOURCE_UNAVAILABLE);
	}

	// Only try to load the function instance code, if it's not overridden yet.
	AutoLocker<Team> locker(fTeam);
	if (fFunctionInstance->SourceCodeState() != FUNCTION_SOURCE_LOADING)
		return B_OK;
	locker.Unlock();

	// disassemble the function
	DisassembledCode* sourceCode = NULL;
	status_t error = fTeam->DebugInfo()->DisassembleFunction(fFunctionInstance,
		sourceCode);

	// set the result
	locker.Lock();
	if (error == B_OK) {
		if (fFunctionInstance->SourceCodeState() == FUNCTION_SOURCE_LOADING) {
			fFunctionInstance->SetSourceCode(sourceCode,
				FUNCTION_SOURCE_LOADED);
			sourceCode->ReleaseReference();
		}
	} else
		fFunctionInstance->SetSourceCode(NULL, FUNCTION_SOURCE_UNAVAILABLE);

	return error;
}


// #pragma mark - ResolveValueNodeValueJob


ResolveValueNodeValueJob::ResolveValueNodeValueJob(
	DebuggerInterface* debuggerInterface, Architecture* architecture,
	CpuState* cpuState, TeamTypeInformation* typeInformation,
	ValueNodeContainer* container, ValueNode* valueNode)
	:
	fKey(valueNode, JOB_TYPE_RESOLVE_VALUE_NODE_VALUE),
	fDebuggerInterface(debuggerInterface),
	fArchitecture(architecture),
	fCpuState(cpuState),
	fTypeInformation(typeInformation),
	fContainer(container),
	fValueNode(valueNode)
{
	if (fCpuState != NULL)
		fCpuState->AcquireReference();
	fContainer->AcquireReference();
	fValueNode->AcquireReference();
}


ResolveValueNodeValueJob::~ResolveValueNodeValueJob()
{
	if (fCpuState != NULL)
		fCpuState->ReleaseReference();
	fContainer->ReleaseReference();
	fValueNode->ReleaseReference();
}


const JobKey&
ResolveValueNodeValueJob::Key() const
{
	return fKey;
}


status_t
ResolveValueNodeValueJob::Do()
{
	// check whether the node still belongs to the container
	AutoLocker<ValueNodeContainer> containerLocker(fContainer);
	if (fValueNode->Container() != fContainer)
		return B_BAD_VALUE;

	// if already resolved, we're done
	status_t nodeResolutionState
		= fValueNode->LocationAndValueResolutionState();
	if (nodeResolutionState != VALUE_NODE_UNRESOLVED)
		return nodeResolutionState;

	containerLocker.Unlock();

	// resolve
	status_t error = _ResolveNodeValue();
	if (error != B_OK) {
		nodeResolutionState = fValueNode->LocationAndValueResolutionState();
		if (nodeResolutionState != VALUE_NODE_UNRESOLVED)
			return nodeResolutionState;

		containerLocker.Lock();
		fValueNode->SetLocationAndValue(NULL, NULL, error);
		containerLocker.Unlock();
	}

	return error;
}


status_t
ResolveValueNodeValueJob::_ResolveNodeValue()
{
	// get the node child and parent node
	AutoLocker<ValueNodeContainer> containerLocker(fContainer);
	ValueNodeChild* nodeChild = fValueNode->NodeChild();
	BReference<ValueNodeChild> nodeChildReference(nodeChild);

	ValueNode* parentNode = nodeChild->Parent();
	BReference<ValueNode> parentNodeReference(parentNode);

	// Check whether the node child location has been resolved already
	// (successfully).
	status_t nodeChildResolutionState = nodeChild->LocationResolutionState();
	bool nodeChildDone = nodeChildResolutionState != VALUE_NODE_UNRESOLVED;
	if (nodeChildDone && nodeChildResolutionState != B_OK)
		return nodeChildResolutionState;

	// If the child node location has not been resolved yet, check whether the
	// parent node location and value have been resolved already (successfully).
	bool parentDone = true;
	if (!nodeChildDone && parentNode != NULL) {
		status_t parentResolutionState
			= parentNode->LocationAndValueResolutionState();
		parentDone = parentResolutionState != VALUE_NODE_UNRESOLVED;
		if (parentDone && parentResolutionState != B_OK)
			return parentResolutionState;
	}

	containerLocker.Unlock();

	// resolve the parent node location and value, if necessary
	if (!parentDone) {
		status_t error = _ResolveParentNodeValue(parentNode);
		if (error != B_OK) {
			TRACE_LOCALS("ResolveValueNodeValueJob::_ResolveNodeValue(): value "
				"node: %p (\"%s\"): _ResolveParentNodeValue(%p) failed\n",
				fValueNode, fValueNode->Name().String(), parentNode);
			return error;
		}

		if (State() == JOB_STATE_WAITING)
			return B_OK;
	}

	// resolve the node child location, if necessary
	if (!nodeChildDone) {
		status_t error = _ResolveNodeChildLocation(nodeChild);
		if (error != B_OK) {
			TRACE_LOCALS("ResolveValueNodeValueJob::_ResolveNodeValue(): value "
				"node: %p (\"%s\"): _ResolveNodeChildLocation(%p) failed\n",
				fValueNode, fValueNode->Name().String(), nodeChild);
			return error;
		}
	}

	// resolve the node location and value
	ValueLoader valueLoader(fArchitecture, fDebuggerInterface,
		fTypeInformation, fCpuState);
	ValueLocation* location;
	Value* value;
	status_t error = fValueNode->ResolvedLocationAndValue(&valueLoader,
		location, value);
	if (error != B_OK) {
		TRACE_LOCALS("ResolveValueNodeValueJob::_ResolveNodeValue(): value "
			"node: %p (\"%s\"): fValueNode->ResolvedLocationAndValue() "
			"failed\n", fValueNode, fValueNode->Name().String());
		return error;
	}
	BReference<ValueLocation> locationReference(location, true);
	BReference<Value> valueReference(value, true);

	// set location and value on the node
	containerLocker.Lock();
	status_t nodeResolutionState
		= fValueNode->LocationAndValueResolutionState();
	if (nodeResolutionState != VALUE_NODE_UNRESOLVED)
		return nodeResolutionState;
	fValueNode->SetLocationAndValue(location, value, B_OK);
	containerLocker.Unlock();

	return B_OK;
}


status_t
ResolveValueNodeValueJob::_ResolveNodeChildLocation(ValueNodeChild* nodeChild)
{
	// resolve the location
	ValueLoader valueLoader(fArchitecture, fDebuggerInterface,
		fTypeInformation, fCpuState);
	ValueLocation* location = NULL;
	status_t error = nodeChild->ResolveLocation(&valueLoader, location);
	BReference<ValueLocation> locationReference(location, true);

	// set the location on the node child
	AutoLocker<ValueNodeContainer> containerLocker(fContainer);
	status_t nodeChildResolutionState = nodeChild->LocationResolutionState();
	if (nodeChildResolutionState == VALUE_NODE_UNRESOLVED)
		nodeChild->SetLocation(location, error);
	else
		error = nodeChildResolutionState;

	return error;
}


status_t
ResolveValueNodeValueJob::_ResolveParentNodeValue(ValueNode* parentNode)
{
	AutoLocker<ValueNodeContainer> containerLocker(fContainer);

	if (parentNode->Container() != fContainer)
		return B_BAD_VALUE;

	// if the parent node already has a value, we're done
	status_t nodeResolutionState
		= parentNode->LocationAndValueResolutionState();
	if (nodeResolutionState != VALUE_NODE_UNRESOLVED)
		return nodeResolutionState;

	// check whether a job is already in progress
	AutoLocker<Worker> workerLocker(GetWorker());
	SimpleJobKey jobKey(parentNode, JOB_TYPE_RESOLVE_VALUE_NODE_VALUE);
	if (GetWorker()->GetJob(jobKey) == NULL) {
		workerLocker.Unlock();

		// schedule the job
		status_t error = GetWorker()->ScheduleJob(
			new(std::nothrow) ResolveValueNodeValueJob(fDebuggerInterface,
				fArchitecture, fCpuState, fTypeInformation, fContainer,
				parentNode));
		if (error != B_OK) {
			// scheduling failed -- set the value to invalid
			parentNode->SetLocationAndValue(NULL, NULL, error);
			return error;
		}
	}

	// wait for the job to finish
	workerLocker.Unlock();
	containerLocker.Unlock();

	switch (WaitFor(jobKey)) {
		case JOB_DEPENDENCY_SUCCEEDED:
		case JOB_DEPENDENCY_NOT_FOUND:
			// "Not found" can happen due to a race condition between
			// unlocking the worker and starting to wait.
			break;
		case JOB_DEPENDENCY_ACTIVE:
			return B_OK;
		case JOB_DEPENDENCY_FAILED:
		case JOB_DEPENDENCY_ABORTED:
		default:
			return B_ERROR;
	}

	containerLocker.Lock();

	// now there should be a value for the node
	nodeResolutionState = parentNode->LocationAndValueResolutionState();
	return nodeResolutionState != VALUE_NODE_UNRESOLVED
		? nodeResolutionState : B_ERROR;
}


RetrieveMemoryBlockJob::RetrieveMemoryBlockJob(Team* team,
	TeamMemory* teamMemory, TeamMemoryBlock* memoryBlock)
	:
	fKey(memoryBlock, JOB_TYPE_GET_MEMORY_BLOCK),
	fTeam(team),
	fTeamMemory(teamMemory),
	fMemoryBlock(memoryBlock)
{
	fTeamMemory->AcquireReference();
	fMemoryBlock->AcquireReference();
}


RetrieveMemoryBlockJob::~RetrieveMemoryBlockJob()
{
	fTeamMemory->ReleaseReference();
	fMemoryBlock->ReleaseReference();
}


const JobKey&
RetrieveMemoryBlockJob::Key() const
{
	return fKey;
}


status_t
RetrieveMemoryBlockJob::Do()
{
	ssize_t result = fTeamMemory->ReadMemory(fMemoryBlock->BaseAddress(),
		fMemoryBlock->Data(), fMemoryBlock->Size());
	if (result < 0)
		return result;

	uint32 protection = 0;
	uint32 locking = 0;
	status_t error = get_memory_properties(fTeam->ID(),
		(const void *)fMemoryBlock->BaseAddress(), &protection, &locking);
	if (error != B_OK)
		return error;

	fMemoryBlock->SetWritable((protection & B_WRITE_AREA) != 0);
	fMemoryBlock->MarkValid();
	return B_OK;
}
