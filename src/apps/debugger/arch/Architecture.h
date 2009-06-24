/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef ARCHITECTURE_H
#define ARCHITECTURE_H

#include <OS.h>

#include <Referenceable.h>


class CpuState;
class DebuggerInterface;
class FunctionDebugInfo;
class Image;
class ImageDebugInfoProvider;
class Register;
class SourceCode;
class StackFrame;
class StackTrace;
class Team;


class Architecture : public Referenceable {
public:
								Architecture(
									DebuggerInterface* debuggerInterface);
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
	virtual	status_t			DisassembleCode(FunctionDebugInfo* function,
									const void* buffer, size_t bufferSize,
									SourceCode*& _sourceCode) = 0;

			status_t			CreateStackTrace(Team* team,
									ImageDebugInfoProvider* imageInfoProvider,
									CpuState* cpuState,
									StackTrace*& _stackTrace);
										// team is not locked

protected:
			DebuggerInterface*	fDebuggerInterface;
};


#endif	// ARCHITECTURE_H
