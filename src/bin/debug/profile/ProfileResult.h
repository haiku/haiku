/*
 * Copyright 2008-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PROFILE_RESULT_H
#define PROFILE_RESULT_H


#include <Referenceable.h>

#include <util/DoublyLinkedList.h>

#include "SharedImage.h"


class ProfiledEntity;
class Team;


class ImageProfileResult : public BReferenceable {
public:
								ImageProfileResult(SharedImage* image,
									image_id id);
	virtual						~ImageProfileResult();

	virtual	status_t			Init();

	inline	image_id			ID() const;
	inline	SharedImage*		GetImage() const;

	inline	int64				TotalHits() const;

protected:
			SharedImage*		fImage;
			int64				fTotalHits;
			image_id			fImageID;
};


class ImageProfileResultContainer {
public:
			class Visitor;


public:
	virtual						~ImageProfileResultContainer();

	virtual	int32				CountImages() const = 0;
	virtual	ImageProfileResult*	VisitImages(Visitor& visitor) const = 0;
	virtual	ImageProfileResult*	FindImage(addr_t address,
									addr_t& _loadDelta) const = 0;
};


class ImageProfileResultContainer::Visitor {
public:
	virtual						~Visitor();

	virtual	bool				VisitImage(ImageProfileResult* image) = 0;
};


class ProfileResult : public BReferenceable {
public:
								ProfileResult();
	virtual						~ProfileResult();

	virtual	status_t			Init(ProfiledEntity* entity);

			ProfiledEntity*		Entity() const	{ return fEntity; }

	virtual	void				SetInterval(bigtime_t interval);

	virtual	void				AddSamples(
									ImageProfileResultContainer* container,
									addr_t* samples,
									int32 sampleCount) = 0;
	virtual	void				AddDroppedTicks(int32 dropped) = 0;
	virtual	void				PrintResults(
									ImageProfileResultContainer* container) = 0;

	virtual status_t			GetImageProfileResult(SharedImage* image,
									image_id id,
									ImageProfileResult*& _imageResult) = 0;

protected:
			template<typename ImageProfileResultType>
			int32				GetHitImages(
									ImageProfileResultContainer* container,
									ImageProfileResultType** images) const;

protected:
			ProfiledEntity*		fEntity;
			bigtime_t			fInterval;
};


// #pragma mark - ImageProfileResult


image_id
ImageProfileResult::ID() const
{
	return fImageID;
}


SharedImage*
ImageProfileResult::GetImage() const
{
	return fImage;
}


int64
ImageProfileResult::TotalHits() const
{
	return fTotalHits;
}


// #pragma mark - ProfileResult


template<typename ImageProfileResultType>
int32
ProfileResult::GetHitImages(ImageProfileResultContainer* container,
	ImageProfileResultType** images) const
{
	struct Visitor : ImageProfileResultContainer::Visitor {
		ImageProfileResultType**	images;
		int32						imageCount;

		virtual bool VisitImage(ImageProfileResult* image)
		{
			if (image->TotalHits() > 0) {
				images[imageCount++]
					= static_cast<ImageProfileResultType*>(image);
			}
			return false;
		}
	} visitor;

	visitor.images = images;
	visitor.imageCount = 0;

	container->VisitImages(visitor);

	return visitor.imageCount;
}


#endif	// PROFILE_RESULT_H
