/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "ImageDebugInfo.h"

#include <new>

#include "DebuggerDebugInfo.h"


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

	return B_OK;
}


FunctionDebugInfo*
ImageDebugInfo::FindFunction(target_addr_t address)
{
	for (int32 i = 0; DebugInfo* debugInfo = fDebugInfos.ItemAt(i); i++) {
		FunctionDebugInfo* functionInfo = debugInfo->FindFunction(address);
		if (functionInfo)
			return functionInfo;
	}

	return NULL;
}
