/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "ImageDebugInfo.h"

#include <new>

#include "DebuggerDebugInfo.h"
#include "FunctionDebugInfo.h"


ImageDebugInfo::ImageDebugInfo(const ImageInfo& imageInfo,
	DebuggerInterface* debuggerInterface, Architecture* architecture)
	:
	fImageInfo(imageInfo),
	fDebuggerInterface(debuggerInterface),
	fArchitecture(architecture)
{
}


ImageDebugInfo::~ImageDebugInfo()
{
	for (int32 i = 0; FunctionDebugInfo* function = fFunctions.ItemAt(i); i++)
		function->RemoveReference();
}


status_t
ImageDebugInfo::Init()
{
	// Create debug infos for every kind debug info available, sorted
	// descendingly by expressiveness.

	// TODO: DWARF, etc.

	// debugger based info
	DebuggerDebugInfo* debuggerDebugInfo = new(std::nothrow) DebuggerDebugInfo(
		fImageInfo, fDebuggerInterface, fArchitecture);
	if (debuggerDebugInfo == NULL)
		return B_NO_MEMORY;

	status_t error = debuggerDebugInfo->Init();
	if (error == B_OK && !fDebugInfos.AddItem(debuggerDebugInfo))
		error = B_NO_MEMORY;

	if (error != B_OK) {
		delete debuggerDebugInfo;
		if (error == B_NO_MEMORY)
			return error;
				// only "no memory" is fatal
	}

	// get functions -- get them from most expressive debug info first and add
	// missing functions from less expressive debug infos
	for (int32 i = 0; DebugInfo* debugInfo = fDebugInfos.ItemAt(i); i++) {
		FunctionList functions;
		status_t error = debugInfo->GetFunctions(functions);
		if (error != B_OK)
			return error;

		for (int32 k = 0; FunctionDebugInfo* function = functions.ItemAt(k);
				k++) {
			if (FunctionAtAddress(function->Address()) == NULL) {
				if (!fFunctions.BinaryInsert(function, &_CompareFunctions)) {
					for (; (function = functions.ItemAt(k)); k++) {
						function->RemoveReference();
						return B_NO_MEMORY;
					}
				}
			} else
				function->RemoveReference();
		}
	}

	return B_OK;
}


int32
ImageDebugInfo::CountFunctions() const
{
	return fFunctions.CountItems();
}


FunctionDebugInfo*
ImageDebugInfo::FunctionAt(int32 index) const
{
	return fFunctions.ItemAt(index);
}


FunctionDebugInfo*
ImageDebugInfo::FunctionAtAddress(target_addr_t address) const
{
	return fFunctions.BinarySearchByKey(address, &_CompareAddressFunction);
}


/*static*/ int
ImageDebugInfo::_CompareFunctions(const FunctionDebugInfo* a,
	const FunctionDebugInfo* b)
{
	return a->Address() < b->Address()
		? -1 : (a->Address() == b->Address() ? 0 : 1);
}


/*static*/ int
ImageDebugInfo::_CompareAddressFunction(const target_addr_t* address,
	const FunctionDebugInfo* function)
{
	if (*address < function->Address())
		return -1;
	return *address < function->Address() + function->Size() ? 0 : 1;
}
