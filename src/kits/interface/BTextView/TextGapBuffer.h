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
//	File Name:		TextGapBuffer.h
//	Author:			Marc Flerackers (mflerackers@androme.be)
//	Description:	Text storage used by BTextView
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include "SupportDefs.h"

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

// _BTextGapBuffer_ class ------------------------------------------------------
class _BTextGapBuffer_ {

public:
				_BTextGapBuffer_();
virtual			~_BTextGapBuffer_();

		void	InsertText(const char *text, int32 length, int32 offset);
		void	RemoveRange(int32 from, int32 to);

		char 	*Text();
		char 	*RealText();
		void	GetString(int32 offset, int32 length, char *buffer);
		void	GetString(int32, int32 *);
		
		char	RealCharAt(int32 offset) const;
		bool	FindChar(char c, int32 inOffset, int32 *outOffset);
				
		void	SizeGapTo(int32);
		void	MoveGapTo(int32);
		void	InsertText(BFile *, int32, int32, int32); 
		bool	PasswordMode() const;
		void	SetPasswordMode(bool);

		void	Resize(int32 size);

		char	*fText;
		int32	fLogicalBytes;
		int32	fPhysicalBytes;
};
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */
