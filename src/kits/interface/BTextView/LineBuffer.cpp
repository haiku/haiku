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
//	File Name:		LineBuffer.cpp
//	Author:			Marc Flerackers (mflerackers@androme.be)
//	Description:	Line storage used by BTextView
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <string.h>

// System Includes -------------------------------------------------------------
#include "LineBuffer.h"

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
_BLineBuffer_::_BLineBuffer_()
	:	fBlockSize(20),
		fItemCount(2),
		fPhysicalSize(22),
		fObjectList(NULL)
{
	fObjectList = new STELine[fPhysicalSize];

	memset(fObjectList, 0, fPhysicalSize * sizeof(STELine));
}
//------------------------------------------------------------------------------
_BLineBuffer_::~_BLineBuffer_()
{
	delete[] fObjectList;
}
//------------------------------------------------------------------------------
void _BLineBuffer_::InsertLine(STELine *line, int32 index)
{
	if (fItemCount == fPhysicalSize - 1)
	{
		STELine *new_list = new STELine[fPhysicalSize + fBlockSize];

		memcpy(new_list, fObjectList, fPhysicalSize * sizeof(STELine));
		delete fObjectList;
		fObjectList = new_list;

		fPhysicalSize += fBlockSize;
	}

	if (index < fItemCount)
		memmove(fObjectList + index + 1, fObjectList + index,
			(fItemCount - index) * sizeof(STELine));
	memcpy(fObjectList + index, line, sizeof(STELine));

	fItemCount++;
}
//------------------------------------------------------------------------------
void _BLineBuffer_::RemoveLines(int32 index, int32 count)
{
	memmove(fObjectList + index, fObjectList + index + count,
		(fItemCount - index - count) * sizeof(STELine));

	fItemCount -= count;
}
//------------------------------------------------------------------------------
void _BLineBuffer_::RemoveLineRange(int32 from, int32 to)
{
	int32 linefrom = OffsetToLine(from);
	int32 lineto = OffsetToLine(to);

	if (linefrom < lineto)
		RemoveLines(linefrom + 1, lineto - linefrom);

	BumpOffset(linefrom + 1, from - to);
}
//------------------------------------------------------------------------------
int32 _BLineBuffer_::OffsetToLine(int32 offset) const
{
	if (offset == 0)
		return 0;

	for (int i = 0; i < fItemCount; i++)
	{
		if (offset < fObjectList[i].fOffset)
			return i - 1;
	}

	return fItemCount - 2;
}
//------------------------------------------------------------------------------
int32 _BLineBuffer_::PixelToLine(float height) const
{
	for (int i = 0; i < fItemCount; i++)
	{
		if (height < fObjectList[i].fHeight)
			return i - 1;
	}

	return fItemCount - 2;
}
//------------------------------------------------------------------------------
void _BLineBuffer_::BumpOffset(int32 line, int32 offset)
{
	for (int i = line; i < fItemCount; i++)
		fObjectList[i].fOffset += offset;
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */
