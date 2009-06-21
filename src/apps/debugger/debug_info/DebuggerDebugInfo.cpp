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


struct DebuggerDebugInfo::FindByAddressPredicate : UnaryPredicate<SymbolInfo> {
	FindByAddressPredicate(target_addr_t address)
		:
		fAddress(address)
	{
	}

	virtual int operator()(const SymbolInfo* info) const
	{
		if (fAddress < info->Address())
			return -1;
		return fAddress < info->Address() + info->Size() ? 0 : 1;
	}

private:
	target_addr_t	fAddress;
};


DebuggerDebugInfo::DebuggerDebugInfo(const ImageInfo& imageInfo,
	DebuggerInterface* debuggerInterface, Architecture* architecture)
	:
	fImageInfo(imageInfo),
	fDebuggerInterface(debuggerInterface),
	fArchitecture(architecture),
	fSymbols(20, true)
{
}


DebuggerDebugInfo::~DebuggerDebugInfo()
{
}


status_t
DebuggerDebugInfo::Init()
{
	// TODO: Extend DebuggerInterface to find a symbol on demand!

	status_t error = fDebuggerInterface->GetSymbolInfos(fImageInfo.TeamID(),
		fImageInfo.ImageID(), fSymbols);
	if (error != B_OK)
		return error;

	// sort the symbols
	fSymbols.SortItems(&_CompareSymbols);

	return B_OK;
}


FunctionDebugInfo*
DebuggerDebugInfo::FindFunction(target_addr_t address)
{
	SymbolInfo* symbolInfo = _FindSymbol(address);
	if (symbolInfo == NULL || symbolInfo->Type() != B_SYMBOL_TYPE_TEXT)
		return NULL;

	return new(std::nothrow) BasicFunctionDebugInfo(this, symbolInfo->Address(),
		symbolInfo->Size(), symbolInfo->Name(),
		Demangler::Demangle(symbolInfo->Name()));
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


SymbolInfo*
DebuggerDebugInfo::_FindSymbol(target_addr_t address)
{
	return fSymbols.BinarySearchByKey(address, &_CompareAddressSymbol);
}


/*static*/ int
DebuggerDebugInfo::_CompareSymbols(const SymbolInfo* a, const SymbolInfo* b)
{
	return a->Address() < b->Address()
		? -1 : (a->Address() == b->Address() ? 0 : 1);
}


/*static*/ int
DebuggerDebugInfo::_CompareAddressSymbol(const target_addr_t* address,
	const SymbolInfo* info)
{
	if (*address < info->Address())
		return -1;
	return *address < info->Address() + info->Size() ? 0 : 1;
}
