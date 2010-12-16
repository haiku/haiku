/*
 * Copyright 2008-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "ProfileResult.h"


// #pragma mark - ImageProfileResultContainer


ImageProfileResultContainer::~ImageProfileResultContainer()
{
}


// #pragma mark - ImageProfileResultContainer::Visitor


ImageProfileResultContainer::Visitor::~Visitor()
{
}


// #pragma mark - ImageProfileResult


ImageProfileResult::ImageProfileResult(SharedImage* image, image_id id)
	:
	fImage(image),
	fTotalHits(0),
	fImageID(id)
{
	fImage->AcquireReference();
}


ImageProfileResult::~ImageProfileResult()
{
	fImage->ReleaseReference();
}


status_t
ImageProfileResult::Init()
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
