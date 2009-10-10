/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DWARF_TEAM_DEBUG_INFO_H
#define DWARF_TEAM_DEBUG_INFO_H

#include "SpecificTeamDebugInfo.h"


class Architecture;
class DwarfManager;
class FileManager;
class ImageInfo;
class GlobalTypeCache;
class GlobalTypeLookup;
class TeamMemory;


class DwarfTeamDebugInfo : public SpecificTeamDebugInfo {
public:
								DwarfTeamDebugInfo(Architecture* architecture,
									TeamMemory* teamMemory,
									FileManager* fileManager,
									GlobalTypeLookup* typeLookup,
									GlobalTypeCache* typeCache);
	virtual						~DwarfTeamDebugInfo();

			status_t			Init();

	virtual	status_t			CreateImageDebugInfo(const ImageInfo& imageInfo,
									LocatableFile* imageFile,
									SpecificImageDebugInfo*& _imageDebugInfo);

private:
			Architecture*		fArchitecture;
			TeamMemory*			fTeamMemory;
			FileManager*		fFileManager;
			DwarfManager*		fManager;
			GlobalTypeLookup*	fTypeLookup;
			GlobalTypeCache*	fTypeCache;
};


#endif	// DWARF_TEAM_DEBUG_INFO_H
