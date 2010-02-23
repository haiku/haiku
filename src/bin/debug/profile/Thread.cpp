/*
 * Copyright 2008-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "Thread.h"

#include <algorithm>
#include <new>

#include <debug_support.h>

#include "debug_utils.h"

#include "Image.h"
#include "Options.h"
#include "Team.h"


// #pragma mark - ThreadImage


ThreadImage::ThreadImage(Image* image, ImageProfileResult* result)
	:
	fImage(image),
	fResult(result)
{
	fImage->AcquireReference();
	fResult->AcquireReference();
}


ThreadImage::~ThreadImage()
{
	fImage->ReleaseReference();
	fResult->ReleaseReference();
}


// #pragma mark - ThreadI


Thread::Thread(thread_id threadID, const char* name, Team* team)
	:
	fID(threadID),
	fName(name),
	fTeam(team),
	fSampleArea(-1),
	fSamples(NULL),
	fProfileResult(NULL),
	fLazyImages(true)
{
	fTeam->AcquireReference();
}


Thread::~Thread()
{
	if (fSampleArea >= 0)
		delete_area(fSampleArea);

	if (fProfileResult != NULL)
		fProfileResult->ReleaseReference();

	while (ThreadImage* image = fImages.RemoveHead())
		delete image;
	while (ThreadImage* image = fOldImages.RemoveHead())
		delete image;

	fTeam->ReleaseReference();
}


int32
Thread::EntityID() const
{
	return ID();
}


const char*
Thread::EntityName() const
{
	return Name();
}


const char*
Thread::EntityType() const
{
	return "thread";
}


void
Thread::SetProfileResult(ProfileResult* result)
{
	ProfileResult* oldResult = fProfileResult;

	fProfileResult = result;
	if (fProfileResult != NULL)
		fProfileResult->AcquireReference();

	if (oldResult)
		oldResult->ReleaseReference();
}


void
Thread::UpdateInfo(const char* name)
{
	fName = name;
}


void
Thread::SetSampleArea(area_id area, addr_t* samples)
{
	fSampleArea = area;
	fSamples = samples;
}


void
Thread::SetInterval(bigtime_t interval)
{
	fProfileResult->SetInterval(interval);
}


void
Thread::SetLazyImages(bool lazy)
{
	fLazyImages = lazy;
}


status_t
Thread::AddImage(Image* image)
{
	ImageProfileResult* result;
	status_t error = fProfileResult->GetImageProfileResult(
		image->GetSharedImage(), image->ID(), result);
	if (error != B_OK)
		return error;

	BReference<ImageProfileResult> resultReference(result, true);

	ThreadImage* threadImage = new(std::nothrow) ThreadImage(image, result);
	if (threadImage == NULL)
		return B_NO_MEMORY;

	if (fLazyImages)
		fNewImages.Add(threadImage);
	else
		fImages.Add(threadImage);

	return B_OK;
}


void
Thread::RemoveImage(Image* image)
{
	ImageList::Iterator it = fImages.GetIterator();
	while (ThreadImage* threadImage = it.Next()) {
		if (threadImage->GetImage() == image) {
			it.Remove();
			if (threadImage->Result()->TotalHits() > 0)
				fOldImages.Add(threadImage);
			else
				delete threadImage;
			break;
		}
	}
}


void
Thread::AddSamples(int32 count, int32 dropped, int32 stackDepth,
	bool variableStackDepth, int32 event)
{
	_SynchronizeImages(event);

	if (variableStackDepth) {
		addr_t* samples = fSamples;

		while (count > 0) {
			addr_t sampleCount = *(samples++);

			if (sampleCount >= B_DEBUG_PROFILE_EVENT_BASE) {
				int32 eventParameterCount
					= sampleCount & B_DEBUG_PROFILE_EVENT_PARAMETER_MASK;
				if (sampleCount == B_DEBUG_PROFILE_IMAGE_EVENT) {
					_SynchronizeImages((int32)samples[0]);
				} else {
					fprintf(stderr, "unknown profile event: %#lx\n",
						sampleCount);
				}

				samples += eventParameterCount;
				count -= eventParameterCount + 1;
				continue;
			}

			fProfileResult->AddSamples(this, samples, sampleCount);

			samples += sampleCount;
			count -= sampleCount + 1;
		}
	} else {
		count = count / stackDepth * stackDepth;

		for (int32 i = 0; i < count; i += stackDepth)
			fProfileResult->AddSamples(this, fSamples + i, stackDepth);
	}

	fProfileResult->AddDroppedTicks(dropped);
}


void
Thread::AddSamples(addr_t* samples, int32 sampleCount)
{
	fProfileResult->AddSamples(this, samples, sampleCount);
}


void
Thread::PrintResults()
{
	fProfileResult->PrintResults(this);
}


int32
Thread::CountImages() const
{
	return fImages.Count() + fOldImages.Count();
}


ImageProfileResult*
Thread::VisitImages(Visitor& visitor) const
{
	ImageList::ConstIterator it = fOldImages.GetIterator();
	while (ThreadImage* image = it.Next()) {
		if (visitor.VisitImage(image->Result()))
			return image->Result();
	}

	it = fImages.GetIterator();
	while (ThreadImage* image = it.Next()) {
		if (visitor.VisitImage(image->Result()))
			return image->Result();
	}

	return NULL;
}


ImageProfileResult*
Thread::FindImage(addr_t address, addr_t& _loadDelta) const
{
	ImageList::ConstIterator it = fImages.GetIterator();
	while (ThreadImage* image = it.Next()) {
		if (image->GetImage()->ContainsAddress(address)) {
			_loadDelta = image->GetImage()->LoadDelta();
			return image->Result();
		}
	}
	return NULL;
}


void
Thread::_SynchronizeImages(int32 event)
{
	// remove obsolete images
	ImageList::Iterator it = fImages.GetIterator();
	while (ThreadImage* image = it.Next()) {
		int32 deleted = image->GetImage()->DeletionEvent();
		if (deleted >= 0 && event >= deleted) {
			it.Remove();
			if (image->Result()->TotalHits() > 0)
				fOldImages.Add(image);
			else
				delete image;
		}
	}

	// add new images
	it = fNewImages.GetIterator();
	while (ThreadImage* image = it.Next()) {
		if (image->GetImage()->CreationEvent() <= event) {
			it.Remove();
			int32 deleted = image->GetImage()->DeletionEvent();
			if (deleted >= 0 && event >= deleted) {
				// image already deleted
				delete image;
			} else
				fImages.Add(image);
		}
	}
}
