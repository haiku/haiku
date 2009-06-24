/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "Architecture.h"

#include <new>

#include <AutoDeleter.h>
#include <AutoLocker.h>

#include "CpuState.h"
#include "DebugInfo.h"
#include "FunctionDebugInfo.h"
#include "Image.h"
#include "ImageDebugInfo.h"
#include "ImageDebugInfoProvider.h"
#include "StackTrace.h"
#include "Team.h"


Architecture::Architecture(DebuggerInterface* debuggerInterface)
	:
	fDebuggerInterface(debuggerInterface)
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
		FunctionDebugInfo* function = NULL;
		if (imageDebugInfo != NULL)
			function = imageDebugInfo->FindFunction(instructionPointer);
		Reference<FunctionDebugInfo> functionReference(function, true);

		// create the frame using the debug info
		StackFrame* previousFrame = NULL;
		CpuState* previousCpuState = NULL;
		if (function != NULL) {
			status_t error = function->GetDebugInfo()->CreateFrame(image,
				function, cpuState, previousFrame, previousCpuState);
			if (error != B_OK && error != B_UNSUPPORTED)
				break;
		}

		// If we have no frame yet, let the architecture create it.
		if (previousFrame == NULL) {
			status_t error = CreateStackFrame(image, function, cpuState,
				frame == NULL, previousFrame, previousCpuState);
			if (error != B_OK)
				break;
		}

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
