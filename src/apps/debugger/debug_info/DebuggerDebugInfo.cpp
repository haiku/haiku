/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "DebuggerDebugInfo.h"

#include <algorithm>
#include <new>

#include <AutoDeleter.h>

#include "Architecture.h"
#include "BasicFunctionDebugInfo.h"
#include "DebuggerInterface.h"
#include "Demangler.h"
#include "SymbolInfo.h"


DebuggerDebugInfo::DebuggerDebugInfo(const ImageInfo& imageInfo,
	DebuggerInterface* debuggerInterface, Architecture* architecture)
	:
	fImageInfo(imageInfo),
	fDebuggerInterface(debuggerInterface),
	fArchitecture(architecture)
{
}


DebuggerDebugInfo::~DebuggerDebugInfo()
{
}


status_t
DebuggerDebugInfo::Init()
{
	return B_OK;
}


status_t
DebuggerDebugInfo::GetFunctions(BObjectList<FunctionDebugInfo>& functions)
{
	BObjectList<SymbolInfo> symbols(20, true);
	status_t error = fDebuggerInterface->GetSymbolInfos(fImageInfo.TeamID(),
		fImageInfo.ImageID(), symbols);
	if (error != B_OK)
		return error;

	// sort the symbols -- not necessary, but a courtesy to ImageDebugInfo which
	// will peform better when inserting functions at the end of a list
	symbols.SortItems(&_CompareSymbols);

	// create the function infos
	int32 functionsAdded = 0;
	for (int32 i = 0; SymbolInfo* symbol = symbols.ItemAt(i); i++) {
		if (symbol->Type() != B_SYMBOL_TYPE_TEXT)
			continue;

		FunctionDebugInfo* function = new(std::nothrow) BasicFunctionDebugInfo(
			this, symbol->Address(), symbol->Size(), symbol->Name(),
			Demangler::Demangle(symbol->Name()));
		if (function == NULL || !functions.AddItem(function)) {
			delete function;
			int32 index = functions.CountItems() - 1;
			for (; functionsAdded >= 0; functionsAdded--, index--) {
				function = functions.RemoveItemAt(index);
				delete function;
			}
			return B_NO_MEMORY;
		}

		functionsAdded++;
	}

	return B_OK;
}


status_t
DebuggerDebugInfo::CreateFrame(Image* image, FunctionDebugInfo* function,
	CpuState* cpuState, StackFrame*& _previousFrame,
	CpuState*& _previousCpuState)
{
	return B_UNSUPPORTED;
}


status_t
DebuggerDebugInfo::LoadSourceCode(FunctionDebugInfo* function,
	SourceCode*& _sourceCode)
{
	// allocate a buffer for the function code
	static const target_size_t kMaxBufferSize = 64 * 1024;
	target_size_t bufferSize = std::min(function->Size(), kMaxBufferSize);
	void* buffer = malloc(bufferSize);
	if (buffer == NULL)
		return B_NO_MEMORY;
	MemoryDeleter bufferDeleter(buffer);

	// read the function code
	ssize_t bytesRead = fDebuggerInterface->ReadMemory(function->Address(),
		buffer, bufferSize);
	if (bytesRead < 0)
		return bytesRead;

	return fArchitecture->DisassembleCode(function, buffer, bytesRead,
		_sourceCode);
}


status_t
DebuggerDebugInfo::GetStatement(FunctionDebugInfo* function,
	target_addr_t address, Statement*& _statement)
{
	return fArchitecture->GetStatement(function, address, _statement);
}


/*static*/ int
DebuggerDebugInfo::_CompareSymbols(const SymbolInfo* a, const SymbolInfo* b)
{
	return a->Address() < b->Address()
		? -1 : (a->Address() == b->Address() ? 0 : 1);
}
