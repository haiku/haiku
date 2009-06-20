/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "DebuggerDebugInfo.h"

#include <new>

#include "BasicFunctionDebugInfo.h"
#include "DebuggerInterface.h"
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
		symbolInfo->Size(), symbolInfo->Name(), symbolInfo->Name());
			// TODO: Demangle!
}


status_t
DebuggerDebugInfo::CreateFrame(Image* image, FunctionDebugInfo* function,
	CpuState* cpuState, StackFrame*& _previousFrame,
	CpuState*& _previousCpuState)
{
	return B_UNSUPPORTED;
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
