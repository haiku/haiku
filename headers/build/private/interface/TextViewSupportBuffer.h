//------------------------------------------------------------------------------
//	Copyright (c) 2001-2004, Haiku, Inc.
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		TextViewSupportBuffer.h
//	Authors			Marc Flerackers (mflerackers@androme.be)
//					Stefano Ceccherini (burton666@libero.it)
//	Description:	Template class used to implement the various BTextView
//					buffer classes.
//------------------------------------------------------------------------------

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

		void	InsertItemsAt(int32 inNumItems, int32 inAtIndex, const T *inItem);
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
	fBuffer = (T *)calloc(fExtraCount + fItemCount, sizeof(T));
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
												const T *inItem)
{
	if (inNumItems < 1)
		return;
	
	inAtIndex = (inAtIndex > fItemCount) ? fItemCount : inAtIndex;
	inAtIndex = (inAtIndex < 0) ? 0 : inAtIndex;

	int32 delta = inNumItems * sizeof(T);
	int32 logSize = fItemCount * sizeof(T);
	if ((logSize + delta) >= fBufferCount) {
		fBufferCount = logSize + delta + (fExtraCount * sizeof(T));
		fBuffer = (T *)realloc(fBuffer, fBufferCount);
		if (fBuffer == NULL)
			debugger("InsertItemsAt(): reallocation failed");
	}
	
	T *loc = fBuffer + inAtIndex;
	memmove(loc + inNumItems, loc, (fItemCount - inAtIndex) * sizeof(T));
	memcpy(loc, inItem, delta);
	
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
	
	T *loc = fBuffer + inAtIndex;
	memmove(loc, loc + inNumItems, 
			(fItemCount - (inNumItems + inAtIndex)) * sizeof(T));
	
	int32 delta = inNumItems * sizeof(T);
	int32 logSize = fItemCount * sizeof(T);
	uint32 extraSize = fBufferCount - (logSize - delta);
	if (extraSize > (fExtraCount * sizeof(T))) {
		fBufferCount = (logSize - delta) + (fExtraCount * sizeof(T));
		fBuffer = (T *)realloc(fBuffer, fBufferCount);
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
