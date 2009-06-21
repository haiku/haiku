/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef ARCHITECTURE_X86_H
#define ARCHITECTURE_X86_H

#include "Architecture.h"
#include "Array.h"
#include "Register.h"


class ArchitectureX86 : public Architecture {
public:
								ArchitectureX86(
									DebuggerInterface* debuggerInterface);
	virtual						~ArchitectureX86();

	virtual	status_t			Init();

	virtual	int32				CountRegisters() const;
	virtual	const Register*		Registers() const;

	virtual	status_t			CreateCpuState(const void* cpuStateData,
									size_t size, CpuState*& _state);
	virtual	status_t			CreateStackFrame(Image* image,
									FunctionDebugInfo* function,
									CpuState* cpuState,
									StackFrame*& _previousFrame,
									CpuState*& _previousCpuState);
	virtual	status_t			DisassembleCode(FunctionDebugInfo* function,
									const void* buffer, size_t bufferSize,
									SourceCode*& _sourceCode);

private:
			void				_AddRegister(int32 index, const char* name,
									register_format format, uint32 bitSize,
									register_type type);
			void				_AddIntegerRegister(int32 index,
									const char* name, uint32 bitSize,
									register_type type);

private:
			Array<Register>		fRegisters;
};


#endif	// ARCHITECTURE_X86_H
