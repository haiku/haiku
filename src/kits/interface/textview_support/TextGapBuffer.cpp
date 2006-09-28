/*
 * Copyright 2001-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Stefano Ceccherini (burton666@libero.it)
 */

#include <cstdlib>
#include <cstring>

#include <utf8_functions.h>

#include <File.h>
#include <InterfaceDefs.h> // for B_UTF8_BULLET

#include "TextGapBuffer.h"


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


_BTextGapBuffer_::~_BTextGapBuffer_()
{
	free(fBuffer);
	free(fScratchBuffer);
}


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
	} else {
		srcIndex = toIndex;
		dstIndex = toIndex + (gapEndIndex - fGapIndex);
		count = gapEndIndex - dstIndex;
	}
	
	if (count > 0)
		memmove(fBuffer + dstIndex, fBuffer + srcIndex, count);	

	fGapIndex = toIndex;
}


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


const char *
_BTextGapBuffer_::GetString(int32 fromOffset, int32 numBytes, int32 *returnedBytes)
{
	char *result = "";
	
	if (numBytes < 1) {
		if (returnedBytes != NULL)
			*returnedBytes = 0;
		return result;
	}
	
	bool isStartBeforeGap = (fromOffset < fGapIndex);
	bool isEndBeforeGap = ((fromOffset + numBytes - 1) < fGapIndex);

	if (isStartBeforeGap == isEndBeforeGap) {
		result = fBuffer + fromOffset;
		if (!isStartBeforeGap)
			result += fGapCount;
	
	} else {
		if (fScratchSize < numBytes) {
			fScratchBuffer = (char *)realloc(fScratchBuffer, numBytes);
			fScratchSize = numBytes;
		}
		
		for (long i = 0; i < numBytes; i++)
			fScratchBuffer[i] = (*this)[fromOffset + i];

		result = fScratchBuffer;
	}

	if (returnedBytes != NULL)
		*returnedBytes = numBytes;

	// TODO: this could be improved. We are overwriting what we did some lines ago,
	// we could just avoid to do that.
	if (fPasswordMode) {
		uint32 numChars = UTF8CountChars(result, numBytes);
		uint32 charLen = UTF8CountBytes(B_UTF8_BULLET, 1);
		uint32 newSize = numChars * charLen + 1;
		if ((uint32)fScratchSize < newSize) {
			fScratchBuffer = (char *)realloc(fScratchBuffer, newSize);
			fScratchSize = newSize;
		}
		result = fScratchBuffer;

		char *scratchPtr = result;
		for (uint32 i = 0; i < numChars; i++) {
			memcpy(scratchPtr, B_UTF8_BULLET, charLen);
			scratchPtr += charLen;
		}
		scratchPtr = '\0';	
		if (returnedBytes != NULL)
			*returnedBytes = newSize - 1;	
	}

	return result;
}


bool 
_BTextGapBuffer_::FindChar(char inChar, long fromIndex, long *ioDelta)
{
	long numChars = *ioDelta;
	for (long i = 0; i < numChars; i++) {
		if (((*this)[fromIndex + i] & 0xc0) == 0x80)
			continue;
		if ((*this)[fromIndex + i] == inChar) {
			*ioDelta = i;
			return true;
		}
	}
	
	return false;
}


const char *
_BTextGapBuffer_::Text()
{
	MoveGapTo(fItemCount);
	fBuffer[fItemCount] = '\0';
	
	return fBuffer;
}


/*char *
_BTextGapBuffer_::RealText()
{
	return fText;
}*/


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


char 
_BTextGapBuffer_::RealCharAt(int32 offset) const
{
	return (offset < fGapIndex) ? fBuffer[offset] : fBuffer[offset + fGapCount];
}


bool
_BTextGapBuffer_::PasswordMode() const
{
	return fPasswordMode;
}


void
_BTextGapBuffer_::SetPasswordMode(bool state)
{
	fPasswordMode = state;
}
