/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "Architecture.h"

#include <new>

#include <AutoDeleter.h>
#include <AutoLocker.h>

#include "CpuState.h"
#include "FunctionInstance.h"
#include "Image.h"
#include "ImageDebugInfo.h"
#include "ImageDebugInfoProvider.h"
#include "SpecificImageDebugInfo.h"
#include "StackTrace.h"
#include "Team.h"


Architecture::Architecture(TeamMemory* teamMemory)
	:
	fTeamMemory(teamMemory)
{
}


Architecture::~Architecture()
{
}


status_t
Architecture::Init()
{
	return B_OK;
}


status_t
Architecture::CreateStackTrace(Team* team,
	ImageDebugInfoProvider* imageInfoProvider, CpuState* cpuState,
	StackTrace*& _stackTrace)
{
	Reference<CpuState> cpuStateReference(cpuState);

	// create the object
	StackTrace* stackTrace = new(std::nothrow) StackTrace;
	if (stackTrace == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<StackTrace> stackTraceDeleter(stackTrace);

	bool architectureFrame = false;
	StackFrame* frame = NULL;

	while (cpuState != NULL) {
		// get the instruction pointer
		target_addr_t instructionPointer = cpuState->InstructionPointer();
		if (instructionPointer == 0)
			break;

		// get the image for the instruction pointer
		AutoLocker<Team> teamLocker(team);
		Image* image = team->ImageByAddress(instructionPointer);
		Reference<Image> imageReference(image);
		teamLocker.Unlock();

		// get the image debug info
		ImageDebugInfo* imageDebugInfo = NULL;
		if (image != NULL)
			imageInfoProvider->GetImageDebugInfo(image, imageDebugInfo);
		Reference<ImageDebugInfo> imageDebugInfoReference(imageDebugInfo, true);

		// get the function
		teamLocker.Lock();
		FunctionInstance* function = NULL;
		FunctionDebugInfo* functionDebugInfo = NULL;
		if (imageDebugInfo != NULL) {
			function = imageDebugInfo->FunctionAtAddress(instructionPointer);
			if (function != NULL)
				functionDebugInfo = function->GetFunctionDebugInfo();
		}
		Reference<FunctionInstance> functionReference(function);
		teamLocker.Unlock();

		// If the last frame had been created by the architecture, we update the
		// CPU state.
		if (architectureFrame) {
			UpdateStackFrameCpuState(frame, image,
				functionDebugInfo, cpuState);
		}

		// create the frame using the debug info
		StackFrame* previousFrame = NULL;
		CpuState* previousCpuState = NULL;
		if (function != NULL) {
			status_t error = functionDebugInfo->GetSpecificImageDebugInfo()
				->CreateFrame(image, functionDebugInfo, cpuState, previousFrame,
					previousCpuState);
			if (error != B_OK && error != B_UNSUPPORTED)
				break;
		}

		// If we have no frame yet, let the architecture create it.
		if (previousFrame == NULL) {
			status_t error = CreateStackFrame(image, functionDebugInfo,
				cpuState, frame == NULL, previousFrame, previousCpuState);
			if (error != B_OK)
				break;
			architectureFrame = true;
		} else
			architectureFrame = false;

		cpuStateReference.SetTo(previousCpuState, true);

		previousFrame->SetImage(image);
		previousFrame->SetFunction(function);

		if (!stackTrace->AddFrame(previousFrame))
			return B_NO_MEMORY;

		frame = previousFrame;
		cpuState = previousCpuState;
	}

	stackTraceDeleter.Detach();
	_stackTrace = stackTrace;
	return B_OK;
}
