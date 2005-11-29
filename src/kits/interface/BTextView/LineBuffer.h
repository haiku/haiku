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
//	File Name:		LineBuffer.h
//	Author:			Marc Flerackers (mflerackers@androme.be)
//	Description:	Line storage used by BTextView
//------------------------------------------------------------------------------

#include <SupportDefs.h>
#include "TextViewSupportBuffer.h"

typedef struct STELine {
	long			offset;		// offset of first character of line
	float			origin;		// pixel position of top of line
	float			ascent;		// maximum ascent for line
	float			width;		// not used for now, but could be
} STELine;


// _BLineBuffer_ class ---------------------------------------------------------
class _BLineBuffer_ : public _BTextViewSupportBuffer_<STELine> {

public:
						_BLineBuffer_();
virtual					~_BLineBuffer_();

		void			InsertLine(STELine *inLine, int32 index);
		void			RemoveLines(int32 index, int32 count = 1);
		void			RemoveLineRange(int32 fromOffset, int32 toOffset);

		int32			OffsetToLine(int32 offset) const;
		int32			PixelToLine(float pixel) const;
			
		void			BumpOrigin(float delta, int32 index);
		void			BumpOffset(int32 delta, int32 index);

		long			NumLines() const;
		STELine *		operator[](int32 index) const;
};


inline int32
_BLineBuffer_::NumLines() const
{
	return fItemCount - 1;
}


inline STELine *
_BLineBuffer_::operator[](int32 index) const
{
	return &fBuffer[index];
}
