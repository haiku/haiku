//------------------------------------------------------------------------------
//	Copyright (c) 2001-2004, Haiku
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
//	File Name:		TextGapBuffer.h
//	Author:			Marc Flerackers (mflerackers@androme.be)
//	Description:	Text storage used by BTextView
//------------------------------------------------------------------------------
#ifndef __TEXTGAPBUFFER_H
#define __TEXTGAPBUFFER_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <SupportDefs.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class BFile;
// _BTextGapBuffer_ class ------------------------------------------------------
class _BTextGapBuffer_ {

public:
					_BTextGapBuffer_();
virtual				~_BTextGapBuffer_();

		void		InsertText(const char *inText, int32 inNumItems, int32 inAtIndex);
		void		InsertText(BFile *file, int32 fileOffset, int32 amount, int32 atIndex);
		void		RemoveRange(int32 start, int32 end);
		
		void		MoveGapTo(int32 toIndex);
		void		SizeGapTo(int32 inCount);

		const char *GetString(int32 fromOffset, int32 numChars);
		bool		FindChar(char inChar, int32 fromIndex, int32 *ioDelta);

		const char *Text();
		int32		Length() const;
		char		operator[](int32 index) const;

		
//		char 	*RealText();
		void	GetString(int32 offset, int32 length, char *buffer);
//		void	GetString(int32, int32 *);
		
		char	RealCharAt(int32 offset) const;
				
		bool	PasswordMode() const;
		void	SetPasswordMode(bool);

//		void	Resize(int32 size);

protected:
		int32	fExtraCount;		// when realloc()-ing
		int32	fItemCount;			// logical count
		char	*fBuffer;			// allocated memory
		int32	fBufferCount;		// physical count
		int32	fGapIndex;			// gap position
		int32	fGapCount;			// gap count
		char	*fScratchBuffer;	// for GetString
		int32	fScratchSize;		// scratch size
		bool	fPasswordMode;
};
//------------------------------------------------------------------------------
inline int32 
_BTextGapBuffer_::Length() const
{
	return fItemCount;
}
//------------------------------------------------------------------------------
inline char 
_BTextGapBuffer_::operator[](long index) const
{
	return (index < fGapIndex) ? fBuffer[index] : fBuffer[index + fGapCount];
}

#endif //__TEXTGAPBUFFER_H
/*
 * $Log $
 *
 * $Id  $
 *
 */
