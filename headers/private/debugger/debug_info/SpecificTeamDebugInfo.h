/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef SPECIFIC_TEAM_DEBUG_INFO_H
#define SPECIFIC_TEAM_DEBUG_INFO_H

#include <SupportDefs.h>


class ImageDebugInfoLoadingState;
class ImageInfo;
class LocatableFile;
class SpecificImageDebugInfo;

class SpecificTeamDebugInfo {
public:
	virtual						~SpecificTeamDebugInfo();

	virtual	status_t			CreateImageDebugInfo(const ImageInfo& imageInfo,
									LocatableFile* imageFile,
									ImageDebugInfoLoadingState& _state,
									SpecificImageDebugInfo*& _imageDebugInfo)
										= 0;
};


#endif	// SPECIFIC_TEAM_DEBUG_INFO_H
