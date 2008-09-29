/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef BASIC_THREAD_PROFILE_RESULT_H
#define BASIC_THREAD_PROFILE_RESULT_H

#include "Thread.h"


class BasicThreadImage : public ThreadImage,
	public DoublyLinkedListLinkImpl<BasicThreadImage> {
public:
								BasicThreadImage(Image* image);
	virtual						~BasicThreadImage();

	virtual	status_t			Init();

	inline	void				AddHit(addr_t address);
	inline	void				AddSymbolHit(int32 symbolIndex);
	inline	void				AddImageHit();

	inline	const int64*		SymbolHits() const;
	inline	int64				UnknownHits() const;

private:
			int64*				fSymbolHits;
			int64				fUnknownHits;
};


class BasicThreadProfileResult
	: public AbstractThreadProfileResult<BasicThreadImage> {
public:
								BasicThreadProfileResult();

	virtual	void				AddDroppedTicks(int32 dropped);
	virtual	void				PrintResults();

	virtual BasicThreadImage*	CreateThreadImage(Image* image);

protected:
			int64				fTotalTicks;
			int64				fUnkownTicks;
			int64				fDroppedTicks;
			int64				fTotalSampleCount;
};


class InclusiveThreadProfileResult : public BasicThreadProfileResult {
public:
	virtual	void				AddSamples(addr_t* samples,
									int32 sampleCount);
};


class ExclusiveThreadProfileResult : public BasicThreadProfileResult {
public:
	virtual	void				AddSamples(addr_t* samples,
									int32 sampleCount);
};


#endif	// BASIC_THREAD_PROFILE_RESULT_H
