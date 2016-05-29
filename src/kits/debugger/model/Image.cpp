/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "Image.h"

#include "ImageDebugInfo.h"
#include "LocatableFile.h"
#include "Team.h"
#include "TeamDebugInfo.h"


Image::Image(Team* team,const ImageInfo& imageInfo, LocatableFile* imageFile)
	:
	fTeam(team),
	fInfo(imageInfo),
	fImageFile(imageFile),
	fDebugInfo(NULL),
	fDebugInfoState(IMAGE_DEBUG_INFO_NOT_LOADED)
{
	if (fImageFile != NULL)
		fImageFile->AcquireReference();
}


Image::~Image()
{
	if (fDebugInfo != NULL) {
		if (fTeam != NULL)
			fTeam->DebugInfo()->RemoveImageDebugInfo(fDebugInfo);
		fDebugInfo->ReleaseReference();
	}
	if (fImageFile != NULL)
		fImageFile->ReleaseReference();
}


status_t
Image::Init()
{
	return B_OK;
}


bool
Image::ContainsAddress(target_addr_t address) const
{
	return (address >= fInfo.TextBase()
			&& address < fInfo.TextBase() + fInfo.TextSize())
		|| (address >= fInfo.DataBase()
			&& address < fInfo.DataBase() + fInfo.DataSize());
}


status_t
Image::SetImageDebugInfo(ImageDebugInfo* debugInfo,
	image_debug_info_state state)
{
	if (debugInfo == fDebugInfo && state == fDebugInfoState)
		return B_OK;

	if (fDebugInfo != NULL) {
		fTeam->DebugInfo()->RemoveImageDebugInfo(fDebugInfo);
		fDebugInfo->ReleaseReference();
	}

	fDebugInfo = debugInfo;
	fDebugInfoState = state;

	status_t error = B_OK;
	if (fDebugInfo != NULL) {
		error = fTeam->DebugInfo()->AddImageDebugInfo(fDebugInfo);
		if (error == B_OK) {
			fDebugInfo->AcquireReference();
		} else {
			fDebugInfo = NULL;
			fDebugInfoState = IMAGE_DEBUG_INFO_UNAVAILABLE;
		}
	}

	// notify listeners
	fTeam->NotifyImageDebugInfoChanged(this);

	return error;
}
