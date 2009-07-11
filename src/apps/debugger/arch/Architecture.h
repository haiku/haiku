/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef ARCHITECTURE_H
#define ARCHITECTURE_H

#include <OS.h>

#include <Referenceable.h>

#include "Types.h"


class CpuState;
class DisassembledCode;
class FunctionDebugInfo;
class Image;
class ImageDebugInfoProvider;
class InstructionInfo;
class Register;
class StackFrame;
class StackTrace;
class Statement;
class Team;
class TeamMemory;


class Architecture : public Referenceable {
public:
								Architecture(TeamMemory* teamMemory);
	virtual						~Architecture();

	virtual	status_t			Init();

	virtual	int32				CountRegisters() const = 0;
	virtual	const Register*		Registers() const = 0;

	virtual	status_t			CreateCpuState(const void* cpuStateData,
									size_t size, CpuState*& _state) = 0;
	virtual	status_t			CreateStackFrame(Image* image,
									FunctionDebugInfo* function,
									CpuState* cpuState, bool isTopFrame,
									StackFrame*& _previousFrame,
									CpuState*& _previousCpuState) = 0;
										// returns reference to previous frame
										// and CPU state; returned CPU state
										// can be NULL
	virtual	void				UpdateStackFrameCpuState(
									const StackFrame* frame,
									Image* previousImage,
									FunctionDebugInfo* previousFunction,
									CpuState* previousCpuState) = 0;
										// Called after a CreateStackFrame()
										// with the image/function corresponding
										// to the CPU state.

	virtual	status_t			DisassembleCode(FunctionDebugInfo* function,
									const void* buffer, size_t bufferSize,
									DisassembledCode*& _sourceCode) = 0;
	virtual	status_t			GetStatement(FunctionDebugInfo* function,
									target_addr_t address,
									Statement*& _statement) = 0;
	virtual	status_t			GetInstructionInfo(target_addr_t address,
									InstructionInfo& _info) = 0;

			status_t			CreateStackTrace(Team* team,
									ImageDebugInfoProvider* imageInfoProvider,
									CpuState* cpuState,
									StackTrace*& _stackTrace);
										// team is not locked

protected:
			TeamMemory*			fTeamMemory;
};


#endif	// ARCHITECTURE_H
