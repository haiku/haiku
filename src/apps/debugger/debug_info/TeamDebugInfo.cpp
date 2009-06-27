/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "TeamDebugInfo.h"

#include <new>

#include <AutoDeleter.h>

#include "DebuggerTeamDebugInfo.h"
#include "ImageDebugInfo.h"
#include "SpecificImageDebugInfo.h"


TeamDebugInfo::TeamDebugInfo(DebuggerInterface* debuggerInterface,
	Architecture* architecture)
	:
	fDebuggerInterface(debuggerInterface),
	fArchitecture(architecture),
	fSpecificInfos(10, true)
{
}


TeamDebugInfo::~TeamDebugInfo()
{
}


status_t
TeamDebugInfo::Init()
{
	// create specific infos for all types of debug info we support

	// DWARF
	// TODO:...

	// debugger based info
	DebuggerTeamDebugInfo* debuggerInfo
		= new(std::nothrow) DebuggerTeamDebugInfo(fDebuggerInterface,
			fArchitecture);
	if (debuggerInfo == NULL || !fSpecificInfos.AddItem(debuggerInfo)) {
		delete debuggerInfo;
		return B_NO_MEMORY;
	}

	status_t error = debuggerInfo->Init();
	if (error != B_OK)
		return error;

	return B_OK;
}


status_t
TeamDebugInfo::LoadImageDebugInfo(const ImageInfo& imageInfo,
	ImageDebugInfo*& _imageDebugInfo)
{
	ImageDebugInfo* imageDebugInfo = new(std::nothrow) ImageDebugInfo(
		imageInfo);
	if (imageDebugInfo == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<ImageDebugInfo> imageDebugInfoDeleter(imageDebugInfo);

	for (int32 i = 0; SpecificTeamDebugInfo* specificTeamInfo
			= fSpecificInfos.ItemAt(i); i++) {
		SpecificImageDebugInfo* specificImageInfo;
		status_t error = specificTeamInfo->CreateImageDebugInfo(imageInfo,
			specificImageInfo);
		if (error == B_OK) {
			if (!imageDebugInfo->AddSpecificInfo(specificImageInfo)) {
				delete specificImageInfo;
				return B_NO_MEMORY;
			}
		} else if (error == B_NO_MEMORY)
			return error;
				// fail only when out of memory
	}

	status_t error = imageDebugInfo->FinishInit();
	if (error != B_OK)
		return error;

	_imageDebugInfo = imageDebugInfoDeleter.Detach();
	return B_OK;
}
