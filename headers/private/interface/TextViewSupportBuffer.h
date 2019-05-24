/*
 * Copyright 2001-2010, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */

#ifndef __TEXT_VIEW_SUPPORT_BUFFER__H__
#define __TEXT_VIEW_SUPPORT_BUFFER__H__

#include <cstdlib>
#include <cstring>

#include <OS.h>
#include <SupportDefs.h>


// _BTextViewSupportBuffer_ class ----------------------------------------------
template <class T>
class _BTextViewSupportBuffer_ {

public:
				_BTextViewSupportBuffer_(int32 inExtraCount = 0, int32 inCount = 0);
virtual			~_BTextViewSupportBuffer_();

		void	InsertItemsAt(int32 inNumItems, int32 inAtIndex, const T* inItem);
		void	RemoveItemsAt(int32 inNumItems, int32 inAtIndex);

		int32	ItemCount() const;

protected:
		int32	fExtraCount;
		int32	fItemCount;
		int32	fBufferCount;
		T*		fBuffer;
};


template <class T>
_BTextViewSupportBuffer_<T>::_BTextViewSupportBuffer_(int32 inExtraCount,
													  int32 inCount)
	:	fExtraCount(inExtraCount),
		fItemCount(inCount),
		fBufferCount(fExtraCount + fItemCount),
		fBuffer(NULL)
{
	fBuffer = (T*)calloc(fExtraCount + fItemCount, sizeof(T));
}


template <class T>
_BTextViewSupportBuffer_<T>::~_BTextViewSupportBuffer_()
{
	free(fBuffer);
}


template <class T>
void
_BTextViewSupportBuffer_<T>::InsertItemsAt(int32 inNumItems,
												int32 inAtIndex,
												const T* inItem)
{
	if (inNumItems < 1)
		return;

	inAtIndex = (inAtIndex > fItemCount) ? fItemCount : inAtIndex;
	inAtIndex = (inAtIndex < 0) ? 0 : inAtIndex;

	int32 delta = inNumItems * sizeof(T);
	int32 logSize = fItemCount * sizeof(T);
	if ((logSize + delta) >= fBufferCount) {
		fBufferCount = logSize + delta + (fExtraCount * sizeof(T));
		fBuffer = (T*)realloc((void*)fBuffer, fBufferCount);
		if (fBuffer == NULL)
			debugger("InsertItemsAt(): reallocation failed");
	}

	T* loc = fBuffer + inAtIndex;
	memmove((void*)(loc + inNumItems), (void*)loc,
		(fItemCount - inAtIndex) * sizeof(T));
	memcpy((void*)loc, (void*)inItem, delta);

	fItemCount += inNumItems;
}


template <class T>
void
_BTextViewSupportBuffer_<T>::RemoveItemsAt(int32 inNumItems,
												int32 inAtIndex)
{
	if (inNumItems < 1)
		return;

	inAtIndex = (inAtIndex > fItemCount - 1) ? (fItemCount - 1) : inAtIndex;
	inAtIndex = (inAtIndex < 0) ? 0 : inAtIndex;

	T* loc = fBuffer + inAtIndex;
	memmove(loc, loc + inNumItems,
			(fItemCount - (inNumItems + inAtIndex)) * sizeof(T));

	int32 delta = inNumItems * sizeof(T);
	int32 logSize = fItemCount * sizeof(T);
	uint32 extraSize = fBufferCount - (logSize - delta);
	if (extraSize > (fExtraCount * sizeof(T))) {
		fBufferCount = (logSize - delta) + (fExtraCount * sizeof(T));
		fBuffer = (T*)realloc(fBuffer, fBufferCount);
		if (fBuffer == NULL)
			debugger("RemoveItemsAt(): reallocation failed");
	}

	fItemCount -= inNumItems;
}


template<class T>
inline int32
_BTextViewSupportBuffer_<T>::ItemCount() const
{
	return fItemCount;
}


#endif // __TEXT_VIEW_SUPPORT_BUFFER__H__
