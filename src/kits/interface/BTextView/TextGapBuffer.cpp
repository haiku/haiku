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
//	File Name:		TextGapBuffer.cpp
//	Author:			Marc Flerackers (mflerackers@androme.be)
//	Description:	Text storage used by BTextView
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <cstdlib>
#include <cstring>

// System Includes -------------------------------------------------------------
#include <File.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "TextGapBuffer.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
_BTextGapBuffer_::_BTextGapBuffer_()
	:	fExtraCount(2048),
		fItemCount(0),
		fBuffer(NULL),
		fBufferCount(fExtraCount + fItemCount),
		fGapIndex(fItemCount),
		fGapCount(fBufferCount - fGapIndex),
		fScratchBuffer(NULL),
		fScratchSize(0),
		fPasswordMode(false)
{
	fBuffer = (char *)malloc(fExtraCount + fItemCount);
	fScratchBuffer = NULL;
}
//------------------------------------------------------------------------------
_BTextGapBuffer_::~_BTextGapBuffer_()
{
	free(fBuffer);
	free(fScratchBuffer);
}
//------------------------------------------------------------------------------
void
_BTextGapBuffer_::InsertText(const char *inText, int32 inNumItems,
								  int32 inAtIndex)
{
	if (inNumItems < 1)
		return;
	
	inAtIndex = (inAtIndex > fItemCount) ? fItemCount : inAtIndex;
	inAtIndex = (inAtIndex < 0) ? 0 : inAtIndex;
		
	if (inAtIndex != fGapIndex)
		MoveGapTo(inAtIndex);
	
	if (fGapCount < inNumItems)
		SizeGapTo(inNumItems + fExtraCount);
		
	memcpy(fBuffer + fGapIndex, inText, inNumItems);
	
	fGapCount -= inNumItems;
	fGapIndex += inNumItems;
	fItemCount += inNumItems;
}
//------------------------------------------------------------------------------
void
_BTextGapBuffer_::InsertText(BFile *file, int32 fileOffset, int32 inNumItems, int32 inAtIndex)
{
	off_t fileSize;

	if (file->GetSize(&fileSize) != B_OK
		|| !file->IsReadable())
		return;
	
	// Clamp the text length to the file size
	fileSize -= fileOffset;

	if (fileSize < inNumItems)
		inNumItems = fileSize;

	if (inNumItems < 1)
		return;

	inAtIndex = (inAtIndex > fItemCount) ? fItemCount : inAtIndex;
	inAtIndex = (inAtIndex < 0) ? 0 : inAtIndex;
		
	if (inAtIndex != fGapIndex)
		MoveGapTo(inAtIndex);
	
	if (fGapCount < inNumItems)
		SizeGapTo(inNumItems + fExtraCount);
	
	// Finally, read the data and put it into the buffer
	if (file->ReadAt(fileOffset, fBuffer + fGapIndex, inNumItems) > 0) {
		fGapCount -= inNumItems;
		fGapIndex += inNumItems;
		fItemCount += inNumItems;
	}
}
//------------------------------------------------------------------------------
void
_BTextGapBuffer_::RemoveRange(int32 start, int32 end)
{
	long inAtIndex = start;
	long inNumItems = end - start;
	
	if (inNumItems < 1)
		return;
	
	inAtIndex = (inAtIndex > fItemCount - 1) ? (fItemCount - 1) : inAtIndex;
	inAtIndex = (inAtIndex < 0) ? 0 : inAtIndex;
	
	MoveGapTo(inAtIndex);
	
	fGapCount += inNumItems;
	fItemCount -= inNumItems;
	
	if (fGapCount > fExtraCount)
		SizeGapTo(fExtraCount);	
}
//------------------------------------------------------------------------------
void
_BTextGapBuffer_::MoveGapTo(int32 toIndex)
{
	if (toIndex == fGapIndex)
		return;
	
	long gapEndIndex = fGapIndex + fGapCount;
	long srcIndex = 0;
	long dstIndex = 0;
	long count = 0;
	if (toIndex > fGapIndex) {
		long trailGapCount = fBufferCount - gapEndIndex;
		srcIndex = toIndex + (gapEndIndex - toIndex);
		dstIndex =  fGapIndex;
		count = fGapCount + (toIndex - srcIndex);
		count = (count > trailGapCount) ? trailGapCount : count;
	}
	else {
		srcIndex = toIndex;
		dstIndex = toIndex + (gapEndIndex - fGapIndex);
		count = gapEndIndex - dstIndex;
	}
	
	if (count > 0)
		memmove(fBuffer + dstIndex, fBuffer + srcIndex, count);	

	fGapIndex = toIndex;
}
//------------------------------------------------------------------------------
void
_BTextGapBuffer_::SizeGapTo(long inCount)
{
	if (inCount == fGapCount)
		return;
		
	fBuffer = (char *)realloc(fBuffer, fItemCount + inCount);
	memmove(fBuffer + fGapIndex + inCount, 
			fBuffer + fGapIndex + fGapCount, 
			fBufferCount - (fGapIndex + fGapCount));

	fGapCount = inCount;
	fBufferCount = fItemCount + fGapCount;
}
//------------------------------------------------------------------------------
const char *
_BTextGapBuffer_::GetString(int32 fromOffset, int32 numChars)
{
	char *result = "";
	
	if (numChars < 1)
		return (result);
	
	bool isStartBeforeGap = (fromOffset < fGapIndex);
	bool isEndBeforeGap = ((fromOffset + numChars - 1) < fGapIndex);

	if (isStartBeforeGap == isEndBeforeGap) {
		result = fBuffer + fromOffset;
		if (!isStartBeforeGap)
			result += fGapCount;
	
	} else {
		if (fScratchSize < numChars) {
			fScratchBuffer = (char *)realloc(fScratchBuffer, numChars);
			fScratchSize = numChars;
		}
		
		for (long i = 0; i < numChars; i++)
			fScratchBuffer[i] = (*this)[fromOffset + i];

		result = fScratchBuffer;
	}
	
	return result;
}
//------------------------------------------------------------------------------
bool 
_BTextGapBuffer_::FindChar(char inChar, long fromIndex, long *ioDelta)
{
	long numChars = *ioDelta;
	for (long i = 0; i < numChars; i++) {
		if (((*this)[fromIndex + i] & 0xc0) == 0x80)
			continue;
		if ((*this)[fromIndex + i] == inChar) {
			*ioDelta = i;
			return (true);
		}
	}
	
	return false;
}
//------------------------------------------------------------------------------
const char *
_BTextGapBuffer_::Text()
{
	MoveGapTo(fItemCount);
	fBuffer[fItemCount] = '\0';
	
	return fBuffer;
}
//------------------------------------------------------------------------------
/*char *_BTextGapBuffer_::RealText()
{
	return fText;
}*/
//------------------------------------------------------------------------------
void
_BTextGapBuffer_::GetString(int32 offset, int32 length, char *buffer)
{
	if (buffer == NULL)
		return;

	int32 textLen = Length();
	
	if (offset < 0 || offset > (textLen - 1) || length < 1) {
		buffer[0] = '\0';
		return;
	}
	
	length = ((offset + length) > textLen) ? textLen - offset : length;

	bool isStartBeforeGap = (offset < fGapIndex);
	bool isEndBeforeGap = ((offset + length - 1) < fGapIndex);

	if (isStartBeforeGap == isEndBeforeGap) {
		char *source = fBuffer + offset;
		if (!isStartBeforeGap)
			source += fGapCount;
	
		memcpy(buffer, source, length);
	
	} else {		
		// if we are here, it can only be that start is before gap,
		// and the end is after gap.

		int32 beforeLen = fGapIndex - offset;
		int32 afterLen = length - beforeLen;

		memcpy(buffer, fBuffer + offset, beforeLen);
		memcpy(buffer + beforeLen, fBuffer + fGapIndex, afterLen);
			
	}
	
	buffer[length] = '\0';
}
//------------------------------------------------------------------------------
char 
_BTextGapBuffer_::RealCharAt(int32 offset) const
{
	return (offset < fGapIndex) ? fBuffer[offset] : fBuffer[offset + fGapCount];
}
//------------------------------------------------------------------------------
bool
_BTextGapBuffer_::PasswordMode() const
{
	return fPasswordMode;
}
//------------------------------------------------------------------------------
void
_BTextGapBuffer_::SetPasswordMode(bool state)
{
	fPasswordMode = state;
}
/*
 * $Log $
 *
 * $Id  $
 *
 */
