/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef CALLGRIND_THREAD_PROFILE_RESULT_H
#define CALLGRIND_THREAD_PROFILE_RESULT_H

#include <stdio.h>

#include "Thread.h"


class CallgrindThreadImage;


struct CallgrindCalledFunction {
	CallgrindCalledFunction*	next;
	CallgrindThreadImage*		image;
	int32						function;
	int64						hits;

	CallgrindCalledFunction(CallgrindThreadImage* image, int32 function)
		:
		next(NULL),
		image(image),
		function(function),
		hits(0)
	{
	}
};


struct CallgrindFunction {
	int64						hits;
	CallgrindCalledFunction*	calledFunctions;
	int32						outputIndex;
									// index when generating the output file
};


class CallgrindThreadImage : public ThreadImage,
	public DoublyLinkedListLinkImpl<CallgrindThreadImage> {
public:
								CallgrindThreadImage(Image* image);
	virtual						~CallgrindThreadImage();

	virtual	status_t			Init();

	inline	void				AddSymbolHit(int32 symbolIndex,
									CallgrindThreadImage* calledImage,
									int32 calledSymbol);

	inline	CallgrindFunction*	Functions() const;

	inline	int32				OutputIndex() const;
	inline	void				SetOutputIndex(int32 index);

private:
			CallgrindFunction*	fFunctions;
			int32				fOutputIndex;
};


class CallgrindThreadProfileResult
	: public AbstractThreadProfileResult<CallgrindThreadImage> {
public:
								CallgrindThreadProfileResult();

	virtual	void				AddSamples(addr_t* samples,
									int32 sampleCount);
	virtual	void				AddDroppedTicks(int32 dropped);
	virtual	void				PrintResults();

	virtual CallgrindThreadImage* CreateThreadImage(Image* image);

private:
			void				_PrintFunction(FILE* out,
									CallgrindThreadImage* image,
									int32 functionIndex, bool called);
private:
			int64				fTotalTicks;
			int64				fUnkownTicks;
			int64				fDroppedTicks;
			int32				fNextImageOutputIndex;
			int32				fNextFunctionOutputIndex;
};


#endif	// CALLGRIND_THREAD_PROFILE_RESULT_H
