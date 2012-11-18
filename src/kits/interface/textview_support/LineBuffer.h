/*
 * Copyright 2001-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 */
#ifndef __LINE_BUFFER_H
#define __LINE_BUFFER_H


#include <SupportDefs.h>
#include <TextView.h>

#include "TextViewSupportBuffer.h"

struct STELine {
	long		offset;		// offset of first character of line
	float		origin;		// pixel position of top of line
	float		ascent;		// maximum ascent for line
	float		width;		// cached width of line in pixels
};


class BTextView::LineBuffer : public _BTextViewSupportBuffer_<STELine> {

public:
								LineBuffer();
	virtual						~LineBuffer();

			void				InsertLine(STELine* inLine, int32 index);
			void				RemoveLines(int32 index, int32 count = 1);
			void				RemoveLineRange(int32 fromOffset,
									int32 toOffset);

			int32				OffsetToLine(int32 offset) const;
			int32				PixelToLine(float pixel) const;

			void				BumpOrigin(float delta, int32 index);
			void				BumpOffset(int32 delta, int32 index);

			int32				NumLines() const;
			float				MaxWidth() const;
			STELine*			operator[](int32 index) const;
};


inline int32
BTextView::LineBuffer::NumLines() const
{
	return fItemCount - 1;
}


inline STELine *
BTextView::LineBuffer::operator[](int32 index) const
{
	return &fBuffer[index];
}


#endif	// __LINE_BUFFER_H
