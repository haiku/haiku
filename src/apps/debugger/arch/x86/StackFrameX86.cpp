/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "StackFrameX86.h"

#include "CpuStateX86.h"


StackFrameX86::StackFrameX86(stack_frame_type type, CpuStateX86* cpuState)
	:
	fType(type),
	fCpuState(cpuState),
	fPreviousFrameAddress(0),
	fReturnAddress(0)
{
	fCpuState->AddReference();
}


StackFrameX86::~StackFrameX86()
{
	fCpuState->RemoveReference();
}


void
StackFrameX86::SetPreviousAddresses(target_addr_t previousFrameAddress,
	target_addr_t returnAddress)
{
	fPreviousFrameAddress = previousFrameAddress;
	fReturnAddress = returnAddress;
}


stack_frame_type
StackFrameX86::Type() const
{
	return fType;
}


CpuState*
StackFrameX86::GetCpuState() const
{
	return fCpuState;
}


target_addr_t
StackFrameX86::InstructionPointer() const
{
	return fCpuState->IntRegisterValue(X86_REGISTER_EIP);
}


target_addr_t
StackFrameX86::FrameAddress() const
{
	return fCpuState->IntRegisterValue(X86_REGISTER_EBP);
}


target_addr_t
StackFrameX86::ReturnAddress() const
{
	return fReturnAddress;
}


target_addr_t
StackFrameX86::PreviousFrameAddress() const
{
	return fPreviousFrameAddress;
}
