/*
 * Copyright 2012, Rene Gollent, rene@gollent.com.
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "Jobs.h"

#include <AutoLocker.h>

#include "Image.h"
#include "ImageDebugInfo.h"
#include "TeamDebugInfo.h"
#include "Team.h"


LoadImageDebugInfoJob::LoadImageDebugInfoJob(Image* image)
	:
	fKey(image, JOB_TYPE_LOAD_IMAGE_DEBUG_INFO),
	fImage(image)
{
	fImage->AcquireReference();
}


LoadImageDebugInfoJob::~LoadImageDebugInfoJob()
{
	fImage->ReleaseReference();
}


const JobKey&
LoadImageDebugInfoJob::Key() const
{
	return fKey;
}


status_t
LoadImageDebugInfoJob::Do()
{
	// get an image info for the image
	AutoLocker<Team> locker(fImage->GetTeam());
	ImageInfo imageInfo(fImage->Info());
	locker.Unlock();

	// create the debug info
	ImageDebugInfo* debugInfo;
	status_t error = fImage->GetTeam()->DebugInfo()->LoadImageDebugInfo(
		imageInfo, fImage->ImageFile(), debugInfo);

	// set the result
	locker.Lock();
	if (error == B_OK) {
		error = fImage->SetImageDebugInfo(debugInfo, IMAGE_DEBUG_INFO_LOADED);
		debugInfo->ReleaseReference();
	} else
		fImage->SetImageDebugInfo(NULL, IMAGE_DEBUG_INFO_UNAVAILABLE);

	return error;
}


/*static*/ status_t
LoadImageDebugInfoJob::ScheduleIfNecessary(Worker* worker, Image* image,
	ImageDebugInfo** _imageDebugInfo)
{
	AutoLocker<Team> teamLocker(image->GetTeam());

	// If already loaded, we're done.
	if (image->GetImageDebugInfo() != NULL) {
		if (_imageDebugInfo != NULL) {
			*_imageDebugInfo = image->GetImageDebugInfo();
			(*_imageDebugInfo)->AcquireReference();
		}
		return B_OK;
	}

	// If already loading, the caller has to wait, if desired.
	if (image->ImageDebugInfoState() == IMAGE_DEBUG_INFO_LOADING) {
		if (_imageDebugInfo != NULL)
			*_imageDebugInfo = NULL;
		return B_OK;
	}

	// If an earlier load attempt failed, bail out.
	if (image->ImageDebugInfoState() != IMAGE_DEBUG_INFO_NOT_LOADED)
		return B_ERROR;

	// schedule a job
	LoadImageDebugInfoJob* job = new(std::nothrow) LoadImageDebugInfoJob(image);
	if (job == NULL)
		return B_NO_MEMORY;

	status_t error = worker->ScheduleJob(job);
	if (error != B_OK) {
		image->SetImageDebugInfo(NULL, IMAGE_DEBUG_INFO_UNAVAILABLE);
		return error;
	}

	image->SetImageDebugInfo(NULL, IMAGE_DEBUG_INFO_LOADING);

	if (_imageDebugInfo != NULL)
		*_imageDebugInfo = NULL;
	return B_OK;
}
