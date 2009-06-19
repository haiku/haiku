/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef STACK_FRAME_X86_H
#define STACK_FRAME_X86_H

#include "StackFrame.h"


class CpuStateX86;


class StackFrameX86 : public StackFrame {
public:
								StackFrameX86(stack_frame_type type,
									CpuStateX86* cpuState);
	virtual						~StackFrameX86();

			void				SetPreviousAddresses(
									target_addr_t previousFrameAddress,
									target_addr_t returnAddress);

	virtual	stack_frame_type	Type() const;

	virtual	CpuState*			GetCpuState() const;

	virtual	target_addr_t		InstructionPointer() const;
	virtual	target_addr_t		FrameAddress() const;
	virtual	target_addr_t		ReturnAddress() const;
	virtual	target_addr_t		PreviousFrameAddress() const;

private:
			stack_frame_type	fType;
			CpuStateX86*		fCpuState;
			target_addr_t		fPreviousFrameAddress;
			target_addr_t		fReturnAddress;
};


#endif	// STACK_FRAME_X86_H
