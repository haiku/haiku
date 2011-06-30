/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "DebuggerImageDebugInfo.h"

#include <algorithm>
#include <new>

#include <AutoDeleter.h>

#include "Architecture.h"
#include "BasicFunctionDebugInfo.h"
#include "DebuggerInterface.h"
#include "Demangler.h"
#include "SymbolInfo.h"


DebuggerImageDebugInfo::DebuggerImageDebugInfo(const ImageInfo& imageInfo,
	DebuggerInterface* debuggerInterface, Architecture* architecture)
	:
	fImageInfo(imageInfo),
	fDebuggerInterface(debuggerInterface),
	fArchitecture(architecture)
{
}


DebuggerImageDebugInfo::~DebuggerImageDebugInfo()
{
}


status_t
DebuggerImageDebugInfo::Init()
{
	return B_OK;
}


status_t
DebuggerImageDebugInfo::GetFunctions(BObjectList<FunctionDebugInfo>& functions)
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
DebuggerImageDebugInfo::GetType(GlobalTypeCache* cache,
	const BString& name, const TypeLookupConstraints& constraints,
	Type*& _type)
{
	return B_UNSUPPORTED;
}


AddressSectionType
DebuggerImageDebugInfo::GetAddressSectionType(target_addr_t address)
{
	return ADDRESS_SECTION_TYPE_UNKNOWN;
}


status_t
DebuggerImageDebugInfo::CreateFrame(Image* image,
	FunctionInstance* functionInstance, CpuState* cpuState,
	StackFrame*& _previousFrame, CpuState*& _previousCpuState)
{
	return B_UNSUPPORTED;
}


status_t
DebuggerImageDebugInfo::GetStatement(FunctionDebugInfo* function,
	target_addr_t address, Statement*& _statement)
{
	return fArchitecture->GetStatement(function, address, _statement);
}


status_t
DebuggerImageDebugInfo::GetStatementAtSourceLocation(
	FunctionDebugInfo* function, const SourceLocation& sourceLocation,
	Statement*& _statement)
{
	return B_ENTRY_NOT_FOUND;
}


status_t
DebuggerImageDebugInfo::GetSourceLanguage(FunctionDebugInfo* function,
	SourceLanguage*& _language)
{
	return B_UNSUPPORTED;
}


ssize_t
DebuggerImageDebugInfo::ReadCode(target_addr_t address, void* buffer,
	size_t size)
{
	return fDebuggerInterface->ReadMemory(address, buffer, size);
}


status_t
DebuggerImageDebugInfo::AddSourceCodeInfo(LocatableFile* file,
	FileSourceCode* sourceCode)
{
	return B_UNSUPPORTED;
}


/*static*/ int
DebuggerImageDebugInfo::_CompareSymbols(const SymbolInfo* a,
	const SymbolInfo* b)
{
	return a->Address() < b->Address()
		? -1 : (a->Address() == b->Address() ? 0 : 1);
}
