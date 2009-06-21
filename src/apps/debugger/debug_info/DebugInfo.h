/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DEBUG_INFO_H
#define DEBUG_INFO_H

#include <Referenceable.h>

#include "ArchitectureTypes.h"


class Architecture;
class CpuState;
class DebuggerInterface;
class FunctionDebugInfo;
class Image;
class SourceCode;
class StackFrame;


class DebugInfo : public Referenceable {
public:
	virtual						~DebugInfo();

	virtual	FunctionDebugInfo*	FindFunction(target_addr_t address) = 0;
									// returns a reference

	virtual	status_t			CreateFrame(Image* image,
									FunctionDebugInfo* function,
									CpuState* cpuState,
									StackFrame*& _previousFrame,
									CpuState*& _previousCpuState) = 0;
										// returns reference to previous frame
										// and CPU state; returned CPU state
										// can be NULL; can return B_UNSUPPORTED
	virtual	status_t			LoadSourceCode(FunctionDebugInfo* function,
									SourceCode*& _sourceCode) = 0;
										// returns reference
};


#endif	// DEBUG_INFO_H
