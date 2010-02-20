/*
 * Copyright 2008-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef CALLGRIND_PROFILE_RESULT_H
#define CALLGRIND_PROFILE_RESULT_H


#include <stdio.h>

#include "ProfileResult.h"


class CallgrindProfileResultImage;


struct CallgrindCalledFunction {
	CallgrindCalledFunction*		next;
	CallgrindProfileResultImage*	image;
	int32							function;
	int64							hits;

	CallgrindCalledFunction(CallgrindProfileResultImage* image, int32 function)
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


class CallgrindProfileResultImage : public ProfileResultImage,
	public DoublyLinkedListLinkImpl<CallgrindProfileResultImage> {
public:
								CallgrindProfileResultImage(Image* image);
	virtual						~CallgrindProfileResultImage();

	virtual	status_t			Init();

	inline	void				AddSymbolHit(int32 symbolIndex,
									CallgrindProfileResultImage* calledImage,
									int32 calledSymbol);

	inline	CallgrindFunction*	Functions() const;

	inline	int32				OutputIndex() const;
	inline	void				SetOutputIndex(int32 index);

private:
			CallgrindFunction*	fFunctions;
			int32				fOutputIndex;
};


class CallgrindProfileResult
	: public AbstractProfileResult<CallgrindProfileResultImage> {
public:
								CallgrindProfileResult();

	virtual	void				AddSamples(addr_t* samples,
									int32 sampleCount);
	virtual	void				AddDroppedTicks(int32 dropped);
	virtual	void				PrintResults();

	virtual CallgrindProfileResultImage* CreateProfileResultImage(Image* image);

private:
			void				_PrintFunction(FILE* out,
									CallgrindProfileResultImage* image,
									int32 functionIndex, bool called);
private:
			int64				fTotalTicks;
			int64				fUnkownTicks;
			int64				fDroppedTicks;
			int32				fNextImageOutputIndex;
			int32				fNextFunctionOutputIndex;
};


#endif	// CALLGRIND_PROFILE_RESULT_H
