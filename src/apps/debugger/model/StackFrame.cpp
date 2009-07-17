/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "StackFrame.h"

#include "CpuState.h"
#include "FunctionInstance.h"
#include "Image.h"
#include "Variable.h"


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
	fCpuState->AcquireReference();
}


StackFrame::~StackFrame()
{
	for (int32 i = 0; Variable* variable = fParameters.ItemAt(i); i++)
		variable->ReleaseReference();

	for (int32 i = 0; Variable* variable = fLocalVariables.ItemAt(i); i++)
		variable->ReleaseReference();

	SetImage(NULL);
	SetFunction(NULL);
	fCpuState->ReleaseReference();
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
		fImage->ReleaseReference();

	fImage = image;

	if (fImage != NULL)
		fImage->AcquireReference();
}


void
StackFrame::SetFunction(FunctionInstance* function)
{
	if (fFunction != NULL)
		fFunction->ReleaseReference();

	fFunction = function;

	if (fFunction != NULL)
		fFunction->AcquireReference();
}


int32
StackFrame::CountParameters() const
{
	return fParameters.CountItems();
}


Variable*
StackFrame::ParameterAt(int32 index) const
{
	return fParameters.ItemAt(index);
}


bool
StackFrame::AddParameter(Variable* parameter)
{
	if (!fParameters.AddItem(parameter))
		return false;

	parameter->AcquireReference();
	return true;
}


int32
StackFrame::CountLocalVariables() const
{
	return fLocalVariables.CountItems();
}


Variable*
StackFrame::LocalVariableAt(int32 index) const
{
	return fLocalVariables.ItemAt(index);
}


bool
StackFrame::AddLocalVariable(Variable* variable)
{
	if (!fLocalVariables.AddItem(variable))
		return false;

	variable->AcquireReference();
	return true;
}
