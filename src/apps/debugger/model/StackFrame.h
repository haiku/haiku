/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef STACK_FRAME_H
#define STACK_FRAME_H

#include <OS.h>

#include <Referenceable.h>

#include "Types.h"


enum stack_frame_type {
	STACK_FRAME_TYPE_SYSCALL,		// syscall frame
	STACK_FRAME_TYPE_STANDARD,		// standard frame
	STACK_FRAME_TYPE_SIGNAL,		// signal handler frame
	STACK_FRAME_TYPE_FRAMELESS		// dummy frame for a frameless function
};


class CpuState;
class Image;
class FunctionInstance;


class StackFrame : public Referenceable {
public:
								StackFrame(stack_frame_type type,
									CpuState* cpuState,
									target_addr_t frameAddress,
									target_addr_t instructionPointer);
								~StackFrame();

			stack_frame_type	Type() const			{ return fType; }
			CpuState*			GetCpuState() const		{ return fCpuState; }
			target_addr_t		FrameAddress() const { return fFrameAddress; }

			target_addr_t		InstructionPointer() const
									{ return fInstructionPointer; }

			target_addr_t		ReturnAddress() const { return fReturnAddress; }
			void				SetReturnAddress(target_addr_t address);

			Image*				GetImage() const		{ return fImage; }
			void				SetImage(Image* image);

			FunctionInstance*	Function() const		{ return fFunction; }
			void				SetFunction(FunctionInstance* function);

private:
			stack_frame_type	fType;
			CpuState*			fCpuState;
			target_addr_t		fFrameAddress;
			target_addr_t		fInstructionPointer;
			target_addr_t		fReturnAddress;
			Image*				fImage;
			FunctionInstance*	fFunction;
};


#endif	// STACK_FRAME_H
