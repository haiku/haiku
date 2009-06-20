/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DEBUGGER_DEBUG_INFO_H
#define DEBUGGER_DEBUG_INFO_H

#include <String.h>

#include <ObjectList.h>

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

	virtual	FunctionDebugInfo*	FindFunction(target_addr_t address);
	virtual	status_t			CreateFrame(Image* image,
									FunctionDebugInfo* function,
									CpuState* cpuState,
									StackFrame*& _previousFrame,
									CpuState*& _previousCpuState);

private:
			typedef BObjectList<SymbolInfo> SymbolList;

			struct FindByAddressPredicate;

private:
			SymbolInfo*			_FindSymbol(target_addr_t address);
	static	int					_CompareSymbols(const SymbolInfo* a,
									const SymbolInfo* b);
	static	int					_CompareAddressSymbol(
									const target_addr_t* address,
									const SymbolInfo* info);

private:
			ImageInfo			fImageInfo;
			DebuggerInterface*	fDebuggerInterface;
			Architecture*		fArchitecture;
			SymbolList			fSymbols;
};


#endif	// DEBUGGER_DEBUG_INFO_H
