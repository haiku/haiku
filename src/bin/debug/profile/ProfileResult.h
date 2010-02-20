/*
 * Copyright 2008-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PROFILE_RESULT_H
#define PROFILE_RESULT_H


#include <util/DoublyLinkedList.h>

#include "Image.h"


class ProfiledEntity;
class Team;


class ProfileResultImage {
public:
								ProfileResultImage(Image* image);
	virtual						~ProfileResultImage();

	virtual	status_t			Init();

	inline	image_id			ID() const;
	inline	Image*				GetImage() const;

	inline	bool				ContainsAddress(addr_t address) const;

	inline	int64				TotalHits() const;

protected:
			Image*				fImage;
			int64				fTotalHits;
};


class ProfileResult {
public:
								ProfileResult();
	virtual						~ProfileResult();

	virtual	status_t			Init(ProfiledEntity* entity);

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
			ProfiledEntity*		fEntity;
			bigtime_t			fInterval;
};


template<typename ProfileResultImageType>
class AbstractProfileResult : public ProfileResult {
public:
								AbstractProfileResult();
	virtual						~AbstractProfileResult();

	virtual	void				SetLazyImages(bool lazy);

	virtual	status_t			AddImage(Image* image);
	virtual	void				RemoveImage(Image* image);
	virtual	void				SynchronizeImages(int32 event);

			ProfileResultImageType* FindImage(addr_t address) const;
			int32				GetHitImages(
									ProfileResultImageType** images) const;

	virtual ProfileResultImageType*	CreateProfileResultImage(Image* image) = 0;

protected:
	typedef DoublyLinkedList<ProfileResultImageType>	ImageList;

			ImageList			fImages;
			ImageList			fNewImages;
			ImageList			fOldImages;
			bool				fLazyImages;
};


// #pragma mark -


image_id
ProfileResultImage::ID() const
{
	return fImage->ID();
}


bool
ProfileResultImage::ContainsAddress(addr_t address) const
{
	return fImage->ContainsAddress(address);
}


Image*
ProfileResultImage::GetImage() const
{
	return fImage;
}


int64
ProfileResultImage::TotalHits() const
{
	return fTotalHits;
}


// #pragma mark - AbstractProfileResult


template<typename ProfileResultImageType>
AbstractProfileResult<ProfileResultImageType>::AbstractProfileResult()
	:
	fImages(),
	fNewImages(),
	fOldImages(),
	fLazyImages(true)
{
}


template<typename ProfileResultImageType>
AbstractProfileResult<ProfileResultImageType>::~AbstractProfileResult()
{
	while (ProfileResultImageType* image = fImages.RemoveHead())
		delete image;
	while (ProfileResultImageType* image = fOldImages.RemoveHead())
		delete image;
}


template<typename ProfileResultImageType>
void
AbstractProfileResult<ProfileResultImageType>::SetLazyImages(bool lazy)
{
	fLazyImages = lazy;
}


template<typename ProfileResultImageType>
status_t
AbstractProfileResult<ProfileResultImageType>::AddImage(Image* image)
{
	ProfileResultImageType* resultImage = CreateProfileResultImage(image);
	if (resultImage == NULL)
		return B_NO_MEMORY;

	status_t error = resultImage->Init();
	if (error != B_OK) {
		delete resultImage;
		return error;
	}

	if (fLazyImages)
		fNewImages.Add(resultImage);
	else
		fImages.Add(resultImage);

	return B_OK;
}


template<typename ProfileResultImageType>
void
AbstractProfileResult<ProfileResultImageType>::RemoveImage(Image* image)
{
	typename ImageList::Iterator it = fImages.GetIterator();
	while (ProfileResultImageType* resultImage = it.Next()) {
		if (resultImage->GetImage() == image) {
			it.Remove();
			if (resultImage->TotalHits() > 0)
				fOldImages.Add(resultImage);
			else
				delete resultImage;
			break;
		}
	}
}


template<typename ProfileResultImageType>
void
AbstractProfileResult<ProfileResultImageType>::SynchronizeImages(int32 event)
{
	// remove obsolete images
	typename ImageList::Iterator it = fImages.GetIterator();
	while (ProfileResultImageType* image = it.Next()) {
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
	while (ProfileResultImageType* image = it.Next()) {
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


template<typename ProfileResultImageType>
ProfileResultImageType*
AbstractProfileResult<ProfileResultImageType>::FindImage(addr_t address) const
{
	typename ImageList::ConstIterator it = fImages.GetIterator();
	while (ProfileResultImageType* image = it.Next()) {
		if (image->ContainsAddress(address))
			return image;
	}
	return NULL;
}


template<typename ProfileResultImageType>
int32
AbstractProfileResult<ProfileResultImageType>::GetHitImages(
	ProfileResultImageType** images) const
{
	int32 imageCount = 0;

	typename ImageList::ConstIterator it = fOldImages.GetIterator();
	while (ProfileResultImageType* image = it.Next()) {
		if (image->TotalHits() > 0)
			images[imageCount++] = image;
	}

	it = fImages.GetIterator();
	while (ProfileResultImageType* image = it.Next()) {
		if (image->TotalHits() > 0)
			images[imageCount++] = image;
	}

	return imageCount;
}


#endif	// PROFILE_RESULT_H
