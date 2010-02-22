/*
 * Copyright 2008-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef BASIC_PROFILE_RESULT_H
#define BASIC_PROFILE_RESULT_H


#include "ProfileResult.h"


class BasicImageProfileResult : public ImageProfileResult,
	public DoublyLinkedListLinkImpl<BasicImageProfileResult> {
public:
								BasicImageProfileResult(SharedImage* image,
									image_id id);
	virtual						~BasicImageProfileResult();

			status_t			Init();

	inline	bool				AddHit(addr_t address);
	inline	void				AddUnknownHit();
	inline	void				AddSymbolHit(int32 symbolIndex);
	inline	void				AddImageHit();

	inline	const int64*		SymbolHits() const;
	inline	int64				UnknownHits() const;

private:
			int64*				fSymbolHits;
			int64				fUnknownHits;
};


class BasicProfileResult : public ProfileResult {
public:
								BasicProfileResult();

	virtual	void				AddDroppedTicks(int32 dropped);
	virtual	void				PrintResults(
									ImageProfileResultContainer* container);

	virtual status_t			GetImageProfileResult(SharedImage* image,
									image_id id,
									ImageProfileResult*& _imageResult);

protected:
			int64				fTotalTicks;
			int64				fUnkownTicks;
			int64				fDroppedTicks;
			int64				fTotalSampleCount;
};


class InclusiveProfileResult : public BasicProfileResult {
public:
	virtual	void				AddSamples(
									ImageProfileResultContainer* container,
									addr_t* samples, int32 sampleCount);
};


class ExclusiveProfileResult : public BasicProfileResult {
public:
	virtual	void				AddSamples(
									ImageProfileResultContainer* container,
									addr_t* samples, int32 sampleCount);
};


#endif	// BASIC_PROFILE_RESULT_H
