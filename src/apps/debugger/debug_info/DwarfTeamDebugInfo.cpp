/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "DwarfTeamDebugInfo.h"

#include <new>

#include <string.h>

#include "DwarfFile.h"
#include "DwarfImageDebugInfo.h"
#include "DwarfManager.h"
#include "GlobalTypeLookup.h"
#include "LocatableFile.h"


DwarfTeamDebugInfo::DwarfTeamDebugInfo(Architecture* architecture,
	TeamMemory* teamMemory, FileManager* fileManager,
	GlobalTypeLookup* typeLookup, GlobalTypeCache* typeCache)
	:
	fArchitecture(architecture),
	fTeamMemory(teamMemory),
	fFileManager(fileManager),
	fManager(NULL),
	fTypeLookup(typeLookup),
	fTypeCache(typeCache)
{
	fTypeCache->AcquireReference();
}


DwarfTeamDebugInfo::~DwarfTeamDebugInfo()
{
	fTypeCache->ReleaseReference();
	delete fManager;
}


status_t
DwarfTeamDebugInfo::Init()
{
	fManager = new(std::nothrow) DwarfManager;
	if (fManager == NULL)
		return B_NO_MEMORY;

	status_t error = fManager->Init();
	if (error != B_OK)
		return error;

	return B_OK;
}


status_t
DwarfTeamDebugInfo::CreateImageDebugInfo(const ImageInfo& imageInfo,
	LocatableFile* imageFile, SpecificImageDebugInfo*& _imageDebugInfo)
{
	// We only like images whose file we can play with.
	BString filePath;
	if (imageFile == NULL || !imageFile->GetLocatedPath(filePath))
		return B_ENTRY_NOT_FOUND;

	// try to load the DWARF file
	DwarfFile* file;
	status_t error = fManager->LoadFile(filePath, file);
	if (error != B_OK)
		return error;
	BReference<DwarfFile> fileReference(file, true);

	error = fManager->FinishLoading();
	if (error != B_OK)
		return error;

	// create the image debug info
	DwarfImageDebugInfo* debugInfo = new(std::nothrow) DwarfImageDebugInfo(
		imageInfo, fArchitecture, fTeamMemory, fFileManager, fTypeLookup,
		fTypeCache, file);
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
