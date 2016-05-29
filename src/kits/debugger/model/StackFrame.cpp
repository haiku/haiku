/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "StackFrame.h"

#include <new>

#include "CpuState.h"
#include "FunctionInstance.h"
#include "Image.h"
#include "StackFrameDebugInfo.h"
#include "StackFrameValueInfos.h"
#include "StackFrameValues.h"
#include "Variable.h"


// #pragma mark - StackFrame


StackFrame::StackFrame(stack_frame_type type, CpuState* cpuState,
	target_addr_t frameAddress, target_addr_t instructionPointer,
	StackFrameDebugInfo* debugInfo)
	:
	fType(type),
	fCpuState(cpuState),
	fPreviousCpuState(NULL),
	fFrameAddress(frameAddress),
	fInstructionPointer(instructionPointer),
	fReturnAddress(0),
	fDebugInfo(debugInfo),
	fImage(NULL),
	fFunction(NULL),
	fValues(NULL),
	fValueInfos(NULL)
{
	fCpuState->AcquireReference();
	fDebugInfo->AcquireReference();
}


StackFrame::~StackFrame()
{
	for (int32 i = 0; Variable* variable = fParameters.ItemAt(i); i++)
		variable->ReleaseReference();

	for (int32 i = 0; Variable* variable = fLocalVariables.ItemAt(i); i++)
		variable->ReleaseReference();

	SetImage(NULL);
	SetFunction(NULL);
	SetPreviousCpuState(NULL);

	fDebugInfo->ReleaseReference();
	fCpuState->ReleaseReference();
}


status_t
StackFrame::Init()
{
	// create values map
	fValues = new(std::nothrow) StackFrameValues;
	if (fValues == NULL)
		return B_NO_MEMORY;

	status_t error = fValues->Init();
	if (error != B_OK)
		return error;

	// create value infos map
	fValueInfos = new(std::nothrow) StackFrameValueInfos;
	if (fValueInfos == NULL)
		return B_NO_MEMORY;

	error = fValueInfos->Init();
	if (error != B_OK)
		return error;

	return B_OK;
}


void
StackFrame::SetPreviousCpuState(CpuState* state)
{
	if (fPreviousCpuState != NULL)
		fPreviousCpuState->ReleaseReference();

	fPreviousCpuState = state;

	if (fPreviousCpuState != NULL)
		fPreviousCpuState->AcquireReference();
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


void
StackFrame::AddListener(Listener* listener)
{
	fListeners.Add(listener);
}


void
StackFrame::RemoveListener(Listener* listener)
{
	fListeners.Remove(listener);
}


void
StackFrame::NotifyValueRetrieved(Variable* variable, TypeComponentPath* path)
{
	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->StackFrameValueRetrieved(this, variable, path);
	}
}


// #pragma mark - StackFrame


StackFrame::Listener::~Listener()
{
}


void
StackFrame::Listener::StackFrameValueRetrieved(StackFrame* stackFrame,
	Variable* variable, TypeComponentPath* path)
{
}
