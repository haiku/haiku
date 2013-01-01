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
	fDebuggerInterface->AcquireReference();
}


DebuggerImageDebugInfo::~DebuggerImageDebugInfo()
{
	fDebuggerInterface->ReleaseReference();
}


status_t
DebuggerImageDebugInfo::Init()
{
	return B_OK;
}


status_t
DebuggerImageDebugInfo::GetFunctions(BObjectList<FunctionDebugInfo>& functions)
{
	return SpecificImageDebugInfo::GetFunctionsFromSymbols(functions,
		fDebuggerInterface, fImageInfo, this);
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
	bool getFullFrameInfo, target_addr_t returnFunctionAddress,
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
