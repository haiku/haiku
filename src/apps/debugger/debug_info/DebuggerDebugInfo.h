/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DEBUGGER_DEBUG_INFO_H
#define DEBUGGER_DEBUG_INFO_H

#include <String.h>

#include "DebugInfo.h"
#include "ImageInfo.h"


class Architecture;
class DebuggerInterface;
class FunctionDebugInfo;
class SymbolInfo;


class DebuggerDebugInfo : public DebugInfo {
public:
								DebuggerDebugInfo(const ImageInfo& imageInfo,
									DebuggerInterface* debuggerInterface,
									Architecture* architecture);
	virtual						~DebuggerDebugInfo();

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

private:
	static	int					_CompareSymbols(const SymbolInfo* a,
									const SymbolInfo* b);

private:
			ImageInfo			fImageInfo;
			DebuggerInterface*	fDebuggerInterface;
			Architecture*		fArchitecture;
};


#endif	// DEBUGGER_DEBUG_INFO_H
