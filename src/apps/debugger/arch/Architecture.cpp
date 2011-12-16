/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "Architecture.h"

#include <new>

#include <AutoDeleter.h>
#include <AutoLocker.h>

#include "CfaContext.h"
#include "CpuState.h"
#include "FunctionInstance.h"
#include "Image.h"
#include "ImageDebugInfo.h"
#include "ImageDebugInfoProvider.h"
#include "Register.h"
#include "RegisterMap.h"
#include "SpecificImageDebugInfo.h"
#include "StackTrace.h"
#include "Team.h"


Architecture::Architecture(TeamMemory* teamMemory, uint8 addressSize,
	bool bigEndian)
	:
	fTeamMemory(teamMemory),
	fAddressSize(addressSize),
	fBigEndian(bigEndian)
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
Architecture::InitRegisterRules(CfaContext& context) const
{
	// Init the initial register rules. The DWARF 3 specs on the
	// matter: "The default rule for all columns before
	// interpretation of the initial instructions is the undefined
	// rule. However, an ABI authoring body or a compilation system
	// authoring body may specify an alternate default value for any
	// or all columns."
	// GCC's assumes the "same value" rule for all callee preserved
	// registers. We set them respectively.
	// the stack pointer is initialized to
	// CFA offset 0 by default.
	const Register* registers = Registers();
	RegisterMap* toDwarf = NULL;
	status_t result = GetDwarfRegisterMaps(&toDwarf, NULL);
	if (result != B_OK)
		return result;

	BReference<RegisterMap> toDwarfMapReference(toDwarf, true);
	for (int32 i = 0; i < CountRegisters(); i++) {
		int32 dwarfReg = toDwarf->MapRegisterIndex(i);
		if (dwarfReg < 0 || dwarfReg > CountRegisters() - 1)
			continue;

		// TODO: on CPUs that have a return address register
		// a default rule should be set up to use that to
		// extract the instruction pointer
		switch (registers[i].Type()) {
			case REGISTER_TYPE_STACK_POINTER:
			{
				context.RegisterRule(dwarfReg)->SetToValueOffset(0);
				break;
			}
			default:
			{
				context.RegisterRule(dwarfReg)->SetToSameValue();
				break;
			}
		}
	}

	return result;
}


status_t
Architecture::CreateStackTrace(Team* team,
	ImageDebugInfoProvider* imageInfoProvider, CpuState* cpuState,
	StackTrace*& _stackTrace, int32 maxStackDepth, bool useExistingTrace)
{
	BReference<CpuState> cpuStateReference(cpuState);

	StackTrace* stackTrace = NULL;
	ObjectDeleter<StackTrace> stackTraceDeleter;
	StackFrame* nextFrame = NULL;

	if (useExistingTrace)
		stackTrace = _stackTrace;
	else {
		// create the object
		stackTrace = new(std::nothrow) StackTrace;
		if (stackTrace == NULL)
			return B_NO_MEMORY;
		stackTraceDeleter.SetTo(stackTrace);
	}

	// if we're passed an already existing partial stack trace,
	// attempt to continue building it from where it left off.
	if (stackTrace->CountFrames() > 0) {
		nextFrame = stackTrace->FrameAt(stackTrace->CountFrames() - 1);
		cpuState = nextFrame->GetPreviousCpuState();
	}

	while (cpuState != NULL) {
		// get the instruction pointer
		target_addr_t instructionPointer = cpuState->InstructionPointer();
		if (instructionPointer == 0)
			break;

		// get the image for the instruction pointer
		AutoLocker<Team> teamLocker(team);
		Image* image = team->ImageByAddress(instructionPointer);
		BReference<Image> imageReference(image);
		teamLocker.Unlock();

		// get the image debug info
		ImageDebugInfo* imageDebugInfo = NULL;
		if (image != NULL)
			imageInfoProvider->GetImageDebugInfo(image, imageDebugInfo);
		BReference<ImageDebugInfo> imageDebugInfoReference(imageDebugInfo,
			true);

		// get the function
		teamLocker.Lock();
		FunctionInstance* function = NULL;
		FunctionDebugInfo* functionDebugInfo = NULL;
		if (imageDebugInfo != NULL) {
			function = imageDebugInfo->FunctionAtAddress(instructionPointer);
			if (function != NULL)
				functionDebugInfo = function->GetFunctionDebugInfo();
		}
		BReference<FunctionInstance> functionReference(function);
		teamLocker.Unlock();

		// If the CPU state's instruction pointer is actually the return address
		// of the next frame, we let the architecture fix that.
		if (nextFrame != NULL
			&& nextFrame->ReturnAddress() == cpuState->InstructionPointer()) {
			UpdateStackFrameCpuState(nextFrame, image,
				functionDebugInfo, cpuState);
		}

		// create the frame using the debug info
		StackFrame* frame = NULL;
		CpuState* previousCpuState = NULL;
		if (function != NULL) {
			status_t error = functionDebugInfo->GetSpecificImageDebugInfo()
				->CreateFrame(image, function, cpuState, frame,
					previousCpuState);
			if (error != B_OK && error != B_UNSUPPORTED)
				break;
		}

		// If we have no frame yet, let the architecture create it.
		if (frame == NULL) {
			status_t error = CreateStackFrame(image, functionDebugInfo,
				cpuState, nextFrame == NULL, frame, previousCpuState);
			if (error != B_OK)
				break;
		}

		cpuStateReference.SetTo(previousCpuState, true);

		frame->SetImage(image);
		frame->SetFunction(function);

		if (!stackTrace->AddFrame(frame)) {
			delete frame;
			return B_NO_MEMORY;
		}

		frame = nextFrame;
		cpuState = previousCpuState;
		if (--maxStackDepth == 0)
			break;
	}

	stackTraceDeleter.Detach();
	_stackTrace = stackTrace;
	return B_OK;
}
