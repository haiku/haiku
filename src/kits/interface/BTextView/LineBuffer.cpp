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
//	File Name:		LineBuffer.cpp
//	Author:			Marc Flerackers (mflerackers@androme.be)
//	Description:	Line storage used by BTextView
//------------------------------------------------------------------------------

#include "LineBuffer.h"


_BLineBuffer_::_BLineBuffer_()
	:	_BTextViewSupportBuffer_<STELine>(20, 2)
{
}


_BLineBuffer_::~_BLineBuffer_()
{
}


void
_BLineBuffer_::InsertLine(STELine *inLine, int32 index)
{
	InsertItemsAt(1, index, inLine);
}


void
_BLineBuffer_::RemoveLines(int32 index, int32 count)
{
	RemoveItemsAt(count, index);
}


void
_BLineBuffer_::RemoveLineRange(int32 fromOffset, int32 toOffset)
{
	int32 fromLine = OffsetToLine(fromOffset);
	int32 toLine = OffsetToLine(toOffset);

	int32 count = toLine - fromLine;
	if (count > 0)
		RemoveLines(fromLine + 1, count);

	BumpOffset(fromOffset - toOffset, fromLine + 1);
}


int32
_BLineBuffer_::OffsetToLine(int32 offset) const
{
	int32 minIndex = 0;
	int32 maxIndex = fItemCount - 1;
	int32 index = 0;
	
	while (minIndex < maxIndex) {
		index = (minIndex + maxIndex) >> 1;
		if (offset >= fBuffer[index].offset) {
			if (offset < fBuffer[index + 1].offset)
				break;
			else
				minIndex = index + 1;
		} else
			maxIndex = index;
	}
	
	return index;
}


int32 _BLineBuffer_::PixelToLine(float pixel) const
{
	int32 minIndex = 0;
	int32 maxIndex = fItemCount - 1;
	int32 index = 0;
	
	while (minIndex < maxIndex) {
		index = (minIndex + maxIndex) >> 1;
		if (pixel >= fBuffer[index].origin) {
			if (pixel < fBuffer[index + 1].origin)
				break;
			else
				minIndex = index + 1;
		} else
			maxIndex = index;
	}
	
	return index;
}


void
_BLineBuffer_::BumpOrigin(float delta, long index)
{	
	for (long i = index; i < fItemCount; i++)
		fBuffer[i].origin += delta;
}


void
_BLineBuffer_::BumpOffset(int32 delta, int32 index)
{
	for (long i = index; i < fItemCount; i++)
		fBuffer[i].offset += delta;
}
