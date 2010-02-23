/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "SummaryProfileResult.h"

#include <new>


// #pragma mark - SummaryImage


SummaryImage::SummaryImage(ImageProfileResult* result)
	:
	fResult(result)
{
	fResult->AcquireReference();
}


SummaryImage::~SummaryImage()
{
	fResult->ReleaseReference();
}


// #pragma mark - SummaryProfileResult


SummaryProfileResult::SummaryProfileResult(ProfileResult* result)
	:
	fResult(result),
	fNextImageID(1)
{
	fResult->AcquireReference();
}


SummaryProfileResult::~SummaryProfileResult()
{
	fResult->ReleaseReference();
}


void
SummaryProfileResult::SetInterval(bigtime_t interval)
{
	ProfileResult::SetInterval(interval);
	fResult->SetInterval(interval);
}


status_t
SummaryProfileResult::Init(ProfiledEntity* entity)
{
	status_t error = ProfileResult::Init(entity);
	if (error != B_OK)
		return error;

	error = fImages.Init();
	if (error != B_OK)
		return error;

	return B_OK;
}


void
SummaryProfileResult::AddSamples(ImageProfileResultContainer* container,
	addr_t* samples, int32 sampleCount)
{
	fResult->AddSamples(container, samples, sampleCount);
}


void
SummaryProfileResult::AddDroppedTicks(int32 dropped)
{
	fResult->AddDroppedTicks(dropped);
}


void
SummaryProfileResult::PrintResults(ImageProfileResultContainer* container)
{
	// This is called for individual threads. We only print results in
	// PrintSummaryResults(), though.
}


void
SummaryProfileResult::PrintSummaryResults()
{
	fResult->PrintResults(this);
}


status_t
SummaryProfileResult::GetImageProfileResult(SharedImage* image, image_id id,
	ImageProfileResult*& _imageResult)
{
	// Check whether we do already know the image.
	SummaryImage* summaryImage = fImages.Lookup(image);
	if (summaryImage == NULL) {
		// nope, create it
		ImageProfileResult* imageResult;
		status_t error = fResult->GetImageProfileResult(image, fNextImageID++,
			imageResult);
		if (error != B_OK)
			return error;

		BReference<ImageProfileResult> imageResultReference(imageResult, true);

		summaryImage = new(std::nothrow) SummaryImage(imageResult);
		if (summaryImage == NULL)
			return B_NO_MEMORY;

		fImages.Insert(summaryImage);
	}

	_imageResult = summaryImage->Result();
	_imageResult->AcquireReference();

	return B_OK;
}


int32
SummaryProfileResult::CountImages() const
{
	return fImages.CountElements();
}


ImageProfileResult*
SummaryProfileResult::VisitImages(Visitor& visitor) const
{
	for (ImageTable::Iterator it = fImages.GetIterator();
			SummaryImage* image = it.Next();) {
		if (visitor.VisitImage(image->Result()))
			return image->Result();
	}

	return NULL;
}


ImageProfileResult*
SummaryProfileResult::FindImage(addr_t address, addr_t& _loadDelta) const
{
	// We cannot and don't need to implement this. It's only relevant for
	// AddSamples(), where we use the caller's container implementation.
	return NULL;
}
