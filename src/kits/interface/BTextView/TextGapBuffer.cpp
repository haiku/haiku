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
//	File Name:		TextGapBuffer.cpp
//	Author:			Marc Flerackers (mflerackers@androme.be)
//	Description:	Text storage used by BTextView
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <string.h>

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "TextGapBuffer.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
_BTextGapBuffer_::_BTextGapBuffer_()
	:	fText(NULL),
		fLogicalBytes(0),
		fPhysicalBytes(2048)
{
	fText = new char[fPhysicalBytes];
	*fText = '\0';
}
//------------------------------------------------------------------------------
_BTextGapBuffer_::~_BTextGapBuffer_()
{
	delete[] fText;
}
//------------------------------------------------------------------------------
void _BTextGapBuffer_::InsertText(const char *text, int32 length, int32 offset)
{
	// If needed, resize buffer
	if (fPhysicalBytes < fLogicalBytes + length + 1)
		Resize(fLogicalBytes + length + 1);

	// Move text after insertion point
	memcpy(fText + offset + length, fText + offset,
		fLogicalBytes + 1 - offset);

	// Copy new text
	memcpy(fText + offset, text, length);

	// Update used bytes
	fLogicalBytes += length;
}
//------------------------------------------------------------------------------
void _BTextGapBuffer_::RemoveRange(int32 from, int32 to)
{
	// Move text after deletion point
	memcpy(fText + from, fText + to, fLogicalBytes + 1 - to);

	// Update used bytes
	fLogicalBytes -= to - from;
}
//------------------------------------------------------------------------------
char *_BTextGapBuffer_::Text()
{
	if (fLogicalBytes == 0 || fText[fLogicalBytes - 1] != '\0')
	{
		if (fPhysicalBytes < fLogicalBytes + 1)
		{
			char *new_text = new char[fLogicalBytes + 1];

			if (fText)
			{
				memcpy(new_text, fText, fLogicalBytes );
				delete fText;
			}

			fText = new_text;
		}

		fText[fLogicalBytes] = '\0';
	}

	return fText;
}
//------------------------------------------------------------------------------
char *_BTextGapBuffer_::RealText()
{
	return fText;
}
//------------------------------------------------------------------------------
char _BTextGapBuffer_::RealCharAt(int32 offset) const
{
	return *(fText + offset);
}
//------------------------------------------------------------------------------
void _BTextGapBuffer_::Resize(int32 size)
{
	if (fPhysicalBytes < size)
	{
		char *text = new char[size];

		if (fText)
		{
			memcpy(text, fText, fLogicalBytes);
			delete fText;
		}

		fText = text;
		fPhysicalBytes = size;
	}
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */
