/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TEAM_DEBUG_INFO_H
#define TEAM_DEBUG_INFO_H

#include <ObjectList.h>
#include <Referenceable.h>

#include "ImageInfo.h"


class Architecture;
class DebuggerInterface;
class FileManager;
class ImageDebugInfo;
class ImageInfo;
class LocatableFile;
class SpecificTeamDebugInfo;


class TeamDebugInfo : public Referenceable {
public:
								TeamDebugInfo(
									DebuggerInterface* debuggerInterface,
									Architecture* architecture,
									FileManager* fileManager);
								~TeamDebugInfo();

			status_t			Init();

			status_t			LoadImageDebugInfo(const ImageInfo& imageInfo,
									LocatableFile* imageFile,
									ImageDebugInfo*& _imageDebugInfo);

private:
			typedef BObjectList<SpecificTeamDebugInfo> SpecificInfoList;

private:
			DebuggerInterface*	fDebuggerInterface;
			Architecture*		fArchitecture;
			FileManager*		fFileManager;
			SpecificInfoList	fSpecificInfos;
};


#endif	// TEAM_DEBUG_INFO_H
