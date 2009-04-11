/*
 * Copyright 2008-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef THREAD_H
#define THREAD_H

#include <util/DoublyLinkedList.h>

#include "Image.h"


class Team;
class ThreadImage;
class ThreadProfileResult;


class ThreadImage {
public:
								ThreadImage(Image* image);
	virtual						~ThreadImage();

	virtual	status_t			Init();

	inline	image_id			ID() const;
	inline	Image*				GetImage() const;

	inline	bool				ContainsAddress(addr_t address) const;

	inline	int64				TotalHits() const;

protected:
			Image*				fImage;
			int64				fTotalHits;
};


class Thread : public DoublyLinkedListLinkImpl<Thread> {
public:
								Thread(const thread_info& info, Team* team);
								~Thread();

	inline	thread_id			ID() const;
	inline	const char*			Name() const;
	inline	addr_t*				Samples() const;
	inline	Team*				GetTeam() const;

	inline	ThreadProfileResult* ProfileResult() const;
			void				SetProfileResult(ThreadProfileResult* result);

			void				UpdateInfo();

			void				SetSampleArea(area_id area, addr_t* samples);
			void				SetInterval(bigtime_t interval);

	inline	status_t			AddImage(Image* image);
	inline	void				RemoveImage(Image* image);

			void				AddSamples(int32 count, int32 dropped,
									int32 stackDepth, bool variableStackDepth,
									int32 event);
			void				AddSamples(addr_t* samples, int32 sampleCount);
			void				PrintResults() const;

private:
	typedef DoublyLinkedList<ThreadImage>	ImageList;

private:
			thread_info			fInfo;
			::Team*				fTeam;
			area_id				fSampleArea;
			addr_t*				fSamples;
			ThreadProfileResult* fProfileResult;
};


class ThreadProfileResult {
public:
								ThreadProfileResult();
	virtual						~ThreadProfileResult();

	virtual	status_t			Init(Thread* thread);

			void				SetInterval(bigtime_t interval);

	virtual	void				SetLazyImages(bool lazy) = 0;

	virtual	status_t			AddImage(Image* image) = 0;
	virtual	void				RemoveImage(Image* image) = 0;
	virtual	void				SynchronizeImages(int32 event) = 0;

	virtual	void				AddSamples(addr_t* samples,
									int32 sampleCount) = 0;
	virtual	void				AddDroppedTicks(int32 dropped) = 0;
	virtual	void				PrintResults() = 0;

protected:
			Thread*				fThread;
			bigtime_t			fInterval;
};


template<typename ThreadImageType>
class AbstractThreadProfileResult : public ThreadProfileResult {
public:
								AbstractThreadProfileResult();
	virtual						~AbstractThreadProfileResult();

	virtual	void				SetLazyImages(bool lazy);

	virtual	status_t			AddImage(Image* image);
	virtual	void				RemoveImage(Image* image);
	virtual	void				SynchronizeImages(int32 event);

			ThreadImageType*	FindImage(addr_t address) const;
			int32				GetHitImages(ThreadImageType** images) const;

	virtual	void				AddSamples(addr_t* samples,
									int32 sampleCount) = 0;
	virtual	void				AddDroppedTicks(int32 dropped) = 0;
	virtual	void				PrintResults() = 0;

	virtual ThreadImageType*	CreateThreadImage(Image* image) = 0;

protected:
	typedef DoublyLinkedList<ThreadImageType>	ImageList;

			ImageList			fImages;
			ImageList			fNewImages;
			ImageList			fOldImages;
			bool				fLazyImages;
};


// #pragma mark -


image_id
ThreadImage::ID() const
{
	return fImage->ID();
}


bool
ThreadImage::ContainsAddress(addr_t address) const
{
	return fImage->ContainsAddress(address);
}


Image*
ThreadImage::GetImage() const
{
	return fImage;
}


int64
ThreadImage::TotalHits() const
{
	return fTotalHits;
}


// #pragma mark -


thread_id
Thread::ID() const
{
	return fInfo.thread;
}


const char*
Thread::Name() const
{
	return fInfo.name;
}


addr_t*
Thread::Samples() const
{
	return fSamples;
}


Team*
Thread::GetTeam() const
{
	return fTeam;
}


ThreadProfileResult*
Thread::ProfileResult() const
{
	return fProfileResult;
}


status_t
Thread::AddImage(Image* image)
{
	return fProfileResult->AddImage(image);
}


void
Thread::RemoveImage(Image* image)
{
	fProfileResult->RemoveImage(image);
}


// #pragma mark - AbstractThreadProfileResult


template<typename ThreadImageType>
AbstractThreadProfileResult<ThreadImageType>::AbstractThreadProfileResult()
	:
	fImages(),
	fNewImages(),
	fOldImages(),
	fLazyImages(true)
{
}


template<typename ThreadImageType>
AbstractThreadProfileResult<ThreadImageType>::~AbstractThreadProfileResult()
{
	while (ThreadImageType* image = fImages.RemoveHead())
		delete image;
	while (ThreadImageType* image = fOldImages.RemoveHead())
		delete image;
}


template<typename ThreadImageType>
void
AbstractThreadProfileResult<ThreadImageType>::SetLazyImages(bool lazy)
{
	fLazyImages = lazy;
}


template<typename ThreadImageType>
status_t
AbstractThreadProfileResult<ThreadImageType>::AddImage(Image* image)
{
	ThreadImageType* threadImage = CreateThreadImage(image);
	if (threadImage == NULL)
		return B_NO_MEMORY;

	status_t error = threadImage->Init();
	if (error != B_OK) {
		delete threadImage;
		return error;
	}

	if (fLazyImages)
		fNewImages.Add(threadImage);
	else
		fImages.Add(threadImage);

	return B_OK;
}


template<typename ThreadImageType>
void
AbstractThreadProfileResult<ThreadImageType>::RemoveImage(Image* image)
{
	typename ImageList::Iterator it = fImages.GetIterator();
	while (ThreadImageType* threadImage = it.Next()) {
		if (threadImage->GetImage() == image) {
			it.Remove();
			if (threadImage->TotalHits() > 0)
				fOldImages.Add(threadImage);
			else
				delete threadImage;
			break;
		}
	}
}


template<typename ThreadImageType>
void
AbstractThreadProfileResult<ThreadImageType>::SynchronizeImages(int32 event)
{
	// remove obsolete images
	typename ImageList::Iterator it = fImages.GetIterator();
	while (ThreadImageType* image = it.Next()) {
		int32 deleted = image->GetImage()->DeletionEvent();
		if (deleted >= 0 && event >= deleted) {
			it.Remove();
			if (image->TotalHits() > 0)
				fOldImages.Add(image);
			else
				delete image;
		}
	}

	// add new images
	it = fNewImages.GetIterator();
	while (ThreadImageType* image = it.Next()) {
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


template<typename ThreadImageType>
ThreadImageType*
AbstractThreadProfileResult<ThreadImageType>::FindImage(addr_t address) const
{
	typename ImageList::ConstIterator it = fImages.GetIterator();
	while (ThreadImageType* image = it.Next()) {
		if (image->ContainsAddress(address))
			return image;
	}
	return NULL;
}


template<typename ThreadImageType>
int32
AbstractThreadProfileResult<ThreadImageType>::GetHitImages(
	ThreadImageType** images) const
{
	int32 imageCount = 0;

	typename ImageList::ConstIterator it = fOldImages.GetIterator();
	while (ThreadImageType* image = it.Next()) {
		if (image->TotalHits() > 0)
			images[imageCount++] = image;
	}

	it = fImages.GetIterator();
	while (ThreadImageType* image = it.Next()) {
		if (image->TotalHits() > 0)
			images[imageCount++] = image;
	}

	return imageCount;
}

#endif	// THREAD_H
