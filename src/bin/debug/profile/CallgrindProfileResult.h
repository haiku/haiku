/*
 * Copyright 2008-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef CALLGRIND_PROFILE_RESULT_H
#define CALLGRIND_PROFILE_RESULT_H


#include <stdio.h>

#include "ProfileResult.h"


class CallgrindImageProfileResult;


struct CallgrindCalledFunction {
	CallgrindCalledFunction*		next;
	CallgrindImageProfileResult*	image;
	int32							function;
	int64							hits;

	CallgrindCalledFunction(CallgrindImageProfileResult* image, int32 function)
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


class CallgrindImageProfileResult : public ImageProfileResult,
	public DoublyLinkedListLinkImpl<CallgrindImageProfileResult> {
public:
								CallgrindImageProfileResult(SharedImage* image,
									image_id id);
	virtual						~CallgrindImageProfileResult();

			status_t			Init();

	inline	void				AddSymbolHit(int32 symbolIndex,
									CallgrindImageProfileResult* calledImage,
									int32 calledSymbol);

	inline	CallgrindFunction*	Functions() const;

	inline	int32				OutputIndex() const;
	inline	void				SetOutputIndex(int32 index);

private:
			CallgrindFunction*	fFunctions;
			int32				fOutputIndex;
};


class CallgrindProfileResult : public ProfileResult {
public:
								CallgrindProfileResult();

	virtual	void				AddSamples(
									ImageProfileResultContainer* container,
									addr_t* samples, int32 sampleCount);
	virtual	void				AddDroppedTicks(int32 dropped);
	virtual	void				PrintResults(
									ImageProfileResultContainer* container);

	virtual status_t			GetImageProfileResult(SharedImage* image,
									image_id id,
									ImageProfileResult*& _imageResult);

private:
			void				_PrintFunction(FILE* out,
									CallgrindImageProfileResult* image,
									int32 functionIndex, bool called);
private:
			int64				fTotalTicks;
			int64				fUnkownTicks;
			int64				fDroppedTicks;
			int32				fNextImageOutputIndex;
			int32				fNextFunctionOutputIndex;
};


#endif	// CALLGRIND_PROFILE_RESULT_H
