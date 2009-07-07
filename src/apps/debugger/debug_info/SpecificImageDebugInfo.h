/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef SPECIFIC_IMAGE_DEBUG_INFO_H
#define SPECIFIC_IMAGE_DEBUG_INFO_H

#include <ObjectList.h>
#include <Referenceable.h>

#include "Types.h"


class Architecture;
class CpuState;
class DebuggerInterface;
class FunctionDebugInfo;
class Image;
class SourceCode;
class SourceLocation;
class StackFrame;
class Statement;


class SpecificImageDebugInfo : public Referenceable {
public:
	virtual						~SpecificImageDebugInfo();

	virtual	status_t			GetFunctions(
									BObjectList<FunctionDebugInfo>& functions)
										= 0;
									// returns references

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
	virtual	status_t			GetStatement(FunctionDebugInfo* function,
									target_addr_t address,
									Statement*& _statement) = 0;
										// returns reference
	virtual	status_t			GetStatementForSourceLocation(
									FunctionDebugInfo* function,
									const SourceLocation& sourceLocation,
									Statement*& _statement) = 0;
										// returns reference
};


#endif	// SPECIFIC_IMAGE_DEBUG_INFO_H
