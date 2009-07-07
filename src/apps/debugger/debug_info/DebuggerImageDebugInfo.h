/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DEBUGGER_IMAGE_DEBUG_INFO_H
#define DEBUGGER_IMAGE_DEBUG_INFO_H

#include "ImageInfo.h"
#include "SpecificImageDebugInfo.h"


class Architecture;
class DebuggerInterface;
class SymbolInfo;


class DebuggerImageDebugInfo : public SpecificImageDebugInfo {
public:
								DebuggerImageDebugInfo(
									const ImageInfo& imageInfo,
									DebuggerInterface* debuggerInterface,
									Architecture* architecture);
	virtual						~DebuggerImageDebugInfo();

			status_t			Init();

	virtual	status_t			GetFunctions(
									BObjectList<FunctionDebugInfo>& functions);
	virtual	status_t			CreateFrame(Image* image,
									FunctionDebugInfo* function,
									CpuState* cpuState,
									StackFrame*& _previousFrame,
									CpuState*& _previousCpuState);
	virtual	status_t			LoadSourceCode(FunctionDebugInfo* function,
									SourceCode*& _sourceCode);
	virtual	status_t			GetStatement(FunctionDebugInfo* function,
									target_addr_t address,
									Statement*& _statement);
	virtual	status_t			GetStatementForSourceLocation(
									FunctionDebugInfo* function,
									const SourceLocation& sourceLocation,
									Statement*& _statement);

private:
	static	int					_CompareSymbols(const SymbolInfo* a,
									const SymbolInfo* b);

private:
			ImageInfo			fImageInfo;
			DebuggerInterface*	fDebuggerInterface;
			Architecture*		fArchitecture;
};


#endif	// DEBUGGER_IMAGE_DEBUG_INFO_H
