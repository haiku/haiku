/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "DebuggerTeamDebugInfo.h"

#include <new>

#include "DebuggerImageDebugInfo.h"


DebuggerTeamDebugInfo::DebuggerTeamDebugInfo(
	DebuggerInterface* debuggerInterface, Architecture* architecture)
	:
	fDebuggerInterface(debuggerInterface),
	fArchitecture(architecture)
{
}


DebuggerTeamDebugInfo::~DebuggerTeamDebugInfo()
{
}


status_t
DebuggerTeamDebugInfo::Init()
{
	return B_OK;
}


status_t
DebuggerTeamDebugInfo::CreateImageDebugInfo(const ImageInfo& imageInfo,
	LocatableFile* imageFile, SpecificImageDebugInfo*& _imageDebugInfo)
{
	DebuggerImageDebugInfo* debuggerInfo
		= new(std::nothrow) DebuggerImageDebugInfo(imageInfo,
			fDebuggerInterface, fArchitecture);
	if (debuggerInfo == NULL)
		return B_NO_MEMORY;

	status_t error = debuggerInfo->Init();
	if (error != B_OK) {
		delete debuggerInfo;
		return error;
	}

	_imageDebugInfo = debuggerInfo;
	return B_OK;
}
