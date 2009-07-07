/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "StackFrame.h"

#include "CpuState.h"
#include "FunctionInstance.h"
#include "Image.h"


// #pragma mark - StackFrame


StackFrame::StackFrame(stack_frame_type type, CpuState* cpuState,
	target_addr_t frameAddress, target_addr_t instructionPointer)
	:
	fType(type),
	fCpuState(cpuState),
	fFrameAddress(frameAddress),
	fInstructionPointer(instructionPointer),
	fReturnAddress(0),
	fImage(NULL),
	fFunction(NULL)
{
	fCpuState->AddReference();
}


StackFrame::~StackFrame()
{
	SetImage(NULL);
	SetFunction(NULL);
	fCpuState->RemoveReference();
}


void
StackFrame::SetReturnAddress(target_addr_t address)
{
	fReturnAddress = address;
}


void
StackFrame::SetImage(Image* image)
{
	if (fImage != NULL)
		fImage->RemoveReference();

	fImage = image;

	if (fImage != NULL)
		fImage->AddReference();
}


void
StackFrame::SetFunction(FunctionInstance* function)
{
	if (fFunction != NULL)
		fFunction->RemoveReference();

	fFunction = function;

	if (fFunction != NULL)
		fFunction->AddReference();
}
