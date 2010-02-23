/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef SUMMARY_PROFILE_RESULT_H
#define SUMMARY_PROFILE_RESULT_H


#include <util/OpenHashTable.h>

#include "ProfileResult.h"


class SummaryImage {
public:
								SummaryImage(ImageProfileResult* result);
								~SummaryImage();

			ImageProfileResult*	Result() const	{ return fResult; }
			SharedImage*		GetImage() const { return fResult->GetImage(); }

			SummaryImage*&		HashNext()			{ return fHashNext; }

private:
			ImageProfileResult*	fResult;
			SummaryImage*		fHashNext;
};


struct SummaryImageHashDefinition {
	typedef SharedImage*	KeyType;
	typedef	SummaryImage	ValueType;

	size_t HashKey(SharedImage* key) const
	{
		return (addr_t)key / (2 * sizeof(void*));
	}

	size_t Hash(SummaryImage* value) const
	{
		return HashKey(value->GetImage());
	}

	bool Compare(SharedImage* key, SummaryImage* value) const
	{
		return value->GetImage() == key;
	}

	SummaryImage*& GetLink(SummaryImage* value) const
	{
		return value->HashNext();
	}
};


class SummaryProfileResult : public ProfileResult,
	private ImageProfileResultContainer {
public:
								SummaryProfileResult(ProfileResult* result);
	virtual						~SummaryProfileResult();

	virtual	status_t			Init(ProfiledEntity* entity);

	virtual	void				SetInterval(bigtime_t interval);

	virtual	void				AddSamples(
									ImageProfileResultContainer* container,
									addr_t* samples, int32 sampleCount);
	virtual	void				AddDroppedTicks(int32 dropped);
	virtual	void				PrintResults(
									ImageProfileResultContainer* container);

	virtual status_t			GetImageProfileResult(SharedImage* image,
									image_id id,
									ImageProfileResult*& _imageResult);

			void				PrintSummaryResults();

private:
			typedef BOpenHashTable<SummaryImageHashDefinition> ImageTable;

private:
	// ImageProfileResultContainer
	virtual	int32				CountImages() const;
	virtual	ImageProfileResult*	VisitImages(Visitor& visitor) const;
	virtual	ImageProfileResult*	FindImage(addr_t address,
									addr_t& _loadDelta) const;

private:
			ProfileResult*		fResult;
			ImageTable			fImages;
			image_id			fNextImageID;
};


#endif	// SUMMARY_PROFILE_RESULT_H
