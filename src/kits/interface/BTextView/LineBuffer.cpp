/*
 * Copyright 2001-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 */

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


int32
_BLineBuffer_::PixelToLine(float pixel) const
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
