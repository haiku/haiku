/*
 * Copyright 2008-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "ProfileResult.h"


// #pragma mark - ProfileResultImage


ProfileResultImage::ProfileResultImage(Image* image)
	:
	fImage(image),
	fTotalHits(0)
{
	fImage->AddReference();
}


ProfileResultImage::~ProfileResultImage()
{
	fImage->RemoveReference();
}


status_t
ProfileResultImage::Init()
{
	return B_OK;
}


// #pragma mark - ProfileResult


ProfileResult::ProfileResult()
	:
	fEntity(NULL),
	fInterval(1)
{
}


ProfileResult::~ProfileResult()
{
}


status_t
ProfileResult::Init(ProfiledEntity* entity)
{
	fEntity = entity;
	return B_OK;
}


void
ProfileResult::SetInterval(bigtime_t interval)
{
	fInterval = interval;
}
