/*
 * Copyright 2008-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef BASIC_PROFILE_RESULT_H
#define BASIC_PROFILE_RESULT_H


#include "ProfileResult.h"


class BasicProfileResultImage : public ProfileResultImage,
	public DoublyLinkedListLinkImpl<BasicProfileResultImage> {
public:
								BasicProfileResultImage(Image* image);
	virtual						~BasicProfileResultImage();

	virtual	status_t			Init();

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


class BasicProfileResult
	: public AbstractProfileResult<BasicProfileResultImage> {
public:
								BasicProfileResult();

	virtual	void				AddDroppedTicks(int32 dropped);
	virtual	void				PrintResults();

	virtual BasicProfileResultImage* CreateProfileResultImage(Image* image);

protected:
			int64				fTotalTicks;
			int64				fUnkownTicks;
			int64				fDroppedTicks;
			int64				fTotalSampleCount;
};


class InclusiveProfileResult : public BasicProfileResult {
public:
	virtual	void				AddSamples(addr_t* samples,
									int32 sampleCount);
};


class ExclusiveProfileResult : public BasicProfileResult {
public:
	virtual	void				AddSamples(addr_t* samples,
									int32 sampleCount);
};


#endif	// BASIC_PROFILE_RESULT_H
