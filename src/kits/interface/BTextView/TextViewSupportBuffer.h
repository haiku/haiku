//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
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
//	Author:			Marc Flerackers (mflerackers@androme.be)
//	Description:	Template class used to implement the various BTextView
//					buffer classes.
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <malloc.h>

// System Includes -------------------------------------------------------------
#include "SupportDefs.h"

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

// _BTextViewSupportBuffer_ class ----------------------------------------------
template <class T>
class _BTextViewSupportBuffer_ {

public:
				_BTextViewSupportBuffer_(int32 count, int32	blockSize);
virtual			~_BTextViewSupportBuffer_();

		void	InsertItemsAt(int32 offset, int32 count, const T *items);
		void	RemoveItemsAt(int32 offset, int32 count);

		int32	fBlockSize;
		int32	fCount;
		int32	fPhysicalSize;
		T		*fItems;
		int32	fReserved;
};
//------------------------------------------------------------------------------
template <class T>
_BTextViewSupportBuffer_<T>::_BTextViewSupportBuffer_(int32 count, int32 blockSize)
	:
		fBlockSize(blockSize),
		fCount(count),
		fPhysicalSize(count + blockSize),
		fItems(NULL)
{
	fItems = (T*)malloc(fPhysicalSize * sizeof(T));
}
//------------------------------------------------------------------------------
template <class T>
_BTextViewSupportBuffer_<T>::~_BTextViewSupportBuffer_()
{
	free(fItems);
}
//------------------------------------------------------------------------------
template <class T>
void _BTextViewSupportBuffer_<T>::InsertItemsAt(int32 offset, int32 count,
											  const T *items)
{
	if (fPhysicalSize < fCount + count)
	{
		int32 new_size = (((fCount + count) / fBlockSize) + 1) * fBlockSize;

		fItems = realloc(fItems, new_size);
	}

	memcpy(fItems + offset, items, count);
	fCount += count;
}
//------------------------------------------------------------------------------
template <class T>
void _BTextViewSupportBuffer_<T>::RemoveItemsAt(int32 offset, int32 count)
{

}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */
