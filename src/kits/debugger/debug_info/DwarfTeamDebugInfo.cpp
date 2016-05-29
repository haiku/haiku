/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */

#include "DwarfTeamDebugInfo.h"

#include <new>

#include <string.h>

#include "arch/Architecture.h"
#include "DebuggerInterface.h"
#include "DwarfFile.h"
#include "DwarfImageDebugInfo.h"
#include "DwarfImageDebugInfoLoadingState.h"
#include "DwarfManager.h"
#include "GlobalTypeLookup.h"
#include "ImageDebugInfoLoadingState.h"
#include "LocatableFile.h"


DwarfTeamDebugInfo::DwarfTeamDebugInfo(Architecture* architecture,
	DebuggerInterface* interface, FileManager* fileManager,
	GlobalTypeLookup* typeLookup, GlobalTypeCache* typeCache)
	:
	fArchitecture(architecture),
	fDebuggerInterface(interface),
	fFileManager(fileManager),
	fManager(NULL),
	fTypeLookup(typeLookup),
	fTypeCache(typeCache)
{
	fDebuggerInterface->AcquireReference();
	fTypeCache->AcquireReference();
}


DwarfTeamDebugInfo::~DwarfTeamDebugInfo()
{
	fDebuggerInterface->ReleaseReference();
	fTypeCache->ReleaseReference();
	delete fManager;
}


status_t
DwarfTeamDebugInfo::Init()
{
	fManager = new(std::nothrow) DwarfManager(fArchitecture->AddressSize());
	if (fManager == NULL)
		return B_NO_MEMORY;

	status_t error = fManager->Init();
	if (error != B_OK)
		return error;

	return B_OK;
}


status_t
DwarfTeamDebugInfo::CreateImageDebugInfo(const ImageInfo& imageInfo,
	LocatableFile* imageFile, ImageDebugInfoLoadingState& _state,
	SpecificImageDebugInfo*& _imageDebugInfo)
{
	// We only like images whose file we can play with.
	BString filePath;
	if (imageFile == NULL || !imageFile->GetLocatedPath(filePath))
		return B_ENTRY_NOT_FOUND;

	// try to load the DWARF file
	DwarfImageDebugInfoLoadingState* dwarfState;
	if (_state.HasSpecificDebugInfoLoadingState()) {
		dwarfState = dynamic_cast<DwarfImageDebugInfoLoadingState*>(
			_state.GetSpecificDebugInfoLoadingState());
		if (dwarfState == NULL)
			return B_BAD_VALUE;
	} else {
	 	dwarfState = new(std::nothrow) DwarfImageDebugInfoLoadingState();
		if (dwarfState == NULL)
			return B_NO_MEMORY;
		_state.SetSpecificDebugInfoLoadingState(dwarfState);
	}

	status_t error = fManager->LoadFile(filePath, dwarfState->GetFileState());
	if (error != B_OK)
		return error;

	error = fManager->FinishLoading();
	if (error != B_OK)
		return error;

	// create the image debug info
	DwarfImageDebugInfo* debugInfo = new(std::nothrow) DwarfImageDebugInfo(
		imageInfo, fDebuggerInterface, fArchitecture, fFileManager,
		fTypeLookup, fTypeCache, dwarfState->GetFileState().dwarfFile);
	if (debugInfo == NULL)
		return B_NO_MEMORY;

	error = debugInfo->Init();
	if (error != B_OK) {
		delete debugInfo;
		return error;
	}

	_imageDebugInfo = debugInfo;
	return B_OK;
}
