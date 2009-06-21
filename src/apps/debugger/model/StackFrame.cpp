/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "StackFrame.h"

#include "CpuState.h"
#include "FunctionDebugInfo.h"
#include "Image.h"
#include "SourceCode.h"


// #pragma mark - StackFrame


StackFrame::StackFrame(stack_frame_type type, CpuState* cpuState,
	target_addr_t frameAddress)
	:
	fType(type),
	fCpuState(cpuState),
	fFrameAddress(frameAddress),
	fReturnAddress(0),
	fImage(NULL),
	fFunction(NULL),
	fSourceCode(NULL),
	fSourceCodeState(STACK_SOURCE_NOT_LOADED)
{
	fCpuState->AddReference();
}


StackFrame::~StackFrame()
{
	SetSourceCode(NULL, STACK_SOURCE_NOT_LOADED);
	SetImage(NULL);
	SetFunction(NULL);
	fCpuState->RemoveReference();
}


target_addr_t
StackFrame::InstructionPointer() const
{
	return fCpuState->InstructionPointer();
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
StackFrame::SetFunction(FunctionDebugInfo* function)
{
	if (fFunction != NULL)
		fFunction->RemoveReference();

	fFunction = function;

	if (fFunction != NULL)
		fFunction->AddReference();
}


void
StackFrame::SetSourceCode(SourceCode* source, stack_frame_source_state state)
{
	if (fSourceCode != NULL)
		fSourceCode->RemoveReference();

	fSourceCode = source;
	fSourceCodeState = state;

	if (fSourceCode != NULL)
		fSourceCode->AddReference();

	// notify listeners
	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->StackFrameSourceCodeChanged(this);
	}
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


// #pragma mark - Listener


StackFrame::Listener::~Listener()
{
}


void
StackFrame::Listener::StackFrameSourceCodeChanged(StackFrame* frame)
{
}
