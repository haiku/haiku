/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef SPECIFIC_TEAM_DEBUG_INFO_H
#define SPECIFIC_TEAM_DEBUG_INFO_H

#include <SupportDefs.h>


class ImageInfo;
class LocatableFile;
class SpecificImageDebugInfo;


class SpecificTeamDebugInfo {
public:
	virtual						~SpecificTeamDebugInfo();

	virtual	status_t			CreateImageDebugInfo(const ImageInfo& imageInfo,
									LocatableFile* imageFile,
									SpecificImageDebugInfo*& _imageDebugInfo)
										= 0;
};


#endif	// SPECIFIC_TEAM_DEBUG_INFO_H
